INCLUDES=-I$(top_srcdir) $(NOTIFICATION_DAEMON_CFLAGS)

EXTRA_DIST=notifydaemon.xml

libexec_PROGRAMS = notify-daemon

notify_daemon_SOURCES=		\
	eggnotificationbubblewidget.c	\
	eggnotificationbubblewidget.h	\
	notifydaemon.c			\
	notifydaemon.h

notify_daemon_LDADD= $(NOTIFICATION_DAEMON_LIBS)

BUILT_SOURCES = notifydaemon-dbus-glue.h

notifydaemon-dbus-glue.h: notifydaemon.xml
	dbus-binding-tool --mode=glib-server notifydaemon.xml > notifydaemon-dbus-glue.h
