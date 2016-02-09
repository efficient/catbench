#!/usr/bin/perl

# TODO: calculate average time per memory access?
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

$multiplier = 1;
if($ARGV[1] ne "") {
	$multiplier = $ARGV[1];
}

print($sum / @data_arr * $multiplier);
print("\n");

