CONTENDER_BIN="square_evictions"
SINGLETON_CONTENDER="false"

inherit_default_init="$inherit_default_init CONTENDER_DIR CONTENDER_MIN_REV"
inherit_default_impl="$inherit_default_impl"

gencontenderargs() {
	if [ "$SPAWNCONTENDERS" = "oninit" -o "$SPAWNCONTENDERS" = "onwarmup" ]
	then
		printf "%s %s %s" -c"$TRASH_ALLOC" -e"$TRASH_ALLOC" -uhr
	else
		printf "%s %s %s %s %s" -c"$TRASH_ALLOC" -e"$TRASH_ALLOC" -whr -p 100
	fi
}

extractcontendertput() {
	total_accesses=0
	total_time=0

	if [ "$SPAWNCONTENDERS" != "oninit" -a "$SPAWNCONTENDERS" != "onwarmup" ]
	then
		for contender in `seq "$contenders"`
		do
			in_file="rtt_contender_$((contender - 1))"

			# add up all accesses
			for accesses in $(grep 'Accesses performed: ' $in_file | sed -e 's/Accesses performed: \(.*\)/\1/')
			do
				total_accesses="$((total_accesses + accesses))"
			done

			# parse the real elapsed time from "time -p"
			# use bc -l to handle floating-point numbers
			time="$(grep 'real ' $in_file | sed -e 's/real \(.*\)/\1/')"
			total_time="$(echo $total_time + $time | bc -l)"
		done
	fi

	if [ "$total_time" != "0" ]
	then
		echo $total_time / $total_time | bc -l
	else
		echo 0
	fi
}
