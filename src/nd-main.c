/*
 * Copyright (C) 2015 Alberts Muktupāvels
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "nd-daemon.h"

static gboolean debug = FALSE;
static gboolean replace = FALSE;

static GOptionEntry entries[] =
{
  {
    "debug", 0, G_OPTION_FLAG_NONE,
    G_OPTION_ARG_NONE, &debug,
    N_("Enable debugging code"),
    NULL
  },
  {
    "replace", 'r', G_OPTION_FLAG_NONE,
    G_OPTION_ARG_NONE, &replace,
    N_("Replace a currently running application"),
    NULL
  },
  {
    NULL
  }
};

static gboolean
parse_arguments (int    *argc,
                 char ***argv)
{
  GOptionContext *context;
  GOptionGroup *gtk_group;
  GError *error;

  context = g_option_context_new (NULL);
  gtk_group = gtk_get_option_group (FALSE);

  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gtk_group);

  error = NULL;
  if (g_option_context_parse (context, argc, argv, &error) == FALSE)
    {
      g_warning ("Failed to parse command line arguments: %s", error->message);
      g_error_free (error);

      return FALSE;
    }

  if (debug)
    g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  NdDaemon *daemon;

  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  if (!parse_arguments (&argc, &argv))
    return EXIT_FAILURE;

  daemon = nd_daemon_new (replace);

  gtk_main ();

  g_object_unref (daemon);

  return EXIT_SUCCESS;
}
