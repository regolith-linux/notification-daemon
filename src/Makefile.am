INCLUDES = -I$(top_srcdir) $(NOTIFICATION_DAEMON_CFLAGS)

EXTRA_DIST = notificationdaemon.xml

libexec_PROGRAMS = notification-daemon

notification_daemon_SOURCES =		\
	eggnotificationbubblewidget.c	\
	eggnotificationbubblewidget.h	\
	daemon.c			\
	daemon.h

notification_daemon_LDADD = $(NOTIFICATION_DAEMON_LIBS)

BUILT_SOURCES = notificationdaemon-dbus-glue.h

notificationdaemon-dbus-glue.h: notificationdaemon.xml
	dbus-binding-tool --mode=glib-server $(srcdir)/notificationdaemon.xml > notificationdaemon-dbus-glue.h
