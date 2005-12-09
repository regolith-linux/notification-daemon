/* notifydaemon.c - Implementation of the destop notification spec
 *
 * Copyright (C) 2005 John (J5) Palmieri <johnp@redhat.com>
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
#include "notifydaemon.h"
#include "eggnotificationbubblewidget.h"
#include "notifydaemon-dbus-glue.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <gtk/gtk.h>

struct _NotifyTimeout
{
   GTimeVal expiration;
   gboolean has_timeout;
   guint id;

   EggNotificationBubbleWidget *widget;
};

typedef struct _NotifyTimeout NotifyTimeout;

struct _NotifyDaemonPrivate
{
  guint next_id;
  guint timeout_source;
  GHashTable *notification_hash;
};

static void notify_daemon_finalize (GObject * object);
static void _emit_closed_signal (GObject *notify_widget);

G_DEFINE_TYPE (NotifyDaemon, notify_daemon, G_TYPE_OBJECT);

static void
notify_daemon_class_init (NotifyDaemonClass * daemon_class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (daemon_class);

  object_class->finalize = notify_daemon_finalize;

  g_type_class_add_private (daemon_class, sizeof (NotifyDaemonPrivate));
}

static void
_notify_timeout_destroy (NotifyTimeout *nt)
{
  gtk_widget_destroy ((GtkWidget *)nt->widget);
  g_free (nt);
}

static void
notify_daemon_init (NotifyDaemon * daemon)
{
  daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE (daemon,
					      NOTIFY_TYPE_DAEMON,
					      NotifyDaemonPrivate);

  daemon->priv->next_id = 1;
  daemon->priv->timeout_source = 0;
  daemon->priv->notification_hash = g_hash_table_new_full ((GHashFunc) g_int_hash, 
                                                           (GEqualFunc) g_int_equal,
                                                           (GDestroyNotify) g_free,
                                                           (GDestroyNotify) _notify_timeout_destroy);
}

static void
notify_daemon_finalize (GObject * object)
{
  NotifyDaemon *daemon;
  GObjectClass *parent_class;

  daemon = NOTIFY_DAEMON (object);

  g_hash_table_destroy (daemon->priv->notification_hash);

  parent_class = G_OBJECT_CLASS (notify_daemon_parent_class);

  if (parent_class->finalize != NULL)
    parent_class->finalize (object);
}

NotifyDaemon *
notify_daemon_new (void)
{
  NotifyDaemon *daemon;

  daemon = g_object_new (NOTIFY_TYPE_DAEMON, NULL);

  return daemon;
}

static void
_emit_action_invoked_signal (GObject *notify_widget, gchar *action)
{
  DBusConnection *con;
  DBusError error;

  dbus_error_init (&error);

  con = dbus_bus_get (DBUS_BUS_SESSION, &error);

  if (con == NULL)
    {
      g_warning ("Error sending ActionInvoked signal: %s", error.message);
      dbus_error_free (&error);
    }
  else
    {
      DBusMessage *message;
          
      gchar *dest;
      guint id;
        
      message = dbus_message_new_signal ("/org/freedesktop/Notifications", 
                                         "org.freedesktop.Notifications",
                                         "ActionInvoked");

      dest = g_object_get_data (notify_widget,
                                "_notify_sender");
      id = GPOINTER_TO_UINT (g_object_get_data (notify_widget,
                                                "_notify_id"));

      g_assert (dest != NULL);

      dbus_message_set_destination (message, dest); 
      dbus_message_append_args (message, 
                                DBUS_TYPE_UINT32, &id, 
                                DBUS_TYPE_STRING, &action,
                                DBUS_TYPE_INVALID);
                                 
      dbus_connection_send (con, message, NULL);
     
      dbus_message_unref (message);
      dbus_connection_unref (con);
    }
}

static void
_emit_closed_signal (GObject *notify_widget)
{
  DBusConnection *con;
  DBusError error;

  dbus_error_init (&error);

  con = dbus_bus_get (DBUS_BUS_SESSION, &error);

  if (con == NULL)
    {
      g_warning ("Error sending Close signal: %s", error.message);
      dbus_error_free (&error);
    }
  else
    {
      DBusMessage *message;
          
      gchar *dest;
      guint id;
        
      message = dbus_message_new_signal ("/org/freedesktop/Notifications", 
                                         "org.freedesktop.Notifications",
                                         "NotificationClosed");

      dest = g_object_get_data (notify_widget,
                                "_notify_sender");
      id = GPOINTER_TO_UINT (g_object_get_data (notify_widget,
                                                "_notify_id"));

      g_assert (dest != NULL);

      dbus_message_set_destination (message, dest); 
      dbus_message_append_args (message, 
                                DBUS_TYPE_UINT32, &id,
                                DBUS_TYPE_INVALID);
                                 
      dbus_connection_send (con, message, NULL);
     
      dbus_message_unref (message);
      dbus_connection_unref (con);
    }
}

static void
_close_notification (NotifyDaemon *daemon, 
                     guint id)
{
  NotifyDaemonPrivate *priv;
  NotifyTimeout *nt;

  priv = daemon->priv;

  nt = (NotifyTimeout *)
          g_hash_table_lookup (priv->notification_hash, &id);

  if (nt)
    {
      _emit_closed_signal (G_OBJECT (nt->widget));

      egg_notification_bubble_widget_hide (nt->widget);
      g_hash_table_remove (priv->notification_hash, &id);
    }
}


static gboolean
_is_expired (gpointer key,
             gpointer value,
             gpointer data)
{
  NotifyTimeout *nt;
  gboolean *phas_more_timeouts;
  GTimeVal now;
  GTimeVal expiration;

  nt = (NotifyTimeout *) value;
  phas_more_timeouts = data;

  if (!nt->has_timeout)
    return FALSE;

  g_get_current_time (&now);
  expiration = nt->expiration;

  if (now.tv_sec > expiration.tv_sec)
    {
      _emit_closed_signal (G_OBJECT (nt->widget));
      return TRUE;
    }
  else if (now.tv_sec == expiration.tv_sec)
    {
      if (now.tv_usec > expiration.tv_usec)
        {
          _emit_closed_signal (G_OBJECT (nt->widget));
          return TRUE;
        }
    }

  *phas_more_timeouts = TRUE;
  
  return FALSE;
}

static gboolean
_check_expiration (gpointer data)
{
  NotifyDaemon *daemon;
  gboolean has_more_timeouts;

  has_more_timeouts = FALSE;

  daemon = (NotifyDaemon *) data;

  g_hash_table_foreach_remove (daemon->priv->notification_hash, 
                                (GHRFunc) _is_expired, 
                                (gpointer) &has_more_timeouts);

  if (!has_more_timeouts)
    daemon->priv->timeout_source = 0;

  return has_more_timeouts;

}

static void
_calculate_timeout (NotifyDaemon *daemon, NotifyTimeout *nt, int timeout)
{
  if (timeout == 0)
      nt->has_timeout = FALSE;
  else
    {
      gulong usec;
    
      nt->has_timeout = TRUE;
      if (timeout == -1)
        timeout = NOTIFY_DAEMON_DEFAULT_TIMEOUT;

      usec = timeout * 1000; /* convert from msec to usec */
      g_get_current_time (&nt->expiration);
      g_time_val_add (&nt->expiration, usec);
      
      if (daemon->priv->timeout_source == 0)
        daemon->priv->timeout_source = 
          g_timeout_add (500, 
                         (GSourceFunc) _check_expiration, 
                         (gpointer) daemon);
    }

}

