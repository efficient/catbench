SERVER_DIR="../mica2-catbench/build"
SERVER_BIN="server"
CLIENT_DIR="../mica2-catbench/build"
CLIENT_BIN="netbench_interval"

INDEPENDENT_VAR_WHITELIST="cache_ways table_entries mite_tput_limit zipf_alpha"

PERF_INIT_PHRASE="tput="

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl genserverargs extractavelatency"

SERVER_MIN_REV="ec10efe"
CLIENT_MIN_REV="50ece37"

genclientargs() {
	local multiplier="1"
	if type genclientargs_table_entries_multiplier >/dev/null 2>&1
	then
		multiplier="`genclientargs_table_entries_multiplier`"
	fi

	echo "-n '$MICA_RECORD_ITERATIONS' -o /dev/null -p '$((table_entries * multiplier))' '$MICA_GET_RATIO' '$zipf_alpha' '$mite_tput_limit'"
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
	grep "tput=" rtt_client | tail -n2 | head -n1 | cut -d"=" -f2 | perl -nle 'print $1 if /(\S+) Mops/'
}

extractalllatencies() {
	true
}

extracttaillatency() {
	local ign="$1"
	local percentile="$2"

	grep "$percentile-th" rtt_client | cut -d" " -f2
}