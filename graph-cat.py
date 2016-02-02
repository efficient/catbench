#!/usr/bin/python

import argparse;
import os;
import sys;
import matplotlib as mpl;
mpl.use('Agg');
import matplotlib.pyplot as plt;

def setup_optparse():
    parser = argparse.ArgumentParser(description='Example: ./graph-cat.py -i file.csv --series 0 --xdata 1 --ydata 2 3 --ignore 4 5 --title Graph --ylabel Execution time (s)');
    parser.add_argument('--input', '-i', dest='datafile',
                        help='input csv with header');
    parser.add_argument('--series', '-s', type=int, dest='series_columns',
                        help='series column');
    parser.add_argument('--xdata', '-x', type=int, dest='x_column',
                        help='X axis coordinates, single column');
    parser.add_argument('--ydata', '-y', type=int, nargs='+', dest='y_columns',
                        help='Y axis data columns. Enter single number or list (-s 1 2 3)');
    parser.add_argument('--ignore', nargs='+', type=int, dest='ignore_columns',
                        help='Ignore these columns when summarizing information. Enter list (--ignore 1 2 3');
    parser.add_argument('--title', '-t', dest='title',
                        help='Graph title');
    parser.add_argument('--ylabel', dest='ylabel',
                        help='Y axis label');
    parser.add_argument('--outfile', '-o', dest='outfile', default="graph.png",
                        help='Output filename');
    args = parser.parse_args();
    return args.datafile, args.series_columns, args.x_column, args.y_columns, args.ignore_columns, args.title, args.ylabel, args.outfile;

def graph(filename, scol, xcol, ycols, icols):
    xpoints = list();
    series50 = list();
    series50cat = list();
    series100 = list();
    series100cat = list();

    fd = open(filename, 'r');
    lines = fd.read().splitlines();
    lines.pop(0);
    count = 0;
    for line in lines:
        cols = line.split(',');
        if(count % 2 == 0):
            xpoints.append(cols[0]);
        if(cols[1] == "50"):
            series50cat.append(cols[2]);
            series50.append(cols[3]);
        if(cols[1] == "100"):
            series100cat.append(cols[2]);
            series100.append(cols[3]);
        count = count + 1;
    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);

    ax.set_xlabel("Trash Process Memory usage (Cache Way %)");
    ax.set_ylabel("Lockserver memory access(es) latency (s)");

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width, box.height * 0.7])

    line1, = ax.plot(xpoints, series50cat, label='Lockserver 50%, with CAT');
    line2, = ax.plot(xpoints, series50, label='Lockserver 50%, without CAT');
    line3, = ax.plot(xpoints, series100cat, label='Lockserver 100%, with CAT');
    line4, = ax.plot(xpoints, series100, label='Lockserver 100%, without CAT');

    ax.set_title("Time Taken for Lockserver to complete 300m Memory Accesses");
    plt.legend([line1, line2, line3, line4],loc="upper center", bbox_to_anchor=(0.5,1.5));

    plt.ylim(ymin=0);
    fig.savefig('graph.png');

def get_columns(comma_sep_line):
    return comma_sep_line.split(',');

def graph2(filename, scol, xcol, ycols, icols, title, ylabel, outfile):
    # First sort into Y buckets
    ylist = dict();

    fd = open(filename, 'r');
    lines = fd.read().splitlines();
    count = 0;
# TODO ignore top metadata
    line = lines[0];
    headers = get_columns(line);
    scol_empty = scol == None;
# If no series, add a dummy series
    if(scol_empty):
        headers.append("");
        scol = len(headers) - 1;
    ycol_def = 0;
# Break header line into columns:
    for ycol in ycols:
# Just grab any y column as a default item
        ycol_def = ycol;
        # Add a new bucket for each Y column
        ylist[headers[ycol]] = dict();
    lines.pop(0);
    # Scan through and grab all of the series labels
    for line in lines:
        cols = get_columns(line);
        if(scol_empty):
            cols.append("");
        scol_val = cols[scol];
# Val is the list that represents this Y data column
# Add lists for each series
        for key, val in ylist.items():
            if(not (scol_val in val)):
                val[scol_val] = list();
# Scan through and add all (x, y) tuples
    for line in lines:
        cols = get_columns(line);
        if(scol_empty):
            cols.append("");
        scol_val = cols[scol];
        for ycol in ycols:
            assert(scol_val in ylist[headers[ycol]]);
            ylist[headers[ycol]][scol_val].append((cols[xcol], cols[ycol]));
    for key, val in ylist.items():
        print(key);
        print(val);
        print("");
    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);

    ax.set_xlabel(headers[xcol]);
    ax.set_ylabel(ylabel);

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width, box.height * 0.7])

    for ycol_header, y in ylist.items():
        line = list();
        for series_value, series in y.items():
            xy = map(list, zip(*series));
            if(scol_empty):
                line_label = ycol_header;
            else:
                line_label = ycol_header + ":" + headers[scol] + " = " + series_value;
            line.append((ax.plot(xy[0], xy[1], label=line_label))[0]);
    ax.set_title(title);
    plt.legend(loc="upper center", bbox_to_anchor=(0.5,1.5));

    plt.ylim(ymin=0);
    fig.savefig(outfile);


def main():
    filename, scol, xcol, ycols, icols, title, ylabel, outfile = setup_optparse();
    graph2(filename, scol, xcol, ycols, icols, title, ylabel, outfile);
    #graph(filename);

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
