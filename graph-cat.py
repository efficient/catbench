#!/usr/bin/python

import argparse;
import os;
import sys;
import matplotlib as mpl;
mpl.use('Agg');
import matplotlib.pyplot as plt;
import json;
from matplotlib import ticker;
import numpy as np;

from graph_helper import get_tuples;
from graph_helper import get_sample_description;
from graph_helper import get_label;
from graph_helper import get_arg_label;
from graph_helper import get_arg_unit;
from graph_helper import get_aux;
from graph_helper import get_commit;
from graph_helper import get_series_aux;
from graph_helper import color;
from graph_helper import color_alpha;
from graph_helper import marker;
from graph_helper import marker_size;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='datafile',
                        help='input json');
    parser.add_argument('--series', '-s', nargs='+', dest='series_labels',
                        help='series label(s). Single label or space separated list');
    parser.add_argument('--xdata', '-x', dest='x_label',
                        help='X axis label');
    parser.add_argument('--ydata', '-y', nargs='+', dest='y_labels',
                        help='Y axis data labels. Single label or space separated list');
    parser.add_argument('--include', nargs='+', dest='include_labels',
                        help='Include these labels when summarizing information');
    parser.add_argument('--title', '-t', dest='title',
                        help='Graph title');
    parser.add_argument('--outfile', '-o', dest='outfile', default="graph.png",
                        help='Output filename');
    parser.add_argument('--fit', '-f', dest='fit', default=False);
    parser.add_argument('--ymin', dest='ymin', default=None,
                        help="Y axis minimum. Default 0");
    parser.add_argument('--ymax', dest='ymax', default=None,
                        help="Y axis maximum. Default 1.75 * the max y value found in the data");
    parser.add_argument('--no-commit', '-n', dest='no_commit_message', action='store_true', default=False,
                        help="Do not include commit number on graph");
    parser.add_argument('--log-y', dest='logy', action='store_true', default=False);
    parser.add_argument('--log-x', dest='logx', action='store_true', default=False);
    parser.add_argument('--cdf', dest='cdf', action='store_true', default=False);
    parser.add_argument('--legend-x', dest='legend_x', type=float, default=1.0,
                        help="Legend box location x coordinate (default 1.0)");
    parser.add_argument('--legend-y', dest='legend_y', type=float, default=0.3,
                        help="Legend boy location y coordinate (default 0.3)");
    parser.add_argument('--grid-y', dest='grid_y', action='store_true', default=False);
    parser.add_argument('--smart-x', dest='smart_x', action='store_true', default=False,
                        help="Put x ticks only when there is a data point for that x value");
    parser.add_argument('--nosort', dest='nosort', action='store_true', default=False,
                        help="Do not alphabetically sort legend entries, preserve input order");
    parser.add_argument('--xmax', dest='xmax', type=float, default=0,
                        help="Maximum x point to graph");
    parser.add_argument('--xmin', dest='xmin', type=float, default=None,
                        help="Minimum x point to graph");
    args = parser.parse_args();
    if(type(args.series_labels) != list):
        args.series_labels = [args.series_labels];
    if(args.ymin != None):
        args.ymin = float(args.ymin)
    if(args.ymax != None):
        args.ymax = float(args.ymax)
    return args.datafile, args.series_labels, args.x_label, args.y_labels, args.include_labels, args.title, args.outfile, args.fit, args.ymin, args.ymax, args.no_commit_message, args.logx, args.logy, args.cdf, args.legend_x, args.legend_y, args.grid_y, args.smart_x, args.nosort, args.xmax, args.xmin;

