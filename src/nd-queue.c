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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include "nd-queue.h"

#include "nd-notification.h"
#include "nd-stack.h"

#define ND_QUEUE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ND_TYPE_QUEUE, NdQueuePrivate))

#define IMAGE_SIZE    48
#define BODY_X_OFFSET (IMAGE_SIZE + 8)
#define WIDTH         400

typedef struct
{
        NdStack   **stacks;
        int         n_stacks;
        Atom        workarea_atom;
} NotifyScreen;

struct NdQueuePrivate
{
        GHashTable    *notifications;
        GHashTable    *bubbles;
        GQueue        *queue;

        GtkStatusIcon *status_icon;
        GtkWidget     *dock;
        GtkWidget     *dock_scrolled_window;

        NotifyScreen **screens;
        int            n_screens;

        guint          update_id;
};

enum {
        CHANGED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     nd_queue_class_init  (NdQueueClass *klass);
static void     nd_queue_init        (NdQueue      *queue);
static void     nd_queue_finalize    (GObject         *object);
static void     queue_update         (NdQueue *queue);
static void     on_notification_close (NdNotification *notification,
                                       int             reason,
                                       NdQueue        *queue);

static gpointer queue_object = NULL;

G_DEFINE_TYPE (NdQueue, nd_queue, G_TYPE_OBJECT)

static void
create_stack_for_monitor (NdQueue    *queue,
                          GdkScreen  *screen,
                          int         monitor_num)
{
        NotifyScreen *nscreen;
        int           screen_num;

        screen_num = gdk_screen_get_number (screen);
        nscreen = queue->priv->screens[screen_num];

        nscreen->stacks[monitor_num] = nd_stack_new (screen,
                                                     monitor_num);
}

static void
on_screen_monitors_changed (GdkScreen *screen,
                            NdQueue   *queue)
{
        NotifyScreen *nscreen;
        int           screen_num;
        int           n_monitors;
        int           i;

        screen_num = gdk_screen_get_number (screen);
        nscreen = queue->priv->screens[screen_num];

        n_monitors = gdk_screen_get_n_monitors (screen);

        if (n_monitors > nscreen->n_stacks) {
                /* grow */
                nscreen->stacks = g_renew (NdStack *,
                                           nscreen->stacks,
                                           n_monitors);

                /* add more stacks */
                for (i = nscreen->n_stacks; i < n_monitors; i++) {
                        create_stack_for_monitor (queue, screen, i);
                }

                nscreen->n_stacks = n_monitors;
        } else if (n_monitors < nscreen->n_stacks) {
                NdStack *last_stack;

                last_stack = nscreen->stacks[n_monitors - 1];

                /* transfer items before removing stacks */
                for (i = n_monitors; i < nscreen->n_stacks; i++) {
                        NdStack     *stack;
                        GList       *bubbles;
                        GList       *l;

                        stack = nscreen->stacks[i];
                        bubbles = g_list_copy (nd_stack_get_bubbles (stack));
                        for (l = bubbles; l != NULL; l = l->next) {
                                /* skip removing the bubble from the
                                   old stack since it will try to
                                   unrealize the window.  And the
                                   stack is going away anyhow. */
                                nd_stack_add_bubble (last_stack, l->data, TRUE);
                        }
                        g_list_free (bubbles);
                        g_object_unref (stack);
                        nscreen->stacks[i] = NULL;
                }

                /* remove the extra stacks */
                nscreen->stacks = g_renew (NdStack *,
                                           nscreen->stacks,
                                           n_monitors);
                nscreen->n_stacks = n_monitors;
        }
}

static void
create_stacks_for_screen (NdQueue   *queue,
                          GdkScreen *screen)
{
        NotifyScreen *nscreen;
        int           screen_num;
        int           i;

        screen_num = gdk_screen_get_number (screen);
        nscreen = queue->priv->screens[screen_num];

        nscreen->n_stacks = gdk_screen_get_n_monitors (screen);

        nscreen->stacks = g_renew (NdStack *,
                                   nscreen->stacks,
                                   nscreen->n_stacks);

        for (i = 0; i < nscreen->n_stacks; i++) {
                create_stack_for_monitor (queue, screen, i);
        }
}

static GdkFilterReturn
screen_xevent_filter (GdkXEvent    *xevent,
                      GdkEvent     *event,
                      NotifyScreen *nscreen)
{
        XEvent *xev;

        xev = (XEvent *) xevent;

        if (xev->type == PropertyNotify &&
            xev->xproperty.atom == nscreen->workarea_atom) {
                int i;

                for (i = 0; i < nscreen->n_stacks; i++) {
                        nd_stack_queue_update_position (nscreen->stacks[i]);
                }
        }

        return GDK_FILTER_CONTINUE;
}

static void
create_screens (NdQueue *queue)
{
        GdkDisplay  *display;
        int          i;

        g_assert (queue->priv->screens == NULL);

        display = gdk_display_get_default ();
        queue->priv->n_screens = gdk_display_get_n_screens (display);

        queue->priv->screens = g_new0 (NotifyScreen *, queue->priv->n_screens);

        for (i = 0; i < queue->priv->n_screens; i++) {
                GdkScreen *screen;
                GdkWindow *gdkwindow;

                screen = gdk_display_get_screen (display, i);
                g_signal_connect (screen,
                                  "monitors-changed",
                                  G_CALLBACK (on_screen_monitors_changed),
                                  queue);

                queue->priv->screens[i] = g_new0 (NotifyScreen, 1);

                queue->priv->screens[i]->workarea_atom = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "_NET_WORKAREA", True);
                gdkwindow = gdk_screen_get_root_window (screen);
                gdk_window_add_filter (gdkwindow, (GdkFilterFunc) screen_xevent_filter, queue->priv->screens[i]);
                gdk_window_set_events (gdkwindow, gdk_window_get_events (gdkwindow) | GDK_PROPERTY_CHANGE_MASK);

                create_stacks_for_screen (queue, gdk_display_get_screen (display, i));
        }
}

