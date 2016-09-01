genclientargs_table_entries_multiplier() {
	echo "mica_wss_percentage: Fixing table_entries PERCENTAGE instead of absolute value" >&2
	echo "$cache_ways" | tr [a-z] [A-Z] | sed 's/0X/ibase=16; obase=2; /' | bc | tr -d "0\n" | wc -c
}
