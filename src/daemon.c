/* daemon.c - Implementation of the destop notification spec
 *
 * Copyright (C) 2006 Christian Hammond <chipx86@chipx86.com>
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

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "daemon.h"
#include "engines.h"
#include "notificationdaemon-dbus-glue.h"

#define IMAGE_SIZE 48

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
	gboolean url_clicked_lock;
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

static void notify_daemon_finalize(GObject *object);
static void _emit_closed_signal(GObject *notify_widget);
static void _action_invoked_cb(GtkWindow *nw, const char *key);

G_DEFINE_TYPE(NotifyDaemon, notify_daemon, G_TYPE_OBJECT);

static void
notify_daemon_class_init(NotifyDaemonClass *daemon_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(daemon_class);

	object_class->finalize = notify_daemon_finalize;

	g_type_class_add_private(daemon_class, sizeof(NotifyDaemonPrivate));
}

static void
_notify_timeout_destroy(NotifyTimeout *nt)
{
	gtk_widget_destroy(GTK_WIDGET(nt->nw));
	g_free(nt);
}

static void
notify_daemon_init(NotifyDaemon *daemon)
{
	daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE(daemon, NOTIFY_TYPE_DAEMON,
											   NotifyDaemonPrivate);

	daemon->priv->next_id = 1;
	daemon->priv->timeout_source = 0;
	daemon->priv->notification_hash =
		g_hash_table_new_full(g_int_hash, g_int_equal, g_free,
							  (GDestroyNotify)_notify_timeout_destroy);
}

static void
notify_daemon_finalize(GObject *object)
{
	NotifyDaemon *daemon;
	GObjectClass *parent_class;

	daemon = NOTIFY_DAEMON(object);

	g_hash_table_destroy(daemon->priv->notification_hash);
	g_slist_free(daemon->priv->poptart_stack);

	parent_class = G_OBJECT_CLASS(notify_daemon_parent_class);

	if (parent_class->finalize != NULL)
		parent_class->finalize(object);
}

NotifyDaemon *
notify_daemon_new(void)
{
	return g_object_new(NOTIFY_TYPE_DAEMON, NULL);
}

static void
_action_invoked_cb(GtkWindow *nw, const char *key)
{
	DBusConnection *con;
	DBusError error;

	dbus_error_init(&error);

	con = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (con == NULL)
	{
		g_warning("Error sending ActionInvoked signal: %s", error.message);
		dbus_error_free(&error);
	}
	else
	{
		DBusMessage *message;
		gchar *dest;
		guint id;

		message = dbus_message_new_signal("/org/freedesktop/Notifications",
										  "org.freedesktop.Notifications",
										  "ActionInvoked");

		dest = g_object_get_data(G_OBJECT(nw), "_notify_sender");
		id = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(nw), "_notify_id"));

		g_assert(dest != NULL);

		dbus_message_set_destination(message, dest);
		dbus_message_append_args(message,
								 DBUS_TYPE_UINT32, &id,
								 DBUS_TYPE_STRING, &key,
								 DBUS_TYPE_INVALID);

		dbus_connection_send(con, message, NULL);

		dbus_message_unref(message);
		dbus_connection_unref(con);
	}
}

static void
_emit_closed_signal(GObject *notify_widget)
{
	DBusConnection *con;
	DBusError error;

	dbus_error_init(&error);

	con = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (con == NULL)
	{
		g_warning("Error sending Close signal: %s", error.message);
		dbus_error_free(&error);
	}
	else
	{
		DBusMessage *message;
		gchar *dest;
		guint id;

		message = dbus_message_new_signal("/org/freedesktop/Notifications",
										  "org.freedesktop.Notifications",
										  "NotificationClosed");

		dest = g_object_get_data(notify_widget, "_notify_sender");
		id = GPOINTER_TO_UINT(g_object_get_data(notify_widget, "_notify_id"));

		g_assert(dest != NULL);

		dbus_message_set_destination(message, dest);
		dbus_message_append_args(message,
								 DBUS_TYPE_UINT32, &id,
								 DBUS_TYPE_INVALID);

		dbus_connection_send(con, message, NULL);

		dbus_message_unref(message);
		dbus_connection_unref(con);
	}
}

static void
_close_notification(NotifyDaemon *daemon, guint id, gboolean hide_notification)
{
	NotifyDaemonPrivate *priv = daemon->priv;
	NotifyTimeout *nt;

	nt = (NotifyTimeout *)g_hash_table_lookup(priv->notification_hash, &id);

	if (nt != NULL)
	{
		_emit_closed_signal(G_OBJECT(nt->nw));

		if (hide_notification)
			theme_hide_notification(nt->nw);

		g_hash_table_remove(priv->notification_hash, &id);
	}
}

static void
_notification_destroyed_cb(GtkWindow *nw, NotifyDaemon *daemon)
{
	_close_notification(
		daemon,
		GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(nw), "_notify_id")),
		FALSE);
}

static gboolean
_is_expired(gpointer key, gpointer value, gpointer data)
{
	NotifyTimeout *nt = (NotifyTimeout *)value;
	gboolean *phas_more_timeouts = (gboolean *)data;
	glong remaining;
	GTimeVal now;

	if (!nt->has_timeout)
		return FALSE;

	g_get_current_time(&now);

	remaining = ((nt->expiration.tv_sec * 1000) +
				 (nt->expiration.tv_usec / 1000)) -
	            ((now.tv_sec * 1000) + (now.tv_usec / 1000));

	theme_notification_tick(nt->nw, remaining);

	if (remaining <= 0)
	{
		_emit_closed_signal(G_OBJECT(nt->nw));
		return TRUE;
	}

	*phas_more_timeouts = TRUE;

	return FALSE;
}

static gboolean
_check_expiration(gpointer data)
{
	NotifyDaemon *daemon = (NotifyDaemon *)data;
	gboolean has_more_timeouts = FALSE;

	g_hash_table_foreach_remove(daemon->priv->notification_hash,
								_is_expired, (gpointer)&has_more_timeouts);

	if (!has_more_timeouts)
		daemon->priv->timeout_source = 0;

	return has_more_timeouts;
}

static void
_calculate_timeout(NotifyDaemon *daemon, NotifyTimeout *nt, int timeout)
{
	if (timeout == 0)
		nt->has_timeout = FALSE;
	else
	{
		gulong usec;

		nt->has_timeout = TRUE;

		if (timeout == -1)
			timeout = NOTIFY_DAEMON_DEFAULT_TIMEOUT;

		theme_set_notification_timeout(nt->nw, timeout);

		usec = timeout * 1000;	/* convert from msec to usec */
		g_get_current_time(&nt->expiration);
		g_time_val_add(&nt->expiration, usec);

		if (daemon->priv->timeout_source == 0)
		{
			daemon->priv->timeout_source = g_timeout_add(100,
														 _check_expiration,
														 daemon);
		}
	}
}

