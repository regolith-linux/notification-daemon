#include <gtk/gtk.h>
#include <libsexy/sexy-url-label.h>

typedef void (*ActionInvokedCb)(GtkWindow *nw, const char *key);
typedef void (*UrlClickedCb)(GtkWindow *nw, const char *url);

typedef struct
{
	GtkWidget *spacer;
	GtkWidget *iconbox;
	GtkWidget *icon;
	GtkWidget *contentbox;
	GtkWidget *summary_label;
	GtkWidget *body_label;
	GtkWidget *actions_box;
	GtkWidget *last_sep;

	guint num_actions_added;

	gboolean has_arrow;
	int point_x;
	int point_y;
	GdkGC *gc;
	GdkPoint arrow_points[7];
	GdkRegion *window_region;

	guchar urgency;

	UrlClickedCb url_clicked;

} WindowData;

#define WIDTH         300
#define MIN_HEIGHT    100
#define IMAGE_SIZE    48
#define IMAGE_PADDING 10
#define ARROW_OFFSET  20
#define ARROW_HEIGHT  20
#define ARROW_WIDTH   20

static gboolean
draw_border(GtkWidget *win, GdkEventExpose *event, WindowData *windata)
{
	int w, h;

	if (windata->gc == NULL)
	{
		GdkColor color;

		windata->gc = gdk_gc_new(win->window);
		gdk_color_parse("black", &color);
		gdk_gc_set_rgb_fg_color(windata->gc, &color);
	}

	gdk_drawable_get_size(win->window, &w, &h);

	if (windata->has_arrow)
	{
		gdk_draw_polygon(win->window, windata->gc, FALSE,
						 windata->arrow_points,
						 G_N_ELEMENTS(windata->arrow_points));

		/* HACK! */
		gdk_draw_line(win->window, windata->gc,
					  ARROW_OFFSET + 1, ARROW_HEIGHT,
					  ARROW_OFFSET + ARROW_WIDTH / 2 + 1, 0);
		gdk_draw_line(win->window, windata->gc,
					  ARROW_OFFSET + ARROW_WIDTH / 2 - 1, 0,
					  ARROW_OFFSET + ARROW_WIDTH - 1, ARROW_HEIGHT);
		gdk_draw_line(win->window, windata->gc, 0, h - 1, w - 1, h - 1);
		gdk_window_shape_combine_region(win->window, windata->window_region,
										0, 0);
	}
	else
	{
		gdk_draw_rectangle(win->window, windata->gc, FALSE,
						   0, 0, w - 1, h - 1);
	}

	return FALSE;
}

GtkWindow *
create_notification(UrlClickedCb url_clicked)
{
	GtkWidget *win;
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GdkColor color;
	GtkRequisition req;
	WindowData *windata;

	windata = g_new0(WindowData, 1);
	windata->url_clicked = url_clicked;

	win = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_add_events(win, GDK_BUTTON_RELEASE_MASK);
	g_object_set_data(G_OBJECT(win), "windata", windata);
	gtk_widget_set_app_paintable(win, TRUE);

	g_signal_connect(G_OBJECT(win), "expose-event",
					 G_CALLBACK(draw_border), windata);

	// TODO: Connect to button-release-event

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(win), main_vbox);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 1);

	windata->spacer = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), windata->spacer, FALSE, FALSE, 0);
	gtk_widget_set_size_request(windata->spacer, -1, ARROW_HEIGHT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

	windata->iconbox = gtk_event_box_new();
	gtk_widget_show(windata->iconbox);
	gtk_box_pack_start(GTK_BOX(hbox), windata->iconbox, FALSE, TRUE, 0);

	windata->icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
											 GTK_ICON_SIZE_DIALOG);
	gtk_widget_show(windata->icon);
	gtk_container_add(GTK_CONTAINER(windata->iconbox), windata->icon);
	gtk_misc_set_alignment(GTK_MISC(windata->icon), 0.5, 0.0);
	gtk_container_set_border_width(GTK_CONTAINER(windata->iconbox), 12);

	/* TODO: Make this like a view::BaseBGBox */
	windata->contentbox = gtk_event_box_new();
	gtk_widget_show(windata->contentbox);
	gtk_box_pack_start(GTK_BOX(hbox), windata->contentbox, TRUE, TRUE, 0);
	gdk_color_parse("white", &color);
	gtk_widget_modify_bg(windata->contentbox, GTK_STATE_NORMAL, &color);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(windata->contentbox), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

	windata->summary_label = gtk_label_new(NULL);
	gtk_widget_show(windata->summary_label);
	gtk_box_pack_start(GTK_BOX(vbox), windata->summary_label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(windata->summary_label), 0, 0);

	/* TODO: Use a SexyUrlLabel, and attach to url_activated. */
	windata->body_label = sexy_url_label_new();
	gtk_widget_show(windata->body_label);
	gtk_box_pack_start(GTK_BOX(vbox), windata->body_label, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(windata->body_label), 0, 0);
	gtk_label_set_line_wrap(GTK_LABEL(windata->body_label), TRUE);
	g_signal_connect_swapped(G_OBJECT(windata->body_label), "url_activated",
							 G_CALLBACK(windata->url_clicked), win);

	windata->actions_box = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(windata->actions_box);
	gtk_box_pack_start(GTK_BOX(vbox), windata->actions_box, FALSE, TRUE, 0);

	gtk_widget_size_request(hbox, &req);
	gtk_widget_set_size_request(hbox, -1, MAX(MIN_HEIGHT, req.height));

	return GTK_WINDOW(win);
}

