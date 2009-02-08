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

#include "widgets/ario-connection-widget.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "lib/gtk-builder-helpers.h"
#ifdef ENABLE_AVAHI
#include "ario-avahi.h"
#endif
#include "ario-debug.h"
#include "ario-profiles.h"
#include "servers/ario-server.h"

static void ario_connection_widget_finalize (GObject *object);
G_MODULE_EXPORT void ario_connection_widget_name_changed_cb (GtkWidget *widget,
                                                             ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_host_changed_cb (GtkWidget *widget,
                                                             ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_port_changed_cb (GtkWidget *widget,
                                                             ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_type_changed_cb (GtkToggleAction *toggleaction,
                                                             ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_password_changed_cb (GtkWidget *widget,
                                                                 ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_local_changed_cb (GtkWidget *widget,
                                                              ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_musicdir_changed_cb (GtkWidget *widget,
                                                                 ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_autodetect_cb (GtkWidget *widget,
                                                           ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_open_cb (GtkWidget *widget,
                                                     ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_new_profile_cb (GtkWidget *widget,
                                                            ArioConnectionWidget *connection_widget);
G_MODULE_EXPORT void ario_connection_widget_delete_profile_cb (GtkWidget *widget,
                                                               ArioConnectionWidget *connection_widget);

enum
{
        PROFILE_CHANGED,
        LAST_SIGNAL
};

static guint ario_connection_widget_signals[LAST_SIGNAL] = { 0 };

struct ArioConnectionWidgetPrivate
{
        GtkWidget *profile_treeview;
        GtkListStore *profile_model;
        GtkTreeSelection *profile_selection;
        GSList *profiles;
        ArioProfile *current_profile;

        GtkWidget *name_entry;
        GtkWidget *host_entry;
        GtkWidget *port_spinbutton;
        GtkWidget *password_entry;
        GtkWidget *local_checkbutton;
        GtkWidget *musicdir_entry;
        GtkWidget *musicdir_hbox;
        GtkWidget *musicdir_label;
        GtkWidget *autodetect_button;
        GtkWidget *mpd_radiobutton;
        GtkWidget *xmms_radiobutton;
        GtkListStore *autodetect_model;
        GtkTreeSelection *autodetect_selection;
};


enum
{
        NAME_COLUMN,
        HOST_COLUMN,
        PORT_COLUMN,
        N_COLUMN
};

#define ARIO_CONNECTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_CONNECTION_WIDGET, ArioConnectionWidgetPrivate))
G_DEFINE_TYPE (ArioConnectionWidget, ario_connection_widget, GTK_TYPE_VBOX)

static void
ario_connection_widget_class_init (ArioConnectionWidgetClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        ario_connection_widget_signals[PROFILE_CHANGED] =
                g_signal_new ("profile_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioConnectionWidgetClass, profile_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        object_class->finalize = ario_connection_widget_finalize;

        g_type_class_add_private (klass, sizeof (ArioConnectionWidgetPrivate));
}

static void
ario_connection_widget_init (ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        connection_widget->priv = ARIO_CONNECTION_WIDGET_GET_PRIVATE (connection_widget);

        connection_widget->priv->current_profile = NULL;
}

/* Return TRUE if the profile has changed */
static gboolean
ario_connection_widget_profile_selection_update (ArioConnectionWidget *connection_widget,
                                                 gboolean force_update)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *profile = NULL;
        GList *paths;
        gint *indices;
        GtkTreeModel *model = GTK_TREE_MODEL (connection_widget->priv->profile_model);
        GSList *tmp = connection_widget->priv->profiles;
        int i;

        paths = gtk_tree_selection_get_selected_rows (connection_widget->priv->profile_selection,
                                                      &model);

        if (!paths)
                return FALSE;

        indices = gtk_tree_path_get_indices ((GtkTreePath *) paths->data);
        for (i = 0; i < indices[0] && tmp; ++i) {
                tmp = g_slist_next (tmp);
        }
        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        if (tmp) {
                profile = (ArioProfile *) tmp->data;
                if (!force_update && connection_widget->priv->current_profile == profile)
                        return FALSE;

                connection_widget->priv->current_profile = profile;
                ario_profiles_set_current (connection_widget->priv->profiles, profile);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->name_entry),
                                                 G_CALLBACK (ario_connection_widget_name_changed_cb),
                                                 connection_widget);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->host_entry),
                                                 G_CALLBACK (ario_connection_widget_host_changed_cb),
                                                 connection_widget);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->port_spinbutton),
                                                 G_CALLBACK (ario_connection_widget_port_changed_cb),
                                                 connection_widget);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->mpd_radiobutton),
                                                 G_CALLBACK (ario_connection_widget_type_changed_cb),
                                                 connection_widget);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->password_entry),
                                                 G_CALLBACK (ario_connection_widget_password_changed_cb),
                                                 connection_widget);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->musicdir_entry),
                                                 G_CALLBACK (ario_connection_widget_musicdir_changed_cb),
                                                 connection_widget);

                g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->local_checkbutton),
                                                 G_CALLBACK (ario_connection_widget_local_changed_cb),
                                                 connection_widget);

                gtk_entry_set_text (GTK_ENTRY (connection_widget->priv->name_entry), profile->name);
                gtk_entry_set_text (GTK_ENTRY (connection_widget->priv->host_entry), profile->host);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (connection_widget->priv->port_spinbutton), (gdouble) profile->port);
                if (profile->type == ArioServerXmms)
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (connection_widget->priv->xmms_radiobutton), TRUE);
                else
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (connection_widget->priv->mpd_radiobutton), TRUE);
                gtk_entry_set_text (GTK_ENTRY (connection_widget->priv->password_entry), profile->password ? profile->password : "");
                gtk_entry_set_text (GTK_ENTRY (connection_widget->priv->musicdir_entry), profile->musicdir ? profile->musicdir : "");
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (connection_widget->priv->local_checkbutton), profile->local);

                if (profile->local) {
                        gtk_widget_show (connection_widget->priv->musicdir_hbox);
                        gtk_widget_show (connection_widget->priv->musicdir_label);
                } else {
                        gtk_widget_hide (connection_widget->priv->musicdir_hbox);
                        gtk_widget_hide (connection_widget->priv->musicdir_label);
                }

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->name_entry),
                                                   G_CALLBACK (ario_connection_widget_name_changed_cb),
                                                   connection_widget);

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->host_entry),
                                                   G_CALLBACK (ario_connection_widget_host_changed_cb),
                                                   connection_widget);

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->port_spinbutton),
                                                   G_CALLBACK (ario_connection_widget_port_changed_cb),
                                                   connection_widget);

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->mpd_radiobutton),
                                                   G_CALLBACK (ario_connection_widget_type_changed_cb),
                                                   connection_widget);

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->password_entry),
                                                   G_CALLBACK (ario_connection_widget_password_changed_cb),
                                                   connection_widget);

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->musicdir_entry),
                                                   G_CALLBACK (ario_connection_widget_musicdir_changed_cb),
                                                   connection_widget);

                g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->local_checkbutton),
                                                   G_CALLBACK (ario_connection_widget_local_changed_cb),
                                                   connection_widget);
        }
        return TRUE;
}

