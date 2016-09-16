#!/usr/bin/python

import argparse;
import os;
import sys;
import json;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='file1',
                        help='json to append to');
    parser.add_argument('--append', '-a', dest='file2',
                        help='json to be appended to --input.');
    parser.add_argument('--suffix', '-s', dest='suffix', default="",
                        help='Suffix to attach to series from the second file');
    parser.add_argument('--outfile', '-o', dest='outfile',
                        help='Output json. Note that if -i and -o are the same, -i will be overwritten.');
    parser.add_argument('--norm', '-n', dest='norm', default="",
                        help='Norm to normalize all (other) series against');
    parser.add_argument('--norm-suffix', dest='norm_suffix', default="",
                        help='Suffix to add to normalized series');
    parser.add_argument('--norm-x', dest='norm_x', default="",
                        help='Do not normalize these values');
    parser.add_argument('--series', '-d', nargs='+', dest='series', default=[],
                        help='Only copy specified data series (still applies suffix). Note that if suffix is empty, a replacement will be done.')
    parser.add_argument('--baseline-contention', '-b', dest='baselinecontention', action='store_true', default=False,
                        help='Only copy baseline and contention (leave suffix blank for best results). Overrides -d switch!');
    args = parser.parse_args();
    if args.baselinecontention:
        args.series = ["baseline", "contention"];
    return args.file1, args.file2, args.suffix, args.outfile, args.norm, args.norm_suffix, args.norm_x, set(args.series);

constant_keys=("cache_ways", "mite_tput_limit", "zipf_alpha");
def verify(file1, file2):
    fd1 = open(file1, 'r');
    fd2 = open(file2, 'r');
    json1 = json.load(fd1);
    json2 = json.load(fd2);
    data1 = json1.get("data");
    data2 = json2.get("data");
    
    found_violation = {};
    for key in data1.keys():
        for entry in data1[key]["samples"]:
            for const_key in constant_keys:
                if(const_key not in entry):
                    continue;
                for entry2 in data2["baseline"]["samples"]:
                    if(const_key not in entry2):
                        continue;
		    print(entry2[const_key] + " = " + entry[const_key]);
                    if(entry2[const_key] != entry[const_key]):
                        found_violation[const_key] = True;
    for key in found_violation.keys():
        if(found_violation[key]):
            print("Warning, variable " + key + " mismatch between baseline file and experiment file");
        

def combine(file1, file2, suffix, outfile, norm, norm_suffix, norm_x, series):
    fd1 = open(file1, 'r');
    fd2 = open(file2, 'r');
    json1 = json.load(fd1);
    json2 = json.load(fd2);
    data1 = json1.get("data");
    data2 = json2.get("data");
    for key in data2.keys():
        if(len(series) and key not in series):
            continue;
        new_key = key + suffix;
        if(new_key in data1):
            print("Warning, overwriting " + new_key + " in " + file1);
        data1[new_key] = data2[key];
        data1[new_key]["description"] = data1[new_key]["description"] + suffix;
    if(norm != ""):
        for key in data2.keys():
            if(key == norm):
                continue;
            new_key = key + suffix + norm_suffix
            index = 0;
            while(index < len(data2[key]["samples"])):
                sample = data2[key]["samples"][index];
                base_sample = data2[norm]["samples"][index];
                for ylabel in sample:
                    if(base_sample[ylabel] != 0 and ylabel != norm_x):
                        data2[key]["samples"][index][ylabel] = sample[ylabel] / base_sample[ylabel];
                index += 1
        data1[new_key] = data2[key];
        data1[new_key]["description"] = data1[new_key]["description"] + suffix + " normalized to " + norm;
    fd1.close();
    fd2.close();
    outfd = open(outfile, 'w');
    json.dump(json1, outfd, indent=4, sort_keys=True);

def main():
    file1, file2, suffix, outfile, norm, norm_suffix, norm_x, series = setup_optparse();
    #if(baselinecontention == True):
    #    verify(file1, file2);
    if((norm == "" and norm_suffix == "" and norm_x == "") or (norm != "" and norm_suffix != "" and norm_x != "")):
        combine(file1, file2, suffix, outfile, norm, norm_suffix, norm_x, series);
    else:
        print("Missing one of: --norm, --norm-suffix, --norm-x\n");

main();
