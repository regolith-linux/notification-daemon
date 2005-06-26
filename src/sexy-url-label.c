/**
 * @file libsexy/sexy-url-label.c URL Label
 *
 * @Copyright (C) 2005 Christian Hammond
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */
#include "sexy-url-label.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#define SEXY_URL_LABEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), SEXY_TYPE_URL_LABEL, SexyUrlLabelPrivate))

typedef struct
{
	int x1;
	int y1;
	int x2;
	int y2;
	const gchar *url;

} SexyUrlLabelLink;

typedef struct
{
	GList *links;
	GList *urls;
	SexyUrlLabelLink *active_link;

	GtkWidget *popup_menu;

	GdkWindow *event_window;

	GString *temp_markup_result;

} SexyUrlLabelPrivate;

enum
{
	URL_CLICKED,
	LAST_SIGNAL
};

static void sexy_url_label_finalize(GObject *obj);
static void sexy_url_label_realize(GtkWidget *widget);
static void sexy_url_label_unrealize(GtkWidget *widget);
static void sexy_url_label_map(GtkWidget *widget);
static void sexy_url_label_unmap(GtkWidget *widget);
static void sexy_url_label_size_allocate(GtkWidget *widget,
										 GtkAllocation *allocation);
static gboolean sexy_url_label_motion_notify_event(GtkWidget *widget,
												   GdkEventMotion *event);
static gboolean sexy_url_label_leave_notify_event(GtkWidget *widget,
												  GdkEventCrossing *event);
static gboolean sexy_url_label_button_release_event(GtkWidget *widget,
													GdkEventButton *event);

static void open_link_activate_cb(GtkMenuItem *menu_item,
								  SexyUrlLabel *url_label);
static void copy_link_activate_cb(GtkMenuItem *menu_item,
								  SexyUrlLabel *url_label);

static void sexy_url_label_clear_links(SexyUrlLabel *url_label);
static void sexy_url_label_clear_urls(SexyUrlLabel *url_label);
static void sexy_url_label_rescan_label(SexyUrlLabel *url_label);

static GtkLabelClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(SexyUrlLabel, sexy_url_label, GTK_TYPE_LABEL);

static void
sexy_url_label_class_init(SexyUrlLabelClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = sexy_url_label_finalize;

	widget_class->realize              = sexy_url_label_realize;
	widget_class->unrealize            = sexy_url_label_unrealize;
	widget_class->map                  = sexy_url_label_map;
	widget_class->unmap                = sexy_url_label_unmap;
	widget_class->size_allocate        = sexy_url_label_size_allocate;
	widget_class->motion_notify_event  = sexy_url_label_motion_notify_event;
	widget_class->leave_notify_event   = sexy_url_label_leave_notify_event;
	widget_class->button_release_event = sexy_url_label_button_release_event;

	g_type_class_add_private(klass, sizeof(SexyUrlLabelPrivate));

	signals[URL_CLICKED] =
		g_signal_new("url_clicked",
					 G_TYPE_FROM_CLASS(object_class),
					 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					 G_STRUCT_OFFSET(SexyUrlLabelClass, url_clicked),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__STRING,
					 G_TYPE_NONE, 1,
					 G_TYPE_STRING);
}

static void
sexy_url_label_init(SexyUrlLabel *url_label)
{
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);
	GtkWidget *item;
	GtkWidget *image;

	priv->links        = NULL;
	priv->active_link  = NULL;
	priv->event_window = NULL;

	priv->popup_menu = gtk_menu_new();

	item = gtk_image_menu_item_new_with_mnemonic("_Open Link");
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(priv->popup_menu), item);

	g_signal_connect(G_OBJECT(item), "activate",
					 G_CALLBACK(open_link_activate_cb), url_label);

	image = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	gtk_widget_show(image);

	item = gtk_menu_item_new_with_mnemonic("Copy _Link Address");
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(priv->popup_menu), item);

	g_signal_connect(G_OBJECT(item), "activate",
					 G_CALLBACK(copy_link_activate_cb), url_label);
}

