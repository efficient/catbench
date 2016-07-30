#!/bin/sh

# gzip_wrapper.sh IN-PATH OUT-PATH [GZIP-OPTIONS]

input_path=$1
shift
output_path=$1
shift

input_size=$(stat --printf="%s" "$input_path")
echo "input size: $input_size"

output_size=$(gzip "$@" -c "$input_path" | tee "$output_path" | wc -c)
echo "output size: $output_size"

