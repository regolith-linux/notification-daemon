/**
 * @file main.c Main GNotification Daemon file.
 *
 * Copyright (C) 2004 Christian Hammond.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <stdlib.h>

static DBusHandlerResult
filter_func(DBusConnection *dbus_conn, DBusMessage *message, void *user_data)
{
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int
main(int argc, char **argv)
{
	DBusConnection *dbus_conn;
	DBusError error;
	GMainLoop *loop;

	dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

	dbus_error_init(&error);

	if (dbus_conn == NULL)
	{
		fprintf(stderr, "Unable to get session bus: %s\n", error.message);
		exit(1);
	}

	dbus_connection_setup_with_g_main(dbus_conn, NULL);

	dbus_bus_acquire_service(dbus_conn, "org.freedesktop.Notifications",
							 0, &error);

	if (dbus_error_is_set(&error))
	{
		fprintf(stderr,
				"Unable to acquire service org.freedesktop.Notifications: %s\n",
				error.message);

		exit(1);
	}

	dbus_connection_add_filter(dbus_conn, filter_func, NULL, NULL);

	loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(loop);

	return 0;
}
