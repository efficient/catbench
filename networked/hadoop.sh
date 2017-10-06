CONTENDER_DIR="."
CONTENDER_BIN="hadoop_wrapper"
SINGLETON_CONTENDER="false"

CONTENDER_TPUT_UNIT="MB/s"

inherit_default_init="$inherit_default_init CONTENDER_MIN_REV"
inherit_default_impl="$inherit_default_impl"

TEXT_DIR="text/text_medium"
EXPECTS_FILES="$EXPECTS_FILES"

gencontenderargs() {
	echo "$TEXT_DIR"
}

extractcontendertput() {
	total_count=0
	total_time=0

	if [ "$SPAWNCONTENDERS" != "oninit" -a "$SPAWNCONTENDERS" != "onwarmup" ]
	then
		for contender in `seq "$contenders"`
		do
			in_file="rtt_contender_$((contender - 1))"

			count="$(grep 'Bytes Read= ' $in_file | sed -e 's/[:space:]*Bytes Read=\(.*\)/\1/')"
			total_count="$((total_count + count))"

			# parse the real elapsed time from "time -p"
			# use bc -l to handle floating-point numbers
			time="$(grep 'real ' $in_file | sed -e 's/real \(.*\)/\1/')"
			total_time="$(echo $total_time + $time | bc -l)"
		done
	fi

	if [ "$total_time" != "0" ]
	then
		echo "`du -sb $TEXT_DIR | cut -f1` * $contenders / $total_time" | bc -l
	else
		echo 0
	fi
}
