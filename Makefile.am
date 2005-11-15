SUBDIRS = src

servicedir       = $(DBUS_SERVICES_DIR)
service_DATA     = notify-daemon.service

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	notify-daemon.service

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

