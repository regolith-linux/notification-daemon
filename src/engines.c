#include <gconf/gconf-client.h>
#include "engines.h"

typedef struct
{
	GModule *module;
	guint ref_count;

	GtkWindow *(*create_notification)(void);
	void (*destroy_notification)(GtkWindow *nw);
	void (*show_notification)(GtkWindow *nw);
	void (*hide_notification)(GtkWindow *nw);
	void (*set_notification_hints)(GtkWindow *nw, GHashTable *hints);
	void (*set_notification_text)(GtkWindow *nw, const char *summary,
								  const char *body);
	void (*set_notification_icon)(GtkWindow *nw, GdkPixbuf *pixbuf);
	void (*set_notification_arrow)(GtkWindow *nw, gboolean visible,
								   int x, int y);
	void (*add_notification_action)(GtkWindow *nw, const char *label,
									const char *key, GCallback cb);
	void (*move_notification)(GtkWindow *nw, int x, int y);

} ThemeEngine;

static guint theme_prop_notify_id = 0;
static ThemeEngine *active_engine = NULL;

static ThemeEngine *
load_theme_engine(const char *name)
{
	ThemeEngine *engine;
	char *filename;
	char *path;

	filename = g_strdup_printf("lib%s.so", name);
	path = g_build_filename(ENGINES_DIR, filename, NULL);
	g_free(filename);

	printf("Loading path '%s'\n", path);
	engine = g_new0(ThemeEngine, 1);
	engine->ref_count = 1;
	engine->module = g_module_open(path, G_MODULE_BIND_LAZY);

	g_free(path);

	if (engine->module == NULL)
	{
		g_free(engine);
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
	BIND_REQUIRED_FUNC(set_notification_hints);
	BIND_REQUIRED_FUNC(set_notification_text);
	BIND_REQUIRED_FUNC(set_notification_icon);
	BIND_REQUIRED_FUNC(set_notification_arrow);
	BIND_REQUIRED_FUNC(add_notification_action);
	BIND_REQUIRED_FUNC(move_notification);

	return engine;
}

static void
destroy_engine(ThemeEngine *engine)
{
	g_assert(engine->ref_count == 0);

	if (active_engine == engine)
		active_engine = NULL;

	g_module_close(engine->module);
	g_free(engine);
}

static void
theme_changed_cb(GConfClient *client, guint cnxn_id, GConfEntry *entry,
				 gpointer user_data)
{
	if (active_engine == NULL)
		return;

	active_engine->ref_count--;

	if (active_engine->ref_count == 0)
		destroy_engine(active_engine);

	/* This is no longer the true active engine, so reset this. */
	active_engine = NULL;
}

static ThemeEngine *
get_theme_engine(void)
{
	if (active_engine == NULL)
	{
		GConfClient *client = get_gconf_client();
		char *enginename = gconf_client_get_string(client,
			"/apps/notification-daemon/theme", NULL);

		if (theme_prop_notify_id == 0)
		{
			theme_prop_notify_id = gconf_client_notify_add(client,
				"/apps/notification-daemon/theme", theme_changed_cb, NULL,
				NULL, NULL);
		}

		if (enginename == NULL)
		{
			active_engine = load_theme_engine("standard");
			g_assert(active_engine != NULL);
		}
		else
		{
			active_engine = load_theme_engine(enginename);

			if (active_engine == NULL)
			{
				g_warning("Unable to load theme engine '%s'", enginename);
				active_engine = load_theme_engine("standard");
			}

			g_free(enginename);

			g_assert(active_engine != NULL);
		}
	}

	return active_engine;
}

GtkWindow *
theme_create_notification(void)
{
	ThemeEngine *engine = get_theme_engine();
	GtkWindow *nw = engine->create_notification();
	g_object_set_data(G_OBJECT(nw), "_theme_engine", engine);
	engine->ref_count++;
	return nw;
}

void
theme_destroy_notification(GtkWindow *nw)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->destroy_notification(nw);

	engine->ref_count--;

	if (engine->ref_count == 0)
		destroy_engine(engine);
}

void
theme_show_notification(GtkWindow *nw)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->show_notification(nw);
}

void
theme_hide_notification(GtkWindow *nw)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->hide_notification(nw);
}

void
theme_set_notification_hints(GtkWindow *nw, GHashTable *hints)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->set_notification_hints(nw, hints);
}

void
theme_set_notification_text(GtkWindow *nw, const char *summary,
							const char *body)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->set_notification_text(nw, summary, body);
}

void
theme_set_notification_icon(GtkWindow *nw, GdkPixbuf *pixbuf)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->set_notification_icon(nw, pixbuf);
}

void
theme_set_notification_arrow(GtkWindow *nw, gboolean visible, int x, int y)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->set_notification_arrow(nw, visible, x, y);
}

void
theme_add_notification_action(GtkWindow *nw, const char *label,
							  const char *key, GCallback cb)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->add_notification_action(nw, label, key, cb);
}

void
theme_move_notification(GtkWindow *nw, int x, int y)
{
	ThemeEngine *engine = g_object_get_data(G_OBJECT(nw), "_theme_engine");
	engine->move_notification(nw, x, y);
}
