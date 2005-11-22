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
#include <math.h>

#include <gtk/gtk.h>
#include "eggnotificationbubblewidget.h"

#define BORDER_SIZE 30 
#define CURVE_LENGTH 25
#define TRIANGLE_START 45 
#define TRIANGLE_WIDTH 60 
#define TEXT_WIDTH_THRESHOLD 100 

static void egg_notification_bubble_widget_class_init        (EggNotificationBubbleWidgetClass *klass);
static void egg_notification_bubble_widget_init              (EggNotificationBubbleWidget      *bubble_widget);
static void egg_notification_bubble_widget_finalize          (GObject        *object);
static void egg_notification_bubble_widget_event_handler     (GtkWidget   *widget,
                                                              GdkEvent    *event,
                                                              gpointer     user_data);
static gboolean egg_notification_bubble_widget_expose        (GtkWidget *widget, 
                                                              GdkEventExpose *event);

static gboolean egg_notification_bubble_widget_body_label_expose_handler (GtkWidget *widget, 
                                                                          GdkEventExpose *event, 
                                                                          EggNotificationBubbleWidget *bw);

static void egg_notification_bubble_widget_context_changed_handler (EggNotificationBubbleWidget      *bubble_widget);

static void egg_notification_bubble_widget_realize           (GtkWidget *widget);

static void egg_notification_bubble_widget_screen_changed    (GtkWidget *widget,
                                                              GdkScreen *old_screen);

static void _populate_window (EggNotificationBubbleWidget *bubble_widget);
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
  widget_class->realize = egg_notification_bubble_widget_realize;
  widget_class->screen_changed = egg_notification_bubble_widget_screen_changed;

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
_size_request_handler (GtkWidget *widget, GtkRequisition *requisition, EggNotificationBubbleWidget *bw)
{
  #if 0
  GtkRequisition req;
  int new_height;

  if (!GTK_WIDGET_VISIBLE (widget) ||
      GTK_WIDGET (bw)->allocation.width <= 1)
  return;
  
  gtk_widget_set_size_request (GTK_WIDGET (bw), 0, -1);
  gtk_widget_size_request (GTK_WIDGET (bw), &req);
  g_print ("req width: %d, req height: %d\n", req.width, req.height);

  new_height = GTK_WIDGET (bw)->allocation.height - widget->allocation.height
               + req.height;

  if (GTK_WIDGET (bw)->allocation.width > 0 && new_height > 0 && new_height !=
      widget->allocation.height)
      {

        g_print ("window req width: %d, req height: %d\n", 
                       GTK_WIDGET (bw)->allocation.width,
                       new_height);
    gtk_window_resize (GTK_WINDOW (bw), 
                       GTK_WIDGET (bw)->allocation.width,
                       new_height);
     }
     #endif

#if 0
  if (req.width > MAX_BUBBLE_WIDTH)
    {
      g_print ("-");
    fflush (stdout);
      gtk_widget_set_size_request (GTK_WIDGET (bubble_widget), MAX_BUBBLE_WIDTH, -1);
      gtk_widget_size_request (GTK_WIDGET (bubble_widget), &req);
      gtk_window_resize (GTK_WINDOW (bubble_widget), req.width, req.height);
    }
#endif
}