static guint
_store_notification(NotifyDaemon *daemon, GtkWindow *nw, int timeout)
{
	NotifyDaemonPrivate *priv = daemon->priv;
	NotifyTimeout *nt;
	guint id = 0;

	do
	{
		id = priv->next_id;

		if (id != UINT_MAX)
			priv->next_id++;
		else
			priv->next_id = 1;

		if (g_hash_table_lookup(priv->notification_hash, &id) != NULL)
			id = 0;

	} while (id == 0);

	nt = g_new0(NotifyTimeout, 1);
	nt->id = id;
	nt->nw = nw;

	_calculate_timeout(daemon, nt, timeout);

	g_hash_table_insert(priv->notification_hash,
						g_memdup(&id, sizeof(guint)), nt);

	return id;
}

static gboolean
_notify_daemon_process_icon_data(NotifyDaemon *daemon, GtkWindow *nw,
								 GValue *icon_data)
{
	const guchar *data = NULL;
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

	if (!G_VALUE_HOLDS(icon_data, G_TYPE_VALUE_ARRAY))
	{
		g_warning("_notify_daemon_process_icon_data expected a "
				  "GValue of type GValueArray");
		return FALSE;
	}

	image_struct = (GValueArray *)g_value_get_boxed(icon_data);
	value = g_value_array_get_nth(image_struct, 0);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected position "
				  "0 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value, G_TYPE_INT))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 0 of the GValueArray to be of type int");
		return FALSE;
	}

	width = g_value_get_int(value);
	value = g_value_array_get_nth(image_struct, 1);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 1 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value, G_TYPE_INT))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 1 of the GValueArray to be of type int");
		return FALSE;
	}

	height = g_value_get_int(value);
	value = g_value_array_get_nth(image_struct, 2);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 2 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value, G_TYPE_INT))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 2 of the GValueArray to be of type int");
		return FALSE;
	}

	rowstride = g_value_get_int(value);
	value = g_value_array_get_nth(image_struct, 3);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 3 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value, G_TYPE_BOOLEAN))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 3 of the GValueArray to be of type gboolean");
		return FALSE;
	}

	has_alpha = g_value_get_boolean(value);
	value = g_value_array_get_nth(image_struct, 4);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 4 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value, G_TYPE_INT))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 4 of the GValueArray to be of type int");
		return FALSE;
	}

	bits_per_sample = g_value_get_int(value);
	value = g_value_array_get_nth(image_struct, 5);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 5 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value, G_TYPE_INT))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 5 of the GValueArray to be of type int");
		return FALSE;
	}

	n_channels = g_value_get_int(value);
	value = g_value_array_get_nth(image_struct, 6);

	if (value == NULL)
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 6 of the GValueArray to exist");
		return FALSE;
	}

	if (!G_VALUE_HOLDS(value,
					   dbus_g_type_get_collection("GArray", G_TYPE_UCHAR)))
	{
		g_warning("_notify_daemon_process_icon_data expected "
				  "position 6 of the GValueArray to be of type GArray");
		return FALSE;
	}

	tmp_array = (GArray *)g_value_get_boxed(value);
	expected_len = (height - 1) * rowstride + width *
	               ((n_channels * bits_per_sample + 7) / 8);

	if (expected_len != tmp_array->len)
	{
		g_warning("_notify_daemon_process_icon_data expected image "
				  "data to be of length %i but got a length of %i",
				  expected_len, tmp_array->len);
		return FALSE;
	}

	data = (guchar *)g_memdup(tmp_array->data, tmp_array->len);
	pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, has_alpha,
									  bits_per_sample, width, height,
									  rowstride,
									  (GdkPixbufDestroyNotify)g_free,
									  NULL);
	theme_set_notification_icon(nw, pixbuf);
	g_object_unref(G_OBJECT(pixbuf));

	return TRUE;
}

