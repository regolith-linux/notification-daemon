/** -*- mode: c++-mode; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; -*-
 * @file popup-notifier.cpp GTK+ based popup notifier
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
private:
	static const int width = 300; // FIXME: make these relative to screen size

    GtkWindow *window; /* the popup window. this has a black background to give the border */
	GtkWidget *hbox, *vbox, *summary_label, *body_label, *image;

	int height_offset;

	void boldify(GtkLabel *label) {
		PangoAttribute *bold = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
		PangoAttrList *attrs = pango_attr_list_new();

		bold->start_index = 0;
		bold->end_index = G_MAXINT;

		pango_attr_list_insert(attrs, bold);

		gtk_label_set_attributes(label, attrs);

		pango_attr_list_unref(attrs);
	}

	void greyify(GtkWidget *widget) {
		GdkColor color;
		gdk_color_parse("black", &color);
		gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &color);
	}

	GdkGC *gc;

	static gboolean draw_border(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
		PopupNotification *n = (PopupNotification *) user_data;

		if (!n->gc) {
			n->gc = gdk_gc_new(event->window);

			GdkColor color;
			gdk_color_parse("black", &color);
			gdk_gc_set_rgb_fg_color(n->gc, &color);
		}

		int width, height;
		gdk_drawable_get_size(event->window, &width, &height);

		gdk_draw_rectangle(event->window, n->gc, FALSE, 0, 0, width-1, height-1);

		return FALSE; // propogate further
	}

public:

	PopupNotification() {
        Notification::Notification();
		gc = NULL;
		window = NULL;
		height_offset = 0;
    }

    ~PopupNotification() {
        TRACE("destroying notification %d, windows=%p\n", id, window);
        gtk_widget_hide(GTK_WIDGET(window));
		g_object_unref(gc);
        //g_object_unref(G_OBJECT(window));  -- this crashes with an invalid cast in gtk_object_dispose, why?
    }

	void generate() {
        TRACE("Generating new PopupNotification GUI for nid %d\n", id);

		const int image_padding = 15;

        window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));

        hbox = gtk_hbox_new(FALSE, 4);
        vbox = gtk_vbox_new(FALSE, 2);
        image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_LARGE_TOOLBAR); // FIXME

		/* now set up the labels containing the notification text */
		summary_label = gtk_label_new(summary);
		boldify(GTK_LABEL(summary_label));
		gtk_misc_set_alignment(GTK_MISC(summary_label), 0, 0.5);

		body_label = gtk_label_new(body);
		gtk_label_set_use_markup(GTK_LABEL(body_label), TRUE);
		gtk_label_set_line_wrap(GTK_LABEL(body_label), TRUE);
		gtk_misc_set_alignment(GTK_MISC(body_label), 0, 0.5);

		/* we want to fix the width so the notifications expand upwards but not outwards.
		   firstly, we need to grab the natural size request of the containing box, then we
		   need to set the size request of the label to that width so it will always line wrap.
		 */

		GtkRequisition req;
		gtk_widget_size_request(image, &req);
		gtk_widget_set_size_request(body_label, width - (req.width + image_padding) - 10 /* FIXME */, -1);

		gtk_widget_show(summary_label);
		gtk_widget_show(body_label);

        gtk_box_pack_start(GTK_BOX(vbox), summary_label, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(vbox), body_label, TRUE, TRUE, 10);

        gtk_box_pack_end_defaults(GTK_BOX(hbox), vbox);
        gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, image_padding);

		/* now we setup an expose event handler to draw the border */
		g_signal_connect(G_OBJECT(window), "expose-event", G_CALLBACK(draw_border), this);
		gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);

		gtk_widget_show(image);
		gtk_widget_show(vbox);
		gtk_widget_show(hbox);

		gtk_container_add(GTK_CONTAINER(window), hbox);

        /* FIXME: calculate border offsets from NETWM window geometries */
        gtk_window_set_gravity(window, GDK_GRAVITY_SOUTH_EAST);
		update_position();

		TRACE("window is %p\n", window);
	}

	void show() {
		if (!window) generate();
		gtk_widget_show(GTK_WIDGET(window));
	}

	/* returns the natural height of the notification. generates the gui if not done so already */
	int height() {
		if (!window) generate();

		GtkRequisition req;
		gtk_widget_size_request(GTK_WIDGET(window), &req);
		return req.height;
	}

	void update_position() {
		if (!window) generate();
		gtk_window_move(window, gdk_screen_width() - width, gdk_screen_height() - height() - height_offset);
	}

	void set_height_offset(int value) {
		height_offset = value;
		update_position();
	}
};

PopupNotifier::PopupNotifier(GMainLoop *main_loop, int *argc, char ***argv) : BaseNotifier(main_loop)
{
    gtk_init(argc, argv);
}

/* This method is responsible for calculating the height offsets of all currently
   displayed notifications. In future, it may take into account animations and such.

   This may be called many times per second so it should be reasonably fast.
 */

void
PopupNotifier::reflow()
{
	NotificationsMap::iterator i = notifications.begin();

	/* the height offset is the distance from the top/bottom of the screen to the
	   nearest edge of the popup */
	int offset = 0;

	while (i != notifications.end()) {
		PopupNotification *n = dynamic_cast<PopupNotification*> (i->second);

		n->set_height_offset(offset);

		offset += n->height();

		i++;
	}
}

uint
PopupNotifier::notify(Notification *base)
{
	uint id = BaseNotifier::notify(base);
    PopupNotification *n = dynamic_cast<PopupNotification*> (base);

	reflow();

	TRACE("height is %d, timeout in unix-time is %d\n", n->height(), n->timeout);

    n->show();

    return id;
}

bool
PopupNotifier::unnotify(Notification *n)
{
	bool ret = BaseNotifier::unnotify(n);
	reflow();
	return ret;
}

Notification*
PopupNotifier::create_notification()
{
    return new PopupNotification();
}
