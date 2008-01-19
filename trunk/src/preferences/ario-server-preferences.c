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
#include "preferences/ario-server-preferences.h"
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/eel-gconf-extensions.h"
#include "ario-avahi.h"
#include "ario-debug.h"

static void ario_server_preferences_class_init (ArioServerPreferencesClass *klass);
static void ario_server_preferences_init (ArioServerPreferences *server_preferences);
static void ario_server_preferences_finalize (GObject *object);
static void ario_server_preferences_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void ario_server_preferences_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);
static void ario_server_preferences_sync_server (ArioServerPreferences *server_preferences);
static void ario_server_preferences_server_changed_cb(ArioMpd *mpd,
                                               ArioServerPreferences *server_preferences);
void ario_server_preferences_crossfadetime_changed_cb (GtkWidget *widget,
                                                ArioServerPreferences *server_preferences);
void ario_server_preferences_crossfade_changed_cb (GtkWidget *widget,
                                            ArioServerPreferences *server_preferences);
void ario_server_preferences_updatedb_button_cb (GtkWidget *widget,
                                          ArioServerPreferences *server_preferences);


enum
{
        PROP_0,
        PROP_MPD
};

struct ArioServerPreferencesPrivate
{
        ArioMpd *mpd;

        GtkWidget *crossfade_checkbutton;
        GtkWidget *crossfadetime_spinbutton;
        GtkWidget *updatedb_label;
        GtkWidget *updatedb_button;

        gboolean sync_mpd;
};

static GObjectClass *parent_class = NULL;

GType
ario_server_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_server_preferences_type = 0;

        if (ario_server_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioServerPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_server_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioServerPreferences),
                        0,
                        (GInstanceInitFunc) ario_server_preferences_init
                };

                ario_server_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                       "ArioServerPreferences",
                                                                       &our_info, 0);
        }

        return ario_server_preferences_type;
}

static void
ario_server_preferences_class_init (ArioServerPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_server_preferences_finalize;
        object_class->set_property = ario_server_preferences_set_property;
        object_class->get_property = ario_server_preferences_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_server_preferences_init (ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        server_preferences->priv = g_new0 (ArioServerPreferencesPrivate, 1);
}

GtkWidget *
ario_server_preferences_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioServerPreferences *server_preferences;
        GladeXML *xml;

        server_preferences = g_object_new (TYPE_ARIO_SERVER_PREFERENCES,
                                          "mpd", mpd,
                                           NULL);

        g_return_val_if_fail (server_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "server-prefs.glade",
                                "vbox",
                                server_preferences);

        server_preferences->priv->crossfade_checkbutton = 
                glade_xml_get_widget (xml, "crossfade_checkbutton");
        server_preferences->priv->crossfadetime_spinbutton = 
                glade_xml_get_widget (xml, "crossfadetime_spinbutton");
        server_preferences->priv->updatedb_label = 
                glade_xml_get_widget (xml, "updatedb_label");
        server_preferences->priv->updatedb_button = 
                glade_xml_get_widget (xml, "updatedb_button");

        ario_server_preferences_sync_server (server_preferences);

        gtk_box_pack_start (GTK_BOX (server_preferences), glade_xml_get_widget (xml, "vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (server_preferences);
}

static void
ario_server_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioServerPreferences *server_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SERVER_PREFERENCES (object));

        server_preferences = ARIO_SERVER_PREFERENCES (object);

        g_return_if_fail (server_preferences->priv != NULL);

        g_free (server_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_server_preferences_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioServerPreferences *server_preferences = ARIO_SERVER_PREFERENCES (object);
        
        switch (prop_id) {
        case PROP_MPD:
                server_preferences->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (server_preferences->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_server_preferences_server_changed_cb),
                                         server_preferences, 0);
                g_signal_connect_object (G_OBJECT (server_preferences->priv->mpd),
                                         "updatingdb_changed", G_CALLBACK (ario_server_preferences_server_changed_cb),
                                         server_preferences, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_server_preferences_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioServerPreferences *server_preferences = ARIO_SERVER_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, server_preferences->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_server_preferences_sync_server (ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        int crossfadetime;
        int state;
        gboolean updating;
        long last_update;
        gchar *last_update_char;

        state = ario_mpd_get_current_state (server_preferences->priv->mpd);
        updating = ario_mpd_get_updating (server_preferences->priv->mpd);

        if (state == MPD_STATUS_STATE_UNKNOWN) {
                crossfadetime = 0;
                last_update_char = _("Not connected");
        } else {
                crossfadetime = ario_mpd_get_crossfadetime (server_preferences->priv->mpd);

                if (updating) {
                        last_update_char = _("Updating...");
                } else {
                        last_update = (long) ario_mpd_get_last_update (server_preferences->priv->mpd);
                        last_update_char = ctime (&last_update);
                        /* Remove the new line */
                        last_update_char[strlen (last_update_char)-1] = '\0';
                }
        }

        server_preferences->priv->sync_mpd = TRUE;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (server_preferences->priv->crossfade_checkbutton), (crossfadetime != 0) && (state != MPD_STATUS_STATE_UNKNOWN));
        gtk_widget_set_sensitive (server_preferences->priv->crossfade_checkbutton, state != MPD_STATUS_STATE_UNKNOWN);
        gtk_widget_set_sensitive (server_preferences->priv->crossfadetime_spinbutton, (crossfadetime != 0) && (state != MPD_STATUS_STATE_UNKNOWN));
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton), (gdouble) crossfadetime);

        gtk_widget_set_sensitive (server_preferences->priv->updatedb_button, (!updating && state != MPD_STATUS_STATE_UNKNOWN));
        gtk_label_set_label (GTK_LABEL (server_preferences->priv->updatedb_label), last_update_char);

        server_preferences->priv->sync_mpd = FALSE;
}

static void
ario_server_preferences_server_changed_cb (ArioMpd *mpd,
                                    ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_server_preferences_sync_server (server_preferences);
}

void
ario_server_preferences_crossfadetime_changed_cb (GtkWidget *widget,
                                           ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        int crossfadetime;
        if (!server_preferences->priv->sync_mpd) {
                crossfadetime = gtk_spin_button_get_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton));
                ario_mpd_set_crossfadetime (server_preferences->priv->mpd,
                                            crossfadetime);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (server_preferences->priv->crossfade_checkbutton), (crossfadetime != 0));
                gtk_widget_set_sensitive (server_preferences->priv->crossfadetime_spinbutton, (crossfadetime != 0));
        }
}

void
ario_server_preferences_crossfade_changed_cb (GtkWidget *widget,
                                       ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        gboolean is_active;
        if (!server_preferences->priv->sync_mpd) {
                is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (server_preferences->priv->crossfade_checkbutton));
                if (is_active) {
                        ario_mpd_set_crossfadetime (server_preferences->priv->mpd,
                                                    1);
                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton),
                                                   1);
                } else {
                        ario_mpd_set_crossfadetime (server_preferences->priv->mpd,
                                                    0);
                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (server_preferences->priv->crossfadetime_spinbutton),
                                                   0);
                }
                gtk_widget_set_sensitive (server_preferences->priv->crossfadetime_spinbutton, is_active);
        }
}

void
ario_server_preferences_updatedb_button_cb (GtkWidget *widget,
                                     ArioServerPreferences *server_preferences)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_set_sensitive (server_preferences->priv->updatedb_button, FALSE);
        gtk_label_set_label (GTK_LABEL (server_preferences->priv->updatedb_label), _("Updating..."));
        ario_mpd_update_db (server_preferences->priv->mpd);
}
