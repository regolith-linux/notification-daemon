SUBDIRS = src

dbusdir   = $(DBUS_SYS_DIR)
dbus_DATA = notification-daemon.conf

servicedir       = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Notifications.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

dist-hook: notification-daemon.spec
	cp notification-daemon.spec $(distdir)

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	notification-daemon.spec.in \
	$(dbus_DATA) \
	$(service_in_files)

CLEANFILES = $(service_DATA)