static void
nd_queue_class_init (NdQueueClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = nd_queue_finalize;

        signals [CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (NdQueueClass, changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_type_class_add_private (klass, sizeof (NdQueuePrivate));
}

static void
popdown_dock (NdQueue *queue)
{
        GdkDisplay *display;

        /* ungrab focus */
        display = gtk_widget_get_display (queue->priv->dock);
        gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
        gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
        gtk_grab_remove (queue->priv->dock);

        /* hide again */
        gtk_widget_hide (queue->priv->dock);

        queue_update (queue);
}

static void
release_grab (GtkWidget      *widget,
              GdkEventButton *event)
{
        GdkDisplay *display;

        /* ungrab focus */
        display = gtk_widget_get_display (widget);
        gdk_display_keyboard_ungrab (display, event->time);
        gdk_display_pointer_ungrab (display, event->time);
        gtk_grab_remove (widget);

        /* hide again */
        gtk_widget_hide (widget);
}

/* This is called when the grab is broken for
 * either the dock, or the scale itself */
static void
grab_notify (NdQueue   *queue,
             gboolean   was_grabbed)
{
        GtkWidget *current;

        if (was_grabbed) {
                return;
        }

        if (!gtk_widget_has_grab (queue->priv->dock)) {
                return;
        }

        current = gtk_grab_get_current ();
        if (current == queue->priv->dock
            || gtk_widget_is_ancestor (current, queue->priv->dock)) {
                return;
        }

        popdown_dock (queue);
}

static void
on_dock_grab_notify (GtkWidget *widget,
                     gboolean   was_grabbed,
                     NdQueue   *queue)
{
        grab_notify (queue, was_grabbed);
}

static gboolean
on_dock_grab_broken_event (GtkWidget *widget,
                           gboolean   was_grabbed,
                           NdQueue   *queue)
{
        grab_notify (queue, FALSE);

        return FALSE;
}

static gboolean
on_dock_key_release (GtkWidget   *widget,
                     GdkEventKey *event,
                     NdQueue     *queue)
{
        if (event->keyval == GDK_KEY_Escape) {
                popdown_dock (queue);
                return TRUE;
        }

        return TRUE;
}

static void
clear_stacks (NdQueue *queue)
{
        int i;
        int j;

        for (i = 0; i < queue->priv->n_screens; i++) {
                NotifyScreen *nscreen;
                nscreen = queue->priv->screens[i];
                for (j = 0; j < nscreen->n_stacks; j++) {
                       NdStack *stack;
                       stack = nscreen->stacks[j];
                       nd_stack_remove_all (stack);
                }
        }
}

static void
_nd_queue_remove_all (NdQueue *queue)
{
        GHashTableIter iter;
        gpointer       key, value;
        gboolean       changed;

        changed = FALSE;

        clear_stacks (queue);

        g_queue_clear (queue->priv->queue);
        g_hash_table_iter_init (&iter, queue->priv->notifications);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                NdNotification *n = ND_NOTIFICATION (value);

                g_signal_handlers_disconnect_by_func (n, G_CALLBACK (on_notification_close), queue);
                nd_notification_close (n, ND_NOTIFICATION_CLOSED_USER);
                g_hash_table_iter_remove (&iter);
                changed = TRUE;
        }
        popdown_dock (queue);
        queue_update (queue);

        if (changed) {
                g_signal_emit (queue, signals[CHANGED], 0);
        }
}

static void
on_clear_all_clicked (GtkButton *button,
                      NdQueue   *queue)
{
        _nd_queue_remove_all (queue);
}

static gboolean
on_dock_button_press (GtkWidget      *widget,
                      GdkEventButton *event,
                      NdQueue        *queue)
{
        GtkWidget *event_widget;

        if (event->type != GDK_BUTTON_PRESS) {
                return FALSE;
        }
        event_widget = gtk_get_event_widget ((GdkEvent *)event);
        g_debug ("Button press: %p dock=%p", event_widget, widget);
        if (event_widget == widget) {
                release_grab (widget, event);
                return TRUE;
        }

        return FALSE;
}

static void
create_dock (NdQueue *queue)
{
        GtkWidget *frame;
        GtkWidget *box;
        GtkWidget *button;

        queue->priv->dock = gtk_window_new (GTK_WINDOW_POPUP);
        gtk_widget_add_events (queue->priv->dock,
                               GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

        gtk_widget_set_name (queue->priv->dock, "notification-popup-window");
        g_signal_connect (queue->priv->dock,
                          "grab-notify",
                          G_CALLBACK (on_dock_grab_notify),
                          queue);
        g_signal_connect (queue->priv->dock,
                          "grab-broken-event",
                          G_CALLBACK (on_dock_grab_broken_event),
                          queue);
        g_signal_connect (queue->priv->dock,
                          "key-release-event",
                          G_CALLBACK (on_dock_key_release),
                          queue);
        g_signal_connect (queue->priv->dock,
                          "button-press-event",
                          G_CALLBACK (on_dock_button_press),
                          queue);
#if 0
        g_signal_connect (queue->priv->dock,
                          "scroll-event",
                          G_CALLBACK (on_dock_scroll_event),
                          queue);
#endif
        gtk_window_set_decorated (GTK_WINDOW (queue->priv->dock), FALSE);

        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
        gtk_container_add (GTK_CONTAINER (queue->priv->dock), frame);

        box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (box), 2);
        gtk_container_add (GTK_CONTAINER (frame), box);

        queue->priv->dock_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (queue->priv->dock_scrolled_window),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request (queue->priv->dock_scrolled_window,
                                     WIDTH,
                                     -1);
        gtk_box_pack_start (GTK_BOX (box), queue->priv->dock_scrolled_window, TRUE, TRUE, 0);

        button = gtk_button_new_with_label (_("Clear all notifications"));
        g_signal_connect (button, "clicked", G_CALLBACK (on_clear_all_clicked), queue);
        gtk_box_pack_end (GTK_BOX (box), button, FALSE, FALSE, 0);
}

static void
nd_queue_init (NdQueue *queue)
{
        queue->priv = ND_QUEUE_GET_PRIVATE (queue);
        queue->priv->notifications = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
        queue->priv->bubbles = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
        queue->priv->queue = g_queue_new ();
        queue->priv->status_icon = NULL;

        create_dock (queue);
        create_screens (queue);
}

static void
destroy_screens (NdQueue *queue)
{
        GdkDisplay  *display;
        int          i;
        int          j;

        display = gdk_display_get_default ();

        for (i = 0; i < queue->priv->n_screens; i++) {
                GdkScreen *screen;
                GdkWindow *gdkwindow;

                screen = gdk_display_get_screen (display, i);
                g_signal_handlers_disconnect_by_func (screen,
                                                      G_CALLBACK (on_screen_monitors_changed),
                                                      queue);

                gdkwindow = gdk_screen_get_root_window (screen);
                gdk_window_remove_filter (gdkwindow, (GdkFilterFunc) screen_xevent_filter, queue->priv->screens[i]);
                for (j = 0; i < queue->priv->screens[i]->n_stacks; j++) {
                        g_object_unref (queue->priv->screens[i]->stacks[j]);
                        queue->priv->screens[i]->stacks[j] = NULL;
                }

                g_free (queue->priv->screens[i]->stacks);
        }

        g_free (queue->priv->screens);
        queue->priv->screens = NULL;
}


static void
nd_queue_finalize (GObject *object)
{
        NdQueue *queue;

        g_return_if_fail (object != NULL);
        g_return_if_fail (ND_IS_QUEUE (object));

        queue = ND_QUEUE (object);

        g_return_if_fail (queue->priv != NULL);

        g_hash_table_destroy (queue->priv->notifications);
        g_queue_free (queue->priv->queue);

        destroy_screens (queue);

        G_OBJECT_CLASS (nd_queue_parent_class)->finalize (object);
}

NdNotification *
nd_queue_lookup (NdQueue *queue,
                 guint    id)
{
        NdNotification *notification;

        g_return_val_if_fail (ND_IS_QUEUE (queue), NULL);

        notification = g_hash_table_lookup (queue->priv->notifications, GUINT_TO_POINTER (id));

        return notification;
}

guint
nd_queue_length (NdQueue *queue)
{
        g_return_val_if_fail (ND_IS_QUEUE (queue), 0);

        return g_hash_table_size (queue->priv->notifications);
}

static NdStack *
get_stack_with_pointer (NdQueue *queue)
{
        GdkScreen *screen;
        int        x, y;
        int        screen_num;
        int        monitor_num;

        gdk_display_get_pointer (gdk_display_get_default (),
                                 &screen,
                                 &x,
                                 &y,
                                 NULL);
        screen_num = gdk_screen_get_number (screen);
        monitor_num = gdk_screen_get_monitor_at_point (screen, x, y);

        if (monitor_num >= queue->priv->screens[screen_num]->n_stacks) {
                /* screw it - dump it on the last one we'll get
                   a monitors-changed signal soon enough*/
                monitor_num = queue->priv->screens[screen_num]->n_stacks - 1;
        }

        return queue->priv->screens[screen_num]->stacks[monitor_num];
}

static void
on_bubble_destroyed (NdBubble *bubble,
                     NdQueue  *queue)
{
        g_debug ("Bubble destroyed");
        queue_update (queue);
}

static void
maybe_show_notification (NdQueue *queue)
{
        gpointer        id;
        NdNotification *notification;
        NdBubble       *bubble;
        NdStack        *stack;
        GList          *list;

        /* FIXME: show one at a time if not busy or away */

        /* don't show bubbles when dock is showing */
        if (gtk_widget_get_visible (queue->priv->dock)) {
                g_debug ("Dock is showing");
                return;
        }

        stack = get_stack_with_pointer (queue);
        list = nd_stack_get_bubbles (stack);
        if (g_list_length (list) > 0) {
                /* already showing bubbles */
                g_debug ("Already showing bubbles");
                return;
        }

        id = g_queue_pop_tail (queue->priv->queue);
        if (id == NULL) {
                /* Nothing to do */
                g_debug ("No queued notifications");
                return;
        }

        notification = g_hash_table_lookup (queue->priv->notifications, id);
        g_assert (notification != NULL);

        bubble = nd_bubble_new_for_notification (notification);
        g_signal_connect (bubble, "destroy", G_CALLBACK (on_bubble_destroyed), queue);

        nd_stack_add_bubble (stack, bubble, TRUE);
}

static int
collate_notifications (NdNotification *a,
                       NdNotification *b)
{
        GTimeVal tva;
        GTimeVal tvb;

        nd_notification_get_update_time (a, &tva);
        nd_notification_get_update_time (b, &tvb);
        if (tva.tv_sec > tvb.tv_sec) {
                return 1;
        } else {
                return -1;
        }
}

static void
on_close_button_clicked (GtkButton      *button,
                         NdNotification *notification)
{
        nd_notification_close (notification, ND_NOTIFICATION_CLOSED_USER);
}

static void
on_action_clicked (GtkButton      *button,
                   GdkEventButton *event,
                   NdNotification *notification)
{
        const char *key = g_object_get_data (G_OBJECT (button), "_action_key");

        nd_notification_action_invoked (notification,
                                        key);
}

static GtkWidget *
create_notification_action (NdQueue        *queue,
                            NdNotification *notification,
                            const char     *text,
                            const char     *key)
{
        GtkWidget *label;
        GtkWidget *button;
        GtkWidget *hbox;
        GdkPixbuf *pixbuf;
        char      *buf;

        button = gtk_button_new ();
        gtk_widget_show (button);
        gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
        gtk_container_set_border_width (GTK_CONTAINER (button), 0);

        hbox = gtk_hbox_new (FALSE, 6);
        gtk_widget_show (hbox);
        gtk_container_add (GTK_CONTAINER (button), hbox);

        /* Try to be smart and find a suitable icon. */
        buf = g_strdup_printf ("stock_%s", key);
        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (queue->priv->dock))),
                                           buf,
                                           16,
                                           GTK_ICON_LOOKUP_USE_BUILTIN,
                                           NULL);
        g_free (buf);

        if (pixbuf != NULL) {
                GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
                gtk_widget_show (image);
                gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
                gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);
        }

        label = gtk_label_new (NULL);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
        buf = g_strdup_printf ("<small>%s</small>", text);
        gtk_label_set_markup (GTK_LABEL (label), buf);
        g_free (buf);

        g_object_set_data_full (G_OBJECT (button),
                                "_action_key", g_strdup (key), g_free);
        g_signal_connect (G_OBJECT (button),
                          "button-release-event",
                          G_CALLBACK (on_action_clicked),
                          notification);
        return button;
}

