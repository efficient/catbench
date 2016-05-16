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
    args = parser.parse_args();
    if(type(args.series_labels) != list):
        args.series_labels = [args.series_labels];
    if(args.ymin != None):
        args.ymin = int(args.yim)
    if(args.ymax != None):
        args.ymax = int(args.ymax)
    return args.datafile, args.series_labels, args.x_label, args.y_labels, args.include_labels, args.title, args.outfile, args.fit, args.ymin, args.ymax, args.no_commit_message, args.logx, args.logy, args.cdf, args.legend_x, args.legend_y;

def get_tuples(filename, slabels, xlabel, ylabels):
    fd = open(filename, 'r');
    data = json.load(fd);
    series_tuples = dict();
    for slabel in slabels:
        series_tuples[slabel] = dict();
        for ylabel in ylabels:
            series_tuples[slabel][(xlabel, ylabel)] = list();
    for series_name, series in data.get("data").items():
        if(series_name not in series_tuples):
            continue;
        series_tuples[series_name]["description"] = series.get("description");
        for sample in series.get("samples"):
            for ylabel in ylabels:
#TODO what if there is no entry for a particular ylabel?
                series_tuples[series_name][(xlabel, ylabel)].append((sample.get(xlabel), sample.get(ylabel)));
    fd.close();
    return series_tuples;

def get_sample_description(filename, samplename):
    fd = open(filename, 'r');
    database = json.load(fd);
    label_entry = database.get("legend").get("samples").get(labelname);
    fd.close();
    if(label_entry == None):
        return "";
    return str(label_entry.get("description"));

def get_label(filename, labelname):
    print labelname;
    fd = open(filename, 'r');
    database = json.load(fd);
    label_entry = database.get("legend").get("samples").get(labelname);
    if(label_entry == None):
        return labelname;
    ret = label_entry.get("description");
    if(label_entry.get("unit") != ""):
        ret = ret + " (";
        ret = ret + label_entry.get("unit");
        ret = ret + ")";
    return ret;

def get_arg_label(filename, progname, labelname):
    fd = open(filename, 'r');
    database = json.load(fd);
    fd.close();
    try:
        return database.get("legend").get("args").get(progname).get(labelname).get("description");
    except:
        print("Missing entry " + progname + "." + labelname + "." + "description");
        return "";

def get_arg_unit(filename, progname, labelname):
    fd = open(filename, 'r');
    database = json.load(fd);
    fd.close();
    try:
        return database.get("legend").get("args").get(progname).get(labelname).get("unit");
    except:
        print("Missing entry " "legend" + "." + "args" + "." + progname + "." + labelname + "." + "unit");
        return "";

def get_aux(filename, progname, name, value):
    fd = open(filename, 'r');
    database = json.load(fd);
    fd.close();
    ret = progname + ": " + str(get_arg_label(filename, progname, name)) + "=" + str(value);
    if(get_arg_unit(filename, progname, name) != ""):
        ret = ret + " (" + str(get_arg_unit(filename, progname, name)) + ")";
    return ret;


def get_commit(filename):
    fd = open(filename, 'r');
    database = json.load(fd);
    ret = database.get("meta").get("commit") + "\n";
    return ret;
def get_series_aux(filename, seriesname, ilist):
    fd = open(filename, 'r');
    database = json.load(fd);
    ret = "";
    try:
        for arg in database.get("data").get(seriesname).get("args"):
            if(ilist == None or str(arg.get("name")[1:]) not in ilist):
                continue;
            ret = ret + str(database.get("data").get(seriesname).get("description") + ": " + \
                get_aux(filename, database.get("data").get(seriesname).get("type"), \
                arg.get("name"), arg.get("value")));
        ret = ret + "\n";
        fd.close();
        return ret;
    except:
        fd.close();
        return ret;

