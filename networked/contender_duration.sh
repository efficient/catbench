SPAWNCONTENDERS="onmainprocessing"

const WARMUP_DURATION="3s"
const MAIN_DURATION="10s"

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl"

oninit() {
	true
}

onwarmup() {
	sleep "$WARMUP_DURATION"
}

onmainprocessing() {
	contender_pids=$(cat rtt_contender_pids | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')
	if [ -n "$contender_pids" ]
	then
		wait `cat rtt_contender_pids`
	else
		sleep "$MAIN_DURATION"
	fi
}