static guint
_store_notification (NotifyDaemon *daemon, 
                     EggNotificationBubbleWidget *bw,
                     int timeout)
{
  NotifyDaemonPrivate *priv;
  NotifyTimeout *nt;
  guint id;
  priv = daemon->priv;
  id = 0;

  do 
    {
      id = priv->next_id;

      if (id != UINT_MAX)
        priv->next_id++;
      else
        priv->next_id = 1;

      if (g_hash_table_lookup (priv->notification_hash, &id) != NULL)
        id = 0;
    }
  while (id == 0);
  
  nt = (NotifyTimeout *) g_new0(NotifyTimeout, 1);

  nt->id = id;
  nt->widget = bw;
  
  _calculate_timeout (daemon, nt, timeout);
  
  g_hash_table_insert (priv->notification_hash, 
                       g_memdup(&id, sizeof (guint)), 
                       (gpointer) nt);

  return id;
}

static gboolean 
_notify_daemon_process_icon_data (NotifyDaemon *daemon, 
                                  EggNotificationBubbleWidget *bw,
                                  GValue *icon_data)
{
  const guchar *data;
  gboolean has_alpha;
  int bits_per_sample;
  int width;
  int height;
  int rowstride;
  int n_channels;
  gsize expected_len;
  
  GValueArray *image_struct;
  GValue *value;
  GArray *tmp_array;
 
  data = NULL;
 
  if (!G_VALUE_HOLDS (icon_data, G_TYPE_VALUE_ARRAY))
    {
      g_warning ("_notify_daemon_process_icon_data expected a GValue of type GValueArray");
      return FALSE;
    }
 
  image_struct = (GValueArray *) (g_value_get_boxed (icon_data));
  
  value = g_value_array_get_nth (image_struct, 0);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 0 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, G_TYPE_INT))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 0 of the GValueArray to be of type int");
      return FALSE;
    }

  width = g_value_get_int (value);
  
  value = g_value_array_get_nth (image_struct, 1);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 1 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, G_TYPE_INT))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 1 of the GValueArray to be of type int");
      return FALSE;
    }

  height = g_value_get_int (value);

  value = g_value_array_get_nth (image_struct, 2);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 2 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, G_TYPE_INT))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 2 of the GValueArray to be of type int");
      return FALSE;
    }

  rowstride = g_value_get_int (value);

  value = g_value_array_get_nth (image_struct, 3);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 3 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, G_TYPE_BOOLEAN))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 3 of the GValueArray to be of type gboolean");
      return FALSE;
    }

  has_alpha = g_value_get_boolean (value);

  value = g_value_array_get_nth (image_struct, 4);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 4 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, G_TYPE_INT))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 4 of the GValueArray to be of type int");
      return FALSE;
    }

  bits_per_sample = g_value_get_int (value);

  value = g_value_array_get_nth (image_struct, 5);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 5 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, G_TYPE_INT))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 5 of the GValueArray to be of type int");
      return FALSE;
    }

  n_channels = g_value_get_int (value);


  value = g_value_array_get_nth (image_struct, 6);
  if (!value)
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 6 of the GValueArray to exist");
      return FALSE;
    }

  if (!G_VALUE_HOLDS (value, dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR)))
    {
      g_warning ("_notify_daemon_process_icon_data expected possition 6 of the GValueArray to be of type GArray");
      return FALSE;
    }

  tmp_array = (GArray *) g_value_get_boxed (value);  
  expected_len = (height -1) * rowstride + width * ((n_channels * bits_per_sample + 7) / 8);

  if (expected_len != tmp_array->len)
    {
      g_warning ("_notify_daemon_process_icon_data expected image data to be of length %i but got a length of %i", expected_len, tmp_array->len);
      return FALSE;
    }

  data = (guchar *)g_memdup (tmp_array->data, tmp_array->len);

  egg_notification_bubble_widget_set_icon_from_data (bw,
                                                     data,
                                                     has_alpha,
                                                     bits_per_sample,
                                                     width,
                                                     height,
                                                     rowstride);
  return TRUE;
}

