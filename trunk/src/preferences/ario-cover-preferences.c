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
#include "preferences/ario-cover-preferences.h"
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/eel-gconf-extensions.h"
#include "ario-avahi.h"
#include "ario-debug.h"

static void ario_cover_preferences_class_init (ArioCoverPreferencesClass *klass);
static void ario_cover_preferences_init (ArioCoverPreferences *cover_preferences);
static void ario_cover_preferences_finalize (GObject *object);
static void ario_cover_preferences_sync_cover (ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                                      ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                                                   ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                                        ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                                                    ArioCoverPreferences *cover_preferences);
G_MODULE_EXPORT void ario_cover_preferences_amazon_country_changed_cb (GtkComboBoxEntry *combobox,
                                                                       ArioCoverPreferences *cover_preferences);

struct ArioCoverPreferencesPrivate
{
        GtkWidget *covertree_check;
        GtkWidget *amazon_country;
        GtkWidget *proxy_check;
        GtkWidget *proxy_address_entry;
        GtkWidget *proxy_port_spinbutton;
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

static GObjectClass *parent_class = NULL;

GType
ario_cover_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_cover_preferences_type = 0;

        if (ario_cover_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioCoverPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_cover_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioCoverPreferences),
                        0,
                        (GInstanceInitFunc) ario_cover_preferences_init
                };

                ario_cover_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                      "ArioCoverPreferences",
                                                                      &our_info, 0);
        }

        return ario_cover_preferences_type;
}

static void
ario_cover_preferences_class_init (ArioCoverPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_cover_preferences_finalize;
}

static void
ario_cover_preferences_init (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        cover_preferences->priv = g_new0 (ArioCoverPreferencesPrivate, 1);
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

        cover_preferences = g_object_new (TYPE_ARIO_COVER_PREFERENCES, NULL);

        g_return_val_if_fail (cover_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "cover-prefs.glade",
                                "covers_vbox",
                                cover_preferences);

        cover_preferences->priv->covertree_check =
                glade_xml_get_widget (xml, "covertree_checkbutton");
        cover_preferences->priv->amazon_country =
                glade_xml_get_widget (xml, "amazon_country_combobox");
        cover_preferences->priv->proxy_check =
                glade_xml_get_widget (xml, "proxy_checkbutton");
        cover_preferences->priv->proxy_address_entry = 
                glade_xml_get_widget (xml, "proxy_address_entry");
        cover_preferences->priv->proxy_port_spinbutton = 
                glade_xml_get_widget (xml, "proxy_port_spinbutton");

        rb_glade_boldify_label (xml, "cover_frame_label");
        rb_glade_boldify_label (xml, "proxy_frame_label");
        
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

        ario_cover_preferences_sync_cover (cover_preferences);

        gtk_box_pack_start (GTK_BOX (cover_preferences), glade_xml_get_widget (xml, "covers_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));
        return GTK_WIDGET (cover_preferences);
}

static void
ario_cover_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverPreferences *cover_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_COVER_PREFERENCES (object));

        cover_preferences = ARIO_COVER_PREFERENCES (object);

        g_return_if_fail (cover_preferences->priv != NULL);

        g_free (cover_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_cover_preferences_sync_cover (ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;
        char *current_country;
        char *proxy_address;
        int proxy_port;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->covertree_check), 
                                      !eel_gconf_get_boolean (CONF_COVER_TREE_HIDDEN, FALSE));

        current_country = eel_gconf_get_string (CONF_COVER_AMAZON_COUNTRY, "com");
        for (i = 0; amazon_countries[i]; ++i) {
                if (!strcmp (amazon_countries[i], current_country)) {
                        gtk_combo_box_set_active (GTK_COMBO_BOX (cover_preferences->priv->amazon_country), i);
                        break;
                }
                gtk_combo_box_set_active (GTK_COMBO_BOX (cover_preferences->priv->amazon_country), 0);
        }

        g_free (current_country);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->proxy_check), 
                                      eel_gconf_get_boolean (CONF_USE_PROXY, FALSE));

        proxy_address = eel_gconf_get_string (CONF_PROXY_ADDRESS, "192.168.0.1");
        proxy_port = eel_gconf_get_integer (CONF_PROXY_PORT, 8080);

        gtk_entry_set_text (GTK_ENTRY (cover_preferences->priv->proxy_address_entry), proxy_address);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (cover_preferences->priv->proxy_port_spinbutton), (gdouble) proxy_port);
        g_free(proxy_address);
}

void
ario_cover_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                 ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_string (CONF_PROXY_ADDRESS,
                              gtk_entry_get_text (GTK_ENTRY (cover_preferences->priv->proxy_address_entry)));
}

void
ario_cover_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                              ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_integer (CONF_PROXY_PORT,
                               (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (cover_preferences->priv->proxy_port_spinbutton)));
}

void
ario_cover_preferences_covertree_check_changed_cb (GtkCheckButton *butt,
                                                   ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_boolean (CONF_COVER_TREE_HIDDEN,
                               !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->covertree_check)));
}

void
ario_cover_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                               ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean active;
        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cover_preferences->priv->proxy_check));
        eel_gconf_set_boolean (CONF_USE_PROXY,
                               active);

        gtk_widget_set_sensitive (cover_preferences->priv->proxy_address_entry, active);
        gtk_widget_set_sensitive (cover_preferences->priv->proxy_port_spinbutton, active);
}

void
ario_cover_preferences_amazon_country_changed_cb (GtkComboBoxEntry *combobox,
                                                  ArioCoverPreferences *cover_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (cover_preferences->priv->amazon_country));

        eel_gconf_set_string (CONF_COVER_AMAZON_COUNTRY, 
                              amazon_countries[i]);
}

