dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT(notification-daemon, 0.0.1, chipx86@gnupdate.org)
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR(config.h.in)
AC_COPYRIGHT([Copyright 2004 Christian Hammond])

dnl ################################################################
dnl # Version information
dnl ################################################################
NOTIFICATION_DAEMON_MAJOR_VERSION=0
NOTIFICATION_DAEMON_MINOR_VERSION=0
NOTIFICATION_DAEMON_MICRO_VERSION=1
NOTIFICATION_DAEMON_DEVEL_VERSION=0

NOTIFICATION_DAEMON_VERSION=$NOTIFICATION_DAEMON_MAJOR_VERSION.$NOTIFICATION_DAEMON_MINOR_VERSION.$NOTIFICATION_DAEMON_MICRO_VERSION

if test "x$NOTIFICATION_DAEMON_DEVEL_VERSION" != "x0"; then
	if test "x$NOTIFICATION_DAEMON_MICRO_VERSION" = "x0"; then
		NOTIFICATION_DAEMON_MICRO_VERSION=$NOTIFICATION_DAEMON_DEVEL_VERSION
		NOTIFICATION_DAEMON_VERSION=$NOTIFICATION_DAEMON_MAJOR_VERSION.$NOTIFICATION_DAEMON_MINOR_VERSION.$NOTIFICATION_DAEMON_DEVEL_VERSION
	else
		NOTIFICATION_DAEMON_VERSION=$NOTIFICATION_DAEMON_VERSION.$NOTIFICATION_DAEMON_DEVEL_VERSION
	fi
fi

dnl ################################################################
dnl # Initialize automake
dnl ################################################################
VERSION=$NOTIFICATION_DAEMON_VERSION
PACKAGE=notification-daemon

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

dnl ################################################################
dnl # Specify a header configuration file
dnl ################################################################
AM_CONFIG_HEADER(config.h)

dnl ################################################################
dnl # Check for some standard stuff.
dnl ################################################################
AC_PROG_CC
AC_PROG_CXX
#AC_ARG_PROGRAM
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

AC_C_CONST
AC_TYPE_SIZE_T
AC_EXEEXT

pkg_modules="gtk-2.0 >= 2.2.0, glib-2.0 >= 2.2.2, dbus-1 >= 0.20, dbus-glib-1 >= 0.20"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_DEFINE_UNQUOTED(DBUS_API_SUBJECT_TO_CHANGE, ,DBUS API is subject to change)

dnl # Use wall if we have GCC
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall"
fi

AC_SUBST(CFLAGS)

dnl ################################################################
dnl # Output the Makefiles
dnl ################################################################
AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT

echo
echo $PACKAGE v$VERSION

echo
echo "Now type make to compile"
echo "Then su to root and type: make install"
echo