static gboolean
on_button_release (GtkWidget      *widget,
                   GdkEventButton *event,
                   NdNotification *notification)
{
        g_debug ("CLICK");
        nd_notification_action_invoked (notification, "default");

        return FALSE;
}

static GtkWidget *
create_notification_box (NdQueue        *queue,
                         NdNotification *n)
{
        GtkWidget     *event_box;
        GtkWidget     *box;
        GtkWidget     *iconbox;
        GtkWidget     *icon;
        GtkWidget     *image;
        GtkWidget     *content_hbox;
        GtkWidget     *actions_box;
        GtkWidget     *vbox;
        GtkWidget     *summary_label;
        GtkWidget     *body_label;
        GtkWidget     *alignment;
        GtkWidget     *close_button;
        AtkObject     *atkobj;
        GtkRcStyle    *rcstyle;
        char          *str;
        char          *quoted;
        GtkRequisition req;
        int            summary_width;
        gboolean       have_icon;
        gboolean       have_body;
        gboolean       have_actions;
        GdkPixbuf     *pixbuf;
        char         **actions;
        int            i;

        event_box = gtk_event_box_new ();
        gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box),
                                          FALSE);
        g_signal_connect (event_box, "button-release-event", G_CALLBACK (on_button_release), n);

        box = gtk_hbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (event_box), box);
        gtk_widget_show (box);

        /* First row (icon, vbox, close) */
        iconbox = gtk_alignment_new (0.5, 0, 0, 0);
        gtk_widget_show (iconbox);
        gtk_alignment_set_padding (GTK_ALIGNMENT (iconbox),
                                   5, 0, 0, 0);
        gtk_box_pack_start (GTK_BOX (box),
                            iconbox,
                            FALSE, FALSE, 0);
        gtk_widget_set_size_request (iconbox, BODY_X_OFFSET, -1);

        icon = gtk_image_new ();
        gtk_widget_show (icon);
        gtk_container_add (GTK_CONTAINER (iconbox), icon);

        vbox = gtk_vbox_new (FALSE, 6);
        gtk_widget_show (vbox);
        gtk_box_pack_start (GTK_BOX (box), vbox, TRUE, TRUE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

        /* Add the close button */
        alignment = gtk_alignment_new (0.5, 0, 0, 0);
        gtk_widget_show (alignment);
        gtk_box_pack_start (GTK_BOX (box), alignment, FALSE, FALSE, 0);

        close_button = gtk_button_new ();
        gtk_widget_show (close_button);
        gtk_container_add (GTK_CONTAINER (alignment), close_button);
        gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
        gtk_container_set_border_width (GTK_CONTAINER (close_button), 0);
        g_signal_connect (G_OBJECT (close_button),
                          "clicked",
                          G_CALLBACK (on_close_button_clicked),
                          n);

        rcstyle = gtk_rc_style_new ();
        rcstyle->xthickness = rcstyle->ythickness = 0;
        gtk_widget_modify_style (close_button, rcstyle);
        g_object_unref (rcstyle);

        atkobj = gtk_widget_get_accessible (close_button);
        atk_action_set_description (ATK_ACTION (atkobj), 0,
                                    "Closes the notification.");
        atk_object_set_name (atkobj, "");
        atk_object_set_description (atkobj, "Closes the notification.");

        image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
        gtk_widget_show (image);
        gtk_container_add (GTK_CONTAINER (close_button), image);

        /* center vbox */
        summary_label = gtk_label_new (NULL);
        gtk_widget_show (summary_label);
        gtk_box_pack_start (GTK_BOX (vbox), summary_label, TRUE, TRUE, 0);
        gtk_misc_set_alignment (GTK_MISC (summary_label), 0, 0);
        gtk_label_set_line_wrap (GTK_LABEL (summary_label), TRUE);

        atkobj = gtk_widget_get_accessible (summary_label);
        atk_object_set_description (atkobj, "Notification summary text.");

        content_hbox = gtk_hbox_new (FALSE, 6);
        gtk_widget_show (content_hbox);
        gtk_box_pack_start (GTK_BOX (vbox), content_hbox, FALSE, FALSE, 0);

        vbox = gtk_vbox_new (FALSE, 6);

        gtk_widget_show (vbox);
        gtk_box_pack_start (GTK_BOX (content_hbox), vbox, TRUE, TRUE, 0);

        body_label = gtk_label_new (NULL);
        gtk_widget_show (body_label);
        gtk_box_pack_start (GTK_BOX (vbox), body_label, TRUE, TRUE, 0);
        gtk_misc_set_alignment (GTK_MISC (body_label), 0, 0);
        gtk_label_set_line_wrap (GTK_LABEL (body_label), TRUE);

        atkobj = gtk_widget_get_accessible (body_label);
        atk_object_set_description (atkobj, "Notification body text.");

        alignment = gtk_alignment_new (1, 0.5, 0, 0);
        gtk_widget_show (alignment);
        gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, TRUE, 0);

        actions_box = gtk_hbox_new (FALSE, 6);
        gtk_widget_show (actions_box);
        gtk_container_add (GTK_CONTAINER (alignment), actions_box);

        /* Add content */

        have_icon = FALSE;
        have_body = FALSE;
        have_actions = FALSE;

        /* image */
        pixbuf = nd_notification_load_image (n, IMAGE_SIZE);
        if (pixbuf != NULL) {
                gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);

                g_object_unref (G_OBJECT (pixbuf));
                have_icon = TRUE;
        }

        /* summary */
        quoted = g_markup_escape_text (nd_notification_get_summary (n), -1);
        str = g_strdup_printf ("<b><big>%s</big></b>", quoted);
        g_free (quoted);

        gtk_label_set_markup (GTK_LABEL (summary_label), str);
        g_free (str);

        gtk_widget_size_request (close_button, &req);
        /* -1: main_vbox border width
           -10: vbox border width
           -6: spacing for hbox */
        summary_width = WIDTH - (1*2) - (10*2) - BODY_X_OFFSET - req.width - (6*2);

        gtk_widget_set_size_request (summary_label,
                                     summary_width,
                                     -1);

        /* body */
        gtk_label_set_markup (GTK_LABEL (body_label), nd_notification_get_body (n));

        if (str != NULL && *str != '\0') {
                gtk_widget_set_size_request (body_label,
                                             summary_width,
                                             -1);
                have_body = TRUE;
        }

        /* actions */
        actions = nd_notification_get_actions (n);
        for (i = 0; actions[i] != NULL; i += 2) {
                char *l = actions[i + 1];

                if (l == NULL) {
                        g_warning ("Label not found for action %s. "
                                   "The protocol specifies that a label must "
                                   "follow an action in the actions array",
                                   actions[i]);

                        break;
                }

                if (strcasecmp (actions[i], "default") != 0) {
                        GtkWidget *button;

                        button = create_notification_action (queue,
                                                             n,
                                                             l,
                                                             actions[i]);
                        gtk_box_pack_start (GTK_BOX (actions_box), button, FALSE, FALSE, 0);

                        have_actions = TRUE;
                }
        }

        if (have_icon || have_body || have_actions) {
                gtk_widget_show (content_hbox);
        } else {
                gtk_widget_hide (content_hbox);
        }

        return event_box;
}

