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

#ifdef ENABLE_AVAHI
#include "ario-avahi.h"
#endif
#include "ario-debug.h"
#include "ario-profiles.h"
#include "ario-util.h"
#include "lib/gtk-builder-helpers.h"
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
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* Signals */
        ario_connection_widget_signals[PROFILE_CHANGED] =
                g_signal_new ("profile_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioConnectionWidgetClass, profile_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /* Virtual methods */
        object_class->finalize = ario_connection_widget_finalize;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioConnectionWidgetPrivate));
}

static void
ario_connection_widget_init (ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        connection_widget->priv = ARIO_CONNECTION_WIDGET_GET_PRIVATE (connection_widget);

        connection_widget->priv->current_profile = NULL;
}

/* Return TRUE if the profile has changed */
static gboolean
ario_connection_widget_profile_selection_update (ArioConnectionWidget *connection_widget,
                                                 gboolean force_update)
{
        ARIO_LOG_FUNCTION_START;
        ArioProfile *profile = NULL;
        GList *paths;
        gint *indices;
        GtkTreeModel *model = GTK_TREE_MODEL (connection_widget->priv->profile_model);
        GSList *tmp = connection_widget->priv->profiles;
        int i;

        /* Get selected row */
        paths = gtk_tree_selection_get_selected_rows (connection_widget->priv->profile_selection,
                                                      &model);
        if (!paths)
                return FALSE;

        /* Get the profile corresponding to selected row */
        indices = gtk_tree_path_get_indices ((GtkTreePath *) paths->data);
        for (i = 0; i < indices[0] && tmp; ++i) {
                tmp = g_slist_next (tmp);
        }
        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        if (!tmp)
                return FALSE;

        profile = (ArioProfile *) tmp->data;
        if (!force_update && connection_widget->priv->current_profile == profile)
                return FALSE;

        /* Change the current profile to the selected one */
        connection_widget->priv->current_profile = profile;
        ario_profiles_set_current (connection_widget->priv->profiles, profile);


        /* Block a few signals */
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

        /* Change the different widgets with values of the profile */
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

        /* Unblock signals */
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
        return TRUE;
}

static void
ario_connection_widget_profile_selection_changed_cb (GtkTreeSelection * selection,
                                                     ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_connection_widget_profile_selection_update (connection_widget, FALSE)) {
                /* Emit a signal when profile has changed */
                g_signal_emit (G_OBJECT (connection_widget), ario_connection_widget_signals[PROFILE_CHANGED], 0);
        }
}

static void
ario_connection_widget_profile_update_profiles (ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp;
        GtkTreeIter iter;
        ArioProfile *profile;
        GtkTreeModel *model = GTK_TREE_MODEL (connection_widget->priv->profile_model);

        /* Block profile changement signal */
        g_signal_handlers_block_by_func (G_OBJECT (connection_widget->priv->profile_selection),
                                         G_CALLBACK (ario_connection_widget_profile_selection_changed_cb),
                                         connection_widget);

        /* Clear the list of profiles */
        gtk_list_store_clear (connection_widget->priv->profile_model);

        /* Append the new profiles to the list */
        for (tmp = connection_widget->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                gtk_list_store_append (connection_widget->priv->profile_model, &iter);
                gtk_list_store_set (connection_widget->priv->profile_model, &iter,
                                    0, profile->name,
                                    -1);
        }

        /* Select current profile */
        gtk_tree_model_get_iter_first (model, &iter);
        for (tmp = connection_widget->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                if (profile->current) {
                        gtk_tree_selection_select_iter (connection_widget->priv->profile_selection, &iter);
                        break;
                }
                gtk_tree_model_iter_next (model, &iter);
        }

        /* Unblock profile changement signal */
        g_signal_handlers_unblock_by_func (G_OBJECT (connection_widget->priv->profile_selection),
                                           G_CALLBACK (ario_connection_widget_profile_selection_changed_cb),
                                           connection_widget);
}

GtkWidget *
ario_connection_widget_new (void)
{
        ARIO_LOG_FUNCTION_START;
        GtkBuilder *builder;
        ArioConnectionWidget *connection_widget;
        GtkWidget *profile_treeview;

        connection_widget = g_object_new (TYPE_ARIO_CONNECTION_WIDGET,
                                          NULL);

        g_return_val_if_fail (connection_widget->priv != NULL, NULL);

        /* Create UI using GtkBuilder */
        builder = gtk_builder_helpers_new (UI_PATH "connection-widget.ui",
                                           connection_widget);

        /* Get pointers to the different widgets */
        profile_treeview =
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
        connection_widget->priv->profile_model =
                GTK_LIST_STORE (gtk_builder_get_object (builder, "profile_model"));

        /* Show all widgets except musicdir_box (shown only if Ario is on same computer as
         * music server */
        gtk_widget_show_all (connection_widget->priv->musicdir_hbox);
        gtk_widget_hide (connection_widget->priv->musicdir_hbox);
        gtk_widget_set_no_show_all (connection_widget->priv->musicdir_hbox, TRUE);

        /* Get the model selection */
        connection_widget->priv->profile_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profile_treeview));
        gtk_tree_selection_set_mode (connection_widget->priv->profile_selection,
                                     GTK_SELECTION_BROWSE);

        /* Get the list of profiles */
        connection_widget->priv->profiles = ario_profiles_get ();

        /* Update the list with profiles */
        ario_connection_widget_profile_update_profiles (connection_widget);