static void
_notification_daemon_handle_bubble_widget_action (GtkWidget *b, 
                                                  EggNotificationBubbleWidget *bw)
{
  gchar *action;

  action = (gchar *) g_object_get_data (G_OBJECT (b), "_notify_action");

  _emit_action_invoked_signal (G_OBJECT (bw), action);  
}

static void
_notification_daemon_handle_bubble_widget_default (EggNotificationBubbleWidget *bw,
                                                   NotifyDaemon *daemon)
{
  _close_notification (daemon, GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (bw), "_notify_id")));
}

gboolean 
notify_daemon_notify_handler (NotifyDaemon *daemon,
                              const gchar *app_name,
                              const gchar *icon,
                              guint id,
                              const gchar *summary,
                              const gchar *body,
                              gchar **actions,
                              GHashTable *hints,
                              int timeout,
                              DBusGMethodInvocation *context)
{
  NotifyDaemonPrivate *priv;
  NotifyTimeout *nt;
  EggNotificationBubbleWidget *bw;
  GValue *data;
  gboolean use_pos_data;
  gint x, y;
  guint return_id;
  gchar *sender;
  gint i;

  nt = NULL;

  priv = daemon->priv;
  bw = NULL;
  if (id > 0)
    nt = (NotifyTimeout *) 
           g_hash_table_lookup (priv->notification_hash, &id);

  if (!nt)
    {
      bw = egg_notification_bubble_widget_new ();
      id = 0;
    }
  else
    bw = nt->widget;

  use_pos_data = FALSE;

  egg_notification_bubble_widget_set (bw, summary, icon, body);
 
  /* deal with x, and y hints */
  data = (GValue *) (g_hash_table_lookup (hints, "x"));
  if (data)
    {
      x = g_value_get_int (data);
      data = (GValue *) g_hash_table_lookup (hints, "y");

      if (data)
        {
          y = g_value_get_int (data);
          use_pos_data = TRUE;       
        }
    }

  /* set up action buttons */
  i = 0;
  while (actions[i] != NULL)
    {
      gchar *l;
      GtkWidget *b;

      l = actions[i + 1];
      if (l == NULL)
        {
          g_warning ("Label not found for action %s. "
                     "The protocol specifies that a label must "
                     "follow an action in the actions array", actions[i]);

          break;
        }

      b = egg_notification_bubble_widget_create_button (bw, l);
       
      g_object_set_data_full (G_OBJECT (b), 
                              "_notify_action", 
                              g_strdup (actions[i]), 
                              (GDestroyNotify) g_free);

      g_signal_connect (b, 
                        "clicked",
                        (GCallback)_notification_daemon_handle_bubble_widget_action, 
                        bw);

      i = i + 2;
    }

  if (use_pos_data)
    egg_notification_bubble_widget_set_pos (bw, x, y);
  else
    egg_notification_bubble_widget_set_pos (bw, 100, 20);

  /* check for icon_data if icon == "" */
  if (strcmp ("", icon) == 0)
    {
      data = (GValue *) (g_hash_table_lookup (hints, "icon_data"));
      if (data)
        _notify_daemon_process_icon_data (daemon, bw, data);
    }

  g_signal_connect (bw, "clicked", (GCallback)_notification_daemon_handle_bubble_widget_default, daemon);  

  egg_notification_bubble_widget_show (bw);

  if (id == 0)
    return_id = _store_notification (daemon, bw, timeout);
  else
    return_id = id;

  sender = dbus_g_method_get_sender (context);

  g_object_set_data (G_OBJECT (bw), "_notify_id", GUINT_TO_POINTER (return_id));
  g_object_set_data_full (G_OBJECT (bw), 
                          "_notify_sender", 
                          sender, 
                          (GDestroyNotify) g_free);

  if (nt)
    _calculate_timeout (daemon, nt, timeout);

  dbus_g_method_return (context, return_id);

  return TRUE;
}

