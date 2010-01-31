enginedir = $(libdir)/notification-daemon-1.0/engines

engine_LTLIBRARIES = libstandard.la

libstandard_la_SOURCES = theme.c
libstandard_la_LDFLAGS = -module -avoid-version
libstandard_la_LIBADD  = $(NOTIFICATION_DAEMON_LIBS)

INCLUDES = $(NOTIFICATION_DAEMON_CFLAGS)

-include $(top_srcdir)/git.mk
