/** -*- mode: c++-mode; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; -*-
 * @file main.cpp Main Notification Daemon file.
 *
 * Copyright (C) 2004 Christian Hammond.
 *               2004 Mike Hearn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * TODO:
 *  required:
 *    - caps query
 *    - timeouts
 *    - image support
 *  optional:
 *    - popup sliding/notification
 *    - animated images
 *    - KNotify proxy
 *
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
using std::string;

#include "notifier.h"
#include "logging.h"

BaseNotifier *backend;
GMainLoop *loop;

static bool
handle_initial_messages(DBusMessage *message)
{
	if (equal(dbus_message_get_member(message), "ServiceAcquired")) return true;
	return false;
}

static DBusMessage*
dispatch_notify(DBusMessage *message)
{
	DBusError *error;
	DBusMessageIter iter;
	char *str;
	Notification *n = backend->create_notification();

	dbus_message_iter_init(message, &iter);

#define type dbus_message_iter_get_arg_type(&iter)

	/* urgency */
	validate( type == DBUS_TYPE_BYTE, NULL,
			  "invalid notify message, first argument is not a byte\n" );
	n->urgency = dbus_message_iter_get_byte(&iter);
	dbus_message_iter_next(&iter);

	/* summary */
	validate( type == DBUS_TYPE_STRING, NULL,
			  "invalid notify message, second argument is not a string\n" );

	str = dbus_message_iter_get_string(&iter);
	n->summary = strdup(str);
	dbus_free(str);
	dbus_message_iter_next(&iter);

	/* body, can be NIL */
	validate( (type == DBUS_TYPE_STRING) || (type == DBUS_TYPE_NIL), NULL,
			  "invalid notify message, third argument is not string nor nil\n" );

	if (type != DBUS_TYPE_NIL)
	{
		str = dbus_message_iter_get_string(&iter);
		n->body = strdup(str);
		dbus_free(str);
	}

	dbus_message_iter_next(&iter);

	/* images, array */
	dbus_message_iter_next(&iter);	// FIXME: skip this for now

	/* sound: string or NIL */
	validate( (type == DBUS_TYPE_STRING) || (type == DBUS_TYPE_NIL), NULL,
			  "invalid notify message, fifth argument is not string nor nil\n" );

	if (type != DBUS_TYPE_NIL)
	{
		str = dbus_message_iter_get_string(&iter);
		n->sound = strdup(str);
		dbus_free(str);
	}
	dbus_message_iter_next(&iter);

	/* actions */
	dbus_message_iter_next(&iter); // FIXME: skip this for now

	/* timeout, UINT32 or NIL for no timeout */
	validate( (type == DBUS_TYPE_UINT32) || (type == DBUS_TYPE_NIL), NULL,
			  "invalid notify message, seventh argument is not int32 nor nil (%d)\n", type );

	if (type == DBUS_TYPE_NIL) n->use_timeout = false;
	else n->timeout = dbus_message_iter_get_uint32(&iter);

	int id = backend->notify(n);

	DBusMessage *reply = dbus_message_new_method_return(message);
	assert( reply != NULL );

	dbus_message_append_args(reply, DBUS_TYPE_UINT32, id, DBUS_TYPE_INVALID);
	
#undef type

	return reply;
}

static DBusHandlerResult
filter_func(DBusConnection *dbus_conn, DBusMessage *message, void *user_data)
{
	/* some quick checks that apply to all backends */
	if (handle_initial_messages(message)) return DBUS_HANDLER_RESULT_HANDLED;

	string s;

	s = dbus_message_get_path(message);
	validate( s == "/org/freedesktop/Notifications",
			  DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
			  "message received on unknown object '%s'\n", $(s) );


	s = dbus_message_get_interface(message);
	validate( s == "org.freedesktop.Notifications",
			  DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
			  "unknown message received: %s.%s\n",
			  $(s), dbus_message_get_member(message) );


	/* now we know it's on the only valid interface, dispatch the method call */
	string method = dbus_message_get_member(message);

	TRACE("dispatching %s\n", $(method));

	DBusMessage *ret = NULL;

	if (method == "Notify") ret = dispatch_notify(message);
	else return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (ret) dbus_connection_send(dbus_conn, ret, NULL);
	
	// FIXME: return the reply message here
	return DBUS_HANDLER_RESULT_HANDLED;
}


void
initialize_backend(int *argc, char ***argv)
{
	/* Currently, the backend desired is chosen using an environment variable.
	   In future, we could try and figure out the best backend to use in a smarter
	   way, but let's not get too wrapped up in this. The most common backend will
	   almost certainly be either PopupNotifier or CompositedPopupNotifier.
	 */

	char *envvar = getenv("NOTIFICATION_DAEMON_BACKEND");
	string name = envvar ? envvar : "popup";

	if (name == "console") backend = new ConsoleNotifier();
	else if (name == "popup") backend = new PopupNotifier(loop, argc, argv);
	else {
		fprintf(stderr, "%s: unknown backend specified: %s\n", envvar);
		exit(1);
	}
}


int
main(int argc, char **argv)
{
	DBusConnection *dbus_conn;
	DBusError error;


	dbus_error_init(&error);

	dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (dbus_conn == NULL)
	{
		fprintf(stderr, "%s: unable to get session bus: %s, perhaps you need to start DBUS?\n",
				getenv("_"), error.message);
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

	initialize_backend(&argc, &argv);


	TRACE("started\n");

	loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(loop);

	return 0;
}