static void
ario_connection_widget_profile_selection_changed_cb (GtkTreeSelection * selection,
                                                     ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        if (ario_connection_widget_profile_selection_update (connection_widget, FALSE)) {
                g_signal_emit (G_OBJECT (connection_widget), ario_connection_widget_signals[PROFILE_CHANGED], 0);
        }
}

static void
ario_connection_widget_profile_update_profiles (ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        GtkTreeIter iter;
        ArioProfile *profile;
        GtkTreeModel *model = GTK_TREE_MODEL (connection_widget->priv->profile_model);

        g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->profile_selection),
                                         G_CALLBACK (ario_connection_widget_profile_selection_changed_cb),
                                         connection_widget);

        gtk_list_store_clear (connection_widget->priv->profile_model);
        for (tmp = connection_widget->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                gtk_list_store_append (connection_widget->priv->profile_model, &iter);
                gtk_list_store_set (connection_widget->priv->profile_model, &iter,
                                    0, profile->name,
                                    -1);
        }

        gtk_tree_model_get_iter_first (model, &iter);
        for (tmp = connection_widget->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                if (profile->current) {
                        gtk_tree_selection_select_iter (connection_widget->priv->profile_selection, &iter);
                        break;
                }
                gtk_tree_model_iter_next (model, &iter);
        }

        g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->profile_selection),
                                           G_CALLBACK (ario_connection_widget_profile_selection_changed_cb),
                                           connection_widget);
}