static void
sexy_url_label_finalize(GObject *obj)
{
	SexyUrlLabel *url_label = SEXY_URL_LABEL(obj);

	sexy_url_label_clear_links(url_label);
	sexy_url_label_clear_urls(url_label);

	if (G_OBJECT_CLASS(parent_class)->finalize != NULL)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static gboolean
sexy_url_label_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);
	GdkModifierType state;
	gboolean found = FALSE;
	GList *l;
	int x, y;
	SexyUrlLabelLink *link;

	if (event->is_hint)
		gdk_window_get_pointer(event->window, &x, &y, &state);
	else
	{
		x = event->x;
		y = event->y;
		state = event->state;
	}

	for (l = priv->links; l != NULL; l = l->next)
	{
		link = (SexyUrlLabelLink *)l->data;

		if (x >= link->x1 && x <= link->x2 &&
			y >= link->y1 && y <= link->y2)
		{
			found = TRUE;
			break;
		}
	}

	if (found)
	{
		if (priv->active_link == NULL)
		{
			GdkCursor *cursor;

			cursor = gdk_cursor_new_for_display(
				gtk_widget_get_display(widget), GDK_HAND2);
			gdk_window_set_cursor(priv->event_window, cursor);
			gdk_cursor_unref(cursor);

			priv->active_link = link;
		}
	}
	else
	{
		if (priv->active_link != NULL)
		{
			gdk_window_set_cursor(priv->event_window, NULL);
			priv->active_link = NULL;
		}
	}

	return TRUE;
}

static gboolean
sexy_url_label_leave_notify_event(GtkWidget *widget, GdkEventCrossing *event)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (gtk_get_event_widget((GdkEvent *)event) != widget ||
		event->detail == GDK_NOTIFY_INFERIOR)
	{
		return FALSE;
	}

	if (priv->active_link != NULL)
	{
		if (event->x >= priv->active_link->x1 &&
			event->x <= priv->active_link->x2 &&
			event->y >= priv->active_link->y1 &&
			event->y <= priv->active_link->y2)
		{
			return FALSE;
		}
	}

	gdk_window_set_cursor(priv->event_window, NULL);
	priv->active_link = NULL;

	return FALSE;
}

static gboolean
sexy_url_label_button_release_event(GtkWidget *widget, GdkEventButton *event)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (priv->active_link == NULL)
		return TRUE;

	if (event->button == 1)
	{
		g_signal_emit(url_label, signals[URL_CLICKED], 0,
					  priv->active_link->url);
	}
	else if (event->button == 3)
	{
		gtk_menu_popup(GTK_MENU(priv->popup_menu), NULL, NULL, NULL, NULL,
					   event->button, event->time);
	}

	return TRUE;
}

static void
sexy_url_label_realize(GtkWidget *widget)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);
	GdkWindowAttr attributes;
	gint attributes_mask;

	GTK_WIDGET_CLASS(parent_class)->realize(widget);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events(widget);
	attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
							  GDK_BUTTON_RELEASE_MASK |
							  GDK_POINTER_MOTION_MASK |
							  GDK_POINTER_MOTION_HINT_MASK |
							  GDK_LEAVE_NOTIFY_MASK |
							  GDK_LEAVE_NOTIFY_MASK);
	attributes_mask = GDK_WA_X | GDK_WA_Y;

	priv->event_window =
		gdk_window_new(gtk_widget_get_parent_window(widget), &attributes,
					   attributes_mask);
	gdk_window_set_user_data(priv->event_window, widget);
}

static void
sexy_url_label_unrealize(GtkWidget *widget)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (priv->event_window != NULL)
	{
		gdk_window_set_user_data(priv->event_window, NULL);
		gdk_window_destroy(priv->event_window);
		priv->event_window = NULL;
	}

	GTK_WIDGET_CLASS(parent_class)->unrealize(widget);
}

static void
sexy_url_label_map(GtkWidget *widget)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	GTK_WIDGET_CLASS(parent_class)->map(widget);

	if (priv->event_window != NULL)
		gdk_window_show(priv->event_window);
}

static void
sexy_url_label_unmap(GtkWidget *widget)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (priv->event_window != NULL)
		gdk_window_hide(priv->event_window);

	GTK_WIDGET_CLASS(parent_class)->map(widget);
}

static void
sexy_url_label_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	SexyUrlLabel *url_label = (SexyUrlLabel *)widget;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	GTK_WIDGET_CLASS(parent_class)->size_allocate(widget, allocation);

	if (GTK_WIDGET_REALIZED(widget))
	{
		gdk_window_move_resize(priv->event_window,
							   allocation->x, allocation->y,
							   allocation->width, allocation->height);
	}

	sexy_url_label_rescan_label(url_label);
}

static void
open_link_activate_cb(GtkMenuItem *menu_item, SexyUrlLabel *url_label)
{
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (priv->active_link == NULL)
		return;

	g_signal_emit(url_label, signals[URL_CLICKED], 0, priv->active_link->url);
}

