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

#include "preferences/ario-connection-preferences.h"
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
#include "ario-profiles.h"
#include "servers/ario-server.h"
#include "widgets/ario-connection-widget.h"

static void ario_connection_preferences_sync_connection (ArioConnectionPreferences *connection_preferences);
G_MODULE_EXPORT void ario_connection_preferences_autoconnect_changed_cb (GtkWidget *widget,
                                                                         ArioConnectionPreferences *connection_preferences);
G_MODULE_EXPORT void ario_connection_preferences_connect_cb (GtkWidget *widget,
                                                             ArioConnectionPreferences *connection_preferences);
G_MODULE_EXPORT void ario_connection_preferences_disconnect_cb (GtkWidget *widget,
                                                                ArioConnectionPreferences *connection_preferences);

struct ArioConnectionPreferencesPrivate
{
        GtkWidget *autoconnect_checkbutton;
        GtkWidget *disconnect_button;
        GtkWidget *connect_button;

        gboolean loading;
};

#define ARIO_CONNECTION_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_CONNECTION_PREFERENCES, ArioConnectionPreferencesPrivate))
G_DEFINE_TYPE (ArioConnectionPreferences, ario_connection_preferences, GTK_TYPE_VBOX)

static void
ario_connection_preferences_class_init (ArioConnectionPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        g_type_class_add_private (klass, sizeof (ArioConnectionPreferencesPrivate));
}

static void
ario_connection_preferences_init (ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START;
        connection_preferences->priv = ARIO_CONNECTION_PREFERENCES_GET_PRIVATE (connection_preferences);

        connection_preferences->priv->loading = FALSE;
}

static void
ario_connection_preferences_profile_changed_cb (ArioConnectionWidget *connection_widget,
                                                ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_reconnect ();
        ario_connection_preferences_sync_connection (connection_preferences);
}

GtkWidget *
ario_connection_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START;
        GtkBuilder *builder;
        ArioConnectionPreferences *connection_preferences;
        GtkWidget *alignment, *connection_widget;

        connection_preferences = g_object_new (TYPE_ARIO_CONNECTION_PREFERENCES,
                                               NULL);

        g_return_val_if_fail (connection_preferences->priv != NULL, NULL);

        builder = gtk_builder_helpers_new (UI_PATH "connection-prefs.ui",
                                           connection_preferences);

        alignment = 
                GTK_WIDGET (gtk_builder_get_object (builder, "alignment"));
        connection_preferences->priv->autoconnect_checkbutton = 
                GTK_WIDGET (gtk_builder_get_object (builder, "autoconnect_checkbutton"));
        connection_preferences->priv->disconnect_button = 
                GTK_WIDGET (gtk_builder_get_object (builder, "disconnect_button"));
        connection_preferences->priv->connect_button = 
                GTK_WIDGET (gtk_builder_get_object (builder, "connect_button"));

        gtk_builder_helpers_boldify_label (builder, "connection_label");

        connection_widget = ario_connection_widget_new ();
        gtk_container_add (GTK_CONTAINER (alignment), connection_widget);
        g_signal_connect (connection_widget,
                          "profile_changed",
                          G_CALLBACK (ario_connection_preferences_profile_changed_cb),
                          connection_preferences);

        ario_connection_preferences_sync_connection (connection_preferences);

        gtk_box_pack_start (GTK_BOX (connection_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (connection_preferences);
}

static void
ario_connection_preferences_sync_connection (ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START;
        gboolean autoconnect;

        connection_preferences->priv->loading = TRUE;

        autoconnect = ario_conf_get_boolean (PREF_AUTOCONNECT, PREF_AUTOCONNECT_DEFAULT);

        if (ario_server_is_connected ()) {
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
ario_connection_preferences_autoconnect_changed_cb (GtkWidget *widget,
                                                    ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START;
        if (!connection_preferences->priv->loading)
                ario_conf_set_boolean (PREF_AUTOCONNECT,
                                       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (connection_preferences->priv->autoconnect_checkbutton)));
}

void
ario_connection_preferences_connect_cb (GtkWidget *widget,
                                        ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_connect ();
        ario_connection_preferences_sync_connection (connection_preferences);
}

void
ario_connection_preferences_disconnect_cb (GtkWidget *widget,
                                           ArioConnectionPreferences *connection_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_disconnect ();
        ario_connection_preferences_sync_connection (connection_preferences);
}
