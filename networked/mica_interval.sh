SERVER_DIR="../mica2-catbench/build"
SERVER_BIN="server"
CLIENT_DIR="../mica2-catbench/build"
CLIENT_BIN="netbench_interval"

const PREPOPULATE_TABLE="false"

INDEPENDENT_VAR_WHITELIST="num_trash cache_ways table_entries mite_tput_limit zipf_alpha"

PERF_INIT_PHRASE="tput="

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl extractavelatency"

SERVER_MIN_REV="ef82e16"
CLIENT_MIN_REV="50ece37"

genserverargs() {
	local multiplier="1"
	if type genclientargs_table_entries_multiplier >/dev/null 2>&1
	then
		multiplier="`genclientargs_table_entries_multiplier`"
	fi

	if "$PREPOPULATE_TABLE"
	then
		echo "'$((table_entries * multiplier))'"
	else
		true
	fi
}

genclientargs() {
	local multiplier="1"
	if type genclientargs_table_entries_multiplier >/dev/null 2>&1
	then
		multiplier="`genclientargs_table_entries_multiplier`"
	fi

	echo "-n '$MICA_RECORD_ITERATIONS' -o /dev/null '$((table_entries * multiplier))' '$MICA_GET_RATIO' '$zipf_alpha' '$mite_tput_limit'"
}

prephugepages() {
	"$SERVER_DIR/../script/setup.sh" 16384 0
}

awaitserverinit() {
	waitforalloc 1024
	sleep 3
}

waitbeforeclient() {
	while ! grep '^tput' rtt_server >/dev/null 2>&1; do sleep 1; done
}

extractaillatencies() {
	true
}

extracttput() {
	local all="`grep "tput=" rtt_client | tail -n"$MAIN_DURATION" | cut -d"=" -f2 | perl -nle 'print $1 if /(\S+) Mops/' | sort -n`"

	printf "%s\n" "$all" | tail -n+"$((MAIN_DURATION / 2))" | head -n1
}

extractalllatencies() {
	true
}

extracttaillatency() {
	local ign="$1"
	local percentile="$2"

	grep "^$percentile-th" rtt_client | cut -d" " -f2
}