static void
update_dock (NdQueue *queue)
{
        GtkWidget   *child;
        GList       *list;
        GList       *l;
        int          min_height;
        int          height;
        int          monitor_num;
        GdkScreen   *screen;
        GdkRectangle area;

        g_return_if_fail (queue);

        child = gtk_bin_get_child (GTK_BIN (queue->priv->dock_scrolled_window));
        if (child != NULL)
                gtk_container_remove (GTK_CONTAINER (queue->priv->dock_scrolled_window), child);

        child = gtk_vbox_new (FALSE, 6);
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (queue->priv->dock_scrolled_window),
                                               child);
        gtk_container_set_focus_hadjustment (GTK_CONTAINER (child),
                                             gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (queue->priv->dock_scrolled_window)));
        gtk_container_set_focus_vadjustment (GTK_CONTAINER (child),
                                             gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (queue->priv->dock_scrolled_window)));

        list = g_hash_table_get_values (queue->priv->notifications);
        list = g_list_sort (list, (GCompareFunc)collate_notifications);

        for (l = list; l != NULL; l = l->next) {
                NdNotification *n = l->data;
                GtkWidget      *hbox;
                GtkWidget      *sep;

                hbox = create_notification_box (queue, n);
                gtk_widget_show (hbox);
                gtk_box_pack_start (GTK_BOX (child), hbox, FALSE, FALSE, 0);

                sep = gtk_hseparator_new ();
                gtk_widget_show (sep);
                gtk_box_pack_start (GTK_BOX (child), sep, FALSE, FALSE, 0);
        }
        gtk_widget_show (child);

        if (queue->priv->status_icon != NULL
            && gtk_status_icon_get_visible (GTK_STATUS_ICON (queue->priv->status_icon))) {
                gtk_widget_get_preferred_height (child,
                                                 &min_height,
                                                 &height);
                gtk_status_icon_get_geometry (GTK_STATUS_ICON (queue->priv->status_icon),
                                              &screen,
                                              &area,
                                              NULL);
                monitor_num = gdk_screen_get_monitor_at_point (screen, area.x, area.y);
                gdk_screen_get_monitor_geometry (screen, monitor_num, &area);
                height = MIN (height, (area.height / 2));
                gtk_widget_set_size_request (queue->priv->dock_scrolled_window,
                                             WIDTH,
                                             height);
        }

        g_list_free (list);
}