static void
copy_link_activate_cb(GtkMenuItem *menu_item, SexyUrlLabel *url_label)
{
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);
	GtkClipboard *clipboard;

	if (priv->active_link == NULL)
		return;

	clipboard = gtk_widget_get_clipboard(GTK_WIDGET(url_label),
										 GDK_SELECTION_PRIMARY);

	gtk_clipboard_set_text(clipboard, priv->active_link->url,
						   strlen(priv->active_link->url));
}

GtkWidget *
sexy_url_label_new(void)
{
	return g_object_new(SEXY_TYPE_URL_LABEL, NULL);
}

static void
sexy_url_label_clear_links(SexyUrlLabel *url_label)
{
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (priv->links == NULL)
		return;

	g_list_foreach(priv->links, (GFunc)g_free, NULL);
	g_list_free(priv->links);
	priv->links = NULL;
}

static void
sexy_url_label_clear_urls(SexyUrlLabel *url_label)
{
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (priv->urls == NULL)
		return;

	g_list_foreach(priv->urls, (GFunc)g_free, NULL);
	g_list_free(priv->urls);
	priv->urls = NULL;
}

static void
sexy_url_label_rescan_label(SexyUrlLabel *url_label)
{
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);
	PangoLayout *layout = gtk_label_get_layout(GTK_LABEL(url_label));
	PangoAttrList *list = pango_layout_get_attributes(layout);
	PangoAttrIterator *iter;
	gint layout_x, layout_y;
	GList *url_list;

	sexy_url_label_clear_links(url_label);

	iter = pango_attr_list_get_iterator(list);

	gtk_label_get_layout_offsets(GTK_LABEL(url_label), &layout_x, &layout_y);

	layout_x -= GTK_WIDGET(url_label)->allocation.x;
	layout_y -= GTK_WIDGET(url_label)->allocation.y;

	url_list = priv->urls;

	do
	{
		PangoAttribute *underline;
		PangoAttribute *color;

		underline = pango_attr_iterator_get(iter, PANGO_ATTR_UNDERLINE);
		color     = pango_attr_iterator_get(iter, PANGO_ATTR_FOREGROUND);

		if (underline != NULL && color != NULL)
		{
			gint start, end;
			PangoRectangle start_pos;
			PangoRectangle end_pos;
			SexyUrlLabelLink *link;

			pango_attr_iterator_range(iter, &start, &end);

			pango_layout_index_to_pos(layout, start, &start_pos);
			pango_layout_index_to_pos(layout, end,   &end_pos);

			link = g_new0(SexyUrlLabelLink, 1);
			link->x1 = layout_x + PANGO_PIXELS(start_pos.x);
			link->y1 = layout_y + PANGO_PIXELS(start_pos.y);
			link->x2 = layout_x +
			           PANGO_PIXELS(end_pos.x) + PANGO_PIXELS(end_pos.width);
			link->y2 = layout_y +
			           PANGO_PIXELS(end_pos.y) + PANGO_PIXELS(end_pos.height);

			link->url = (const gchar *)url_list->data;
			priv->links = g_list_append(priv->links, link);

			url_list = url_list->next;
		}

	} while (pango_attr_iterator_next(iter));
}

static void
start_element_handler(GMarkupParseContext *context,
					  const gchar *element_name,
					  const gchar **attribute_names,
					  const gchar **attribute_values,
					  gpointer user_data,
					  GError **error)
{
	SexyUrlLabel *url_label   = SEXY_URL_LABEL(user_data);
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (!strcmp(element_name, "a"))
	{
		const gchar *url = NULL;
		int line_number;
		int char_number;
		int i;

		g_markup_parse_context_get_position(context, &line_number,
											&char_number);

		for (i = 0; attribute_names[i] != NULL; i++)
		{
			const gchar *attr = attribute_names[i];

			if (!strcmp(attr, "href"))
			{
				if (url != NULL)
				{
					g_set_error(error, G_MARKUP_ERROR,
								G_MARKUP_ERROR_INVALID_CONTENT,
								"Attribute '%s' occurs twice on <a> tag "
								"on line %d char %d, may only occur once",
								attribute_names[i], line_number, char_number);
					return;
				}

				url = attribute_values[i];
			}
			else
			{
				g_set_error(error, G_MARKUP_ERROR,
							G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
							"Attribute '%s' is not allowed on the <a> tag "
							"on line %d char %d",
							attribute_names[i], line_number, char_number);
				return;
			}
		}

		if (url == NULL)
		{
			g_set_error(error, G_MARKUP_ERROR,
						G_MARKUP_ERROR_INVALID_CONTENT,
						"Attribute 'href' was missing on the <a> tag "
						"on line %d char %d",
						line_number, char_number);
			return;
		}

		g_string_append(priv->temp_markup_result,
						"<span color=\"blue\" underline=\"single\">");

		priv->urls = g_list_append(priv->urls, g_strdup(url));
	}
	else
	{
		int i;

		g_string_append_printf(priv->temp_markup_result,
							   "<%s", element_name);

		for (i = 0; attribute_names[i] != NULL; i++)
		{
			const gchar *attr  = attribute_names[i];
			const gchar *value = attribute_values[i];

			g_string_append_printf(priv->temp_markup_result,
								   " %s=\"%s\"",
								   attr, value);
		}

		g_string_append_c(priv->temp_markup_result, '>');
	}
}

