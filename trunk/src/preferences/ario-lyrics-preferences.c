/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
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

#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-lyrics-preferences.h"
#include "preferences/ario-preferences.h"
#include "lyrics/ario-lyrics-manager.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "ario-avahi.h"
#include "ario-debug.h"

static void ario_lyrics_preferences_class_init (ArioLyricsPreferencesClass *klass);
static void ario_lyrics_preferences_init (ArioLyricsPreferences *lyrics_preferences);
static void ario_lyrics_preferences_finalize (GObject *object);
static void ario_lyrics_preferences_sync_lyrics_providers (ArioLyricsPreferences *lyrics_preferences);
G_MODULE_EXPORT void ario_lyrics_preferences_top_button_cb (GtkWidget *widget,
                                                            ArioLyricsPreferences *lyrics_preferences);
G_MODULE_EXPORT void ario_lyrics_preferences_up_button_cb (GtkWidget *widget,
                                                           ArioLyricsPreferences *lyrics_preferences);
G_MODULE_EXPORT void ario_lyrics_preferences_down_button_cb (GtkWidget *widget,
                                                             ArioLyricsPreferences *lyrics_preferences);
G_MODULE_EXPORT void ario_lyrics_preferences_bottom_button_cb (GtkWidget *widget,
                                                               ArioLyricsPreferences *lyrics_preferences);
static void ario_lyrics_preferences_lyrics_toggled_cb (GtkCellRendererToggle *cell,
                                                       gchar *path_str,
                                                       ArioLyricsPreferences *lyrics_preferences);


struct ArioLyricsPreferencesPrivate
{
        GtkWidget *lyrics_treeview;
        GtkListStore *lyrics_model;
        GtkTreeSelection *lyrics_selection;
};

enum
{
        ENABLED_COLUMN,
        NAME_COLUMN,
        ID_COLUMN,
        N_COLUMN
};

static GObjectClass *parent_class = NULL;

GType
ario_lyrics_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_lyrics_preferences_type = 0;

        if (ario_lyrics_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioLyricsPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_lyrics_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioLyricsPreferences),
                        0,
                        (GInstanceInitFunc) ario_lyrics_preferences_init
                };

                ario_lyrics_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                       "ArioLyricsPreferences",
                                                                       &our_info, 0);
        }

        return ario_lyrics_preferences_type;
}

static void
ario_lyrics_preferences_class_init (ArioLyricsPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_lyrics_preferences_finalize;
}

static void
ario_lyrics_preferences_init (ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        lyrics_preferences->priv = g_new0 (ArioLyricsPreferencesPrivate, 1);
}