static void
window_clicked_cb(GtkWindow *nw, GdkEventButton *button, NotifyDaemon *daemon)
{
	if (daemon->priv->url_clicked_lock)
	{
		daemon->priv->url_clicked_lock = FALSE;
		return;
	}

	_action_invoked_cb(nw, "default");

	_close_notification(
		daemon,
		GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(nw), "_notify_id")),
		TRUE);
}

static gboolean
get_work_area(GtkWidget *nw, GdkRectangle *rect)
{
	Atom workarea = XInternAtom(GDK_DISPLAY(), "_NET_WORKAREA", True);
	Atom type;
	Window win;
	int format;
	gulong num, leftovers;
	gulong max_len = 4 * 32;
	guchar *ret_workarea;
	long *workareas;
	int result;
	GdkScreen *screen;
	int disp_screen;

	gtk_widget_realize(nw);
	screen = gdk_drawable_get_screen(GDK_DRAWABLE(nw->window));
	disp_screen = GDK_SCREEN_XNUMBER(screen);

	/* Defaults in case of error */
	rect->x = 0;
	rect->y = 0;
	rect->width = gdk_screen_get_width(screen);
	rect->height = gdk_screen_get_height(screen);

	if (workarea == None)
		return FALSE;

	win = XRootWindow(GDK_DISPLAY(), disp_screen);
	result = XGetWindowProperty(GDK_DISPLAY(), win, workarea, 0,
								max_len, False, AnyPropertyType,
								&type, &format, &num, &leftovers,
								&ret_workarea);

	if (result != Success || type == None || format == 0 || leftovers ||
		num % 4)
	{
		return FALSE;
	}

	workareas = (long *)ret_workarea;
	rect->x      = workareas[disp_screen * 4];
	rect->y      = workareas[disp_screen * 4 + 1];
	rect->width  = workareas[disp_screen * 4 + 2];
	rect->height = workareas[disp_screen * 4 + 3];

	XFree(ret_workarea);

	return TRUE;
}

