SUBDIRS = data po src themes

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

