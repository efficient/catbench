#!/usr/bin/python

import argparse;
import os;
import sys;
import matplotlib as mpl;
mpl.use('Agg');
import matplotlib.pyplot as plt;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--datafile', dest='datafile',
                        help='input csv with header');
    args = parser.parse_args();
    return args.datafile;

def graph(filename):
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

    plt.legend([line1, line2, line3, line4],loc="upper center", bbox_to_anchor=(0.5,1.5));

    plt.ylim(ymin=0);
    fig.savefig('graph.png');
filename = setup_optparse();
graph(filename);
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
