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

#include "ario-information.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "covers/ario-cover-handler.h"
#include "preferences/ario-preferences.h"
#include "lib/ario-conf.h"
#include "lib/gtk-builder-helpers.h"
#include "lyrics/ario-lyrics.h"
#include "plugins/ario-plugin.h"
#include "servers/ario-server.h"

static void ario_information_finalize (GObject *object);
static void ario_information_fill_song (ArioInformation *information);
static void ario_information_fill_cover (ArioInformation *information);
static void ario_information_album_foreach (GtkWidget *widget,
                                            GtkContainer *container);
static void ario_information_fill_album (ArioInformation *information);
static void ario_information_state_changed_cb (ArioServer *server,
                                               ArioInformation *information);
static void ario_information_song_changed_cb (ArioServer *server,
                                              ArioInformation *information);
static void ario_information_cover_changed_cb (ArioCoverHandler *cover_handler,
                                               ArioInformation *information);
static void ario_information_album_changed_cb (ArioServer *server,
                                               ArioInformation *information);
static void ario_information_cover_drag_data_get_cb (GtkWidget *widget,
                                                     GdkDragContext *context,
                                                     GtkSelectionData *selection_data,
                                                     guint info, guint time, ArioServerAlbum *album);
static gboolean ario_information_cover_button_press_cb (GtkWidget *widget,
                                                        GdkEventButton *event,
                                                        ArioServerAlbum *album);

struct ArioInformationPrivate
{
        gboolean connected;

        GtkWidget *artist_label;
        GtkWidget *album_label;
        GtkWidget *title_label;
        GtkWidget *length_label;
        GtkWidget *lyrics_label;
        GtkWidget *lyrics_textview;
        GtkTextBuffer *lyrics_textbuffer;
        GtkWidget *cover_image;
        GtkWidget *properties_hbox;
        GtkWidget *albums_hbox;
        GtkWidget *albums_const_label;

        GSList *albums;

        gboolean selected;
};

/* Drag and drop target */
static const GtkTargetEntry criterias_targets  [] = {
        { "text/criterias-list", 0, 0 },
};

/* Object properties */
enum
{
        PROP_0,
};

G_DEFINE_TYPE_WITH_CODE (ArioInformation, ario_information, ARIO_TYPE_SOURCE, G_ADD_PRIVATE(ArioInformation))

static gchar *
ario_information_get_id (ArioSource *source)
{
        return "information";
}

static gchar *
ario_information_get_name (ArioSource *source)
{
        return _("Information");
}

static gchar *
ario_information_get_icon (ArioSource *source)
{
        return "media-optical";
}

static void
ario_information_select (ArioSource *source)
{
        ArioInformation *information = ARIO_INFORMATION (source);

        /* Fill all information when tab is selected */
        information->priv->selected = TRUE;

        /* Fill song info */
        ario_information_fill_song (information);

        /* Fill cover arts */
        ario_information_fill_cover (information);

        /* Fill album info */
        ario_information_fill_album (information);
}

static void
ario_information_unselect (ArioSource *source)
{
        ArioInformation *information = ARIO_INFORMATION (source);

        /* Remember to be lazy until tab is selected again */
        information->priv->selected = FALSE;
}

static void
ario_information_class_init (ArioInformationClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* GObject virtual methods */
        object_class->finalize = ario_information_finalize;

        /* ArioSource virtual methods */
        source_class->get_id = ario_information_get_id;
        source_class->get_name = ario_information_get_name;
        source_class->get_icon = ario_information_get_icon;
        source_class->select = ario_information_select;
        source_class->unselect = ario_information_unselect;
}

static gboolean
ario_information_button_press_cb (GtkWidget *widget,
                                  GdkEventButton *event,
                                  ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        return TRUE;
}

