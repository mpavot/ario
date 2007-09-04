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
#include <string.h>
#include <time.h>
#include "rb-glade-helpers.h"
#include "eel-gconf-extensions.h"
#include "libmpdclient.h"
#include <glib/gi18n.h>
#include "ario-preferences.h"
#include "ario-debug.h"

static void ario_preferences_class_init (ArioPreferencesClass *klass);
static void ario_preferences_init (ArioPreferences *preferences);
static void ario_preferences_finalize (GObject *object);
static gboolean ario_preferences_window_delete_cb (GtkWidget *window,
                                                   GdkEventAny *event,
                                                   ArioPreferences *preferences);
static void ario_preferences_response_cb (GtkDialog *dialog,
                                          int response_id,
                                          ArioPreferences *preferences);
static void ario_preferences_sync (ArioPreferences *preferences);
static void ario_preferences_sync_cover (ArioPreferences *preferences);
static void ario_preferences_sync_interface (ArioPreferences *preferences);
static void ario_preferences_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void ario_preferences_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);
static void ario_preferences_show_cb (GtkWidget *widget,
                                      ArioPreferences *preferences);
void ario_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                  ArioPreferences *preferences);
void ario_preferences_amazon_country_changed_cb (GtkComboBoxEntry *combobox,
                                                 ArioPreferences *preferences);
void ario_preferences_trayicon_behavior_changed_cb (GtkComboBoxEntry *combobox,
                                                    ArioPreferences *preferences);
void ario_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                                 ArioPreferences *preferences);
static gboolean ario_preferences_update_mpd (ArioPreferences *preferences);
void ario_preferences_host_changed_cb (GtkWidget *widget,
                                       ArioPreferences *preferences);
void ario_preferences_port_changed_cb (GtkWidget *widget,
                                       ArioPreferences *preferences);
void ario_preferences_autoconnect_changed_cb (GtkWidget *widget,
                                              ArioPreferences *preferences);
void ario_preferences_authentication_changed_cb (GtkWidget *widget,
                                                 ArioPreferences *preferences);
void ario_preferences_password_changed_cb (GtkWidget *widget,
                                           ArioPreferences *preferences);
void ario_preferences_connect_cb (GtkWidget *widget,
                                  ArioPreferences *preferences);
void ario_preferences_disconnect_cb (GtkWidget *widget,
                                     ArioPreferences *preferences);
void ario_preferences_crossfadetime_changed_cb (GtkWidget *widget,
                                                ArioPreferences *preferences);
void ario_preferences_crossfade_changed_cb (GtkWidget *widget,
                                            ArioPreferences *preferences);
void ario_preferences_updatedb_button_cb (GtkWidget *widget,
                                          ArioPreferences *preferences);
void ario_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                ArioPreferences *preferences);
void ario_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                             ArioPreferences *preferences);
void ario_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                              ArioPreferences *preferences);

static const char *amazon_countries[] = {
        "com",
        "fr",
        "de",
        "uk",
        "ca",
        "jp",
        NULL
};

static const char *trayicon_behavior[] = {
        N_("Play/Pause"),       // TRAY_ICON_PLAY_PAUSE
        N_("Play next song"),   // TRAY_ICON_NEXT_SONG
        N_("Do nothing"),       // TRAY_ICON_DO_NOTHING
        NULL
};

enum
{
        PROP_0,
        PROP_MPD
};

struct ArioPreferencesPrivate
{
        GtkWidget *notebook;

        ArioMpd *mpd;

        GtkWidget *host_entry;
        GtkWidget *port_spinbutton;
        GtkWidget *authentication_checkbutton;
        GtkWidget *password_entry;
        GtkWidget *autoconnect_checkbutton;
        GtkWidget *disconnect_button;
        GtkWidget *connect_button;

        GtkWidget *crossfade_checkbutton;
        GtkWidget *crossfadetime_spinbutton;
        GtkWidget *updatedb_label;
        GtkWidget *updatedb_button;

        GtkWidget *covertree_check;
        GtkWidget *amazon_country;
        GtkWidget *proxy_check;
        GtkWidget *proxy_address_entry;
        GtkWidget *proxy_port_spinbutton;
        
        GtkWidget *showtabs_check;
        GtkWidget *trayicon_combobox;

        gboolean loading;
        gboolean sync_mpd;

        gboolean destroy;
};

static GObjectClass *parent_class = NULL;

GType
ario_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_preferences_type = 0;

        if (ario_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioPreferences),
                        0,
                        (GInstanceInitFunc) ario_preferences_init
                };

                ario_preferences_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                           "ArioPreferences",
                                                           &our_info, 0);
        }

        return ario_preferences_type;
}

