CONTENDER_DIR="."
CONTENDER_BIN="bzip2_wrapper"
SINGLETON_CONTENDER="false"

CONTENDER_TPUT_UNIT="B/s"

inherit_default_init="$inherit_default_init CONTENDER_MIN_REV"
inherit_default_impl="$inherit_default_impl"

BZ_FILENAME="linux-4.7.tar"
EXPECTS_FILES="$EXPECTS_FILES \"$CONTENDER_DIR/$BZ_FILENAME\""

gencontenderargs() {
	echo "$BZ_FILENAME /dev/null -9"
}

extractcontendertput() {
	total_filesize=0
	total_time=0

	if [ "$SPAWNCONTENDERS" != "oninit" -a "$SPAWNCONTENDERS" != "onwarmup" ]
	then
		for contender in `seq "$contenders"`
		do
			in_file="rtt_contender_$((contender - 1))"

			filesize="$(grep 'input size: ' $in_file | sed -e 's/input size: \(.*\)/\1/')"
			total_filesize="$((total_filesize + filesize))"

			# parse the real elapsed time from "time -p"
			# use bc -l to handle floating-point numbers
			time="$(grep 'real ' $in_file | sed -e 's/real \(.*\)/\1/')"
			total_time="$(echo $total_time + $time | bc -l)"
		done
	fi

	if [ "$total_time" != "0" ]
	then
		echo "$total_filesize * $contenders / $total_time" | bc -l
	else
		echo 0
	fi
}
