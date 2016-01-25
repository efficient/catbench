#!/usr/bin/perl

open($infile, "<", "$ARGV[0]");

@data_arr;
while(my $line = <$infile>) {
	if($line =~ /([\d\.]+)\s*seconds/) {
		push(@data_arr, $1);
	}
}

$sum = 0;
foreach(@data_arr) {
	$sum += $_;
}

print($sum / @data_arr);
print("\n");