static void
ario_preferences_class_init (ArioPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_preferences_finalize;
        object_class->set_property = ario_preferences_set_property;
        object_class->get_property = ario_preferences_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_preferences_init (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        preferences->priv = g_new0 (ArioPreferencesPrivate, 1);
        preferences->priv->loading = FALSE;

        g_signal_connect_object (G_OBJECT (preferences),
                                 "delete_event",
                                 G_CALLBACK (ario_preferences_window_delete_cb),
                                 preferences, 0);
        g_signal_connect_object (G_OBJECT (preferences),
                                 "response",
                                 G_CALLBACK (ario_preferences_response_cb),
                                 preferences, 0);
        g_signal_connect_object (G_OBJECT (preferences),
                                 "show",
                                 G_CALLBACK (ario_preferences_show_cb),
                                 preferences, 0);

        gtk_dialog_add_button (GTK_DIALOG (preferences),
                               GTK_STOCK_CLOSE,
                               GTK_RESPONSE_CLOSE);

        gtk_dialog_set_default_response (GTK_DIALOG (preferences),
                                         GTK_RESPONSE_CLOSE);

        gtk_window_set_title (GTK_WINDOW (preferences), _("Ario Preferences"));
        gtk_window_set_resizable (GTK_WINDOW (preferences), FALSE);

        preferences->priv->notebook = GTK_WIDGET (gtk_notebook_new ());
        gtk_container_set_border_width (GTK_CONTAINER (preferences->priv->notebook), 5);

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preferences)->vbox),
                           preferences->priv->notebook);

        gtk_container_set_border_width (GTK_CONTAINER (preferences), 5);
        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (preferences)->vbox), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (preferences), FALSE);

        preferences->priv->destroy = FALSE;
}

static void
ario_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioPreferences *preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_PREFERENCES (object));

        preferences = ARIO_PREFERENCES (object);

        g_return_if_fail (preferences->priv != NULL);

        g_free (preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_preferences_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioPreferences *preferences = ARIO_PREFERENCES (object);
        
        switch (prop_id) {
        case PROP_MPD:
                preferences->priv->mpd = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_preferences_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioPreferences *preferences = ARIO_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, preferences->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_preferences_append_connection_config (ArioPreferences *preferences,
                                           ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;

        g_return_if_fail (IS_ARIO_PREFERENCES (preferences));
        g_return_if_fail (IS_ARIO_MPD (mpd));

        xml = rb_glade_xml_new (GLADE_PATH "connection-prefs.glade",
                                "vbox",
                                preferences);

        preferences->priv->host_entry = 
                glade_xml_get_widget (xml, "host_entry");
        preferences->priv->port_spinbutton = 
                glade_xml_get_widget (xml, "port_spinbutton");
        preferences->priv->authentication_checkbutton = 
                glade_xml_get_widget (xml, "authentication_checkbutton");
        preferences->priv->password_entry = 
                glade_xml_get_widget (xml, "password_entry");
        preferences->priv->autoconnect_checkbutton = 
                glade_xml_get_widget (xml, "autoconnect_checkbutton");
        preferences->priv->disconnect_button = 
                glade_xml_get_widget (xml, "disconnect_button");
        preferences->priv->connect_button = 
                glade_xml_get_widget (xml, "connect_button");

        gtk_notebook_append_page (GTK_NOTEBOOK (preferences->priv->notebook),
                                  glade_xml_get_widget (xml, "vbox"),
                                  gtk_label_new (_("Connection")));

        g_object_unref (G_OBJECT (xml));
}

static void
ario_preferences_append_server_config (ArioPreferences *preferences,
                                       ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;

        g_return_if_fail (IS_ARIO_PREFERENCES (preferences));
        g_return_if_fail (IS_ARIO_MPD (mpd));

        xml = rb_glade_xml_new (GLADE_PATH "server-prefs.glade",
                                "vbox",
                                preferences);

        preferences->priv->crossfade_checkbutton = 
                glade_xml_get_widget (xml, "crossfade_checkbutton");
        preferences->priv->crossfadetime_spinbutton = 
                glade_xml_get_widget (xml, "crossfadetime_spinbutton");
        preferences->priv->updatedb_label = 
                glade_xml_get_widget (xml, "updatedb_label");
        preferences->priv->updatedb_button = 
                glade_xml_get_widget (xml, "updatedb_button");
                
        gtk_notebook_append_page (GTK_NOTEBOOK (preferences->priv->notebook),
                                  glade_xml_get_widget (xml, "vbox"),
                                  gtk_label_new (_("Server")));

        g_object_unref (G_OBJECT (xml));
}

static void
ario_preferences_append_ario_cover_config (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;

        g_return_if_fail (IS_ARIO_PREFERENCES (preferences));

        xml = rb_glade_xml_new (GLADE_PATH "cover-prefs.glade",
                                "covers_vbox",
                                preferences);

        preferences->priv->covertree_check =
                glade_xml_get_widget (xml, "covertree_checkbutton");
        preferences->priv->amazon_country =
                glade_xml_get_widget (xml, "amazon_country_combobox");
        preferences->priv->proxy_check =
                glade_xml_get_widget (xml, "proxy_checkbutton");
        preferences->priv->proxy_address_entry = 
                glade_xml_get_widget (xml, "proxy_address_entry");
        preferences->priv->proxy_port_spinbutton = 
                glade_xml_get_widget (xml, "proxy_port_spinbutton");
                
        list_store = gtk_list_store_new (1, G_TYPE_STRING);

        for (i = 0; amazon_countries[i] != NULL; i++) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, amazon_countries[i],
                                    -1);
        }

        gtk_combo_box_set_model (GTK_COMBO_BOX (preferences->priv->amazon_country),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (preferences->priv->amazon_country));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (preferences->priv->amazon_country), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (preferences->priv->amazon_country), renderer,
                                        "text", 0, NULL);
        
        ario_preferences_sync_cover (preferences);

        gtk_notebook_append_page (GTK_NOTEBOOK (preferences->priv->notebook),
                                  glade_xml_get_widget (xml, "covers_vbox"),
                                  gtk_label_new (_("Covers")));

        g_object_unref (G_OBJECT (xml));
}

