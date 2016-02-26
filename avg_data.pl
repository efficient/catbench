#!/usr/bin/perl

# TODO: calculate average time per memory access?
open($infile, "<", "$ARGV[0]");

$unit = "seconds";
if($ARGV[1] ne "") {
	$unit = $ARGV[1];
}

@data_arr;
while(my $line = <$infile>) {
	if($line =~ /([\d\.]+)\s*$unit/) {
		push(@data_arr, $1);
	}
}

$sum = 0;
foreach(@data_arr) {
	$sum += $_;
}

print($sum / @data_arr);
print("\n");

