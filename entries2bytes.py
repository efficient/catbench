#!/usr/bin/python

import argparse;
import os;
import sys;
import json;

bytes_per_entry=56;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='input',
                        help='json to append to');
    args = parser.parse_args();
    return args.input;

def entries2bytes(input):
    fd = open(input, 'r');
    jsonfile = json.load(fd);
    data = jsonfile.get("data");
    legend = jsonfile.get("legend");
    if(input != ""):
    	legend["samples"]["working_set_size"] = {};
    	legend["samples"]["working_set_size"]["description"] = "Working set size";
    	legend["samples"]["working_set_size"]["unit"] = "mb";
        for key in data.keys():
            index = 0;
            while(index < len(data[key]["samples"])):
                sample = data[key]["samples"][index]["table_entries"];
		size_mb= sample * bytes_per_entry / 1024 / 1024;
                data[key]["samples"][index]["working_set_size"] = size_mb;
                index += 1
    fd.close();
    fd = open(input, 'w');
    json.dump(jsonfile, fd , indent=4, sort_keys=True);

def main():
    input = setup_optparse();
    if(input != ""):
        entries2bytes(input);
    else:
        print("Missing input file");

main();