void
destroy_notification(GtkWindow *nw)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	if (windata->gc != NULL)
		g_object_unref(G_OBJECT(windata->gc));

	if (windata->window_region != NULL)
		gdk_region_destroy(windata->window_region);

	gtk_widget_destroy(GTK_WIDGET(nw));
}

void
show_notification(GtkWindow *nw)
{
	gtk_widget_show(GTK_WIDGET(nw));
}

void
hide_notification(GtkWindow *nw)
{
	gtk_widget_hide(GTK_WIDGET(nw));
}

void
set_notification_hints(GtkWindow *nw, GHashTable *hints)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	GValue *value;

	g_assert(windata != NULL);

	value = (GValue *)g_hash_table_lookup(hints, "urgency");

	if (value)
		windata->urgency = g_value_get_uchar(value);
}

void
set_notification_text(GtkWindow *nw, const char *summary, const char *body)
{
	char *str;
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	str = g_strdup_printf("<b><big>%s</big></b>", summary);
	gtk_label_set_markup(GTK_LABEL(windata->summary_label), str);
	g_free(str);

	sexy_url_label_set_markup(SEXY_URL_LABEL(windata->body_label), body);

	gtk_widget_set_size_request(
		((body != NULL && *body == '\0')
		 ? windata->body_label : windata->summary_label),
		WIDTH - (IMAGE_SIZE + IMAGE_PADDING) - 10,
		-1);
}

void
set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	gtk_image_set_from_pixbuf(GTK_IMAGE(windata->icon), pixbuf);
}

void
set_notification_arrow(GtkWindow *nw, gboolean visible, int x, int y)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	windata->has_arrow = visible;
	windata->point_x   = x;
	windata->point_y   = y;

	if (visible)
		gtk_widget_show(windata->spacer);
	else
		gtk_widget_hide(windata->spacer);
}

static void
action_clicked_cb(GtkWidget *evbox, GdkEventButton *event,
				  ActionInvokedCb action_cb)
{
	GtkWindow *nw   = g_object_get_data(G_OBJECT(evbox), "_nw");
	const char *key = g_object_get_data(G_OBJECT(evbox), "_action_key");

	action_cb(nw, key);
}

