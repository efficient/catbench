#!/usr/bin/python

import argparse;
import os;
import sys;
import json;

def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='file1',
                        help='json to append to');
    parser.add_argument('--append', '-a', nargs='+', dest='files2',
                        help='json(s) to be appended to --input.');
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
    parser.add_argument('--median', '-m', dest='median', default=None,
                        help='Select each point in the specified --series from a group of --append files based on the median of the specified field. ' +
                             'Using this with suffix is untested, and probably not a good idea, and your data files should probably all have the same domain...' +
                             'Normalization is right out.');

    args = parser.parse_args();

    if args.median:
      if not isinstance(args.files2, list):
        sys.stderr.write('ERROR: Use of --median requires more than one file to --append');
        sys.exit(1);
    else:
      if isinstance(args.files2, list):
        sys.stderr.write('ERROR: I don\'t know what to do with more than one --append file');
        sys.exit(1);
      else:
        args.files2 = [args.files2];

    if args.baselinecontention:
        args.series = ["baseline", "contention"];
    return args.file1, args.files2, args.suffix, args.outfile, args.norm, args.norm_suffix, args.norm_x, set(args.series), args.median;

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
        

def combine(file1, files2, suffix, outfile, norm, norm_suffix, norm_x, series, median):
    fd1 = open(file1, 'r');
    fds2 = [open(each, 'r') for each in files2];
    json1 = json.load(fd1);
    jsons2 = [json.load(each) for each in fds2];
    data1 = json1.get("data");
    datas2 = [each.get("data") for each in jsons2];

    if median:
      alldat = [data1] + datas2;
      if not len(series):
        series = data1.keys();
      for group in series:
        samps = [each[group]['samples'] for each in alldat];
        res = samps[0];
        if len(samps) != len(alldat):
          sys.stderr.write('ERROR: Couldn\'t find series \'series\' in all files')
          exit(1)
        nsamps = len(res);
        if filter(lambda elm: len(elm) != nsamps, samps):
          sys.stderr.write('ERROR: Not all input files have the same number of elements in \'series\'')
          exit(1)
        for idx in range(nsamps):
          order = sorted([each[idx] for each in samps], key=lambda elm: elm[median]);
          res[idx] = order[len(order) / 2];

    else:
      data2 = datas2[0];
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
    for each in fds2:
      each.close();
    outfd = open(outfile, 'w');
    json.dump(json1, outfd, indent=4, sort_keys=True);

def main():
    file1, files2, suffix, outfile, norm, norm_suffix, norm_x, series, median = setup_optparse();
    #if(baselinecontention == True):
    #    verify(file1, file2);
    if((norm == "" and norm_suffix == "" and norm_x == "") or (norm != "" and norm_suffix != "" and norm_x != "")):
        combine(file1, files2, suffix, outfile, norm, norm_suffix, norm_x, series, median);
    else:
        print("Missing one of: --norm, --norm-suffix, --norm-x\n");

main();
