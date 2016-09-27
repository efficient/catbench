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
    fig = plt.figure(figsize=(8, 4));
    ax = fig.add_subplot(1,1,1);

    temp = list();
    range_top = 2000;
    t_total = np.arange(0, range_top, 1);
    temp.append(ax.plot(t_total, t_total, 'm:', linewidth=4.0, label="NoContention"));

    baseline_mite = 5.15;
    #series_tuples = get_tuples(filename, slabels, xlabel, ylabels);
# Contention
    contention_mite = 4.85
    #contention_mite = 4.33
    em = 0.951;
    ek = contention_mite / baseline_mite;
    n = 1000;
    range_bottom = n / ek * em;

    t_bottom = np.arange(0, range_bottom, 1);
    t_top = np.arange(range_bottom, range_top, 1);
    temp.append(ax.plot(t_bottom, t_bottom / em - (t_bottom / em) * ek, 'r--', linewidth=2.0, label="Contention-NoCAT"));
    temp.append(ax.plot(t_top, (n / ek - n) + (t_top - (n / ek) * em), 'r--', linewidth=2.0));#, label="Contention-NoCAT"));


    allocation_mite = 5.09;
    #allocation_mite = 4.81;
    em = 0.951;
    ek = allocation_mite / baseline_mite;
    range_bottom = n / ek * em;
    t_bottom = np.arange(0, range_bottom, 1);
    t_top = np.arange(range_bottom, range_top, 1);
    temp.append(ax.plot(t_bottom, t_bottom / em - (t_bottom / em) * ek, 'b-', linewidth=2.0, label="Contention-CAT"));
    temp.append(ax.plot(t_top, (n / ek - n) + (t_top - (n / ek) * em), 'b-', linewidth=2.0));#, label="Contention-CAT"));
    handles, labels = ax.get_legend_handles_labels()
    import operator
    handles2 = None;
    labels2 = None;

    hl = zip(handles,labels);#sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    lgd = ax.legend(handles2, labels2, loc="upper center");

    ax.set_xlabel("Machine learning throughput");
    ax.set_ylabel("Number of extra machines");
    plt.ylim(ymin=0, ymax=200);
    plt.xlim(xmin=0, xmax=1200);

    plt.savefig(filename, bbox_extra_artists=(lgd,), bbox_inches='tight');
    exit();

def main():
    graph();

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
