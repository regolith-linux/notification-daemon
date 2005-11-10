SUBDIRS = src

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	Doxyfile.in \
	autogen.sh \
	configure.ac \
	notification-daemon.service \
	notification-daemon.service.in

servicedir       = $(DBUS_SERVICES_DIR)
service_in_files = notification-daemon.service.in
service_DATA     = notification-daemon.service

%.service: %.service.in
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