static void
egg_notification_bubble_widget_init (EggNotificationBubbleWidget *bubble_widget)
{
  GtkWindow *win;
  
  win = GTK_WINDOW (bubble_widget);

  bubble_widget->can_composite = FALSE;

  egg_notification_bubble_widget_screen_changed (GTK_WIDGET (bubble_widget),
                                                 NULL);

 
  _populate_window (bubble_widget);

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
_populate_window (EggNotificationBubbleWidget *bubble_widget)
{
  g_return_if_fail (EGG_IS_NOTIFICATION_BUBBLE_WIDGET (bubble_widget));

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *widget;
  GdkGeometry geom;
 
  widget = GTK_WIDGET (bubble_widget);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_app_paintable (widget, TRUE);
  gtk_window_set_resizable (GTK_WINDOW (bubble_widget), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (bubble_widget), BORDER_SIZE + 5);

  bubble_widget->bubble_widget_header_label = gtk_label_new (NULL);

  //use placeholder so we can use pango/cairo to draw body
  bubble_widget->bubble_widget_body_label = gtk_frame_new("");
  gtk_frame_set_shadow_type (bubble_widget->bubble_widget_body_label, GTK_SHADOW_NONE);
  bubble_widget->icon = gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_BUTTON);
  gtk_widget_ref (bubble_widget->bubble_widget_header_label);
  gtk_widget_ref (bubble_widget->bubble_widget_body_label);
  gtk_widget_ref (bubble_widget->icon);

  gtk_label_set_line_wrap (GTK_LABEL (bubble_widget->bubble_widget_header_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (bubble_widget->bubble_widget_header_label), 0.0, 0.0);

  gtk_misc_set_alignment (GTK_MISC (bubble_widget->icon), 0.0, 0.0);
  gtk_widget_show (bubble_widget->icon);
  gtk_widget_show (bubble_widget->bubble_widget_header_label);
  gtk_widget_show (bubble_widget->bubble_widget_body_label);

  bubble_widget->table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (bubble_widget->table, 5);
  gtk_table_set_row_spacings (bubble_widget->table, 5);
 
  gtk_table_attach (GTK_TABLE (bubble_widget->table),
                    bubble_widget->icon,
                    0, 1, 1, 2,
                    GTK_FILL, GTK_FILL,
                    0, 0);

  gtk_table_attach (GTK_TABLE (bubble_widget->table),
                    bubble_widget->bubble_widget_header_label,
                    1, 2, 0, 1,
                    GTK_FILL, GTK_FILL,
                    0, 0);

  gtk_table_attach (GTK_TABLE (bubble_widget->table),
                    bubble_widget->bubble_widget_body_label,
                    1, 2, 1, 2,
                    GTK_FILL, GTK_FILL,
                    0, 0);
  
  gtk_container_add (GTK_CONTAINER (bubble_widget), bubble_widget->table);

  bubble_widget->body_layout = pango_layout_new (
                                 gtk_widget_get_pango_context (
                                   GTK_WIDGET (bubble_widget)));

  g_signal_connect (bubble_widget, "style-set",
                    G_CALLBACK (egg_notification_bubble_widget_context_changed_handler),
                    NULL);

  g_signal_connect (bubble_widget, "direction-changed",
                    G_CALLBACK (egg_notification_bubble_widget_context_changed_handler),
                    NULL);

  g_signal_connect_after (bubble_widget, "event-after",
                          G_CALLBACK (egg_notification_bubble_widget_event_handler),
                          bubble_widget);
   
  g_signal_connect (bubble_widget->bubble_widget_body_label, "expose-event",
                    G_CALLBACK (egg_notification_bubble_widget_body_label_expose_handler),
                    bubble_widget);
}

static void
_layout_window (EggNotificationBubbleWidget *bubble_widget,
                int alignment)
{

  gtk_container_remove (GTK_CONTAINER (bubble_widget->table), 
                        bubble_widget->icon);

  gtk_container_remove (GTK_CONTAINER (bubble_widget->table), 
                        bubble_widget->bubble_widget_header_label);

  gtk_container_remove (GTK_CONTAINER (bubble_widget->table), 
                        bubble_widget->bubble_widget_body_label);

  if (alignment == TRIANGLE_LEFT)
    {
      gtk_table_attach (GTK_TABLE (bubble_widget->table),
                        bubble_widget->icon,
                        0, 1, 1, 2,
                        GTK_FILL, GTK_FILL,
                        0, 0);

      gtk_table_attach (GTK_TABLE (bubble_widget->table),
                        bubble_widget->bubble_widget_header_label,
                        1, 2, 0, 1,
                        GTK_FILL, GTK_FILL,
                        0, 0);

      gtk_table_attach (GTK_TABLE (bubble_widget->table),
                        bubble_widget->bubble_widget_body_label,
                        1, 2, 1, 2,
                        GTK_FILL, GTK_FILL,
                        0, 0);
    }
  else
    {
      gtk_table_attach (GTK_TABLE (bubble_widget->table),
                        bubble_widget->icon,
                        1, 2, 1, 2,
                        GTK_FILL, GTK_FILL,
                        0, 0);

      gtk_table_attach (GTK_TABLE (bubble_widget->table),
                        bubble_widget->bubble_widget_header_label,
                        0, 1, 0, 1,
                        GTK_FILL, GTK_FILL,
                        0, 0);

      gtk_table_attach (GTK_TABLE (bubble_widget->table),
                        bubble_widget->bubble_widget_body_label,
                        0, 1, 1, 2,
                        GTK_FILL, GTK_FILL,
                        0, 0);

    }
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

static void
_calculate_pango_layout_from_aspect (PangoLayout *layout,
                                     const char *text, 
                                     double factor)
{
  gint len;
  gint w, h;
  double x;
  
  len = strlen (text);
  pango_layout_set_width(layout, -1);
  pango_layout_set_text (layout, text, len);
  
  pango_layout_get_pixel_size (layout, &w, &h);

  if (w > TEXT_WIDTH_THRESHOLD)
    {
      pango_layout_get_size (layout, &w, &h);

      x = sqrt (factor * w / h);
      w = round (w / x);

      pango_layout_set_width(layout, w);
    }
}

void
egg_notification_bubble_widget_set (EggNotificationBubbleWidget *bubble_widget,
			     const gchar *bubble_widget_header_text,
                             const gchar *icon,
			     const gchar *bubble_widget_body_text)
{
  gchar *markupquoted;
  gchar *markuptext;
  gint w, h;

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
  markuptext = g_strdup_printf ("<span size=\"larger\" weight=\"ultrabold\">%s</span>", markupquoted);
  gtk_label_set_markup (GTK_LABEL (bubble_widget->bubble_widget_header_label), markuptext);
  g_free (markuptext);
  g_free (markupquoted);

  _calculate_pango_layout_from_aspect (bubble_widget->body_layout,
                                       bubble_widget->bubble_widget_body_text, 
                                       0.25);

  pango_layout_get_pixel_size (bubble_widget->body_layout, &w, &h);
  gtk_widget_set_size_request (bubble_widget->bubble_widget_body_label, w, h);
  
  
}

void
egg_notification_bubble_widget_set_pos (EggNotificationBubbleWidget   *bubble_widget,
                                 gint x, gint y)
{
  GdkScreen *screen;
  gint monitor_num;
  GdkRectangle monitor;

  bubble_widget->x = x;
  bubble_widget->y = y;

  screen = gtk_window_get_screen (GTK_WINDOW(bubble_widget));
  monitor_num = gdk_screen_get_monitor_at_point (screen, x, y);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  gtk_window_move (GTK_WINDOW (bubble_widget), x, y);

  if (x < (monitor.x + monitor.width) / 2)
      _layout_window (bubble_widget, TRIANGLE_LEFT);
  else
      _layout_window (bubble_widget, TRIANGLE_RIGHT);
}

static void
egg_notification_bubble_widget_realize (GtkWidget *widget)
{
  GtkWidgetClass *widget_parent_class;

  widget_parent_class = (GtkWidgetClass *)parent_class;
  if (widget_parent_class->realize)
    widget_parent_class->realize (widget);

  draw_bubble_widget (EGG_NOTIFICATION_BUBBLE_WIDGET(widget));
}

static void
egg_notification_bubble_widget_screen_changed (GtkWidget *widget,
                                               GdkScreen *old_screen)
{
  GtkWidgetClass *widget_parent_class;
  EggNotificationBubbleWidget *bw;
  GdkScreen *screen;
  GdkColormap *colormap;
  gboolean can_composite;
 
  bw = EGG_NOTIFICATION_BUBBLE_WIDGET (widget);

  widget_parent_class = (GtkWidgetClass *)parent_class;
  if (widget_parent_class->screen_changed)
    widget_parent_class->screen_changed (widget, old_screen);

  can_composite = TRUE;
  
  screen = gtk_widget_get_screen (widget);
  colormap = gdk_screen_get_rgba_colormap (screen);
  
  if (!colormap)
    {
      colormap = gdk_screen_get_rgb_colormap (screen);
      can_composite = FALSE;
    }
    
  gtk_widget_set_colormap (widget, colormap);
  
  bw->can_composite = can_composite;
}


static gboolean
egg_notification_bubble_widget_body_label_expose_handler (GtkWidget *widget, 
                                                          GdkEventExpose *event, 
                                                          EggNotificationBubbleWidget *bw)
{
  cairo_t *cr;
  cr = gdk_cairo_create (GTK_WIDGET(bw)->window);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba (cr, bw->body_text_color.red, 
                             bw->body_text_color.green, 
                             bw->body_text_color.blue, 
                             0.60);
  cairo_move_to (cr, event->area.x, event->area.y);

  pango_cairo_layout_path (cr, bw->body_layout);

  cairo_fill (cr);
  cairo_destroy (cr);
}

static gboolean 
egg_notification_bubble_widget_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkWidgetClass *widget_parent_class;
  EggNotificationBubbleWidget *bw;

  bw = EGG_NOTIFICATION_BUBBLE_WIDGET (widget);

  draw_bubble_widget (bw);

  widget_parent_class = (GtkWidgetClass *)parent_class;
  if (widget_parent_class->expose_event)
    widget_parent_class->expose_event (widget, event);
 
  return TRUE;
}

static GdkPoint 
_stencil_bubble_top_right (cairo_t *cr,
                           GdkRectangle *rect,
			   int pos_x, int pos_y)
{
  GdkPoint triangle[3];
  double d, p1x, p2x, p3x, pdx, pvx, p1y, p2y, p3y, pdy, pvy;

  triangle[2].x = rect->x + rect->width - TRIANGLE_START;
  triangle[2].y = rect->y;

  triangle[0].x = triangle[2].x - TRIANGLE_WIDTH;
  triangle[0].y = rect->y;
  triangle[1].x = (triangle[2].x - triangle[0].x) / 2 + triangle[0].x;
  triangle[1].y = rect->y - BORDER_SIZE + 5;

#if 0
  if (triangle[1].x + (BORDER_SIZE - 5) < pos_x)
    triangle[1].x = pos_x - (BORDER_SIZE + 5);
#endif 

  cairo_move_to (cr, triangle[0].x, triangle[0].y);
  cairo_line_to (cr, triangle[1].x, triangle[1].y);
  cairo_line_to (cr, triangle[2].x, triangle[2].y);

  cairo_line_to (cr, rect->x + rect->width - CURVE_LENGTH, rect->y);
  cairo_curve_to (cr, 
                  rect->x + rect->width,
                  rect->y,
                  rect->x + rect->width,
                  rect->y,
                  rect->x + rect->width, 
                  rect->y + CURVE_LENGTH);

  cairo_line_to (cr, rect->x + rect->width, rect->y + rect->height - CURVE_LENGTH);

  p1x = rect->x + rect->width;
  p2x = rect->x;
  p1y = rect->y + rect->height + (BORDER_SIZE - 5);
  p2y = rect->y + rect->height;
  
  pdx = p1x - p2x;
  pdy = p1y - p2y;
  
  d = sqrt (pdx * pdx + 
            pdy * pdy);

  pvx = (pdx / d);
  pvy = (pdy / d);
 
  p3x = p1x - CURVE_LENGTH * pvx;
  p3y = p1y - CURVE_LENGTH * pvy;

  cairo_curve_to (cr, 
                  p1x,
                  p1y,
                  p1x,
                  p1y,
                  p3x, 
                  p3y);

  p3x = p2x + CURVE_LENGTH * pvx;
  p3y = p2y + CURVE_LENGTH * pvy;
  
  cairo_line_to (cr, p3x, p3y);

  cairo_curve_to (cr, 
                  p2x,
                  p2y,
                  p2x,
                  p2y,
                  p2x, 
                  p2y - CURVE_LENGTH);

  cairo_line_to (cr, rect->x, rect->y + CURVE_LENGTH);
  p1x = rect->x + CURVE_LENGTH;
  if (p1x < triangle[0].x)
    {
      cairo_curve_to (cr, 
                      rect->x,
                      rect->y,
                      rect->x,
                      rect->y,
                      p1x, 
                      rect->y);
      cairo_line_to (cr, triangle[0].x, rect->y);
    }
  else
    {
      cairo_curve_to (cr, 
                      rect->x,
                      rect->y,
                      rect->x,
                      rect->y,
                      triangle[0].x, 
                      rect->y);
    }

  return triangle[1];
}

static GdkPoint
_stencil_bubble_top_left  (cairo_t *cr,
                           GdkRectangle *rect, 
                           int pos_x, int pos_y)
{
  GdkPoint triangle[3];
  double d, p1x, p2x, p3x, pdx, pvx, p1y, p2y, p3y, pdy, pvy;

  triangle[0].x = rect->x + TRIANGLE_START;
  triangle[0].y = rect->y;
  triangle[2].x = triangle[0].x + TRIANGLE_WIDTH;
  triangle[2].y = rect->y;
  triangle[1].x = (triangle[2].x - triangle[0].x) / 2 + triangle[0].x;
  triangle[1].y = rect->y - BORDER_SIZE + 5;

  //if (triangle[1].x - (BORDER_SIZE - 5 ) > pos_x)
  //  triangle[1].x = pos_x + (BORDER_SIZE + 5);

  cairo_move_to (cr, triangle[0].x, triangle[0].y);
  cairo_line_to (cr, triangle[1].x, triangle[1].y);
  cairo_line_to (cr, triangle[2].x, triangle[2].y);

  cairo_line_to (cr, rect->x + rect->width - CURVE_LENGTH, rect->y);
  cairo_curve_to (cr, 
                  rect->x + rect->width,
                  rect->y,
                  rect->x + rect->width,
                  rect->y,
                  rect->x + rect->width, 
                  rect->y + CURVE_LENGTH);

  cairo_line_to (cr, rect->x + rect->width, rect->y + rect->height - CURVE_LENGTH);

  p1x = rect->x + rect->width;
  p2x = rect->x;
  p1y = rect->y + rect->height;
  p2y = rect->y + rect->height + (BORDER_SIZE - 5);
  
  pdx = p1x - p2x;
  pdy = p1y - p2y;
  
  d = sqrt (pdx * pdx + 
            pdy * pdy);

  pvx = (pdx / d);
  pvy = (pdy / d);
 
  p3x = p1x - CURVE_LENGTH * pvx;
  p3y = p1y - CURVE_LENGTH * pvy;

  cairo_curve_to (cr, 
                  p1x,
                  p1y,
                  p1x,
                  p1y,
                  p3x, 
                  p3y);

  p3x = p2x + CURVE_LENGTH * pvx;
  p3y = p2y + CURVE_LENGTH * pvy;
  
  cairo_line_to (cr, p3x, p3y);

  cairo_curve_to (cr, 
                  p2x,
                  p2y,
                  p2x,
                  p2y,
                  p2x, 
                  p2y - CURVE_LENGTH);

  cairo_line_to (cr, rect->x, rect->y + CURVE_LENGTH);
  p1x = rect->x + CURVE_LENGTH;
  if (p1x < triangle[0].x)
    {
      cairo_curve_to (cr, 
                      rect->x,
                      rect->y,
                      rect->x,
                      rect->y,
                      p1x, 
                      rect->y);
      cairo_line_to (cr, triangle[0].x, rect->y);
    }
  else
    {
      cairo_curve_to (cr, 
                      rect->x,
                      rect->y,
                      rect->x,
                      rect->y,
                      triangle[0].x, 
                      rect->y);
    }

  return triangle[1];
}

static GdkPoint
_stencil_bubble_bottom_right (cairo_t *cr,
                              GdkRectangle *rect,
			      int pos_x, int pos_y)
{

}

static GdkPoint 
_stencil_bubble_bottom_left  (cairo_t *cr,
                              GdkRectangle *rect,
			      int pos_x, int pos_y)
{

}

static GdkColor
_blend_colors (GdkColor color1, GdkColor color2, gdouble factor)
{
  GdkColor result;
  gint channel_intensity;
  
  channel_intensity = color1.red * factor + color2.red * (1 - factor);
  result.red = CLAMP (channel_intensity, 0, 65535);
  
  channel_intensity = color1.green * factor + color2.green * (1 - factor);
  result.green = CLAMP (channel_intensity, 0, 65535);

  channel_intensity = color1.blue * factor + color2.blue * (1 - factor);
  result.blue = CLAMP (channel_intensity, 0, 65535);

  return result;
}

static void
_calculate_colors_from_style (EggNotificationBubbleWidget *bw)
{
  GtkStyle *style;
  GtkWidget *widget;
  GdkColor header_text_color;
  GdkColor body_text_color;
  GdkColor bg_end_gradient;
  GdkColor border_color;
  GdkColor bg_start_gradient;

  widget = GTK_WIDGET (bw);
  
  gtk_widget_ensure_style (widget);
  style = widget->style;

  header_text_color = style->text[GTK_STATE_NORMAL];
  body_text_color = style->text[GTK_STATE_NORMAL];
  bg_start_gradient = style->base[GTK_STATE_NORMAL];
  bg_end_gradient = style->bg[GTK_STATE_SELECTED];
  border_color = style->mid[GTK_STATE_NORMAL];
  
  bg_end_gradient = _blend_colors (bg_start_gradient, bg_end_gradient, 0.25);
 
  bw->header_text_color = header_text_color;
  bw->body_text_color = body_text_color;
  bw->bg_start_gradient = bg_start_gradient;
  bw->bg_end_gradient = bg_end_gradient;
  bw->border_color = border_color;
}


static void
draw_bubble_widget (EggNotificationBubbleWidget *bubble_widget)
{
  GtkRequisition requisition;
  gint x, y, w, h;
  GdkScreen *screen;
  gint monitor_num;
  GdkRectangle monitor;
  GdkRectangle rectangle;
  cairo_pattern_t *pat;
  GdkPixmap *mask;
  GdkPoint arrow_pos;
  PangoAttribute *body_text_color;
  PangoAttrList *attrlist;
  
  int orient;
  int orient_triangle;  
  guint rectangle_border;
  GtkWidget *widget;
  cairo_t *cairo_context;
  cairo_t *mask_cr;
  gboolean can_composite;

  widget = GTK_WIDGET(bubble_widget);
  cairo_context = gdk_cairo_create (widget->window);
  
  can_composite = bubble_widget->can_composite;

  _calculate_colors_from_style (bubble_widget);
 
  x = bubble_widget->x;
  y = bubble_widget->y;

  screen = gtk_window_get_screen (GTK_WINDOW(widget));
  monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if (x < (monitor.x + monitor.width) / 2)
    {
      orient_triangle = TRIANGLE_LEFT;
    }
  else
    { 
      orient_triangle = TRIANGLE_RIGHT;
    }

  gtk_widget_size_request (widget, &requisition);
  w = requisition.width;
  h = requisition.height;

  if (!can_composite)
    {
      mask = gdk_pixmap_new (NULL, w, h, 1);
      mask_cr = gdk_cairo_create ((GdkDrawable *) mask);
    }

  orient = ORIENT_TOP;

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

  if (orient == ORIENT_TOP)
    {
      if (orient_triangle == TRIANGLE_LEFT)
        {
          arrow_pos = 
            _stencil_bubble_top_left (cairo_context, &rectangle, x, y);
          if (!can_composite)
            _stencil_bubble_top_left (mask_cr, &rectangle, x, y);
        }
      else
        {
          arrow_pos = 
            _stencil_bubble_top_right (cairo_context, &rectangle, x, y);
          if (!can_composite)
            _stencil_bubble_top_right (mask_cr, &rectangle, x, y);
        }
    }
  else
    {
      if (orient_triangle == TRIANGLE_LEFT)
        {
          arrow_pos = 
            _stencil_bubble_bottom_left (cairo_context, &rectangle, x, y);
          if (!can_composite)
            _stencil_bubble_bottom_left (mask_cr, &rectangle, x, y);
        }
      else
        {
          arrow_pos = 
            _stencil_bubble_bottom_right (cairo_context, &rectangle, x, y);
          if (!can_composite)
            _stencil_bubble_bottom_right (mask_cr, &rectangle, x, y);
        }
    }

  //cairo_set_source_rgba (cairo_context, 0.43, 0.49, 0.55, 1);

  if (can_composite)
    cairo_set_source_rgba (cairo_context, 1, 1, 1, 0);
  else
    cairo_set_source_rgba (cairo_context, 
                           bubble_widget->border_color.red / 65535.0, 
                           bubble_widget->border_color.green / 65535.0,
                           bubble_widget->border_color.blue / 65535.0, 1);
                         
  cairo_set_operator (cairo_context, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cairo_context);
 
  cairo_set_operator (cairo_context, CAIRO_OPERATOR_OVER);

#if 0
  //create shadow
  if (can_composite)
    {
      cairo_path_t *path;
      cairo_pattern_t *blur;

      path = cairo_copy_path_flat (cairo_context);
      cairo_new_path (cairo_context);
      
      cairo_translate (cairo_context, 5, 5);
      cairo_append_path (cairo_context, path);
      blur = cairo_pattern_create_rgba (0, 0, 0, .75);
      cairo_pattern_set_filter (blur, CAIRO_FILTER_GAUSSIAN);

      cairo_set_source (cairo_context, blur);
      cairo_fill (cairo_context);
     
      cairo_identity_matrix (cairo_context);
      cairo_append_path (cairo_context, path);
     
      cairo_pattern_destroy (blur);
      cairo_path_destroy (path);
    }

#endif

  pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, h);
  //cairo_pattern_add_color_stop_rgba (pat, 1, 0.59, 0.76, 0.93, 1);
  cairo_pattern_add_color_stop_rgba (pat, 0, 
                                     bubble_widget->bg_start_gradient.red /
                                     65535.0, 
                                     bubble_widget->bg_start_gradient.green /
                                     65535.0,
                                     bubble_widget->bg_start_gradient.blue /
                                     65535.0, 1);

  cairo_pattern_add_color_stop_rgba (pat, 1, 
                                     bubble_widget->bg_end_gradient.red /
                                     65535.0, 
                                     bubble_widget->bg_end_gradient.green /
                                     65535.0,
                                     bubble_widget->bg_end_gradient.blue /
                                     65535.0, 1);

  cairo_set_source (cairo_context, pat);
  cairo_fill_preserve (cairo_context);
  cairo_pattern_destroy (pat);
  
  cairo_set_line_width (cairo_context, 3.5);
  cairo_set_source_rgba (cairo_context, 0.43, 0.49, 0.55, 1);
  cairo_stroke (cairo_context);

  if (!can_composite)
    {
      cairo_set_operator (mask_cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (mask_cr);

      cairo_set_operator (mask_cr, CAIRO_OPERATOR_OVER);
      cairo_set_line_width (mask_cr, 3.5);
      cairo_set_source_rgba (mask_cr, 1, 1, 1, 1);
      cairo_fill_preserve (mask_cr);
      cairo_stroke (mask_cr);

      gdk_window_shape_combine_mask (widget->window,
                                     (GdkBitmap *) mask,
                                     0,
                                     0);
      gdk_pixmap_unref (mask);
      cairo_destroy (mask_cr);
    }

  cairo_destroy (cairo_context);
 
  gtk_window_move (GTK_WINDOW (widget), x - arrow_pos.x, y + 5);
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
  return g_object_new (EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET, 
                       "type", GTK_WINDOW_POPUP, 
                       NULL);
}

static void 
egg_notification_bubble_widget_context_changed_handler (EggNotificationBubbleWidget *bubble_widget)
{
  pango_layout_context_changed (bubble_widget->body_layout);
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

