libexec_PROGRAMS = notification-daemon

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
	main.cc

notification_daemon_LDADD = $(PACKAGE_LIBS)

INCLUDES = \
	$(PACKAGE_CFLAGS)
