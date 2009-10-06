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

#include "widgets/ario-tooltip.h"
#include <gtk/gtk.h>
#include <config.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "covers/ario-cover-handler.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "widgets/ario-tray-icon.h"

static void ario_tooltip_song_changed_cb (ArioServer *server,
                                          ArioTooltip *tooltip);
static void ario_tooltip_state_changed_cb (ArioServer *server,
                                           ArioTooltip *tooltip);
static void ario_tooltip_construct_tooltip (ArioTooltip *tooltip);
static void ario_tooltip_sync_tooltip_song (ArioTooltip *tooltip);
static void ario_tooltip_sync_tooltip_album (ArioTooltip *tooltip);
static void ario_tooltip_sync_tooltip_cover (ArioTooltip *tooltip);
static void ario_tooltip_sync_tooltip_time (ArioTooltip *tooltip);
static void ario_tooltip_album_changed_cb (ArioServer *server,
                                           ArioTooltip *tooltip);
static void ario_tooltip_time_changed_cb (ArioServer *server,
                                          int elapsed,
                                          ArioTooltip *tooltip);
static void ario_tooltip_cover_changed_cb (ArioCoverHandler *cover_handler,
                                           ArioTooltip *tooltip);

struct ArioTooltipPrivate
{
        GtkWidget *tooltip_primary;
        GtkWidget *tooltip_secondary;
        GtkWidget *tooltip_progress_bar;
        GtkWidget *tooltip_image_box;
        GtkWidget *cover_image;
};

#define ARIO_TOOLTIP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TOOLTIP, ArioTooltipPrivate))
G_DEFINE_TYPE (ArioTooltip, ario_tooltip, GTK_TYPE_HBOX)

static void
ario_tooltip_class_init (ArioTooltipClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioTooltipPrivate));
}

static void
ario_tooltip_init (ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;

        tooltip->priv = ARIO_TOOLTIP_GET_PRIVATE (tooltip);

        /* Construct tooltip */
        ario_tooltip_construct_tooltip (tooltip);

        /* Connect signal for current cover changes */
        g_signal_connect_object (ario_cover_handler_get_instance (),
                                 "cover_changed",
                                 G_CALLBACK (ario_tooltip_cover_changed_cb),
                                 tooltip, 0);
}

GtkWidget *
ario_tooltip_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioServer *server = ario_server_get_instance ();

        ArioTooltip *tooltip = g_object_new (TYPE_ARIO_TOOLTIP,
                                             NULL);

        /* Connect signals for synchonization with server */
        g_signal_connect_object (server,
                                 "song_changed",
                                 G_CALLBACK (ario_tooltip_song_changed_cb),
                                 tooltip, 0);
        g_signal_connect_object (server,
                                 "album_changed",
                                 G_CALLBACK (ario_tooltip_album_changed_cb),
                                 tooltip, 0);
        g_signal_connect_object (server,
                                 "elapsed_changed",
                                 G_CALLBACK (ario_tooltip_time_changed_cb),
                                 tooltip, 0);
        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_tooltip_state_changed_cb),
                                 tooltip, 0);
        g_signal_connect_object (server,
                                 "playlist_changed",
                                 G_CALLBACK (ario_tooltip_state_changed_cb),
                                 tooltip, 0);

        ario_tooltip_sync_tooltip_song (tooltip);
        ario_tooltip_sync_tooltip_album (tooltip);
        ario_tooltip_sync_tooltip_cover (tooltip);
        ario_tooltip_sync_tooltip_time (tooltip);

        return GTK_WIDGET (tooltip);
}

static void
ario_tooltip_song_changed_cb (ArioServer *server,
                              ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchonize song info */
        ario_tooltip_sync_tooltip_song (tooltip);

        /* Synchonize time info */
        ario_tooltip_sync_tooltip_time (tooltip);
}

static void
ario_tooltip_state_changed_cb (ArioServer *server,
                               ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize song info */
        ario_tooltip_sync_tooltip_song (tooltip);

        /* Synchronize album info */
        ario_tooltip_sync_tooltip_album (tooltip);

        /* Synchronize time info */
        ario_tooltip_sync_tooltip_time (tooltip);
}

static void
ario_tooltip_construct_tooltip (ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *vbox;
        gint size;
        PangoFontDescription *font_desc;

        /* Create primary label */
        tooltip->priv->tooltip_primary = gtk_label_new (TRAY_ICON_DEFAULT_TOOLTIP);

        /* Set primary label style */
        gtk_widget_modify_font (tooltip->priv->tooltip_primary, NULL);
        size = pango_font_description_get_size (gtk_widget_get_style (tooltip->priv->tooltip_primary)->font_desc);
        font_desc = pango_font_description_new ();
        pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
        pango_font_description_set_size (font_desc, size * PANGO_SCALE_LARGE);
        gtk_widget_modify_font (tooltip->priv->tooltip_primary, font_desc);
        pango_font_description_free (font_desc);
        gtk_label_set_line_wrap (GTK_LABEL (tooltip->priv->tooltip_primary),
                                 TRUE);
        gtk_misc_set_alignment  (GTK_MISC  (tooltip->priv->tooltip_primary),
                                 0.0, 0.0);

        /* Create secondary label */
        tooltip->priv->tooltip_secondary = gtk_label_new (NULL);
        gtk_label_set_line_wrap (GTK_LABEL (tooltip->priv->tooltip_secondary),
                                 TRUE);
        gtk_misc_set_alignment  (GTK_MISC  (tooltip->priv->tooltip_secondary),
                                 0.0, 0.0);

        /* Create cover art image */
        tooltip->priv->cover_image = gtk_image_new ();
        tooltip->priv->tooltip_image_box = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tooltip->priv->tooltip_image_box), tooltip->priv->cover_image,
                            TRUE, FALSE, 0);

        /* Set main hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (tooltip), FALSE);
        gtk_box_set_spacing (GTK_BOX (tooltip), 6);

        /* Create vbox for labels */
        vbox = gtk_vbox_new (FALSE, 2);

        /* Create progress bar */
        tooltip->priv->tooltip_progress_bar = gtk_progress_bar_new ();

        /* Add widgets to vbox */
        gtk_box_pack_start (GTK_BOX (vbox), tooltip->priv->tooltip_primary,
                            FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), tooltip->priv->tooltip_secondary,
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), tooltip->priv->tooltip_progress_bar,
                            TRUE, TRUE, 0);

        /* Add widgets to main hbox */
        gtk_box_pack_start (GTK_BOX (tooltip), tooltip->priv->tooltip_image_box,
                            FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tooltip), vbox,
                            TRUE, TRUE, 0);
        gtk_widget_show_all (GTK_WIDGET (tooltip));
}

