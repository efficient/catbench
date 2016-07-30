#!/bin/sh

# lepton_wrapper.sh IN-DIR-PATH [LEPTON-OPTIONS]

input_dir_path=$1
shift

count=0

for filename in "$input_dir_path/"*
do
	lepton "$@" - < "$filename" > /dev/null
	if [ "$?" -eq 0 ]; then count="$((count + 1))"; fi
done

echo "count: $count"

