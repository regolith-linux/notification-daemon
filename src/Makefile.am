NULL =

libexec_PROGRAMS = \
	notification-daemon \
	$(NULL)

notification_daemon_SOURCES = \
	nd-bubble.c \
	nd-bubble.h \
	nd-daemon.c \
	nd-daemon.h \
	nd-main.c \
	nd-notification.c \
	nd-notification.h \
	nd-notification-box.c \
	nd-notification-box.h \
	nd-queue.c \
	nd-queue.h \
	nd-stack.c \
	nd-stack.h \
	$(BUILT_SOURCES) \
	$(NULL)

notification_daemon_CFLAGS = \
	-DLOCALE_DIR=\""$(localedir)"\" \
	-I$(top_builddir)/src \
	$(NOTIFICATION_DAEMON_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

notification_daemon_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

notification_daemon_LDADD = \
	$(NOTIFICATION_DAEMON_LIBS) \
	$(NULL)

nd-fd-notifications.h:
nd-fd-notifications.c: org.freedesktop.Notifications.xml
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix org.freedesktop.Notifications \
		--c-namespace Nd \
		--generate-c-code nd-fd-notifications \
		$(srcdir)/org.freedesktop.Notifications.xml

BUILT_SOURCES = \
	nd-fd-notifications.c \
	nd-fd-notifications.h \
	$(NULL)

EXTRA_DIST = \
	org.freedesktop.Notifications.xml \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

-include $(top_srcdir)/git.mk
