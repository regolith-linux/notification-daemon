/**
 * @file PopupNotification.cpp GTK+ based popup notifier
 *
 * Copyright (C) 2004 Mike Hearn <mike@navi.cx>
 * Copyright (C) 2004 Christian Hammond <chipx86@chipx86.com>
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

#define GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifndef _WIN32
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/Xatom.h>
# include <gdk/gdkx.h>
#endif

#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>

#include "PopupNotification.hh"
#include "logging.hh"
#include "sexy-url-label.h"

#include <assert.h>

#define S(str) std::string(str)

struct expose_data
{
    GtkWidget *widget;
    gulong handler_id;
};

void
PopupNotification::format_summary(GtkLabel *label)
{
    PangoAttribute *bold = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
    PangoAttribute *large = pango_attr_scale_new(1.2);
    PangoAttrList *attrs = pango_attr_list_new();

    bold->start_index = large->start_index = 0;
    bold->end_index = large->end_index = G_MAXINT;

    pango_attr_list_insert(attrs, bold);
    pango_attr_list_insert(attrs, large);

    gtk_label_set_attributes(label, attrs);

    pango_attr_list_unref(attrs);

    /* the attributes aren't leaked, they are now owned by GTK  */
}

void
PopupNotification::process_body_markup(GtkLabel *label)
{
    /* we can't use pango markup here because ... zzzzzzzzzzzzz  */
}

/* Make a label blue and underlined */
void
PopupNotification::linkify(GtkLabel *label)
{
    PangoAttribute *blue = pango_attr_foreground_new(0, 0, 65535);
    PangoAttribute *underline = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
    PangoAttrList *attrs = pango_attr_list_new();

    blue->start_index = underline->start_index = 0;
    blue->end_index = underline->end_index = G_MAXINT;

    pango_attr_list_insert(attrs, blue);
    pango_attr_list_insert(attrs, underline);

    gtk_label_set_attributes(label, attrs);

    pango_attr_list_unref(attrs);
}

void
PopupNotification::whiten(GtkWidget *eventbox)
{
    GdkColor white;
    gdk_color_parse("white", &white);

    GtkStyle *style = gtk_widget_get_style(eventbox);   /* override the theme. white is GOOD dammit :) */
    style->bg[GTK_STATE_NORMAL] = white;

    gtk_widget_set_style(eventbox, style);
}

static gboolean
draw_border(GtkWidget *widget, GdkEventExpose *event, PopupNotification *n)
{
    if (!n->gc)
    {
        n->gc = gdk_gc_new(event->window);

        GdkColor color;
        gdk_color_parse("black", &color);
        gdk_gc_set_rgb_fg_color(n->gc, &color);
    }

    int w, h;
    gdk_drawable_get_size(event->window, &w, &h);

	if (n->has_arrow())
	{
		gdk_draw_polygon(event->window, n->gc, FALSE, n->mArrowPoints,
						 G_N_ELEMENTS(n->mArrowPoints));

		/* HACK! HACK! HACK! */
		gdk_draw_line(event->window, n->gc,
					  n->ARROW_OFFSET + 1, n->ARROW_HEIGHT,
					  n->ARROW_OFFSET + n->ARROW_WIDTH / 2 + 1, 0);
		gdk_draw_line(event->window, n->gc,
					  n->ARROW_OFFSET + n->ARROW_WIDTH / 2 - 1, 0,
					  n->ARROW_OFFSET + n->ARROW_WIDTH - 1, n->ARROW_HEIGHT);
		gdk_draw_line(event->window, n->gc, 0, h - 1, w - 1, h - 1);
	}
	else
	{
		gdk_draw_rectangle(event->window, n->gc, FALSE, 0, 0, w-1, h-1);
	}

    return FALSE; /* propogate further */
}

void
PopupNotification::window_button_release(GdkEventButton *event)
{
	const ActionsMap &actions = GetActions();

    if (actions.find(0) == actions.end())
		notifier->unnotify(this);
    else
		action_invoke(0);
}

static gboolean
_window_button_release(GtkWidget *widget, GdkEventButton *event,
                       PopupNotification *n)
{
    n->window_button_release(event);
    return TRUE;
}