static gboolean
popup_dock (NdQueue *queue,
            guint    time)
{
        GdkRectangle   area;
        GtkOrientation orientation;
        GdkDisplay    *display;
        GdkScreen     *screen;
        gboolean       res;
        int            x;
        int            y;
        int            monitor_num;
        GdkRectangle   monitor;
        GtkRequisition dock_req;

        update_dock (queue);

        res = gtk_status_icon_get_geometry (GTK_STATUS_ICON (queue->priv->status_icon),
                                            &screen,
                                            &area,
                                            &orientation);
        if (! res) {
                g_warning ("Unable to determine geometry of status icon");
                return FALSE;
        }

        /* position roughly */
        gtk_window_set_screen (GTK_WINDOW (queue->priv->dock), screen);

        monitor_num = gdk_screen_get_monitor_at_point (screen, area.x, area.y);
        gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

        gtk_container_foreach (GTK_CONTAINER (queue->priv->dock),
                               (GtkCallback) gtk_widget_show_all, NULL);
        gtk_widget_get_preferred_size (queue->priv->dock, &dock_req, NULL);

        if (orientation == GTK_ORIENTATION_VERTICAL) {
                if (area.x + area.width + dock_req.width <= monitor.x + monitor.width) {
                        x = area.x + area.width;
                } else {
                        x = area.x - dock_req.width;
                }
                if (area.y + dock_req.height <= monitor.y + monitor.height) {
                        y = area.y;
                } else {
                        y = monitor.y + monitor.height - dock_req.height;
                }
        } else {
                if (area.y + area.height + dock_req.height <= monitor.y + monitor.height) {
                        y = area.y + area.height;
                } else {
                        y = area.y - dock_req.height;
                }
                if (area.x + dock_req.width <= monitor.x + monitor.width) {
                        x = area.x;
                } else {
                        x = monitor.x + monitor.width - dock_req.width;
                }
        }

        gtk_window_move (GTK_WINDOW (queue->priv->dock), x, y);

        /* FIXME: without this, the popup window appears as a square
         * after changing the orientation
         */
        gtk_window_resize (GTK_WINDOW (queue->priv->dock), 1, 1);

        gtk_widget_show_all (queue->priv->dock);

        /* grab focus */
        gtk_grab_add (queue->priv->dock);

        if (gdk_pointer_grab (gtk_widget_get_window (queue->priv->dock), TRUE,
                              GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK, NULL, NULL,
                              time)
            != GDK_GRAB_SUCCESS) {
                gtk_grab_remove (queue->priv->dock);
                gtk_widget_hide (queue->priv->dock);
                return FALSE;
        }

        if (gdk_keyboard_grab (gtk_widget_get_window (queue->priv->dock), TRUE, time) != GDK_GRAB_SUCCESS) {
                display = gtk_widget_get_display (queue->priv->dock);
                gdk_display_pointer_ungrab (display, time);
                gtk_grab_remove (queue->priv->dock);
                gtk_widget_hide (queue->priv->dock);
                return FALSE;
        }

        gtk_widget_grab_focus (queue->priv->dock);

        return TRUE;
}

