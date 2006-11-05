bin_PROGRAMS = notification-properties

@INTLTOOL_DESKTOP_RULE@

notification_properties_LDADD = $(NOTIFICATION_CAPPLET_LIBS)
notification_properties_SOURCES = notification-properties.c

gladedir = $(GNOMECC_GLADE_DIR)
glade_DATA = notification-properties.glade

desktopdir = $(datadir)/applications
desktop_in_files = notification-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

INCLUDES = \
	-I$(top_srcdir) \
	$(NOTIFICATION_CAPPLET_CFLAGS) \
	-DENGINES_DIR=\"$(libdir)/notification-daemon-1.0/engines\"

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = $(glade_DATA) $(pixmap_DATA)
