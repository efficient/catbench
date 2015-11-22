tool=""
toolargs=""

driver_init() {
	builddeps "$1"
	startlogging "$2"
}

builddeps() {
	if ! make "$1"
	then
		echo "Failed to build target utility!"
		exit 1
	fi
	if ! make -C.. external/pqos/pqos
	then
		echo "Failed to build pqos utility!"
		exit 1
	fi
}

startlogging() {
	exec >"$1" 2>&1
}

logstatus() {
	runlog git log --oneline -1
	runlog git status -uno
	runlog git diff HEAD
	echo ===
	echo

	runlog sudo ../external/pqos/pqos -s
}

runtrial() {
	local aftymask="$1"
	shift

	if [ -n "$tool" ]
	then
		eval runlog sudo sh -c \'PATH=\"$PATH\" pcm.x $toolargs -- taskset \"$aftymask\" "$@"\'
	else
		runlog perf stat -e cache-misses,cache-references $toolargs taskset "$aftymask" "$@"
	fi
}

runlog() {
	echo "\$ $@"
	"$@"
}
