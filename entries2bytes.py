#!/usr/bin/python

import argparse;
import os;
import sys;
import json;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='input',
                        help='input json (modified in place)');
    parser.add_argument('--bytes', '-b', dest='bytes', type=int, default=56,
                        help='Bytes per entry');
    args = parser.parse_args();
    return args.input, args.bytes;

def entries2bytes(input, bytes_per_entry):
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
		size_mb = sample * bytes_per_entry / 1024 / 1024;
                data[key]["samples"][index]["working_set_size"] = size_mb;
                index += 1
    fd.close();
    fd = open(input, 'w');
    json.dump(jsonfile, fd , indent=4, sort_keys=True);

def main():
    input, bytes_per_entry = setup_optparse();
    if(input != ""):
        entries2bytes(input, bytes_per_entry);
    else:
        print("Missing input file");

main();
