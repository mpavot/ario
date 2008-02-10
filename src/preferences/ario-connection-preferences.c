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
#include "preferences/ario-connection-preferences.h"
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/eel-gconf-extensions.h"
#include "ario-avahi.h"
#include "ario-debug.h"
#include "ario-profiles.h"

static void ario_connection_preferences_class_init (ArioConnectionPreferencesClass *klass);
static void ario_connection_preferences_init (ArioConnectionPreferences *connection_preferences);
static void ario_connection_preferences_finalize (GObject *object);
static void ario_connection_preferences_set_property (GObject *object,
                                                      guint prop_id,
                                                      const GValue *value,
                                                      GParamSpec *pspec);
static void ario_connection_preferences_get_property (GObject *object,
                                                      guint prop_id,
                                                      GValue *value,
                                                      GParamSpec *pspec);
static void ario_connection_preferences_sync_connection (ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_name_changed_cb (GtkWidget *widget,
                                                  ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_host_changed_cb (GtkWidget *widget,
                                                  ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_port_changed_cb (GtkWidget *widget,
                                                  ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_autoconnect_changed_cb (GtkWidget *widget,
                                                         ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_password_changed_cb (GtkWidget *widget,
                                                      ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_autodetect_cb (GtkWidget *widget,
                                                ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_new_profile_cb (GtkWidget *widget,
                                                 ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_delete_profile_cb (GtkWidget *widget,
                                                    ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_connect_cb (GtkWidget *widget,
                                             ArioConnectionPreferences *connection_preferences);
void ario_connection_preferences_disconnect_cb (GtkWidget *widget,
                                                ArioConnectionPreferences *connection_preferences);

enum
{
        PROP_0,
        PROP_MPD
};

struct ArioConnectionPreferencesPrivate
{
        ArioMpd *mpd;

        GtkWidget *profile_treeview;
        GtkListStore *profile_model;
        GtkTreeSelection *profile_selection;
        GSList *profiles;
        ArioProfile *current_profile;

        GtkWidget *name_entry;
        GtkWidget *host_entry;
        GtkWidget *port_spinbutton;
        GtkWidget *password_entry;
        GtkWidget *autoconnect_checkbutton;
        GtkWidget *autodetect_button;
        GtkWidget *disconnect_button;
        GtkWidget *connect_button;
        GtkListStore *autodetect_model;
        GtkTreeSelection *autodetect_selection;

        gboolean loading;
};


enum
{
        NAME_COLUMN,
        HOST_COLUMN,
        PORT_COLUMN,
        N_COLUMN
};

static GObjectClass *parent_class = NULL;

GType
ario_connection_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_connection_preferences_type = 0;

        if (ario_connection_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioConnectionPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_connection_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioConnectionPreferences),
                        0,
                        (GInstanceInitFunc) ario_connection_preferences_init
                };

                ario_connection_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                           "ArioConnectionPreferences",
                                                                           &our_info, 0);
        }

        return ario_connection_preferences_type;
}

static void
ario_connection_preferences_class_init (ArioConnectionPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_connection_preferences_finalize;
        object_class->set_property = ario_connection_preferences_set_property;
        object_class->get_property = ario_connection_preferences_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_connection_preferences_init (ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        connection_preferences->priv = g_new0 (ArioConnectionPreferencesPrivate, 1);

        connection_preferences->priv->loading = FALSE;
        connection_preferences->priv->current_profile = NULL;
}

static void
ario_connection_preferences_profile_selection_update (ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *profile = NULL;
        GList *paths;
        gint *indices;
        GtkTreeModel *model = GTK_TREE_MODEL (connection_preferences->priv->profile_model);
        GSList *tmp = connection_preferences->priv->profiles;
        int i;

        paths = gtk_tree_selection_get_selected_rows (connection_preferences->priv->profile_selection,
                                                      &model);

        if (!paths)
                return;

        indices = gtk_tree_path_get_indices ((GtkTreePath *) paths->data);
        for (i = 0; i < indices[0] && tmp; ++i) {
                tmp = g_slist_next (tmp);
        }
        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        if (tmp) {
                profile = (ArioProfile *) tmp->data;
                connection_preferences->priv->current_profile = profile;
                ario_profiles_set_current (connection_preferences->priv->profiles, profile);

                gtk_entry_set_text (GTK_ENTRY (connection_preferences->priv->name_entry), profile->name);
                gtk_entry_set_text (GTK_ENTRY (connection_preferences->priv->host_entry), profile->host);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (connection_preferences->priv->port_spinbutton), (gdouble) profile->port);
                gtk_entry_set_text (GTK_ENTRY (connection_preferences->priv->password_entry), profile->password ? profile->password : "");
        }
}

static void
ario_connection_preferences_profile_selection_changed_cb (GtkTreeSelection * selection,
                                                          ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_connection_preferences_profile_selection_update (connection_preferences);
}


static void
ario_connection_preferences_profile_update_profiles (ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        GtkTreeIter iter;
        ArioProfile *profile;
        GtkTreeModel *model = GTK_TREE_MODEL (connection_preferences->priv->profile_model);

        gtk_list_store_clear (connection_preferences->priv->profile_model);
        for (tmp = connection_preferences->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                gtk_list_store_append (connection_preferences->priv->profile_model, &iter);
                gtk_list_store_set (connection_preferences->priv->profile_model, &iter,
                                    0, profile->name,
                                    -1);
        }

        gtk_tree_model_get_iter_first (model, &iter);
        for (tmp = connection_preferences->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                if (profile->current) {
                        gtk_tree_selection_select_iter (connection_preferences->priv->profile_selection, &iter);
                        break;
                }
                gtk_tree_model_iter_next (model, &iter);
        }
}

GtkWidget *
ario_connection_preferences_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        ArioConnectionPreferences *connection_preferences;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        connection_preferences = g_object_new (TYPE_ARIO_CONNECTION_PREFERENCES,
                                               "mpd", mpd,
                                               NULL);

        g_return_val_if_fail (connection_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "connection-prefs.glade",
                                "vbox",
                                connection_preferences);

        connection_preferences->priv->profile_treeview = 
                glade_xml_get_widget (xml, "profile_treeview");
        connection_preferences->priv->name_entry = 
                glade_xml_get_widget (xml, "name_entry");
        connection_preferences->priv->host_entry = 
                glade_xml_get_widget (xml, "host_entry");
        connection_preferences->priv->port_spinbutton = 
                glade_xml_get_widget (xml, "port_spinbutton");
        connection_preferences->priv->password_entry = 
                glade_xml_get_widget (xml, "password_entry");
        connection_preferences->priv->autoconnect_checkbutton = 
                glade_xml_get_widget (xml, "autoconnect_checkbutton");
        connection_preferences->priv->autodetect_button = 
                glade_xml_get_widget (xml, "autodetect_button");
        connection_preferences->priv->disconnect_button = 
                glade_xml_get_widget (xml, "disconnect_button");
        connection_preferences->priv->connect_button = 
                glade_xml_get_widget (xml, "connect_button");

        rb_glade_boldify_label (xml, "connection_label");

        connection_preferences->priv->profile_model = gtk_list_store_new (1, G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (connection_preferences->priv->profile_treeview),
                                 GTK_TREE_MODEL (connection_preferences->priv->profile_model));
        connection_preferences->priv->profile_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (connection_preferences->priv->profile_treeview));
        gtk_tree_selection_set_mode (connection_preferences->priv->profile_selection,
                                     GTK_SELECTION_BROWSE);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Profile"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 120);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (connection_preferences->priv->profile_treeview), column);
        g_signal_connect_object (G_OBJECT (connection_preferences->priv->profile_selection),
                                 "changed",
                                 G_CALLBACK (ario_connection_preferences_profile_selection_changed_cb),
                                 connection_preferences, 0);

        connection_preferences->priv->profiles = ario_profiles_read ();
        ario_connection_preferences_profile_update_profiles (connection_preferences);

        ario_connection_preferences_sync_connection (connection_preferences);

        gtk_box_pack_start (GTK_BOX (connection_preferences), glade_xml_get_widget (xml, "vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (connection_preferences);
}

static void
ario_connection_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioConnectionPreferences *connection_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_CONNECTION_PREFERENCES (object));

        connection_preferences = ARIO_CONNECTION_PREFERENCES (object);

        g_return_if_fail (connection_preferences->priv != NULL);

        ario_profiles_save (connection_preferences->priv->profiles);
        g_slist_foreach (connection_preferences->priv->profiles, (GFunc) ario_profiles_free, NULL);
        g_slist_free (connection_preferences->priv->profiles);

        g_free (connection_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_connection_preferences_set_property (GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioConnectionPreferences *connection_preferences = ARIO_CONNECTION_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                connection_preferences->priv->mpd = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_connection_preferences_get_property (GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioConnectionPreferences *connection_preferences = ARIO_CONNECTION_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, connection_preferences->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_connection_preferences_sync_connection (ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean autoconnect;

        connection_preferences->priv->loading = TRUE;

        autoconnect = eel_gconf_get_boolean (CONF_AUTOCONNECT, FALSE);

        if (ario_mpd_is_connected (connection_preferences->priv->mpd)) {
                gtk_widget_set_sensitive (connection_preferences->priv->connect_button, FALSE);
                gtk_widget_set_sensitive (connection_preferences->priv->disconnect_button, TRUE);
        } else {
                gtk_widget_set_sensitive (connection_preferences->priv->connect_button, TRUE);
                gtk_widget_set_sensitive (connection_preferences->priv->disconnect_button, FALSE);
        }

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (connection_preferences->priv->autoconnect_checkbutton), autoconnect);

        connection_preferences->priv->loading = FALSE;
}

void
ario_connection_preferences_name_changed_cb (GtkWidget *widget,
                                             ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!connection_preferences->priv->loading) {
                g_free (connection_preferences->priv->current_profile->name);
                connection_preferences->priv->current_profile->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (connection_preferences->priv->name_entry)));
                ario_connection_preferences_profile_update_profiles (connection_preferences);
        }
}

void
ario_connection_preferences_host_changed_cb (GtkWidget *widget,
                                             ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!connection_preferences->priv->loading) {
                eel_gconf_set_string (CONF_HOST,
                                      gtk_entry_get_text (GTK_ENTRY (connection_preferences->priv->host_entry)));
                g_free (connection_preferences->priv->current_profile->host);
                connection_preferences->priv->current_profile->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (connection_preferences->priv->host_entry)));
        }
}

