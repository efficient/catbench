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
from graph_helper import color;
from graph_helper import color_copy;
from graph_helper import color_alpha;
from graph_helper import marker;
from graph_helper import marker_copy;
from graph_helper import marker_size;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input-top', '-l', dest='left_datafile',
                        help='input json for left side y axis');
    parser.add_argument('--input-bottom', '-r', dest='right_datafile',
                        help='input json for right side y axis');
    parser.add_argument('--series', '-s', nargs='+', dest='series',
                        help='series label(s). Single label or space separated list');
    parser.add_argument('--xdata-top', dest='x_left',
                        help='X axis label');
    parser.add_argument('--ydata-top', nargs='+', dest='y_left',
                        help='Y axis data labels. Single label or space separated list');
    parser.add_argument('--xdata-bottom', dest='x_right',
                        help='X axis label');
    parser.add_argument('--ydata-bottom', nargs='+', dest='y_right',
                        help='Y axis data labels. Single label or space separated list');
    parser.add_argument('--title', '-t', dest='title',
                        help='Graph title');
    parser.add_argument('--outfile', '-o', dest='outfile', default="graph.png",
                        help='Output filename');
    parser.add_argument('--no-commit', '-n', dest='no_commit_message', action='store_true', default=False,
                        help="Do not include commit number on graph");
    parser.add_argument('--legend-xl', dest='legend_xl', type=float, default=1.0,
                        help="Legend box location x coordinate for bottom side (default 1.0)");
    parser.add_argument('--legend-yl', dest='legend_yl', type=float, default=0.3,
                        help="Legend box location y coordinate for bottom side (default 0.3)");
    parser.add_argument('--legend-xr', dest='legend_xr', type=float, default=1.0,
                        help="Legend box location x coordinate for top side (default 1.0)");
    parser.add_argument('--legend-yr', dest='legend_yr', type=float, default=0.3,
                        help="Legend box location y coordinate for top side (default 0.3)");
    parser.add_argument('--notitle', dest='no_title', action='store_true', default=False,
                        help="Pass this flag to remove title");
    parser.add_argument('--legend-loc-top', dest='loc_top', default=None,
                        help='bottom right, bottom left, upper left, upper right, ignores --legend-x and --legend-y');
    parser.add_argument('--legend-loc-bottom', dest='loc_bot', default=None,
                        help='bottom right, bottom left, upper left, upper right, ignores --legend-x and --legend-y');
    args = parser.parse_args();
    return args.left_datafile, args.right_datafile, args.series, args.x_left, args.y_left, args.x_right, args.y_right,args.title, args.outfile, args.no_commit_message, args.legend_xl, args.legend_yl, args.legend_xr, args.legend_yr, args.no_title, args.loc_top, args.loc_bot;

def get_series_sublabel(filename, key):
    fd = open(filename, 'r');
    data = json.load(fd);
    return "";
    #return data["legend"]["samples"][key]["description"];

def graph_double(left, right, series, x_left, y_left, x_right, y_right, title, outfile, nocommit, legend_xl, legend_yl, legend_xr, legend_yr, no_title, loc_top, loc_bot):
    left_tuples = get_tuples(left, series, x_left, y_left);
    right_tuples = get_tuples(right, series, x_right, y_right);
    left_yvar = y_left[0];
    right_yvar = y_right[0];
    axl = plt.subplot(212);
    axr = plt.subplot(211, sharex=axl);
    plt.setp(axr.get_xticklabels(), visible=False);
    axl.set_xlabel(get_label(left, x_left));
    axl.set_ylabel(get_label(left, y_left[0]));
    axr.set_ylabel(get_label(right, y_right[0]));
    axl.ticklabel_format(useOffset=False);
    #axr.ticklabel_format(useOffset=False);

    box = axl.get_position();
    #axl.set_position([box.x0, box.y0, box.width, box.height * 0.7]);
    box = axr.get_position();
    #axr.set_position([box.x0, box.y0, box.width, box.height * 0.7]);

    left_lines = list();
    x_copy=None;
    for series_label, fields in left_tuples.items():
        line = list();
        for field, tuples in fields.items():
            if(field == "description" or field == "order"):
                continue;
            points = map(list, zip(*tuples));
            line_label = fields["description"] + " " + get_series_sublabel(left, y_left[0]);
            line.append((axl.plot(points[0], points[1], '-o', color=color.next(), label=line_label, marker=marker.next())));
            left_lines.append((line[-1][0], fields["order"], line_label));
    right_lines = list();
    for series_label, fields in right_tuples.items():
        line = list();
        for field, tuples in fields.items():
            if(field == "description" or field == "order"):
                continue;
            points = map(list, zip(*tuples));
            line_label = fields["description"] + " " + get_series_sublabel(right, y_right[0]);
            line.append((axr.plot(points[0], points[1], '-o', color=color_copy.next(), label=line_label, marker=marker_copy.next())));
            right_lines.append((line[-1][0], fields["order"], line_label));

    if(no_title == False):
        axr.set_title(title);
        axr.title.set_position((0.5, 1.08));
    import matplotlib.ticker as plticker;

    loc = plticker.MultipleLocator(base=1.0); # this locator puts ticks at regular intervals
    axr.xaxis.set_major_locator(loc);
    axl.xaxis.set_major_locator(loc);

    handles, labels = axl.get_legend_handles_labels()
    import operator
    handles2 = None;
    labels2 = None;
    hl = sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    if(loc_bot is None):
        lgdl = axl.legend(handles2, labels2, loc="lower right", bbox_to_anchor=(legend_xl, legend_yl));
    else:
        lgdl = axl.legend(handles2, labels2, loc=loc_bot);

    from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
    plt.ylim(ymin=0);
    axl.set_ylim(0);
    axr.set_ylim(0);
    plt.savefig(outfile, dpi=600, bbox_inches='tight');

def main():
    left, right, series, x_left, y_left, x_right, y_right, title, outfile, nocommit, legend_xl, legend_yl, legend_xr, legend_yr, no_title, loc_top, loc_bot = setup_optparse();
    graph_double(left, right, series, x_left, y_left, x_right, y_right, title, outfile, nocommit, legend_xl, legend_yl, legend_xr, legend_yr, no_title, loc_top, loc_bot);

main();
