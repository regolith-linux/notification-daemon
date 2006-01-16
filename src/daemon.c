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
#include "daemon.h"
#include "engines.h"
#include "notificationdaemon-dbus-glue.h"

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

struct _NotifyTimeout
{
   GTimeVal expiration;
   gboolean has_timeout;
   guint id;

   GtkWindow *nw;
};

typedef struct _NotifyTimeout NotifyTimeout;

struct _NotifyDaemonPrivate
{
  guint next_id;
  guint timeout_source;
  GHashTable *notification_hash;
  GSList *poptart_stack;
};

static GConfClient *gconf_client = NULL;

#define CHECK_DBUS_VERSION(major, minor) \
	(DBUS_MAJOR_VER > (major) || \
	 (DBUS_MAJOR_VER == (major) && DBUS_MINOR_VER >= (minor)))

#if !CHECK_DBUS_VERSION(0, 60)
/* This is a hack that will go away in time. For now, it's fairly safe. */
struct _DBusGMethodInvocation
{
	DBusGConnection *connection;
	DBusGMessage *message;
	const DBusGObjectInfo *object;
	const DBusGMethodInfo *method;
};
#endif /* D-BUS < 0.60 */

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
  gtk_widget_destroy(GTK_WIDGET(nt->nw));
  g_free(nt);
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
  g_slist_free (daemon->priv->poptart_stack);

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

/*
 * XXX The notify_widget thing needs to be replaced with some struct.
 */
#if 0
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
      id = GPOINTER_TO_UINT (g_object_get_data (notify_widget, "_notify_id"));

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
#endif

static void
_action_invoked_cb(const char *key)
{
	g_message("'%s' invoked", key);
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
      _emit_closed_signal(G_OBJECT(nt->nw));

	  theme_hide_notification(nt->nw);
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
      _emit_closed_signal(G_OBJECT(nt->nw));
      return TRUE;
    }
  else if (now.tv_sec == expiration.tv_sec)
    {
      if (now.tv_usec > expiration.tv_usec)
        {
          _emit_closed_signal (G_OBJECT (nt->nw));
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
_store_notification(NotifyDaemon *daemon, GtkWindow *nw, int timeout)
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
  nt->nw = nw;

  _calculate_timeout (daemon, nt, timeout);

  g_hash_table_insert (priv->notification_hash,
                       g_memdup(&id, sizeof (guint)),
                       (gpointer) nt);

  return id;
}

static gboolean
_notify_daemon_process_icon_data(NotifyDaemon *daemon, GtkWindow *nw,
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
  GdkPixbuf *pixbuf;

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
	pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, has_alpha,
									  bits_per_sample, width, height,
									  rowstride,
									  (GdkPixbufDestroyNotify)g_free, NULL);
	theme_set_notification_icon(nw, pixbuf);
	g_object_unref(G_OBJECT(pixbuf));

  return TRUE;
}

#if 0
static void
_notification_daemon_handle_bubble_widget_action(GtkWidget *b, GtkWindow *nw)
{
  gchar *action;

  action = (gchar *) g_object_get_data (G_OBJECT (b), "_notify_action");

  _emit_action_invoked_signal (G_OBJECT (nw), action);
}
#endif

static void
_notification_daemon_handle_bubble_widget_default(GtkWindow *nw,
												  GdkEventButton *button,
												  NotifyDaemon *daemon)
{
  _close_notification(daemon,
	GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(nw), "_notify_id")));
}

static void
_remove_bubble_from_poptart_stack(GtkWindow *nw, NotifyDaemon *daemon)
{
  NotifyDaemonPrivate *priv;
  GdkRectangle workarea;
  GSList *remove_link;
  GSList *link;

  gint x, y;

  priv = daemon->priv;

  link = priv->poptart_stack;
  remove_link = NULL;

  workarea.x = 0;
  workarea.y = 0;
  workarea.width  = gdk_screen_width();
  workarea.height = gdk_screen_height();

  y = workarea.y + workarea.height;
  x = 0;
  while (link)
  {
	  GtkWindow *nw2;
      GtkRequisition req;

	  nw2 = link->data;

      if (nw2 != nw)
	  {
		  printf ("dude\n");

          gtk_widget_size_request(GTK_WIDGET(nw2), &req);

          x = workarea.x + workarea.width - req.width;
          y = y - req.height;

		  theme_move_notification(nw2, x, y);
	  }
      else
	  {
		  remove_link = link;
	  }

      link = link->next;
    }

    if (remove_link)
      priv->poptart_stack = g_slist_remove_link (priv->poptart_stack, remove_link);
}