def graph_cdf(filename, slabels, xlabel, ypoints, title, outfile):
    fd = open(filename, 'r');
    data = json.load(fd);
    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);
    #ax.set_xlabel(xlabel);
    y = list();
    cdf = list();
    num_bins = 68;
    series_descriptions = list();
    for series in slabels:
        desc = data.get("data").get(series).get("description");
        cur_series_data = (data.get("data").get(series).get("samples")[0]).get(ypoints[0]);
        y.append(cur_series_data);
        print series_descriptions;
        print y;
        cur_series_data.sort();
        counts, bin_edges = np.histogram(cur_series_data, bins=max(cur_series_data), normed=True);
        ax.plot(cur_series_data, np.linspace(0,1,len(cur_series_data)), label=desc);
        cdf.append(np.cumsum(counts));
    count = 0;
    for yp in y:
        count += 1;
    box = ax.get_position();
    ax.set_position([box.x0, box.y0, box.width, box.height * 0.7]);

    #for c in cdf:
        #print c;
        #print bin_edges[1:];
        #ax.plot(bin_edges[1:], c);
    for yp in y:
        ys = sorted(yp);
        import scipy.stats as stats
        fit = stats.norm.pdf(ys, np.mean(ys), np.std(ys));
        density = stats.kde.gaussian_kde(ys);
        #x = np.arange(1000, 20000, 1000);
        #ax.plot(ys, fit, '-o');
        #ax.hist(ys, normed=True);
        #ax.plot(x, density(x), label=series_descriptions[count]);
        count += 1;
    ax.set_title(title);
    ax.title.set_position((0.5, 1.08));
    handles, labels = ax.get_legend_handles_labels()
    import operator
    hl = sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    lgd = ax.legend(handles2, labels2, loc="center right", bbox_to_anchor=(1.5, 0.5));
    fig.savefig(outfile, bbox_extra_artists=(lgd,), bbox_inches='tight');

def graph(filename, slabels, xlabel, ylabels, ilabels, title, outfile, fit, user_ymin, user_ymax, no_commit_message, logx, logy, legend_x, legend_y):
    series_tuples = get_tuples(filename, slabels, xlabel, ylabels);

    fig = plt.figure();
    ax = fig.add_subplot(1,1,1);

    ax.set_xlabel(get_label(filename, xlabel));
    ax.set_ylabel(get_label(filename, ylabels[0]));

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
    for key, val in series_tuples.items():
        line = list();
        for key2, val2 in val.items():
            if(key2 == "description"):
                continue;
            xy = map(list, zip(*val2));
            line.append((ax.plot(xy[0], xy[1], label=val["description"])));# + str(key2[1])))[0]);
            ax.scatter(xy[0], xy[1]);
        if(max(xy[0]) > cur_xmax):
            cur_xmax = max(xy[0]);
            x_copy = xy[0];
        if(max(xy[1]) > cur_ymax):
            cur_ymax = max(xy[1]);

    if(logx):
        ax.set_xscale('log');
    if(logy):
        ax.set_yscale('log');
    min_dist = cur_xmax / 16;
    idx = 0;
    if(x_copy[0] > 0):
        x_copy.insert(0, 0);
    #while(idx + 1 < len(x_copy)):
        #if(x_copy[idx] + min_dist > x_copy[idx+1]):
            #x_copy.pop(idx+1);
            #continue;
        #idx += 1;
    if(logx == logy == False):
        plt.xticks(x_copy);
    ax.set_title(title);
    ax.title.set_position((0.5, 1.08));

    handles, labels = ax.get_legend_handles_labels()

    # or sort them by labels
    import operator
    hl = sorted(zip(handles, labels), key=operator.itemgetter(1))
    handles2, labels2 = zip(*hl)
    lgd = ax.legend(handles2, labels2, loc="center right", bbox_to_anchor=(1.5, 0.5));
    lgd = plt.legend(loc="center right", bbox_to_anchor=(legend_x,legend_y));

#    cur_ymax = cur_ymax * 1.75;
    plt.xlim(xmin=0);
    plt.setp(ax.get_xticklabels(), fontsize=10, rotation=30)
    if(logy == False and logx == False ):
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
    #ax.axis("tight");
    fig.savefig(outfile, format='png', bbox_extra_artists=(lgd,), bbox_inches='tight');

def main():
    filename, slabels, xlabel, ylabels, ilabels, title, outfile, fit, ymin, ymax, no_commit_message, logx, logy, cdf, legend_x, legend_y = setup_optparse();
    if(cdf == False):
        graph(filename, slabels, xlabel, ylabels, ilabels, title, outfile, fit, ymin, ymax, no_commit_message, logx, logy, legend_x, legend_y);
    else:
        graph_cdf(filename, slabels, xlabel, ylabels, title, outfile);

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
