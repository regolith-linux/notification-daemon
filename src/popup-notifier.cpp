/** -*- mode: c++-mode; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; -*-
 * @file popup-notifier.cpp Base class for popup notifiers
 *
 * Copyright (C) 2004 Mike Hearn
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

#include <gtk/gtk.h>

#include "notifier.h"
#include "logging.h"


class PopupNotification : public Notification {
public:    
    GtkWindow *window; /* the popup window */

    PopupNotification() {
        Notification::Notification();

        TRACE("initializing new PopupNotification object (%d)\n", id);

        const int width = 200;
        const int height = 100;
        
        window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
        gtk_window_set_default_size(window, width, height);

        /* FIXME: calculate border offsets from NETWM window geometries */
        gtk_window_set_gravity(window, GDK_GRAVITY_SOUTH_EAST);
        gtk_window_move (window, gdk_screen_width() - width, gdk_screen_height() - height);

    }

    ~PopupNotification() {
        TRACE("destroying notification %d\n", id);
        gtk_widget_hide(GTK_WIDGET(window));
        g_object_unref(window);
    }

    /* build the GTK widget contents of the popup */
    void generate() {
        
    }
};

PopupNotifier::PopupNotifier(GMainLoop *loop, int *argc, char ***argv)
{
    gtk_init(argc, argv);
}

uint
PopupNotifier::notify(Notification *base)
{
    PopupNotification *n = dynamic_cast<PopupNotification*> (base);

    gtk_widget_show(GTK_WIDGET(n->window));
    
    return BaseNotifier::notify(base);
}

bool
PopupNotifier::unnotify(uint id)
{
    return BaseNotifier::unnotify(id);
}

Notification*
PopupNotifier::create_notification()
{
    return new PopupNotification();
}
