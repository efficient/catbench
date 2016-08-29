#!/usr/bin/python

import argparse;
import os;
import sys;
import matplotlib as mpl;

mpl.use('Agg');
import matplotlib.pyplot as plt;
import json;
from matplotlib import ticker;
import numpy as np

from graph_helper import get_tuples;
from graph_helper import get_sample_description;
from graph_helper import get_label;
from graph_helper import get_arg_label;
from graph_helper import get_arg_unit;
from graph_helper import get_aux;
from graph_helper import get_commit;
from graph_helper import get_series_aux;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='datafile',
                        help='input json');
    parser.add_argument('--series', '-s', nargs='+', dest='series',
                        help='series label(s). Single label or space separated list');
    parser.add_argument('--xdata', dest='x',
                        help='X axis label');
    parser.add_argument('--ydata', nargs='+', dest='y',
                        help='Y axis data labels. Single label or space separated list');
    parser.add_argument('--title', '-t', dest='title',
                        help='Graph title');
    parser.add_argument('--outfile', '-o', dest='outfile', default="graph.png",
                        help='Output filename');
    parser.add_argument('--no-commit', '-n', dest='no_commit_message', action='store_true', default=False,
                        help="Do not include commit number on graph");
    parser.add_argument('--legend-x', dest='legend_x', type=float, default=1.0,
                        help="Legend box location x coordinate (default 1.0)");
    parser.add_argument('--legend-y', dest='legend_y', type=float, default=0.3,
                        help="Legend boy location y coordinate (default 0.3)");
    args = parser.parse_args();
    return args.datafile, args.series, args.x, args.y, args.title, args.outfile, args.no_commit_message, args.legend_x, args.legend_y;

def graph_bar(infile, series, x, y, title, outfile, nocommit, legend_x, legend_y):
    width = 1;
    tuples = get_tuples(infile, series, x, y);
    yvar = y[0];
    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);
    ax.set_xlabel(get_label(infile, x));
    ax.set_ylabel(get_label(infile, y[0]));
    ax.ticklabel_format(useOffset=False);
    colors = [
    	'b', 'g', 'r', 'm' ,'y', 'k'
    ]
    import itertools;
    marker = itertools.cycle(('o', '>', 'D', 's', 'h', '+', '<', '^'));
    color = itertools.cycle(('b', 'g', 'r', 'm' ,'y', 'k'));

    box = ax.get_position();
    ax.set_position([box.x0, box.y0, box.width, box.height * 0.7]);

    lines = list();
    for series_label, fields in tuples.items():
        line = list();
        for field, tuples in fields.items():
            if(field == "description" or field == "order"):
                continue;
            points = map(list, zip(*tuples));
            line_label = fields["description"] + " " + y[0];
            line.append((ax.bar(points[0], points[1], width, align='center', color=color.next(), label=line_label)));
            lines.append((line[-1][0], fields["order"], line_label));
	plt.xticks(points[0]);

    ax.set_title(title);
    ax.title.set_position((0.5, 1.08));

    handles, labels = ax.get_legend_handles_labels()

    import operator
    handles2 = None;
    labels2 = None;
    h = sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*h)
    lgd = ax.legend(handles2, labels2, loc="center right", bbox_to_anchor=(legend_x, legend_y));

    from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
    ax.xaxis.set_major_formatter(FormatStrFormatter('%.0f'))
    #fig.savefig(outfile, format='png', dpi=600, bbox_extra_artists=(lgd,), bbox_inches='tight');
    plt.xlim(xmin=0);
    fig.savefig(outfile, format='png', dpi=600, bbox_inches='tight');

def main():
    infile, series, x, y, title, outfile, nocommit, legend_x, legend_y = setup_optparse();
    graph_bar(infile, series, x, y, title, outfile, nocommit, legend_x, legend_y);

main();