static gboolean
_action_label_click(GtkWidget * widget, GdkEventButton * event, uint *action)
{
    PopupNotification *n = (PopupNotification *) g_object_get_data(G_OBJECT(widget), "notification-instance");
    n->action_invoke(*action);
    return TRUE;
}

/*
 * this ensures that the hyperlinks have a hand cursor. it's done
 * in a callback because we have to wait for the window to be
 * realized (ie for an X window to be created for the widget)
 */
static gboolean
_set_cursor(GtkWidget *widget, GdkEventExpose *event,
            struct expose_data *data)
{

    GdkCursor *cursor = gdk_cursor_new(GDK_HAND2);
    gdk_window_set_cursor(event->window, cursor);
    gdk_cursor_unref(cursor);

    g_signal_handler_disconnect(data->widget, data->handler_id);

    delete data;

    return FALSE; /* propogate further  */
}

PopupNotification::PopupNotification(PopupNotifier *n,
                                     DBusConnection *dbusConn)
    : Notification(dbusConn),
      notifier(n),
      window(NULL),
      disp_screen(0),
      height_offset(0),
      gc(NULL)
{
}

bool
PopupNotification::has_arrow(void)
	const
{
	return HasHint("x") && HasHint("y");
}

void
PopupNotification::generate_arrow(int &ret_arrow_x, int &ret_arrow_y)
{
	if (!has_arrow())
	{
		gtk_widget_hide(mSpacer);
		return;
	}

	gtk_widget_show(mSpacer);

	gtk_widget_realize(window);

	GtkRequisition req;
	gtk_widget_size_request(window, &req);

	int new_height = get_height() + ARROW_HEIGHT;

	/* TODO: Be smarter about the location of the arrow. */
	mArrowPoints[0].x = 0;
	mArrowPoints[0].y = ARROW_HEIGHT;

	mArrowPoints[1].x = ARROW_OFFSET;
	mArrowPoints[1].y = ARROW_HEIGHT;

	mArrowPoints[2].x = ARROW_OFFSET + ARROW_WIDTH / 2;
	mArrowPoints[2].y = 0;

	mArrowPoints[3].x = ARROW_OFFSET + ARROW_WIDTH;
	mArrowPoints[3].y = ARROW_HEIGHT;

	mArrowPoints[4].x = req.width;
	mArrowPoints[4].y = ARROW_HEIGHT;

	mArrowPoints[5].x = req.width;
	mArrowPoints[5].y = new_height;

	mArrowPoints[6].x = 0;
	mArrowPoints[6].y = new_height;

	GdkRegion *region = gdk_region_polygon(mArrowPoints,
										   G_N_ELEMENTS(mArrowPoints),
										   GDK_EVEN_ODD_RULE);

	gdk_window_shape_combine_region(window->window, region, 0, 0);

	gdk_region_destroy(region);

	mArrowPoints[4].x = req.width - 1;
	mArrowPoints[5].x = req.width - 1;
	mArrowPoints[5].y = new_height - 1;
	mArrowPoints[6].y = new_height - 1;

	ret_arrow_x = ARROW_OFFSET + ARROW_WIDTH / 2;
	ret_arrow_y = 0;
}

PopupNotification::~PopupNotification()
{
    TRACE("destroying notification %d, window=%p\n", GetId(), window);

    if (gc != NULL)
		g_object_unref(gc);

    if (window)
    {
        gtk_widget_hide(window);
        gtk_widget_destroy(window);
        window = NULL;
    }
}

