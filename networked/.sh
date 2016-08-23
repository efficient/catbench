# Validates system plugins to ensure they override the appropriate functions.

# Each set of modules should either define each of the following {variables,functions} or include their names in inherit_default_{init,impl}.
readonly REQUIRED_VAR_INITS="SERVER_DIR SERVER_BIN SERVER_MIN_REV CLIENT_DIR CLIENT_BIN CLIENT_MIN_REV CONTENDER_DIR CONTENDER_BIN CONTENDER_MIN_REV PERF_INIT_PHRASE SINGLETON_CONTENDER SPAWNCONTENDERS WARMUP_DURATION MAIN_DURATION EXPECTS_FILES"
readonly REQUIRED_FUN_IMPLS="genserverargs genclientargs gencontenderargs prephugepages awaitserverinit waitbeforeclient extracttput extractavelatency extractalllatencies extracttaillatency extractcontendertput oninit onwarmup onmainprocessing checkserverrev checkclientrev checkcontenderrev"

# These variables and functions don't need to be explicitly accounted for by the set of modules.
for implicit in EXPECTS_FILES checkserverrev checkclientrev checkcontenderrev
do
	if ! type "$implicit" >/dev/null 2>&1
	then
		if echo "$REQUIRED_VAR_INITS" | grep "\<$implicit\>" >/dev/null
		then
			inherit_default_init="$inherit_default_init $implicit"
		elif echo "$REQUIRED_FUN_IMPLS" | grep "\<$implicit\>" >/dev/null
		then
			inherit_default_impl="$inherit_default_impl $implicit"
		else
			echo "Internal error: Accepted module parameter '$implicit' is registered as neither a variable nor a function!"
			exit 1
		fi
	fi
done

if ! echo "$INDEPENDENT_VAR_WHITELIST" | grep "\<$independent\>" >/dev/null
then
	echo "$plugin: Module does not support varying parameter '$independent'!" >&2
	exit 1
fi

uhoh="false"

isfun() {
	local name="$1"
	[ "`type "$name" | sed -n '1{p;q}' | rev | cut -d" " -f1 | rev`" = "function" ]
}

isvar() (
	local name="$1"
	! isfun "$name" && eval echo '"${'"$name"'?}"' >/dev/null 2>&1
)

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
	SERVER_MIN_REV|CLIENT_MIN_REV|CONTENDER_MIN_REV)
		eval "$var"='""'
		;;
	CONTENDER_BIN)
		eval "$var"='"true"'
		;;
	EXPECTS_FILES)
		eval "$var"='""'
		;;
	SINGLETON_CONTENDER)
		eval "$var"='"false"'
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
			true
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
			grep -F "Average:" rtt_client | cut -d" " -f2
		}'
		;;
	extractalllatencies)
		eval "$fun()" '{
			grep -F "Completed after:" rtt_client | cut -d" " -f3
		}'
		;;
	extracttaillatency)
		eval "$fun()" '{
			local entries="$1"
			local percentile="$2"

			sed -n "`echo "$entries * $percentile / 100" | bc`{p;q}"
		}'
		;;
	extractcontendertput)
		eval "$fun()" '{
			echo 0
		}'
		;;
	checkserverrev)
		eval "$fun()" '(
			cd "$SERVER_DIR"
			[ "`git log --oneline --no-abbrev-commit | grep "^$SERVER_MIN_REV" | wc -l`" -eq "1" ]
		)'
		;;
	checkclientrev)
		eval "$fun()" '{
			[ "`ssh "$foreign_client" "
				cd \"$remote_path/$CLIENT_DIR\"
				git log --oneline --no-abbrev-commit" | grep "^$CLIENT_MIN_REV" | wc -l`" -eq "1" ]
		}'
		;;
	checkcontenderrev)
		eval "$fun()" '{
			echo "WARNING: Module specifies CONTENDER_MIN_REV but provides no checkcontenderrev implementation...?" >&2
			false
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
