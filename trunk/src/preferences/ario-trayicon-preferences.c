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
#include "preferences/ario-trayicon-preferences.h"
#include "preferences/ario-preferences.h"
#include "notification/ario-notification-manager.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "ario-avahi.h"
#include "ario-debug.h"

static void ario_trayicon_preferences_sync_trayicon (ArioTrayiconPreferences *trayicon_preferences);
G_MODULE_EXPORT void ario_trayicon_preferences_trayicon_behavior_changed_cb (GtkComboBox *combobox,
                                                                             ArioTrayiconPreferences *trayicon_preferences);
G_MODULE_EXPORT void ario_trayicon_preferences_notification_check_changed_cb (GtkCheckButton *butt,
                                                                              ArioTrayiconPreferences *trayicon_preferences);
G_MODULE_EXPORT void ario_trayicon_preferences_notification_combobox_changed_cb (GtkComboBox *combobox,
                                                                                 ArioTrayiconPreferences *trayicon_preferences);
G_MODULE_EXPORT void ario_trayicon_preferences_trayicon_check_changed_cb (GtkCheckButton *butt,
                                                                          ArioTrayiconPreferences *trayicon_preferences);
G_MODULE_EXPORT void ario_trayicon_preferences_notificationtime_changed_cb (GtkWidget *widget,
                                                                            ArioTrayiconPreferences *trayicon_preferences);

static const char *trayicon_behavior[] = {
        N_("Play/Pause"),       // TRAY_ICON_PLAY_PAUSE
        N_("Play next song"),   // TRAY_ICON_NEXT_SONG
        N_("Do nothing"),       // TRAY_ICON_DO_NOTHING
        NULL
};

struct ArioTrayiconPreferencesPrivate
{
        GtkWidget *notification_check;
        GtkWidget *trayicon_check;
        GtkWidget *trayicon_combobox;
        GtkWidget *notificationtime_spinbutton;
        GtkWidget *notification_combobox;
};

#define ARIO_TRAYICON_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TRAYICON_PREFERENCES, ArioTrayiconPreferencesPrivate))
G_DEFINE_TYPE (ArioTrayiconPreferences, ario_trayicon_preferences, GTK_TYPE_VBOX)

static void
ario_trayicon_preferences_class_init (ArioTrayiconPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioTrayiconPreferencesPrivate));
}

static void
ario_trayicon_preferences_init (ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        trayicon_preferences->priv = ARIO_TRAYICON_PREFERENCES_GET_PRIVATE (trayicon_preferences);
}

