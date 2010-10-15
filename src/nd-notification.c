/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#include <strings.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

#include "nd-notification.h"

#define ND_NOTIFICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ND_TYPE_NOTIFICATION, NdNotificationClass))
#define ND_IS_NOTIFICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ND_TYPE_NOTIFICATION))
#define ND_NOTIFICATION_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), ND_TYPE_NOTIFICATION, NdNotificationClass))

enum {
        CHANGED,
        CLOSED,
        ACTION_INVOKED,
        LAST_SIGNAL
};

struct _NdNotification {
        GObject       parent;

        gboolean      is_closed;

        GTimeVal      update_time;

        char         *sender;
        guint32       id;
        char         *app_name;
        char         *icon;
        char         *summary;
        char         *body;
        char        **actions;
        GHashTable   *hints;
        int           timeout;
};

static void nd_notification_finalize     (GObject      *object);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NdNotification, nd_notification, G_TYPE_OBJECT)

static guint32 notification_serial = 1;

static guint32
get_next_notification_serial (void)
{
        guint32 serial;

        serial = notification_serial++;

        if ((gint32)notification_serial < 0) {
                notification_serial = 1;
        }

        return serial;
}

static void
nd_notification_class_init (NdNotificationClass *class)
{
        GObjectClass *gobject_class;

        gobject_class = G_OBJECT_CLASS (class);

        gobject_class->finalize = nd_notification_finalize;

        signals [CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
        signals [CLOSED] =
                g_signal_new ("closed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE, 1, G_TYPE_INT);
        signals [ACTION_INVOKED] =
                g_signal_new ("action-invoked",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE, 1, G_TYPE_STRING);
}

 static void
_g_value_free (GValue *value)
{
        g_value_unset (value);
        g_free (value);
}

static void
nd_notification_init (NdNotification *notification)
{
        notification->id = get_next_notification_serial ();

        notification->app_name = NULL;
        notification->icon = NULL;
        notification->summary = NULL;
        notification->body = NULL;
        notification->actions = NULL;
        notification->hints = g_hash_table_new_full (g_str_hash,
                                                     g_str_equal,
                                                     g_free,
                                                     (GDestroyNotify) _g_value_free);
}

static void
nd_notification_finalize (GObject *object)
{
        NdNotification *notification;

        notification = ND_NOTIFICATION (object);

        g_free (notification->sender);
        g_free (notification->app_name);
        g_free (notification->icon);
        g_free (notification->summary);
        g_free (notification->body);
        g_strfreev (notification->actions);

        if (notification->hints != NULL) {
                g_hash_table_destroy (notification->hints);
        }

        if (G_OBJECT_CLASS (nd_notification_parent_class)->finalize)
                (*G_OBJECT_CLASS (nd_notification_parent_class)->finalize) (object);
}

gboolean
nd_notification_update (NdNotification *notification,
                        const char     *app_name,
                        const char     *icon,
                        const char     *summary,
                        const char     *body,
                        const char    **actions,
                        GHashTable     *hints,
                        int             timeout)
{
        GHashTableIter iter;
        gpointer       key, value;

        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), FALSE);

        g_free (notification->app_name);
        notification->app_name = g_strdup (app_name);

        g_free (notification->icon);
        notification->icon = g_strdup (icon);

        g_free (notification->summary);
        notification->summary = g_strdup (summary);

        g_free (notification->body);
        notification->body = g_strdup (body);

        g_strfreev (notification->actions);
        notification->actions = g_strdupv ((char **)actions);

        g_hash_table_remove_all (notification->hints);

        g_hash_table_iter_init (&iter, hints);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                GValue *value_copy;

                value_copy = g_new0 (GValue, 1);
                g_value_init (value_copy, G_VALUE_TYPE (value));
                g_value_copy (value, value_copy);

                g_hash_table_insert (notification->hints, g_strdup (key), value_copy);
        }

        g_signal_emit (notification, signals[CHANGED], 0);

        g_get_current_time (&notification->update_time);

        return TRUE;
}

void
nd_notification_get_update_time (NdNotification *notification,
                                 GTimeVal       *tvp)
{
        g_return_if_fail (ND_IS_NOTIFICATION (notification));

        if (tvp == NULL) {
                return;
        }

        tvp->tv_usec = notification->update_time.tv_usec;
        tvp->tv_sec = notification->update_time.tv_sec;
}

gboolean
nd_notification_get_is_closed (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), FALSE);

        return notification->is_closed;
}

guint32
nd_notification_get_id (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), -1);

        return notification->id;
}

