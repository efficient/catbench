#!/usr/bin/python

import argparse;
import os;
import sys;
import matplotlib as mpl;
mpl.use('Agg');
import matplotlib.pyplot as plt;

DELIM = "==";

def setup_optparse():
	parser = argparse.ArgumentParser(description='Example: ./graph-cat.py -i file.csv --series 0 --xdata 1 --ydata 2 3 --ignore 4 5 --title Graph --ylabel Execution time (s)');
	parser.add_argument('--input', '-i', dest='datafile',
						help='input csv with header');
	parser.add_argument('--series', '-s', type=int, dest='series_columns',
						help='series column');
	parser.add_argument('--xdata', '-x', type=int, dest='x_column',
						help='X axis coordinates, single column');
	parser.add_argument('--ydata', '-y', type=int, nargs='+', dest='y_columns',
						help='Y axis data columns. Enter single number or list (-s 1 2 3)');
	parser.add_argument('--ignore', nargs='+', type=int, dest='ignore_columns',
						help='Ignore these columns when summarizing information. Enter list (--ignore 1 2 3');
	parser.add_argument('--title', '-t', dest='title',
						help='Graph title');
	parser.add_argument('--ylabel', dest='ylabel',
						help='Y axis label');
	parser.add_argument('--outfile', '-o', dest='outfile', default="graph.png",
						help='Output filename');
	args = parser.parse_args();
	return args.datafile, args.series_columns, args.x_column, args.y_columns, args.ignore_columns, args.title, args.ylabel, args.outfile;

def get_columns(comma_sep_line):
	return comma_sep_line.split(',');

def graph(filename, scol, xcol, ycols, icols, title, ylabel, outfile):
	# First sort into Y buckets
	ydict = dict();

	fd = open(filename, 'r');
	lines = fd.read().splitlines();
	count = 0;
	while(lines[0] != DELIM):
		lines.pop(0);
	lines.pop(0);
	line = lines[0];
	headers = get_columns(line);
	scol_empty = scol == None;
# If no series, add a dummy series
	if(scol_empty):
		headers.append("");
		scol = len(headers) - 1;
	ycol_def = 0;
# Break header line into columns:
	for ycol in ycols:
# Just grab any y column as a default item
		ycol_def = ycol;
		# Add a new bucket for each Y column
		ydict[headers[ycol]] = dict();
	lines.pop(0);
	# Scan through and grab all of the series labels
	for line in lines:
		cols = get_columns(line);
		if(scol_empty):
			cols.append("");
		scol_val = cols[scol];
# Val is the list that represents this Y data column
# Add lists for each series
		for key, val in ydict.items():
			if(not (scol_val in val)):
				val[scol_val] = list();
# Scan through and add all (x, y) tuples
	for line in lines:
		cols = get_columns(line);
		if(scol_empty):
			cols.append("");
		scol_val = cols[scol];
		for ycol in ycols:
			assert(scol_val in ydict[headers[ycol]]);
			ydict[headers[ycol]][scol_val].append((cols[xcol], cols[ycol]));
	#for key, val in ydict.items():
	#	print(key);
	#	print(val);
	#	print("");
# Scan through and figure out all non-ignored column values
	extra_params = dict();
	flagged_columns = list();
	flagged_columns.append(xcol);
	flagged_columns.append(scol);
	flagged_columns = flagged_columns + ycols;
	if(icols != None):
			flagged_columns = flagged_columns + icols;
	for line in lines:
		cols = get_columns(line);
		for idx in range(0, len(cols)):
			if(idx not in flagged_columns):
				if(headers[idx] not in extra_params):
					extra_params[headers[idx]] = list();
				if(cols[idx] not in extra_params[headers[idx]]):
					extra_params[headers[idx]].append(cols[idx]);
	print extra_params;
	fig = plt.figure();
	ax = fig.add_subplot(1,1,1);

	ax.set_xlabel(headers[xcol]);
	ax.set_ylabel(ylabel);

	box = ax.get_position()
	ax.set_position([box.x0, box.y0, box.width, box.height * 0.7])

	for ycol_header, y in ydict.items():
		line = list();
		for series_value, series in y.items():
			xy = map(list, zip(*series));
			if(scol_empty):
				line_label = ycol_header;
			else:
				line_label = ycol_header + ":" + headers[scol] + " = " + series_value;
			line.append((ax.plot(xy[0], xy[1], label=line_label))[0]);
	ax.set_title(title);
	extra_param_summary = "";
	for param, vals in extra_params.items():
		extra_param_summary += param + ": ";
		for val in vals:
			extra_param_summary += val + " ";
		extra_param_summary += "\n";
	ax.text(0.05, 0.95, extra_param_summary, transform=ax.transAxes, fontsize=12, verticalalignment='top');
	plt.legend(loc="upper center", bbox_to_anchor=(0.5,1.5));

	plt.ylim(ymin=0);
	fig.savefig(outfile);


def main():
	filename, scol, xcol, ycols, icols, title, ylabel, outfile = setup_optparse();
	graph(filename, scol, xcol, ycols, icols, title, ylabel, outfile);

main();
# Col 0 are the x points
# Col 1 is the series 50/100 marker
# Col 2 is the series cat data
# Col 3 is the series no cat data