static void
ario_tooltip_sync_tooltip_song (ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        gchar *title;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Set title in primary label*/
                title = ario_util_format_title (ario_server_get_current_song ());
                gtk_label_set_text (GTK_LABEL (tooltip->priv->tooltip_primary),
                                    title);
                break;
        default:
                /* Set default primary label */
                gtk_label_set_text (GTK_LABEL (tooltip->priv->tooltip_primary),
                                    TRAY_ICON_DEFAULT_TOOLTIP);
                break;
        }
}

static void
ario_tooltip_sync_tooltip_album (ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        gchar *artist;
        gchar *album;
        gchar *secondary;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Set Artist - Album in secondary label */
                artist = ario_server_get_current_artist ();
                album = ario_server_get_current_album ();

                if (!album)
                        album = ARIO_SERVER_UNKNOWN;
                if (!artist)
                        artist = ARIO_SERVER_UNKNOWN;

                secondary = TRAY_ICON_FROM_MARKUP (album, artist);
                gtk_label_set_markup (GTK_LABEL (tooltip->priv->tooltip_secondary),
                                      secondary);
                gtk_widget_show (tooltip->priv->tooltip_secondary);
                g_free(secondary);
                break;
        default:
                /* Set default secondary label */
                gtk_label_set_markup (GTK_LABEL (tooltip->priv->tooltip_secondary), "");
                gtk_widget_hide (tooltip->priv->tooltip_secondary);
                break;
        }
}

static void
ario_tooltip_sync_tooltip_cover (ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        GdkPixbuf *cover;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Set cover art */
                cover = ario_cover_handler_get_cover();
                if (cover) {
                        gtk_image_set_from_pixbuf (GTK_IMAGE (tooltip->priv->cover_image), cover);
                        gtk_widget_show (tooltip->priv->cover_image);
                } else {
                        gtk_widget_hide (tooltip->priv->cover_image);
                }
                break;
        default:
                /* Hide cover art */
                gtk_widget_hide (tooltip->priv->cover_image);
                break;
        }
}

static void
ario_tooltip_sync_tooltip_time (ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        int elapsed;
        int total;
        gchar elapsed_char[ARIO_MAX_TIME_SIZE];
        gchar total_char[ARIO_MAX_TIME_SIZE];
        gchar *time;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Get formated elapsed time */
                elapsed = ario_server_get_current_elapsed ();
                ario_util_format_time_buf (elapsed, elapsed_char, ARIO_MAX_TIME_SIZE);

                /* Get total time */
                total = ario_server_get_current_total_time ();
                if (total) {
                        /* Total time is set, we format it and show it */
                        ario_util_format_time_buf (total, total_char, ARIO_MAX_TIME_SIZE);
                        time = g_strdup_printf ("%s%s%s", elapsed_char, _(" of "), total_char);
                        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (tooltip->priv->tooltip_progress_bar), time);
                        g_free (time);
                } else {
                        /* No total time: We only show elapsed time */
                        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (tooltip->priv->tooltip_progress_bar), elapsed_char);
                }
                /* Adjust progress bar */
                if (total > 0)
                        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (tooltip->priv->tooltip_progress_bar),
                                                       (double) elapsed / (double) total);
                else
                        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (tooltip->priv->tooltip_progress_bar),
                                                       0);
                gtk_widget_show (tooltip->priv->tooltip_progress_bar);
                break;
        default:
                /* Set default value for progress bar */
                gtk_progress_bar_set_text (GTK_PROGRESS_BAR (tooltip->priv->tooltip_progress_bar), NULL);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (tooltip->priv->tooltip_progress_bar),
                                               0);
                gtk_widget_hide (tooltip->priv->tooltip_progress_bar);
                break;
        }
}

static void
ario_tooltip_album_changed_cb (ArioServer *server,
                               ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize album info */
        ario_tooltip_sync_tooltip_album (tooltip);
}

static void
ario_tooltip_time_changed_cb (ArioServer *server,
                              int elapsed,
                              ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize time info */
        ario_tooltip_sync_tooltip_time (tooltip);
}

static void
ario_tooltip_cover_changed_cb (ArioCoverHandler *cover_handler,
                               ArioTooltip *tooltip)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize cover art */
        ario_tooltip_sync_tooltip_cover (tooltip);
}