GHashTable *
nd_notification_get_hints (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), NULL);

        return notification->hints;
}

char **
nd_notification_get_actions (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), NULL);

        return notification->actions;
}

const char *
nd_notification_get_sender (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), NULL);

        return notification->sender;
}

const char *
nd_notification_get_summary (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), NULL);

        return notification->summary;
}

const char *
nd_notification_get_body (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), NULL);

        return notification->body;
}

const char *
nd_notification_get_icon (NdNotification *notification)
{
        g_return_val_if_fail (ND_IS_NOTIFICATION (notification), NULL);

        return notification->icon;
}


static GdkPixbuf *
scale_pixbuf (GdkPixbuf *pixbuf,
              int        max_width,
              int        max_height,
              gboolean   no_stretch_hint)
{
        int        pw;
        int        ph;
        float      scale_factor_x = 1.0;
        float      scale_factor_y = 1.0;
        float      scale_factor = 1.0;

        pw = gdk_pixbuf_get_width (pixbuf);
        ph = gdk_pixbuf_get_height (pixbuf);

        /* Determine which dimension requires the smallest scale. */
        scale_factor_x = (float) max_width / (float) pw;
        scale_factor_y = (float) max_height / (float) ph;

        if (scale_factor_x > scale_factor_y) {
                scale_factor = scale_factor_y;
        } else {
                scale_factor = scale_factor_x;
        }

        /* always scale down, allow to disable scaling up */
        if (scale_factor < 1.0 || !no_stretch_hint) {
                int scale_x;
                int scale_y;

                scale_x = (int) (pw * scale_factor);
                scale_y = (int) (ph * scale_factor);
                return gdk_pixbuf_scale_simple (pixbuf,
                                                scale_x,
                                                scale_y,
                                                GDK_INTERP_BILINEAR);
        } else {
                return g_object_ref (pixbuf);
        }
}

static GdkPixbuf *
_notify_daemon_pixbuf_from_data_hint (GValue *icon_data,
                                      int     size)
{
        const guchar   *data = NULL;
        gboolean        has_alpha;
        int             bits_per_sample;
        int             width;
        int             height;
        int             rowstride;
        int             n_channels;
        gsize           expected_len;
        GdkPixbuf      *pixbuf;
        GValueArray    *image_struct;
        GValue         *value;
        GArray         *tmp_array;
        GType           struct_type;

        struct_type = dbus_g_type_get_struct ("GValueArray",
                                              G_TYPE_INT,
                                              G_TYPE_INT,
                                              G_TYPE_INT,
                                              G_TYPE_BOOLEAN,
                                              G_TYPE_INT,
                                              G_TYPE_INT,
                                              dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR),
                                              G_TYPE_INVALID);

        if (!G_VALUE_HOLDS (icon_data, struct_type)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected a "
                           "GValue of type GValueArray");
                return NULL;
        }

        image_struct = (GValueArray *) g_value_get_boxed (icon_data);
        value = g_value_array_get_nth (image_struct, 0);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected position "
                           "0 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 0 of the GValueArray to be of type int");
                return NULL;
        }

        width = g_value_get_int (value);
        value = g_value_array_get_nth (image_struct, 1);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 1 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 1 of the GValueArray to be of type int");
                return NULL;
        }

        height = g_value_get_int (value);
        value = g_value_array_get_nth (image_struct, 2);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 2 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 2 of the GValueArray to be of type int");
                return NULL;
        }

        rowstride = g_value_get_int (value);
        value = g_value_array_get_nth (image_struct, 3);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 3 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value, G_TYPE_BOOLEAN)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 3 of the GValueArray to be of type gboolean");
                return NULL;
        }

        has_alpha = g_value_get_boolean (value);
        value = g_value_array_get_nth (image_struct, 4);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 4 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 4 of the GValueArray to be of type int");
                return NULL;
        }

        bits_per_sample = g_value_get_int (value);
        value = g_value_array_get_nth (image_struct, 5);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 5 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 5 of the GValueArray to be of type int");
                return NULL;
        }

        n_channels = g_value_get_int (value);
        value = g_value_array_get_nth (image_struct, 6);

        if (value == NULL) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 6 of the GValueArray to exist");
                return NULL;
        }

        if (!G_VALUE_HOLDS (value,
                            dbus_g_type_get_collection ("GArray",
                                                        G_TYPE_UCHAR))) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected "
                           "position 6 of the GValueArray to be of type GArray");
                return NULL;
        }

        tmp_array = (GArray *) g_value_get_boxed (value);
        expected_len = (height - 1) * rowstride + width
                * ((n_channels * bits_per_sample + 7) / 8);

        if (expected_len != tmp_array->len) {
                g_warning ("_notify_daemon_pixbuf_from_data_hint expected image "
                           "data to be of length %" G_GSIZE_FORMAT
                           " but got a " "length of %u", expected_len,
                           tmp_array->len);
                return NULL;
        }

        data = (guchar *) g_memdup (tmp_array->data, tmp_array->len);
        pixbuf = gdk_pixbuf_new_from_data (data,
                                           GDK_COLORSPACE_RGB,
                                           has_alpha,
                                           bits_per_sample,
                                           width,
                                           height,
                                           rowstride,
                                           (GdkPixbufDestroyNotify) g_free,
                                           NULL);
        if (pixbuf != NULL && size > 0) {
                GdkPixbuf *scaled;
                scaled = scale_pixbuf (pixbuf, size, size, TRUE);
                g_object_unref (pixbuf);
                pixbuf = scaled;
        }

        return pixbuf;
}