static void
ario_information_init (ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *scrolledwindow;
        GtkWidget *vbox, *vp;
        GtkBuilder *builder;
        gchar *file;

        information->priv = ario_information_get_instance_private (information);

        /* Get UI file in one of plugins directory */
        file = ario_plugin_find_file ("information.ui");
        g_return_if_fail (file);

        /* Create scrolled window */
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_NONE);

        /* Create viewport to have white background */
        vp = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolledwindow)),
                               gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow)));

        /* Create UI from file */
        builder = gtk_builder_helpers_new (file, information);
        g_free (file);

        /* Get main vbox */
        vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));

        /* Get pointers to various widgets */
        information->priv->artist_label = GTK_WIDGET (gtk_builder_get_object (builder, "artist_label"));
        information->priv->album_label = GTK_WIDGET (gtk_builder_get_object (builder, "album_label"));
        information->priv->title_label = GTK_WIDGET (gtk_builder_get_object (builder, "title_label"));
        information->priv->length_label = GTK_WIDGET (gtk_builder_get_object (builder, "length_label"));
        information->priv->lyrics_label = GTK_WIDGET (gtk_builder_get_object (builder, "lyrics_const_label"));
        information->priv->lyrics_textview = GTK_WIDGET (gtk_builder_get_object (builder, "lyrics_textview"));
        information->priv->cover_image = GTK_WIDGET (gtk_builder_get_object (builder, "cover_image"));
        information->priv->properties_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "properties_hbox"));
        information->priv->albums_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "albums_hbox"));
        information->priv->albums_const_label = GTK_WIDGET (gtk_builder_get_object (builder, "albums_const_label"));

        /* Get text view buffer */
        information->priv->lyrics_textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (information->priv->lyrics_textview));

        /* Change style of some labels */
        gtk_builder_helpers_boldify_label (builder, "artist_const_label");
        gtk_builder_helpers_boldify_label (builder, "album_const_label");
        gtk_builder_helpers_boldify_label (builder, "title_const_label");
        gtk_builder_helpers_boldify_label (builder, "length_const_label");
        gtk_builder_helpers_boldify_label (builder, "albums_const_label");
        gtk_builder_helpers_boldify_label (builder, "lyrics_const_label");

        /* Connect signal for current cover change */
        g_signal_connect (ario_cover_handler_get_instance (),
                          "cover_changed",
                          G_CALLBACK (ario_information_cover_changed_cb),
                          information);

        /* Widgets to scrolled window */
        gtk_container_add (GTK_CONTAINER (vp), vbox);
        gtk_container_add (GTK_CONTAINER (scrolledwindow), vp);

        g_signal_connect_object (scrolledwindow,
                                 "button_press_event",
                                 G_CALLBACK (ario_information_button_press_cb),
                                 information, 0);

        gtk_widget_show_all (scrolledwindow);

        /* Add scrolled window to information */
        gtk_box_pack_start (GTK_BOX (information), scrolledwindow, TRUE, TRUE, 0);
        g_object_unref (builder);
}

static void
ario_information_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioInformation *information;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_INFORMATION (object));

        information = ARIO_INFORMATION (object);

        g_return_if_fail (information->priv != NULL);

        /* Free a few data */
        if (information->priv->albums) {
                g_slist_foreach (information->priv->albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (information->priv->albums);
                information->priv->albums = NULL;
        }

        G_OBJECT_CLASS (ario_information_parent_class)->finalize (object);
}

GtkWidget *
ario_information_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioInformation *information;
        ArioServer *server = ario_server_get_instance ();

        information = g_object_new (TYPE_ARIO_INFORMATION,
                                    NULL);

        g_return_val_if_fail (information->priv != NULL, NULL);

        /* Signals to synchronize the information with server */
        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_information_state_changed_cb),
                                 information, 0);
        g_signal_connect_object (server,
                                 "song_changed",
                                 G_CALLBACK (ario_information_song_changed_cb),
                                 information, 0);
        g_signal_connect_object (server,
                                 "album_changed",
                                 G_CALLBACK (ario_information_album_changed_cb),
                                 information, 0);

        information->priv->connected = ario_server_is_connected ();

        return GTK_WIDGET (information);
}

