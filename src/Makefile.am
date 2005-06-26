libexec_PROGRAMS = notification-daemon

notification_daemon_SOURCES = \
	BaseNotification.cpp \
	ConsoleNotification.cpp \
	Image.cpp \
	PopupNotification.cpp \
	PopupNotifier.cpp \
	dbus-compat.h \
	logging.cpp \
	logging.h \
	main.cpp \
	notifier.h \
	sexy-url-label.c

notification_daemon_LDADD = $(PACKAGE_LIBS)

SEXY_BASE = http://osiris.chipx86.com/svn/osiris-misc/trunk/libsexy
SEXY_FILES = \
	sexy-url-label.c \
	sexy-url-label.h

get-sexy:
	for i in $(SEXY_FILES); do \
		wget $(SEXY_BASE)/libsexy/$$i -O $$i; \
	done

INCLUDES = \
	$(PACKAGE_CFLAGS)
