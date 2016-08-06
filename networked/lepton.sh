CONTENDER_DIR="."
CONTENDER_BIN="lepton_wrapper"
SINGLETON_CONTENDER="false"

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl"

IMAGES_DIR="images"

gencontenderargs() {
	echo "$IMAGES_DIR -allowprogressive -singlethread -unjailed"
}

extractcontendertput() {
	total_count=0
	total_time=0

	if [ "$SPAWNCONTENDERS" != "oninit" -a "$SPAWNCONTENDERS" != "onwarmup" ]
	then
		for contender in `seq "$contenders"`
		do
			in_file="rtt_contender_$((contender - 1))"

			count="$(grep 'count: ' $in_file | sed -e 's/count: \(.*\)/\1/')"
			total_count="$((total_count + count))"

			# parse the real elapsed time from "time -p"
			# use bc -l to handle floating-point numbers
			time="$(grep 'real ' $in_file | sed -e 's/real \(.*\)/\1/')"
			total_time="$(echo $total_time + $time | bc -l)"
		done
	fi

	if [ "$total_time" != "0" ]
	then
		echo $total_count / $total_time | bc -l
	else
		echo 0
	fi
}