static void
ario_information_fill_song (ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerSong *song;
        gchar *length;
        ArioLyrics *lyrics;
        int state;

        /* Do nothing if tab is not visible */
        if (!information->priv->selected)
                return;

        /* Get info on server */
        state = ario_server_get_current_state ();
        song = ario_server_get_current_song ();

        /* Hide song widgets in not connected or not playing */
        if (!information->priv->connected
            || !song
            || (state != ARIO_STATE_PLAY && state != ARIO_STATE_PAUSE)) {
                gtk_widget_hide (information->priv->properties_hbox);
                gtk_widget_hide (information->priv->lyrics_textview);
                gtk_widget_hide (information->priv->lyrics_label);
                return;
        }

        /* Show all song widgets */
        gtk_widget_show_all (information->priv->properties_hbox);

        /* set labels contents */
        gtk_label_set_text (GTK_LABEL (information->priv->title_label), song->title);
        gtk_label_set_text (GTK_LABEL (information->priv->artist_label), song->artist);
        gtk_label_set_text (GTK_LABEL (information->priv->album_label), song->album);
        length = ario_util_format_time (song->time);
        gtk_label_set_text (GTK_LABEL (information->priv->length_label), length);
        g_free (length);

        /* Get lyrics */
        lyrics = ario_lyrics_get_local_lyrics (song->artist, song->title);
        if (lyrics) {
                /* Set lyrics in text buffer */
                gtk_text_buffer_set_text (information->priv->lyrics_textbuffer, lyrics->lyrics, -1);
                gtk_widget_show (information->priv->lyrics_textview);
                gtk_widget_show (information->priv->lyrics_label);
                ario_lyrics_free (lyrics);
        } else {
                /* Hide text view if no lyrics found */
                gtk_widget_hide (information->priv->lyrics_textview);
                gtk_widget_hide (information->priv->lyrics_label);
        }
}

static void
ario_information_fill_cover (ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        GdkPixbuf *cover;

        /* Do nothing if tab is not visible */
        if (!information->priv->selected)
                return;

        /* Get current album cover */
        cover = ario_cover_handler_get_large_cover ();

        /* Set cover widget */
        gtk_image_set_from_pixbuf (GTK_IMAGE (information->priv->cover_image), cover);
}

static void
ario_information_album_foreach (GtkWidget *widget,
                                GtkContainer *container)
{
        /* Remove widget */
        gtk_container_remove (container, widget);
}

