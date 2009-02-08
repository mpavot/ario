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

#include "preferences/ario-interface-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "ario-debug.h"

static void ario_interface_preferences_sync_interface (ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                                                           ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_hideonclose_check_changed_cb (GtkCheckButton *butt,
                                                                              ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_oneinstance_check_changed_cb (GtkCheckButton *butt,
                                                                              ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_track_toogled_cb (GtkCheckButton *butt,
                                                                  ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_title_toogled_cb (GtkCheckButton *butt,
                                                                  ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_artist_toogled_cb (GtkCheckButton *butt,
                                                                   ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_album_toogled_cb (GtkCheckButton *butt,
                                                                  ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_genre_toogled_cb (GtkCheckButton *butt,
                                                                  ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_duration_toogled_cb (GtkCheckButton *butt,
                                                                     ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_file_toogled_cb (GtkCheckButton *butt,
                                                                 ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_date_toogled_cb (GtkCheckButton *butt,
                                                                 ArioInterfacePreferences *interface_preferences);
G_MODULE_EXPORT void ario_interface_preferences_autoscroll_toogled_cb (GtkCheckButton *butt,
                                                                       ArioInterfacePreferences *interface_preferences);

struct ArioInterfacePreferencesPrivate
{
        GtkWidget *showtabs_check;
        GtkWidget *hideonclose_check;
        GtkWidget *oneinstance_check;

        GtkWidget *track_checkbutton;
        GtkWidget *title_checkbutton;
        GtkWidget *artist_checkbutton;
        GtkWidget *album_checkbutton;
        GtkWidget *genre_checkbutton;
        GtkWidget *duration_checkbutton;
        GtkWidget *file_checkbutton;
        GtkWidget *date_checkbutton;
        GtkWidget *autoscroll_checkbutton;
};

#define ARIO_INTERFACE_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_INTERFACE_PREFERENCES, ArioInterfacePreferencesPrivate))
G_DEFINE_TYPE (ArioInterfacePreferences, ario_interface_preferences, GTK_TYPE_VBOX)

static void
ario_interface_preferences_class_init (ArioInterfacePreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioInterfacePreferencesPrivate));
}

static void
ario_interface_preferences_init (ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        interface_preferences->priv = ARIO_INTERFACE_PREFERENCES_GET_PRIVATE (interface_preferences);
}

GtkWidget *
ario_interface_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioInterfacePreferences *interface_preferences;
        GladeXML *xml;

        interface_preferences = g_object_new (TYPE_ARIO_INTERFACE_PREFERENCES, NULL);

        g_return_val_if_fail (interface_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "interface-prefs.glade",
                                "interface_vbox",
                                interface_preferences);

        interface_preferences->priv->showtabs_check =
                glade_xml_get_widget (xml, "showtabs_checkbutton");
        interface_preferences->priv->hideonclose_check =
                glade_xml_get_widget (xml, "hideonclose_checkbutton");
        interface_preferences->priv->oneinstance_check =
                glade_xml_get_widget (xml, "instance_checkbutton");
        interface_preferences->priv->track_checkbutton = 
                glade_xml_get_widget (xml, "track_checkbutton");
        interface_preferences->priv->title_checkbutton = 
                glade_xml_get_widget (xml, "title_checkbutton");
        interface_preferences->priv->artist_checkbutton = 
                glade_xml_get_widget (xml, "artist_checkbutton");
        interface_preferences->priv->album_checkbutton = 
                glade_xml_get_widget (xml, "album_checkbutton");
        interface_preferences->priv->genre_checkbutton = 
                glade_xml_get_widget (xml, "genre_checkbutton");
        interface_preferences->priv->duration_checkbutton = 
                glade_xml_get_widget (xml, "duration_checkbutton");
        interface_preferences->priv->file_checkbutton = 
                glade_xml_get_widget (xml, "file_checkbutton");
        interface_preferences->priv->date_checkbutton = 
                glade_xml_get_widget (xml, "date_checkbutton");
        interface_preferences->priv->autoscroll_checkbutton = 
                glade_xml_get_widget (xml, "autoscroll_checkbutton");

        rb_glade_boldify_label (xml, "interface_label");
        rb_glade_boldify_label (xml, "playlist_label");

        ario_interface_preferences_sync_interface (interface_preferences);

        gtk_box_pack_start (GTK_BOX (interface_preferences), glade_xml_get_widget (xml, "interface_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (interface_preferences);
}

static void
ario_interface_preferences_sync_interface (ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->showtabs_check),
                                      ario_conf_get_boolean (PREF_SHOW_TABS, PREF_SHOW_TABS_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->hideonclose_check),
                                      ario_conf_get_boolean (PREF_HIDE_ON_CLOSE, PREF_HIDE_ON_CLOSE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->oneinstance_check),
                                      ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->track_checkbutton),
                                      ario_conf_get_boolean (PREF_TRACK_COLUMN_VISIBLE, PREF_TRACK_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->title_checkbutton),
                                      ario_conf_get_boolean (PREF_TITLE_COLUMN_VISIBLE, PREF_TITLE_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->artist_checkbutton),
                                      ario_conf_get_boolean (PREF_ARTIST_COLUMN_VISIBLE, PREF_ARTIST_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->album_checkbutton),
                                      ario_conf_get_boolean (PREF_ALBUM_COLUMN_VISIBLE, PREF_ALBUM_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->genre_checkbutton),
                                      ario_conf_get_boolean (PREF_GENRE_COLUMN_VISIBLE, PREF_GENRE_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->duration_checkbutton),
                                      ario_conf_get_boolean (PREF_DURATION_COLUMN_VISIBLE, PREF_DURATION_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->file_checkbutton),
                                      ario_conf_get_boolean (PREF_FILE_COLUMN_VISIBLE, PREF_FILE_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->date_checkbutton),
                                      ario_conf_get_boolean (PREF_DATE_COLUMN_VISIBLE, PREF_DATE_COLUMN_VISIBLE_DEFAULT));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->autoscroll_checkbutton),
                                      ario_conf_get_boolean (PREF_PLAYLIST_AUTOSCROLL, PREF_PLAYLIST_AUTOSCROLL_DEFAULT));
}

void
ario_interface_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                                      ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_SHOW_TABS,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->showtabs_check)));
}

void
ario_interface_preferences_hideonclose_check_changed_cb (GtkCheckButton *butt,
                                                         ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_HIDE_ON_CLOSE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->hideonclose_check)));
}

void
ario_interface_preferences_oneinstance_check_changed_cb (GtkCheckButton *butt,
                                                         ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_ONE_INSTANCE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->oneinstance_check)));
}

void
ario_interface_preferences_track_toogled_cb (GtkCheckButton *butt,
                                             ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_TRACK_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_title_toogled_cb (GtkCheckButton *butt,
                                             ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_TITLE_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_artist_toogled_cb (GtkCheckButton *butt,
                                              ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_ARTIST_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_album_toogled_cb (GtkCheckButton *butt,
                                             ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_ALBUM_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_genre_toogled_cb (GtkCheckButton *butt,
                                             ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_GENRE_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_duration_toogled_cb (GtkCheckButton *butt,
                                                ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_DURATION_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_file_toogled_cb (GtkCheckButton *butt,
                                            ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_FILE_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_date_toogled_cb (GtkCheckButton *butt,
                                            ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_DATE_COLUMN_VISIBLE,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

void
ario_interface_preferences_autoscroll_toogled_cb (GtkCheckButton *butt,
                                                  ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_PLAYLIST_AUTOSCROLL,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (butt)));
}

