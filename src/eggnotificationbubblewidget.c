/* EggNotificationBubbleWidget
 * Copyright (C) 2005 Colin Walters <walters@verbum.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include "eggnotificationbubblewidget.h"

#define BORDER_SIZE 15

static void egg_notification_bubble_widget_class_init        (EggNotificationBubbleWidgetClass *klass);
static void egg_notification_bubble_widget_init              (EggNotificationBubbleWidget      *bubble_widget);
static void egg_notification_bubble_widget_finalize          (GObject        *object);
static void egg_notification_bubble_widget_event_handler     (GtkWidget   *widget,
                                                              GdkEvent    *event,
                                                              gpointer     user_data);
static gboolean egg_notification_bubble_widget_expose        (GtkWidget *widget, GdkEventExpose *event);

static void populate_window (EggNotificationBubbleWidget *bubble_widget);
static void draw_bubble_widget (EggNotificationBubbleWidget *bubble_widget);

static GtkWindowClass *parent_class;

#define BEVEL_ALPHA_LIGHT  0.2
#define BEVEL_ALPHA_MEDIUM 0.5
#define BEVEL_ALPHA_DARK   0.8 


enum 
{
    ORIENT_TOP = 0,
    ORIENT_BOTTOM = 1
}; 

enum {
    TRIANGLE_LEFT = 0,
    TRIANGLE_RIGHT = 1
}; 

enum
{
	NOTIFICATION_CLICKED,
	LAST_SIGNAL
};

static guint egg_notification_bubble_widget_signals[LAST_SIGNAL] = { 0 };

GType
egg_notification_bubble_widget_get_type (void)
{
  static GType bubble_widget_type = 0;

  if (!bubble_widget_type)
    {
      static const GTypeInfo bubble_widget_info =
      {
	sizeof (EggNotificationBubbleWidgetClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) egg_notification_bubble_widget_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EggNotificationBubbleWidget),
	0,		/* n_preallocs */
	(GInstanceInitFunc) egg_notification_bubble_widget_init,
      };

      bubble_widget_type = g_type_register_static (GTK_TYPE_WINDOW, "EggNotificationBubbleWidget",
					      &bubble_widget_info, 0);
    }

  return bubble_widget_type;
}

static void
egg_notification_bubble_widget_class_init (EggNotificationBubbleWidgetClass *class)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = g_type_class_peek_parent (class);

  object_class->finalize = egg_notification_bubble_widget_finalize;
  widget_class->expose_event = egg_notification_bubble_widget_expose;

  egg_notification_bubble_widget_signals[NOTIFICATION_CLICKED] =
    g_signal_new ("clicked",
		  EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (EggNotificationBubbleWidgetClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);
}

static void
egg_notification_bubble_widget_init (EggNotificationBubbleWidget *bubble_widget)
{
  GtkWindow *win;

  win = GTK_WINDOW (bubble_widget);

  populate_window (bubble_widget);

}

static void
egg_notification_bubble_widget_finalize (GObject *object)
{
  EggNotificationBubbleWidget *bubble_widget = EGG_NOTIFICATION_BUBBLE_WIDGET (object);

  g_return_if_fail (bubble_widget != NULL);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
populate_window (EggNotificationBubbleWidget *bubble_widget)
{
  g_return_if_fail (EGG_IS_NOTIFICATION_BUBBLE_WIDGET (bubble_widget));

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *widget;

  widget = GTK_WIDGET (bubble_widget);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_app_paintable (widget, TRUE);
  gtk_window_set_resizable (GTK_WINDOW (bubble_widget), FALSE);
  gtk_widget_set_name (widget, "gtk-tooltips");
  gtk_container_set_border_width (GTK_CONTAINER (bubble_widget), BORDER_SIZE + 5);

  bubble_widget->bubble_widget_header_label = gtk_label_new (NULL);
  bubble_widget->bubble_widget_body_label = gtk_label_new (NULL);
  bubble_widget->icon = gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_BUTTON);
  gtk_label_set_line_wrap (GTK_LABEL (bubble_widget->bubble_widget_header_label), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (bubble_widget->bubble_widget_body_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (bubble_widget->bubble_widget_header_label), 0.0, 0.0);
  gtk_misc_set_alignment (GTK_MISC (bubble_widget->bubble_widget_body_label), 0.0, 0.0);

  gtk_misc_set_alignment (GTK_MISC (bubble_widget->icon), 0.0, 0.0);
  gtk_widget_show (bubble_widget->icon);
  gtk_widget_show (bubble_widget->bubble_widget_header_label);
  gtk_widget_show (bubble_widget->bubble_widget_body_label);

  bubble_widget->main_hbox = gtk_hbox_new (FALSE, 10);
      
  vbox = gtk_vbox_new (FALSE, 5);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), bubble_widget->icon, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), bubble_widget->bubble_widget_header_label);
  gtk_container_add (GTK_CONTAINER (vbox), bubble_widget->bubble_widget_body_label);
  gtk_container_add (GTK_CONTAINER (hbox), vbox);
  gtk_container_add (GTK_CONTAINER (bubble_widget), hbox);

  g_signal_connect_after (bubble_widget, "event-after",
                          G_CALLBACK (egg_notification_bubble_widget_event_handler),
                          bubble_widget);
   
}

