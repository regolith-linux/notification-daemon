m4_define([nd_major_version], [3])
m4_define([nd_minor_version], [17])
m4_define([nd_micro_version], [2])

AC_INIT([notification-daemon], [nd_major_version.nd_minor_version.nd_micro_version],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=notification-daemon])

AX_IS_RELEASE([git-directory])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz tar-ustar -Wno-portability])
AM_MAINTAINER_MODE([[enable]])
AM_SILENT_RULES([yes])

AX_CHECK_ENABLE_DEBUG([yes])

# Checks for programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS])

dnl ---------------------------------------------------------------------------
dnl Initialize Libtool
dnl ---------------------------------------------------------------------------
LT_PREREQ([2.2.6])
LT_INIT


dnl ---------------------------------------------------------------------------
dnl i18n stuff
dnl ---------------------------------------------------------------------------
IT_PROG_INTLTOOL([0.40.0])

AC_SUBST([GETTEXT_PACKAGE], [notification-daemon])
AM_GLIB_GNU_GETTEXT
AC_DEFINE([GETTEXT_PACKAGE], [PACKAGE_TARNAME], [Define to the gettext package name.])


dnl ---------------------------------------------------------------------------
dnl Requirements for the daemon
dnl ---------------------------------------------------------------------------
REQ_GTK_VERSION=3.15.2
REQ_GLIB_VERSION=2.27.0
REQ_LIBCANBERRA_GTK_VERSION=0.4
pkg_modules="
	gtk+-3.0 >= $REQ_GTK_VERSION, \
	glib-2.0 >= $REQ_GLIB_VERSION, \
        gio-2.0 >= $REQ_GLIB_VERSION, \
        libcanberra-gtk3 >= $REQ_LIBCANBERRA_GTK_VERSION, \
        x11 \
"
PKG_CHECK_MODULES(NOTIFICATION_DAEMON, $pkg_modules)
AC_SUBST(NOTIFICATION_DAEMON_CFLAGS)
AC_SUBST(NOTIFICATION_DAEMON_LIBS)

AS_AC_EXPAND(LIBEXECDIR, $libexecdir)

AC_CONFIG_FILES([
Makefile
data/Makefile
data/notification-daemon.desktop.in
data/notification-daemon-autostart.desktop.in
po/Makefile.in
src/Makefile
])

AC_OUTPUT

echo "
    notification-daemon $VERSION
    `echo notification-daemon $VERSION | sed "s/./=/g"`

    prefix:               ${prefix}
    exec_prefix:          ${exec_prefix}

    bindir:               ${bindir}
    datadir:              ${datadir}

    source code location: ${srcdir}
    compiler:             ${CC}

"