static void
_notify_daemon_add_bubble_to_poptart_stack(NotifyDaemon *daemon, GtkWindow *nw)
{
  NotifyDaemonPrivate *priv;
  GtkRequisition req;
  GdkRectangle workarea;
  GSList *link;
  gint x, y;

  priv = daemon->priv;

  gtk_widget_size_request(GTK_WIDGET(nw), &req);

  workarea.x = 0;
  workarea.y = 0;
  workarea.width  = gdk_screen_width();
  workarea.height = gdk_screen_height();

  x = workarea.x + workarea.width - req.width;
  y = workarea.y + workarea.height - req.height;

  g_message ("x %i y %i width %i height %i", x, y, req.width, req.height);

  theme_move_notification(nw, x, y);

  link = priv->poptart_stack;
  while (link)
  {
		GtkWindow *nw2;

		nw2 = GTK_WINDOW(link->data);
		gtk_widget_size_request(GTK_WIDGET(nw2), &req);

		x = workarea.x + workarea.width - req.width;
		y = y - req.height;
		g_message ("x %i y %i width %i height %i", x, y, req.width, req.height);
		theme_move_notification(nw2, x, y);

		link = link->next;
	}

	g_signal_connect(G_OBJECT(nw), "destroy",
					 G_CALLBACK(_remove_bubble_from_poptart_stack), daemon);
	priv->poptart_stack = g_slist_prepend (priv->poptart_stack, nw);
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
  GtkWindow *nw;
  GValue *data;
  gboolean use_pos_data;
  gint x, y;
  guint return_id;
  gchar *sender;
  gint i;

  x = 0;
  y = 0;

  nt = NULL;

  priv = daemon->priv;
  nw = NULL;
  if (id > 0)
    nt = (NotifyTimeout *)
           g_hash_table_lookup (priv->notification_hash, &id);

  if (!nt)
  {
	  nw = theme_create_notification();
      id = 0;
  }
  else
	  nw = nt->nw;

  use_pos_data = FALSE;

  theme_set_notification_text(nw, summary, body);
  /*
   * XXX This needs to handle file URIs and all that.
   */
  /* set_icon_from_data(nw, icon); */

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
      gchar *l = actions[i + 1];

      if (l == NULL)
        {
          g_warning ("Label not found for action %s. "
                     "The protocol specifies that a label must "
                     "follow an action in the actions array", actions[i]);

          break;
        }

	  theme_add_notification_action(nw, l, actions[i],
									G_CALLBACK(_action_invoked_cb));

#if 0
      b = egg_notification_bubble_widget_create_button (nw, l);

      g_object_set_data_full (G_OBJECT (b),
                              "_notify_action",
                              g_strdup (actions[i]),
                              (GDestroyNotify) g_free);

      g_signal_connect (b,
                        "clicked",
                        (GCallback)_notification_daemon_handle_bubble_widget_action,
                        nw);
#endif

      i = i + 2;
    }

  if (use_pos_data)
  {
	  theme_set_notification_arrow(nw, TRUE, 0, 0);
	  theme_move_notification(nw, x, y);
  }
  else
  {
	  theme_set_notification_arrow(nw, FALSE, 0, 0);
      _notify_daemon_add_bubble_to_poptart_stack (daemon, nw);
  }

  /* check for icon_data if icon == "" */
  if (strcmp ("", icon) == 0)
    {
      data = (GValue *) (g_hash_table_lookup (hints, "icon_data"));
      if (data)
        _notify_daemon_process_icon_data(daemon, nw, data);
    }

  g_signal_connect(G_OBJECT(nw), "button-release-event",
	G_CALLBACK(_notification_daemon_handle_bubble_widget_default), daemon);

  theme_show_notification(nw);

  if (id == 0)
    return_id = _store_notification (daemon, nw, timeout);
  else
    return_id = id;

#if CHECK_DBUS_VERSION(0, 60)
  sender = dbus_g_method_get_sender (context);
#else
  sender = g_strdup(dbus_message_get_sender(
	dbus_g_message_get_message(context->message)));
#endif

  g_object_set_data(G_OBJECT(nw), "_notify_id", GUINT_TO_POINTER(return_id));
  g_object_set_data_full(G_OBJECT(nw),
						 "_notify_sender", sender, (GDestroyNotify)g_free);

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

GConfClient *
get_gconf_client(void)
{
	return gconf_client;
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
  gconf_init(argc, argv, NULL);

  gconf_client = gconf_client_get_default();
  gconf_client_add_dir(gconf_client, "/apps/notification-daemon/theme",
					   GCONF_CLIENT_PRELOAD_NONE, NULL);

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

  g_object_unref(G_OBJECT(gconf_client));

  return 0;
}

