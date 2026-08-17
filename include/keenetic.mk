define Build/keenetic-header
	$(STAGING_DIR_HOST)/bin/keenetic-header v1 $(1) $@ $@.new
	mv $@.new $@
endef
