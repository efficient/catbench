.PRECIOUS: $(dir $(lastword $(MAKEFILE_LIST)))%
$(dir $(lastword $(MAKEFILE_LIST)))%:
	if ! [ -f $(@D)/Makefile ]; \
	then \
		git submodule update --init $(dir $(lastword $(MAKEFILE_LIST)))$(firstword $(subst /, ,$*)); \
		if [ -f $(dir $(lastword $(MAKEFILE_LIST)))$(firstword $(subst /, ,$*)).patch ]; \
		then \
			cd $(dir $(lastword $(MAKEFILE_LIST)))$(firstword $(subst /, ,$*)) && git apply ../$(firstword $(subst /, ,$*)).patch; \
		fi; \
	fi
	[ -e $@ ] || $(MAKE) -C $(@D) $(@F)
