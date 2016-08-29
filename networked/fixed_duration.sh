SPAWNCONTENDERS="onwarmup"

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
	sleep "$MAIN_DURATION"
}
