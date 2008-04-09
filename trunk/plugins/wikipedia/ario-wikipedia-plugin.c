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
#include "lib/ario-conf.h"

static void ario_wikipedia_cmd_find_artist (GtkAction *action,
                                            ArioWikipediaPlugin *plugin);
static void ario_wikipedia_plugin_sync_mpd (ArioWikipediaPlugin *plugin,
                                            ArioMpd *mpd);
static void ario_wikipedia_plugin_mpd_state_changed_cb (ArioMpd *mpd,
                                                        ArioWikipediaPlugin *plugin);
#define ARIO_WIKIPEDIA_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_WIKIPEDIA_PLUGIN, ArioWikipediaPluginPrivate))

#define CONF_WIKIPEDIA_LANGUAGE         "plugins/wikipedia-language"

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
        GtkActionGroup *actiongroup;
};

static const char *wikipedia_languages[] = {
        "Català", "ca",
        "Deutsch", "de",
        "English", "en",
        "Español", "es",
        "Français", "fr",
        "Italiano", "it",
        "Nederlands", "nl",
        "日本語", "ja",
        "Norsk (bokmål)", "no",
        "Polski", "pl",
        "Português", "pt",
        "Русский", "ru",
        "Română", "ro",
        "Suomi", "fi",
        "Svenska", "sv",
        "Türkçe", "tr",
        "Volapük", "vo",
        "中文", "zh",
        NULL
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
	ArioWikipediaPlugin *pi = ARIO_WIKIPEDIA_PLUGIN (plugin);
        static gboolean is_loaded = FALSE;
        gchar *file;
        ArioMpd *mpd;

	g_object_get (shell, "ui-manager", &uimanager, NULL);
        file = ario_plugin_find_file ("wikipedia-ui.xml");
        if (file) {
                pi->priv->ui_merge_id = gtk_ui_manager_add_ui_from_file (uimanager,
                                                                         file, NULL);
                g_free (file);
        }
	g_object_unref (uimanager);

        if (!is_loaded) {
	        g_object_get (shell, "action-group", &pi->priv->actiongroup, NULL);
                gtk_action_group_add_actions (pi->priv->actiongroup,
                                              ario_wikipedia_actions,
                                              G_N_ELEMENTS (ario_wikipedia_actions), pi);
	        g_object_unref (pi->priv->actiongroup);

                is_loaded = TRUE;
        }

	g_object_get (shell, "mpd", &mpd, NULL);
        g_signal_connect_object (G_OBJECT (mpd),
                                 "state_changed", G_CALLBACK (ario_wikipedia_plugin_mpd_state_changed_cb),
                                 pi, 0);
        ario_wikipedia_plugin_sync_mpd (pi, mpd);
        g_object_unref (mpd);

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
configure_dialog_response_cb (GtkWidget *widget,
			      gint response,
			      GtkWidget *dialog)
{
        gtk_widget_destroy (dialog);
}

static void
combobox_changed_cb (GtkComboBox *widget,
                     gpointer null)
{
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

        ario_conf_set_string (CONF_WIKIPEDIA_LANGUAGE, 
                              wikipedia_languages[2*i + 1]);
}

static GtkWidget *
impl_create_configure_dialog (ArioPlugin *plugin)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *combobox;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;
        char *current_language;

        dialog = gtk_dialog_new_with_buttons (_("Wikipedia Plugin - Configuration"),
					      NULL,
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE,
					      GTK_RESPONSE_CLOSE,
					      NULL);

        hbox = gtk_hbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
        label = gtk_label_new (_("Wikipedia language :"));

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        for (i = 0; wikipedia_languages[2*i]; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, wikipedia_languages[2*i],
                                    1, wikipedia_languages[2*i+1],
                                    -1);
        }

        combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0, NULL);

        current_language = ario_conf_get_string (CONF_WIKIPEDIA_LANGUAGE, "en");
        for (i = 0; wikipedia_languages[2*i]; ++i) {
                if (!strcmp (wikipedia_languages[2*i+1], current_language)) {
                        gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), i);
                        break;
                }
                gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
        }
        g_free (current_language);

	gtk_box_pack_start_defaults (GTK_BOX (hbox),
				     label);
	gtk_box_pack_start_defaults (GTK_BOX (hbox),
				     combobox);

	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				     hbox);

	g_signal_connect (combobox,
			  "changed",
			  G_CALLBACK (combobox_changed_cb),
			  NULL);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (configure_dialog_response_cb),
			  dialog);

        gtk_widget_show_all (dialog);

        return dialog;
}

static void
ario_wikipedia_plugin_class_init (ArioWikipediaPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        object_class->finalize = ario_wikipedia_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
        plugin_class->create_configure_dialog = impl_create_configure_dialog;

        g_type_class_add_private (object_class, sizeof (ArioWikipediaPluginPrivate));
}

static void
ario_wikipedia_cmd_find_artist (GtkAction *action,
                                ArioWikipediaPlugin *plugin)
{
        ArioMpd *mpd;
        gchar *artist;
        gchar *uri;
        gchar *language;

        g_return_if_fail (ARIO_IS_WIKIPEDIA_PLUGIN (plugin));

	g_object_get (plugin->priv->shell, "mpd", &mpd, NULL);
        artist = g_strdup (ario_mpd_get_current_artist (mpd));
        g_object_unref (mpd);
        if (artist) {
                ario_util_string_replace (&artist, " ", "_");
                ario_util_string_replace (&artist, "/", "_");
                
                language = ario_conf_get_string (CONF_WIKIPEDIA_LANGUAGE, "en");
                uri = g_strdup_printf ("http://%s.wikipedia.org/wiki/%s", language, artist);
                g_free (language);
                g_free (artist);
                ario_util_load_uri (uri);
                g_free (uri);
        }
}

static void
ario_wikipedia_plugin_sync_mpd (ArioWikipediaPlugin *plugin,
                                ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        gboolean is_playing;
        GtkAction *action;

        is_playing = (ario_mpd_is_connected (mpd)
                      && ((ario_mpd_get_current_state (mpd) == MPD_STATUS_STATE_PLAY)
                           || (ario_mpd_get_current_state (mpd) == MPD_STATUS_STATE_PAUSE)));

        action = gtk_action_group_get_action (plugin->priv->actiongroup,
                                              "ToolWikipedia");
        gtk_action_set_sensitive (action, is_playing);
}

static void
ario_wikipedia_plugin_mpd_state_changed_cb (ArioMpd *mpd,
                                            ArioWikipediaPlugin *plugin)
{
        ARIO_LOG_FUNCTION_START

        ario_wikipedia_plugin_sync_mpd (plugin, mpd);
}

