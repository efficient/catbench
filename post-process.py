#!/usr/bin/python

import argparse;
import os;
import sys;
import json;

descriptions = {
    'allocation': 'Contention-CAT',
    'baseline': 'NoContention',
    'basealloc': 'NoContention-CAT',
    'contention': 'Contention-NoCAT'
}

original_descriptions = {
    'Allocation': 'Contention-CAT',
    'Baseline': 'NoContention-NoCAT',
    'Basealloc': 'NoContention-CAT',
}

unit_conversions = {
    'L1-dcache-load-misses': 1000000,
    'L1-icache-load-misses': 1000000,
    'L2_RQSTS/ALL_CODE_RD': 1000000,
    'L2_RQSTS/ALL_DEMAND_DATA_RD': 1000000,
    'L2_RQSTS/ALL_DEMAND_MISS': 1000000,
    'L2_RQSTS/CODE_RD_MISS': 1000000,
    'L2_RQSTS/MISS': 1000000,
    'L2_RQSTS/REFERENCES': 1000000,
    'L2_TRANS/CODE_RD': 1000000,
    'L2_TRANS/DEMAND_DATA_RD': 1000000,
    'LLC_MISS': 1000000,
    'LLC_REFERENCE': 1000000,
    'MEM_LOAD_UOPS_RETIRED/L1_MISS': 1000000,
    'MEM_LOAD_UOPS_RETIRED/L2_MISS': 1000000,
    'MEM_LOAD_UOPS_RETIRED/L3_MISS': 1000000,
    'MEM_UOPS_RETIRED/ALL_LOADS': 1000000,
    'cpu-cycles': 1000000,
    'instructions': 1000000,
    'stalled-cycles': 1000000,
    'L2_RQSTS/ALL_DEMAND_REFERENCES': 1000000,
    'L2_RQSTS/DEMAND_DATA_RD_MISS': 1000000,
    'contender-L1-dcache-load-misses': 1000000,
    'contender-L1-icache-load-misses': 1000000,
    'contender-L2_RQSTS/ALL_CODE_RD': 1000000,
    'contender-L2_RQSTS/ALL_DEMAND_DATA_RD': 1000000,
    'contender-L2_RQSTS/ALL_DEMAND_MISS': 1000000,
    'contender-L2_RQSTS/CODE_RD_MISS': 1000000,
    'contender-L2_RQSTS/MISS': 1000000,
    'contender-L2_RQSTS/REFERENCES': 1000000,
    'contender-L2_TRANS/CODE_RD': 1000000,
    'contender-L2_TRANS/DEMAND_DATA_RD': 1000000,
    'contender-LLC_MISS': 1000000,
    'contender-LLC_REFERENCE': 1000000,
    'contender-MEM_LOAD_UOPS_RETIRED/L1_MISS': 1000000,
    'contender-MEM_LOAD_UOPS_RETIRED/L2_MISS': 1000000,
    'contender-MEM_LOAD_UOPS_RETIRED/L3_MISS': 1000000,
    'contender-MEM_UOPS_RETIRED/ALL_LOADS': 1000000,
    'contender-cpu-cycles': 1000000,
    'contender-instructions': 1000000,
    'contender-stalled-cycles': 1000000,
    'contender-L2_RQSTS/ALL_DEMAND_REFERENCES': 1000000,
    'contender-L2_RQSTS/DEMAND_DATA_RD_MISS': 1000000
}