void
ario_connection_preferences_port_changed_cb (GtkWidget *widget,
                                             ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!connection_preferences->priv->loading) {
                eel_gconf_set_integer (CONF_PORT,
                                       (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (connection_preferences->priv->port_spinbutton)));
                connection_preferences->priv->current_profile->port = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (connection_preferences->priv->port_spinbutton));
        }
}

void
ario_connection_preferences_autoconnect_changed_cb (GtkWidget *widget,
                                                    ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!connection_preferences->priv->loading)
                eel_gconf_set_boolean (CONF_AUTOCONNECT,
                                       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (connection_preferences->priv->autoconnect_checkbutton)));
}

void
ario_connection_preferences_password_changed_cb (GtkWidget *widget,
                                                 ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        const gchar *password;
        if (!connection_preferences->priv->loading) {
                password = gtk_entry_get_text (GTK_ENTRY (connection_preferences->priv->password_entry));
                eel_gconf_set_string (CONF_PASSWORD, password);
                g_free (connection_preferences->priv->current_profile->password);
                if (password && strcmp(password, "")) {
                        connection_preferences->priv->current_profile->password = g_strdup (password);
                } else {
                        connection_preferences->priv->current_profile->password = NULL;         
                }
        }
}

static void
ario_connection_preferences_autohosts_changed_cb (ArioAvahi *avahi,
                                                  ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GSList *hosts;
        gtk_list_store_clear (connection_preferences->priv->autodetect_model);

        for (hosts = ario_avahi_get_hosts (avahi); hosts; hosts = g_slist_next (hosts)) {
                ArioHost *host = hosts->data;
                char *tmp;
                gtk_list_store_append (connection_preferences->priv->autodetect_model, &iter);
                tmp = g_strdup_printf ("%d", host->port);
                gtk_list_store_set (connection_preferences->priv->autodetect_model, &iter,
                                    NAME_COLUMN, host->name,
                                    HOST_COLUMN, host->host,
                                    PORT_COLUMN, tmp,
                                    -1);
                g_free (tmp);
        }

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (connection_preferences->priv->autodetect_model), &iter))
                gtk_tree_selection_select_iter (connection_preferences->priv->autodetect_selection, &iter);
}

