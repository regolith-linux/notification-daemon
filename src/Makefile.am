libexec_PROGRAMS = notification-daemon

notification_daemon_SOURCES = \
	BaseNotification.cpp \
	ConsoleNotification.cpp \
	Image.cpp \
	PopupNotification.cpp \
	main.cpp \
	logging.cpp

notification_daemon_LDADD = $(PACKAGE_LIBS)

INCLUDES = \
	$(PACKAGE_CFLAGS)