miss_counters = [ 
                    'L1-dcache-load-misses',
                    'L1-icache-load-misses',
                    'L2_RQSTS/ALL_CODE_RD',
                    'L2_RQSTS/ALL_DEMAND_DATA_RD',
                    'L2_RQSTS/ALL_DEMAND_MISS',
                    'L2_RQSTS/ALL_DEMAND_REFERENCES',
                    'L2_RQSTS/CODE_RD_MISS',
                    'L2_RQSTS/CODE_RD_MISS_RATIO',
                    'L2_RQSTS/DEMAND_DATA_RD_MISS',
                    'L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO',
                    'L2_RQSTS/MISS',
                    'L2_RQSTS/REFERENCES',
                    'L2_TRANS/CODE_RD',
                    'L2_TRANS/DEMAND_DATA_RD',
                    'LLC_MISS',
                    'LLC_REFERENCE',
                    'dTLB-load-misses',
                    'iTLB-load-misses'
                    'contender-L1-dcache-load-misses',
                    'contender-L1-icache-load-misses',
                    'contender-L2_RQSTS/ALL_CODE_RD',
                    'contender-L2_RQSTS/ALL_DEMAND_DATA_RD',
                    'contender-L2_RQSTS/ALL_DEMAND_MISS',
                    'contender-L2_RQSTS/ALL_DEMAND_REFERENCES',
                    'contender-L2_RQSTS/CODE_RD_MISS',
                    'contender-L2_RQSTS/CODE_RD_MISS_RATIO',
                    'contender-L2_RQSTS/DEMAND_DATA_RD_MISS',
                    'contender-L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO',
                    'contender-L2_RQSTS/MISS',
                    'contender-L2_RQSTS/REFERENCES',
                    'contender-L2_TRANS/CODE_RD',
                    'contender-L2_TRANS/DEMAND_DATA_RD',
                    'contender-LLC_MISS',
                    'contender-LLC_REFERENCE',
                    'contender-dTLB-load-misses',
                    'contender-iTLB-load-misses'
];

def num2word(num):
    if(num == 1000):
        return "thousands";
    if(num == 1000000):
        return "millions";
    return None;


def setup_optparse():
    parser = argparse.ArgumentParser();
    parser.add_argument('--input', '-i', dest='input',
                        help='input json (modified in place)');
    parser.add_argument('--bytes', '-b', dest='bytes', type=int, default=0,
                        help='Bytes per entry, omit or set to 0 to skip this postprocessing step');
    parser.add_argument('--series-description', '-s', dest='series_description', action='store_true', default=False,
                        help='Change series descriptions (legend labels) to something more reasonable. String replacements can be found in this file (post-process.py)');
    parser.add_argument('--perf-descriptions', '-p', dest='perf_descriptions', action='store_true', default=False,
                        help='Add in legend entries for all perf counters');
    parser.add_argument('--change-units', '-c', dest='change_units', action='store_true', default=False,
                        help='Change units on large numbers to thousands, millions, billions.');
    parser.add_argument('--miss-per-instruction', '-m', dest='miss_per_instruction', action='store_true', default=False,
                        help='Add L{1,2,3} cache misses per kilo-instruction to all data points');
    args = parser.parse_args();
    return args.input, args.bytes, args.series_description, args.perf_descriptions, args.change_units, args.miss_per_instruction;

def entries2bytes(jsonfile, bytes_per_entry):
    data = jsonfile.get("data");
    legend = jsonfile.get("legend");
    legend["samples"]["working_set_size"] = {};
    legend["samples"]["working_set_size"]["description"] = "Working set size";
    legend["samples"]["working_set_size"]["unit"] = "MB";
    for key in data.keys():
        index = 0;
        while(index < len(data[key]["samples"])):
            sample = data[key]["samples"][index]["table_entries"];
            size_mb = sample * bytes_per_entry / 1024 / 1024;
            data[key]["samples"][index]["working_set_size"] = size_mb;
            index += 1

def fix_series_descriptions(jsonfile):
    data = jsonfile.get("data");
    for key in descriptions.keys():
        if(key in data):
            data[key]["description"] = descriptions[key];
    for key in data.keys():
        for key2 in original_descriptions:
            if(data[key]["description"].startswith(key2)):
                data[key]["description"] = data[key]["description"].replace(key2, original_descriptions[key2]);

