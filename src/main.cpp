/** -*- mode: c++-mode; tab-width: 4; c-basic-offset: 4; -*-
 * @file main.cpp Main Notification Daemon file.
 *
 * Copyright (C) 2004 Christian Hammond.
 *             2004 Mike Hearn
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

/*
 *
 * TODO:
 *  required:
 *  - image support
 *  - valgrind it
 *  - full markup support
 *  - visual urgency styles
 *
 *  naughty-but-nice:
 *  - popup sliding/notification
 *  - animated images
 *  - make more C++ish, ie use bindings/replace validate() with exceptions,
 *    etc
 *  - global hotkey to clear bottommost notification
 *  - set ctrl-c handler to close active notifications
 *
 * We should maybe make use of the DBUS and GTK+ C++ bindings in order to
 * reduce the C-ness of this program. Right now we're using lots of C
 * strings and pointers and such. It works, but it'd be nice to have a
 * more OO natural API to work with. Downsides? Makes it harder to
 * build/install and increases memory usage. We already have problems
 * with swap time. Probably not worth it. Also makes it harder to get
 * accepted into various desktops ...
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <iostream>
#include <exception>
#include <stdexcept>
#include <memory>

#include "notifier.h"
#include "logging.h"

#define equal(s1, s2) (strcmp(s1, s2) == 0)

BaseNotifier *backend;
static GMainLoop *loop;
static DBusConnection *dbus_conn;

static DBusMessage *
handle_notify(DBusConnection *incoming, DBusMessage *message)
{
    DBusMessageIter iter;
    DBusMessage *reply;
    char *str;
    Notification *n;
    uint replaces;
    /* if we create a new notification, ensure it'll be freed if we throw  */
    std::auto_ptr<Notification> n_holder;

    /*
     * We could probably use get_args here, at a cost of less fine grained
     * error reporting
     */
    dbus_message_iter_init(message, &iter);