static void
_destroy_pixmap_data_func (guchar *pixels,
                           gpointer data)
{
  g_free (pixels);
}

void 
egg_notification_bubble_widget_set_icon_from_data (EggNotificationBubbleWidget *bubble_widget,
                                                   const guchar *data,
                                                   gboolean has_alpha,
                                                   int bits_per_sample,
                                                   int width,
                                                   int height,
                                                   int rowstride)
{
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_data (data,
                                     GDK_COLORSPACE_RGB,
                                     has_alpha,
                                     bits_per_sample,
                                     width,
                                     height,
                                     rowstride,
                                     _destroy_pixmap_data_func,
                                     NULL);

  gtk_image_set_from_pixbuf (GTK_IMAGE (bubble_widget->icon), pixbuf);
  gdk_pixbuf_unref (pixbuf);
}

void
egg_notification_bubble_widget_set (EggNotificationBubbleWidget *bubble_widget,
			     const gchar *bubble_widget_header_text,
                             const gchar *icon,
			     const gchar *bubble_widget_body_text)
{
  gchar *markupquoted;
  gchar *markuptext;
  gchar *paddedtext;

  g_return_if_fail (EGG_IS_NOTIFICATION_BUBBLE_WIDGET (bubble_widget));

  g_free (bubble_widget->bubble_widget_header_text);
  g_free (bubble_widget->bubble_widget_body_text);
  bubble_widget->bubble_widget_header_text = g_strdup (bubble_widget_header_text);
  bubble_widget->bubble_widget_body_text = g_strdup (bubble_widget_body_text);

  if (icon != NULL || strcmp (icon, "") != 0)
    {
      if (g_str_has_prefix (icon, "file://"))
        {
          gchar *icon_path = (gchar *) icon + (7 * sizeof (gchar));
          gtk_image_set_from_file (GTK_IMAGE (bubble_widget->icon), icon_path);
        } 
      else 
        {
          gtk_image_set_from_icon_name (GTK_IMAGE (bubble_widget->icon), icon, GTK_ICON_SIZE_DIALOG);
        }
    }
    
  markupquoted = g_markup_escape_text (bubble_widget->bubble_widget_header_text, -1);
  markuptext = g_strdup_printf ("<b>%s</b>", markupquoted);
  gtk_label_set_markup (GTK_LABEL (bubble_widget->bubble_widget_header_label), markuptext);
  g_free (markuptext);
  g_free (markupquoted);

  paddedtext = g_strdup_printf ("  %s", bubble_widget->bubble_widget_body_text);
  gtk_label_set_text (GTK_LABEL (bubble_widget->bubble_widget_body_label), paddedtext);
  g_free (paddedtext);
}

void
egg_notification_bubble_widget_set_pos (EggNotificationBubbleWidget   *bubble_widget,
                                 gint x, gint y)
{
  bubble_widget->x = x;
  bubble_widget->y = y;
}

