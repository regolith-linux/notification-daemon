#include "engines.h"

typedef struct
{
	GModule *module;
	guint ref_count;

	gpointer (*create_notification)(void);
	void (*destroy_notification)(GtkWindow *nw);
	void (*show_notification)(GtkWindow *nw);
	void (*hide_notification)(GtkWindow *nw);
	void (*set_notification_text)(GtkWindow *nw, const char *summary,
								  const char *body);
	void (*set_notification_icon)(GtkWindow *nw, GdkPixbuf *pixbuf);
	void (*set_notification_arrow)(GtkWindow *nw, gboolean visible,
								   int x, int y);
	void (*add_notification_action)(GtkWindow *nw, const char *label,
									const char *key, GCallback cb);
	void (*move_notification)(GtkWindow *nw, int x, int y);

} ThemeEngine;

static ThemeEngine *active_engine = NULL;
/* static GList *engines = NULL; */

static ThemeEngine *
load_theme_engine(const char *filename)
{
	ThemeEngine *engine = g_new0(ThemeEngine, 1);

	engine->ref_count = 1;
	engine->module = g_module_open(filename, G_MODULE_BIND_LAZY);

	if (engine->module == NULL)
	{
		g_free(engine);
		g_error("The default theme engine doesn't exist. Your install "
				"likely isn't complete.");
		return NULL;
	}

#define BIND_REQUIRED_FUNC(name) \
	if (!g_module_symbol(engine->module, #name, (gpointer *)&engine->name)) \
	{ \
		/* Too harsh! Fall back to default. */ \
		g_error("Theme doesn't provide the required function '%s'", #name); \
		if (!g_module_close(engine->module)) \
			g_warning("%s: %s", filename, g_module_error()); \
		\
		g_free(engine); \
	}

	BIND_REQUIRED_FUNC(create_notification);
	BIND_REQUIRED_FUNC(destroy_notification);
	BIND_REQUIRED_FUNC(show_notification);
	BIND_REQUIRED_FUNC(hide_notification);
	BIND_REQUIRED_FUNC(set_notification_text);
	BIND_REQUIRED_FUNC(set_notification_icon);
	BIND_REQUIRED_FUNC(set_notification_arrow);
	BIND_REQUIRED_FUNC(add_notification_action);
	BIND_REQUIRED_FUNC(move_notification);

	return engine;
}

static ThemeEngine *
get_theme_engine(void)
{
	if (active_engine == NULL)
	{
		/* XXX */
		active_engine = load_theme_engine(ENGINES_DIR"/libstandard.so");
		g_assert(active_engine != NULL);
	}

	return active_engine;
}

gpointer
theme_create_notification(void)
{
	return get_theme_engine()->create_notification();
}

void
theme_destroy_notification(GtkWindow *nw)
{
	get_theme_engine()->destroy_notification(nw);
}

void
theme_show_notification(GtkWindow *nw)
{
	get_theme_engine()->show_notification(nw);
}

void
theme_hide_notification(GtkWindow *nw)
{
	get_theme_engine()->hide_notification(nw);
}

void
theme_set_notification_text(GtkWindow *nw, const char *summary,
							const char *body)
{
	get_theme_engine()->set_notification_text(nw, summary, body);
}

void
theme_set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf)
{
	get_theme_engine()->set_notification_icon(nw, pixbuf);
}

void
theme_set_notification_arrow(GtkWindow *nw, gboolean visible, int x, int y)
{
	get_theme_engine()->set_notification_arrow(nw, visible, x, y);
}

void
theme_add_notification_action(GtkWindow *nw, const char *label,
							  const char *key, GCallback cb)
{
	get_theme_engine()->add_notification_action(nw, label, key, cb);
}

void
theme_move_notification(GtkWindow *nw, int x, int y)
{
	get_theme_engine()->move_notification(nw, x, y);
}
