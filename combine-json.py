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
                        help='json to be appended to --input');
    parser.add_argument('--suffix', '-s', dest='suffix',
                        help='Suffix to attach to series from the second file');
    parser.add_argument('--outfile', '-o', dest='outfile',
                        help='Output json');
    parser.add_argument('--norm', '-n', dest='norm', default="",
                        help='Norm to normalize all (other) series against');
    parser.add_argument('--norm-suffix', dest='norm_suffix', default="",
                        help='Suffix to add to normalized series');
    parser.add_argument('--norm-x', dest='norm_x', default="",
                        help='Do not normalize these values');
    args = parser.parse_args();
    return args.file1, args.file2, args.suffix, args.outfile, args.norm, args.norm_suffix, args.norm_x;

def combine(file1, file2, suffix, outfile, norm, norm_suffix, norm_x):
    fd1 = open(file1, 'r');
    fd2 = open(file2, 'r');
    json1 = json.load(fd1);
    json2 = json.load(fd2);
    data1 = json1.get("data");
    data2 = json2.get("data");
    for key in data2.keys():
        new_key = key + suffix;
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
                print type(sample);
                for ylabel in sample:
                    if(base_sample[ylabel] != 0 and ylabel != norm_x):
                        data2[key]["samples"][index][ylabel] = sample[ylabel] / base_sample[ylabel];
                index += 1
	    data1[new_key] = data2[key];
	    data1[new_key]["description"] = data1[new_key]["description"] + suffix + norm_suffix;
    outfd = open(outfile, 'w');
    json.dump(json1, outfd, indent=4, sort_keys=True);

def main():
    file1, file2, suffix, outfile, norm, norm_suffix, norm_x = setup_optparse();
    if((norm == "" and norm_suffix == "" and norm_x == "") or (norm != "" and norm_suffix != "" and norm_x != "")):
        combine(file1, file2, suffix, outfile, norm, norm_suffix, norm_x);
    else:
        print("Missing one of: --norm, --norm-suffix, --norm-x\n");

main();
