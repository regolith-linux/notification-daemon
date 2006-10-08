SUBDIRS = data po src

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update
