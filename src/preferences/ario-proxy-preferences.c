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
#include "preferences/ario-proxy-preferences.h"
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "ario-debug.h"

static void ario_proxy_preferences_class_init (ArioProxyPreferencesClass *klass);
static void ario_proxy_preferences_init (ArioProxyPreferences *proxy_preferences);
static void ario_proxy_preferences_finalize (GObject *object);
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

static GObjectClass *parent_class = NULL;

GType
ario_proxy_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_proxy_preferences_type = 0;

        if (ario_proxy_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioProxyPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_proxy_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioProxyPreferences),
                        0,
                        (GInstanceInitFunc) ario_proxy_preferences_init
                };

                ario_proxy_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                      "ArioProxyPreferences",
                                                                      &our_info, 0);
        }

        return ario_proxy_preferences_type;
}

static void
ario_proxy_preferences_class_init (ArioProxyPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_proxy_preferences_finalize;
}

static void
ario_proxy_preferences_init (ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        proxy_preferences->priv = g_new0 (ArioProxyPreferencesPrivate, 1);
}

GtkWidget *
ario_proxy_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        ArioProxyPreferences *proxy_preferences;

        proxy_preferences = g_object_new (TYPE_ARIO_PROXY_PREFERENCES, NULL);

        g_return_val_if_fail (proxy_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "proxy-prefs.glade",
                                "proxy_vbox",
                                proxy_preferences);

        proxy_preferences->priv->proxy_check =
                glade_xml_get_widget (xml, "proxy_checkbutton");
        proxy_preferences->priv->proxy_address_entry = 
                glade_xml_get_widget (xml, "proxy_address_entry");
        proxy_preferences->priv->proxy_port_spinbutton = 
                glade_xml_get_widget (xml, "proxy_port_spinbutton");

        rb_glade_boldify_label (xml, "proxy_frame_label");

        ario_proxy_preferences_sync (proxy_preferences);

        gtk_box_pack_start (GTK_BOX (proxy_preferences), glade_xml_get_widget (xml, "proxy_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));
        return GTK_WIDGET (proxy_preferences);
}

static void
ario_proxy_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioProxyPreferences *proxy_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_PROXY_PREFERENCES (object));

        proxy_preferences = ARIO_PROXY_PREFERENCES (object);

        g_return_if_fail (proxy_preferences->priv != NULL);

        g_free (proxy_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_proxy_preferences_sync (ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        char *proxy_address;
        int proxy_port;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proxy_preferences->priv->proxy_check), 
                                      ario_conf_get_boolean (CONF_USE_PROXY, FALSE));

        proxy_address = ario_conf_get_string (CONF_PROXY_ADDRESS, "192.168.0.1");
        proxy_port = ario_conf_get_integer (CONF_PROXY_PORT, 8080);

        gtk_entry_set_text (GTK_ENTRY (proxy_preferences->priv->proxy_address_entry), proxy_address);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (proxy_preferences->priv->proxy_port_spinbutton), (gdouble) proxy_port);
        g_free(proxy_address);
}

void
ario_proxy_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                 ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_string (CONF_PROXY_ADDRESS,
                              gtk_entry_get_text (GTK_ENTRY (proxy_preferences->priv->proxy_address_entry)));
}

void
ario_proxy_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                              ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_integer (CONF_PROXY_PORT,
                               (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (proxy_preferences->priv->proxy_port_spinbutton)));
}

void
ario_proxy_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                               ArioProxyPreferences *proxy_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean active;
        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (proxy_preferences->priv->proxy_check));
        ario_conf_set_boolean (CONF_USE_PROXY,
                               active);

        gtk_widget_set_sensitive (proxy_preferences->priv->proxy_address_entry, active);
        gtk_widget_set_sensitive (proxy_preferences->priv->proxy_port_spinbutton, active);
}

