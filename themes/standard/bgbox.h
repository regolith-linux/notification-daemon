#ifndef _BGBOX_H_
#define _BGBOX_H_

typedef struct _NotifydBgBox      NotifydBgBox;
typedef struct _NotifydBgBoxClass NotifydBgBoxClass;

#include <gtk/gtk.h>

#define NOTIFYD_TYPE_BGBOX (notifyd_bgbox_get_type())
#define NOTIFYD_BGBOX(obj) \
		(G_TYPE_CHECK_INSTANCE_CAST((obj), NOTIFYD_TYPE_BGBOX, NotifydBgBox))
#define NOTIFYD_BGBOX_CLASS(klass) \
		(G_TYPE_CHECK_CLASS_CAST((klass), NOTIFYD_TYPE_BGBOX, NotifydBgBoxClass))
#define NOTIFYD_IS_BGBOX(obj) \
		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NOTIFYD_TYPE_BGBOX))
#define NOTIFYD_IS_BGBOX_CLASS(klass) \
		(G_TYPE_CHECK_CLASS_TYPE((klass), NOTIFYD_TYPE_BGBOX))
#define NOTIFYD_BGBOX_GET_CLASS(obj) \
		(G_TYPE_INSTANCE_GET_CLASS ((obj), NOTIFYD_TYPE_BGBOX, NotifydBgBoxClass))

typedef enum
{
	NOTIFYD_BASE,
	NOTIFYD_BG,
	NOTIFYD_FG

} NotifydPalette;

struct _NotifydBgBox
{
	GtkVBox parent_object;

	NotifydPalette palette;
};

struct _NotifydBgBoxClass
{
	GtkVBoxClass parent_class;
};

GType notifyd_bgbox_get_type(void);

GtkWidget *notifyd_bgbox_new(NotifydPalette palette);

#endif /* _BGBOX_H_ */