static void
ario_preferences_append_ario_interface_config (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;

        g_return_if_fail (IS_ARIO_PREFERENCES (preferences));

        xml = rb_glade_xml_new (GLADE_PATH "interface-prefs.glade",
                                "interface_vbox",
                                preferences);

        preferences->priv->showtabs_check =
                glade_xml_get_widget (xml, "showtabs_checkbutton");
        preferences->priv->trayicon_combobox = 
                glade_xml_get_widget (xml, "trayicon_combobox");
                
        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

        for (i = 0; i < TRAY_ICON_N_BEHAVIOR; i++) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (trayicon_behavior[i]),
                                    1, i,
                                    -1);
        }

        gtk_combo_box_set_model (GTK_COMBO_BOX (preferences->priv->trayicon_combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (preferences->priv->trayicon_combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (preferences->priv->trayicon_combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (preferences->priv->trayicon_combobox), renderer,
                                        "text", 0, NULL);
        
        ario_preferences_sync_interface (preferences);

        gtk_notebook_append_page (GTK_NOTEBOOK (preferences->priv->notebook),
                                  glade_xml_get_widget (xml, "interface_vbox"),
                                  gtk_label_new (_("Interface")));

        g_object_unref (G_OBJECT (xml));
}

GtkWidget *
ario_preferences_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioPreferences *preferences;

        preferences = g_object_new (TYPE_ARIO_PREFERENCES,
                                   "mpd", mpd,
                                   NULL);

        g_return_val_if_fail (preferences->priv != NULL, NULL);

        ario_preferences_append_connection_config (preferences,
                                                   mpd);

        ario_preferences_append_server_config (preferences,
                                               mpd);

        ario_preferences_append_ario_cover_config (preferences);

        ario_preferences_append_ario_interface_config (preferences);

        ario_preferences_sync (preferences);
        ario_preferences_update_mpd (preferences);

        g_timeout_add (1000, (GSourceFunc) ario_preferences_update_mpd, preferences);

        return GTK_WIDGET (preferences);
}

static gboolean
ario_preferences_window_delete_cb (GtkWidget *window,
                                   GdkEventAny *event,
                                   ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        preferences->priv->destroy = TRUE;
        gtk_widget_hide (GTK_WIDGET (preferences));

        return TRUE;
}

static void
ario_preferences_response_cb (GtkDialog *dialog,
                              int response_id,
                              ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (response_id == GTK_RESPONSE_CLOSE) {
                preferences->priv->destroy = TRUE;
                gtk_widget_hide (GTK_WIDGET (preferences));
        }
}

static void
ario_preferences_sync (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        gchar *host;
        gint port;
        gboolean autoconnect;
        gboolean authentication;
        gchar *password;

        preferences->priv->loading = TRUE;

        host = eel_gconf_get_string (CONF_HOST);
        port = eel_gconf_get_integer (CONF_PORT);
        autoconnect = eel_gconf_get_boolean (CONF_AUTOCONNECT);
        authentication = eel_gconf_get_boolean (CONF_AUTH);
        password = eel_gconf_get_string (CONF_PASSWORD);

        if (ario_mpd_is_connected (preferences->priv->mpd)) {
                gtk_widget_set_sensitive (preferences->priv->connect_button, FALSE);
                gtk_widget_set_sensitive (preferences->priv->disconnect_button, TRUE);
        } else {
                gtk_widget_set_sensitive (preferences->priv->connect_button, TRUE);
                gtk_widget_set_sensitive (preferences->priv->disconnect_button, FALSE);
        }

        gtk_entry_set_text (GTK_ENTRY (preferences->priv->host_entry), host);
        gtk_entry_set_text (GTK_ENTRY (preferences->priv->password_entry), password);
        g_free (host);
        g_free (password);

        gtk_spin_button_set_value (GTK_SPIN_BUTTON (preferences->priv->port_spinbutton), (gdouble) port);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preferences->priv->autoconnect_checkbutton), autoconnect);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preferences->priv->authentication_checkbutton), authentication);

        preferences->priv->loading = FALSE;
}

