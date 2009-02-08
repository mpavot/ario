/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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

#include "preferences/ario-playlist-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "ario-debug.h"
#include "playlist/ario-playlist-manager.h"

static void ario_playlist_preferences_sync_playlist (ArioPlaylistPreferences *playlist_preferences);
G_MODULE_EXPORT void ario_playlist_preferences_playlist_mode_changed_cb (GtkComboBox *combobox,
                                                                             ArioPlaylistPreferences *playlist_preferences);

struct ArioPlaylistPreferencesPrivate
{
        GtkWidget *playlist_combobox;
        GtkWidget *vbox;
        GtkWidget *config;
};

#define ARIO_PLAYLIST_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PLAYLIST_PREFERENCES, ArioPlaylistPreferencesPrivate))
G_DEFINE_TYPE (ArioPlaylistPreferences, ario_playlist_preferences, GTK_TYPE_VBOX)

static void
ario_playlist_preferences_class_init (ArioPlaylistPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioPlaylistPreferencesPrivate));
}

static void
ario_playlist_preferences_init (ArioPlaylistPreferences *playlist_preferences)
{
        ARIO_LOG_FUNCTION_START
        playlist_preferences->priv = ARIO_PLAYLIST_PREFERENCES_GET_PRIVATE (playlist_preferences);
}

GtkWidget *
ario_playlist_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistPreferences *playlist_preferences;
        GladeXML *xml;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        GSList *playlist_modes;
        ArioPlaylistMode *playlist_mode;

        playlist_preferences = g_object_new (TYPE_ARIO_PLAYLIST_PREFERENCES, NULL);

        g_return_val_if_fail (playlist_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "playlist-prefs.glade",
                                "playlist_vbox",
                                playlist_preferences);

        playlist_preferences->priv->playlist_combobox = 
                glade_xml_get_widget (xml, "playlist_combobox");
        playlist_preferences->priv->vbox = 
                glade_xml_get_widget (xml, "vbox");

        rb_glade_boldify_label (xml, "playlist_label");

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        playlist_modes = ario_playlist_manager_get_modes (ario_playlist_manager_get_instance ());
        for (; playlist_modes; playlist_modes = g_slist_next (playlist_modes)) {
                playlist_mode = playlist_modes->data;
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, ario_playlist_mode_get_name (playlist_mode),
                                    1, ario_playlist_mode_get_id (playlist_mode),
                                    -1);
        }
        gtk_combo_box_set_model (GTK_COMBO_BOX (playlist_preferences->priv->playlist_combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (playlist_preferences->priv->playlist_combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (playlist_preferences->priv->playlist_combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (playlist_preferences->priv->playlist_combobox), renderer,
                                        "text", 0, NULL);

        ario_playlist_preferences_sync_playlist (playlist_preferences);

        gtk_box_pack_start (GTK_BOX (playlist_preferences), glade_xml_get_widget (xml, "playlist_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (playlist_preferences);
}

static void
ario_playlist_preferences_sync_playlist (ArioPlaylistPreferences *playlist_preferences)
{
        ARIO_LOG_FUNCTION_START
        gchar *id;
        int i = 0;
        GSList *playlist_modes;
        ArioPlaylistMode *playlist_mode;

        id = ario_conf_get_string (PREF_PLAYLIST_MODE, PREF_PLAYLIST_MODE_DEFAULT);
        playlist_modes = ario_playlist_manager_get_modes (ario_playlist_manager_get_instance ());
        gtk_combo_box_set_active (GTK_COMBO_BOX (playlist_preferences->priv->playlist_combobox), 0);
        for (; playlist_modes; playlist_modes = g_slist_next (playlist_modes)) {
                playlist_mode = playlist_modes->data;
                if (!strcmp (ario_playlist_mode_get_id (playlist_mode), id)) {
                        gtk_combo_box_set_active (GTK_COMBO_BOX (playlist_preferences->priv->playlist_combobox), i);
                        break;
                }
                ++i;
        }
        g_free (id);
}

void
ario_playlist_preferences_playlist_mode_changed_cb (GtkComboBox *combobox,
                                                        ArioPlaylistPreferences *playlist_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        gchar *id;
        ArioPlaylistMode *playlist_mode;

        gtk_combo_box_get_active_iter (combobox, &iter);

        gtk_tree_model_get (gtk_combo_box_get_model (combobox), &iter,
                            1, &id, -1);

        ario_conf_set_string (PREF_PLAYLIST_MODE, id);

        if (playlist_preferences->priv->config) {
                gtk_container_remove (GTK_CONTAINER (playlist_preferences->priv->vbox),
                                      playlist_preferences->priv->config);
                playlist_preferences->priv->config = NULL;
        }
        playlist_mode = ario_playlist_manager_get_mode_from_id (ario_playlist_manager_get_instance (),
                                                                id);
        if (playlist_mode) {
                playlist_preferences->priv->config = ario_playlist_mode_get_config (playlist_mode);
                if (playlist_preferences->priv->config) {
                        gtk_box_pack_end (GTK_BOX (playlist_preferences->priv->vbox),
                                          playlist_preferences->priv->config,
                                          TRUE, TRUE, 0);
                        gtk_widget_show_all (playlist_preferences->priv->config);
                }
        }
        g_free (id);
}