gboolean 
notify_daemon_close_notification_handler (NotifyDaemon *daemon,
                                          guint id,
                                          GError **error)
{
  _close_notification (daemon, id);

  return TRUE;
}

int
main (int argc, char **argv)
{
  NotifyDaemon *daemon;
  DBusGConnection *connection;
  DBusGProxy *bus_proxy;
  GError *error;
  guint request_name_result;
  g_log_set_always_fatal (G_LOG_LEVEL_ERROR
			  | G_LOG_LEVEL_CRITICAL);

  g_message ("initializing glib type system");
  gtk_init (&argc, &argv);

  error = NULL;
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (connection == NULL)
    {
      g_printerr ("Failed to open connection to bus: %s\n",
                  error->message);
      g_error_free (error);
      exit (1);
    }

  g_message ("register 'daemon' type with dbus-glib...");
  dbus_g_object_type_install_info (NOTIFY_TYPE_DAEMON, &dbus_glib__object_info);
  g_message ("'daemon' successfully registered");

  bus_proxy = dbus_g_proxy_new_for_name (connection, "org.freedesktop.DBus",
                                         "/org/freedesktop/DBus",
                                         "org.freedesktop.DBus");

  if (!dbus_g_proxy_call (bus_proxy, "RequestName", &error,
                          G_TYPE_STRING, "org.freedesktop.Notifications",
                          G_TYPE_UINT, 0,
                          G_TYPE_INVALID,
                          G_TYPE_UINT, &request_name_result,
                          G_TYPE_INVALID))
    g_error ("Could not aquire name: %s", error->message);

  g_message ("creating instance of 'daemon' object...");
  daemon = notify_daemon_new ();
  g_message ("'daemon' object created successfully");

  g_message ("exporting instance of 'daemon' object over the bus...");
  dbus_g_connection_register_g_object (connection, "/org/freedesktop/Notifications", G_OBJECT (daemon));
  g_message ("'daemon' object exported successfully");

  gtk_main();

  return 0;
}

