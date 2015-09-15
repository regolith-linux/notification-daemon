autostartdir          = $(sysconfdir)/xdg/autostart
autostart_in_in_files = notification-daemon-autostart.desktop.in.in
autostart_in_files    = notification-daemon-autostart.desktop.in
autostart_DATA        = $(autostart_in_files:.desktop.in=.desktop)

desktopdir          = $(datadir)/applications
desktop_in_in_files = notification-daemon.desktop.in.in
desktop_in_files    = notification-daemon.desktop.in
desktop_DATA        = $(desktop_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -o $@-t \
		-d $(top_srcdir)/po && mv $@-t $@

EXTRA_DIST = \
	$(autostart_in_in_files) \
	$(desktop_in_in_files)

CLEANFILES = \
	$(autostart_DATA) \
	$(desktop_DATA)

DISTCLEANFILES = \
	$(autostart_in_files) \
	$(desktop_in_files)

-include $(top_srcdir)/git.mk
