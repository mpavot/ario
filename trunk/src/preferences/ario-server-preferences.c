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

#include "preferences/ario-server-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-preferences.h"
#include "lib/ario-conf.h"
#include "lib/gtk-builder-helpers.h"
#include "ario-debug.h"
#include "servers/ario-server.h"

static void ario_server_preferences_sync_server (ArioServerPreferences *server_preferences);
static void ario_server_preferences_server_changed_cb(ArioServer *server,
                                                      ArioServerPreferences *server_preferences);
G_MODULE_EXPORT void ario_server_preferences_crossfadetime_changed_cb (GtkWidget *widget,
                                                                       ArioServerPreferences *server_preferences);
G_MODULE_EXPORT void ario_server_preferences_crossfade_changed_cb (GtkWidget *widget,
                                                                   ArioServerPreferences *server_preferences);
G_MODULE_EXPORT void ario_server_preferences_updatedb_button_cb (GtkWidget *widget,
                                                                 ArioServerPreferences *server_preferences);
G_MODULE_EXPORT void ario_server_preferences_update_changed_cb (GtkWidget *widget,
                                                                ArioServerPreferences *server_preferences);
G_MODULE_EXPORT void ario_server_preferences_stopexit_changed_cb (GtkWidget *widget,
                                                                  ArioServerPreferences *server_preferences);


struct ArioServerPreferencesPrivate
{
        GtkWidget *crossfade_checkbutton;
        GtkWidget *crossfadetime_spinbutton;
        GtkWidget *updatedb_label;
        GtkWidget *updatedb_button;
        GtkWidget *outputs_treeview;
        GtkListStore *outputs_model;

        GtkWidget *update_checkbutton;
        GtkWidget *stopexit_checkbutton;

        gboolean sync_server;
};

enum
{
        ENABLED_COLUMN,
        NAME_COLUMN,
        ID_COLUMN,
        N_COLUMN
};

#define ARIO_SERVER_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SERVER_PREFERENCES, ArioServerPreferencesPrivate))
G_DEFINE_TYPE (ArioServerPreferences, ario_server_preferences, GTK_TYPE_VBOX)

static void
ario_server_preferences_class_init (ArioServerPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioServerPreferencesPrivate));
}

static void
ario_server_preferences_init (ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        server_preferences->priv = ARIO_SERVER_PREFERENCES_GET_PRIVATE (server_preferences);
}

static void
ario_server_preferences_output_toggled_cb (GtkCellRendererToggle *cell,
                                           gchar *path_str,
                                           ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean state;
        gint id;
        GtkTreeIter iter;
        GtkTreeModel *model = GTK_TREE_MODEL (server_preferences->priv->outputs_model);
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

        if (gtk_tree_model_get_iter (model, &iter, path)) {
                gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state, ID_COLUMN, &id, -1);
                state = !state;
                ario_server_enable_output (id,
                                        state);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, ENABLED_COLUMN, state, -1);
        }
        gtk_tree_path_free (path);
}