static void
end_element_handler(GMarkupParseContext *context,
					const gchar *element_name,
					gpointer user_data,
					GError **error)
{
	SexyUrlLabel *url_label   = SEXY_URL_LABEL(user_data);
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	if (!strcmp(element_name, "a"))
	{
		g_string_append(priv->temp_markup_result, "</span>");
	}
	else
	{
		g_string_append_printf(priv->temp_markup_result,
							   "</%s>", element_name);
	}
}

static void
text_handler(GMarkupParseContext *context,
			 const gchar *text,
			 gsize text_len,
			 gpointer user_data,
			 GError **error)
{
	SexyUrlLabel *url_label   = SEXY_URL_LABEL(user_data);
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);

	g_string_append_len(priv->temp_markup_result, text, text_len);
}

static const GMarkupParser markup_parser =
{
	start_element_handler,
	end_element_handler,
	text_handler,
	NULL,
	NULL
};

static gboolean
xml_isspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static gboolean
parse_custom_markup(SexyUrlLabel *url_label, const gchar *markup,
					gchar **ret_markup)
{
	GMarkupParseContext *context = NULL;
	SexyUrlLabelPrivate *priv = SEXY_URL_LABEL_GET_PRIVATE(url_label);
	GError *error = NULL;
	const gchar *p, *end;
	gboolean needs_root = TRUE;
	gsize length;

	g_return_val_if_fail(markup     != NULL, FALSE);
	g_return_val_if_fail(ret_markup != NULL, FALSE);

	priv->temp_markup_result = g_string_new(NULL);

	length = strlen(markup);
	p = markup;
	end = markup + length;

	while (p != end && xml_isspace(*p))
		p++;

	if (end - p >= 8 && strncmp(p, "<markup>", 8) == 0)
		needs_root = FALSE;

	context = g_markup_parse_context_new(&markup_parser, 0, url_label, NULL);

	if (needs_root)
	{
		if (!g_markup_parse_context_parse(context, "<markup>", -1, &error))
			goto failed;
	}

	if (!g_markup_parse_context_parse(context, markup, strlen(markup), &error))
		goto failed;

	if (needs_root)
	{
		if (!g_markup_parse_context_parse(context, "</markup>", -1, &error))
			goto failed;
	}

	if (!g_markup_parse_context_end_parse(context, &error))
		goto failed;

	if (error != NULL)
		g_error_free(error);

	g_markup_parse_context_free(context);

	*ret_markup = g_string_free(priv->temp_markup_result, FALSE);
	priv->temp_markup_result = NULL;

	return TRUE;

failed:
	fprintf(stderr, "Unable to parse markup: %s\n", error->message);
	g_error_free(error);

	g_string_free(priv->temp_markup_result, TRUE);
	priv->temp_markup_result = NULL;

	g_markup_parse_context_free(context);
	return FALSE;
}

void
sexy_url_label_set_markup(SexyUrlLabel *url_label, const gchar *markup)
{
	gchar *new_markup;

	g_return_if_fail(SEXY_IS_URL_LABEL(url_label));

	sexy_url_label_clear_links(url_label);
	sexy_url_label_clear_urls(url_label);

	if (markup == NULL || *markup == '\0')
	{
		gtk_label_set_markup(GTK_LABEL(url_label), "");
		return;
	}

	if (parse_custom_markup(url_label, markup, &new_markup))
	{
		gtk_label_set_markup(GTK_LABEL(url_label), new_markup);
	}
	else
	{
		gtk_label_set_markup(GTK_LABEL(url_label), "");
	}

	sexy_url_label_rescan_label(url_label);
}
