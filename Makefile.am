SUBDIRS = src

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	notification-daemon.service.in

CLEANFILES = notification-daemon.service

servicedir       = $(DBUS_SERVICES_DIR)
service_in_files = notification-daemon.service.in
service_DATA     = notification-daemon.service

%.service: %.service.in
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

dist-hook: notification-daemon.spec
	cp notification-daemon.spec $(distdir)