static void
show_dock (NdQueue *queue)
{
        /* clear the bubble queue since the user will be looking at a
           full list now */
        clear_stacks (queue);
        g_queue_clear (queue->priv->queue);

        popup_dock (queue, GDK_CURRENT_TIME);
}

static void
on_status_icon_popup_menu (GtkStatusIcon *status_icon,
                           guint          button,
                           guint          activate_time,
                           NdQueue       *queue)
{
        show_dock (queue);
}

static void
on_status_icon_activate (GtkStatusIcon *status_icon,
                         NdQueue       *queue)
{
        show_dock (queue);
}

static void
on_status_icon_visible_notify (GtkStatusIcon *icon,
                               GParamSpec    *pspec,
                               NdQueue       *queue)
{
        gboolean visible;

        g_object_get (icon, "visible", &visible, NULL);
        if (! visible) {
                if (queue->priv->dock != NULL) {
                        gtk_widget_hide (queue->priv->dock);
                }
        }
}

static gboolean
update_idle (NdQueue *queue)
{
        /* Show the status icon when their are stored notifications */
        if (g_hash_table_size (queue->priv->notifications) > 0) {
                if (gtk_widget_get_visible (queue->priv->dock)) {
                        update_dock (queue);
                }

                if (queue->priv->status_icon == NULL) {
                        queue->priv->status_icon = gtk_status_icon_new_from_icon_name ("mail-message-new");
                        gtk_status_icon_set_title (GTK_STATUS_ICON (queue->priv->status_icon),
                                                   _("Notifications"));
                        g_signal_connect (queue->priv->status_icon,
                                          "activate",
                                          G_CALLBACK (on_status_icon_activate),
                                          queue);
                        g_signal_connect (queue->priv->status_icon,
                                          "popup-menu",
                                          G_CALLBACK (on_status_icon_popup_menu),
                                          queue);
                        g_signal_connect (queue->priv->status_icon,
                                          "notify::visible",
                                          G_CALLBACK (on_status_icon_visible_notify),
                                          queue);
                }
                gtk_status_icon_set_visible (queue->priv->status_icon, TRUE);

                maybe_show_notification (queue);
        } else {
                if (gtk_widget_get_visible (queue->priv->dock)) {
                        popdown_dock (queue);
                }

                if (queue->priv->status_icon != NULL) {
                        g_object_unref (queue->priv->status_icon);
                        queue->priv->status_icon = NULL;
                }
        }

        return FALSE;
}

