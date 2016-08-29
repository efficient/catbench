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
    args = parser.parse_args();
    return args.file1, args.file2, args.suffix, args.outfile;

def combine(file1, file2, suffix, outfile):
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
    outfd = open(outfile, 'w');
    json.dump(json1, outfd, indent=4, sort_keys=True);

def main():
    file1, file2, suffix, outfile = setup_optparse();
    combine(file1, file2, suffix, outfile);

main();
