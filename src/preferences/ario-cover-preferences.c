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

#include "preferences/ario-cover-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-preferences.h"
#include "covers/ario-cover-manager.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "ario-debug.h"

static void ario_cover_preferences_sync_cover (ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                                        ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_automatic_check_changed_cb (GtkCheckButton *butt,
                                                                        ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_amazon_country_changed_cb (GtkComboBoxEntry *combobox,
                                                                       ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_top_button_cb (GtkWidget *widget,
                                                           ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_up_button_cb (GtkWidget *widget,
                                                          ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_down_button_cb (GtkWidget *widget,
                                                            ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_bottom_button_cb (GtkWidget *widget,
                                                              ArioCoverPreferences *cover_preferences);
static void ario_cover_preferences_cover_toggled_cb (GtkCellRendererToggle *cell,
                                                     gchar *path_str,
                                                     ArioCoverPreferences *cover_preferences);


struct ArioCoverPreferencesPrivate
{
        GtkWidget *covertree_check;
        GtkWidget *automatic_check;
        GtkWidget *amazon_country;

        GtkWidget *covers_treeview;
        GtkListStore *covers_model;
        GtkTreeSelection *covers_selection;
};

enum
{
        ENABLED_COLUMN,
        NAME_COLUMN,
        ID_COLUMN,
        N_COLUMN
};

static const char *amazon_countries[] = {
        "com",
        "fr",
        "de",
        "uk",
        "ca",
        "jp",
        NULL
};

#define ARIO_COVER_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_COVER_PREFERENCES, ArioCoverPreferencesPrivate))
G_DEFINE_TYPE (ArioCoverPreferences, ario_cover_preferences, GTK_TYPE_VBOX)

static void
ario_cover_preferences_class_init (ArioCoverPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioCoverPreferencesPrivate));
}

static void
ario_cover_preferences_init (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        cover_preferences->priv = ARIO_COVER_PREFERENCES_GET_PRIVATE (cover_preferences);
}

GtkWidget *
ario_cover_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        ArioCoverPreferences *cover_preferences;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;
        GtkTreeViewColumn *column;

        cover_preferences = g_object_new (TYPE_ARIO_COVER_PREFERENCES, NULL);

        g_return_val_if_fail (cover_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "cover-prefs.glade",
                                "covers_vbox",
                                cover_preferences);

        cover_preferences->priv->covertree_check =
                glade_xml_get_widget (xml, "covertree_checkbutton");
        cover_preferences->priv->automatic_check =
                glade_xml_get_widget (xml, "automatic_checkbutton");
        cover_preferences->priv->amazon_country =
                glade_xml_get_widget (xml, "amazon_country_combobox");
        cover_preferences->priv->covers_treeview = 
                glade_xml_get_widget (xml, "covers_treeview");

        rb_glade_boldify_label (xml, "cover_frame_label");
        rb_glade_boldify_label (xml, "cover_sources_frame_label");

        list_store = gtk_list_store_new (1, G_TYPE_STRING);

        for (i = 0; amazon_countries[i]; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, amazon_countries[i],
                                    -1);
        }

        gtk_combo_box_set_model (GTK_COMBO_BOX (cover_preferences->priv->amazon_country),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (cover_preferences->priv->amazon_country));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cover_preferences->priv->amazon_country), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cover_preferences->priv->amazon_country), renderer,
                                        "text", 0, NULL);

        cover_preferences->priv->covers_model = gtk_list_store_new (N_COLUMN,
                                                                    G_TYPE_BOOLEAN,
                                                                    G_TYPE_STRING,
                                                                    G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (cover_preferences->priv->covers_treeview),
                                 GTK_TREE_MODEL (cover_preferences->priv->covers_model));
        renderer = gtk_cell_renderer_toggle_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Enabled"),
                                                           renderer,
                                                           "active", ENABLED_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 80);
        gtk_tree_view_append_column (GTK_TREE_VIEW (cover_preferences->priv->covers_treeview), column);
        g_signal_connect (GTK_OBJECT (renderer),
                          "toggled",
                          G_CALLBACK (ario_cover_preferences_cover_toggled_cb), cover_preferences);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", NAME_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (cover_preferences->priv->covers_treeview), column);

        cover_preferences->priv->covers_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cover_preferences->priv->covers_treeview));
        gtk_tree_selection_set_mode (cover_preferences->priv->covers_selection,
                                     GTK_SELECTION_BROWSE);

        ario_cover_preferences_sync_cover (cover_preferences);

        gtk_box_pack_start (GTK_BOX (cover_preferences), glade_xml_get_widget (xml, "covers_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));
        return GTK_WIDGET (cover_preferences);
}

static void
ario_cover_preferences_sync_cover_providers (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverProvider *cover_provider;
        GSList *providers;
        GSList *tmp;
        GtkTreeIter iter;
        gchar *id = NULL;
        gchar *tmp_id;
        GtkTreeModel *model = GTK_TREE_MODEL (cover_preferences->priv->covers_model);

        if (gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                             &model,
                                             &iter)) {
                gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        }

        gtk_list_store_clear (cover_preferences->priv->covers_model);
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());
        for (tmp = providers; tmp; tmp = g_slist_next (tmp)) {
                cover_provider = (ArioCoverProvider *) tmp->data;
                gtk_list_store_append (cover_preferences->priv->covers_model, &iter);
                gtk_list_store_set (cover_preferences->priv->covers_model, &iter,
                                    ENABLED_COLUMN, ario_cover_provider_is_active (cover_provider),
                                    NAME_COLUMN, ario_cover_provider_get_name (cover_provider),
                                    ID_COLUMN, ario_cover_provider_get_id (cover_provider),
                                    -1);
        }

        if (id) {
                if (gtk_tree_model_get_iter_first (model, &iter)) {
                        do {
                                gtk_tree_model_get (model, &iter, ID_COLUMN, &tmp_id, -1);
                                if (!strcmp (id, tmp_id))
                                        gtk_tree_selection_select_iter (cover_preferences->priv->covers_selection, &iter);
                                g_free (tmp_id);
                        } while (gtk_tree_model_iter_next (model, &iter));
                }

                g_free (id);
        }
}

