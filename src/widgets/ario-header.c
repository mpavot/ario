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

#include "widgets/ario-header.h"
#include <gtk/gtk.h>
#include <config.h>
#include <string.h>
#include <glib/gi18n.h>

#include "ario-util.h"
#include "ario-debug.h"
#include "covers/ario-cover.h"
#include "covers/ario-cover-handler.h"
#include "covers/ario-cover-handler.h"
#include "shell/ario-shell-coverselect.h"
#include "widgets/ario-volume.h"

static GObject* ario_header_constructor (GType type, guint n_construct_properties,
                                         GObjectConstructParam *construct_properties);
static gboolean ario_header_image_press_cb (GtkWidget *widget,
                                            GdkEventButton *event,
                                            ArioHeader *header);
static gboolean ario_header_slider_press_cb (GtkWidget *widget,
                                             GdkEventButton *event,
                                             ArioHeader *header);
static gboolean ario_header_slider_release_cb (GtkWidget *widget,
                                               GdkEventButton *event,
                                               ArioHeader *header);
static void ario_header_slider_value_changed_cb (GtkWidget *widget,
                                                 ArioHeader *header);
static void ario_header_song_changed_cb (ArioServer *server,
                                         ArioHeader *header);
static void ario_header_album_changed_cb (ArioServer *server,
                                          ArioHeader *header);
static void ario_header_state_changed_cb (ArioServer *server,
                                          ArioHeader *header);
static void ario_header_cover_changed_cb (ArioCoverHandler *cover_handler,
                                          ArioHeader *header);
static void ario_header_elapsed_changed_cb (ArioServer *server,
                                            int elapsed,
                                            ArioHeader *header);
static void ario_header_random_changed_cb (ArioServer *server,
                                           ArioHeader *header);
static void ario_header_repeat_changed_cb (ArioServer *server,
                                           ArioHeader *header);
static void ario_header_do_random (ArioHeader *header);
static void ario_header_do_repeat (ArioHeader *header);

#define SONG_MARKUP(xSONG) g_markup_printf_escaped ("<big><b>%s</b></big>", xSONG);
#define FROM_MARKUP(xALBUM, xARTIST) g_markup_printf_escaped (_("<i>from</i> %s <i>by</i> %s"), xALBUM, xARTIST);

struct ArioHeaderPrivate
{
        GtkWidget *prev_button;
        GtkWidget *play_pause_button;
        GtkWidget *random_button;
        GtkWidget *repeat_button;

        GtkWidget *stop_button;
        GtkWidget *next_button;

        GtkWidget *play_image;
        GtkWidget *pause_image;

        GtkWidget *image;

        GtkWidget *song;
        GtkWidget *artist_album;

        GtkWidget *scale;
        GtkAdjustment *adjustment;

        GtkWidget *elapsed;
        GtkWidget *of;
        GtkWidget *total;

        GtkWidget *volume_button;

        gboolean slider_dragging;

        gint image_width;
        gint image_height;
};

#define ARIO_HEADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_HEADER, ArioHeaderPrivate))
G_DEFINE_TYPE (ArioHeader, ario_header, GTK_TYPE_HBOX)

static void
ario_header_class_init (ArioHeaderClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* Virtual methods */
        object_class->constructor = ario_header_constructor;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioHeaderPrivate));
}

static void
ario_header_init (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        header->priv = ARIO_HEADER_GET_PRIVATE (header);
}

static void
ario_header_drag_leave_cb (GtkWidget *widget,
                           GdkDragContext *context,
                           gint x,
                           gint y,
                           GtkSelectionData *data,
                           guint info,
                           guint time,
                           ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        gchar *url;
        gchar *contents;
        gsize length;

        if (info == 1) {
                /* Image dropped */
                ARIO_LOG_INFO ("image  DND : TODO\n");
        } else if (info == 2) {
                /* URL dropped */
                data->data[data->length - 2] = 0;
                url = g_strdup ((gchar *) data->data + 7);
                if (ario_util_uri_exists (url)) {
                        /* Get file content and save it as the cover */
                        if (ario_file_get_contents (url,
                                                    &contents,
                                                    &length,
                                                    NULL)) {
                                ario_cover_save_cover (ario_server_get_current_artist (),
                                                       ario_server_get_current_album (),
                                                       contents, length,
                                                       OVERWRITE_MODE_REPLACE);
                                g_free (contents);
                                ario_cover_handler_force_reload ();
                        }
                }
                g_free (url);
        }

        /* finish the drag */
        gtk_drag_finish (context, TRUE, FALSE, time);
}

