CONTENDER_DIR="."
CONTENDER_BIN="gzip_wrapper"
SINGLETON_CONTENDER="false"

inherit_default_init="$inherit_default_init"
inherit_default_impl="$inherit_default_impl"

GZ_FILENAME="linux-4.7.tar.gz"

gencontenderargs() {
	echo "$GZ_FILENAME /dev/null -d"
}

extractcontendertput() {
	total_filesize=0
	total_time=0

	if [ "$SPAWNCONTENDERS" != "oninit" -a "$SPAWNCONTENDERS" != "onwarmup" ]
	then
		for contender in `seq "$contenders"`
		do
			in_file="rtt_contender_$((contender - 1))"

			filesize="$(grep 'output size: ' $in_file | sed -e 's/output size: \(.*\)/\1/')"
			total_filesize="$((total_filesize + filesize))"

			# parse the real elapsed time from "time -p"
			# use bc -l to handle floating-point numbers
			time="$(grep 'real ' $in_file | sed -e 's/real \(.*\)/\1/')"
			total_time="$(echo $total_time + $time | bc -l)"
		done
	fi

	if [ "$total_time" != "0" ]
	then
		echo $total_filesize / $total_time | bc -l
	else
		echo 0
	fi
}