def graph(filename, slabels, xlabel, ylabels, ilabels, title, outfile, fit, user_ymin, user_ymax, no_commit_message, logx, logy, legend_x, legend_y, grid_y, smart_x, nosort, xmax, xmin):
    series_tuples = get_tuples(filename, slabels, xlabel, ylabels);

    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);
    ax.ticklabel_format(useOffset=False);

    #ax.set_color_cycle(colors);

    ax.set_xlabel(get_label(filename, xlabel));
    ax.set_ylabel(get_label(filename, ylabels[0]));
    ax.ticklabel_format(useOffset=False)

    if(grid_y == True):
        ax.yaxis.grid(True);


    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width, box.height * 0.7])

    aux_text = "";
    if(no_commit_message == False):
        aux_text += get_commit(filename);
    for slabel in slabels:
        aux_text += get_series_aux(filename, slabel, ilabels);
        aux_text += "\n";

    ax.text(0.05, 0.95, aux_text, verticalalignment='top', transform=ax.transAxes);

    cur_ymax=0;
    cur_xmax=0;
    x_copy=None;
    temp = list();
    for key, val in series_tuples.items():
        line = list();
        for key2, val2 in val.items():
            if(key2 == "description" or key2 == "order"):
                continue;
            if(xmin == None or (xmax == 0 and xmin == 0)):
	        val2_cropped = val2;
            else:
	        val2_cropped = filter(lambda x: x[0] <= xmax and x[0] >= xmin, val2);
            xy = map(list, zip(*val2_cropped));
            line_label = val["description"];
            line.append((ax.plot(xy[0], xy[1], '-o', color=color.next(), label=line_label, marker=marker.next(), markersize = marker_size, alpha=color_alpha))); #+ str(key2[1])))[0]);
	    #ax.plot(xy[0], xy[1]);
            temp.append((line[len(line) - 1][0], val["order"], line_label));
            #ax.scatter(xy[0], xy[1]);
        if(x_copy == None):
	    x_copy = xy[0];
        x_copy = list(set(x_copy + xy[0]));
        if(max(xy[0]) > cur_xmax):
            cur_xmax = max(xy[0]);
        if(max(xy[1]) > cur_ymax):
            cur_ymax = max(xy[1]);

    if(logx):
        ax.set_xscale('log');
    if(logy):
        ax.set_yscale('log');
    min_dist = (max(x_copy) - min(x_copy)) / 50;
    idx = 0;
    x_copy.sort();
    while(idx + 1 < len(x_copy)):
        if(x_copy[idx] + min_dist > x_copy[idx+1]):
            x_copy.pop(idx+1);
	    idx = 0;
            continue;
        idx += 1;
    if(logx == logy == smart_x == False):
        plt.xticks(x_copy);
    ax.set_title(title);
    ax.title.set_position((0.5, 1.08));

    handles, labels = ax.get_legend_handles_labels()

    import operator
    handles2 = None;
    labels2 = None;
    if(nosort):
        idx = 0;
        hl = sorted(temp, key=operator.itemgetter(1));
        handles2, trash, labels2 = zip(*hl);
    else:
        # or sort them by labels
        hl = sorted(zip(handles, labels), key=operator.itemgetter(1))
        handles2, labels2 = zip(*hl)
    lgd = ax.legend(handles2, labels2, loc="center right", bbox_to_anchor=(legend_x, legend_y));

    cur_ymax = cur_ymax * 1.75;
    if(xmin != None):
    	plt.xlim(xmin=xmin);
    else:
    	plt.xlim(xmin=min(x_copy));
    plt.setp(ax.get_xticklabels(), fontsize=10, rotation=90)
    if(logy == False):
        if(user_ymin == None and fit == False and user_ymax == None):
            plt.ylim(ymin=0,ymax=cur_ymax);
        elif(fit == False and user_ymin == None):
            plt.ylim(ymin=0,ymax=user_ymax);
        elif(user_ymax == None and fit == False):
            plt.ylim(ymin=float(user_ymin), ymax=float(cur_ymax));
        elif(fit == False):
            plt.ylim(ymin=float(user_ymin), ymax=float(user_ymax));
    if(logy == True):
        yticks = list();
        cur_tick = 1;
        while(cur_tick < cur_ymax * 10):
            yticks.append(cur_tick);
            cur_tick = cur_tick * 10;
        plt.yticks(yticks);
    if(logx == True):
        xticks = list();
        cur_tick = 1;
        while(cur_tick < cur_xmax * 10):
            xticks.append(cur_tick);
            cur_tick = cur_tick * 10;
        plt.xticks(xticks);
    from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
    plt.savefig(outfile, bbox_extra_artists=(lgd,), bbox_inches='tight');

def main():
    filename, slabels, xlabel, ylabels, ilabels, title, outfile, fit, ymin, ymax, no_commit_message, logx, logy, cdf, legend_x, legend_y, grid_y, smart_x, nosort, xmax, xmin = setup_optparse();
    if(cdf == False):
        graph(filename, slabels, xlabel, ylabels, ilabels, title, outfile, fit, ymin, ymax, no_commit_message, logx, logy, legend_x, legend_y, grid_y, smart_x, nosort, xmax, xmin);

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
