/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2005 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <X11/Xproto.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include "daemon.h"
#include "nd-notification.h"
#include "nd-queue.h"
#include "notificationdaemon-dbus-glue.h"

#define MAX_NOTIFICATIONS 20

#define IDLE_SECONDS 30
#define NOTIFICATION_BUS_NAME      "org.freedesktop.Notifications"
#define NOTIFICATION_BUS_PATH      "/org/freedesktop/Notifications"

#define NW_GET_DAEMON(nw) \
        (g_object_get_data(G_OBJECT(nw), "_notify_daemon"))

struct _NotifyDaemonPrivate
{
        guint           exit_timeout_source;
        NdQueue        *queue;
};

static DBusConnection *dbus_conn = NULL;

static void notify_daemon_finalize (GObject *object);

G_DEFINE_TYPE (NotifyDaemon, notify_daemon, G_TYPE_OBJECT);

static void
notify_daemon_class_init (NotifyDaemonClass *daemon_class)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (daemon_class);

        object_class->finalize = notify_daemon_finalize;

        g_type_class_add_private (daemon_class, sizeof (NotifyDaemonPrivate));
}

static gboolean
do_exit (gpointer user_data)
{
        g_debug ("Exiting due to inactivity");
        exit (1);
        return FALSE;
}

static void
add_exit_timeout (NotifyDaemon *daemon)
{
        if (daemon->priv->exit_timeout_source > 0)
                return;

        daemon->priv->exit_timeout_source = g_timeout_add_seconds (IDLE_SECONDS, do_exit, NULL);
}

static void
remove_exit_timeout (NotifyDaemon *daemon)
{
        if (daemon->priv->exit_timeout_source == 0)
                return;

        g_source_remove (daemon->priv->exit_timeout_source);
        daemon->priv->exit_timeout_source = 0;
}

static void
on_queue_changed (NdQueue      *queue,
                 NotifyDaemon *daemon)
{
        if (nd_queue_length (queue) > 0) {
                remove_exit_timeout (daemon);
        } else {
                add_exit_timeout (daemon);
        }
}

static void
notify_daemon_init (NotifyDaemon *daemon)
{
        daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE (daemon,
                                                    NOTIFY_TYPE_DAEMON,
                                                    NotifyDaemonPrivate);

        daemon->priv->queue = nd_queue_new ();
        add_exit_timeout (daemon);
        g_signal_connect (daemon->priv->queue, "changed", G_CALLBACK (on_queue_changed), daemon);
}

static void
notify_daemon_finalize (GObject *object)
{
        NotifyDaemon *daemon;

        daemon = NOTIFY_DAEMON (object);

        remove_exit_timeout (daemon);

        g_object_unref (daemon->priv->queue);

        g_free (daemon->priv);

        G_OBJECT_CLASS (notify_daemon_parent_class)->finalize (object);
}

static DBusMessage *
create_signal_for_notification (NdNotification *notification,
                                const char     *signal_name)
{
        guint           id;
        const char     *dest;
        DBusMessage    *message;

        id = nd_notification_get_id (notification);
        dest = nd_notification_get_sender (notification);
        g_assert (dest != NULL);

        message = dbus_message_new_signal (NOTIFICATION_BUS_PATH,
                                           NOTIFICATION_BUS_NAME,
                                           signal_name);

        dbus_message_set_destination (message, dest);
        dbus_message_append_args (message,
                                  DBUS_TYPE_UINT32,
                                  &id,
                                  DBUS_TYPE_INVALID);

        return message;
}

GQuark
notify_daemon_error_quark (void)
{
        static GQuark   q = 0;

        if (q == 0)
                q = g_quark_from_static_string ("notification-daemon-error-quark");

        return q;
}

static void
on_notification_close (NdNotification *notification,
                       int             reason,
                       NotifyDaemon   *daemon)
{
        DBusMessage *message;

        message = create_signal_for_notification (notification, "NotificationClosed");
        dbus_message_append_args (message,
                                  DBUS_TYPE_UINT32,
                                  &reason,
                                  DBUS_TYPE_INVALID);
        dbus_connection_send (dbus_conn, message, NULL);
        dbus_message_unref (message);

        nd_queue_remove (daemon->priv->queue,
                         nd_notification_get_id (notification));
}

static void
on_notification_action_invoked (NdNotification *notification,
                                const char     *action,
                                NotifyDaemon   *daemon)
{
        guint           id;
        DBusMessage    *message;

        id = nd_notification_get_id (notification);
        message = create_signal_for_notification (notification, "ActionInvoked");
        dbus_message_append_args (message,
                                  DBUS_TYPE_STRING,
                                  &action,
                                  DBUS_TYPE_INVALID);

        dbus_connection_send (dbus_conn, message, NULL);
        dbus_message_unref (message);

        nd_notification_close (notification, ND_NOTIFICATION_CLOSED_USER);
}