GtkWidget *
ario_connection_widget_new (void)
{
        ARIO_LOG_FUNCTION_START
        GtkBuilder *builder;
        ArioConnectionWidget *connection_widget;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        connection_widget = g_object_new (TYPE_ARIO_CONNECTION_WIDGET,
                                          NULL);

        g_return_val_if_fail (connection_widget->priv != NULL, NULL);

        builder = gtk_builder_helpers_new (UI_PATH "connection-widget.ui",
                                           connection_widget);

        connection_widget->priv->profile_treeview = 
                GTK_WIDGET (gtk_builder_get_object (builder, "profile_treeview"));
        connection_widget->priv->name_entry = 
                GTK_WIDGET (gtk_builder_get_object (builder, "name_entry"));
        connection_widget->priv->host_entry = 
                GTK_WIDGET (gtk_builder_get_object (builder, "host_entry"));
        connection_widget->priv->port_spinbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "port_spinbutton"));
        connection_widget->priv->password_entry = 
                GTK_WIDGET (gtk_builder_get_object (builder, "password_entry"));
        connection_widget->priv->local_checkbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "local_checkbutton"));
        connection_widget->priv->musicdir_entry = 
                GTK_WIDGET (gtk_builder_get_object (builder, "musicdir_entry"));
        connection_widget->priv->musicdir_hbox = 
                GTK_WIDGET (gtk_builder_get_object (builder, "musicdir_hbox"));
        connection_widget->priv->musicdir_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "musicdir_label"));
        connection_widget->priv->autodetect_button = 
                GTK_WIDGET (gtk_builder_get_object (builder, "autodetect_button"));
        connection_widget->priv->mpd_radiobutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "mpd_radiobutton"));
        connection_widget->priv->xmms_radiobutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "xmms_radiobutton"));

        gtk_widget_show_all (connection_widget->priv->musicdir_hbox);
        gtk_widget_hide (connection_widget->priv->musicdir_hbox);
        gtk_widget_set_no_show_all (connection_widget->priv->musicdir_hbox, TRUE);

        connection_widget->priv->profile_model = gtk_list_store_new (1, G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (connection_widget->priv->profile_treeview),
                                 GTK_TREE_MODEL (connection_widget->priv->profile_model));
        connection_widget->priv->profile_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (connection_widget->priv->profile_treeview));
        gtk_tree_selection_set_mode (connection_widget->priv->profile_selection,
                                     GTK_SELECTION_BROWSE);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Profile"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 120);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (connection_widget->priv->profile_treeview), column);

        connection_widget->priv->profiles = ario_profiles_get ();
        ario_connection_widget_profile_update_profiles (connection_widget);

#ifndef ENABLE_AVAHI
        gtk_widget_hide (connection_widget->priv->autodetect_button);
#endif

#ifndef ENABLE_XMMS2
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "servertype_hbox")), FALSE);
#endif

        g_signal_connect (connection_widget->priv->profile_selection,
                          "changed",
                          G_CALLBACK (ario_connection_widget_profile_selection_changed_cb),
                          connection_widget);
        ario_connection_widget_profile_selection_update (connection_widget, FALSE);

        gtk_box_pack_start (GTK_BOX (connection_widget), GTK_WIDGET (gtk_builder_get_object (builder, "hbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (connection_widget);
}

static void
ario_connection_widget_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioConnectionWidget *connection_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_CONNECTION_WIDGET (object));

        connection_widget = ARIO_CONNECTION_WIDGET (object);

        g_return_if_fail (connection_widget->priv != NULL);

        ario_profiles_save (connection_widget->priv->profiles);

        G_OBJECT_CLASS (ario_connection_widget_parent_class)->finalize (object);
}

void
ario_connection_widget_name_changed_cb (GtkWidget *widget,
                                        ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        g_free (connection_widget->priv->current_profile->name);
        connection_widget->priv->current_profile->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (connection_widget->priv->name_entry)));
        ario_connection_widget_profile_update_profiles (connection_widget);
}