def add_perf_descriptions(jsonfile):
    legend = jsonfile.get("legend");
    legend_samples = legend["samples"];

    legend_samples['L1-dcache-load-misses'] = {};
    legend_samples['L1-dcache-load-misses']["description"] = "L1 dcache load misses per second";
    legend_samples['L1-dcache-load-misses']["unit"] = "";
    
    legend_samples['L1-icache-load-misses'] = {};
    legend_samples['L1-icache-load-misses']["description"] = "L1 icache load misses per second";
    legend_samples['L1-icache-load-misses']["unit"] = "";
    
    legend_samples['L2_RQSTS/ALL_CODE_RD'] = {};
    legend_samples['L2_RQSTS/ALL_CODE_RD']["description"] = "L2 code read requests per second";
    legend_samples['L2_RQSTS/ALL_CODE_RD']["unit"] = "";
    
    legend_samples['L2_RQSTS/ALL_DEMAND_DATA_RD'] = {};
    legend_samples['L2_RQSTS/ALL_DEMAND_DATA_RD']["description"] = "L2 data read requests per second";
    legend_samples['L2_RQSTS/ALL_DEMAND_DATA_RD']["unit"] = "";
    
    legend_samples['L2_RQSTS/ALL_DEMAND_MISS'] = {};
    legend_samples['L2_RQSTS/ALL_DEMAND_MISS']["description"] = "L2 all misses per second";
    legend_samples['L2_RQSTS/ALL_DEMAND_MISS']["unit"] = "";
    
    legend_samples['L2_RQSTS/ALL_DEMAND_REFERENCES'] = {};
    legend_samples['L2_RQSTS/ALL_DEMAND_REFERENCES']["description"] = "L2 all references per second";
    legend_samples['L2_RQSTS/ALL_DEMAND_REFERENCES']["unit"] = "";
    
    legend_samples['L2_RQSTS/CODE_RD_MISS'] = {};
    legend_samples['L2_RQSTS/CODE_RD_MISS']["description"] = "L2 code read misses per second";
    legend_samples['L2_RQSTS/CODE_RD_MISS']["unit"] = "";
    
    legend_samples['L2_RQSTS/CODE_RD_MISS_RATIO'] = {};
    legend_samples['L2_RQSTS/CODE_RD_MISS_RATIO']["description"] = "L2 code read miss ratio";
    legend_samples['L2_RQSTS/CODE_RD_MISS_RATIO']["unit"] = "";
    
    legend_samples['L2_RQSTS/DEMAND_DATA_RD_MISS'] = {};
    legend_samples['L2_RQSTS/DEMAND_DATA_RD_MISS']["description"] = "L2 data read misses per second";
    legend_samples['L2_RQSTS/DEMAND_DATA_RD_MISS']["unit"] = "";
    
    legend_samples['L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO'] = {};
    legend_samples['L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO']["description"] = "L2 data read miss ratio";
    legend_samples['L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO']["unit"] = "";
    
    legend_samples['L2_RQSTS/MISS'] = {};
    legend_samples['L2_RQSTS/MISS']["description"] = "L2 misses per second";
    legend_samples['L2_RQSTS/MISS']["unit"] = "";
    
    legend_samples['L2_RQSTS/MISS_RATIO'] = {};
    legend_samples['L2_RQSTS/MISS_RATIO']["description"] = "L2 miss ratio";
    legend_samples['L2_RQSTS/MISS_RATIO']["unit"] = "";
    
    legend_samples['L2_RQSTS/REFERENCES'] = {};
    legend_samples['L2_RQSTS/REFERENCES']["description"] = "L2 references per second";
    legend_samples['L2_RQSTS/REFERENCES']["unit"] = "";
    
    legend_samples['L2_TRANS/CODE_RD'] = {};
    legend_samples['L2_TRANS/CODE_RD']["description"] = "L2 trans code read per second";
    legend_samples['L2_TRANS/CODE_RD']["unit"] = "";
    
    legend_samples['L2_TRANS/DEMAND_DATA_RD'] = {};
    legend_samples['L2_TRANS/DEMAND_DATA_RD']["description"] = "L2 trans demand data read per second";
    legend_samples['L2_TRANS/DEMAND_DATA_RD']["unit"] = "";
    
    legend_samples['LLC_MISS'] = {};
    legend_samples['LLC_MISS']["description"] = "LLC misses per second";
    legend_samples['LLC_MISS']["unit"] = "";
    
    legend_samples['LLC_MISS_RATIO'] = {};
    legend_samples['LLC_MISS_RATIO']["description"] = "LLC miss ratio";
    legend_samples['LLC_MISS_RATIO']["unit"] = "";
    
    legend_samples['LLC_REFERENCE'] = {};
    legend_samples['LLC_REFERENCE']["description"] = "LLC references per second";
    legend_samples['LLC_REFERENCE']["unit"] = "";
    
    legend_samples['MEM_LOAD_UOPS_RETIRED/L1_MISS'] = {};
    legend_samples['MEM_LOAD_UOPS_RETIRED/L1_MISS']["description"] = "Mem load uops retired l1 miss per second";
    legend_samples['MEM_LOAD_UOPS_RETIRED/L1_MISS']["unit"] = "";
    
    legend_samples['MEM_LOAD_UOPS_RETIRED/L2_MISS'] = {};
    legend_samples['MEM_LOAD_UOPS_RETIRED/L2_MISS']["description"] = "Mem load uops retired l2 miss per second";
    legend_samples['MEM_LOAD_UOPS_RETIRED/L2_MISS']["unit"] = "";
    
    legend_samples['MEM_LOAD_UOPS_RETIRED/L3_MISS'] = {};
    legend_samples['MEM_LOAD_UOPS_RETIRED/L3_MISS']["description"] = "Mem load uops retired l3 miss per second";
    legend_samples['MEM_LOAD_UOPS_RETIRED/L3_MISS']["unit"] = "";
    
    legend_samples['MEM_UOPS_RETIRED/ALL_LOADS'] = {};
    legend_samples['MEM_UOPS_RETIRED/ALL_LOADS']["description"] = "Mem uops retired all loads per second";
    legend_samples['MEM_UOPS_RETIRED/ALL_LOADS']["unit"] = "";
    
    legend_samples['cpu-cycles'] = {};
    legend_samples['cpu-cycles']["description"] = "CPU cycles per second";
    legend_samples['cpu-cycles']["unit"] = "";
    
    legend_samples['instructions'] = {};
    legend_samples['instructions']["description"] = "Instructions per second";
    legend_samples['instructions']["unit"] = "";

    legend_samples['stalled-cycles'] = {};
    legend_samples['stalled-cycles']["description"] = "Stalled cycles per second";
    legend_samples['stalled-cycles']["unit"] = "";

    legend_samples['dTLB-load-misses'] = {};
    legend_samples['dTLB-load-misses']["description"] = "dTLB load misses per second";
    legend_samples['dTLB-load-misses']["unit"] = "";
    
    legend_samples['iTLB-load-misses'] = {};
    legend_samples['iTLB-load-misses']["description"] = "iTLB load misses per second";
    legend_samples['iTLB-load-misses']["unit"] = "";

    legend_samples['contender-L1-dcache-load-misses'] = {};
    legend_samples['contender-L1-dcache-load-misses']["description"] = "L1 dcache load misses per second";
    legend_samples['contender-L1-dcache-load-misses']["unit"] = "";
    
    legend_samples['contender-L1-icache-load-misses'] = {};
    legend_samples['contender-L1-icache-load-misses']["description"] = "L1 icache load misses per second";
    legend_samples['contender-L1-icache-load-misses']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/ALL_CODE_RD'] = {};
    legend_samples['contender-L2_RQSTS/ALL_CODE_RD']["description"] = "L2 code read requests per second";
    legend_samples['contender-L2_RQSTS/ALL_CODE_RD']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_DATA_RD'] = {};
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_DATA_RD']["description"] = "L2 data read requests per second";
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_DATA_RD']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_MISS'] = {};
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_MISS']["description"] = "L2 all misses per second";
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_MISS']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_REFERENCES'] = {};
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_REFERENCES']["description"] = "L2 all references per second";
    legend_samples['contender-L2_RQSTS/ALL_DEMAND_REFERENCES']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/CODE_RD_MISS'] = {};
    legend_samples['contender-L2_RQSTS/CODE_RD_MISS']["description"] = "L2 code read misses per second";
    legend_samples['contender-L2_RQSTS/CODE_RD_MISS']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/CODE_RD_MISS_RATIO'] = {};
    legend_samples['contender-L2_RQSTS/CODE_RD_MISS_RATIO']["description"] = "L2 code read miss ratio";
    legend_samples['contender-L2_RQSTS/CODE_RD_MISS_RATIO']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/DEMAND_DATA_RD_MISS'] = {};
    legend_samples['contender-L2_RQSTS/DEMAND_DATA_RD_MISS']["description"] = "L2 data read misses per second";
    legend_samples['contender-L2_RQSTS/DEMAND_DATA_RD_MISS']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO'] = {};
    legend_samples['contender-L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO']["description"] = "L2 data read miss ratio";
    legend_samples['contender-L2_RQSTS/DEMAND_DATA_RD_MISS_RATIO']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/MISS'] = {};
    legend_samples['contender-L2_RQSTS/MISS']["description"] = "L2 misses per second";
    legend_samples['contender-L2_RQSTS/MISS']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/MISS_RATIO'] = {};
    legend_samples['contender-L2_RQSTS/MISS_RATIO']["description"] = "L2 miss ratio";
    legend_samples['contender-L2_RQSTS/MISS_RATIO']["unit"] = "";
    
    legend_samples['contender-L2_RQSTS/REFERENCES'] = {};
    legend_samples['contender-L2_RQSTS/REFERENCES']["description"] = "L2 references per second";
    legend_samples['contender-L2_RQSTS/REFERENCES']["unit"] = "";
    
    legend_samples['contender-L2_TRANS/CODE_RD'] = {};
    legend_samples['contender-L2_TRANS/CODE_RD']["description"] = "L2 trans code read per second";
    legend_samples['contender-L2_TRANS/CODE_RD']["unit"] = "";
    
    legend_samples['contender-L2_TRANS/DEMAND_DATA_RD'] = {};
    legend_samples['contender-L2_TRANS/DEMAND_DATA_RD']["description"] = "L2 trans demand data read per second";
    legend_samples['contender-L2_TRANS/DEMAND_DATA_RD']["unit"] = "";
    
    legend_samples['contender-LLC_MISS'] = {};
    legend_samples['contender-LLC_MISS']["description"] = "LLC misses per second";
    legend_samples['contender-LLC_MISS']["unit"] = "";
    
    legend_samples['contender-LLC_MISS_RATIO'] = {};
    legend_samples['contender-LLC_MISS_RATIO']["description"] = "LLC miss ratio";
    legend_samples['contender-LLC_MISS_RATIO']["unit"] = "";
    
    legend_samples['contender-LLC_REFERENCE'] = {};
    legend_samples['contender-LLC_REFERENCE']["description"] = "LLC references per second";
    legend_samples['contender-LLC_REFERENCE']["unit"] = "";
    
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L1_MISS'] = {};
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L1_MISS']["description"] = "Mem load uops retired l1 miss per second";
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L1_MISS']["unit"] = "";
    
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L2_MISS'] = {};
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L2_MISS']["description"] = "Mem load uops retired l2 miss per second";
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L2_MISS']["unit"] = "";
    
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L3_MISS'] = {};
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L3_MISS']["description"] = "Mem load uops retired l3 miss per second";
    legend_samples['contender-MEM_LOAD_UOPS_RETIRED/L3_MISS']["unit"] = "";
    
    legend_samples['contender-MEM_UOPS_RETIRED/ALL_LOADS'] = {};
    legend_samples['contender-MEM_UOPS_RETIRED/ALL_LOADS']["description"] = "Mem uops retired all loads per second";
    legend_samples['contender-MEM_UOPS_RETIRED/ALL_LOADS']["unit"] = "";
    
    legend_samples['contender-cpu-cycles'] = {};
    legend_samples['contender-cpu-cycles']["description"] = "CPU cycles per second";
    legend_samples['contender-cpu-cycles']["unit"] = "";
    
    legend_samples['contender-instructions'] = {};
    legend_samples['contender-instructions']["description"] = "Instructions per second";
    legend_samples['contender-instructions']["unit"] = "";

    legend_samples['contender-stalled-cycles'] = {};
    legend_samples['contender-stalled-cycles']["description"] = "Stalled cycles per second";
    legend_samples['contender-stalled-cycles']["unit"] = "";

    legend_samples['contender-dTLB-load-misses'] = {};
    legend_samples['contender-dTLB-load-misses']["description"] = "dTLB load misses per second";
    legend_samples['contender-dTLB-load-misses']["unit"] = "";
    
    legend_samples['contender-iTLB-load-misses'] = {};
    legend_samples['contender-iTLB-load-misses']["description"] = "iTLB load misses per second";
    legend_samples['contender-iTLB-load-misses']["unit"] = "";

    legend_samples['999tail-latency'] = {};
    legend_samples['999tail-latency']["description"] = "Mite 99.9%-ile tail latency";
    legend_samples['999tail-latency']["unit"] = "us";

    legend_samples['mite_throughput'] = {};
    legend_samples['mite_throughput']["description"] = "Mite Throughput";
    legend_samples['mite_throughput']["unit"] = "Mops";

    if('contender_tput' not in legend_samples):
        legend_samples['contender_tput'] = {};
        legend_samples['contender_tput']["description"] = "Contender throughput";
        legend_samples['contender_tput']["unit"] = "epochs/s";

