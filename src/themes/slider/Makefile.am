NULL =

enginedir = $(libdir)/notification-daemon-1.0/engines
engine_LTLIBRARIES = libslider.la

AM_CPPFLAGS = $(THEME_CFLAGS)

libslider_la_SOURCES = theme.c

libslider_la_LDFLAGS = -module -avoid-version
libslider_la_LIBADD =	\
	$(THEME_LIBS)	\
	$(NULL)

-include $(top_srcdir)/git.mk