static GObject *
ario_header_constructor (GType type, guint n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START;
        ArioHeader *header;
        ArioHeaderClass *klass;
        GObjectClass *parent_class;
        GtkWidget *cover_event_box;
        GtkTargetList *targets;
        GtkTargetEntry *target_entry;
        gint n_elem;
        GtkWidget *image, *hbox, *right_hbox, *vbox, *alignment;

        klass = ARIO_HEADER_CLASS (g_type_class_peek (TYPE_ARIO_HEADER));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        header = ARIO_HEADER (parent_class->constructor (type, n_construct_properties,
                                                         construct_properties));

        /* Construct previous button */
        image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);

        header->priv->prev_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->prev_button), image);
        g_signal_connect_swapped (header->priv->prev_button,
                                  "clicked",
                                  G_CALLBACK (ario_header_do_previous),
                                  header);
        gtk_widget_set_tooltip_text (GTK_WIDGET (header->priv->prev_button),
                                     _("Play previous song"));

        /* Construct button images */
        header->priv->play_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY,
                                                             GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (header->priv->play_image);
        gtk_widget_show (header->priv->play_image);
        header->priv->pause_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE,
                                                              GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (header->priv->pause_image);
        gtk_widget_show (header->priv->pause_image);

        header->priv->play_pause_button = gtk_button_new ();

        gtk_container_add (GTK_CONTAINER (header->priv->play_pause_button), header->priv->pause_image);

        g_signal_connect_swapped (header->priv->play_pause_button,
                                  "clicked",
                                  G_CALLBACK (ario_header_playpause),
                                  header);
        gtk_widget_set_tooltip_text (GTK_WIDGET (header->priv->play_pause_button),
                                     _("Play/Pause the music"));

        /* Construct stop button */
        image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_STOP,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->stop_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->stop_button), image);
        g_signal_connect_swapped (header->priv->stop_button,
                                  "clicked",
                                  G_CALLBACK (ario_header_stop),
                                  header);
        gtk_widget_set_tooltip_text (GTK_WIDGET (header->priv->stop_button),
                                     _("Stop the music"));

        /* Construct next button */
        image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_NEXT,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->next_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->next_button), image);
        g_signal_connect_swapped (header->priv->next_button,
                                  "clicked",
                                  G_CALLBACK (ario_header_do_next),
                                  header);
        gtk_widget_set_tooltip_text (GTK_WIDGET (header->priv->next_button),
                                     _("Play next song"));

        /* Construct cover display */
        cover_event_box = gtk_event_box_new ();
        header->priv->image = gtk_image_new ();
        gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR,
                              &header->priv->image_width,
                              &header->priv->image_height);
        header->priv->image_width += 18;
        header->priv->image_height += 18;
        gtk_container_add (GTK_CONTAINER (cover_event_box), header->priv->image);
        g_signal_connect (cover_event_box,
                          "button_press_event",
                          G_CALLBACK (ario_header_image_press_cb),
                          header);
        targets = gtk_target_list_new (NULL, 0);
        gtk_target_list_add_image_targets (targets, 1, TRUE);
        gtk_target_list_add_uri_targets (targets, 2);
        target_entry = gtk_target_table_new_from_list (targets, &n_elem);
        gtk_target_list_unref (targets);

        gtk_drag_dest_set (cover_event_box,
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP,
                           target_entry, n_elem,
                           GDK_ACTION_COPY);
        gtk_target_table_free (target_entry, n_elem);

        g_signal_connect (cover_event_box,
                          "drag_data_received",
                          G_CALLBACK (ario_header_drag_leave_cb),
                          header);

        g_signal_connect_object (ario_cover_handler_get_instance (),
                                 "cover_changed", G_CALLBACK (ario_header_cover_changed_cb),
                                 header, 0);

        /* Construct Song/Artist/Album display */
        header->priv->song = gtk_label_new ("");
        gtk_label_set_ellipsize (GTK_LABEL (header->priv->song), PANGO_ELLIPSIZE_END);
        gtk_label_set_use_markup (GTK_LABEL (header->priv->song), TRUE);
        gtk_misc_set_alignment (GTK_MISC (header->priv->song), 0, 0);

        header->priv->artist_album = gtk_label_new ("");
        gtk_label_set_ellipsize (GTK_LABEL (header->priv->artist_album), PANGO_ELLIPSIZE_END);
        gtk_misc_set_alignment (GTK_MISC (header->priv->artist_album), 0, 0);

        /* Construct time slider */
        header->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 10.0, 1.0, 10.0, 0.0));
        header->priv->scale = gtk_hscale_new (header->priv->adjustment);

        g_signal_connect (header->priv->scale,
                          "button_press_event",
                          G_CALLBACK (ario_header_slider_press_cb),
                          header);
        g_signal_connect (header->priv->scale,
                          "button_release_event",
                          G_CALLBACK (ario_header_slider_release_cb),
                          header);
        g_signal_connect (header->priv->scale,
                          "value-changed",
                          G_CALLBACK (ario_header_slider_value_changed_cb),
                          header);

        gtk_scale_set_draw_value (GTK_SCALE (header->priv->scale), FALSE);
        gtk_widget_set_size_request (header->priv->scale, 150, -1);

        header->priv->elapsed = gtk_label_new ("0:00");
        /* Translators - This " of " is used to count the elapsed time
           of a song like in "00:59 of 03:24" */
        header->priv->of = gtk_label_new (_(" of "));
        header->priv->total = gtk_label_new ("0:00");

        /* Construct random button */
        image = gtk_image_new_from_stock ("random",
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->random_button = gtk_toggle_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->random_button), image);
        g_signal_connect_swapped (header->priv->random_button,
                                  "clicked",
                                  G_CALLBACK (ario_header_do_random),
                                  header);
        gtk_widget_set_tooltip_text (GTK_WIDGET (header->priv->random_button),
                                     _("Toggle random on/off"));

        /* Construct repeat button */
        image = gtk_image_new_from_stock ("repeat",
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->repeat_button = gtk_toggle_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->repeat_button), image);
        g_signal_connect_swapped (header->priv->repeat_button,
                                  "clicked",
                                  G_CALLBACK (ario_header_do_repeat),
                                  header);
        gtk_widget_set_tooltip_text (GTK_WIDGET (header->priv->repeat_button),
                                     _("Toggle repeat on/off"));

        /* Construct volume button */
        header->priv->volume_button = GTK_WIDGET (ario_volume_new ());
        gtk_widget_set_tooltip_text (header->priv->volume_button,
                                     _("Change the music volume"));

        /* Add everything in header Hbox */
        gtk_box_set_spacing (GTK_BOX (header), 12);

        /* Add command Buttons */
        hbox = gtk_hbox_new (FALSE, 5);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

        gtk_box_pack_start (GTK_BOX (hbox), header->priv->prev_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->play_pause_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->stop_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->next_button, FALSE, TRUE, 0);

        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), hbox);
        gtk_box_pack_start (GTK_BOX (header), alignment, FALSE, TRUE, 0);

        /* Add cover */
        gtk_box_pack_start (GTK_BOX (header), cover_event_box, FALSE, TRUE, 0);

        /* Add song labels */
        vbox = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), header->priv->song, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), header->priv->artist_album, TRUE, TRUE, 0);

        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), vbox);
        gtk_box_pack_start (GTK_BOX (header), alignment, TRUE, TRUE, 0);

        /* Add time slider */
        vbox = gtk_vbox_new (FALSE, 0);
        hbox = gtk_hbox_new (FALSE, 0);
        right_hbox = gtk_hbox_new (FALSE, 5);

        gtk_box_pack_start (GTK_BOX (hbox), header->priv->elapsed, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->of, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->total, FALSE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (vbox), header->priv->scale, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (right_hbox), vbox, FALSE, TRUE, 0);

        /* Add random/repeat buttons */
        gtk_box_pack_start (GTK_BOX (right_hbox), header->priv->random_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (right_hbox), header->priv->repeat_button, FALSE, TRUE, 0);

        /* Add volume button */
        gtk_box_pack_start (GTK_BOX (right_hbox), header->priv->volume_button, FALSE, TRUE, 5);

        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), right_hbox);
        gtk_box_pack_end (GTK_BOX (header), alignment, FALSE, TRUE, 0);

        return G_OBJECT (header);
}