void
ario_preferences_host_changed_cb (GtkWidget *widget,
                                  ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->loading)
                eel_gconf_set_string (CONF_HOST,
                                      gtk_entry_get_text (GTK_ENTRY (preferences->priv->host_entry)));
}

void
ario_preferences_port_changed_cb (GtkWidget *widget,
                                  ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->loading)
                eel_gconf_set_integer (CONF_PORT,
                                       (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (preferences->priv->port_spinbutton)));
}

void
ario_preferences_autoconnect_changed_cb (GtkWidget *widget,
                                         ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->loading)
                eel_gconf_set_boolean (CONF_AUTOCONNECT,
                                       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preferences->priv->autoconnect_checkbutton)));
}

void
ario_preferences_authentication_changed_cb (GtkWidget *widget,
                                            ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preferences->priv->authentication_checkbutton));
        if (!preferences->priv->loading)
                eel_gconf_set_boolean (CONF_AUTH,
                                       active);

        gtk_widget_set_sensitive (preferences->priv->password_entry, active);
}

void
ario_preferences_password_changed_cb (GtkWidget *widget,
                                      ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->loading)
                eel_gconf_set_string (CONF_PASSWORD,
                                      gtk_entry_get_text (GTK_ENTRY (preferences->priv->password_entry)));
}

void
ario_preferences_connect_cb (GtkWidget *widget,
                             ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_connect (preferences->priv->mpd);
        ario_preferences_sync (preferences);
}

void
ario_preferences_disconnect_cb (GtkWidget *widget,
                                ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_disconnect (preferences->priv->mpd);
        ario_preferences_sync (preferences);
}

static void
ario_preferences_show_cb (GtkWidget *widget,
                          ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_preferences_sync (preferences);
}

void
ario_preferences_crossfadetime_changed_cb (GtkWidget *widget,
                                           ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->sync_mpd && !preferences->priv->loading) {
                ario_mpd_set_crossfadetime (preferences->priv->mpd,
                                       gtk_spin_button_get_value (GTK_SPIN_BUTTON (preferences->priv->crossfadetime_spinbutton)));
        }
}

void
ario_preferences_crossfade_changed_cb (GtkWidget *widget,
                                       ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->sync_mpd && !preferences->priv->loading) {
                if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preferences->priv->crossfade_checkbutton)))
                        ario_mpd_set_crossfadetime (preferences->priv->mpd,
                                               1);
                else
                        ario_mpd_set_crossfadetime (preferences->priv->mpd,
                                               0);
        }
}

void
ario_preferences_updatedb_button_cb (GtkWidget *widget,
                                     ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_update_db (preferences->priv->mpd);
}

void
ario_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                           ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->loading)
                eel_gconf_set_string (CONF_PROXY_ADDRESS,
                                      gtk_entry_get_text (GTK_ENTRY (preferences->priv->proxy_address_entry)));
}

void
ario_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                        ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        if (!preferences->priv->loading)
                eel_gconf_set_integer (CONF_PROXY_PORT,
                                       (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (preferences->priv->proxy_port_spinbutton)));
}

