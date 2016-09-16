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

filename="cost-analysis-result.pdf";

def graph():
    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);

    temp = list();
    range_top = 2000;
    t_total = np.arange(0, range_top, 1);
    temp.append(ax.plot(t_total, t_total, 'm:', linewidth=4.0, label="No colocation"));

    #series_tuples = get_tuples(filename, slabels, xlabel, ylabels);
    em = 0.95;
    ek = 0.75;
    n = 1000;
    range_bottom = n / ek * em;

    #range_top = range_bottom + 1000;
    t_bottom = np.arange(0, range_bottom, 1);
    t_top = np.arange(range_bottom, range_top, 1);
    temp.append(ax.plot(t_bottom, t_bottom / em - (t_bottom / em) * ek, 'r--', linewidth=2.0, label="No CAT"));
    temp.append(ax.plot(t_top, (n / ek - n) + (t_top - (n / ek) * em), 'r--', linewidth=2.0));#, label="No CAT"));

    em = 0.95;
    ek = 0.99;
    range_bottom = n / ek * em;
    #range_top = range_bottom + 1000;
    t_bottom = np.arange(0, range_bottom, 1);
    t_top = np.arange(range_bottom, range_top, 1);
    temp.append(ax.plot(t_bottom, t_bottom / em - (t_bottom / em) * ek, 'b-', linewidth=2.0, label="With CAT"));
    temp.append(ax.plot(t_top, (n / ek - n) + (t_top - (n / ek) * em), 'b-', linewidth=2.0));#, label="With CAT"));

    handles, labels = ax.get_legend_handles_labels()
    import operator
    handles2 = None;
    labels2 = None;

    hl = zip(handles,labels);#sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    lgd = ax.legend(handles2, labels2, loc="upper left");

    ax.set_xlabel("Machine learning throughput");
    ax.set_ylabel("Number of extra machines");

    plt.savefig(filename, bbox_extra_artists=(lgd,), bbox_inches='tight');
    exit();

def main():
    graph();

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data