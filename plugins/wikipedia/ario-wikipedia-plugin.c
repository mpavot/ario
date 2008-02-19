/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include "ario-wikipedia-plugin.h"
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-mpd.h>
#include <ario-debug.h>
#include <ario-shell.h>
#include <ario-util.h>

static void ario_wikipedia_cmd_find_artist (GtkAction *action,
                                            ArioWikipediaPlugin *plugin);

#define ARIO_WIKIPEDIA_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_WIKIPEDIA_PLUGIN, ArioWikipediaPluginPrivate))

static GtkActionEntry ario_wikipedia_actions [] =
{
	{ "ToolWikipedia", GTK_STOCK_FIND, N_("Find artist on Wikipedia"), NULL,
	  N_("Find artist on Wikipedia"),
	  G_CALLBACK (ario_wikipedia_cmd_find_artist) }
};

struct _ArioWikipediaPluginPrivate
{
	guint ui_merge_id;
        ArioShell *shell;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioWikipediaPlugin, ario_wikipedia_plugin)

static void
ario_wikipedia_plugin_init (ArioWikipediaPlugin *plugin)
{
        plugin->priv = ARIO_WIKIPEDIA_PLUGIN_GET_PRIVATE (plugin);
}

static void
ario_wikipedia_plugin_finalize (GObject *object)
{
        G_OBJECT_CLASS (ario_wikipedia_plugin_parent_class)->finalize (object);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
	GtkUIManager *uimanager;
        GtkActionGroup *actiongroup;
	ArioWikipediaPlugin *pi = ARIO_WIKIPEDIA_PLUGIN (plugin);
        static gboolean is_loaded = FALSE;
        GdkPixbuf *pb;

	g_object_get (shell, "ui-manager", &uimanager, NULL);
        pi->priv->ui_merge_id = gtk_ui_manager_add_ui_from_file (uimanager,
                                                                 UI_PATH "wikipedia-ui.xml", NULL);
	g_object_unref (uimanager);

        if (!is_loaded) {
        	int icon_size;
	        g_object_get (shell, "action-group", &actiongroup, NULL);
                gtk_action_group_add_actions (actiongroup,
                                              ario_wikipedia_actions,
                                              G_N_ELEMENTS (ario_wikipedia_actions), pi);
	        g_object_unref (actiongroup);

                pb = gdk_pixbuf_new_from_file (PLUGINDIR "wikipedia.png",
                                               NULL);
        	gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &icon_size, NULL);
	        gtk_icon_theme_add_builtin_icon ("wikipedia",
					         icon_size,
					         pb);

                is_loaded = TRUE;
        }

        pi->priv->shell = shell;
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
	GtkUIManager *uimanager;

	ArioWikipediaPlugin *pi = ARIO_WIKIPEDIA_PLUGIN (plugin);

	g_object_get (shell, "ui-manager", &uimanager, NULL);
	gtk_ui_manager_remove_ui (uimanager, pi->priv->ui_merge_id);
	g_object_unref (uimanager);
}

static void
ario_wikipedia_plugin_class_init (ArioWikipediaPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        object_class->finalize = ario_wikipedia_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
        
        g_type_class_add_private (object_class, sizeof (ArioWikipediaPluginPrivate));
}

static void
ario_wikipedia_cmd_find_artist (GtkAction *action,
                                ArioWikipediaPlugin *plugin)
{
        ArioMpd *mpd;
        gchar *artist;
        gchar *command;

        g_return_if_fail (ARIO_IS_WIKIPEDIA_PLUGIN (plugin));

	g_object_get (plugin->priv->shell, "mpd", &mpd, NULL);
        artist = g_strdup (ario_mpd_get_current_artist (mpd));
        g_object_unref (mpd);
        if (artist) {
                ario_util_string_replace (&artist, " ", "_");
                ario_util_string_replace (&artist, "/", "_");
                /* TODO : Change the locale in configuration */
                command = g_strdup_printf("x-www-browser http://%s.wikipedia.org/wiki/%s", "en", artist);
                g_free (artist);
                g_spawn_command_line_sync (command, NULL, NULL, NULL, NULL);
                g_free (command);
        }
}