static void
_remove_bubble_from_poptart_stack(GtkWindow *nw, NotifyDaemon *daemon)
{
	NotifyDaemonPrivate *priv = daemon->priv;
	GdkRectangle workarea;
	GSList *remove_link = NULL;
	GSList *link;
	gint x, y;

	get_work_area(GTK_WIDGET(nw), &workarea);

	y = workarea.y + workarea.height;
	x = 0;

	for (link = priv->poptart_stack; link != NULL; link = link->next)
	{
		GtkWindow *nw2 = link->data;
		GtkRequisition req;

		if (nw2 != nw)
		{
			gtk_widget_size_request(GTK_WIDGET(nw2), &req);

			x = workarea.x + workarea.width - req.width;
			y = y - req.height;

			theme_move_notification(nw2, x, y);
		}
		else
		{
			remove_link = link;
		}
	}

	if (remove_link)
	{
		priv->poptart_stack = g_slist_remove_link(priv->poptart_stack,
												  remove_link);
	}
}

static void
_notify_daemon_add_bubble_to_poptart_stack(NotifyDaemon *daemon,
										   GtkWindow *nw,
										   gboolean new_notification)
{
	NotifyDaemonPrivate *priv = daemon->priv;
	GtkRequisition req;
	GdkRectangle workarea;
	GSList *link;
	gint x, y;

	gtk_widget_size_request(GTK_WIDGET(nw), &req);

	get_work_area(GTK_WIDGET(nw), &workarea);

	x = workarea.x + workarea.width - req.width;
	y = workarea.y + workarea.height - req.height;

	theme_move_notification(nw, x, y);

	for (link = priv->poptart_stack; link != NULL; link = link->next)
	{
		GtkWindow *nw2 = GTK_WINDOW(link->data);

		if (nw2 != nw)
		{
			gtk_widget_size_request(GTK_WIDGET(nw2), &req);
			x = workarea.x + workarea.width - req.width;
			y = y - req.height;
			theme_move_notification(nw2, x, y);
		}
	}

	if (new_notification)
	{
		g_signal_connect(G_OBJECT(nw), "destroy",
						 G_CALLBACK(_remove_bubble_from_poptart_stack),
						 daemon);
		priv->poptart_stack = g_slist_prepend(priv->poptart_stack, nw);
	}
}