void
add_notification_action(GtkWindow *nw, const char *text, const char *key,
						ActionInvokedCb cb)
{
	/*
	 * TODO: Use SexyUrlLabel. This requires a way of disabling the
	 *       right-click menu.
	 */
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	GtkWidget *evbox;
	GtkWidget *label;
	GdkColor color;
	GdkCursor *cursor;
	char *buf;

	g_assert(windata != NULL);

	if (windata->num_actions_added > 0)
	{
		label = gtk_label_new("‧");
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(windata->actions_box), label,
						    FALSE, FALSE, 0);
	}

	evbox = gtk_event_box_new();
	gtk_widget_show(evbox);
	gtk_box_pack_start(GTK_BOX(windata->actions_box), evbox, FALSE, FALSE, 0);
	gtk_widget_realize(evbox);
	gdk_color_parse("white", &color);
	gtk_widget_modify_bg(evbox, GTK_STATE_NORMAL, &color);

	g_object_set_data(G_OBJECT(evbox), "_nw", nw);
	g_object_set_data_full(G_OBJECT(evbox),
						   "_action_key", g_strdup(key), g_free);
	g_signal_connect(G_OBJECT(evbox), "button-release-event",
					 G_CALLBACK(action_clicked_cb), cb);

	cursor = gdk_cursor_new_for_display(gtk_widget_get_display(evbox),
										GDK_HAND2);
	gdk_window_set_cursor(evbox->window, cursor);
	gdk_cursor_unref(cursor);

	label = gtk_label_new(NULL);
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(evbox), label);
	buf = g_strdup_printf("<span color=\"blue\""
						  " underline=\"single\">%s</span>", text);
	gtk_label_set_markup(GTK_LABEL(label), buf);
	g_free(buf);

	windata->num_actions_added++;
}

static void
generate_arrow(GtkWidget *nw, WindowData *windata, int *arrow_x, int *arrow_y)
{
	GtkRequisition req;
	int new_height;

	gtk_widget_realize(nw);
	gtk_widget_size_request(nw, &req);

	new_height = req.height + ARROW_HEIGHT;

	windata->arrow_points[0].x = 0;
	windata->arrow_points[0].y = ARROW_HEIGHT;

	windata->arrow_points[1].x = ARROW_OFFSET;
	windata->arrow_points[1].y = ARROW_HEIGHT;

	windata->arrow_points[2].x = ARROW_OFFSET + ARROW_WIDTH / 2;
	windata->arrow_points[2].y = 0;

	windata->arrow_points[3].x = ARROW_OFFSET + ARROW_WIDTH;
	windata->arrow_points[3].y = ARROW_HEIGHT;

	windata->arrow_points[4].x = req.width;
	windata->arrow_points[4].y = ARROW_HEIGHT;

	windata->arrow_points[5].x = req.width;
	windata->arrow_points[5].y = new_height;

	windata->arrow_points[6].x = 0;
	windata->arrow_points[6].y = new_height;

	windata->window_region =
		gdk_region_polygon(windata->arrow_points,
						   G_N_ELEMENTS(windata->arrow_points),
						   GDK_EVEN_ODD_RULE);

	windata->arrow_points[4].x--;
	windata->arrow_points[5].x--;
	windata->arrow_points[5].y--;
	windata->arrow_points[6].y--;

	*arrow_x = ARROW_OFFSET + ARROW_WIDTH / 2;
	*arrow_y = 0;

	draw_border(nw, NULL, windata);
}

void
move_notification(GtkWindow *nw, int x, int y)
{
	WindowData *windata = g_object_get_data(G_OBJECT(nw), "windata");
	g_assert(windata != NULL);

	if (windata->has_arrow)
	{
		GtkRequisition req;
		//GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(nw));
		//GdkScreen *screen   = gdk_display_get_screen(display, disp_screen);
		//int screen_width    = gdk_screen_get_width(screen);
		//int screen_height   = gdk_screen_get_height(screen);
		int new_height;
		int arrow_x;
		int arrow_y;

		gtk_widget_size_request(GTK_WIDGET(nw), &req);
		new_height = req.height + ARROW_HEIGHT;
		generate_arrow(GTK_WIDGET(nw), windata, &arrow_x, &arrow_y);
#if 0
		x = CLAMP(windata->point_x, 0, screen_width  - req.width);
		y = CLAMP(windata->point_y, 0, screen_height - new_height);
#endif
	}
	else
	{
#if 0
		x = workarea.x + workarea.width - req.width;
		y = workarea.y + workarea.height - get_height() - height_offset;
#endif
	}

	gtk_window_move(GTK_WINDOW(nw), x, y);
}
