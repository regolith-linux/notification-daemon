SUBDIRS = src

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	notify-daemon.service \
	notify-daemon.service.in

servicedir       = $(DBUS_SERVICES_DIR)
service_in_files = notify-daemon.service.in
service_DATA     = notify-daemon.service

%.service: %.service.in
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

