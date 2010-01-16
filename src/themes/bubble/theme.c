/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include <gtk/gtk.h>
#include "eggnotificationbubblewidget.h"

typedef void (*ActionInvokedCb)(GtkWindow *nw, const char *key);

GtkWindow *
create_notification(void)
{
        return GTK_WINDOW(egg_notification_bubble_widget_new());
}

void
show_notification(GtkWindow *nw)
{
        egg_notification_bubble_widget_show(EGG_NOTIFICATION_BUBBLE_WIDGET(nw));
}

void
hide_notification(GtkWindow *nw)
{
        egg_notification_bubble_widget_hide(EGG_NOTIFICATION_BUBBLE_WIDGET(nw));
}

void
set_notification_hints(GtkWindow *nw, GHashTable *hints)
{
        egg_notification_bubble_widget_set_hints(
                EGG_NOTIFICATION_BUBBLE_WIDGET(nw), hints);
}

void
set_notification_text(GtkWindow *nw, const char *summary, const char *body)
{
        egg_notification_bubble_widget_set(EGG_NOTIFICATION_BUBBLE_WIDGET(nw),
                                                                           summary, NULL, body);
}

void
set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf)
{
        EggNotificationBubbleWidget *bubble = EGG_NOTIFICATION_BUBBLE_WIDGET(nw);

        gtk_image_set_from_pixbuf(GTK_IMAGE(bubble->icon), pixbuf);
}

void
set_notification_arrow(GtkWindow *nw, gboolean visible, int x, int y)
{
        egg_notification_bubble_widget_set_draw_arrow(
                EGG_NOTIFICATION_BUBBLE_WIDGET(nw), visible);
}

static void
action_clicked_cb(GtkWidget *w, ActionInvokedCb action_cb)
{
        GtkWindow *nw   = g_object_get_data(G_OBJECT(w), "_nw");
        const char *key = g_object_get_data(G_OBJECT(w), "_action_key");

        action_cb(nw, key);
}

void
add_notification_action(GtkWindow *nw, const char *label, const char *key,
                                                ActionInvokedCb cb)
{
        GtkWidget *b = egg_notification_bubble_widget_create_button(
                EGG_NOTIFICATION_BUBBLE_WIDGET(nw), label);
        g_object_set_data(G_OBJECT(b), "_nw", nw);
        g_object_set_data_full(G_OBJECT(b), "_action_key", g_strdup(key), g_free);

        g_signal_connect(G_OBJECT(b), "clicked",
                                         G_CALLBACK(action_clicked_cb), cb);
}

void
move_notification(GtkWindow *nw, int x, int y)
{
        egg_notification_bubble_widget_set_pos(EGG_NOTIFICATION_BUBBLE_WIDGET(nw),
                                                                                   x, y);
}
