SUBDIRS = data po src

# Point to our macro directory and pick up user flags from the environment
ACLOCAL_AMFLAGS  = -I macros ${ACLOCAL_FLAGS}

EXTRA_DIST = \
	AUTHORS NEWS INSTALL COPYING README \
	ChangeLog \
	autogen.sh \
	configure.ac

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.h.in~ \
	$(srcdir)/config.sub \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	`find "$(srcdir)" -type f -name Makefile.in -print` \
	$(srcdir)/configure \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/macros/gtk-doc.m4 \
	$(srcdir)/macros/intltool.m4 \
	$(srcdir)/macros/libtool.m4 \
	$(srcdir)/macros/ltoptions.m4 \
	$(srcdir)/macros/ltsugar.m4 \
	$(srcdir)/macros/ltversion.m4 \
	$(srcdir)/macros/lt~obsolete.m4

GITIGNOREFILES = $(PACKAGE)-\*.tar.{gz,bz2,xz}
-include $(top_srcdir)/git.mk