void
ario_connection_widget_host_changed_cb (GtkWidget *widget,
                                        ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        g_free (connection_widget->priv->current_profile->host);
        connection_widget->priv->current_profile->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (connection_widget->priv->host_entry)));
}

void
ario_connection_widget_port_changed_cb (GtkWidget *widget,
                                        ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        connection_widget->priv->current_profile->port = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (connection_widget->priv->port_spinbutton));
}

void ario_connection_widget_type_changed_cb (GtkToggleAction *toggleaction,
                                             ArioConnectionWidget *connection_widget)

{
        ARIO_LOG_FUNCTION_START
        ArioServerType type;
        type = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (connection_widget->priv->xmms_radiobutton)) ? ArioServerXmms : ArioServerMpd;
        connection_widget->priv->current_profile->type = type;
}

void
ario_connection_widget_password_changed_cb (GtkWidget *widget,
                                            ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        const gchar *password;
        password = gtk_entry_get_text (GTK_ENTRY (connection_widget->priv->password_entry));
        g_free (connection_widget->priv->current_profile->password);
        if (password && strcmp(password, "")) {
                connection_widget->priv->current_profile->password = g_strdup (password);
        } else {
                connection_widget->priv->current_profile->password = NULL;         
        }
}

void
ario_connection_widget_musicdir_changed_cb (GtkWidget *widget,
                                            ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        const gchar *musicdir;
        musicdir = gtk_entry_get_text (GTK_ENTRY (connection_widget->priv->musicdir_entry));
        g_free (connection_widget->priv->current_profile->musicdir);
        if (musicdir && strcmp(musicdir, "")) {
                connection_widget->priv->current_profile->musicdir = g_strdup (musicdir);
        } else {
                connection_widget->priv->current_profile->musicdir = NULL;         
        }
}

void
ario_connection_widget_local_changed_cb (GtkWidget *widget,
                                         ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        gboolean local;
        local = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (connection_widget->priv->local_checkbutton));
        connection_widget->priv->current_profile->local = local;
        if (local) {
                gtk_widget_show (connection_widget->priv->musicdir_hbox);
                gtk_widget_show (connection_widget->priv->musicdir_label);
        } else {
                gtk_widget_hide (connection_widget->priv->musicdir_hbox);
                gtk_widget_hide (connection_widget->priv->musicdir_label);
        }
}

#ifdef ENABLE_AVAHI
static void
ario_connection_widget_autohosts_changed_cb (ArioAvahi *avahi,
                                             ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GSList *hosts;
        gtk_list_store_clear (connection_widget->priv->autodetect_model);

        for (hosts = ario_avahi_get_hosts (avahi); hosts; hosts = g_slist_next (hosts)) {
                ArioHost *host = hosts->data;
                char *tmp;
                gtk_list_store_append (connection_widget->priv->autodetect_model, &iter);
                tmp = g_strdup_printf ("%d", host->port);
                gtk_list_store_set (connection_widget->priv->autodetect_model, &iter,
                                    NAME_COLUMN, host->name,
                                    HOST_COLUMN, host->host,
                                    PORT_COLUMN, tmp,
                                    -1);
                g_free (tmp);
        }

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (connection_widget->priv->autodetect_model), &iter))
                gtk_tree_selection_select_iter (connection_widget->priv->autodetect_selection, &iter);
}
#endif
void
ario_connection_widget_autodetect_cb (GtkWidget *widget,
                                      ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
#ifdef ENABLE_AVAHI
        ArioAvahi *avahi;
        GtkWidget *dialog, *error_dialog;
        GtkWidget *vbox;
        GtkWidget *label;
        GtkWidget *scrolledwindow;
        GtkWidget *treeview;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        gchar *tmp;
        gchar *host;
        int port;

        gint retval = GTK_RESPONSE_CANCEL;

        avahi = ario_avahi_new ();

        dialog = gtk_dialog_new_with_buttons (_("Server autodetection"),
                                              NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL);
        vbox = gtk_vbox_new (FALSE, 12);
        label = gtk_label_new (_("If you don't see your MPD server thanks to the automatic detection, you should check that zeroconf is activated in your MPD configuration or use the manual configuration."));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
        treeview = gtk_tree_view_new ();

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", NAME_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 150);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Host"),
                                                           renderer,
                                                           "text", HOST_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 150);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Port"),
                                                           renderer,
                                                           "text", PORT_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 50);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        connection_widget->priv->autodetect_model = gtk_list_store_new (N_COLUMN,
                                                                        G_TYPE_STRING,
                                                                        G_TYPE_STRING,
                                                                        G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (connection_widget->priv->autodetect_model));
        connection_widget->priv->autodetect_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (connection_widget->priv->autodetect_selection,
                                     GTK_SELECTION_BROWSE);

        gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);
        gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

        g_signal_connect (avahi,
                          "hosts_changed",
                          G_CALLBACK (ario_connection_widget_autohosts_changed_cb),
                          connection_widget);

        gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 
                           vbox);
        gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 280);
        gtk_widget_show_all (dialog);

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        if (retval != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                g_object_unref (avahi);
                return;
        }

        treemodel = GTK_TREE_MODEL (connection_widget->priv->autodetect_model);
        if (gtk_tree_selection_get_selected (connection_widget->priv->autodetect_selection,
                                             &treemodel,
                                             &iter)) {
                gtk_tree_model_get (treemodel, &iter,
                                    HOST_COLUMN, &host,
                                    PORT_COLUMN, &tmp, -1);
                port = atoi (tmp);
                g_free (tmp);

                g_free (connection_widget->priv->current_profile->host);
                connection_widget->priv->current_profile->host = g_strdup (host);
                g_free (host);
                connection_widget->priv->current_profile->port = port;
                g_free (connection_widget->priv->current_profile->password);
                connection_widget->priv->current_profile->password = NULL;
                g_free (connection_widget->priv->current_profile->musicdir);
                connection_widget->priv->current_profile->musicdir = NULL;

                ario_connection_widget_profile_selection_update (connection_widget, TRUE);
        } else {
                error_dialog = gtk_message_dialog_new(NULL,
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      _("You must select a server."));
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
        }

        g_object_unref (avahi);
        gtk_widget_destroy (dialog);