#ifndef ENABLE_AVAHI
        gtk_widget_hide (connection_widget->priv->autodetect_button);
#endif

        /* Enable the servertype widgets only if XMMS2 support is activated */
#ifndef ENABLE_XMMS2
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "servertype_hbox")), FALSE);
#endif

        /* Connect signal for profile change */
        g_signal_connect (connection_widget->priv->profile_selection,
                          "changed",
                          G_CALLBACK (ario_connection_widget_profile_selection_changed_cb),
                          connection_widget);
        ario_connection_widget_profile_selection_update (connection_widget, FALSE);

        /* Add widgets to ConnectionWidgets */
        gtk_box_pack_start (GTK_BOX (connection_widget),
                            GTK_WIDGET (gtk_builder_get_object (builder, "hbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (connection_widget);
}

static void
ario_connection_widget_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioConnectionWidget *connection_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_CONNECTION_WIDGET (object));

        connection_widget = ARIO_CONNECTION_WIDGET (object);

        g_return_if_fail (connection_widget->priv != NULL);

        /* Save profiles to disk */
        ario_profiles_save (connection_widget->priv->profiles);

        G_OBJECT_CLASS (ario_connection_widget_parent_class)->finalize (object);
}

void
ario_connection_widget_name_changed_cb (GtkWidget *widget,
                                        ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        /* Modify current profile */
        g_free (connection_widget->priv->current_profile->name);
        connection_widget->priv->current_profile->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (connection_widget->priv->name_entry)));

        /* Change profile name in the list */
        ario_connection_widget_profile_update_profiles (connection_widget);
}

void
ario_connection_widget_host_changed_cb (GtkWidget *widget,
                                        ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        /* Modify current profile */
        g_free (connection_widget->priv->current_profile->host);
        connection_widget->priv->current_profile->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (connection_widget->priv->host_entry)));
}

void
ario_connection_widget_port_changed_cb (GtkWidget *widget,
                                        ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        /* Modify current profile */
        gdouble port = gtk_spin_button_get_value (GTK_SPIN_BUTTON (connection_widget->priv->port_spinbutton));
        connection_widget->priv->current_profile->port = (int) port;
}

void ario_connection_widget_type_changed_cb (GtkToggleAction *toggleaction,
                                             ArioConnectionWidget *connection_widget)

{
        ARIO_LOG_FUNCTION_START;
        ArioServerType type;

        /* Modify current profile */
        type = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (connection_widget->priv->xmms_radiobutton)) ? ArioServerXmms : ArioServerMpd;
        connection_widget->priv->current_profile->type = type;
}

