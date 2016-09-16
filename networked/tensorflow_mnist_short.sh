CONTENDER_DIR="."
CONTENDER_BIN="tensorflow_mnist_short"
SINGLETON_CONTENDER="true"

CONTENDER_TPUT_UNIT="epochs/s"

inherit_default_init="$inherit_default_init CONTENDER_MIN_REV"
inherit_default_impl="$inherit_default_impl"

gencontenderargs() {
	true
}

extractcontendertput() {
	total_count=1
	total_time=0

	if [ "$SPAWNCONTENDERS" != "oninit" -a "$SPAWNCONTENDERS" != "onwarmup" -a "$contenders" -ne 0 ]
	then
		in_file="rtt_contender_0"

		# parse the real elapsed time from "time -p"
		total_time="$(grep 'real ' $in_file | sed -e 's/real \(.*\)/\1/')"
	fi

	if [ "$total_time" != "0" ]
	then
		echo $total_count / $total_time | bc -l
	else
		echo 0
	fi
}
