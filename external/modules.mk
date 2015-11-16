.PRECIOUS: external/%
external/%:
	if ! [ -d $(@D) ]; \
	then \
		git submodule update --init external/$(firstword $(subst /, ,$*)); \
		if [ -f external/$(firstword $(subst /, ,$*)).patch ]; \
		then \
			cd external/$(firstword $(subst /, ,$*)) && git apply ../$(firstword $(subst /, ,$*)).patch; \
		fi; \
	fi
	[ -e $@ ] || $(MAKE) -C $(@D) $(@F)