GtkWidget *
ario_trayicon_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioTrayiconPreferences *trayicon_preferences;
        GladeXML *xml;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;
        GSList *notifiers;
        ArioNotifier *notifier;

        trayicon_preferences = g_object_new (TYPE_ARIO_TRAYICON_PREFERENCES, NULL);

        g_return_val_if_fail (trayicon_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "trayicon-prefs.glade",
                                "trayicon_vbox",
                                trayicon_preferences);

        trayicon_preferences->priv->trayicon_combobox = 
                glade_xml_get_widget (xml, "trayicon_combobox");
        trayicon_preferences->priv->notification_check =
                glade_xml_get_widget (xml, "notification_checkbutton");
        trayicon_preferences->priv->trayicon_check =
                glade_xml_get_widget (xml, "trayicon_checkbutton");
        trayicon_preferences->priv->notificationtime_spinbutton = 
                glade_xml_get_widget (xml, "notificationtime_spinbutton");
        trayicon_preferences->priv->notification_combobox = 
                glade_xml_get_widget (xml, "notification_combobox");

        rb_glade_boldify_label (xml, "trayicon_label");
        rb_glade_boldify_label (xml, "notification_label");

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
        for (i = 0; i < TRAY_ICON_N_BEHAVIOR; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (trayicon_behavior[i]),
                                    1, i,
                                    -1);
        }
        gtk_combo_box_set_model (GTK_COMBO_BOX (trayicon_preferences->priv->trayicon_combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (trayicon_preferences->priv->trayicon_combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (trayicon_preferences->priv->trayicon_combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (trayicon_preferences->priv->trayicon_combobox), renderer,
                                        "text", 0, NULL);

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        notifiers = ario_notification_manager_get_notifiers (ario_notification_manager_get_instance ());
        for (; notifiers; notifiers = g_slist_next (notifiers)) {
                notifier = notifiers->data;
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, ario_notifier_get_name (notifier),
                                    1, ario_notifier_get_id (notifier),
                                    -1);
        }
        gtk_combo_box_set_model (GTK_COMBO_BOX (trayicon_preferences->priv->notification_combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (trayicon_preferences->priv->notification_combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (trayicon_preferences->priv->notification_combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (trayicon_preferences->priv->notification_combobox), renderer,
                                        "text", 0, NULL);

        ario_trayicon_preferences_sync_trayicon (trayicon_preferences);

#ifndef ENABLE_EGGTRAYICON
        GtkWidget *tray_frame = glade_xml_get_widget (xml, "tray_frame");
        gtk_widget_hide (tray_frame);
        gtk_widget_set_no_show_all (tray_frame, TRUE);
#endif

        gtk_box_pack_start (GTK_BOX (trayicon_preferences), glade_xml_get_widget (xml, "trayicon_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (trayicon_preferences);
}

static void
ario_trayicon_preferences_sync_trayicon (ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        gchar *id;
        int i = 0;
        GSList *notifiers;
        ArioNotifier *notifier;

        gtk_combo_box_set_active (GTK_COMBO_BOX (trayicon_preferences->priv->trayicon_combobox),
                                  ario_conf_get_integer (PREF_TRAYICON_BEHAVIOR, PREF_TRAYICON_BEHAVIOR_DEFAULT));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (trayicon_preferences->priv->notification_check),
                                      ario_conf_get_boolean (PREF_HAVE_NOTIFICATION, PREF_HAVE_NOTIFICATION_DEFAULT));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (trayicon_preferences->priv->trayicon_check),
                                      ario_conf_get_boolean (PREF_TRAY_ICON, PREF_TRAY_ICON_DEFAULT));

        gtk_widget_set_sensitive (trayicon_preferences->priv->notificationtime_spinbutton, ario_conf_get_boolean (PREF_HAVE_NOTIFICATION, PREF_HAVE_NOTIFICATION_DEFAULT));
        gtk_widget_set_sensitive (trayicon_preferences->priv->notification_combobox, ario_conf_get_boolean (PREF_HAVE_NOTIFICATION, PREF_HAVE_NOTIFICATION_DEFAULT));
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (trayicon_preferences->priv->notificationtime_spinbutton), (gdouble) ario_conf_get_integer (PREF_NOTIFICATION_TIME, PREF_NOTIFICATION_TIME_DEFAULT));


        id = ario_conf_get_string (PREF_NOTIFIER, PREF_NOTIFIER_DEFAULT);
        notifiers = ario_notification_manager_get_notifiers (ario_notification_manager_get_instance ());
        gtk_combo_box_set_active (GTK_COMBO_BOX (trayicon_preferences->priv->notification_combobox), 0);
        for (; notifiers; notifiers = g_slist_next (notifiers)) {
                notifier = notifiers->data;
                if (!strcmp (ario_notifier_get_id (notifier), id)) {
                        gtk_combo_box_set_active (GTK_COMBO_BOX (trayicon_preferences->priv->notification_combobox), i);
                        break;
                }
                ++i;
        }
        g_free (id);
}

void
ario_trayicon_preferences_trayicon_behavior_changed_cb (GtkComboBox *combobox,
                                                        ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (trayicon_preferences->priv->trayicon_combobox));

        ario_conf_set_integer (PREF_TRAYICON_BEHAVIOR, 
                               i);
}

void
ario_trayicon_preferences_notification_check_changed_cb (GtkCheckButton *butt,
                                                         ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_HAVE_NOTIFICATION,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (trayicon_preferences->priv->notification_check)));
        gtk_widget_set_sensitive (trayicon_preferences->priv->notificationtime_spinbutton, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (trayicon_preferences->priv->notification_check)));
        gtk_widget_set_sensitive (trayicon_preferences->priv->notification_combobox, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (trayicon_preferences->priv->notification_check)));
}

void
ario_trayicon_preferences_trayicon_check_changed_cb (GtkCheckButton *butt,
                                                     ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_TRAY_ICON,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (trayicon_preferences->priv->trayicon_check)));
}

void
ario_trayicon_preferences_notificationtime_changed_cb (GtkWidget *widget,
                                                       ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_integer (PREF_NOTIFICATION_TIME, gtk_spin_button_get_value (GTK_SPIN_BUTTON (trayicon_preferences->priv->notificationtime_spinbutton)));
}
void
ario_trayicon_preferences_notification_combobox_changed_cb (GtkComboBox *combobox,
                                                            ArioTrayiconPreferences *trayicon_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        gchar *id;

        gtk_combo_box_get_active_iter (combobox, &iter);

        gtk_tree_model_get (gtk_combo_box_get_model (combobox), &iter,
                            1, &id, -1);

        ario_conf_set_string (PREF_NOTIFIER, id);

        g_free (id);
}

