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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __EGG_NOTIFICATION_BUBBLE_WIDGET_H__
#define __EGG_NOTIFICATION_BUBBLE_WIDGET_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET                  (egg_notification_bubble_widget_get_type ())
#define EGG_NOTIFICATION_BUBBLE_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET, EggNotificationBubbleWidget))
#define EGG_NOTIFICATION_BUBBLE_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET, EggNotificationBubbleWidgetClass))
#define EGG_IS_NOTIFICATION_BUBBLE_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET))
#define EGG_IS_NOTIFICATION_BUBBLE_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET))
#define EGG_NOTIFICATION_BUBBLE_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_NOTIFICATION_BUBBLE_WIDGET, EggNotificationBubbleWidgetClass))


typedef struct _EggNotificationBubbleWidget	 EggNotificationBubbleWidget;
typedef struct _EggNotificationBubbleWidgetClass EggNotificationBubbleWidgetClass;
typedef struct _EggNotificationBubbleWidgetData	 EggNotificationBubbleWidgetData;

struct _DrawingPipeline
{
  gint start_x;
  gint start_y;
  gint start_corner_radius;

  gint last_x;
  gint last_y;
  gint last_corner_radius;

  gboolean is_clear;

  GList *pipeline;
};

struct _EggNotificationBubbleWidget
{
  GtkWindow parent_instance;

  char *bubble_widget_header_text;
  char *bubble_widget_body_text;
  GtkWidget *icon;

  gboolean active;
  GtkWidget *table;
  GtkWidget *bubble_widget_header_label;
  GtkWidget *bubble_widget_body_label;
  GtkWidget *button_hbox;
  PangoLayout *body_layout;

  gint x, y;
  gint offset_x, offset_y;
  gboolean can_composite;
  gboolean draw_arrow;

  GdkColor header_text_color;
  GdkColor body_text_color;
  GdkColor bg_start_gradient;
  GdkColor bg_end_gradient;
  GdkColor border_color;

  /* drawing instructions */
  struct _DrawingPipeline dp;
};

struct _EggNotificationBubbleWidgetClass
{
  GtkWindowClass parent_class;

  void (*clicked) (void);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType		 egg_notification_bubble_widget_get_type	   (void) G_GNUC_CONST;
EggNotificationBubbleWidget*	 egg_notification_bubble_widget_new	   (void);

void		 egg_notification_bubble_widget_set (EggNotificationBubbleWidget   *bubble_widget,
					      const gchar	      *notification_header,
					      const gchar             *icon,
					      const gchar             *notification_body);

void             egg_notification_bubble_widget_show (EggNotificationBubbleWidget *bubble_widget);
void             egg_notification_bubble_widget_hide (EggNotificationBubbleWidget *bubble_widget);


void             egg_notification_bubble_widget_set_pos (EggNotificationBubbleWidget   *bubble_widget,
                                                         gint x, gint y);

void             egg_notification_bubble_widget_set_icon_from_data (EggNotificationBubbleWidget   *bubble_widget,
                                                                    const guchar                  *data,
                                                                    gboolean                       has_alpha,
                                                                    int                            bits_per_sample,
                                                                    int                            width,
                                                                    int                            height,
                                                                    int                            rowstride);


GtkWidget *      egg_notification_bubble_widget_create_button (EggNotificationBubbleWidget *bubble_widget,
                                                               const gchar *label);

void             egg_notification_bubble_widget_clear_buttons (EggNotificationBubbleWidget *bubble_widget);

void             egg_notification_bubble_widget_set_draw_arrow (EggNotificationBubbleWidget *bubble_widget, gboolean value);

G_END_DECLS

#endif /* __EGG_NOTIFICATION_BUBBLE_WIDGET_H__ */