GtkWidget *
ario_header_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioHeader *header;
        ArioServer *server = ario_server_get_instance ();

        header = ARIO_HEADER (g_object_new (TYPE_ARIO_HEADER,
                                            NULL));

        g_return_val_if_fail (header->priv != NULL, NULL);

        /* Signals to synchronize the header with server */
        g_signal_connect_object (server,
                                 "song_changed", G_CALLBACK (ario_header_song_changed_cb),
                                 header, 0);
        g_signal_connect_object (server,
                                 "album_changed", G_CALLBACK (ario_header_album_changed_cb),
                                 header, 0);
        g_signal_connect_object (server,
                                 "state_changed", G_CALLBACK (ario_header_state_changed_cb),
                                 header, 0);
        g_signal_connect_object (server,
                                 "elapsed_changed", G_CALLBACK (ario_header_elapsed_changed_cb),
                                 header, 0);
        g_signal_connect_object (server,
                                 "random_changed", G_CALLBACK (ario_header_random_changed_cb),
                                 header, 0);
        g_signal_connect_object (server,
                                 "repeat_changed", G_CALLBACK (ario_header_repeat_changed_cb),
                                 header, 0);

        return GTK_WIDGET (header);
}

static void
ario_header_change_total_time (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        char *tmp;
        int total_time;

        /* Get total song time from server */
        if (!ario_server_is_connected ()) {
                total_time = 0;
        } else {
                switch (ario_server_get_current_state ()) {
                case MPD_STATUS_STATE_PLAY:
                case MPD_STATUS_STATE_PAUSE:
                        total_time = ario_server_get_current_total_time ();
                        break;
                case MPD_STATUS_STATE_UNKNOWN:
                case MPD_STATUS_STATE_STOP:
                default:
                        total_time = 0;
                        break;
                }

        }

        /* Change label value with total time */
        if (total_time > 0) {
                tmp = ario_util_format_time (total_time);
                gtk_label_set_text (GTK_LABEL (header->priv->total), tmp);
                g_free (tmp);
                gtk_widget_show (header->priv->total);
                gtk_widget_show (header->priv->of);
        } else {
                gtk_widget_hide (header->priv->total);
                gtk_widget_hide (header->priv->of);
        }

        /* Change slider higher value */
        header->priv->adjustment->upper = total_time;
}

