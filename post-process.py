#!/usr/bin/python

import argparse;
import os;
import sys;
import json;

descriptions = {
    'allocation': 'Contention+CAT',
    'baseline': 'No contention, no CAT',
    'basealloc': 'No contention, CAT',
    'contention': 'Contention, no CAT'
}

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='input',
                        help='input json (modified in place)');
    parser.add_argument('--bytes', '-b', dest='bytes', type=int, default=0,
                        help='Bytes per entry, omit or set to 0 to skip this postprocessing step');
    parser.add_argument('--series-description', '-s', dest='series_description', action='store_true', default=False,
                        help='Change series descriptions (legend labels) to something more reasonable. String replacements can be found in this file (post-process.py)');
    args = parser.parse_args();
    return args.input, args.bytes, args.series_description;

def entries2bytes(jsonfile, bytes_per_entry):
    data = jsonfile.get("data");
    legend = jsonfile.get("legend");
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

def fix_series_descriptions(jsonfile):
    data = jsonfile.get("data");
    for key in data.keys():
        data[key]["description"] = descriptions[key];


def main():
    input, bytes_per_entry, series_description = setup_optparse();
    if(input == ""):
        print("Missing input file, please specify with -i <input_filename>");
        exit();
    # Load json into memory
    fd = open(input, 'r');
    jsonfile = json.load(fd);

    # Do post processing based on flags passed
    if(bytes_per_entry != 0):
        try:
            entries2bytes(jsonfile, bytes_per_entry);
        except:
            print("Could not find table_entries, ignoring flag --bytes");
    if(series_description == True):
        try:
            fix_series_descriptions(jsonfile);
        except:
            print('Error, data[series] for some series is missing \"description\" field');
    # Close the fd because even with 'rw' python doesn't like to write to the open file
    fd.close();
    fd = open(input, 'w');
    # write out the modified json
    json.dump(jsonfile, fd , indent=4, sort_keys=True);

main();
