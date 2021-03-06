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

#include "preferences/ario-stats-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "lib/gtk-builder-helpers.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "servers/ario-server.h"

static void ario_stats_preferences_sync_stats (ArioStatsPreferences *stats_preferences);
static void ario_stats_preferences_stats_changed_cb (ArioServer *server,
                                                     ArioStatsPreferences *stats_preferences);

struct ArioStatsPreferencesPrivate
{
        GtkWidget *nbartists_label;
        GtkWidget *nbalbums_label;
        GtkWidget *nbsongs_label;
        GtkWidget *uptime_label;
        GtkWidget *playtime_label;
        GtkWidget *dbplay_time_label;
};

G_DEFINE_TYPE_WITH_CODE (ArioStatsPreferences, ario_stats_preferences, GTK_TYPE_BOX, G_ADD_PRIVATE(ArioStatsPreferences))

static void
ario_stats_preferences_class_init (ArioStatsPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START;
}

static void
ario_stats_preferences_init (ArioStatsPreferences *stats_preferences)
{
        ARIO_LOG_FUNCTION_START;
        stats_preferences->priv = ario_stats_preferences_get_instance_private (stats_preferences);
}

GtkWidget *
ario_stats_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START;
        GtkBuilder *builder;
        ArioStatsPreferences *stats_preferences;

        stats_preferences = g_object_new (TYPE_ARIO_STATS_PREFERENCES,
                                          NULL);

        g_return_val_if_fail (stats_preferences->priv != NULL, NULL);

        g_signal_connect_object (ario_server_get_instance (),
                                 "state_changed",
                                 G_CALLBACK (ario_stats_preferences_stats_changed_cb),
                                 stats_preferences, 0);

        gtk_orientable_set_orientation (GTK_ORIENTABLE (stats_preferences), GTK_ORIENTATION_VERTICAL);

        builder = gtk_builder_helpers_new (UI_PATH "stats-prefs.ui",
                                           stats_preferences);

        stats_preferences->priv->nbartists_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "nbartists_label"));
        stats_preferences->priv->nbalbums_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "nbalbums_label"));
        stats_preferences->priv->nbsongs_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "nbsongs_label"));
        stats_preferences->priv->uptime_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "uptime_label"));
        stats_preferences->priv->playtime_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "playtime_label"));
        stats_preferences->priv->dbplay_time_label = 
                GTK_WIDGET (gtk_builder_get_object (builder, "dbplay_time_label"));
        gtk_widget_set_size_request(stats_preferences->priv->nbartists_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->nbalbums_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->nbsongs_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->uptime_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->playtime_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->dbplay_time_label, 250, -1);

        gtk_builder_helpers_boldify_label (builder, "statistics_frame_label");
        gtk_builder_helpers_boldify_label (builder, "nbartists_const_label");
        gtk_builder_helpers_boldify_label (builder, "nbalbums_const_label");
        gtk_builder_helpers_boldify_label (builder, "nbsongs_const_label");
        gtk_builder_helpers_boldify_label (builder, "uptime_const_label");
        gtk_builder_helpers_boldify_label (builder, "playtime_const_label");
        gtk_builder_helpers_boldify_label (builder, "dbplay_time_const_label");

        ario_stats_preferences_sync_stats (stats_preferences);

        gtk_box_pack_start (GTK_BOX (stats_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (stats_preferences);
}

static void
ario_stats_preferences_sync_stats (ArioStatsPreferences *stats_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerStats *stats = ario_server_get_stats ();
        gchar *tmp;

        if (stats) {
                tmp = g_strdup_printf ("%d", stats->numberOfArtists);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbartists_label), tmp);
                g_free (tmp);

                tmp = g_strdup_printf ("%d", stats->numberOfAlbums);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbalbums_label), tmp);
                g_free (tmp);

                tmp = g_strdup_printf ("%d", stats->numberOfSongs);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbsongs_label), tmp);
                g_free (tmp);

                tmp = ario_util_format_total_time (stats->uptime);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->uptime_label), tmp);
                g_free (tmp);

                tmp = ario_util_format_total_time (stats->playTime);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->playtime_label), tmp);
                g_free (tmp);

                tmp = ario_util_format_total_time (stats->dbPlayTime);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->dbplay_time_label), tmp);
                g_free (tmp);
        } else {
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbartists_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbalbums_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbsongs_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->uptime_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->playtime_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->dbplay_time_label), _("Not connected"));
        }
}

static void
ario_stats_preferences_stats_changed_cb (ArioServer *server,
                                         ArioStatsPreferences *stats_preferences)
{
        ARIO_LOG_FUNCTION_START;
        ario_stats_preferences_sync_stats (stats_preferences);
}
