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

notification_daemon_CFLAGS = \
	-I$(top_srcdir) \
	-DLOCALE_DIR=\""$(datadir)/locale"\" \
	$(NOTIFICATION_DAEMON_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

notification_daemon_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

EXTRA_DIST = notificationdaemon.xml

-include $(top_srcdir)/git.mk