static void
ario_header_change_song_label (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        char *title;
        char *tmp;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Set the label with formated title from server */
                title = ario_util_format_title (ario_server_get_current_song ());

                tmp = SONG_MARKUP (title);
                gtk_label_set_markup (GTK_LABEL (header->priv->song), tmp);
                g_free (tmp);
                break;
        case MPD_STATUS_STATE_UNKNOWN:
        case MPD_STATUS_STATE_STOP:
        default:
                /* Set default label value */
                gtk_label_set_label (GTK_LABEL (header->priv->song), "");
                break;
        }
}

static void
ario_header_change_artist_album_label (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        char *artist;
        char *album;
        char *tmp;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Set label value with server values */
                artist = ario_server_get_current_artist ();
                album = ario_server_get_current_album ();

                if (!album)
                        album = ARIO_SERVER_UNKNOWN;

                if (!artist)
                        artist = ARIO_SERVER_UNKNOWN;

                tmp = FROM_MARKUP (album, artist);
                gtk_label_set_markup (GTK_LABEL (header->priv->artist_album), tmp);
                g_free (tmp);
                break;
        case MPD_STATUS_STATE_UNKNOWN:
        case MPD_STATUS_STATE_STOP:
        default:
                /* Set default label value */
                gtk_label_set_label (GTK_LABEL (header->priv->artist_album), "");
                break;
        }
}

