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
    parser.add_argument('--input-left', '-l', dest='left_datafile',
                        help='input json for left side y axis');
    parser.add_argument('--input-right', '-r', dest='right_datafile',
                        help='input json for right side y axis');
    parser.add_argument('--series', '-s', nargs='+', dest='series',
                        help='series label(s). Single label or space separated list');
    parser.add_argument('--xdata-left', dest='x_left',
                        help='X axis label');
    parser.add_argument('--ydata-left', nargs='+', dest='y_left',
                        help='Y axis data labels. Single label or space separated list');
    parser.add_argument('--xdata-right', dest='x_right',
                        help='X axis label');
    parser.add_argument('--ydata-right', nargs='+', dest='y_right',
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
    return args.left_datafile, args.right_datafile, args.series, args.x_left, args.y_left, args.x_right, args.y_right,args.title, args.outfile, args.no_commit_message, args.legend_x, args.legend_y;

def graph_double(left, right, series, x_left, y_left, x_right, y_right, title, outfile, nocommit, legend_x, legend_y):
    left_tuples = get_tuples(left, series, x_left, y_left);
    right_tuples = get_tuples(right, series, x_right, y_right);
    left_yvar = y_left[0];
    right_yvar = y_right[0];
    fig = plt.figure();
    axl = fig.add_subplot(1,1,1);
    axr = axl.twinx();
    axl.set_xlabel(get_label(left, x_left));
    axl.set_ylabel(get_label(left, y_left[0]));
    axr.set_ylabel(get_label(right, y_right[0]));
    axl.ticklabel_format(useOffset=False);
    #axr.ticklabel_format(useOffset=False);
    colors = [
    	'b', 'g', 'r', 'm' ,'y', 'k'
    ]
    import itertools;
    marker = itertools.cycle(('o', '>', 'D', 's', 'h', '+', '<', '^'));
    color = itertools.cycle(('b', 'g', 'r', 'm' ,'y', 'k'));

    box = axl.get_position();
    axl.set_position([box.x0, box.y0, box.width, box.height * 0.7]);
    box = axr.get_position();
    axr.set_position([box.x0, box.y0, box.width, box.height * 0.7]);

    left_lines = list();
    for series_label, fields in left_tuples.items():
        line = list();
        for field, tuples in fields.items():
            if(field == "description" or field == "order"):
                continue;
            points = map(list, zip(*tuples));
            line_label = fields["description"] + " " + y_left[0];
            line.append((axl.plot(points[0], points[1], '-o', color=color.next(), label=line_label, marker=marker.next())));
            left_lines.append((line[-1][0], fields["order"], line_label));
    right_lines = list();
    for series_label, fields in right_tuples.items():
        line = list();
        for field, tuples in fields.items():
            if(field == "description" or field == "order"):
                continue;
            points = map(list, zip(*tuples));
            line_label = fields["description"] + " " + y_right[0];
            line.append((axr.plot(points[0], points[1], '-o', color=color.next(), label=line_label, marker=marker.next())));
            right_lines.append((line[-1][0], fields["order"], line_label));

    axl.set_title(title);
    axl.title.set_position((0.5, 1.08));

    handles, labels = axl.get_legend_handles_labels()

    import operator
    handles2 = None;
    labels2 = None;
    hl = sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    lgdl = axl.legend(handles2, labels2, loc="center right", bbox_to_anchor=(legend_x, legend_y));

    handles, labels = axr.get_legend_handles_labels()
    import operator
    handles2 = None;
    labels2 = None;
    hl = sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    lgdr = axr.legend(handles2, labels2, loc="center left", bbox_to_anchor=(legend_x, legend_y));

    from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
    axl.xaxis.set_major_formatter(FormatStrFormatter('%.2f'))
    #fig.savefig(outfile, format='png', dpi=600, bbox_extra_artists=(lgd,), bbox_inches='tight');
    plt.xlim(xmin=0);
    plt.ylim(ymin=0);
    axl.set_ylim(0);
    axr.set_ylim(0);
    fig.savefig(outfile, format='png', dpi=600, bbox_inches='tight');

def main():
    left, right, series, x_left, y_left, x_right, y_right, title, outfile, nocommit, legend_x, legend_y = setup_optparse();
    graph_double(left, right, series, x_left, y_left, x_right, y_right, title, outfile, nocommit, legend_x, legend_y);

main();
