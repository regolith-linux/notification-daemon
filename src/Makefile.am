NULL =

libexec_PROGRAMS = notification-daemon

notification_daemon_SOURCES = \
	nd-notification.c \
	nd-notification.h \
	nd-notification-box.c \
	nd-notification-box.h \
	nd-bubble.c \
	nd-bubble.h \
	nd-stack.c \
	nd-stack.h \
	nd-queue.c \
	nd-queue.h \
	daemon.c \
	daemon.h \
	sound.c \
	sound.h \
	$(NULL)

notification_daemon_LDADD = $(NOTIFICATION_DAEMON_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(NOTIFICATION_DAEMON_CFLAGS) \
	-DENGINES_DIR=\"$(libdir)/notification-daemon-1.0/engines\" \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

EXTRA_DIST = notificationdaemon.xml

-include $(top_srcdir)/git.mk
