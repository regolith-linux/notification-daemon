bin_PROGRAMS = notification-properties

@INTLTOOL_DESKTOP_RULE@

notification_properties_LDADD   = $(NOTIFICATION_CAPPLET_LIBS)
notification_properties_SOURCES = notification-properties.c

uidir = $(datadir)/notification-daemon
ui_DATA = notification-properties.ui

desktopdir = $(datadir)/applications
desktop_in_files = notification-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/daemon \
	$(NOTIFICATION_CAPPLET_CFLAGS) \
	-DENGINES_DIR=\"$(libdir)/notification-daemon-1.0/engines\" \
	-DNOTIFICATION_LOCALEDIR=\"$(datadir)/locale\" \
	-DNOTIFICATION_UIDIR=\"$(datadir)/notification-daemon\"

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = $(ui_DATA) $(pixmap_DATA) $(desktop_in_files)

-include $(top_srcdir)/git.mk
