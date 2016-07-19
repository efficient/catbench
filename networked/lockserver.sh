SERVER_BIN="lockserver-ng"
CLIENT_BIN="lockclient"

PERF_INIT_PHRASE="Initialization complete!"

inherit_default_init="SERVER_DIR CLIENT_DIR CONTENDER_DIR CONTENDER_BIN"
inherit_default_impl="gencontenderargs prephugepages awaitserverinit waitbeforeclient extractavelatency extractalllatencies extracttaillatency"

genserverargs() {
	echo "-l '$entries' -- -c 0x1"
}

genclientargs() {
	echo "-m '$local_macaddr' -s '$client_sleep'"
}

extracttput() {
	echo "0.0"
}