gboolean
notify_daemon_notify_handler (NotifyDaemon *daemon,
                              const char   *app_name,
                              guint         id,
                              const char   *icon,
                              const char   *summary,
                              const char   *body,
                              char        **actions,
                              GHashTable   *hints,
                              int           timeout,
                              DBusGMethodInvocation *context)
{
        NotifyDaemonPrivate *priv = daemon->priv;
        NdNotification      *notification;

        if (nd_queue_length (priv->queue) > MAX_NOTIFICATIONS) {
                GError *error;

                error = g_error_new (notify_daemon_error_quark (),
                                     1,
                                     _("Exceeded maximum number of notifications"));
                dbus_g_method_return_error (context, error);
                g_error_free (error);

                return TRUE;
        }

        if (id > 0) {
                notification = nd_queue_lookup (priv->queue, id);
                if (notification == NULL) {
                        id = 0;
                } else {
                        g_object_ref (notification);
                }
        }

        if (id == 0) {
                char *sender;

                sender = dbus_g_method_get_sender (context);

                notification = nd_notification_new (sender);

                g_free (sender);
        }

        nd_notification_update (notification,
                                app_name,
                                icon,
                                summary,
                                body,
                                (const char **)actions,
                                hints,
                                timeout);
        g_signal_connect (notification, "closed", G_CALLBACK (on_notification_close), daemon);
        g_signal_connect (notification, "action-invoked", G_CALLBACK (on_notification_action_invoked), daemon);

        if (id == 0) {
                nd_queue_add (priv->queue, notification);
        }

        dbus_g_method_return (context, nd_notification_get_id (notification));

        g_object_unref (notification);

        return TRUE;
}

gboolean
notify_daemon_close_notification_handler (NotifyDaemon *daemon,
                                          guint         id,
                                          GError      **error)
{
        if (id == 0) {
                g_set_error (error,
                             notify_daemon_error_quark (),
                             100,
                             _("%u is not a valid notification ID"),
                             id);
                return FALSE;
        } else {
                NdNotification *notification;

                notification = nd_queue_lookup (daemon->priv->queue, id);
                if (notification != NULL) {
                        nd_notification_close (notification, ND_NOTIFICATION_CLOSED_API);
                }

                return TRUE;
        }
}

gboolean
notify_daemon_get_capabilities (NotifyDaemon *daemon,
                                char       ***caps)
{
        GPtrArray *a;
        char     **_caps;

        a = g_ptr_array_new ();
        g_ptr_array_add (a, g_strdup ("actions"));
        g_ptr_array_add (a, g_strdup ("body"));
        g_ptr_array_add (a, g_strdup ("body-hyperlinks"));
        g_ptr_array_add (a, g_strdup ("body-markup"));
        g_ptr_array_add (a, g_strdup ("icon-static"));
        g_ptr_array_add (a, g_strdup ("sound"));
        g_ptr_array_add (a, NULL);
        _caps = (char **) g_ptr_array_free (a, FALSE);

        *caps = _caps;

        return TRUE;
}

gboolean
notify_daemon_get_server_information (NotifyDaemon *daemon,
                                      char        **out_name,
                                      char        **out_vendor,
                                      char        **out_version,
                                      char        **out_spec_ver)
{
        *out_name = g_strdup ("Notification Daemon");
        *out_vendor = g_strdup ("GNOME");
        *out_version = g_strdup (PACKAGE_VERSION);
        *out_spec_ver = g_strdup ("1.1");

        return TRUE;
}

int
main (int argc, char **argv)
{
        NotifyDaemon    *daemon;
        DBusGConnection *connection;
        DBusGProxy      *bus_proxy;
        GError          *error;
        gboolean         res;
        guint            request_name_result;

        g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);

        gtk_init (&argc, &argv);

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                g_printerr ("Failed to open connection to bus: %s\n",
                            error->message);
                g_error_free (error);
                exit (1);
        }

        dbus_conn = dbus_g_connection_get_connection (connection);

        dbus_g_object_type_install_info (NOTIFY_TYPE_DAEMON,
                                         &dbus_glib_notification_daemon_object_info);

        bus_proxy = dbus_g_proxy_new_for_name (connection,
                                               "org.freedesktop.DBus",
                                               "/org/freedesktop/DBus",
                                               "org.freedesktop.DBus");

        res = dbus_g_proxy_call (bus_proxy,
                                 "RequestName",
                                 &error,
                                 G_TYPE_STRING, NOTIFICATION_BUS_NAME,
                                 G_TYPE_UINT, 0,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &request_name_result,
                                 G_TYPE_INVALID);
        if (! res
            || request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire name %s: %s",
                                   NOTIFICATION_BUS_NAME,
                                   error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire name %s", NOTIFICATION_BUS_NAME);
                }
                goto out;
        }

        daemon = g_object_new (NOTIFY_TYPE_DAEMON, NULL);

        dbus_g_connection_register_g_object (connection,
                                             "/org/freedesktop/Notifications",
                                             G_OBJECT (daemon));

        gtk_main ();

        g_object_unref (daemon);
 out:

        return 0;
}