void
PopupNotification::generate()
{
    TRACE("Generating PopupNotification GUI for nid %d\n", GetId());

    GtkWidget *win = window, *bodybox_widget, *hbox, *vbox, *summary_label;
    GtkWidget *body_label = NULL, *image_widget;

    try
    {
        if (!window)
        {
            /* win will be assigned to window at the end */
            win = gtk_window_new(GTK_WINDOW_POPUP);

            gtk_widget_add_events(win, GDK_BUTTON_RELEASE_MASK);

            g_signal_connect(win, "button-release-event", G_CALLBACK(_window_button_release), this);
        }

		bodybox_widget = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(bodybox_widget);
		gtk_container_add(GTK_CONTAINER(win), bodybox_widget);

		mSpacer = gtk_image_new();
		gtk_box_pack_start(GTK_BOX(bodybox_widget), mSpacer, FALSE, FALSE, 0);
		gtk_widget_set_size_request(mSpacer, -1, ARROW_HEIGHT);

        hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(bodybox_widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show(hbox);

		const ImageList &images = GetImages();

        if (images.empty())
        {
            /* let's default to a nice generic bling! image :-)  */
            image_widget = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
        }
        else
        {
            Image *image = images[0];

			switch (image->GetType())
			{
				case Image::THEME:
				{
					TRACE("new from icon theme: %s\n",
						  image->GetFile().c_str());

					GtkIconTheme *theme = gtk_icon_theme_new();
					GError *error = NULL;
					GdkPixbuf *icon =
						gtk_icon_theme_load_icon(theme,
												 image->GetFile().c_str(),
												 IMAGE_SIZE,
												 GTK_ICON_LOOKUP_USE_BUILTIN,
												 &error);
					g_object_unref(G_OBJECT(theme));

					if (error) throw std::runtime_error(S("could not load icon: ") +
                                                        S(error->message));

                    image_widget = gtk_image_new_from_pixbuf(icon);
                    gdk_pixbuf_unref(icon);

					break;
				}

				case Image::ABSOLUTE:
				{
					TRACE("new from file: %s\n", image->GetFile().c_str());

					GError *error = NULL;
					GdkPixbuf *buf;

					buf = gdk_pixbuf_new_from_file(image->GetFile().c_str(),
												   &error);

					if (error)
					{
						throw std::runtime_error(S("could not load file: ") +
												 S(error->message));
					}

					image_widget = gtk_image_new_from_pixbuf(buf);
					break;
				}

				case Image::RAW:
				{
					unsigned char *data;
					size_t dataLen;

					image->GetData(&data, &dataLen);

					TRACE("new from raw: %c%c%c%c\n",
						  data[0], data[1], data[2], data[3]);

					GError *error = NULL;

					GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
					if (error) throw std::runtime_error(S(error->message));

					gdk_pixbuf_loader_write(loader, data, dataLen, &error);
					if (error)
					{
						g_object_unref(loader);
						throw std::runtime_error(S(error->message));
					}

					GdkPixbuf *buf = gdk_pixbuf_loader_get_pixbuf(loader);
					if (!buf) throw std::runtime_error("Could not get pixbuf from loader");

                    g_object_ref(G_OBJECT(buf));

					gdk_pixbuf_loader_close(loader, NULL);
					g_object_unref(loader);

					image_widget = gtk_image_new_from_pixbuf(buf);

					g_object_unref(G_OBJECT(buf));
					break;
				}

				default:
					std::ostringstream s;
					s << "unhandled image type " << image->GetType();
					throw std::runtime_error( s.str() );
					break;
            }
        }

        vbox = gtk_vbox_new(FALSE, 2);
        gtk_widget_show(vbox);

        /* now we want another hbox to provide some padding on the left, between
           the coloured part and the white part. in future it could also do a gradient
           or blend but for now we'll use a lame hack. there must be a better way to
           add padding to one side of a box in GTK.    */

        GtkWidget *padding = gtk_hbox_new(FALSE, 0);
        GtkWidget *padding_label = gtk_label_new("  ");
        gtk_widget_show(padding_label);
        gtk_box_pack_start_defaults(GTK_BOX(padding), padding_label);
        gtk_box_pack_end_defaults(GTK_BOX(padding), vbox);
        gtk_widget_show(padding);

        /* now set up the labels containing the notification text */
        summary_label = gtk_label_new(GetSummary().c_str());
        format_summary(GTK_LABEL(summary_label));
        gtk_misc_set_alignment(GTK_MISC(summary_label), 0, 0.5);
        gtk_widget_show(summary_label);
        gtk_box_pack_start(GTK_BOX(vbox), summary_label, TRUE, TRUE, 5);

        if (GetBody().c_str())
        {
            body_label = sexy_url_label_new();
			sexy_url_label_set_markup(SEXY_URL_LABEL(body_label),
									  GetBody().c_str());

            //process_body_markup(body_label);

            gtk_label_set_line_wrap(GTK_LABEL(body_label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(body_label), 0, 0.5);
            gtk_widget_show(body_label);
            gtk_box_pack_start(GTK_BOX(vbox), body_label, TRUE, TRUE, 10);
        }

        /* we want to fix the width so the notifications expand upwards but not outwards.
           firstly, we need to grab the natural size request of the containing box, then we
           need to set the size request of the label to that width so it will always line wrap. */

        gtk_widget_set_size_request(
			GetBody() != "" ? body_label : summary_label,
			WIDTH - (IMAGE_SIZE + IMAGE_PADDING) - 10 /* FIXME */, -1);

        summary_label = body_label = NULL;

		const ActionsMap &actions = GetActions();

        if (!actions.empty())
        {
            /*
             * now let's do the actions. we'll show them as hyperlinks to
             * save space, and because it looks cooler that way :)
             */
            GtkWidget *actions_hbox = gtk_hbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(vbox), actions_hbox, FALSE, FALSE, 5);
            gtk_widget_show(actions_hbox);

			for (ActionsMap::const_iterator i = actions.begin();
				 i != actions.end();
				 i++)
			{
                TRACE("action %d is %s\n", i->first, i->second.c_str());

                if (i->first == 0) continue;     /* skip the default action */

                GtkWidget *eventbox = gtk_event_box_new();
                gtk_box_pack_start(GTK_BOX(actions_hbox), eventbox, FALSE, FALSE, 0);

                GtkWidget *label = gtk_label_new(i->second.c_str());
                linkify(GTK_LABEL(label));
                whiten(eventbox);
                gtk_container_add(GTK_CONTAINER(eventbox), label);

                g_signal_connect(G_OBJECT(eventbox), "button-release-event",
                                 G_CALLBACK(_action_label_click), (void *) &i->first);

                g_object_set_data(G_OBJECT(eventbox), "notification-instance", this);

                gtk_widget_show(label);

                /* this will set the mouse cursor to be a hand on the hyperlinks  */
                struct expose_data *data = new struct expose_data;
                data->widget = eventbox;
                data->handler_id = g_signal_connect(G_OBJECT(eventbox), "expose-event",
                                                    G_CALLBACK(_set_cursor), (void *) data);

                /* if it's not the last item ...  */
                if (i->second != GetAction(actions.size() - 1))
                {
                    label = gtk_label_new(" | ");    /* ... add a separator */
                    gtk_box_pack_start(GTK_BOX(actions_hbox), label, FALSE, FALSE, 0);
                    gtk_widget_show(label);
                }

                gtk_widget_show(eventbox);
            }
        }

        /*
         * set up an eventbox so we can get a white background. for some
         * reason GTK insists that it be given a new X window if you want
         * to change the background colour. it will just silently ignore
         * requests to change the colours of eg, a box.
         */

        GtkWidget *eventbox = gtk_event_box_new();
        whiten(eventbox);
        gtk_container_set_border_width(GTK_CONTAINER(eventbox), 1);   /* don't overdraw the black border */
        gtk_widget_show(eventbox);

        gtk_container_add(GTK_CONTAINER(eventbox), padding);
        gtk_box_pack_end_defaults(GTK_BOX(hbox), eventbox);

        GtkWidget *imagebox = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(imagebox),
                                       IMAGE_PADDING);

        if (image_widget)
        {
            gtk_widget_show(image_widget);
            gtk_box_pack_start_defaults(GTK_BOX(imagebox), image_widget);
            gtk_misc_set_alignment(GTK_MISC(image_widget), 0.5, 0.0);
            image_widget = NULL;
        }
        gtk_widget_show(imagebox);
        gtk_box_pack_start(GTK_BOX(hbox), imagebox, FALSE, FALSE, 0);

        /* now we setup an expose event handler to draw the border */
        g_signal_connect(win, "expose-event", G_CALLBACK(draw_border), this);
        gtk_widget_set_app_paintable(win, TRUE);

        gtk_widget_show(hbox);

        /* now we want to ensure the height of the content is never less than MIN_HEIGHT */
        GtkRequisition req;
        gtk_widget_size_request(hbox, &req);
        gtk_widget_set_size_request(hbox, -1, MAX(MIN_HEIGHT, req.height));
    }
    catch (...)
    {
        if (win) gtk_widget_destroy(win);   /* don't leak if an exception is thrown */
        throw;
    }

    /* now we have successfully built the UI, commit it to the instance  */
    this->body_box = bodybox_widget;
    this->window = win;
    TRACE("window is %p\n", this->window);

    update_position();
}

