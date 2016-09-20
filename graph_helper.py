#!/usr/bin/python
import json;

import itertools;
marker = itertools.cycle(('o', '>', 'D', 's', 'h', '+', '<', '^'));
marker_copy = itertools.cycle(('o', '>', 'D', 's', 'h', '+', '<', '^'));
color = itertools.cycle(('b', 'k', 'r', 'y'));
color_copy = itertools.cycle(('b', 'k', 'r', 'y'));
color_alpha = 1.0;
marker_size = 6;

def all_close(series, tuples, threshold):
    for name in series:
        for comp in series:
            if(tuples[name]["mean"] + threshold <= tuples[comp]["mean"] or tuples[name]["mean"] - threshold >= tuples[comp]["mean"]):
                return False;
    return True;

def order_bucket(series, tuples, threshold):
    if(len(series) == 1 or len(series) == 0):
        return series;
    baseline = series[0];
    baseline_mean = tuples[baseline]["mean"];
    smaller_bucket = list();
    bigger_bucket = list();
    same_bucket = list();
    for name in series:
        cur_mean = tuples[name]["mean"];
        if(name == baseline):
            continue;
        if(all_close(series, tuples, threshold)):
            series = sorted(series, key=lambda x: tuples[x]["description"]);
	    series.reverse();
	    return series;
        if(baseline_mean - threshold > cur_mean):
            smaller_bucket.append(name);
        elif(baseline_mean + threshold < cur_mean):
            bigger_bucket.append(name);
        else:
            same_bucket.append(name);
    same_bucket.append(baseline);
    return [order_bucket(smaller_bucket, tuples, threshold), order_bucket(same_bucket, tuples, threshold), order_bucket(bigger_bucket, tuples, threshold)];

def flatten(S):
    if S == []:
        return S
    if isinstance(S[0], list):
        return flatten(S[0]) + flatten(S[1:])
    return S[:1] + flatten(S[1:])

def order_ybar(tuples, xkey, ykey):
    series_names = tuples.keys();
    ymin = 9999999;
    ymax = 0;
    for series in series_names:
        yvals = [i[0] for i in tuples[series][(xkey, ykey)]];
        ymin = min(ymin, min(yvals));
        ymax = max(ymax, max(yvals));
    threshold = (ymax - ymin) / 100;
    series_order = order_bucket(series_names, tuples, threshold);
    if(series_order != None):
	series_order = flatten(series_order);
    	series_order.reverse();
    else:
        series_order = series_names;
    index = 0;
    for series in series_order:
        tuples[series]["order"] = index;
        index += 1;

def get_tuples(filename, slabels, xlabel, ylabels, nosort):
    count = 0;
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
        series_tuples[series_name]["order"] = slabels.index(series_name);
        count += 1;
        series_tuples[series_name]["mean"] = 0;
	count2 = 0;
        for sample in series.get("samples"):
	    ylabel = ylabels[0];
            if(sample.get(ylabel) != None):
                series_tuples[series_name][(xlabel, ylabel)].append((sample.get(xlabel), sample.get(ylabel)));
                series_tuples[series_name]["mean"] += sample.get(ylabel);
		count2 += 1;
	if(count2 == 0):
	    count2 = 1;
	series_tuples[series_name]["mean"] /= count2;
	print(series_name + "." + ylabels[0] + " = " + str(series_tuples[series_name]["mean"]));
    fd.close();
    if(nosort == True):
        order_ybar(series_tuples, xlabel, ylabels[0]);
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