static GdkPixbuf *
_notify_daemon_pixbuf_from_path (const char *path,
                                 int         size)
{
        GFile *file;
        GdkPixbuf *pixbuf = NULL;

        file = g_file_new_for_commandline_arg (path);
        if (g_file_is_native (file)) {
                char *realpath;

                realpath = g_file_get_path (file);
                pixbuf = gdk_pixbuf_new_from_file_at_size (realpath, size, size, NULL);
                g_free (realpath);
        }
        g_object_unref (file);

        if (pixbuf == NULL) {
                /* Load icon theme icon */
                GtkIconTheme *theme;
                GtkIconInfo  *icon_info;

                theme = gtk_icon_theme_get_default ();
                icon_info = gtk_icon_theme_lookup_icon (theme,
                                                        path,
                                                        size,
                                                        GTK_ICON_LOOKUP_USE_BUILTIN);

                if (icon_info != NULL) {
                        gint icon_size;

                        icon_size = MIN (size,
                                         gtk_icon_info_get_base_size (icon_info));

                        if (icon_size == 0)
                                icon_size = size;

                        pixbuf = gtk_icon_theme_load_icon (theme,
                                                           path,
                                                           icon_size,
                                                           GTK_ICON_LOOKUP_USE_BUILTIN,
                                                           NULL);

                        gtk_icon_info_free (icon_info);
                }
        }

        return pixbuf;
}

GdkPixbuf *
nd_notification_load_image (NdNotification *notification,
                            int             size)
{
        GValue    *data;
        GdkPixbuf *pixbuf;

        pixbuf = NULL;

        if ((data = (GValue *) g_hash_table_lookup (notification->hints, "image_data"))) {
                pixbuf = _notify_daemon_pixbuf_from_data_hint (data, size);
        } else if ((data = (GValue *) g_hash_table_lookup (notification->hints, "image_path"))) {
                if (G_VALUE_HOLDS_STRING (data)) {
                        const char *path = g_value_get_string (data);
                        pixbuf = _notify_daemon_pixbuf_from_path (path, size);
                } else {
                        g_warning ("notify_daemon_notify_handler expected "
                                   "image_path hint to be of type string");
                }
        } else if (*notification->icon != '\0') {
                pixbuf = _notify_daemon_pixbuf_from_path (notification->icon, size);
        } else if ((data = (GValue *) g_hash_table_lookup (notification->hints, "icon_data"))) {
                g_warning("\"icon_data\" hint is deprecated, please use \"image_data\" instead");
                pixbuf = _notify_daemon_pixbuf_from_data_hint (data, size);
        }

        return pixbuf;
}

void
nd_notification_close (NdNotification            *notification,
                       NdNotificationClosedReason reason)
{
        g_return_if_fail (ND_IS_NOTIFICATION (notification));

        g_object_ref (notification);
        g_signal_emit (notification, signals[CLOSED], 0, reason);
        g_object_unref (notification);

        notification->is_closed = TRUE;
}

void
nd_notification_action_invoked (NdNotification  *notification,
                                const char      *action)
{
        g_return_if_fail (ND_IS_NOTIFICATION (notification));

        g_object_ref (notification);
        g_signal_emit (notification, signals[ACTION_INVOKED], 0, action);
        g_object_unref (notification);
}

NdNotification *
nd_notification_new (const char *sender)
{
        NdNotification *notification;

        notification = (NdNotification *) g_object_new (ND_TYPE_NOTIFICATION, NULL);
        notification->sender = g_strdup (sender);

        return notification;
}
