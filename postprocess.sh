reuse() {
	local key="$1"
	local type="$2"
	local dest="$key"
	if [ $# -ge 3 ]
	then
		dest="$3"
	fi
	jaguar/jaguar set "$outfile" "$dest" "$type" "`jaguar/jaguar get "$infile" "$key"`"
}

bounds() {
	local series="$1"
	local index="$2"
	jaguar/jaguar get "$infile" "data.$series.samples[$elem]" >/dev/null 2>&1
}

sample() {
	local field="$1"
	local series="$2"
	local index="$3"
	jaguar/jaguar get "$infile" "data.$series.samples[$index].$field"
}

decode() {
	local metadata="$1"
	jaguar/jaguar get "$infile" "meta.$metadata" | base64 -d
}

confirm() {
	local msg="$1"
	local approved
	echo -n "$msg (y/N)? "
	read approved
	[ "$approved" = "y" ]
}

if [ $# -ne 2 ]
then
	echo "USAGE: $0 <infile> <outfile>"
	exit 2
fi
infile="$1"
outfile="$2"

if [ ! -e "$infile" ]
then
	echo "$infile: No such file or directory" >&2
	exit 3
fi
if [ -e "$outfile" ]
then
	echo "$outfile: File exists and would be clobbered" >&2
	exit 4
fi

cp jaguar/template_postprocessed.json "$outfile"
if jaguar/jaguar get "$infile" meta.origfile >/dev/null 2>&1
then
	reuse meta.origfile string
else
	jaguar/jaguar set "$outfile" meta.origfile "`md5sum "$infile" | cut -d" " -f1`"
fi
