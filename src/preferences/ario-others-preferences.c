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

#include "preferences/ario-others-preferences.h"
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

static void ario_others_preferences_sync_others (ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                                                        ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_hideonclose_check_changed_cb (GtkCheckButton *butt,
                                                                           ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_oneinstance_check_changed_cb (GtkCheckButton *butt,
                                                                           ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                                       ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                                                    ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                                                     ArioOthersPreferences *others_preferences);
G_MODULE_EXPORT void ario_others_preferences_playlist_position_changed_cb (GtkRadioButton *butt,
                                                                           ArioOthersPreferences *others_preferences);

struct ArioOthersPreferencesPrivate
{
        GtkWidget *showtabs_check;
        GtkWidget *hideonclose_check;
        GtkWidget *oneinstance_check;

        GtkWidget *proxy_check;
        GtkWidget *proxy_address_entry;
        GtkWidget *proxy_port_spinbutton;

        GtkWidget *below_radiobutton;
        GtkWidget *right_radiobutton;
        GtkWidget *in_radiobutton;
};

G_DEFINE_TYPE_WITH_CODE (ArioOthersPreferences, ario_others_preferences, GTK_TYPE_BOX, G_ADD_PRIVATE(ArioOthersPreferences))

static void
ario_others_preferences_class_init (ArioOthersPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START;
}

static void
ario_others_preferences_init (ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        others_preferences->priv = ario_others_preferences_get_instance_private (others_preferences);
}

GtkWidget *
ario_others_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioOthersPreferences *others_preferences;
        GtkBuilder *builder;

        others_preferences = g_object_new (TYPE_ARIO_OTHERS_PREFERENCES, NULL);

        g_return_val_if_fail (others_preferences->priv != NULL, NULL);

        gtk_orientable_set_orientation (GTK_ORIENTABLE (others_preferences), GTK_ORIENTATION_VERTICAL);

        builder = gtk_builder_helpers_new (UI_PATH "others-prefs.ui",
                                           others_preferences);

        others_preferences->priv->showtabs_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "showtabs_checkbutton"));
        others_preferences->priv->hideonclose_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "hideonclose_checkbutton"));
        others_preferences->priv->oneinstance_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "instance_checkbutton"));
        others_preferences->priv->proxy_check =
                GTK_WIDGET (gtk_builder_get_object (builder, "proxy_checkbutton"));
        others_preferences->priv->proxy_address_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "proxy_address_entry"));
        others_preferences->priv->proxy_port_spinbutton =
                GTK_WIDGET (gtk_builder_get_object (builder, "proxy_port_spinbutton"));
        others_preferences->priv->below_radiobutton =
                GTK_WIDGET (gtk_builder_get_object (builder, "pl_below_radiobutton"));
        others_preferences->priv->right_radiobutton =
                GTK_WIDGET (gtk_builder_get_object (builder, "pl_right_radiobutton"));
        others_preferences->priv->in_radiobutton =
                GTK_WIDGET (gtk_builder_get_object (builder, "pl_in_radiobutton"));

        gtk_image_set_from_icon_name (GTK_IMAGE (gtk_builder_get_object (builder, "pl_below_image")),
                                      "pl-below", GTK_ICON_SIZE_LARGE_TOOLBAR);

        gtk_image_set_from_icon_name (GTK_IMAGE (gtk_builder_get_object (builder, "pl_right_image")),
                                      "pl-right", GTK_ICON_SIZE_LARGE_TOOLBAR);

        gtk_image_set_from_icon_name (GTK_IMAGE (gtk_builder_get_object (builder, "pl_in_image")),
                                      "pl-inside", GTK_ICON_SIZE_LARGE_TOOLBAR);

        gtk_builder_helpers_boldify_label (builder, "interface_label");
        gtk_builder_helpers_boldify_label (builder, "proxy_frame_label");

        ario_others_preferences_sync_others (others_preferences);

        gtk_box_pack_start (GTK_BOX (others_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "others_vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (others_preferences);
}

static void
ario_others_preferences_sync_others (ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        const char *proxy_address;
        int proxy_port;
        int playlist_posistion;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->showtabs_check),
                                      ario_conf_get_boolean (PREF_SHOW_TABS, PREF_SHOW_TABS_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->hideonclose_check),
                                      ario_conf_get_boolean (PREF_HIDE_ON_CLOSE, PREF_HIDE_ON_CLOSE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->oneinstance_check),
                                      ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->proxy_check),
                                      ario_conf_get_boolean (PREF_USE_PROXY, PREF_USE_PROXY_DEFAULT));

        proxy_address = ario_conf_get_string (PREF_PROXY_ADDRESS, PREF_PROXY_ADDRESS_DEFAULT);
        proxy_port = ario_conf_get_integer (PREF_PROXY_PORT, PREF_PROXY_PORT_DEFAULT);

        gtk_entry_set_text (GTK_ENTRY (others_preferences->priv->proxy_address_entry), proxy_address);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (others_preferences->priv->proxy_port_spinbutton), (gdouble) proxy_port);

        playlist_posistion = ario_conf_get_integer (PREF_PLAYLIST_POSITION, PREF_PLAYLIST_POSITION_DEFAULT);
        if (playlist_posistion == PLAYLIST_POSITION_INSIDE)
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->in_radiobutton), TRUE); 
        else if (playlist_posistion == PLAYLIST_POSITION_RIGHT)
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->right_radiobutton), TRUE); 
        else
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (others_preferences->priv->below_radiobutton), TRUE); 
}