GtkWidget *
ario_lyrics_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        ArioLyricsPreferences *lyrics_preferences;
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        lyrics_preferences = g_object_new (TYPE_ARIO_LYRICS_PREFERENCES, NULL);

        g_return_val_if_fail (lyrics_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "lyrics-prefs.glade",
                                "lyrics_vbox",
                                lyrics_preferences);

        lyrics_preferences->priv->lyrics_treeview = 
                glade_xml_get_widget (xml, "lyrics_treeview");

        rb_glade_boldify_label (xml, "lyrics_sources_frame_label");

        lyrics_preferences->priv->lyrics_model = gtk_list_store_new (N_COLUMN,
                                                                     G_TYPE_BOOLEAN,
                                                                     G_TYPE_STRING,
                                                                     G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (lyrics_preferences->priv->lyrics_treeview),
                                 GTK_TREE_MODEL (lyrics_preferences->priv->lyrics_model));
        renderer = gtk_cell_renderer_toggle_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Enabled"),
                                                           renderer,
                                                           "active", ENABLED_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 80);
        gtk_tree_view_append_column (GTK_TREE_VIEW (lyrics_preferences->priv->lyrics_treeview), column);
        g_signal_connect (GTK_OBJECT (renderer),
                          "toggled",
                          G_CALLBACK (ario_lyrics_preferences_lyrics_toggled_cb), lyrics_preferences);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", NAME_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (lyrics_preferences->priv->lyrics_treeview), column);

        lyrics_preferences->priv->lyrics_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (lyrics_preferences->priv->lyrics_treeview));
        gtk_tree_selection_set_mode (lyrics_preferences->priv->lyrics_selection,
                                     GTK_SELECTION_BROWSE);

        ario_lyrics_preferences_sync_lyrics_providers (lyrics_preferences);

        gtk_box_pack_start (GTK_BOX (lyrics_preferences), glade_xml_get_widget (xml, "lyrics_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));
        return GTK_WIDGET (lyrics_preferences);
}

static void
ario_lyrics_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsPreferences *lyrics_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_LYRICS_PREFERENCES (object));

        lyrics_preferences = ARIO_LYRICS_PREFERENCES (object);

        g_return_if_fail (lyrics_preferences->priv != NULL);

        g_free (lyrics_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_lyrics_preferences_sync_lyrics_providers (ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsProvider *lyrics_provider;
        GSList *providers;
        GSList *tmp;
        GtkTreeIter iter;
        gchar *id = NULL;
        gchar *tmp_id;
        GtkTreeModel *model = GTK_TREE_MODEL (lyrics_preferences->priv->lyrics_model);

        if (gtk_tree_selection_get_selected (lyrics_preferences->priv->lyrics_selection,
                                             &model,
                                             &iter)) {
                gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        }

        gtk_list_store_clear (lyrics_preferences->priv->lyrics_model);
        providers = ario_lyrics_manager_get_providers (ario_lyrics_manager_get_instance ());
        for (tmp = providers; tmp; tmp = g_slist_next (tmp)) {
                lyrics_provider = (ArioLyricsProvider *) tmp->data;
                gtk_list_store_append (lyrics_preferences->priv->lyrics_model, &iter);
                gtk_list_store_set (lyrics_preferences->priv->lyrics_model, &iter,
                                    ENABLED_COLUMN, ario_lyrics_provider_is_active (lyrics_provider),
                                    NAME_COLUMN, ario_lyrics_provider_get_name (lyrics_provider),
                                    ID_COLUMN, ario_lyrics_provider_get_id (lyrics_provider),
                                    -1);
        }

        if (id) {
                if (gtk_tree_model_get_iter_first (model, &iter)) {
                        do {
                                gtk_tree_model_get (model, &iter, ID_COLUMN, &tmp_id, -1);
                                if (!strcmp (id, tmp_id))
                                        gtk_tree_selection_select_iter (lyrics_preferences->priv->lyrics_selection, &iter);
                                g_free (tmp_id);
                        } while (gtk_tree_model_iter_next (model, &iter));
                }

                g_free (id);
        }
}

void
ario_lyrics_preferences_top_button_cb (GtkWidget *widget,
                                       ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioLyricsProvider *lyrics_provider;
        GSList *providers;

        if (!gtk_tree_selection_get_selected (lyrics_preferences->priv->lyrics_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        lyrics_provider = ario_lyrics_manager_get_provider_from_id (ario_lyrics_manager_get_instance (), id);
        g_free (id);
        providers = ario_lyrics_manager_get_providers (ario_lyrics_manager_get_instance ());

        providers = g_slist_remove (providers, lyrics_provider);
        providers = g_slist_prepend (providers, lyrics_provider);

        ario_lyrics_manager_set_providers (ario_lyrics_manager_get_instance (), providers);

        ario_lyrics_preferences_sync_lyrics_providers (lyrics_preferences);
}

void
ario_lyrics_preferences_up_button_cb (GtkWidget *widget,
                                      ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioLyricsProvider *lyrics_provider;
        GSList *providers;
        gint index;

        if (!gtk_tree_selection_get_selected (lyrics_preferences->priv->lyrics_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        lyrics_provider = ario_lyrics_manager_get_provider_from_id (ario_lyrics_manager_get_instance (), id);
        g_free (id);
        providers = ario_lyrics_manager_get_providers (ario_lyrics_manager_get_instance ());

        index = g_slist_index (providers, lyrics_provider);
        if (index > 0) {
                providers = g_slist_remove (providers, lyrics_provider);
                providers = g_slist_insert (providers, lyrics_provider, index - 1);
                ario_lyrics_manager_set_providers (ario_lyrics_manager_get_instance (), providers);

                ario_lyrics_preferences_sync_lyrics_providers (lyrics_preferences);
        }
}

void
ario_lyrics_preferences_down_button_cb (GtkWidget *widget,
                                        ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioLyricsProvider *lyrics_provider;
        GSList *providers;
        gint index;

        if (!gtk_tree_selection_get_selected (lyrics_preferences->priv->lyrics_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        lyrics_provider = ario_lyrics_manager_get_provider_from_id (ario_lyrics_manager_get_instance (), id);
        g_free (id);
        providers = ario_lyrics_manager_get_providers (ario_lyrics_manager_get_instance ());

        index = g_slist_index (providers, lyrics_provider);
        providers = g_slist_remove (providers, lyrics_provider);
        providers = g_slist_insert (providers, lyrics_provider, index + 1);
        ario_lyrics_manager_set_providers (ario_lyrics_manager_get_instance (), providers);

        ario_lyrics_preferences_sync_lyrics_providers (lyrics_preferences);
}

void
ario_lyrics_preferences_bottom_button_cb (GtkWidget *widget,
                                          ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioLyricsProvider *lyrics_provider;
        GSList *providers;

        if (!gtk_tree_selection_get_selected (lyrics_preferences->priv->lyrics_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        lyrics_provider = ario_lyrics_manager_get_provider_from_id (ario_lyrics_manager_get_instance (), id);
        g_free (id);
        providers = ario_lyrics_manager_get_providers (ario_lyrics_manager_get_instance ());

        providers = g_slist_remove (providers, lyrics_provider);
        providers = g_slist_append (providers, lyrics_provider);

        ario_lyrics_manager_set_providers (ario_lyrics_manager_get_instance (), providers);

        ario_lyrics_preferences_sync_lyrics_providers (lyrics_preferences);
}

static void
ario_lyrics_preferences_lyrics_toggled_cb (GtkCellRendererToggle *cell,
                                           gchar *path_str,
                                           ArioLyricsPreferences *lyrics_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean state;
        gchar *id;
        GtkTreeIter iter;
        GtkTreeModel *model = GTK_TREE_MODEL (lyrics_preferences->priv->lyrics_model);
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
        ArioLyricsProvider *lyrics_provider;

        if (gtk_tree_model_get_iter (model, &iter, path)) {
                gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state, ID_COLUMN, &id, -1);
                state = !state;
                lyrics_provider = ario_lyrics_manager_get_provider_from_id (ario_lyrics_manager_get_instance (), id);
                ario_lyrics_provider_set_active (lyrics_provider, state);
                g_free (id);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, ENABLED_COLUMN, state, -1);
        }
        gtk_tree_path_free (path);
}
