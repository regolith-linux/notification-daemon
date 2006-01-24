enginedir = $(libdir)/notification-daemon-1.0/engines

engine_LTLIBRARIES = libstandard.la

libstandard_la_SOURCES = \
	bgbox.c \
	bgbox.h \
	theme.c

libstandard_la_LIBADD = $(NOTIFICATION_DAEMON_LIBS)

INCLUDES = $(NOTIFICATION_DAEMON_CFLAGS)
