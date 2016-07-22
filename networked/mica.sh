SERVER_DIR="../mica2-catbench/build"
SERVER_BIN="server"
CLIENT_DIR="../mica2-catbench/build"
CLIENT_BIN="netbench"

INDEPENDENT_VAR_WHITELIST="cache_ways mite_tput_limit zipf_alpha"

PERF_INIT_PHRASE="tput="

inherit_default_init="CONTENDER_DIR CONTENDER_BIN"
inherit_default_impl="genserverargs gencontenderargs extractavelatency"

genclientargs() {
	local num_iter="$MICA_NUM_ITERATIONS"
	if [ "$independent" = "mite_tput_limit" ]
	then
		num_iter="`perl -e "print int($MICA_NUM_ITERATIONS * $mite_tput_limit / 3)"`"
	fi
	local num_warm="`perl -e "print int($num_iter / 3)"`"

	echo "-n '$num_iter' -w '$num_warm' -o /dev/null -p '$MICA_NUM_ITEMS' '$MICA_GET_RATIO' '$alpha' '$mite_tput_limit'"
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
