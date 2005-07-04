/**
 * @file PopupNotification.hh GTK+ based popup notifier
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
#ifndef _POPUP_NOTIFICATION_HH_
#define _POPUP_NOTIFICATION_HH_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "notifier.hh"

class PopupNotification : public Notification
{
protected:
	void update_position(void);

	bool get_work_area(GdkRectangle &rect);

	void format_summary(GtkLabel *label);
	void process_body_markup(GtkLabel *label);
	void linkify(GtkLabel *label);
	void whiten(GtkWidget *eventbox);

	static const int WIDTH         = 300;
	static const int MIN_HEIGHT    = 50;
	static const int IMAGE_SIZE    = 48;
	static const int IMAGE_PADDING = 10;

	PopupNotifier *notifier;

	GtkWidget *window;
	GtkWidget *body_box;
	int disp_screen;
	int height_offset;

public:
	GdkGC *gc;

	PopupNotification(PopupNotifier *n);
	~PopupNotification();

	void generate(void);
	void show(void);
	void update(void);

	void set_height_offset(int value);
	int get_height(void);
	void window_button_release(GdkEventButton *event);

	bool has_arrow(void) const;

	void generate_arrow(int &ret_arrow_x, int &ret_arrow_y);

	GdkPoint mArrowPoints[7];

	static const int ARROW_OFFSET = 20;
	static const int ARROW_WIDTH  = 20;
	static const int ARROW_HEIGHT = 20;
};

#endif /* _POPUP_NOTIFICATION_HH_ */
