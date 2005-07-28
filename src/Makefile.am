libexec_PROGRAMS = notification-daemon

SEXY_BASE = http://osiris.chipx86.com/svn/osiris-misc/trunk/libsexy
SEXY_FILES = \
	sexy-url-label.c \
	sexy-url-label.h

notification_daemon_SOURCES = \
	BaseNotifier.cc \
	BaseNotifier.hh \
	ConsoleNotifier.cc \
	ConsoleNotifier.hh \
	Hint.hh \
	Image.cc \
	Image.hh \
	Notification.cc \
	Notification.hh \
	PopupNotification.cc \
	PopupNotification.hh \
	PopupNotifier.cc \
	PopupNotifier.hh \
	dbus-compat.h \
	logging.cc \
	logging.hh \
	main.cc \
	$(SEXY_FILES)

notification_daemon_LDADD = $(PACKAGE_LIBS)

get-sexy:
	for i in $(SEXY_FILES); do \
		wget $(SEXY_BASE)/libsexy/$$i -O $$i; \
	done

INCLUDES = \
	$(PACKAGE_CFLAGS)
