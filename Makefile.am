SUBDIRS = src

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac

dist-hook: notification-daemon.spec
	cp notification-daemon.spec $(distdir)
