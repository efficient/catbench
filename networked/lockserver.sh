SERVER_BIN="lockserver-ng"
CLIENT_BIN="lockclient"

PERF_INIT_PHRASE="Initialization complete!"

inherit_default_init="SERVER_DIR CLIENT_DIR CONTENDER_DIR CONTENDER_BIN"
inherit_default_impl="gencontenderargs prephugepages extractavelatency extractalllatencies extracttaillatency"

genserverargs() {
	echo "-l '$entries' -- -c 0x1"
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
