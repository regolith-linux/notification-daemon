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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
static GMainLoop *loop;
static DBusConnection *dbus_conn;

static DBusMessage*
handle_notify(DBusConnection *incoming, DBusMessage *message)
{
    DBusError *error;
    DBusMessageIter iter;
    char *str;
    Notification *n;
    uint replaces;

    /* we could probably use get_args here, at a cost of less fine grained error reporting */
    
    dbus_message_iter_init(message, &iter);

#define type dbus_message_iter_get_arg_type(&iter)

    /* replaces */
    validate( type == DBUS_TYPE_UINT32, NULL, "invalid notify message, first argument (replaces) is not a uint32\n");
    replaces = dbus_message_iter_get_uint32(&iter);
    dbus_message_iter_next(&iter);

    if (replaces == 0) {
        n = backend->create_notification();
        n->connection = incoming;
    } else {
        TRACE("replaces=%d\n", replaces);

        n = backend->get(replaces);
        validate( n != NULL, NULL, "invalid replacement ID (%d) given\n", replaces );
    }

    validate( n->connection != NULL, NULL, "backend is not set on notification\n" );
    
    /* urgency */
    validate( type == DBUS_TYPE_BYTE, NULL,
              "invalid notify message, second argument (urgency) is not a byte\n" );
    n->urgency = dbus_message_iter_get_byte(&iter);
    dbus_message_iter_next(&iter);

    /* summary */
    validate( type == DBUS_TYPE_STRING, NULL,
              "invalid notify message, third argument (summary) is not a string\n" );

    str = dbus_message_iter_get_string(&iter);
    n->summary = strdup(str);
    dbus_free(str);
    dbus_message_iter_next(&iter);

    /* body, can be NIL */
    validate( (type == DBUS_TYPE_STRING) || (type == DBUS_TYPE_NIL), NULL,
              "invalid notify message, fourth argument (body) is not string nor nil\n" );

    if (type != DBUS_TYPE_NIL)
    {
        str = dbus_message_iter_get_string(&iter);
        n->body = strdup(str);
        dbus_free(str);
    }

    dbus_message_iter_next(&iter);

    /* images, array */
    dbus_message_iter_next(&iter);  // FIXME: skip this for now

    /* sound: string or NIL */
    validate( (type == DBUS_TYPE_STRING) || (type == DBUS_TYPE_NIL), NULL,
              "invalid notify message, sixth argument (sound) is not string nor nil\n" );

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
              "invalid notify message, eigth argument (timeout) is not int32 nor nil (%d)\n", type );

    if (type != DBUS_TYPE_NIL) {
        n->use_timeout = true;
        n->timeout = dbus_message_iter_get_uint32(&iter);
    }

    if (replaces) backend->update(n);

    int id = replaces ? replaces : backend->notify(n);

    TRACE("id is %d\n", id);

    DBusMessage *reply = dbus_message_new_method_return(message);
    assert( reply != NULL );

    dbus_message_append_args(reply, DBUS_TYPE_UINT32, id, DBUS_TYPE_INVALID);

#undef type

    return reply;
}

static DBusMessage*
handle_get_caps(DBusMessage *message)
{
    DBusMessage *reply = dbus_message_new_method_return(message);

    DBusMessageIter iter;
    dbus_message_iter_init(reply, &iter);

    static const char* caps[] = { "body", "markup" };
    dbus_message_iter_append_string_array(&iter, caps, sizeof(caps) / sizeof(caps[0]));

    return reply;
}

static DBusMessage*
handle_get_info(DBusMessage *message)
{
    DBusMessage *reply = dbus_message_new_method_return(message);

    DBusMessageIter iter;
    dbus_message_iter_init(reply, &iter);

    dbus_message_iter_append_string(&iter, "freedesktop.org Reference Implementation server");
    dbus_message_iter_append_string(&iter, "freedesktop.org");
    dbus_message_iter_append_string(&iter, "1.0");

    return reply;
}

static DBusMessage*
handle_close(DBusMessage *message)
{
    uint id;
    DBusError error;
    
    if (!dbus_message_get_args(message, &error, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID)) {
        FIXME("error parsing message args but not propogating\n");
        return NULL;
    }

    backend->unnotify(id);

    return NULL;
}

static DBusHandlerResult
filter_func(DBusConnection *conn, DBusMessage *message, void *user_data)
{
    int message_type = dbus_message_get_type(message);

    if (message_type == DBUS_MESSAGE_TYPE_ERROR) {
        WARN("Error received: %s\n", dbus_message_get_error_name(message));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (message_type == DBUS_MESSAGE_TYPE_SIGNAL) {
        if (equal(dbus_message_get_member(message), "ServiceAcquired"))
            return DBUS_HANDLER_RESULT_HANDLED;

        WARN("Received signal (%d)\n", message_type);

        // FIXME: dbus-send sends messages as signals not as methods
        // return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    TRACE("method=%s\n", dbus_message_get_member(message));

    const char *s = dbus_message_get_path(message);
    validate( equal(s, "/org/freedesktop/Notifications"),
              DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
              "message received on unknown object '%s'\n", s );

    s = dbus_message_get_interface(message);
    validate( equal(s, "org.freedesktop.Notifications"),
              DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
              "unknown message received: %s.%s\n",
              s, dbus_message_get_member(message) );


    /* now we know it's on the only valid interface, dispatch the method call */
    DBusMessage *ret = NULL;

    if (equal(dbus_message_get_member(message), "Notify")) ret = handle_notify(conn, message);
    else if (equal(dbus_message_get_member(message), "GetCapabilities")) ret = handle_get_caps(message);
    else if (equal(dbus_message_get_member(message), "GetServerInfo")) ret = handle_get_info(message);
    else if (equal(dbus_message_get_member(message), "CloseNotification")) ret = handle_close(message);
    else return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
    /* we always reply to messages, even if it's just empty */
    if (!ret) ret = dbus_message_new_method_return(message);
    
    dbus_connection_send(conn, ret, NULL);
    dbus_message_unref(ret);

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

    if (name == "console") backend = new ConsoleNotifier(loop);
    else if (name == "popup") backend = new PopupNotifier(loop, argc, argv);
    else {
        fprintf(stderr, "%s: unknown backend specified: %s\n", envvar);
        exit(1);
    }
}


int
main(int argc, char **argv)
{

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

    loop = g_main_loop_new(NULL, FALSE);

    initialize_backend(&argc, &argv);

    TRACE("started\n");

    g_main_loop_run(loop);

    return 0;
}

