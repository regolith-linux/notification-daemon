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
    GtkWindow *window; /* the popup window. this has a black background to give the border */
	GtkWidget *hbox, *vbox, *summary, *body, *image;

	void boldify(GtkLabel *label) {
		PangoAttribute *bold = pango_attr_weight_new(PANGO_WEIGHT_BOLD);		
		PangoAttrList *attrs = pango_attr_list_new();

		bold->start_index = 0;
		bold->end_index = G_MAXINT;
		
		pango_attr_list_insert(attrs, bold);
		
		gtk_label_set_attributes(label, attrs);
		
		pango_attr_list_unref(attrs);
	}
	
    PopupNotification() {
        Notification::Notification();

        TRACE("initializing new PopupNotification object (%d)\n", id);

        const int width = 200;
        const int height = 50;

        window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
        gtk_window_set_default_size(window, width, height);

        /* FIXME: calculate border offsets from NETWM window geometries */
        gtk_window_set_gravity(window, GDK_GRAVITY_SOUTH_EAST);
        gtk_window_move(window, gdk_screen_width() - width, gdk_screen_height() - height);

        hbox = gtk_hbox_new(FALSE, 4);
        vbox = gtk_vbox_new(FALSE, 2);
        image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_LARGE_TOOLBAR); // FIXME

		summary = gtk_label_new("summary");
		boldify(GTK_LABEL(summary));
		
		body = gtk_label_new("body");
		
		gtk_widget_show(summary);
		gtk_widget_show(body);
		
        gtk_box_pack_start_defaults(GTK_BOX(vbox), summary);
        gtk_box_pack_end_defaults(GTK_BOX(vbox), body);
        
        gtk_box_pack_end_defaults(GTK_BOX(hbox), vbox);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), image);

		gtk_widget_show(image);
		gtk_widget_show(vbox);
		gtk_widget_show(hbox);
		
		gtk_container_add(GTK_CONTAINER(window), hbox);

		TRACE("done\n");
    }

    ~PopupNotification() {
        TRACE("destroying notification %d\n", id);
        gtk_widget_hide(GTK_WIDGET(window));
        g_object_unref(window);
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
