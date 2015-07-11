NULL =

SUBDIRS = \
	data \
	src \
	po \
	$(NULL)

ACLOCAL_AMFLAGS = --install -I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST = \
	AUTHORS \
	NEWS \
	COPYING \
	README \
	ChangeLog \
	$(NULL)

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	`find "m4" -type f -name "*.m4" -print` \
	$(srcdir)/INSTALL \
	$(srcdir)/config.h.in~ \
	$(srcdir)/configure \
	$(NULL)

GITIGNOREFILES = $(PACKAGE)-\*.tar.{gz,bz2,xz}

-include $(top_srcdir)/git.mk
