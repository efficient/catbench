#!/usr/bin/python
import json;

import itertools;
marker = itertools.cycle(('o', '>', 'D', 's', 'h', '+', '<', '^'));
color = itertools.cycle(('b', 'm', 'k', 'r' ,'y'));
color_alpha = 1.0;
marker_size = 4;

def get_tuples(filename, slabels, xlabel, ylabels):
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