#endif
}

void
ario_connection_widget_open_cb (GtkWidget *widget,
                                ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;

        dialog = gtk_file_chooser_dialog_new (NULL,
                                              NULL,
                                              GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                              NULL);

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                char *filename;

                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
                if (filename) {
                        gtk_entry_set_text (GTK_ENTRY (connection_widget->priv->musicdir_entry), filename);
                        g_free (filename);
                }
        }

        gtk_widget_destroy (dialog);
}

void
ario_connection_widget_new_profile_cb (GtkWidget *widget,
                                       ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *profile;
        ArioProfile *tmp_profile;
        GSList *tmp;

        profile = (ArioProfile *) g_malloc0 (sizeof (ArioProfile));
        profile->name = g_strdup (_("New Profile"));
        profile->host = g_strdup ("localhost");
        profile->port = 6600;
        profile->type = ArioServerMpd;

        for (tmp = connection_widget->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                tmp_profile = (ArioProfile *) tmp->data;
                tmp_profile->current = FALSE;
        }
        profile->current = TRUE;
        connection_widget->priv->profiles = g_slist_append (connection_widget->priv->profiles, profile);
        ario_connection_widget_profile_update_profiles (connection_widget);
        ario_connection_widget_profile_selection_update (connection_widget, FALSE);
        g_signal_emit (G_OBJECT (connection_widget), ario_connection_widget_signals[PROFILE_CHANGED], 0);
}

void
ario_connection_widget_delete_profile_cb (GtkWidget *widget,
                                          ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *first_profile;

        if (g_slist_length (connection_widget->priv->profiles) < 2)
                return;

        if (connection_widget->priv->current_profile) {
                connection_widget->priv->profiles = g_slist_remove (connection_widget->priv->profiles, connection_widget->priv->current_profile);
                ario_profiles_free (connection_widget->priv->current_profile);
                if (connection_widget->priv->profiles) {
                        first_profile = (ArioProfile *) connection_widget->priv->profiles->data;
                        first_profile->current = TRUE;
                }
                ario_connection_widget_profile_update_profiles (connection_widget);
                ario_connection_widget_profile_selection_update (connection_widget, FALSE);
                g_signal_emit (G_OBJECT (connection_widget), ario_connection_widget_signals[PROFILE_CHANGED], 0);
        }
}