static void
url_clicked_cb(GtkWindow *nw, const char *url)
{
	NotifyDaemon *daemon = g_object_get_data(G_OBJECT(nw), "_notify_daemon");
	char *escaped_url;
	char *cmd = NULL;

	/* Somewhat of a hack.. */
	daemon->priv->url_clicked_lock = TRUE;

	escaped_url = g_shell_quote(url);

	/*
	 * We can't actually check for GNOME_DESKTOP_SESSION_ID, because it's
	 * not in the environment for this program :(
	 */
	if (/*g_getenv("GNOME_DESKTOP_SESSION_ID") != NULL &&*/
		g_find_program_in_path("gnome-open") != NULL)
	{
		cmd = g_strdup_printf("gnome-open %s", escaped_url);
	}
	else if (g_find_program_in_path("mozilla-firefox") != NULL)
	{
		cmd = g_strdup_printf("mozilla-firefox %s", escaped_url);
	}
	else if (g_find_program_in_path("firefox") != NULL)
	{
		cmd = g_strdup_printf("firefox %s", escaped_url);
	}
	else if (g_find_program_in_path("mozilla") != NULL)
	{
		cmd = g_strdup_printf("mozilla %s", escaped_url);
	}
	else
	{
		g_warning("Unable to find a browser.");
	}

	g_free(escaped_url);

	if (cmd != NULL)
	{
		g_spawn_command_line_async(cmd, NULL);
		g_free(cmd);
	}
}

static gboolean
screensaver_active(GtkWidget *nw)
{
	GdkDisplay *display = gdk_drawable_get_display(GDK_DRAWABLE(nw->window));
	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	unsigned char *temp_data;
	gboolean active = FALSE;
	Atom XA_BLANK = gdk_x11_get_xatom_by_name_for_display(display, "BLANK");
	Atom XA_LOCK = gdk_x11_get_xatom_by_name_for_display(display, "LOCK");

	/* Check for a screensaver first. */
	if (XGetWindowProperty(
		GDK_DISPLAY_XDISPLAY(display),
		GDK_ROOT_WINDOW(),
		gdk_x11_get_xatom_by_name_for_display(display, "_SCREENSAVER_STATUS"),
		0, G_MAXLONG, False, XA_INTEGER, &type, &format, &nitems,
		&bytes_after, &temp_data) == Success &&
		type && temp_data != NULL)
	{
		CARD32 *data = (CARD32 *)temp_data;

		active = (type == XA_INTEGER && nitems >= 3 &&
				  (time_t)data[1] > (time_t)666000000L &&
				  (data[0] == XA_BLANK || data[0] == XA_LOCK));
	}

	if (temp_data != NULL)
		free(temp_data);

	return active;
}

static gboolean
fullscreen_window_exists(GtkWidget *nw)
{
	WnckScreen *wnck_screen;
	GList *l;

	wnck_screen = wnck_screen_get(GDK_SCREEN_XNUMBER(
		gdk_drawable_get_screen(GDK_DRAWABLE(GTK_WIDGET(nw)->window))));
	wnck_screen_force_update(wnck_screen);

	for (l = wnck_screen_get_windows_stacked(wnck_screen);
		 l != NULL;
		 l = l->next)
	{
		WnckWindow *wnck_win = (WnckWindow *)l->data;

		if (wnck_window_is_fullscreen(wnck_win))
		{
			/*
			 * Sanity check if the window is _really_ fullscreen to
			 * work around a bug in libwnck that doesn't get all
			 * unfullscreen events.
			 */
			int sw = wnck_screen_get_width(wnck_screen);
			int sh = wnck_screen_get_height(wnck_screen);
			int x, y, w, h;

			wnck_window_get_geometry(wnck_win, &x, &y, &w, &h);

			if (sw == w && sh == h)
				return TRUE;
		}
	}

	return FALSE;
}

