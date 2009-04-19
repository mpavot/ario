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
#include <string.h>

#include "ario-debug.h"
#include "covers/ario-cover-manager.h"
#include "lib/ario-conf.h"
#include "lib/gtk-builder-helpers.h"
#include "preferences/ario-preferences.h"

static void ario_cover_preferences_sync_cover (ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                                        ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_automatic_check_changed_cb (GtkCheckButton *butt,
                                                                        ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_amazon_country_changed_cb (GtkComboBox *combobox,
                                                                       ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_top_button_cb (GtkWidget *widget,
                                                           ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_up_button_cb (GtkWidget *widget,
                                                          ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_down_button_cb (GtkWidget *widget,
                                                            ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_bottom_button_cb (GtkWidget *widget,
                                                              ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_cover_toggled_cb (GtkCellRendererToggle *cell,
                                                              gchar *path_str,
                                                              ArioCoverPreferences *cover_preferences);

/* Private attributes */
struct ArioCoverPreferencesPrivate
{
        GtkWidget *covertree_check;
        GtkWidget *automatic_check;
        GtkWidget *amazon_country;

        GtkListStore *covers_model;
        GtkTreeSelection *covers_selection;
};

/* Tree model columns */
enum
{
        ENABLED_COLUMN,
        NAME_COLUMN,
        ID_COLUMN,
        N_COLUMN
};

#define ARIO_COVER_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_COVER_PREFERENCES, ArioCoverPreferencesPrivate))
G_DEFINE_TYPE (ArioCoverPreferences, ario_cover_preferences, GTK_TYPE_VBOX)

static void
ario_cover_preferences_class_init (ArioCoverPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioCoverPreferencesPrivate));
}

static void
ario_cover_preferences_init (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        cover_preferences->priv = ARIO_COVER_PREFERENCES_GET_PRIVATE (cover_preferences);
}

GtkWidget *
ario_cover_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START;
        GtkBuilder *builder;
        ArioCoverPreferences *cover_preferences;
        GtkWidget *covers_treeview;

        cover_preferences = g_object_new (TYPE_ARIO_COVER_PREFERENCES, NULL);

        g_return_val_if_fail (cover_preferences->priv != NULL, NULL);

        /* Generate UI using GtkBuilder */
        builder = gtk_builder_helpers_new (UI_PATH "cover-prefs.ui",
                                           cover_preferences);

        /* Get pointers to various widgets */
        cover_preferences->priv->covertree_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "covertree_checkbutton"));
        cover_preferences->priv->automatic_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "automatic_checkbutton"));
        cover_preferences->priv->amazon_country =
                GTK_WIDGET (gtk_builder_get_object (builder, "amazon_country_combobox"));
        cover_preferences->priv->covers_model =
                GTK_LIST_STORE (gtk_builder_get_object (builder, "covers_model"));
        covers_treeview =
                GTK_WIDGET (gtk_builder_get_object (builder, "covers_treeview"));

        /* Set style for labels */
        gtk_builder_helpers_boldify_label (builder, "cover_frame_label");
        gtk_builder_helpers_boldify_label (builder, "cover_sources_frame_label");

        /* Get model selection */
        cover_preferences->priv->covers_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (covers_treeview));
        gtk_tree_selection_set_mode (cover_preferences->priv->covers_selection,
                                     GTK_SELECTION_BROWSE);

        /* Synchronize widgets with configuration */
        ario_cover_preferences_sync_cover (cover_preferences);

        gtk_box_pack_start (GTK_BOX (cover_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "covers_vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);
        return GTK_WIDGET (cover_preferences);
}

static void
ario_cover_preferences_sync_cover_providers (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverProvider *cover_provider;
        GSList *providers;
        GSList *tmp;
        GtkTreeIter iter;
        gchar *id = NULL;
        gchar *tmp_id;
        GtkTreeModel *model = GTK_TREE_MODEL (cover_preferences->priv->covers_model);

        /* Remember id of currently selected row */
        if (gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                             &model,
                                             &iter)) {
                gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
        }

        /* Empty list */
        gtk_list_store_clear (cover_preferences->priv->covers_model);

        /* Get all covers providers */
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        /* For each covers provider */
        for (tmp = providers; tmp; tmp = g_slist_next (tmp)) {
                cover_provider = (ArioCoverProvider *) tmp->data;

                /* Append cover provider to tree */
                gtk_list_store_append (cover_preferences->priv->covers_model, &iter);
                gtk_list_store_set (cover_preferences->priv->covers_model, &iter,
                                    ENABLED_COLUMN, ario_cover_provider_is_active (cover_provider),
                                    NAME_COLUMN, ario_cover_provider_get_name (cover_provider),
                                    ID_COLUMN, ario_cover_provider_get_id (cover_provider),
                                    -1);
        }

        /* Select again previously selected row */
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

static gboolean
ario_cover_preferences_sync_cover_foreach (GtkTreeModel *model,
                                           GtkTreePath *path,
                                           GtkTreeIter *iter,
                                           ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        gchar *country;

        /* Get country of current row */
        gtk_tree_model_get (model, iter,
                            0, &country,
                            -1);

        if (!strcmp (country, ario_conf_get_string (PREF_COVER_AMAZON_COUNTRY, PREF_COVER_AMAZON_COUNTRY_DEFAULT))) {
                /* Row of current country foud: activate this row */
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (cover_preferences->priv->amazon_country), iter);
                g_free (country);
                /* Stop iterations */
                return TRUE;
        }
        g_free (country);

        /* Continue iterations */
        return FALSE;
}

