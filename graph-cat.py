#!/usr/bin/python

import argparse;
import os;
import sys;
import matplotlib as mpl;
mpl.use('Agg');
import matplotlib.pyplot as plt;
import json;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='datafile',
                        help='input csv with header');
    parser.add_argument('--series', '-s', nargs='+', dest='series_labels',
                        help='series label(s). Enter single key or multiple keys');
    parser.add_argument('--xdata', '-x', dest='x_label',
                        help='X axis label');
    parser.add_argument('--ydata', '-y', nargs='+', dest='y_labels',
                        help='Y axis data labels. Single label or space separated list');
    parser.add_argument('--ignore', nargs='+', dest='ignore_labels',
                        help='Ignore these labels when summarizing information');
    parser.add_argument('--title', '-t', dest='title',
                        help='Graph title');
    parser.add_argument('--outfile', '-o', dest='outfile', default="graph.png",
                        help='Output filename');
    args = parser.parse_args();
    if(type(args.series_labels) != list):
        args.series_labels = [args.series_labels];
    return args.datafile, args.series_labels, args.x_label, args.y_labels, args.ignore_labels, args.title, args.outfile;

def get_tuples(filename, slabels, xlabel, ylabels):
    fd = open(filename, 'r');
    data = json.load(fd);
    series_tuples = dict();
    for slabel in slabels:
        series_tuples[slabel] = dict();
        for ylabel in ylabels:
            series_tuples[slabel][(xlabel, ylabel)] = list();
    for series in data.get("data"):
        if(series.get("name") not in series_tuples):
            continue;
        series_name = series.get("name");
        series_tuples[series_name]["description"] = series.get("description");
        for sample in series.get("samples"):
            for ylabel in ylabels:
#TODO what if there is no entry for a particular ylabel?
                series_tuples[series_name][(xlabel, ylabel)].append((float(sample.get(xlabel)), float(sample.get(ylabel))));
    return series_tuples;

def get_label(filename, labelname):
    fd = open(filename, 'r');
    database = json.load(fd);
    label_entry = database.get("legend").get("samples").get(labelname);
    return str(label_entry.get("description")) + " (" + str(label_entry.get("unit")) + ")";


def graph(filename, slabels, xlabel, ylabels, ilabels, title, outfile):
    series_tuples = get_tuples(filename, slabels, xlabel, ylabels);

    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);

    ax.set_xlabel(get_label(filename, xlabel));
    ax.set_ylabel(get_label(filename, ylabels[0]));

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width, box.height * 0.7])

    for key, val in series_tuples.items():
        line = list();
        for key2, val2 in val.items():
            if(key2 == "description"):
                continue;
            xy = map(list, zip(*val2));
            print xy;
            line.append((ax.plot(xy[0], xy[1], label=val["description"]))[0]);
    ax.set_title(title);
    plt.legend(loc="upper center", bbox_to_anchor=(0.5,1.5));

    plt.ylim(ymin=0);
    fig.savefig(outfile);


def main():
    filename, slabels, xlabel, ylabels, ilabels, title, outfile = setup_optparse();
    graph(filename, slabels, xlabel, ylabels, ilabels, title, outfile);
    #graph2(filename, scol, xcol, ycols, icols, title, ylabel, outfile);
    #get_tuples(filename, scol, xcol, ycols, icols);
    #graph(filename);

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