void
ario_connection_preferences_autodetect_cb (GtkWidget *widget,
                                           ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START

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
        connection_preferences->priv->autodetect_model = gtk_list_store_new (N_COLUMN,
                                                                             G_TYPE_STRING,
                                                                             G_TYPE_STRING,
                                                                             G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (connection_preferences->priv->autodetect_model));
        connection_preferences->priv->autodetect_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (connection_preferences->priv->autodetect_selection,
                                     GTK_SELECTION_BROWSE);

        gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);
        gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

        g_signal_connect_object (G_OBJECT (avahi),
                                 "hosts_changed", G_CALLBACK (ario_connection_preferences_autohosts_changed_cb),
                                 connection_preferences, 0);

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

        treemodel = GTK_TREE_MODEL (connection_preferences->priv->autodetect_model);
        if (gtk_tree_selection_get_selected (connection_preferences->priv->autodetect_selection,
                                             &treemodel,
                                             &iter)) {
                gtk_tree_model_get (treemodel, &iter,
                                    HOST_COLUMN, &host,
                                    PORT_COLUMN, &tmp, -1);
                port = atoi (tmp);
                g_free (tmp);

                g_free (connection_preferences->priv->current_profile->host);
                connection_preferences->priv->current_profile->host = g_strdup (host);
                g_free (host);
                connection_preferences->priv->current_profile->port = port;
                g_free (connection_preferences->priv->current_profile->password);
                connection_preferences->priv->current_profile->password = NULL;

                ario_connection_preferences_profile_selection_update (connection_preferences);
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
}