static void
ario_information_fill_album (ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerSong *song;
        int state;
        ArioServerAtomicCriteria atomic_criteria;
        ArioServerCriteria *criteria = NULL;
        GSList *tmp;
        ArioServerAlbum *album;
        gchar *cover_path;
        GdkPixbuf *pixbuf;
        GtkWidget *image;
        int nb = 0;
        GtkWidget *event_box;

        /* Do nothing if tab is not visible */
        if (!information->priv->selected)
                return;

        /* Remove all cover arts widgets */
        gtk_container_foreach (GTK_CONTAINER (information->priv->albums_hbox),
                               (GtkCallback) ario_information_album_foreach,
                               information->priv->albums_hbox);

        /* Free a few data */
        if (information->priv->albums) {
                g_slist_foreach (information->priv->albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (information->priv->albums);
                information->priv->albums = NULL;
        }
        gtk_widget_hide (information->priv->albums_const_label);

        /* Get info on server */
        state = ario_server_get_current_state ();
        song = ario_server_get_current_song ();

        /* Stop here is not connected or not playing */
        if (!information->priv->connected
            || !song
            || (state != ARIO_STATE_PLAY && state != ARIO_STATE_PAUSE)) {
                return;
        }

        /* Get all albums of current artist */
        criteria = g_slist_append (criteria, &atomic_criteria);
        atomic_criteria.tag = ARIO_TAG_ARTIST;
        atomic_criteria.value = song->artist;

        information->priv->albums = ario_server_get_albums (criteria);
        g_slist_free (criteria);

        /* For each album */
        for (tmp = information->priv->albums; tmp && nb < 8; tmp = g_slist_next (tmp)) {
                album = tmp->data;
                /* Do not consider current album */
                if ((!album->album && !song->album)
                    || (album->album && song->album && !strcmp (album->album, song->album)))
                        continue;

                /* Get albums cover */
                cover_path = ario_cover_make_cover_path (album->artist, album->album, SMALL_COVER);
                pixbuf = gdk_pixbuf_new_from_file_at_size (cover_path, COVER_SIZE, COVER_SIZE, NULL);
                g_free (cover_path);
                if (pixbuf) {
                        /* Cover found: create widgets to add cover art */
                        event_box = gtk_event_box_new ();
                        image = gtk_image_new_from_pixbuf (pixbuf);

                        /* Add drag and drop feature to image */
                        gtk_drag_source_set (event_box,
                                             GDK_BUTTON1_MASK,
                                             criterias_targets,
                                             G_N_ELEMENTS (criterias_targets),
                                             GDK_ACTION_COPY);
                        gtk_drag_source_set_icon_pixbuf (event_box, pixbuf);

                        g_signal_connect (event_box,
                                          "drag_data_get",
                                          G_CALLBACK (ario_information_cover_drag_data_get_cb), album);

                        g_signal_connect (event_box,
                                          "button_press_event",
                                          G_CALLBACK (ario_information_cover_button_press_cb), album);

                        gtk_container_add (GTK_CONTAINER (event_box), image);
                        gtk_box_pack_start (GTK_BOX (information->priv->albums_hbox), event_box, FALSE, FALSE, 0);
                        g_object_unref (pixbuf);
                        ++nb;
                }
        }

        /* Show albums widgets if there are some */
        if (nb > 0) {
                gtk_widget_show_all (information->priv->albums_hbox);
                gtk_widget_show (information->priv->albums_const_label);
        }
}

static void
ario_information_state_changed_cb (ArioServer *server,
                                   ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        information->priv->connected = ario_server_is_connected ();

        /* Fill song info */
        ario_information_fill_song (information);

        /* Fill cover arts */
        ario_information_fill_cover (information);

        /* Fill album info */
        ario_information_fill_album (information);
}

static void
ario_information_song_changed_cb (ArioServer *server,
                                  ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill song info */
        ario_information_fill_song (information);
}

static void
ario_information_cover_changed_cb (ArioCoverHandler *cover_handler,
                                   ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill cover arts */
        ario_information_fill_cover (information);
}

static void
ario_information_album_changed_cb (ArioServer *server,
                                   ArioInformation *information)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill album info */
        ario_information_fill_album (information);
}

static void
ario_information_cover_drag_data_get_cb (GtkWidget *widget,
                                         GdkDragContext *context,
                                         GtkSelectionData *selection_data,
                                         guint info, guint time, ArioServerAlbum *album)
{
        ARIO_LOG_FUNCTION_START;
        gchar *str;

        /* Get drag data corresponding to dragged album */
        str = g_strdup_printf ("2\n%d\n%s\n%d\n%s\n", ARIO_TAG_ARTIST, album->artist, ARIO_TAG_ALBUM, album->album);
        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (const guchar *) str,
                                strlen (str) * sizeof(guchar));

        g_free (str);
}

static gboolean
ario_information_cover_button_press_cb (GtkWidget *widget,
                                        GdkEventButton *event,
                                        ArioServerAlbum *album)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerAtomicCriteria atomic_criteria1;
        ArioServerAtomicCriteria atomic_criteria2;
        ArioServerCriteria *criteria = NULL;
        GSList *criterias = NULL;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                /* Double click on image */
                atomic_criteria1.tag = ARIO_TAG_ARTIST;
                atomic_criteria1.value = album->artist;
                atomic_criteria2.tag = ARIO_TAG_ALBUM;
                atomic_criteria2.value = album->album;

                criteria = g_slist_append (criteria, &atomic_criteria1);
                criteria = g_slist_append (criteria, &atomic_criteria2);

                criterias = g_slist_append (criterias, criteria);

                /* Add album to playlist */
                ario_server_playlist_append_criterias (criterias,
                                                       ario_conf_get_integer (PREF_DOUBLECLICK_BEHAVIOR, PREF_DOUBLECLICK_BEHAVIOR_DEFAULT),
                                                       -1);

                g_slist_free (criteria);
                g_slist_free (criterias);
        }

        return FALSE;
}

