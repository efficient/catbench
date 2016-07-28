CONTENDER_BIN="square_evictions"

inherit_default_init="$inherit_default_init CONTENDER_DIR"
inherit_default_impl="$inherit_default_impl"

gencontenderargs() {
	printf "%s %s %s" -c"$TRASH_ALLOC" -e"$TRASH_ALLOC" -uhr
}
