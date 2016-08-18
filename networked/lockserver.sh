SERVER_BIN="lockserver-ng"
CLIENT_BIN="lockclient"

INDEPENDENT_VAR_WHITELIST="cache_ways client_sleep table_entries"

PERF_INIT_PHRASE="Initialization complete!"

inherit_default_init="$inherit_default_init SERVER_DIR SERVER_MIN_REV CLIENT_DIR CLIENT_MIN_REV"
inherit_default_impl="$inherit_default_impl prephugepages extractavelatency extractalllatencies extracttaillatency"

SERVER_MIN_REV=""
CLIENT_MIN_REV=""

genserverargs() {
	echo "-l '$table_entries' -- -c 0x1"
}

genclientargs() {
	echo "-m '$local_macaddr' -s '$client_sleep'"
}

awaitserverinit() {
	waitforalloc 0
	sleep 3
}

waitbeforeclient() {
	while ! grep "Initialization complete" >/dev/null 2>&1; do sleep 1; done
}

extracttput() {
	echo "0.0"
}
