dnl **************************************************************************
dnl Define version info
dnl **************************************************************************

m4_define([nd_major_version], [3])
m4_define([nd_minor_version], [20])
m4_define([nd_micro_version], [0])
m4_define([nd_version], [nd_major_version.nd_minor_version.nd_micro_version])

dnl **************************************************************************
dnl Define bugzilla address
dnl **************************************************************************

m4_define([nd_bugzilla_url],
          [https://bugzilla.gnome.org/enter_bug.cgi?product=notification-daemon])

dnl **************************************************************************
dnl Initialize autoconf
dnl **************************************************************************

AC_INIT([notification-daemon], [nd_version], [nd_bugzilla_url])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

dnl **************************************************************************
dnl Set release policy
dnl **************************************************************************

AX_IS_RELEASE([git-directory])

dnl **************************************************************************
dnl Enable debug
dnl **************************************************************************

AX_CHECK_ENABLE_DEBUG([yes])

dnl **************************************************************************
dnl Add warning flags
dnl **************************************************************************

AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS])

dnl **************************************************************************
dnl Initialize automake
dnl **************************************************************************

AM_INIT_AUTOMAKE([1.13 no-dist-gzip dist-xz tar-ustar -Wno-portability])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

dnl **************************************************************************
dnl Check for required programs
dnl **************************************************************************

AC_PROG_CC

dnl **************************************************************************
dnl Initialize libtool
dnl **************************************************************************

LT_PREREQ([2.2.6])
LT_INIT

dnl **************************************************************************
dnl Internationalization
dnl **************************************************************************

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.4])

GETTEXT_PACKAGE=notification-daemon
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [Define the gettext package to be used])

dnl **************************************************************************
dnl Check forrequired packages
dnl **************************************************************************

GTK_REQUIRED=3.19.5
GLIB_REQUIRED=2.27.0

PKG_CHECK_MODULES([NOTIFICATION_DAEMON], [
  gtk+-3.0 >= $GTK_REQUIRED
  glib-2.0 >= $GLIB_REQUIRED
  gio-2.0 >= $GLIB_REQUIRED
  x11
])

dnl **************************************************************************
dnl Process .in files
dnl **************************************************************************

AC_CONFIG_FILES([
  Makefile

  data/Makefile

  po/Makefile.in

  src/Makefile
])

AC_OUTPUT

dnl **************************************************************************
dnl Show summary
dnl **************************************************************************

echo ""
echo "  Notification Daemon $VERSION"
echo "  `echo Notification Daemon $VERSION | sed "s/./=/g"`"
echo ""
echo "  srcdir ......................: ${srcdir}"
echo ""
echo "  compiler ....................: ${CC}"
echo "  cflags ......................: ${CFLAGS}"
echo ""
echo "  prefix ......................: ${prefix}"
echo "  exec_prefix .................: ${exec_prefix}"
echo "  bindir ......................: ${bindir}"
echo "  datadir .....................: ${datadir}"
echo ""
