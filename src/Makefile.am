libexec_PROGRAMS = notification-daemon

notification_daemon_SOURCES = \
	BaseNotification.cpp \
	ConsoleNotification.cpp \
	Image.cpp \
	PopupNotification.cpp \
	logging.cpp \
	logging.h \
	main.cpp \
	notifier.h

notification_daemon_LDADD = $(PACKAGE_LIBS)

INCLUDES = \
	$(PACKAGE_CFLAGS)
