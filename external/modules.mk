.PRECIOUS: external/%
external/%:
	[ -d $(@D) ] || git submodule update --init $(firstword $(subst /, ,$@))
	[ -e $@ ] || $(MAKE) -C $(@D) $(@F)