static gboolean 
egg_notification_bubble_widget_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkRequisition req;
  GtkWidgetClass *widget_parent_class;

  gtk_widget_size_request (widget, &req);
  gtk_paint_flat_box (widget->style, widget->window,
		      GTK_STATE_NORMAL, GTK_SHADOW_OUT, 
		      NULL, widget, "notification",
		      0, 0, req.width, req.height);

  draw_bubble_widget (EGG_NOTIFICATION_BUBBLE_WIDGET(widget));

  widget_parent_class = (GtkWidgetClass *)parent_class;
  if (widget_parent_class->expose_event)
    widget_parent_class->expose_event (widget, event);
 
  return FALSE;
}

static void
subtract_rectangle (GdkRegion *region, GdkRectangle *rectangle)
{
  GdkRegion *temp_region;

  temp_region = gdk_region_rectangle (rectangle);
  gdk_region_subtract (region, temp_region);
  gdk_region_destroy (temp_region);
}

static GdkRegion *
add_bevels_to_rectangle (GdkRectangle *rectangle, cairo_t *cairo_context, int orient, int orient_triangle)
{
  GdkRectangle temp_rect;
  GdkRegion *region = gdk_region_rectangle (rectangle);
  
  /* Top left */
  if (!(orient == ORIENT_TOP && orient_triangle == TRIANGLE_LEFT))
    {
      temp_rect.width = 5;
      temp_rect.height = 1;
      temp_rect.x = rectangle->x;
      temp_rect.y = rectangle->y;  
      subtract_rectangle (region, &temp_rect);

      temp_rect.y += 1;
      temp_rect.width -= 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y += 1;
      temp_rect.width -= 1;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y += 1;
      temp_rect.width -= 1;
      temp_rect.height = 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.x = rectangle->x;
      temp_rect.y = rectangle->y;

      cairo_move_to (cairo_context, temp_rect.x + 3, temp_rect.y + 1.5);
      cairo_line_to (cairo_context, temp_rect.x + 5, temp_rect.y + 1.5);
      
      cairo_move_to (cairo_context, temp_rect.x + 2, temp_rect.y + 2.5);
      cairo_line_to (cairo_context, temp_rect.x + 3, temp_rect.y + 2.5);

      cairo_move_to (cairo_context, temp_rect.x + 1, temp_rect.y + 3.5);
      cairo_line_to (cairo_context, temp_rect.x + 2, temp_rect.y + 3.5);

      cairo_move_to (cairo_context, temp_rect.x + 1, temp_rect.y + 4.5);
      cairo_line_to (cairo_context, temp_rect.x + 2, temp_rect.y + 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);
      
      cairo_move_to (cairo_context, temp_rect.x + 3, temp_rect.y + 2.5);
      cairo_line_to (cairo_context, temp_rect.x + 5, temp_rect.y + 2.5);

      cairo_move_to (cairo_context, temp_rect.x + 2, temp_rect.y + 3.5);
      cairo_line_to (cairo_context, temp_rect.x + 3, temp_rect.y + 3.5);
      
      cairo_move_to (cairo_context, temp_rect.x + 2, temp_rect.y + 4.5);
      cairo_line_to (cairo_context, temp_rect.x + 3, temp_rect.y + 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);
    }

  /* Top right */
  if (!(orient == ORIENT_TOP && orient_triangle == TRIANGLE_RIGHT))
    {
      temp_rect.width = 5;
      temp_rect.height = 1;
      
      temp_rect.x = (rectangle->x + rectangle->width) - temp_rect.width;
      temp_rect.y = rectangle->y;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y += 1;
      temp_rect.x += 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y += 1;
      temp_rect.x += 1;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y += 1;
      temp_rect.x += 1;
      temp_rect.height = 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.x = rectangle->x + rectangle->width;
      temp_rect.y = rectangle->y;

      cairo_move_to (cairo_context, temp_rect.x - 3, temp_rect.y + 1.5);
      cairo_line_to (cairo_context, temp_rect.x - 5, temp_rect.y + 1.5);
      
      cairo_move_to (cairo_context, temp_rect.x - 2, temp_rect.y + 2.5);
      cairo_line_to (cairo_context, temp_rect.x - 3, temp_rect.y + 2.5);

      cairo_move_to (cairo_context, temp_rect.x - 1, temp_rect.y + 3.5);
      cairo_line_to (cairo_context, temp_rect.x - 2, temp_rect.y + 3.5);

      cairo_move_to (cairo_context, temp_rect.x - 1, temp_rect.y + 4.5);
      cairo_line_to (cairo_context, temp_rect.x - 2, temp_rect.y + 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);
      
      cairo_move_to (cairo_context, temp_rect.x - 3, temp_rect.y + 2.5);
      cairo_line_to (cairo_context, temp_rect.x - 5, temp_rect.y + 2.5);

      cairo_move_to (cairo_context, temp_rect.x - 2, temp_rect.y + 3.5);
      cairo_line_to (cairo_context, temp_rect.x - 3, temp_rect.y + 3.5);
      
      cairo_move_to (cairo_context, temp_rect.x - 2, temp_rect.y + 4.5);
      cairo_line_to (cairo_context, temp_rect.x - 3, temp_rect.y + 4.5);
     
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

    }

  /* Bottom right */
  if (!(orient == ORIENT_BOTTOM && orient_triangle == TRIANGLE_RIGHT))
    {
      temp_rect.width = 5;
      temp_rect.height = 1;

      temp_rect.x = (rectangle->x + rectangle->width) - temp_rect.width;
      temp_rect.y = (rectangle->y + rectangle->height) - temp_rect.height;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y -= 1;
      temp_rect.x += 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y -= 1;
      temp_rect.x += 1;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y -= 2;
      temp_rect.x += 1;
      temp_rect.height = 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.x = rectangle->x + rectangle->width; 
      temp_rect.y = rectangle->y + rectangle->height;

      cairo_move_to (cairo_context, temp_rect.x - 3, temp_rect.y - 1.5);
      cairo_line_to (cairo_context, temp_rect.x - 5, temp_rect.y - 1.5);
      
      cairo_move_to (cairo_context, temp_rect.x - 2, temp_rect.y - 2.5);
      cairo_line_to (cairo_context, temp_rect.x - 3, temp_rect.y - 2.5);

      cairo_move_to (cairo_context, temp_rect.x - 1, temp_rect.y - 3.5);
      cairo_line_to (cairo_context, temp_rect.x - 2, temp_rect.y - 3.5);

      cairo_move_to (cairo_context, temp_rect.x - 1, temp_rect.y - 4.5);
      cairo_line_to (cairo_context, temp_rect.x - 2, temp_rect.y - 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);
      
      cairo_move_to (cairo_context, temp_rect.x - 3, temp_rect.y - 2.5);
      cairo_line_to (cairo_context, temp_rect.x - 5, temp_rect.y - 2.5);

      cairo_move_to (cairo_context, temp_rect.x - 2, temp_rect.y - 3.5);
      cairo_line_to (cairo_context, temp_rect.x - 3, temp_rect.y - 3.5);
      
      cairo_move_to (cairo_context, temp_rect.x - 2, temp_rect.y - 4.5);
      cairo_line_to (cairo_context, temp_rect.x - 3, temp_rect.y - 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

    }

  /* Bottom left */
  if (!(orient == ORIENT_BOTTOM && orient_triangle == TRIANGLE_LEFT))
    {
      temp_rect.width = 5;
      temp_rect.height = 1;
  
      temp_rect.x = rectangle->x;
      temp_rect.y = rectangle->y + rectangle->height - 1;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y -= 1;
      temp_rect.width -= 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y -= 1;
      temp_rect.width -= 1;
      subtract_rectangle (region, &temp_rect);

      temp_rect.y -= 2;
      temp_rect.width -= 1;
      temp_rect.height = 2;
      subtract_rectangle (region, &temp_rect);

      temp_rect.x = rectangle->x;
      temp_rect.y = rectangle->y + rectangle->height;

      cairo_move_to (cairo_context, temp_rect.x + 3, temp_rect.y - 1.5);
      cairo_line_to (cairo_context, temp_rect.x + 5, temp_rect.y - 1.5);
      
      cairo_move_to (cairo_context, temp_rect.x + 2, temp_rect.y - 2.5);
      cairo_line_to (cairo_context, temp_rect.x + 3, temp_rect.y - 2.5);

      cairo_move_to (cairo_context, temp_rect.x + 1, temp_rect.y - 3.5);
      cairo_line_to (cairo_context, temp_rect.x + 2, temp_rect.y - 3.5);

      cairo_move_to (cairo_context, temp_rect.x + 1, temp_rect.y - 4.5);
      cairo_line_to (cairo_context, temp_rect.x + 2, temp_rect.y - 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);
      
      cairo_move_to (cairo_context, temp_rect.x + 3, temp_rect.y - 2.5);
      cairo_line_to (cairo_context, temp_rect.x + 5, temp_rect.y - 2.5);

      cairo_move_to (cairo_context, temp_rect.x + 2, temp_rect.y - 3.5);
      cairo_line_to (cairo_context, temp_rect.x + 3, temp_rect.y - 3.5);
      
      cairo_move_to (cairo_context, temp_rect.x + 2, temp_rect.y - 4.5);
      cairo_line_to (cairo_context, temp_rect.x + 3, temp_rect.y - 4.5);
      
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);
    }

  return region;
}