#define type dbus_message_iter_get_arg_type(&iter)


    /*********************************************************************
     * App Name
     *********************************************************************/
    validate(type == DBUS_TYPE_STRING || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. app name argument is "
             "not a string or nil\n");
    dbus_message_iter_next(&iter);

    /*********************************************************************
     * App Icon
     *********************************************************************/
    validate(type == DBUS_TYPE_ARRAY || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. app icon argument is "
             "not a string or nil\n");
    dbus_message_iter_next(&iter);
    
    /*********************************************************************
     * Replaces ID
     *********************************************************************/
    validate(type == DBUS_TYPE_UINT32, NULL,
             "Invalid notify message. replaces argument is not a uint32\n");
    replaces = dbus_message_iter_get_uint32(&iter);
    dbus_message_iter_next(&iter);

    if (replaces == 0)
    {
        n = backend->create_notification();
        n->connection = incoming;

        n_holder.reset(n);
    }
    else
    {
        TRACE("replaces = %d\n", replaces);

        n = backend->get(replaces);
        validate(n != NULL, NULL, "Invalid replacement ID (%d) given\n",
                 replaces );
    }

    validate(n->connection != NULL, NULL,
             "Backend is not set on notification\n" );

    /*********************************************************************
     * Notification type
     *********************************************************************/
    validate(type == DBUS_TYPE_STRING || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. Type argument is not a string or nil\n");
    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Urgency level
     *********************************************************************/
    validate(type == DBUS_TYPE_BYTE, NULL,
             "Invalid notify message. Urgency argument is not a byte\n" );
    n->urgency = dbus_message_iter_get_byte(&iter);
    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Summary
     *********************************************************************/
    validate(type == DBUS_TYPE_STRING, NULL,
             "Invalid notify message. Summary argument is not a string\n");

    str = dbus_message_iter_get_string(&iter);
    n->summary = strdup(str);
    dbus_free(str);
    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Body
     *********************************************************************/
    validate(type == DBUS_TYPE_STRING || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. Body argument is not string nor nil\n");

    if (type != DBUS_TYPE_NIL)
    {
        str = dbus_message_iter_get_string(&iter);
        n->body = strdup(str);
        dbus_free(str);
    }

    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Images
     *********************************************************************/
    validate(type == DBUS_TYPE_ARRAY || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. Images argument is not an "
             "array or nil\n" );

    if (type != DBUS_TYPE_NIL)
    {
        DBusMessageIter arrayiter;
        dbus_message_iter_init_array_iterator(&iter, &arrayiter, NULL);
    
        int arraytype = dbus_message_iter_get_array_type(&iter);

        if ((arraytype == DBUS_TYPE_STRING) || (arraytype == DBUS_TYPE_ARRAY))
        {
            /* yes, the dbus api is this bad. they may look like java iterators, but they aren't */
            if (dbus_message_iter_get_arg_type(&arrayiter) != DBUS_TYPE_INVALID)
            {
                do
                {
                    dbus_message_iter_next(&arrayiter);
            
                    if (arraytype == DBUS_TYPE_STRING)
                    {
                        char *s = dbus_message_iter_get_string(&arrayiter);

                        n->images.push_back(new Image(s));
                
                        dbus_free(s);
                    }
                    else if (arraytype == DBUS_TYPE_ARRAY)
                    {
                        unsigned char *data;
                        int len;

                        if (! dbus_message_iter_get_byte_array(&arrayiter, &data, &len) )
                            throw std::runtime_error( "could not retrieve marshalled image" );

                        n->images.push_back(new Image(data, len));
                    }
                } while (dbus_message_iter_next(&arrayiter));
            }

            TRACE("There are %d images\n", n->images.size());
        
        }
    }
    

    dbus_message_iter_next(&iter);
    
    /*********************************************************************
     * Actions
     *********************************************************************/
    validate(type == DBUS_TYPE_DICT || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. Actions argument is not dict or nil\n" );

    if (type == DBUS_TYPE_DICT)
    {
        DBusMessageIter action_iter;

        if (dbus_message_iter_init_dict_iterator(&iter, &action_iter))
        {
            do
            {
                /*
                 * Confusingly on the wire, the dict maps action text to ID,
                 * whereas internally we map the id to the action text.
                 */
                char *key     = dbus_message_iter_get_dict_key(&action_iter);
                uint actionid = dbus_message_iter_get_uint32(&action_iter);

                TRACE("demarshal: action %d : %s\n", actionid, key);

                n->actions[actionid] = strdup(key);
                dbus_free(key);
            } while (dbus_message_iter_next(&action_iter));
        }
    }

    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Hints
     *********************************************************************/
    validate(type == DBUS_TYPE_DICT || type == DBUS_TYPE_NIL, NULL,
             "Invalid notify message. Hints argument is not dict or nil\n" );

    if (type != DBUS_TYPE_NIL)
    {
        n->use_timeout = true;
        n->timeout = dbus_message_iter_get_uint32(&iter);
    }
    
    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Expires
     *********************************************************************/
    validate(type == DBUS_TYPE_BOOLEAN, NULL,
             "Invalid notify message. Expires argument is not uint32\n");

    n->use_timeout = true;
    dbus_message_iter_next(&iter);

    /*********************************************************************
     * Expiration Timeout
     *********************************************************************/
    validate(type == DBUS_TYPE_UINT32, NULL,
             "Invalid notify message. Timeout argument is not uint32\n");

    n->timeout = dbus_message_iter_get_uint32(&iter);
    dbus_message_iter_next(&iter);

#undef type
    
    /* end of demarshalling code  */
    
    if (replaces) backend->update(n);
    uint id = replaces ? replaces : backend->notify(n);

    n_holder.release();  /* Commit the notification  */

    reply = dbus_message_new_method_return(message);

    dbus_message_append_args(reply,
                             DBUS_TYPE_UINT32, id,
                             DBUS_TYPE_INVALID);

    return reply;
}

/* XXX This should probably delegate to the notifier backend  */
static DBusMessage *
handle_get_caps(DBusMessage *message)
{
    DBusMessage *reply = dbus_message_new_method_return(message);
    DBusMessageIter iter;
    static const char *caps[] = { "body", "actions", "static-image" };

    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_append_string_array(&iter, caps, G_N_ELEMENTS(caps));

    return reply;
}

static DBusMessage *
handle_get_info(DBusMessage *message)
{
    DBusMessage *reply = dbus_message_new_method_return(message);
    DBusMessageIter iter;

    dbus_message_iter_init(reply, &iter);

    dbus_message_iter_append_string(&iter,
        "freedesktop.org Reference Implementation server");
    dbus_message_iter_append_string(&iter, "freedesktop.org");
    dbus_message_iter_append_string(&iter, "0.1.0");

    return reply;
}

static DBusMessage *
handle_close(DBusMessage *message)
{
    uint id;

    if (!dbus_message_get_args(message, NULL,
                               DBUS_TYPE_UINT32, &id,
                               DBUS_TYPE_INVALID))
    {
        FIXME("error parsing message args but not propogating\n");
        return NULL;
    }

    TRACE("closing notification %d\n", id);

    backend->unnotify(id);

    return NULL;
}

static DBusHandlerResult
filter_func(DBusConnection *conn, DBusMessage *message, void *user_data)
{
    int message_type = dbus_message_get_type(message);
    const char *s;
    DBusMessage *ret = NULL;

    if (message_type == DBUS_MESSAGE_TYPE_ERROR)
    {
        WARN("Error received: %s\n", dbus_message_get_error_name(message));
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (message_type == DBUS_MESSAGE_TYPE_SIGNAL)
    {
        const char *member = dbus_message_get_member(message);
        
        if (equal(member, "ServiceAcquired"))
            return DBUS_HANDLER_RESULT_HANDLED;

        WARN("Received signal %s\n", member);

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    TRACE("method = %s\n", dbus_message_get_member(message));

    s = dbus_message_get_path(message);

    validate(equal(s, "/org/freedesktop/Notifications"),
             DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
             "message received on unknown object '%s'\n", s );

    s = dbus_message_get_interface(message);

    validate(equal(s, "org.freedesktop.Notifications"),
             DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
             "unknown message received: %s.%s\n",
             s, dbus_message_get_member(message) );

    try
    {
        if (equal(dbus_message_get_member(message), "Notify")) ret = handle_notify(conn, message);
        else if (equal(dbus_message_get_member(message), "GetCapabilities")) ret = handle_get_caps(message);
        else if (equal(dbus_message_get_member(message), "GetServerInfo")) ret = handle_get_info(message);
        else if (equal(dbus_message_get_member(message), "CloseNotification")) ret = handle_close(message);
        else return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "notification-daemon: ** exception thrown while processing "
                  << dbus_message_get_member(message) << " request: " << e.what() << "\n";
        ret = dbus_message_new_error(message, "org.freedesktop.Notifications.Error", e.what());
    }
    
    /* we always reply to messages, even if it's just empty */
    if (!ret) ret = dbus_message_new_method_return(message);

    TRACE("created return message\n");
    
    dbus_connection_send(conn, ret, NULL);
    dbus_message_unref(ret);
    dbus_message_unref(message);

    TRACE("sent reply\n");
    
    return DBUS_HANDLER_RESULT_HANDLED;
}


void
initialize_backend(int *argc, char ***argv)
{
    /*
     * Currently, the backend desired is chosen using an environment variable.
     * In future, we could try and figure out the best backend to use in a
     * smarter way, but let's not get too wrapped up in this. The most common
     * backend will almost certainly be either PopupNotifier or
     * CompositedPopupNotifier.
     */
    char *envvar = getenv("NOTIFICATION_DAEMON_BACKEND");
    string name = envvar ? envvar : "popup";

    if (name == "console")
        backend = new ConsoleNotifier(loop);
    else if (name == "popup")
        backend = new PopupNotifier(loop, argc, argv);
    else
    {
        fprintf(stderr, "%s: unknown backend specified: %s\n", envvar);
        exit(1);
    }
}


int
main(int argc, char **argv)
{
    std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
    DBusError error;

    dbus_error_init(&error);

    dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

    if (dbus_conn == NULL)
    {
        fprintf(stderr, "%s: unable to get session bus: %s, perhaps you "
                "need to start DBUS?\n",
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
