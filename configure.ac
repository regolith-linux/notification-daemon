dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT(notify-daemon, 0.3.0, johnp@redhat.com)
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR(config.h.in)
AC_COPYRIGHT([Copyright 2005 John (J5) Palmieri])


dnl ################################################################
dnl # Version information
dnl ################################################################
NOTIFY_DAEMON_MAJOR_VERSION=0
NOTIFY_DAEMON_MINOR_VERSION=3
NOTIFY_DAEMON_MICRO_VERSION=0
NOTIFY_DAEMON_DEVEL_VERSION=0

NOTIFY_DAEMON_VERSION=$NOTIFY_DAEMON_MAJOR_VERSION.$NOTIFY_DAEMON_MINOR_VERSION.$NOTIFY_DAEMON_MICRO_VERSION

if test "x$NOTIFY_DAEMON_DEVEL_VERSION" != "x0"; then
	if test "x$NOTIFY_DAEMON_MICRO_VERSION" = "x0"; then
		NOTIFY_DAEMON_MICRO_VERSION=$NOTIFY_DAEMON_DEVEL_VERSION
		NOTIFY_DAEMON_VERSION=$NOTIFY_DAEMON_MAJOR_VERSION.$NOTIFY_DAEMON_MINOR_VERSION.$NOTIFY_DAEMON_DEVEL_VERSION
	else
		NOTIFY_DAEMON_VERSION=$NOTIFY_DAEMON_VERSION.$NOTIFY_DAEMON_DEVEL_VERSION
	fi
fi

AC_DEFINE_UNQUOTED(NOTIFY_DAEMON_MAJOR_VERSION, $NOTIFY_DAEMON_MAJOR_VERSION,
	[notify-daemon major version.])
AC_DEFINE_UNQUOTED(NOTIFY_DAEMON_MINOR_VERSION, $NOTIFY_DAEMON_MINOR_VERSION,
	[notify-daemon minor version.])
AC_DEFINE_UNQUOTED(NOTIFY_DAEMON_MICRO_VERSION, $NOTIFY_DAEMON_MICRO_VERSION,
	[notify-daemon micro version.])
AC_DEFINE_UNQUOTED(NOTIFY_DAEMON_VERSION, "$NOTIFY_DAEMON_VERSION",
	[notify-daemon version.])

dnl ################################################################
dnl # Initialize automake
dnl ################################################################
VERSION=$NOTIFY_DAEMON_VERSION
PACKAGE=notify-daemon

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

dnl ################################################################
dnl # Specify a header configuration file
dnl ################################################################
AM_CONFIG_HEADER(config.h)

dnl ################################################################
dnl # Check for some standard stuff.
dnl ################################################################
AC_PROG_CC
#AC_ARG_PROGRAM
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

AC_C_CONST
AC_TYPE_SIZE_T
AC_EXEEXT

AM_PROG_LIBTOOL

AC_CHECK_LIB([popt], [poptGetArg], , AC_MSG_ERROR([Popt is required]))

pkg_modules="gtk+-2.0 >= 2.2.2 glib-2.0 >= 2.2.2, dbus-1 >= 0.35, dbus-glib-1 >= 0.35"
PKG_CHECK_MODULES(NOTIFY_DAEMON, [$pkg_modules])
AC_SUBST(NOTIFY_DAEMON_CFLAGS)
AC_SUBST(NOTIFY_DAEMON_LIBS)

gdk_modules="gdk-2.0 gdk-pixbuf-2.0"
PKG_CHECK_MODULES(GDK, [$gdk_modules], have_gdk=yes,
[
	have_gdk=no

	AC_WARN("Some test apps will not be built")
])

AM_CONDITIONAL(HAVE_GDK, test "x$have_gdk" = "xyes")

AC_SUBST(GDK_CFLAGS)
AC_SUBST(GDK_LIBS)

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
AS_AC_EXPAND(LIBEXECDIR, $libexecdir)

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
fi

AC_SUBST(CFLAGS)

dnl ################################################################
dnl # Output the Makefiles
dnl ################################################################
AC_CONFIG_FILES([
Makefile
notify-daemon.service
src/Makefile
])

AC_OUTPUT

echo
echo $PACKAGE v$VERSION

echo prefix............... : $prefix
echo dbus-1 system.d dir.. : $DBUS_SYS_DIR
echo dbus-1 services dir.. : $DBUS_SERVICES_DIR

echo "Now type make to compile"
echo "Then su to root and type: make install"
echo