def change_counter_units(jsonfile):
    data = jsonfile.get("data");
    legend = jsonfile.get("legend");
    for series in data.keys():
        index = 0;
        while(index < len(data[series]["samples"])):
            sample = data[series]["samples"][index];
            for datakey in sample.keys():
                if(datakey not in unit_conversions):
		            continue;
                new_key = datakey + "-scaled";
                if(new_key not in legend["samples"]):
                    legend["samples"][new_key] = {};
                    legend["samples"][new_key]["description"] = legend["samples"][datakey]["description"];
                    legend["samples"][new_key]["unit"] = num2word(unit_conversions[datakey]);
                sample[new_key] = sample[datakey] / unit_conversions[datakey];
            index += 1

def add_misses_per_instruction(jsonfile):
    data = jsonfile.get("data");
    legend = jsonfile.get("legend");
    for series in data.keys():
        index = 0;
        while(index < len(data[series]["samples"])):
            sample = data[series]["samples"][index];
            for datakey in sample.keys():
                if(datakey not in miss_counters):
                    continue;
                new_key = datakey + "-per-kilo-instruction";
                if(new_key not in legend["samples"]):
                    legend["samples"][new_key] = {};
                    legend["samples"][new_key]["description"] = ' '.join(legend["samples"][datakey]["description"].split(' ')[:-2]) + " per kilo-instruction";
                    legend["samples"][new_key]["unit"] = "";
                if(datakey in sample):
                    sample[new_key] = sample[datakey] / (sample["instructions"] / 1000);
            index += 1;

def main():
    input, bytes_per_entry, series_description, perf_descriptions, change_units, miss_per_instruction = setup_optparse();
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
    # add_perf_descriptions must happen before add_misses_per_instruction and change_counter_units
    if(perf_descriptions == True):
        try:
            add_perf_descriptions(jsonfile);
        except:
            print('Error, missing some perf counter');
    # add_misses_per_instruction must happen before change_counter_units
    if(miss_per_instruction == True):
    	try:
            add_misses_per_instruction(jsonfile);
	except:
	    print("Something went wrong with post-process.py:add_misses_per_instruction(1)");
    if(perf_descriptions == False and change_units == True):
        print('Error, cannot run -c without -p');
        exit();
    if(change_units == True):
        change_counter_units(jsonfile);

    # Close the fd because even with 'rw' python doesn't like to write to the open file
    fd.close();
    fd = open(input, 'w');
    # write out the modified json
    json.dump(jsonfile, fd , indent=4, sort_keys=True);

main();
