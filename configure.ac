dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT(notification-daemon, 0.2.1.90, chipx86@chipx86.com)
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR(config.h.in)
AC_COPYRIGHT([Copyright 2004-2005 Christian Hammond])

dnl ################################################################
dnl # Version information
dnl ################################################################
NOTIFICATION_DAEMON_MAJOR_VERSION=0
NOTIFICATION_DAEMON_MINOR_VERSION=2
NOTIFICATION_DAEMON_MICRO_VERSION=1
NOTIFICATION_DAEMON_DEVEL_VERSION=90

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

pkg_modules="gtk+-2.0 >= 2.2.0, glib-2.0 >= 2.2.2, dbus-1 >= 0.20, dbus-glib-1 >= 0.20"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)


dnl
dnl Check the D-BUS version.
dnl

AC_MSG_CHECKING([dbus version])
DBUS_VERSION=`$PKG_CONFIG --modversion dbus-1`
DBUS_MAJOR_VER=`echo $DBUS_VERSION | cut -d. -f 1`
DBUS_MINOR_VER=`echo $DBUS_VERSION | cut -d. -f 2`
DBUS_MICRO_VER=`echo $DBUS_VERSION | cut -d. -f 3`

AC_MSG_RESULT($DBUS_VERSION)
AC_DEFINE_UNQUOTED(DBUS_MAJOR_VER, $DBUS_MAJOR_VER, [D-BUS major version.])
AC_DEFINE_UNQUOTED(DBUS_MINOR_VER, $DBUS_MINOR_VER, [D-BUS minor version.])
AC_DEFINE_UNQUOTED(DBUS_MICRO_VER, $DBUS_MICRO_VER, [D-BUS micro version.])

AC_DEFINE_UNQUOTED(DBUS_API_SUBJECT_TO_CHANGE, ,DBUS API is subject to change)


AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
AS_AC_EXPAND(LIBDIR, $libdir)
AS_AC_EXPAND(DATADIR, $datadir)

AC_ARG_WITH(dbus-sys, [  --with-dbus-sys=<dir>   where D-BUS system.d directory is])
AC_ARG_WITH(dbus-services, [  --with-dbus-services=<dir>   where D-BUS services directory is])

if ! test -z "$with_dbus_sys" ; then
	DBUS_SYS_DIR="$with_dbus_sys"
else
	DBUS_SYS_DIR="$SYSCONFDIR/dbus-1/system.d"
fi

if ! test -z "$with_dbus_services" ; then
	DBUS_SERVICES_DIR="$with_dbus_services"
else
	# D-BUS 0.23 and higher use $prefix/share/dbus-1/services
	# D-BUS 0.22 and lower use $prefix/lib/dbus-1/services
	if test "$DBUS_MAJOR_VER" -ge 1 -o \
	        "$DBUS_MINOR_VER" -ge 23; then
		DBUS_SERVICES_DIR="$DATADIR/dbus-1/services"
	else
		DBUS_SERVICES_DIR="$LIBDIR/dbus-1/services"
	fi
fi

AC_SUBST(DBUS_SYS_DIR)
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SYSTEMD_DIR, "$DBUS_SYS_DIR", [Where system.d dir for DBUS is])
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for DBUS is])

dnl # Use wall if we have GCC
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall"
	CXXFLAGS="$CXXFLAGS -Wall"
fi

AC_SUBST(CFLAGS)

dnl ################################################################
dnl # Output the Makefiles
dnl ################################################################
AC_CONFIG_FILES([
Makefile
notification-daemon.spec
src/Makefile
])

AC_OUTPUT

echo
echo $PACKAGE v$VERSION
echo

echo prefix............... : $prefix
echo dbus-1 system.d dir.. : $DBUS_SYS_DIR
echo dbus-1 services dir.. : $DBUS_SERVICES_DIR

echo
echo "Now type make to compile"
echo "Then su to root and type: make install"
echo
