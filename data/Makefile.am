autostartdir          = $(sysconfdir)/xdg/autostart
autostart_in_in_files = notification-daemon.desktop.in.in
autostart_in_files    = notification-daemon.desktop.in
autostart_DATA        = $(autostart_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(autostart_in_in_files)

CLEANFILES = \
	$(autostart_DATA)

DISTCLEANFILES = \
	$(autostart_in_files)

-include $(top_srcdir)/git.mk