void
PopupNotification::show()
{
    if (!window) generate();    // can throw
    gtk_widget_show(window);
}

/*
 * returns the natural height of the notification. generates the gui if
 * not done so already
 */
int
PopupNotification::get_height()
{
    if (!window) generate();    // can throw

    GtkRequisition req;
    gtk_widget_size_request(window, &req);
    return req.height;
}

void
PopupNotification::update_position()
{
    if (!window) generate();    // can throw

    GtkRequisition req;
    gtk_widget_size_request(window, &req);

    GdkRectangle workarea;

    if (!get_work_area(workarea))
    {
        workarea.width  = gdk_screen_width();
        workarea.height = gdk_screen_height();
    }

	int x, y;

	/*
	 * See if the caller has specified where the want the notification to
	 * point to.
	 */
	if (has_arrow())
	{
		GdkDisplay *display = gtk_widget_get_display(window);
		GdkScreen *screen   = gdk_display_get_screen(display, disp_screen);
		int screen_width    = gdk_screen_get_width(screen);
		int screen_height   = gdk_screen_get_height(screen);
		int new_height = get_height() + ARROW_HEIGHT;

		int arrow_x, arrow_y;
		generate_arrow(arrow_x, arrow_y);

		/*
		 * TODO: Maybe try to make the notification stay in the workarea,
		 *       and just extend the arrow? Dunno.
		 */
		int hint_x = atoi(GetHint("x").c_str());
		int hint_y = atoi(GetHint("y").c_str());

		x = CLAMP(hint_x - arrow_x, 0, screen_width  - req.width);
		y = CLAMP(hint_y - arrow_y, 0, screen_height - new_height);
	}
	else
	{
		x = workarea.x + workarea.width - req.width;
		y = workarea.y + workarea.height - get_height() - height_offset;
	}

    gtk_window_move(GTK_WINDOW(window), x, y);
}