void
ario_connection_widget_password_changed_cb (GtkWidget *widget,
                                            ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *password;

        /* Modify current profile */
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
        ARIO_LOG_FUNCTION_START;
        const gchar *musicdir;

        /* Modify current profile */
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
        ARIO_LOG_FUNCTION_START;
        gboolean local;

        /* Modify current profile */
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
/* Method called when list of hosts detected by avahi has changed */
static void
ario_connection_widget_autohosts_changed_cb (ArioAvahi *avahi,
                                             ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GSList *hosts;
        char tmp[INTLEN];

        /* Clear the hosts lists */
        gtk_list_store_clear (connection_widget->priv->autodetect_model);

        /* Append all found hosts to the list */
        for (hosts = ario_avahi_get_hosts (avahi); hosts; hosts = g_slist_next (hosts)) {
                ArioHost *host = hosts->data;
                gtk_list_store_append (connection_widget->priv->autodetect_model, &iter);
                g_snprintf (tmp, INTLEN, "%d", host->port);
                gtk_list_store_set (connection_widget->priv->autodetect_model, &iter,
                                    NAME_COLUMN, host->name,
                                    HOST_COLUMN, host->host,
                                    PORT_COLUMN, tmp,
                                    -1);
        }

        /* Select first host */
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (connection_widget->priv->autodetect_model), &iter))
                gtk_tree_selection_select_iter (connection_widget->priv->autodetect_selection, &iter);
}
#endif
void
ario_connection_widget_autodetect_cb (GtkWidget *widget,
                                      ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
#ifdef ENABLE_AVAHI
        GtkBuilder *builder;
        ArioAvahi *avahi;
        GtkWidget *dialog, *error_dialog;
        GtkWidget *treeview;
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        gchar *tmp;
        gchar *host;
        int port;
        gint retval;

        /* Create UI using GtkBuilder */
        builder = gtk_builder_helpers_new (UI_PATH "connection-autodetect.ui", NULL);

        /* Get pointers to the different widgets */
        dialog =
                GTK_WIDGET (gtk_builder_get_object (builder, "dialog"));
        treeview =
                GTK_WIDGET (gtk_builder_get_object (builder, "treeview"));
        connection_widget->priv->autodetect_model =
                GTK_LIST_STORE (gtk_builder_get_object (builder, "autodetect_model"));
        g_object_unref (builder);

        /* Create avahi proxy */
        avahi = ario_avahi_new ();

        /* Get selection */
        connection_widget->priv->autodetect_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (connection_widget->priv->autodetect_selection,
                                     GTK_SELECTION_BROWSE);

        /* Connect signal called when avahi detects new hosts */
        g_signal_connect (avahi,
                          "hosts_changed",
                          G_CALLBACK (ario_connection_widget_autohosts_changed_cb),
                          connection_widget);

        /* Run dialog */
        gtk_widget_show_all (dialog);
        retval = gtk_dialog_run (GTK_DIALOG(dialog));

        /* Stop here if pressed button is not OK */
        if (retval != 1) {
                gtk_widget_destroy (dialog);
                g_object_unref (avahi);
                return;
        }

        treemodel = GTK_TREE_MODEL (connection_widget->priv->autodetect_model);
        if (gtk_tree_selection_get_selected (connection_widget->priv->autodetect_selection,
                                             &treemodel,
                                             &iter)) {
                /* Get infos about selected host */
                gtk_tree_model_get (treemodel, &iter,
                                    HOST_COLUMN, &host,
                                    PORT_COLUMN, &tmp, -1);
                port = atoi (tmp);
                g_free (tmp);

                /* Update current profile with avahi info */
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
                /* No server selected */
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
        ARIO_LOG_FUNCTION_START;
        GtkWidget *dialog;

        /* Create a dialog to choose the music directory */
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
                        /* Set the text in the entry with choosen path */
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
        ARIO_LOG_FUNCTION_START;
        ArioProfile *profile;
        ArioProfile *tmp_profile;
        GSList *tmp;

        /* Create a new profile with default values */
        profile = (ArioProfile *) g_malloc0 (sizeof (ArioProfile));
        profile->name = g_strdup (_("New Profile"));
        profile->host = g_strdup ("localhost");
        profile->port = 6600;
        profile->type = ArioServerMpd;

        /* Remove 'current' flag from all profiles */
        for (tmp = connection_widget->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                tmp_profile = (ArioProfile *) tmp->data;
                tmp_profile->current = FALSE;
        }

        /* Append new profile to the list */
        connection_widget->priv->profiles = g_slist_append (connection_widget->priv->profiles, profile);

        /* Set the new profile as current */
        profile->current = TRUE;
        ario_connection_widget_profile_update_profiles (connection_widget);
        ario_connection_widget_profile_selection_update (connection_widget, FALSE);

        /* Notify that current profile has changed */
        g_signal_emit (G_OBJECT (connection_widget), ario_connection_widget_signals[PROFILE_CHANGED], 0);
}

void
ario_connection_widget_delete_profile_cb (GtkWidget *widget,
                                          ArioConnectionWidget *connection_widget)
{
        ARIO_LOG_FUNCTION_START;
        ArioProfile *first_profile;

        /* We need to keep at least one profile */
        if (g_slist_length (connection_widget->priv->profiles) < 2)
                return;

        if (connection_widget->priv->current_profile) {
                /* Remove the profile from the list */
                connection_widget->priv->profiles = g_slist_remove (connection_widget->priv->profiles, connection_widget->priv->current_profile);

                /* Delete the profile */
                ario_profiles_free (connection_widget->priv->current_profile);

                /* Set the first profile as the current one */
                if (connection_widget->priv->profiles) {
                        first_profile = (ArioProfile *) connection_widget->priv->profiles->data;
                        first_profile->current = TRUE;
                }
                ario_connection_widget_profile_update_profiles (connection_widget);
                ario_connection_widget_profile_selection_update (connection_widget, FALSE);

                /* Notify that current profile has changed */
                g_signal_emit (G_OBJECT (connection_widget), ario_connection_widget_signals[PROFILE_CHANGED], 0);
        }
}