gboolean
ario_preferences_update_mpd (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        int crossfadetime;
        int state;
        gboolean updating;
        long last_update;
        gchar *last_update_char;

        if (preferences->priv->destroy) {
                gtk_widget_destroy (GTK_WIDGET (preferences));
                return FALSE;
        }

        state = ario_mpd_get_current_state (preferences->priv->mpd);
        updating = ario_mpd_get_updating (preferences->priv->mpd);

        if (state == MPD_STATUS_STATE_UNKNOWN) {
                crossfadetime = 0;
                last_update_char = _("Not connected");
        } else {
                crossfadetime = ario_mpd_get_crossfadetime (preferences->priv->mpd);

                if (updating) {
                        last_update_char = _("Updating...");
                } else {
                        last_update = (long) ario_mpd_get_last_update (preferences->priv->mpd);
                        last_update_char = ctime (&last_update);
                        /* Remove the new line */
                        last_update_char[strlen (last_update_char)-1] = '\0';
                }
        }

        preferences->priv->sync_mpd = TRUE;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preferences->priv->crossfade_checkbutton), (crossfadetime != 0));
        gtk_widget_set_sensitive (preferences->priv->crossfadetime_spinbutton, (crossfadetime != 0));
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (preferences->priv->crossfadetime_spinbutton), (gdouble) crossfadetime);

        gtk_widget_set_sensitive (preferences->priv->updatedb_button, (!updating && state != MPD_STATUS_STATE_UNKNOWN));

        preferences->priv->sync_mpd = FALSE;
        
        gtk_label_set_label (GTK_LABEL (preferences->priv->updatedb_label), last_update_char);

        return TRUE;
}

void
ario_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                             ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_boolean (CONF_COVER_TREE_HIDDEN,
                               !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preferences->priv->covertree_check)));
}

void
ario_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                         ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean active;
        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preferences->priv->proxy_check));
        eel_gconf_set_boolean (CONF_USE_PROXY,
                               active);

        gtk_widget_set_sensitive (preferences->priv->proxy_address_entry, active);
        gtk_widget_set_sensitive (preferences->priv->proxy_port_spinbutton, active);
}

void
ario_preferences_amazon_country_changed_cb (GtkComboBoxEntry *combobox,
                                            ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (preferences->priv->amazon_country));

        eel_gconf_set_string (CONF_COVER_AMAZON_COUNTRY, 
                              amazon_countries[i]);
}

void
ario_preferences_trayicon_behavior_changed_cb (GtkComboBoxEntry *combobox,
                                               ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (preferences->priv->trayicon_combobox));

        eel_gconf_set_integer (CONF_TRAYICON_BEHAVIOR, 
                               i);
}

void
ario_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                            ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_boolean (CONF_SHOW_TABS,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preferences->priv->showtabs_check)));
}

static void
ario_preferences_sync_cover (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;
        char *current_country;
        char *proxy_address;
        int proxy_port;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preferences->priv->covertree_check), 
                                      !eel_gconf_get_boolean (CONF_COVER_TREE_HIDDEN));

        current_country = eel_gconf_get_string (CONF_COVER_AMAZON_COUNTRY);
        if (!current_country)
                current_country = g_strdup("com");
        for (i = 0; amazon_countries[i] != NULL; i++) {
                if (!strcmp (amazon_countries[i], current_country)) {
                        gtk_combo_box_set_active (GTK_COMBO_BOX (preferences->priv->amazon_country), i);
                        break;
                }
                gtk_combo_box_set_active (GTK_COMBO_BOX (preferences->priv->amazon_country), 0);
        }

        g_free (current_country);
        
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preferences->priv->proxy_check), 
                                      eel_gconf_get_boolean (CONF_USE_PROXY));
        
        proxy_address = eel_gconf_get_string (CONF_PROXY_ADDRESS);
        if (!proxy_address)
                proxy_address = g_strdup("192.168.0.1");
        proxy_port = eel_gconf_get_integer (CONF_PROXY_PORT);
        if (proxy_port == 0)
                proxy_port = 8080;
        
        gtk_entry_set_text (GTK_ENTRY (preferences->priv->proxy_address_entry), proxy_address);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (preferences->priv->proxy_port_spinbutton), (gdouble) proxy_port);
        g_free(proxy_address);
}

static void
ario_preferences_sync_interface (ArioPreferences *preferences)
{
        ARIO_LOG_FUNCTION_START

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preferences->priv->showtabs_check), 
                                      eel_gconf_get_boolean (CONF_SHOW_TABS));

        gtk_combo_box_set_active (GTK_COMBO_BOX (preferences->priv->trayicon_combobox),
                                  eel_gconf_get_integer (CONF_TRAYICON_BEHAVIOR));
}