static void
draw_bubble_widget (EggNotificationBubbleWidget *bubble_widget)
{
  GtkRequisition requisition;
  GtkStyle *style;
  gint x, y, w, h;
  GdkScreen *screen;
  gint monitor_num;
  GdkRectangle monitor;
  GdkPoint triangle_points[3];
  GdkRectangle rectangle;
  GdkRegion *region;
  GdkRegion *triangle_region;

  int orient;
  int orient_triangle;  
  guint rectangle_border;
  GtkWidget *widget;
  cairo_t *cairo_context;

  widget = GTK_WIDGET(bubble_widget);
  cairo_context = gdk_cairo_create (widget->window);
  
 
  gtk_widget_ensure_style (widget);
  style = widget->style;
 
  gtk_widget_size_request (widget, &requisition);
  w = requisition.width;
  h = requisition.height;

  x = bubble_widget->x;
  y = bubble_widget->y;

  orient = ORIENT_TOP;

  screen = gtk_window_get_screen (GTK_WINDOW(widget));
  monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if ((y + h) > monitor.y + monitor.height)
    {
      y -= (h + 5);
      orient = ORIENT_BOTTOM;
    }
  else
    y = y + 5;

  rectangle_border = BORDER_SIZE-2; 

  rectangle.x = rectangle_border;
  rectangle.y = rectangle_border;
  rectangle.width = w - (rectangle_border * 2);
  rectangle.height = h - (rectangle_border * 2);

  if (x < (monitor.x + monitor.width) / 2)
    {
      orient_triangle = TRIANGLE_LEFT;
      triangle_points[0].x = rectangle.x;
    }
  else
    { 
      orient_triangle = TRIANGLE_RIGHT;
      triangle_points[0].x = rectangle.x + rectangle.width - 10;
    }

  cairo_set_line_width (cairo_context, 1.0);
  region = add_bevels_to_rectangle (&rectangle, cairo_context, orient, orient_triangle);

  triangle_points[0].y = orient == ORIENT_TOP ? rectangle.y : rectangle.y + rectangle.height;
  triangle_points[1].x = triangle_points[0].x + 10;
  triangle_points[1].y = triangle_points[0].y;
  triangle_points[2].y = orient == ORIENT_TOP ? 0 : h;

  if (orient_triangle == TRIANGLE_LEFT)
    triangle_points[2].x = triangle_points[0].x;
  else
    triangle_points[2].x = triangle_points[1].x;

  triangle_region = gdk_region_polygon (triangle_points, 3, GDK_WINDING_RULE);
  gdk_region_union (region, triangle_region);
  gdk_region_destroy (triangle_region);

  gdk_window_shape_combine_region (widget->window, region, 0, 0);

  if (orient == ORIENT_TOP)
    {
      /* top from triangle */
      if (orient_triangle == TRIANGLE_LEFT)
        {
          cairo_move_to (cairo_context, triangle_points[0].x + 0.5, triangle_points[0].y); 
          cairo_line_to (cairo_context, triangle_points[2].x + 0.5, triangle_points[2].y);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
          
          cairo_move_to (cairo_context, triangle_points[2].x - 0.5, triangle_points[2].y + 0.5);
          cairo_line_to (cairo_context, triangle_points[1].x - 0.5, triangle_points[1].y + 0.5);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[0].x + 1.5, triangle_points[0].y + 5); 
          cairo_line_to (cairo_context, triangle_points[2].x + 1.5, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x - 1.5, triangle_points[1].y + 1.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[1].x, triangle_points[1].y + 0.5);
          cairo_line_to (cairo_context, rectangle.x + rectangle.width, rectangle.y + 0.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[1].x + 1, triangle_points[1].y + 1.5);
          cairo_line_to (cairo_context, rectangle.x + rectangle.width - 5, rectangle.y + 1.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);
        }
      
      /* right */
      cairo_move_to (cairo_context, rectangle.x + rectangle.width - 0.5, rectangle.y + 1.5);
      cairo_line_to (cairo_context, rectangle.x + rectangle.width - 0.5 , rectangle.y + rectangle.height);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
      cairo_stroke (cairo_context);
      
      cairo_move_to (cairo_context, rectangle.x + rectangle.width - 1.5, rectangle.y + 4.5);
      cairo_line_to (cairo_context, rectangle.x + rectangle.width - 1.5 , rectangle.y + rectangle.height - 4.5);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);
 
      /* bottom */
      cairo_move_to (cairo_context, rectangle.x + rectangle.width, rectangle.y + rectangle.height - 0.5);
      cairo_line_to (cairo_context, rectangle.x, rectangle.y + rectangle.height - 0.5);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
      cairo_stroke (cairo_context);

      cairo_move_to (cairo_context, rectangle.x + rectangle.width - 5, rectangle.y + rectangle.height - 1.5);
      cairo_line_to (cairo_context, rectangle.x + 5, rectangle.y + rectangle.height - 1.5);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

      /* left */
      cairo_move_to (cairo_context, rectangle.x + 0.5, rectangle.y + rectangle.height);
      cairo_line_to (cairo_context, rectangle.x + 0.5, rectangle.y);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);

      cairo_move_to (cairo_context, rectangle.x + 1.5, rectangle.y + rectangle.height - 5);
      cairo_line_to (cairo_context, rectangle.x + 1.5, rectangle.y + 5);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

      /* top back to triangle */
      if (orient_triangle == TRIANGLE_RIGHT)
        {

          cairo_move_to (cairo_context, triangle_points[0].x - 0.5, triangle_points[0].y + 0.5); 
          cairo_line_to (cairo_context, triangle_points[2].x - 0.5, triangle_points[2].y + 0.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
          cairo_stroke (cairo_context);
          
          cairo_move_to (cairo_context, triangle_points[2].x - 0.5, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x - 0.5, triangle_points[1].y + 2);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[0].x + 1.5, triangle_points[0].y); 
          cairo_line_to (cairo_context, triangle_points[2].x, triangle_points[2].y);
          cairo_move_to (cairo_context, triangle_points[1].x - 1.5, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x - 1.5, triangle_points[1].y + 4.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);
  
          cairo_move_to (cairo_context, rectangle.x, rectangle.y + 0.5);
          cairo_line_to (cairo_context, triangle_points[0].x, triangle_points[0].y + 0.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, rectangle.x + 5, rectangle.y + 1.5);
          cairo_line_to (cairo_context, triangle_points[0].x, triangle_points[0].y + 1.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);

        }


    }
  else
    {
      /* bottom from triangle */
      if (orient_triangle == TRIANGLE_LEFT)
        {
          cairo_move_to (cairo_context, triangle_points[0].x + 0.5, triangle_points[0].y); 
          cairo_line_to (cairo_context, triangle_points[2].x + 0.5, triangle_points[2].y);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
          cairo_stroke (cairo_context);
 
          cairo_move_to (cairo_context, triangle_points[2].x, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x, triangle_points[1].y);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[0].x + 1.5, triangle_points[0].y); 
          cairo_line_to (cairo_context, triangle_points[2].x + 1.5, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x - 1.5, triangle_points[1].y + 1.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[1].x, triangle_points[1].y - 0.5);
          cairo_line_to (cairo_context, rectangle.x + rectangle.width, rectangle.y + rectangle.height - 0.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
          cairo_stroke (cairo_context);
      
          cairo_move_to (cairo_context, triangle_points[1].x, triangle_points[1].y - 1.5);
          cairo_line_to (cairo_context, rectangle.x + rectangle.width, rectangle.y + rectangle.height - 1.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);
        }

      /* top */
      cairo_move_to (cairo_context, rectangle.x, rectangle.y + 0.5);
      cairo_line_to (cairo_context, rectangle.x + rectangle.width, rectangle.y + 0.5);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);

      cairo_move_to (cairo_context, rectangle.x, rectangle.y + 1.5);
      cairo_line_to (cairo_context, rectangle.x + rectangle.width, rectangle.y + 1.5);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

      /* left */
      cairo_move_to (cairo_context, rectangle.x + 0.5, rectangle.y + rectangle.height);
      cairo_line_to (cairo_context, rectangle.x + 0.5, rectangle.y);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_MEDIUM);
      cairo_stroke (cairo_context);

      cairo_move_to (cairo_context, rectangle.x + 1.5, rectangle.y + rectangle.height);
      cairo_line_to (cairo_context, rectangle.x + 1.5, rectangle.y);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

      /* right */
      cairo_move_to (cairo_context, rectangle.x + rectangle.width - 0.5, rectangle.y + 1);
      cairo_line_to (cairo_context, rectangle.x + rectangle.width - 0.5 , rectangle.y + rectangle.height);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
      cairo_stroke (cairo_context);
      
      cairo_move_to (cairo_context, rectangle.x + rectangle.width - 1.5, rectangle.y + 1);
      cairo_line_to (cairo_context, rectangle.x + rectangle.width - 1.5 , rectangle.y + rectangle.height);
      cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
      cairo_stroke (cairo_context);

      /* bottom to triangle */
      if (orient_triangle == TRIANGLE_RIGHT)
        {
          cairo_move_to (cairo_context, triangle_points[0].x + 0.5, triangle_points[0].y); 
          cairo_line_to (cairo_context, triangle_points[2].x + 0.5, triangle_points[2].y);
 
          cairo_move_to (cairo_context, triangle_points[2].x - 0.5, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x - 0.5, triangle_points[1].y);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[0].x + 1.5, triangle_points[0].y); 
          cairo_line_to (cairo_context, triangle_points[2].x - 1.5, triangle_points[2].y);
          cairo_line_to (cairo_context, triangle_points[1].x - 1.5, triangle_points[1].y);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);
 
          cairo_move_to (cairo_context, triangle_points[0].x, triangle_points[0].y  - 0.5);
          cairo_line_to (cairo_context, rectangle.x, rectangle.y + rectangle.height - 0.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_DARK);
          cairo_stroke (cairo_context);

          cairo_move_to (cairo_context, triangle_points[0].x, triangle_points[0].y  - 1.5);
          cairo_line_to (cairo_context, rectangle.x, rectangle.y + rectangle.height - 1.5);
          cairo_set_source_rgba (cairo_context, 0.0, 0.0, 0.0, BEVEL_ALPHA_LIGHT);
          cairo_stroke (cairo_context);
        }

    }

  region = add_bevels_to_rectangle (&rectangle, cairo_context, orient, orient_triangle);
   
  gtk_window_move (GTK_WINDOW (widget), x - triangle_points[2].x, y);
  bubble_widget->active = TRUE;
}

void
egg_notification_bubble_widget_show (EggNotificationBubbleWidget *bubble_widget)
{
  gtk_widget_show_all (GTK_WIDGET (bubble_widget));
}

void
egg_notification_bubble_widget_hide (EggNotificationBubbleWidget *bubble_widget)
{
  if (bubble_widget)
    gtk_widget_hide (GTK_WIDGET (bubble_widget));
}

EggNotificationBubbleWidget*
egg_notification_bubble_widget_new (void)
{
  return g_object_new (EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET, "type", GTK_WINDOW_POPUP, NULL);
}

static void
egg_notification_bubble_widget_event_handler (GtkWidget *widget,
				       GdkEvent  *event,
				       gpointer user_data)
{
  EggNotificationBubbleWidget *bubble_widget;

  bubble_widget = EGG_NOTIFICATION_BUBBLE_WIDGET (user_data);

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      g_signal_emit (bubble_widget, egg_notification_bubble_widget_signals[NOTIFICATION_CLICKED], 0);
      break;
    default:
      break;
    }
}

