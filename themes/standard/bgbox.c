#include "bgbox.h"

G_DEFINE_TYPE(NotifydBgBox, notifyd_bgbox, GTK_TYPE_HBOX);

static gboolean
notifyd_bgbox_expose_event(GtkWidget *bgbox, GdkEventExpose *event)
{
	if (GTK_WIDGET_DRAWABLE(bgbox))
	{
		GtkStyle *style = gtk_widget_get_style(bgbox);
		GtkStateType state = GTK_WIDGET_STATE(bgbox);
		GdkGC *gc;

		printf("Expose\n");

		switch (NOTIFYD_BGBOX(bgbox)->palette)
		{
			case NOTIFYD_BASE:
				gc = style->base_gc[state];
				break;

			case NOTIFYD_BG:
				gc = style->bg_gc[state];
				break;

			case NOTIFYD_FG:
				gc = style->fg_gc[state];
				break;

			default:
				g_assert_not_reached();
		}

		gdk_draw_rectangle(GDK_DRAWABLE(bgbox->window), gc, TRUE,
						   bgbox->allocation.x,
						   bgbox->allocation.y,
						   bgbox->allocation.width,
						   bgbox->allocation.height);
	}

	return GTK_WIDGET_CLASS(notifyd_bgbox_parent_class)->expose_event(bgbox,
																	  event);
}

static void
notifyd_bgbox_class_init(NotifydBgBoxClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	widget_class->expose_event = notifyd_bgbox_expose_event;
}

static void
notifyd_bgbox_init(NotifydBgBox *bgbox)
{
	bgbox->palette = NOTIFYD_BASE;
}

GtkWidget *
notifyd_bgbox_new(NotifydPalette palette)
{
	NotifydBgBox *bgbox = g_object_new(NOTIFYD_TYPE_BGBOX, NULL);
	bgbox->palette = palette;

	return GTK_WIDGET(bgbox);
}
