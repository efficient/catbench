# Validates system plugins to ensure they override the appropriate functions.

readonly REQUIRED_VAR_INITS="SERVER_DIR SERVER_BIN CLIENT_DIR CLIENT_BIN CONTENDER_DIR CONTENDER_BIN PERF_INIT_PHRASE"
readonly REQUIRED_FUN_IMPLS="genserverargs genclientargs gencontenderargs prephugepages awaitserverinit waitbeforeclient extracttput extractavelatency extractalllatencies extracttaillatency"

uhoh="false"

isfun() {
	local name="$1"
	[ "`type "$name" | sed -n '1{p;q}' | rev | cut -d" " -f1 | rev`" = "function" ]
}

isvar() {
	local name="$1"
	eval [ -n '"$'"$name"'"' ]
}

for var in $inherit_default_init
do
	if isvar "$var"
	then
		echo "Internal error: Module requested default initialization of specified constant '$var'!" >&2
		uhoh="true"
		continue
	fi

	case "$var" in
	SERVER_DIR|CLIENT_DIR|CONTENDER_DIR)
		eval "$var"='"clients"'
		;;
	CONTENDER_BIN)
		eval "$var"='"square_evictions"'
		;;
	*)
		echo "Internal error: Module requested nonexistant default initialization for constant '$var'!" >&2
		uhoh="true"
		;;
	esac
done
unset inherit_default_init

for var in $REQUIRED_VAR_INITS
do
	if ! isvar "$var"
	then
		echo "Internal error: Module neither defines nor defers to default initialization of constant '$var'!" >&2
		uhoh="true"
		continue
	fi

	readonly "$var"
done
unset var

for fun in $inherit_default_impl
do
	if isfun "$fun"
	then
		echo "Internal error: Module requested default implementation of overridden function '$fun'!" >&2
		uhoh="true"
		continue
	fi

	case "$fun" in
	genserverargs|genclientargs)
		eval "$fun()" '{
			true
		}'
		;;
	gencontenderargs)
		eval "$fun()" '{
			printf %s -c "$TRASH_ALLOC" -e "$TRASH_ALLOC" -uhr
		}'
		;;
	prephugepages)
		eval "$fun()" '{
			sudo sh -c "echo 16384 >/proc/sys/vm/nr_hugepages"
		}'
		;;
	awaitserverinit)
		eval "$fun()" '{
			waitforalloc 0
		}'
		;;
	waitbeforeclient)
		eval "$fun()" '{
			true
		}'
		;;
	extractavelatency)
		eval "$fun()" '{
			grep -F 'Average:' rtt_client | cut -d" " -f2
		}'
		;;
	extractalllatencies)
		eval "$fun()" '{
			grep -F 'Completed after:' rtt_client | cut -d" " -f3
		}'
		;;
	extracttaillatency)
		eval "$fun()" '{
			local entries="$1"
			local percentile="$2"

			sed -n "`echo "$entries * $percentile / 100" | bc`{p;q}"
		}'
		;;
	*)
		echo "Internal error: Module requested nonexistant default implementation for function '$fun'!" >&2
		uhoh="true"
		;;
	esac
done
unset inherit_default_impl

for fun in $REQUIRED_FUN_IMPLS
do
	if ! isfun "$fun"
	then
		echo "Internal error: Module neither defines nor defers to default implementation of function '$fun'!" >&2
		uhoh="true"
	fi
done
unset fun

if "$uhoh"
then
	exit 1
fi
unset uhoh
