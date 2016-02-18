desktopdir          = $(datadir)/applications
desktop_in_in_files = notification-daemon.desktop.in.in
desktop_in_files    = notification-daemon.desktop.in
desktop_DATA        = $(desktop_in_files:.desktop.in=.desktop)

%.desktop.in: %.desktop.in.in
	$(AM_V_GEN) sed -e 's,[@]libexecdir[@],$(libexecdir),g' < $< > $@

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -o $@-t \
		-d $(top_srcdir)/po && mv $@-t $@

EXTRA_DIST = \
	$(desktop_in_in_files)

CLEANFILES = \
	$(desktop_in_files) \
	$(desktop_DATA)

-include $(top_srcdir)/git.mk