void
ario_connection_preferences_new_profile_cb (GtkWidget *widget,
                                            ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *profile;
        ArioProfile *tmp_profile;
        GSList *tmp;

        profile = (ArioProfile *) g_malloc (sizeof (ArioProfile));
        profile->name = g_strdup (_("New Profile"));
        profile->host = g_strdup ("localhost");
        profile->port = 6600;
        profile->password = NULL;

        for (tmp = connection_preferences->priv->profiles; tmp; tmp = g_slist_next (tmp)) {
                tmp_profile = (ArioProfile *) tmp->data;
                tmp_profile->current = FALSE;
        }
        profile->current = TRUE;
        connection_preferences->priv->profiles = g_slist_append (connection_preferences->priv->profiles, profile);
        ario_connection_preferences_profile_update_profiles (connection_preferences);
}

void
ario_connection_preferences_delete_profile_cb (GtkWidget *widget,
                                               ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *first_profile;

        if (g_slist_length (connection_preferences->priv->profiles) < 2)
                return;

        if (connection_preferences->priv->current_profile) {
                connection_preferences->priv->profiles = g_slist_remove (connection_preferences->priv->profiles, connection_preferences->priv->current_profile);
                ario_profiles_free (connection_preferences->priv->current_profile);
                if (connection_preferences->priv->profiles) {
                        first_profile = (ArioProfile *) connection_preferences->priv->profiles->data;
                        first_profile->current = TRUE;
                }
                ario_connection_preferences_profile_update_profiles (connection_preferences);
        }
}

void
ario_connection_preferences_connect_cb (GtkWidget *widget,
                                        ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_connect (connection_preferences->priv->mpd);
        ario_connection_preferences_sync_connection (connection_preferences);
}

void
ario_connection_preferences_disconnect_cb (GtkWidget *widget,
                                           ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_disconnect (connection_preferences->priv->mpd);
        ario_connection_preferences_sync_connection (connection_preferences);
}
