SPAWNCONTENDERS="onmainprocessing"

WARMUP_DURATION="3s"
MAIN_DURATION="contender"

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl"

oninit() {
	true
}

onwarmup() {
	sleep "$WARMUP_DURATION"
}

onmainprocessing() {
	wait `cat rtt_contender_pids`
	true
}