void
ario_others_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                                   ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_conf_set_boolean (PREF_SHOW_TABS,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (others_preferences->priv->showtabs_check)));
}

void
ario_others_preferences_hideonclose_check_changed_cb (GtkCheckButton *butt,
                                                      ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_conf_set_boolean (PREF_HIDE_ON_CLOSE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (others_preferences->priv->hideonclose_check)));
}

void
ario_others_preferences_oneinstance_check_changed_cb (GtkCheckButton *butt,
                                                      ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_conf_set_boolean (PREF_ONE_INSTANCE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (others_preferences->priv->oneinstance_check)));
}

void
ario_others_preferences_proxy_address_changed_cb (GtkWidget *widget,
                                                  ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_conf_set_string (PREF_PROXY_ADDRESS,
                              gtk_entry_get_text (GTK_ENTRY (others_preferences->priv->proxy_address_entry)));
}

void
ario_others_preferences_proxy_port_changed_cb (GtkWidget *widget,
                                               ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        gdouble port = gtk_spin_button_get_value (GTK_SPIN_BUTTON (others_preferences->priv->proxy_port_spinbutton));
        ario_conf_set_integer (PREF_PROXY_PORT, (int) port);
}

void
ario_others_preferences_proxy_check_changed_cb (GtkCheckButton *butt,
                                                ArioOthersPreferences *others_preferences)
{
        ARIO_LOG_FUNCTION_START;
        gboolean active;
        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (others_preferences->priv->proxy_check));
        ario_conf_set_boolean (PREF_USE_PROXY,
                               active);

        gtk_widget_set_sensitive (others_preferences->priv->proxy_address_entry, active);
        gtk_widget_set_sensitive (others_preferences->priv->proxy_port_spinbutton, active);
}

void
ario_others_preferences_playlist_position_changed_cb (GtkRadioButton *butt,
                                                      ArioOthersPreferences *others_preferences)
{
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (others_preferences->priv->in_radiobutton)))
                ario_conf_set_integer (PREF_PLAYLIST_POSITION, PLAYLIST_POSITION_INSIDE);
        else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (others_preferences->priv->right_radiobutton)))
                ario_conf_set_integer (PREF_PLAYLIST_POSITION, PLAYLIST_POSITION_RIGHT);
        else
                ario_conf_set_integer (PREF_PLAYLIST_POSITION, PLAYLIST_POSITION_BELOW);
}