GtkWidget *
ario_server_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioServerPreferences *server_preferences;
        GtkBuilder *builder;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        ArioServer *server = ario_server_get_instance ();

        server_preferences = g_object_new (TYPE_ARIO_SERVER_PREFERENCES,
                                           NULL);

        g_return_val_if_fail (server_preferences->priv != NULL, NULL);

        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_server_preferences_server_changed_cb),
                                 server_preferences, 0);
        g_signal_connect_object (server,
                                 "updatingdb_changed",
                                 G_CALLBACK (ario_server_preferences_server_changed_cb),
                                 server_preferences, 0);

        builder = gtk_builder_helpers_new (UI_PATH "server-prefs.ui",
                                           server_preferences);

        server_preferences->priv->crossfade_checkbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "crossfade_checkbutton"));
        server_preferences->priv->crossfadetime_spinbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "crossfadetime_spinbutton"));
        server_preferences->priv->updatedb_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "updatedb_label"));
        server_preferences->priv->updatedb_button = 
                GTK_WIDGET (gtk_builder_get_object (builder, "updatedb_button"));
        server_preferences->priv->outputs_treeview = 
                GTK_WIDGET (gtk_builder_get_object (builder, "outputs_treeview"));
        server_preferences->priv->update_checkbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "update_checkbutton"));
        server_preferences->priv->stopexit_checkbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "stopexit_checkbutton"));

        gtk_builder_helpers_boldify_label (builder, "crossfade_frame_label");
        gtk_builder_helpers_boldify_label (builder, "database_frame_label");
        gtk_builder_helpers_boldify_label (builder, "ouputs_frame_label");

        server_preferences->priv->outputs_model = gtk_list_store_new (N_COLUMN,
                                                                      G_TYPE_BOOLEAN,
                                                                      G_TYPE_STRING,
                                                                      G_TYPE_INT);
        gtk_tree_view_set_model (GTK_TREE_VIEW (server_preferences->priv->outputs_treeview),
                                 GTK_TREE_MODEL (server_preferences->priv->outputs_model));
        renderer = gtk_cell_renderer_toggle_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Enabled"),
                                                           renderer,
                                                           "active", ENABLED_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 80);
        gtk_tree_view_append_column (GTK_TREE_VIEW (server_preferences->priv->outputs_treeview), column);
        g_signal_connect (GTK_OBJECT (renderer),
                          "toggled",
                          G_CALLBACK (ario_server_preferences_output_toggled_cb), server_preferences);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", NAME_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (server_preferences->priv->outputs_treeview), column);

        ario_server_preferences_sync_server (server_preferences);

        gtk_box_pack_start (GTK_BOX (server_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (server_preferences);
}

static void
ario_server_preferences_sync_server (ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        int crossfadetime;
        int state;
        gboolean updating;
        long last_update;
        gchar *last_update_char;
        GtkTreeIter iter;
        GSList *tmp;
        ArioServerOutput *output;
        GSList *outputs;

        state = ario_server_get_current_state ();
        updating = ario_server_get_updating ();

        if (state == MPD_STATUS_STATE_UNKNOWN) {
                crossfadetime = 0;
                last_update_char = _("Not connected");
        } else {
                crossfadetime = ario_server_get_crossfadetime ();

                if (updating) {
                        last_update_char = _("Updating...");
                } else {
                        last_update = (long) ario_server_get_last_update ();
                        last_update_char = ctime (&last_update);
                        /* Remove the new line */
                        if (last_update_char && strlen(last_update_char))
                                last_update_char[strlen (last_update_char)-1] = '\0';
                }
        }

        server_preferences->priv->sync_server = TRUE;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (server_preferences->priv->crossfade_checkbutton), (crossfadetime != 0) && (state != MPD_STATUS_STATE_UNKNOWN));
        gtk_widget_set_sensitive (server_preferences->priv->crossfade_checkbutton, state != MPD_STATUS_STATE_UNKNOWN);
        gtk_widget_set_sensitive (server_preferences->priv->crossfadetime_spinbutton, (crossfadetime != 0) && (state != MPD_STATUS_STATE_UNKNOWN));
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton), (gdouble) crossfadetime);

        gtk_widget_set_sensitive (server_preferences->priv->updatedb_button, (!updating && state != MPD_STATUS_STATE_UNKNOWN));
        gtk_label_set_label (GTK_LABEL (server_preferences->priv->updatedb_label), last_update_char);

        outputs = ario_server_get_outputs ();
        gtk_list_store_clear (server_preferences->priv->outputs_model);
        for (tmp = outputs; tmp; tmp = g_slist_next (tmp)) {
                output = (ArioServerOutput *) tmp->data;
                gtk_list_store_append (server_preferences->priv->outputs_model, &iter);
                gtk_list_store_set (server_preferences->priv->outputs_model, &iter,
                                    ENABLED_COLUMN, output->enabled,
                                    NAME_COLUMN, output->name,
                                    ID_COLUMN, output->id,
                                    -1);
        }
        g_slist_foreach (outputs, (GFunc) ario_server_free_output, NULL);
        g_slist_free (outputs);

        server_preferences->priv->sync_server = FALSE;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (server_preferences->priv->update_checkbutton),
                                      ario_conf_get_boolean (PREF_UPDATE_STARTUP, PREF_UPDATE_STARTUP_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (server_preferences->priv->stopexit_checkbutton),
                                      ario_conf_get_boolean (PREF_STOP_EXIT, PREF_STOP_EXIT_DEFAULT));
}

static void
ario_server_preferences_server_changed_cb (ArioServer *server,
                                           ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_server_preferences_sync_server (server_preferences);
}

void
ario_server_preferences_crossfadetime_changed_cb (GtkWidget *widget,
                                                  ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        int crossfadetime;
        if (!server_preferences->priv->sync_server) {
                crossfadetime = gtk_spin_button_get_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton));
                ario_server_set_crossfadetime (crossfadetime);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (server_preferences->priv->crossfade_checkbutton), (crossfadetime != 0));
                gtk_widget_set_sensitive (server_preferences->priv->crossfadetime_spinbutton, (crossfadetime != 0));
        }
}

void
ario_server_preferences_crossfade_changed_cb (GtkWidget *widget,
                                              ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean is_active;
        if (!server_preferences->priv->sync_server) {
                is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (server_preferences->priv->crossfade_checkbutton));
                if (is_active) {
                        ario_server_set_crossfadetime (1);
                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton),
                                                   1);
                } else {
                        ario_server_set_crossfadetime (0);
                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton),
                                                   0);
                }
                gtk_widget_set_sensitive (server_preferences->priv->crossfadetime_spinbutton, is_active);
        }
}

void
ario_server_preferences_updatedb_button_cb (GtkWidget *widget,
                                            ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_set_sensitive (server_preferences->priv->updatedb_button, FALSE);
        gtk_label_set_label (GTK_LABEL (server_preferences->priv->updatedb_label), _("Updating..."));
        ario_server_update_db ();
}

void
ario_server_preferences_update_changed_cb (GtkWidget *widget,
                                           ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_UPDATE_STARTUP,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

void
ario_server_preferences_stopexit_changed_cb (GtkWidget *widget,
                                             ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_STOP_EXIT,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