static void
queue_update (NdQueue *queue)
{
        if (queue->priv->update_id > 0) {
                g_source_remove (queue->priv->update_id);
        }

        queue->priv->update_id = g_idle_add ((GSourceFunc)update_idle, queue);
}

static void
_nd_queue_remove (NdQueue        *queue,
                  NdNotification *notification)
{
        guint id;

        id = nd_notification_get_id (notification);
        g_debug ("Removing id %u", id);

        /* FIXME: withdraw currently showing bubbles */

        g_signal_handlers_disconnect_by_func (notification, G_CALLBACK (on_notification_close), queue);

        if (queue->priv->queue != NULL) {
                g_queue_remove (queue->priv->queue, GUINT_TO_POINTER (id));
        }
        g_hash_table_remove (queue->priv->notifications, GUINT_TO_POINTER (id));

        /* FIXME: should probably only emit this when it really removes something */
        g_signal_emit (queue, signals[CHANGED], 0);

        queue_update (queue);
}

static void
on_notification_close (NdNotification *notification,
                       int             reason,
                       NdQueue        *queue)
{
        g_debug ("Notification closed - removing from queue");
        _nd_queue_remove (queue, notification);
}

void
nd_queue_remove_for_id (NdQueue *queue,
                        guint    id)
{
        NdNotification *notification;

        g_return_if_fail (ND_IS_QUEUE (queue));

        notification = g_hash_table_lookup (queue->priv->notifications, GUINT_TO_POINTER (id));
        if (notification != NULL) {
                _nd_queue_remove (queue, notification);
        }
}

void
nd_queue_add (NdQueue        *queue,
              NdNotification *notification)
{
        guint id;

        g_return_if_fail (ND_IS_QUEUE (queue));

        id = nd_notification_get_id (notification);
        g_debug ("Adding id %u", id);
        g_hash_table_insert (queue->priv->notifications, GUINT_TO_POINTER (id), g_object_ref (notification));
        g_queue_push_head (queue->priv->queue, GUINT_TO_POINTER (id));

        g_signal_connect (notification, "closed", G_CALLBACK (on_notification_close), queue);

        /* FIXME: should probably only emit this when it really adds something */
        g_signal_emit (queue, signals[CHANGED], 0);

        queue_update (queue);
}

NdQueue *
nd_queue_new (void)
{
        if (queue_object != NULL) {
                g_object_ref (queue_object);
        } else {
                queue_object = g_object_new (ND_TYPE_QUEUE, NULL);
                g_object_add_weak_pointer (queue_object,
                                           (gpointer *) &queue_object);
        }

        return ND_QUEUE (queue_object);
}
