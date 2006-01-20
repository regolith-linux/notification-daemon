#ifndef _ENGINES_H_
#define _ENGINES_H_

#include <gtk/gtk.h>

typedef void (*UrlClickedCb)(GtkWindow *nw, const char *url);

GtkWindow *theme_create_notification(UrlClickedCb url_clicked_cb);
void theme_destroy_notification(GtkWindow *nw);
void theme_show_notification(GtkWindow *nw);
void theme_hide_notification(GtkWindow *nw);
void theme_set_notification_hints(GtkWindow *nw, GHashTable *hints);
void theme_set_notification_text(GtkWindow *nw, const char *summary,
								 const char *body);
void theme_set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf);
void theme_set_notification_arrow(GtkWindow *nw, gboolean visible,
								  int x, int y);
void theme_add_notification_action(GtkWindow *nw, const char *label,
								   const char *key, GCallback cb);
void theme_move_notification(GtkWindow *nw, int x, int y);

#endif /* _ENGINES_H_ */
