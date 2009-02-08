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

#include "preferences/ario-proxy-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-preferences.h"
#include "lib/gtk-builder-helpers.h"
#include "lib/ario-conf.h"
#include "ario-debug.h"

static void ario_proxy_preferences_sync (ArioProxyPreferences *proxy_preferences);
G_MODULE_EXPORT void ario_proxy_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                                      ArioProxyPreferences *proxy_preferences);
G_MODULE_EXPORT void ario_proxy_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                                                   ArioProxyPreferences *proxy_preferences);
G_MODULE_EXPORT void ario_proxy_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                                                    ArioProxyPreferences *proxy_preferences);


struct ArioProxyPreferencesPrivate
{
        GtkWidget *proxy_check;
        GtkWidget *proxy_address_entry;
        GtkWidget *proxy_port_spinbutton;
};

#define ARIO_PROXY_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PROXY_PREFERENCES, ArioProxyPreferencesPrivate))
G_DEFINE_TYPE (ArioProxyPreferences, ario_proxy_preferences, GTK_TYPE_VBOX)

static void
ario_proxy_preferences_class_init (ArioProxyPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioProxyPreferencesPrivate));
}

static void
ario_proxy_preferences_init (ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        proxy_preferences->priv = ARIO_PROXY_PREFERENCES_GET_PRIVATE (proxy_preferences);
}

GtkWidget *
ario_proxy_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        GtkBuilder *builder;
        ArioProxyPreferences *proxy_preferences;

        proxy_preferences = g_object_new (TYPE_ARIO_PROXY_PREFERENCES, NULL);

        g_return_val_if_fail (proxy_preferences->priv != NULL, NULL);

        builder = gtk_builder_helpers_new (UI_PATH "proxy-prefs.ui",
                                           proxy_preferences);

        proxy_preferences->priv->proxy_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "proxy_checkbutton"));
        proxy_preferences->priv->proxy_address_entry = 
                GTK_WIDGET (gtk_builder_get_object (builder, "proxy_address_entry"));
        proxy_preferences->priv->proxy_port_spinbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "proxy_port_spinbutton"));

        gtk_builder_helpers_boldify_label (builder, "proxy_frame_label");

        ario_proxy_preferences_sync (proxy_preferences);

        gtk_box_pack_start (GTK_BOX (proxy_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "proxy_vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);
        return GTK_WIDGET (proxy_preferences);
}

static void
ario_proxy_preferences_sync (ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        char *proxy_address;
        int proxy_port;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proxy_preferences->priv->proxy_check), 
                                      ario_conf_get_boolean (PREF_USE_PROXY, PREF_USE_PROXY_DEFAULT));

        proxy_address = ario_conf_get_string (PREF_PROXY_ADDRESS, PREF_PROXY_ADDRESS_DEFAULT);
        proxy_port = ario_conf_get_integer (PREF_PROXY_PORT, PREF_PROXY_PORT_DEFAULT);

        gtk_entry_set_text (GTK_ENTRY (proxy_preferences->priv->proxy_address_entry), proxy_address);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (proxy_preferences->priv->proxy_port_spinbutton), (gdouble) proxy_port);
        g_free(proxy_address);
}

void
ario_proxy_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                 ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_string (PREF_PROXY_ADDRESS,
                              gtk_entry_get_text (GTK_ENTRY (proxy_preferences->priv->proxy_address_entry)));
}

void
ario_proxy_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                              ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_integer (PREF_PROXY_PORT,
                               (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (proxy_preferences->priv->proxy_port_spinbutton)));
}

void
ario_proxy_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                               ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean active;
        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (proxy_preferences->priv->proxy_check));
        ario_conf_set_boolean (PREF_USE_PROXY,
                               active);

        gtk_widget_set_sensitive (proxy_preferences->priv->proxy_address_entry, active);
        gtk_widget_set_sensitive (proxy_preferences->priv->proxy_port_spinbutton, active);
}

