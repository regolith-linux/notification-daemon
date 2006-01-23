servicedir   = $(DBUS_SERVICES_DIR)
service_DATA = notification-daemon.service

schemasdir       = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = notification-daemon.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local:
	if test -z "$(DESTDIR)"; then \
		for p in $(schemas_DATA); do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p; \
		done \
	fi

EXTRA_DIST = \
	$(schemas_in_files) \
	notification-daemon.service

CLEANFILES = $(schemas_DATA)
