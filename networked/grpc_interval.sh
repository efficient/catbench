SERVER_DIR="../grpc-catbench/build"
SERVER_BIN="greeter_async_server"
CLIENT_DIR="../grpc-catbench/build"
CLIENT_BIN="netbench"

const PREPOPULATE_TABLE="false"

INDEPENDENT_VAR_WHITELIST="num_trash cache_ways table_entries mite_tput_limit zipf_alpha"

PERF_INIT_PHRASE="grpc"

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl extractavelatency"

SERVER_MIN_REV="ef82e16"
CLIENT_MIN_REV="50ece37"

set -x

genserverargs() {
	local multiplier="1"
	#if type genclientargs_table_entries_multiplier >/dev/null 2>&1
	#then
	#	multiplier="`genclientargs_table_entries_multiplier`"
	#fi

#	if "$PREPOPULATE_TABLE"
#	then
#		echo "'$((table_entries * multiplier))'"
#	else
#		true
#	fi
}

genclientargs() {
	local multiplier="1"
	if type genclientargs_table_entries_multiplier >/dev/null 2>&1
	then
		multiplier="`genclientargs_table_entries_multiplier`"
	fi
	sleep 1

	echo "-n '$MICA_RECORD_ITERATIONS' -o /dev/null '$((table_entries * multiplier))' '$MICA_GET_RATIO' '$zipf_alpha' '$mite_tput_limit'"
}

prephugepages() {
	sleep 1
	#"$SERVER_DIR/../script/setup.sh" 16384 0
}

awaitserverinit() {
	#waitforalloc 1024
	sleep 1
}

waitbeforeclient() {
	while ! grep '^tput' rtt_server >/dev/null 2>&1; do sleep 1; done
}

extractaillatencies() {
	true
}

extracttput() {
	local all="`grep "tput=" rtt_client | tail -n"$MAIN_DURATION" | cut -d"=" -f2 | perl -nle 'print $1 if /(\S+) Mops/' | sort -n`"

	#printf "%s\n" "$all" | tail -n+"$((MAIN_DURATION / 2))" | head -n1
	echo 0
}

extractalllatencies() {
	true
}

extracttaillatency() {
	local ign="$1"
	local percentile="$2"

	grep "^$percentile-th" rtt_client | cut -d" " -f2
}