gboolean
notify_daemon_notify_handler(NotifyDaemon *daemon,
							 const gchar *app_name,
							 guint id,
							 const gchar *icon,
							 const gchar *summary,
							 const gchar *body,
							 gchar **actions,
							 GHashTable *hints,
							 int timeout, DBusGMethodInvocation *context)
{
	NotifyDaemonPrivate *priv = daemon->priv;
	NotifyTimeout *nt = NULL;
	GtkWindow *nw = NULL;
	GValue *data;
	gboolean use_pos_data = FALSE;
	gboolean new_notification = FALSE;
	gint x = 0;
	gint y = 0;
	guint return_id;
	gchar *sender;
	gint i;

	if (id > 0)
	{
		nt = (NotifyTimeout *)g_hash_table_lookup(priv->notification_hash,
												  &id);

		if (nt != NULL)
			nw = nt->nw;
	}

	if (nw == NULL)
	{
		nw = theme_create_notification(url_clicked_cb);
		g_object_set_data(G_OBJECT(nw), "_notify_daemon", daemon);
		new_notification = TRUE;

		g_signal_connect(G_OBJECT(nw), "button-release-event",
						 G_CALLBACK(window_clicked_cb), daemon);
		g_signal_connect(G_OBJECT(nw), "destroy",
						 G_CALLBACK(_notification_destroyed_cb), daemon);
	}

	theme_set_notification_text(nw, summary, body);
	theme_set_notification_hints(nw, hints);

	/*
	 *XXX This needs to handle file URIs and all that.
	 */

	/* deal with x, and y hints */
	if ((data = (GValue *)g_hash_table_lookup(hints, "x")) != NULL)
	{
		x = g_value_get_int(data);

		if ((data = (GValue *)g_hash_table_lookup(hints, "y")) != NULL)
		{
			y = g_value_get_int(data);
			use_pos_data = TRUE;
		}
	}

	/* set up action buttons */
	for (i = 0; actions[i] != NULL; i += 2)
	{
		gchar *l = actions[i + 1];

		if (l == NULL)
		{
			g_warning("Label not found for action %s. "
					  "The protocol specifies that a label must "
					  "follow an action in the actions array", actions[i]);

			break;
		}

		if (strcasecmp(actions[i], "default"))
		{
			theme_add_notification_action(nw, l, actions[i],
										  G_CALLBACK(_action_invoked_cb));
		}
	}

	if (use_pos_data)
	{
		/*
		 * Typically, the theme engine will set its own position based on
		 * the arrow X, Y hints. However, in case, move the notification to
		 * that position.
		 */
		theme_set_notification_arrow(nw, TRUE, x, y);
		theme_move_notification(nw, x, y);
	}
	else
	{
		theme_set_notification_arrow(nw, FALSE, 0, 0);
		_notify_daemon_add_bubble_to_poptart_stack(daemon, nw,
												   new_notification);
	}

	/* check for icon_data if icon == "" */
	if (*icon == '\0')
	{
		data = (GValue *)g_hash_table_lookup(hints, "icon_data");

		if (data)
			_notify_daemon_process_icon_data(daemon, nw, data);
	}
	else
	{
		GdkPixbuf *pixbuf = NULL;

		if (!strncmp(icon, "file://", 7) || *icon == '/')
		{
			if (!strncmp(icon, "file://", 7))
				icon += 7;

			/* Load file */
			pixbuf = gdk_pixbuf_new_from_file(icon, NULL);
		}
		else
		{
			/* Load icon theme icon */
			GtkIconTheme *theme = gtk_icon_theme_new();
			GtkIconInfo *icon_info =
				gtk_icon_theme_lookup_icon(theme, icon, IMAGE_SIZE,
										   GTK_ICON_LOOKUP_USE_BUILTIN);

			if (icon_info != NULL)
			{
				pixbuf = gtk_icon_theme_load_icon(
					theme, icon,
					MAX(IMAGE_SIZE, gtk_icon_info_get_base_size(icon_info)),
					GTK_ICON_LOOKUP_USE_BUILTIN, NULL);

				gtk_icon_info_free(icon_info);
			}

			if (pixbuf == NULL)
			{
				/* Well... maybe this is a file afterall. */
				pixbuf = gdk_pixbuf_new_from_file(icon, NULL);
			}
		}

		if (pixbuf != NULL)
		{
			theme_set_notification_icon(nw, pixbuf);
			g_object_unref(G_OBJECT(pixbuf));
		}
	}

	if (!screensaver_active(GTK_WIDGET(nw)) &&
		!fullscreen_window_exists(GTK_WIDGET(nw)))
	{
		theme_show_notification(nw);
	}

	return_id = (id == 0 ? _store_notification(daemon, nw, timeout) : id);

#if CHECK_DBUS_VERSION(0, 60)
	sender = dbus_g_method_get_sender(context);
#else
	sender = g_strdup(dbus_message_get_sender(
		dbus_g_message_get_message(context->message)));
#endif

	g_object_set_data(G_OBJECT(nw), "_notify_id",
					  GUINT_TO_POINTER(return_id));
	g_object_set_data_full(G_OBJECT(nw), "_notify_sender", sender,
						   (GDestroyNotify)g_free);

	if (nt)
		_calculate_timeout(daemon, nt, timeout);

	dbus_g_method_return(context, return_id);

	return TRUE;
}