static void
ario_header_change_cover (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        GdkPixbuf *cover;
        GdkPixbuf *small_cover = NULL;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Get cover from cover handler and display it */
                cover = ario_cover_handler_get_cover ();
                if (cover) {
                        small_cover = gdk_pixbuf_scale_simple (cover,
                                                               header->priv->image_width,
                                                               header->priv->image_height,
                                                               GDK_INTERP_BILINEAR);
                }

                gtk_image_set_from_pixbuf (GTK_IMAGE (header->priv->image), small_cover);

                if (small_cover)
                        g_object_unref (small_cover);
                break;
        case MPD_STATUS_STATE_UNKNOWN:
        case MPD_STATUS_STATE_STOP:
        default:
                /* Set default cover (empty) */
                gtk_image_set_from_pixbuf (GTK_IMAGE (header->priv->image), NULL);
                break;
        }
}

static void
ario_header_song_changed_cb (ArioServer *server,
                             ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize song label */
        ario_header_change_song_label (header);

        /* Synchronize total time label */
        ario_header_change_total_time (header);
}

static void
ario_header_album_changed_cb (ArioServer *server,
                              ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize Artist-Album label */
        ario_header_change_artist_album_label (header);
}

static void
ario_header_cover_changed_cb (ArioCoverHandler *cover_handler,
                              ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize cover art */
        ario_header_change_cover (header);
}

static void
ario_header_state_changed_cb (ArioServer *server,
                              ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;

        /* Synchronize song label */
        ario_header_change_song_label (header);

        /* Synchronize Artist-Album label */
        ario_header_change_artist_album_label (header);

        /* Synchronize total time label */
        ario_header_change_total_time (header);

        /* Remove icon from play/pause button */
        gtk_container_remove (GTK_CONTAINER (header->priv->play_pause_button),
                              gtk_bin_get_child (GTK_BIN (header->priv->play_pause_button)));

        /* Set the appropriate icon in play/pause button */
        if (ario_server_is_paused ())
                gtk_container_add (GTK_CONTAINER (header->priv->play_pause_button),
                                   header->priv->play_image);
        else
                gtk_container_add (GTK_CONTAINER (header->priv->play_pause_button),
                                   header->priv->pause_image);

        if (!ario_server_is_connected ()) {
                /* Set button insensitive if Ario is not connected to a server */
                gtk_widget_set_sensitive (header->priv->prev_button, FALSE);
                gtk_widget_set_sensitive (header->priv->play_pause_button, FALSE);

                gtk_widget_set_sensitive (header->priv->random_button, FALSE);
                gtk_widget_set_sensitive (header->priv->repeat_button, FALSE);

                gtk_widget_set_sensitive (header->priv->stop_button, FALSE);
                gtk_widget_set_sensitive (header->priv->next_button, FALSE);

                gtk_widget_set_sensitive (header->priv->scale, FALSE);

                gtk_widget_set_sensitive (header->priv->volume_button, FALSE);
        } else {
                /* Set button sensitive if Ario is connected to a server */
                gtk_widget_set_sensitive (header->priv->prev_button, TRUE);
                gtk_widget_set_sensitive (header->priv->play_pause_button, TRUE);

                gtk_widget_set_sensitive (header->priv->random_button, TRUE);
                gtk_widget_set_sensitive (header->priv->repeat_button, TRUE);

                gtk_widget_set_sensitive (header->priv->stop_button, TRUE);
                gtk_widget_set_sensitive (header->priv->next_button, TRUE);

                gtk_widget_set_sensitive (header->priv->scale, TRUE);

                gtk_widget_set_sensitive (header->priv->volume_button, TRUE);
        }
}

static void
ario_header_elapsed_changed_cb (ArioServer *server,
                                int elapsed,
                                ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        gchar time[ARIO_MAX_TIME_SIZE];

        if (header->priv->slider_dragging)
                return;

        /* Update elapsed time label */
        ario_util_format_time_buf (elapsed, time, ARIO_MAX_TIME_SIZE);
        gtk_label_set_text (GTK_LABEL (header->priv->elapsed), time);

        /* Update slider value */
        gtk_adjustment_set_value (header->priv->adjustment, (gdouble) elapsed);
}

