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
#define BORDER_LINE_WIDTH 2
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
static void _stencil_bubble (EggNotificationBubbleWidget *bw);


static GtkWindowClass *parent_class;

#define BEVEL_ALPHA_LIGHT  0.2
#define BEVEL_ALPHA_MEDIUM 0.5
#define BEVEL_ALPHA_DARK   0.8

enum
{
  DRAW_MOVE  = 0,
  DRAW_LINE  = 1,
  DRAW_CAP   = 2,
  DRAW_CLOSE = 3
};

typedef struct _DrawingInstruction
{
  gint type;

  gint end_x, end_y;
  gint corner_x, corner_y;
} DrawingInstruction;

enum
{
    ORIENT_TOP    = 0,
    ORIENT_BOTTOM = 1,
    ORIENT_LEFT   = 2,
    ORIENT_RIGHT  = 3
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
egg_notification_bubble_widget_init (EggNotificationBubbleWidget *bubble_widget)
{
  GtkWindow *win;

  win = GTK_WINDOW (bubble_widget);

  bubble_widget->can_composite = FALSE;

  egg_notification_bubble_widget_screen_changed (GTK_WIDGET (bubble_widget),
                                                 NULL);

  bubble_widget->dp.is_clear = TRUE;
  bubble_widget->dp.pipeline = NULL;
  bubble_widget->draw_arrow = FALSE;

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
_layout_window (EggNotificationBubbleWidget *bubble_widget,
                int alignment)
{
  if (bubble_widget->draw_arrow)
    gtk_container_set_border_width (GTK_CONTAINER (bubble_widget), BORDER_SIZE + 5);
  else
    gtk_container_set_border_width (GTK_CONTAINER (bubble_widget), 10);

  if (gtk_widget_get_parent (bubble_widget->icon))
    gtk_container_remove (GTK_CONTAINER (bubble_widget->table),
                          bubble_widget->icon);

  if (gtk_widget_get_parent (bubble_widget->bubble_widget_header_label))
    gtk_container_remove (GTK_CONTAINER (bubble_widget->table),
                          bubble_widget->bubble_widget_header_label);

  if (gtk_widget_get_parent (bubble_widget->bubble_widget_body_label))
    gtk_container_remove (GTK_CONTAINER (bubble_widget->table),
                          bubble_widget->bubble_widget_body_label);

  if (bubble_widget->button_hbox != NULL &&
        gtk_widget_get_parent (bubble_widget->button_hbox) != NULL)
    gtk_container_remove (GTK_CONTAINER (bubble_widget->table),
                        bubble_widget->button_hbox);

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

    if (bubble_widget->button_hbox != NULL)
      {
        gtk_table_attach (GTK_TABLE (bubble_widget->table),
                          bubble_widget->button_hbox,
                          0, 2, 2, 3,
                          GTK_FILL, GTK_FILL,
                          0, 0);

      }

    gtk_widget_show_all (bubble_widget->table);
}

static void
_drawing_instruction_internal_add (GList **pipeline,
                                   guint type,
                                   gint end_x, gint end_y,
                                   gint corner_x, gint corner_y)
{
  DrawingInstruction *di;

  di = g_new0 (DrawingInstruction, 1);
  di->type = type;
  di->end_x = end_x;
  di->end_y = end_y;
  di->corner_x = corner_x;
  di->corner_y = corner_y;

  *pipeline = g_list_append (*pipeline, di);
}

static void
_drawing_instruction_move (GList **pipeline,
                           gint x, gint y)
{
  _drawing_instruction_internal_add (pipeline, DRAW_MOVE, x, y, 0, 0);
}

static void
_drawing_instruction_line (GList **pipeline,
                           gint x, gint y)
{
  _drawing_instruction_internal_add (pipeline, DRAW_LINE, x, y, 0, 0);
}

static void
_drawing_instruction_cap  (GList **pipeline,
                           gint x, gint y,
                           gint corner_x, gint corner_y)
{
  _drawing_instruction_internal_add (pipeline, DRAW_CAP, x, y, corner_x, corner_y);
}

static void
_drawing_instruction_close  (GList **pipeline)
{
  _drawing_instruction_internal_add (pipeline, DRAW_CLOSE, 0, 0, 0, 0);
}

/* given a distance from start point x1, y1
   calculate the point dist units away */
static GdkPoint
_calc_point_on_line (x1, y1, x2, y2, dist)
{
  GdkPoint result;
  gint dx, dy;
  gdouble d, vx, vy;

  dx = x2 - x1;
  dy = y2 - y1;

  d = sqrt (dx * dx + dy * dy);
  vx = dx / d;
  vy = dy / d;

  result.x = x1 + dist * vx;
  result.y = y1 + dist * vy;

  return result;
}

static void
_edge_line_to (EggNotificationBubbleWidget *bw,
               gint x, gint y,
               gint corner_radius)
{
  if (bw->dp.is_clear == TRUE)
    {
      bw->dp.start_x = x;
      bw->dp.start_y = y;
      bw->dp.start_corner_radius = corner_radius;
      bw->dp.is_clear = FALSE;
    }
  else
    {
      GdkPoint start_p;
      GdkPoint end_p;

      start_p = _calc_point_on_line (bw->dp.last_x,
                                     bw->dp.last_y,
                                     x,
                                     y,
                                     bw->dp.last_corner_radius);
      end_p = _calc_point_on_line (x,
                                   y,
                                   bw->dp.last_x,
                                   bw->dp.last_y,
                                   corner_radius);

      if (bw->dp.last_x == bw->dp.start_x &&
          bw->dp.last_y == bw->dp.start_y)
        _drawing_instruction_move (&bw->dp.pipeline, start_p.x, start_p.y);
      else
        _drawing_instruction_cap (&bw->dp.pipeline,
                                  start_p.x,
                                  start_p.y,
                                  bw->dp.last_x,
                                  bw->dp.last_y);

      _drawing_instruction_line (&bw->dp.pipeline, end_p.x, end_p.y);

    }

  bw->dp.last_x = x;
  bw->dp.last_y = y;
  bw->dp.last_corner_radius = corner_radius;
}

static void
_close_path (EggNotificationBubbleWidget *bw)
{
  GdkPoint start_p;
  GdkPoint end_p;
  DrawingInstruction *di;

  start_p = _calc_point_on_line (bw->dp.last_x,
                                 bw->dp.last_y,
                                 bw->dp.start_x,
                                 bw->dp.start_y,
                                 bw->dp.last_corner_radius);

  end_p = _calc_point_on_line (bw->dp.start_x,
                               bw->dp.start_y,
                               bw->dp.last_x,
                               bw->dp.last_y,
                               bw->dp.start_corner_radius);


  _drawing_instruction_cap (&bw->dp.pipeline,
                             start_p.x,
                             start_p.y,
                             bw->dp.last_x,
                             bw->dp.last_y);

  _drawing_instruction_line (&bw->dp.pipeline,
                             end_p.x,
                             end_p.y);

  di = (DrawingInstruction *) bw->dp.pipeline->data;
  _drawing_instruction_cap (&bw->dp.pipeline,
                             di->end_x,
                             di->end_y,
                             bw->dp.start_x,
                             bw->dp.start_y);

  _drawing_instruction_close (&bw->dp.pipeline);

}

static void
_drawing_instruction_draw (DrawingInstruction *di, cairo_t *cr)
{
  switch (di->type)
    {
      case DRAW_MOVE:
        cairo_move_to (cr, di->end_x, di->end_y);
        break;
      case DRAW_LINE:
        cairo_line_to (cr, di->end_x, di->end_y);
      break;
      case DRAW_CAP:
        cairo_curve_to (cr,
                        di->corner_x, di->corner_y,
                        di->corner_x, di->corner_y,
                        di->end_x, di->end_y);
      break;
      case DRAW_CLOSE:
        cairo_close_path (cr);
      break;
    }
}

static void
_drawing_pipeline_clear (EggNotificationBubbleWidget *bw)
{
  bw->dp.is_clear = TRUE;

  g_list_foreach (bw->dp.pipeline, (GFunc) g_free, NULL);
  g_list_free (bw->dp.pipeline);
  bw->dp.pipeline = NULL;
}

static void
_populate_window (EggNotificationBubbleWidget *bubble_widget)
{
  GtkWidget *widget;

  g_return_if_fail (EGG_IS_NOTIFICATION_BUBBLE_WIDGET (bubble_widget));

  widget = GTK_WIDGET (bubble_widget);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_app_paintable (widget, TRUE);
  gtk_window_set_resizable (GTK_WINDOW (bubble_widget), FALSE);

  bubble_widget->bubble_widget_header_label = gtk_label_new (NULL);

  //use placeholder so we can use pango/cairo to draw body
  bubble_widget->bubble_widget_body_label = gtk_frame_new("");
  gtk_frame_set_shadow_type (GTK_FRAME (bubble_widget->bubble_widget_body_label), GTK_SHADOW_NONE);
  bubble_widget->icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_BUTTON);
  gtk_widget_ref (bubble_widget->bubble_widget_header_label);
  gtk_widget_ref (bubble_widget->bubble_widget_body_label);
  gtk_widget_ref (bubble_widget->icon);

  gtk_label_set_line_wrap (GTK_LABEL (bubble_widget->bubble_widget_header_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (bubble_widget->bubble_widget_header_label), 0.0, 0.0);

  gtk_misc_set_alignment (GTK_MISC (bubble_widget->icon), 0.0, 0.0);
  gtk_widget_show (bubble_widget->icon);
  gtk_widget_show (bubble_widget->bubble_widget_header_label);
  gtk_widget_show (bubble_widget->bubble_widget_body_label);

  bubble_widget->table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (bubble_widget->table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (bubble_widget->table), 5);

  gtk_container_add (GTK_CONTAINER (bubble_widget), bubble_widget->table);

  bubble_widget->body_layout = pango_layout_new (
                                 gtk_widget_get_pango_context (
                                   GTK_WIDGET (bubble_widget)));

  /* do a fake layout for now so we can calculate
     height and width */
  _layout_window (bubble_widget, TRIANGLE_RIGHT);

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
  pango_layout_set_markup(layout, text, len);

  pango_layout_get_pixel_size (layout, &w, &h);

  if (w > TEXT_WIDTH_THRESHOLD)
    {
      double f;

      pango_layout_get_size (layout, &w, &h);

      x = sqrt (factor * w / h);
      if (x == 0)
        x = 1;

      f = (double) w / x;

      w = (gint) (f + 0.5);

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

  if (icon != NULL && strcmp (icon, "") != 0)
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

  /* TODO: This is wrong - if elements are added before
           set_pos is called the layout become wrong */
  if (x < (monitor.x + monitor.width) / 2)
      _layout_window (bubble_widget, TRIANGLE_LEFT);
  else
      _layout_window (bubble_widget, TRIANGLE_RIGHT);

  _stencil_bubble (bubble_widget);

  gtk_window_move (GTK_WINDOW (bubble_widget),
                   x - bubble_widget->offset_x,
                   y - bubble_widget->offset_y);
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

  return TRUE;
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

static void
_stencil_bubble_top_right (EggNotificationBubbleWidget *bw,
                           GdkRectangle *rect)
{
  GdkPoint triangle[3];

  triangle[0].y = rect->y;
  triangle[0].x = rect->x + rect->width - (TRIANGLE_START + TRIANGLE_WIDTH);
  triangle[2].x = triangle[0].x + TRIANGLE_WIDTH;
  triangle[2].y = rect->y;
  triangle[1].x = (triangle[2].x - triangle[0].x) / 2 + triangle[0].x;
  triangle[1].y = rect->y - BORDER_SIZE + 5;


  bw->offset_x = triangle[1].x;
  bw->offset_y = triangle[1].y;

  _edge_line_to (bw, triangle[0].x, triangle[0].y, 0);
  _edge_line_to (bw, triangle[1].x, triangle[1].y, 0);
  _edge_line_to (bw, triangle[2].x, triangle[2].y, 0);

  _edge_line_to (bw, rect->x + rect->width, rect->y, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y + rect->height, CURVE_LENGTH);

  _edge_line_to (bw, rect->x, rect->y + rect->height + BORDER_SIZE - 5, CURVE_LENGTH);

  _edge_line_to (bw, rect->x, rect->y, CURVE_LENGTH);

  _close_path (bw);
}

static void
_stencil_bubble_top_left  (EggNotificationBubbleWidget *bw,
                           GdkRectangle *rect)
{
  GdkPoint triangle[3];

  triangle[0].x = rect->x + TRIANGLE_START;
  triangle[0].y = rect->y;
  triangle[2].x = triangle[0].x + TRIANGLE_WIDTH;
  triangle[2].y = rect->y;
  triangle[1].x = (triangle[2].x - triangle[0].x) / 2 + triangle[0].x;
  triangle[1].y = rect->y - BORDER_SIZE + 5;

  bw->offset_x = triangle[1].x;
  bw->offset_y = triangle[1].y;

  _edge_line_to (bw, triangle[0].x, triangle[0].y, 0);
  _edge_line_to (bw, triangle[1].x, triangle[1].y, 0);
  _edge_line_to (bw, triangle[2].x, triangle[2].y, 0);

  _edge_line_to (bw, rect->x + rect->width, rect->y, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y + rect->height + BORDER_SIZE - 5, CURVE_LENGTH);

  _edge_line_to (bw, rect->x, rect->y + rect->height, CURVE_LENGTH);

  _edge_line_to (bw, rect->x, rect->y, CURVE_LENGTH);

  _close_path (bw);

}

static void
_stencil_bubble_bottom_right (EggNotificationBubbleWidget *bw,
                              GdkRectangle *rect)
{
  GdkPoint triangle[3];

  triangle[2].x = rect->x + rect->width - TRIANGLE_START;
  triangle[2].y = rect->y + rect->height;

  triangle[0].x = triangle[2].x - TRIANGLE_WIDTH;
  triangle[0].y = rect->y + rect->height;
  triangle[1].x = (triangle[2].x - triangle[0].x) / 2 + triangle[0].x;
  triangle[1].y = rect->y + rect->height +  BORDER_SIZE - 5;

  bw->offset_x = triangle[1].x;
  bw->offset_y = triangle[1].y;

  _edge_line_to (bw, triangle[2].x, triangle[2].y, 0);
  _edge_line_to (bw, triangle[1].x, triangle[1].y, 0);
  _edge_line_to (bw, triangle[0].x, triangle[0].y, 0);


  _edge_line_to (bw, rect->x, rect->y + rect->height, CURVE_LENGTH);
  _edge_line_to (bw, rect->x, rect->y - BORDER_SIZE + 5, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y + rect->height, CURVE_LENGTH);

  _close_path (bw);
}

static void
_stencil_bubble_bottom_left  (EggNotificationBubbleWidget *bw,
                              GdkRectangle *rect)
{
  GdkPoint triangle[3];

  triangle[0].x = rect->x + TRIANGLE_START;
  triangle[0].y = rect->y + rect->height;
  triangle[2].x = triangle[0].x + TRIANGLE_WIDTH;
  triangle[2].y = rect->y + rect->height;
  triangle[1].x = (triangle[2].x - triangle[0].x) / 2 + triangle[0].x;
  triangle[1].y = rect->y + rect->height + BORDER_SIZE - 5;

  bw->offset_x = triangle[1].x;
  bw->offset_y = triangle[1].y;

  _edge_line_to (bw, triangle[2].x, triangle[2].y, 0);
  _edge_line_to (bw, triangle[1].x, triangle[1].y, 0);
  _edge_line_to (bw, triangle[0].x, triangle[0].y, 0);

  _edge_line_to (bw, rect->x, rect->y + rect->height, CURVE_LENGTH);
  _edge_line_to (bw, rect->x, rect->y, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y - BORDER_SIZE + 5, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y + rect->height, CURVE_LENGTH);

  _close_path (bw);
}

static void
_stencil_bubble_no_arrow (EggNotificationBubbleWidget *bw,
                          GdkRectangle *rect)
{
  bw->offset_x = 0;
  bw->offset_y = 0;

  _edge_line_to (bw, rect->x + rect->width, rect->y, CURVE_LENGTH);
  _edge_line_to (bw, rect->x + rect->width, rect->y + rect->height, CURVE_LENGTH);

  _edge_line_to (bw, rect->x, rect->y + rect->height, CURVE_LENGTH);

  _edge_line_to (bw, rect->x, rect->y, CURVE_LENGTH);

  _close_path (bw);

}

static void
_stencil_bubble (EggNotificationBubbleWidget *bw)
{
  GdkRectangle rect;
  GtkRequisition req;
  gint rect_border;

  gtk_widget_size_request (GTK_WIDGET (bw), &req);

  if (bw->draw_arrow)
    rect_border = BORDER_SIZE - BORDER_LINE_WIDTH;
  else
    rect_border = BORDER_LINE_WIDTH;

  rect.x = rect_border;
  rect.y = rect_border;
  rect.width = req.width - (rect_border * 2);
  rect.height = req.height - (rect_border * 2);

  _drawing_pipeline_clear (bw);

  if (bw->draw_arrow)
    {
      GdkScreen *screen;
      GdkRectangle monitor;
      gint monitor_num;
      gint orient;
      gint orient_triangle;
      gint x, y;

      x = bw->x;
      y = bw->y;
      screen = gtk_window_get_screen (GTK_WINDOW(bw));
      monitor_num = gdk_screen_get_monitor_at_point (screen, x, y);
      gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

      /* TODO: draw corner cases */
      if (x < (monitor.x + monitor.width) / 2)
        orient_triangle = TRIANGLE_LEFT;
      else
        orient_triangle = TRIANGLE_RIGHT;

      orient = ORIENT_TOP;

      if ((y + req.height) > monitor.y + monitor.height)
        orient = ORIENT_BOTTOM;

      if (orient == ORIENT_TOP)
        {
          if (orient_triangle == TRIANGLE_RIGHT)
            _stencil_bubble_top_right (bw, &rect);
          else if (orient_triangle == TRIANGLE_LEFT)
            _stencil_bubble_top_left (bw, &rect);
        }
      else if (orient == ORIENT_BOTTOM)
        {
          if (orient_triangle == TRIANGLE_RIGHT)
            _stencil_bubble_bottom_right (bw, &rect);
          else if (orient_triangle == TRIANGLE_LEFT)
            _stencil_bubble_bottom_left (bw, &rect);
        }
    }
  else
    _stencil_bubble_no_arrow (bw, &rect);
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

  result.pixel = 0xFF; /* This is never really used. */

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
  gint w, h;
  cairo_pattern_t *pat;
  GdkPixmap *mask;
  GdkPoint arrow_pos;

  GtkWidget *widget;
  cairo_t *cairo_context;
  cairo_t *mask_cr;
  gboolean can_composite;

  mask_cr = NULL;
  mask = NULL;

  arrow_pos.x = 0;
  arrow_pos.y = 0;

  widget = GTK_WIDGET(bubble_widget);
  cairo_context = gdk_cairo_create (widget->window);

  can_composite = bubble_widget->can_composite;

  _calculate_colors_from_style (bubble_widget);

  gtk_widget_size_request (widget, &requisition);
  w = requisition.width;
  h = requisition.height;

  if (!can_composite)
    {
      mask = gdk_pixmap_new (NULL, w, h, 1);
      mask_cr = gdk_cairo_create ((GdkDrawable *) mask);
    }

  g_list_foreach (bubble_widget->dp.pipeline, (GFunc) _drawing_instruction_draw, cairo_context);
  if (!can_composite)
    g_list_foreach (bubble_widget->dp.pipeline, (GFunc) _drawing_instruction_draw, mask_cr);

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

GtkWidget *
egg_notification_bubble_widget_create_button (EggNotificationBubbleWidget *bubble_widget,
                                              const gchar *label)
{
  GtkWidget *b;
  GtkWidget *l;
  gchar *label_markup;

  b = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (b), GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (b), 0);

  label_markup = g_markup_printf_escaped ("<span weight=\"bold\" underline=\"single\" foreground=\"blue\">%s</span>", label);

  l = gtk_label_new (label_markup);
  gtk_label_set_use_markup (GTK_LABEL (l), TRUE);

  g_free (label_markup);

  gtk_container_add (GTK_CONTAINER (b), l);

  gtk_widget_show_all (b);

  if (bubble_widget->button_hbox == NULL)
    bubble_widget->button_hbox = gtk_hbox_new (FALSE, 0);

  gtk_box_pack_end (GTK_BOX (bubble_widget->button_hbox),
                    b,
                    FALSE, FALSE,
                    0);

  return (b);
}

void
egg_notification_bubble_widget_clear_buttons (EggNotificationBubbleWidget *bubble_widget)
{
  if (bubble_widget->button_hbox != NULL)
    gtk_widget_destroy (bubble_widget->button_hbox);

  bubble_widget->button_hbox = NULL;
}

void
egg_notification_bubble_widget_set_draw_arrow (EggNotificationBubbleWidget *bubble_widget,
                                               gboolean value)
{
  bubble_widget->draw_arrow = value;
}