gboolean
notify_daemon_close_notification_handler(NotifyDaemon *daemon,
										 guint id, GError ** error)
{
	_close_notification(daemon, id, TRUE);

	return TRUE;
}

gboolean
notify_daemon_get_capabilities(NotifyDaemon *daemon, char ***caps)
{
	*caps = g_new0(char *, 6);

	(*caps)[0] = g_strdup("actions");
	(*caps)[1] = g_strdup("body");
	(*caps)[2] = g_strdup("body-hyperlinks");
	(*caps)[3] = g_strdup("body-markup");
	(*caps)[4] = g_strdup("icon-static");
	(*caps)[5] = NULL;

	return TRUE;
}

gboolean
notify_daemon_get_server_information(NotifyDaemon *daemon,
									 char **out_name,
									 char **out_vendor,
									 char **out_version,
									 char **out_spec_ver)
{
	*out_name     = g_strdup("Notification Daemon");
	*out_vendor   = g_strdup("Galago Project");
	*out_version  = g_strdup(VERSION);
	*out_spec_ver = g_strdup("0.9");

	return TRUE;
}

GConfClient *
get_gconf_client(void)
{
	return gconf_client;
}

int
main(int argc, char **argv)
{
	NotifyDaemon *daemon;
	DBusGConnection *connection;
	DBusGProxy *bus_proxy;
	GError *error;
	guint request_name_result;

	g_log_set_always_fatal(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);

	gtk_init(&argc, &argv);
	gconf_init(argc, argv, NULL);

	gconf_client = gconf_client_get_default();
	gconf_client_add_dir(gconf_client, "/apps/notification-daemon/theme",
						 GCONF_CLIENT_PRELOAD_NONE, NULL);

	error = NULL;
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
	{
		g_printerr("Failed to open connection to bus: %s\n",
				   error->message);
		g_error_free(error);
		exit(1);
	}

	dbus_g_object_type_install_info(NOTIFY_TYPE_DAEMON,
									&dbus_glib__object_info);

	bus_proxy = dbus_g_proxy_new_for_name(connection,
										  "org.freedesktop.DBus",
										  "/org/freedesktop/DBus",
										  "org.freedesktop.DBus");

	if (!dbus_g_proxy_call(bus_proxy, "RequestName", &error,
						   G_TYPE_STRING, "org.freedesktop.Notifications",
						   G_TYPE_UINT, 0,
						   G_TYPE_INVALID,
						   G_TYPE_UINT, &request_name_result,
						   G_TYPE_INVALID))
	{
		g_error("Could not aquire name: %s", error->message);
	}

	daemon = notify_daemon_new();

	dbus_g_connection_register_g_object(connection,
										"/org/freedesktop/Notifications",
										G_OBJECT(daemon));

	gtk_main();

	g_object_unref(G_OBJECT(gconf_client));

	return 0;
}