static void
ario_cover_preferences_sync_cover (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;
        char *current_country;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->covertree_check), 
                                      !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->automatic_check), 
                                      ario_conf_get_boolean (PREF_AUTOMATIC_GET_COVER, PREF_AUTOMATIC_GET_COVER_DEFAULT));

        current_country = ario_conf_get_string (PREF_COVER_AMAZON_COUNTRY, PREF_COVER_AMAZON_COUNTRY_DEFAULT);
        for (i = 0; amazon_countries[i]; ++i) {
                if (!strcmp (amazon_countries[i], current_country)) {
                        gtk_combo_box_set_active (GTK_COMBO_BOX (cover_preferences->priv->amazon_country), i);
                        break;
                }
                gtk_combo_box_set_active (GTK_COMBO_BOX (cover_preferences->priv->amazon_country), 0);
        }
        g_free (current_country);

        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                   ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_COVER_TREE_HIDDEN,
                               !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->covertree_check)));
}

void
ario_cover_preferences_automatic_check_changed_cb (GtkCheckButton *butt,
                                                   ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_AUTOMATIC_GET_COVER,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->automatic_check)));
}

void
ario_cover_preferences_amazon_country_changed_cb (GtkComboBoxEntry *combobox,
                                                  ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (cover_preferences->priv->amazon_country));

        ario_conf_set_string (PREF_COVER_AMAZON_COUNTRY, 
                              amazon_countries[i]);
}

void
ario_cover_preferences_top_button_cb (GtkWidget *widget,
                                      ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;

        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        providers = g_slist_remove (providers, cover_provider);
        providers = g_slist_prepend (providers, cover_provider);

        ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_up_button_cb (GtkWidget *widget,
                                     ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;
        gint index;

        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        index = g_slist_index (providers, cover_provider);
        if (index > 0) {
                providers = g_slist_remove (providers, cover_provider);
                providers = g_slist_insert (providers, cover_provider, index - 1);
                ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

                ario_cover_preferences_sync_cover_providers (cover_preferences);
        }
}

void
ario_cover_preferences_down_button_cb (GtkWidget *widget,
                                       ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;
        gint index;

        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        index = g_slist_index (providers, cover_provider);
        providers = g_slist_remove (providers, cover_provider);
        providers = g_slist_insert (providers, cover_provider, index + 1);
        ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_bottom_button_cb (GtkWidget *widget,
                                         ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;

        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        providers = g_slist_remove (providers, cover_provider);
        providers = g_slist_append (providers, cover_provider);

        ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

static void
ario_cover_preferences_cover_toggled_cb (GtkCellRendererToggle *cell,
                                         gchar *path_str,
                                         ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean state;
        gchar *id;
        GtkTreeIter iter;
        GtkTreeModel *model = GTK_TREE_MODEL (cover_preferences->priv->covers_model);
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
        ArioCoverProvider *cover_provider;

        if (gtk_tree_model_get_iter (model, &iter, path)) {
                gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state, ID_COLUMN, &id, -1);
                state = !state;
                cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
                ario_cover_provider_set_active (cover_provider, state);
                g_free (id);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, ENABLED_COLUMN, state, -1);
        }
        gtk_tree_path_free (path);
}