bool
PopupNotification::get_work_area(GdkRectangle &rect)
{
#ifndef _WIN32
    Atom workarea = XInternAtom(GDK_DISPLAY(), "_NET_WORKAREA", True);

    if (workarea == None)
        return false;

    Window win = XRootWindow(GDK_DISPLAY(), disp_screen);

    Atom type;
    int format;
    unsigned long num, leftovers;
    unsigned long max_len = 4 * 32;
    unsigned char *ret_workarea;
    int result = XGetWindowProperty(GDK_DISPLAY(), win, workarea, 0,
                                     max_len, False, AnyPropertyType,
                                     &type, &format, &num,
                                     &leftovers, &ret_workarea);

    if (result != Success || type == None || format == 0 ||
        leftovers || num % 4)
    {
        return false;
    }

    guint32 *workareas = (guint32 *)ret_workarea;

    rect.x      = workareas[disp_screen * 4];
    rect.y      = workareas[disp_screen * 4 + 1];
    rect.width  = workareas[disp_screen * 4 + 2];
    rect.height = workareas[disp_screen * 4 + 3];

    XFree(ret_workarea);

    return true;
#else /* _WIN32 */
    return false;
#endif /* _WIN32 */
}

void
PopupNotification::set_height_offset(int value)
{
    height_offset = value;
    update_position();     // can throw
}

void
PopupNotification::update()
{
    /* contents have changed, so scrap current UI and regenerate */
    TRACE("updating for %d\n", GetId());

    if (window)
    {
        gtk_container_remove(GTK_CONTAINER(window), body_box);
        body_box = NULL;
    }

    generate();    // can throw
}

