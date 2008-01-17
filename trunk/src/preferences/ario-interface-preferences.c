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
#include "preferences/ario-interface-preferences.h"
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/eel-gconf-extensions.h"
#include "ario-avahi.h"
#include "ario-debug.h"

static void ario_interface_preferences_class_init (ArioInterfacePreferencesClass *klass);
static void ario_interface_preferences_init (ArioInterfacePreferences *interface_preferences);
static void ario_interface_preferences_finalize (GObject *object);
static void ario_interface_preferences_sync_interface (ArioInterfacePreferences *interface_preferences);
void ario_interface_preferences_trayicon_behavior_changed_cb (GtkComboBoxEntry *combobox,
                                                    ArioInterfacePreferences *interface_preferences);
void ario_interface_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                                 ArioInterfacePreferences *interface_preferences);

static const char *trayicon_behavior[] = {
        N_("Play/Pause"),       // TRAY_ICON_PLAY_PAUSE
        N_("Play next song"),   // TRAY_ICON_NEXT_SONG
        N_("Do nothing"),       // TRAY_ICON_DO_NOTHING
        NULL
};

struct ArioInterfacePreferencesPrivate
{
        GtkWidget *showtabs_check;
        GtkWidget *trayicon_combobox;
};

static GObjectClass *parent_class = NULL;

GType
ario_interface_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_interface_preferences_type = 0;

        if (ario_interface_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioInterfacePreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_interface_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioInterfacePreferences),
                        0,
                        (GInstanceInitFunc) ario_interface_preferences_init
                };

                ario_interface_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                           "ArioInterfacePreferences",
                                                           &our_info, 0);
        }

        return ario_interface_preferences_type;
}

static void
ario_interface_preferences_class_init (ArioInterfacePreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_interface_preferences_finalize;
}

static void
ario_interface_preferences_init (ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        interface_preferences->priv = g_new0 (ArioInterfacePreferencesPrivate, 1);
}

GtkWidget *
ario_interface_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioInterfacePreferences *interface_preferences;
        GladeXML *xml;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;

        interface_preferences = g_object_new (TYPE_ARIO_INTERFACE_PREFERENCES, NULL);

        g_return_val_if_fail (interface_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "interface-prefs.glade",
                                "interface_vbox",
                                interface_preferences);

        interface_preferences->priv->showtabs_check =
                glade_xml_get_widget (xml, "showtabs_checkbutton");
        interface_preferences->priv->trayicon_combobox = 
                glade_xml_get_widget (xml, "trayicon_combobox");
                
        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

        for (i = 0; i < TRAY_ICON_N_BEHAVIOR; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (trayicon_behavior[i]),
                                    1, i,
                                    -1);
        }

        gtk_combo_box_set_model (GTK_COMBO_BOX (interface_preferences->priv->trayicon_combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (interface_preferences->priv->trayicon_combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (interface_preferences->priv->trayicon_combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (interface_preferences->priv->trayicon_combobox), renderer,
                                        "text", 0, NULL);
        
        ario_interface_preferences_sync_interface (interface_preferences);

        gtk_box_pack_start (GTK_BOX (interface_preferences), glade_xml_get_widget (xml, "interface_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (interface_preferences);
}

static void
ario_interface_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioInterfacePreferences *interface_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_INTERFACE_PREFERENCES (object));

        interface_preferences = ARIO_INTERFACE_PREFERENCES (object);

        g_return_if_fail (interface_preferences->priv != NULL);

        g_free (interface_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_interface_preferences_sync_interface (ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->showtabs_check), 
                                      eel_gconf_get_boolean (CONF_SHOW_TABS, TRUE));

        gtk_combo_box_set_active (GTK_COMBO_BOX (interface_preferences->priv->trayicon_combobox),
                                  eel_gconf_get_integer (CONF_TRAYICON_BEHAVIOR, 0));
}

void
ario_interface_preferences_trayicon_behavior_changed_cb (GtkComboBoxEntry *combobox,
                                               ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (interface_preferences->priv->trayicon_combobox));

        eel_gconf_set_integer (CONF_TRAYICON_BEHAVIOR, 
                               i);
}

void
ario_interface_preferences_showtabs_check_changed_cb (GtkCheckButton *butt,
                                            ArioInterfacePreferences *interface_preferences)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_boolean (CONF_SHOW_TABS,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (interface_preferences->priv->showtabs_check)));
}