static void
ario_header_random_changed_cb (ArioServer *server,
                               ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        gboolean random;

        /* Get random state on server */
        random = ario_server_get_current_random ();

        /* Block random button signal */
        g_signal_handlers_block_by_func (G_OBJECT (header->priv->random_button),
                                         G_CALLBACK (ario_header_do_random),
                                         header);

        /* Change button state depending on random value */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (header->priv->random_button),
                                      random);

        /* Unblock random button signal */
        g_signal_handlers_unblock_by_func (G_OBJECT (header->priv->random_button),
                                           G_CALLBACK (ario_header_do_random),
                                           header);
}

static void
ario_header_repeat_changed_cb (ArioServer *server,
                               ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        gboolean repeat;

        /* Get repeat state on server */
        repeat = ario_server_get_current_repeat ();

        /* Block repeat button signal */
        g_signal_handlers_block_by_func (G_OBJECT (header->priv->repeat_button),
                                         G_CALLBACK (ario_header_do_repeat),
                                         header);

        /* Change button state depending on repeat value */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (header->priv->repeat_button),
                                      repeat);

        /* Unblock repeat button signal */
        g_signal_handlers_unblock_by_func (G_OBJECT (header->priv->repeat_button),
                                           G_CALLBACK (ario_header_do_repeat),
                                           header);
}

static gboolean
ario_header_image_press_cb (GtkWidget *widget,
                            GdkEventButton *event,
                            ArioHeader *header)
{
        GtkWidget *coverselect;
        ArioServerAlbum server_album;

        /* Double click on cover art launches the cover selection dialog */
        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                server_album.artist = ario_server_get_current_artist ();
                server_album.album = ario_server_get_current_album ();
                server_album.path = g_path_get_dirname ((ario_server_get_current_song ())->file);

                if (!server_album.album)
                        server_album.album = ARIO_SERVER_UNKNOWN;

                if (!server_album.artist)
                        server_album.artist = ARIO_SERVER_UNKNOWN;

                coverselect = ario_shell_coverselect_new (&server_album);
                gtk_dialog_run (GTK_DIALOG (coverselect));
                gtk_widget_destroy (coverselect);
                g_free (server_album.path);
        }

        return FALSE;
}

static gboolean
ario_header_slider_press_cb (GtkWidget *widget,
                             GdkEventButton *event,
                             ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        header->priv->slider_dragging = TRUE;
        return FALSE;
}

static gboolean
ario_header_slider_release_cb (GtkWidget *widget,
                               GdkEventButton *event,
                               ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        gdouble scale;

        header->priv->slider_dragging = FALSE;
        /* Change elapsed time on server */
        scale = gtk_range_get_value (GTK_RANGE (header->priv->scale));
        ario_server_set_current_elapsed ((int) scale);
        return FALSE;
}

static void ario_header_slider_value_changed_cb (GtkWidget *widget,
                                                 ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        gchar time[ARIO_MAX_TIME_SIZE];
        int elapsed;
        gdouble scale;

        if (header->priv->slider_dragging) {
                /* If user is dragging the slider, we update displayed value */
                scale = gtk_range_get_value (GTK_RANGE (header->priv->scale));
                elapsed = (int) scale;
                ario_util_format_time_buf (elapsed, time, ARIO_MAX_TIME_SIZE);
                gtk_label_set_text (GTK_LABEL (header->priv->elapsed), time);
        }
}

void
ario_header_do_next (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (IS_ARIO_HEADER (header));
        /* Change to next song */
        ario_server_do_next ();
}

void
ario_header_do_previous (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (IS_ARIO_HEADER (header));
        /* Change to previous song */
        ario_server_do_prev ();
}

void
ario_header_playpause (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (IS_ARIO_HEADER (header));
        /* Play/pause on server */
        if (ario_server_is_paused ())
                ario_server_do_play ();
        else
                ario_server_do_pause ();
}

void
ario_header_stop (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (IS_ARIO_HEADER (header));
        /* Stop music */
        ario_server_do_stop ();
}

static void
ario_header_do_random (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (IS_ARIO_HEADER (header));
        /* Change random on server */
        ario_server_set_current_random (!ario_server_get_current_random ());
}

static void
ario_header_do_repeat (ArioHeader *header)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (IS_ARIO_HEADER (header));
        /* Change repeat on server */
        ario_server_set_current_repeat (!ario_server_get_current_repeat ());
}