static void
ario_cover_preferences_sync_cover (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;

        /* Activate covertre_check */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->covertree_check),
                                      !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));

        /* Activate automatic_check */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->automatic_check),
                                      ario_conf_get_boolean (PREF_AUTOMATIC_GET_COVER, PREF_AUTOMATIC_GET_COVER_DEFAULT));

        /* Select activate amazon country */
        gtk_tree_model_foreach (gtk_combo_box_get_model (GTK_COMBO_BOX (cover_preferences->priv->amazon_country)),
                                (GtkTreeModelForeachFunc) ario_cover_preferences_sync_cover_foreach,
                                cover_preferences);

        /* Synchonize covers providers */
        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                   ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        /* Update configuration */
        ario_conf_set_boolean (PREF_COVER_TREE_HIDDEN,
                               !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->covertree_check)));
}

void
ario_cover_preferences_automatic_check_changed_cb (GtkCheckButton *butt,
                                                   ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        /* Update configuration */
        ario_conf_set_boolean (PREF_AUTOMATIC_GET_COVER,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->automatic_check)));
}

void
ario_cover_preferences_amazon_country_changed_cb (GtkComboBox *combobox,
                                                  ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        gchar *country;

        /* Get country of activated row */
        treemodel = gtk_combo_box_get_model (combobox);
        gtk_combo_box_get_active_iter (combobox,
                                       &iter);
        gtk_tree_model_get (treemodel, &iter,
                            0, &country,
                            -1);

        /* Update configuration */
        ario_conf_set_string (PREF_COVER_AMAZON_COUNTRY,
                              country);
        g_free (country);
}

void
ario_cover_preferences_top_button_cb (GtkWidget *widget,
                                      ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;

        /* Stop here if no row is selected */
        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;

        /* Get ID of selected provider */
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);

        /* Get selected covers provider */
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);

        /* Get list of covers providers */
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        /* Remove provider from list */
        providers = g_slist_remove (providers, cover_provider);

        /* Add provider to head of list */
        providers = g_slist_prepend (providers, cover_provider);

        ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

        /* Synchronize tree */
        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_up_button_cb (GtkWidget *widget,
                                     ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;
        gint index;

        /* Stop here if no row is selected */
        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;

        /* Get ID of selected provider */
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);

        /* Get selected covers provider */
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);

        /* Get list of covers providers */
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        /* Get index of selected provider */
        index = g_slist_index (providers, cover_provider);
        if (index > 0) {
                /* Remove provider from list */
                providers = g_slist_remove (providers, cover_provider);

                /* Insert provider at previous position */
                providers = g_slist_insert (providers, cover_provider, index - 1);
                ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

                /* Synchronize tree */
                ario_cover_preferences_sync_cover_providers (cover_preferences);
        }
}

void
ario_cover_preferences_down_button_cb (GtkWidget *widget,
                                       ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;
        gint index;

        /* Stop here if no row is selected */
        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;

        /* Get ID of selected provider */
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);

        /* Get selected covers provider */
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);

        /* Get list of covers providers */
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        /* Get index of selected provider */
        index = g_slist_index (providers, cover_provider);

        /* Remove provider from list */
        providers = g_slist_remove (providers, cover_provider);

        /* Insert provider at next position */
        providers = g_slist_insert (providers, cover_provider, index + 1);
        ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

        /* Synchronize tree */
        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_bottom_button_cb (GtkWidget *widget,
                                         ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GtkTreeModel *model;
        gchar *id;
        ArioCoverProvider *cover_provider;
        GSList *providers;

        /* Stop here if no row is selected */
        if (!gtk_tree_selection_get_selected (cover_preferences->priv->covers_selection,
                                              &model,
                                              &iter))
                return;

        /* Get ID of selected provider */
        gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);

        /* Get selected covers provider */
        cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);
        g_free (id);

        /* Get list of covers providers */
        providers = ario_cover_manager_get_providers (ario_cover_manager_get_instance ());

        /* Remove provider from list */
        providers = g_slist_remove (providers, cover_provider);

        /* Add provider to end of list */
        providers = g_slist_append (providers, cover_provider);

        ario_cover_manager_set_providers (ario_cover_manager_get_instance (), providers);

        /* Synchronize tree */
        ario_cover_preferences_sync_cover_providers (cover_preferences);
}

void
ario_cover_preferences_cover_toggled_cb (GtkCellRendererToggle *cell,
                                         gchar *path_str,
                                         ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START;
        gboolean state;
        gchar *id;
        GtkTreeIter iter;
        GtkTreeModel *model = GTK_TREE_MODEL (cover_preferences->priv->covers_model);
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
        ArioCoverProvider *cover_provider;

        if (gtk_tree_model_get_iter (model, &iter, path)) {
                /* Get info from toggled row */
                gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state, ID_COLUMN, &id, -1);
                state = !state;

                /* Get covers provider corresponding to toggled row */
                cover_provider = ario_cover_manager_get_provider_from_id (ario_cover_manager_get_instance (), id);

                /* (De)activate cover provider */
                ario_cover_provider_set_active (cover_provider, state);
                g_free (id);

                /* Update tree */
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, ENABLED_COLUMN, state, -1);
        }
        gtk_tree_path_free (path);
}
