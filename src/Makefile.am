libexec_PROGRAMS = notification-daemon

notification_daemon_SOURCES = \
	nd-notification.c \
	nd-notification.h \
	nd-bubble.c \
	nd-bubble.h \
	nd-stack.c \
	nd-stack.h \
	nd-queue.c \
	nd-queue.h \
	daemon.c \
	daemon.h \
	sound.c \
	sound.h

notification_daemon_LDADD = $(NOTIFICATION_DAEMON_LIBS)

BUILT_SOURCES = notificationdaemon-dbus-glue.h

notificationdaemon-dbus-glue.h: notificationdaemon.xml
	dbus-binding-tool --mode=glib-server --prefix=notification_daemon \
		$(srcdir)/notificationdaemon.xml > notificationdaemon-dbus-glue.h

INCLUDES = \
	-I$(top_srcdir) \
	$(NOTIFICATION_DAEMON_CFLAGS) \
	-DENGINES_DIR=\"$(libdir)/notification-daemon-1.0/engines\"

EXTRA_DIST = notificationdaemon.xml
DISTCLEANFILES = \
	notificationdaemon-dbus-glue.h

-include $(top_srcdir)/git.mk
