enginedir = $(libdir)/notification-daemon-1.0/engines

engine_LTLIBRARIES = libbubble.la

noinst_HEADERS = \
	eggnotificationbubblewidget.h

libbubble_la_SOURCES = \
	eggnotificationbubblewidget.c \
	theme.c

libbubble_la_LDFLAGS = -module -avoid-version

INCLUDES = \
	$(NOTIFICATION_DAEMON_CFLAGS)
