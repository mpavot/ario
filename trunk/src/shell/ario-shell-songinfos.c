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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "shell/ario-shell-songinfos.h"
#include "lib/rb-glade-helpers.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "ario-lyrics.h"
#include "shell/ario-shell-lyricsselect.h"

#define ARIO_PREVIOUS 1
#define ARIO_NEXT 2

typedef struct ArioShellSonginfosData
{
        gchar *artist;
        gchar *title;
        gchar *hid;
        gboolean finalize;
} ArioShellSonginfosData;

static void ario_shell_songinfos_class_init (ArioShellSonginfosClass *klass);
static void ario_shell_songinfos_init (ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_finalize (GObject *object);
static void ario_shell_songinfos_set_property (GObject *object,
                                               guint prop_id,
                                               const GValue *value,
                                               GParamSpec *pspec);
static void ario_shell_songinfos_get_property (GObject *object,
                                               guint prop_id,
                                               GValue *value,
                                               GParamSpec *pspec);
static gboolean ario_shell_songinfos_window_delete_cb (GtkWidget *window,
                                                       GdkEventAny *event,
                                                       ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_response_cb (GtkDialog *dialog,
                                              int response_id,
                                              ArioShellSonginfos *shell_songinfos);
static void ario_shell_set_current_song (ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_search_cb (GtkButton *button,
                                            ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_save_cb (GtkButton *button,
                                          ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                                        ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_get_lyrics_thread (ArioShellSonginfos *shell_songinfos);

enum
{
        PROP_0,
        PROP_MPD
};

struct ArioShellSonginfosPrivate
{
        GtkWidget *notebook;

        ArioMpd *mpd;

        GList *songs;

        GtkWidget *title_label;
        GtkWidget *artist_label;
        GtkWidget *album_label;
        GtkWidget *track_label;
        GtkWidget *length_label;
        GtkWidget *date_label;
        GtkWidget *file_label;
        GtkWidget *genre_label;
        GtkWidget *composer_label;
        GtkWidget *performer_label;
        GtkWidget *disc_label;
        GtkWidget *comment_label;

        GtkTextBuffer *textbuffer;
        GtkWidget *textview;
        GtkWidget *save_button;
        GtkWidget *search_button;

        GtkWidget *previous_button;
        GtkWidget *next_button;

        GThread *thread;
        GAsyncQueue *queue;
        ArioLyrics *lyrics;
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_songinfos_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_songinfos_type = 0;

        if (ario_shell_songinfos_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellSonginfosClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_songinfos_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellSonginfos),
                        0,
                        (GInstanceInitFunc) ario_shell_songinfos_init
                };

                ario_shell_songinfos_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                                    "ArioShellSonginfos",
                                                                    &our_info, 0);
        }

        return ario_shell_songinfos_type;
}

static void
ario_shell_songinfos_class_init (ArioShellSonginfosClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_songinfos_finalize;
        object_class->set_property = ario_shell_songinfos_set_property;
        object_class->get_property = ario_shell_songinfos_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_shell_songinfos_init (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        shell_songinfos->priv = g_new0 (ArioShellSonginfosPrivate, 1);

        g_signal_connect_object (G_OBJECT (shell_songinfos),
                                 "delete_event",
                                 G_CALLBACK (ario_shell_songinfos_window_delete_cb),
                                 shell_songinfos, 0);
        g_signal_connect_object (G_OBJECT (shell_songinfos),
                                 "response",
                                 G_CALLBACK (ario_shell_songinfos_response_cb),
                                 shell_songinfos, 0);

        shell_songinfos->priv->previous_button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
        gtk_dialog_add_action_widget (GTK_DIALOG (shell_songinfos),
                                      shell_songinfos->priv->previous_button,
                                      ARIO_PREVIOUS);

        shell_songinfos->priv->next_button = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
        gtk_dialog_add_action_widget (GTK_DIALOG (shell_songinfos),
                                      shell_songinfos->priv->next_button,
                                      ARIO_NEXT);

        gtk_dialog_add_button (GTK_DIALOG (shell_songinfos),
                               GTK_STOCK_CLOSE,
                               GTK_RESPONSE_CLOSE);

        gtk_dialog_set_default_response (GTK_DIALOG (shell_songinfos),
                                         GTK_RESPONSE_CLOSE);

        gtk_window_set_title (GTK_WINDOW (shell_songinfos), _("Song Properties"));
        gtk_window_set_resizable (GTK_WINDOW (shell_songinfos), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell_songinfos), 450, 350);

        shell_songinfos->priv->notebook = GTK_WIDGET (gtk_notebook_new ());
        gtk_container_set_border_width (GTK_CONTAINER (shell_songinfos->priv->notebook), 5);

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell_songinfos)->vbox),
                           shell_songinfos->priv->notebook);

        gtk_container_set_border_width (GTK_CONTAINER (shell_songinfos), 5);
        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (shell_songinfos)->vbox), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (shell_songinfos), FALSE);
}

GtkWidget *
ario_shell_songinfos_new (ArioMpd *mpd,
                          GList *songs)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos;
        GtkWidget *widget;
        GladeXML *xml;
        GtkWidget *scrolledwindow;
        GtkWidget *vbox;
        GtkWidget *hbox;

        shell_songinfos = g_object_new (TYPE_ARIO_SHELL_SONGINFOS,
                                        "mpd", mpd,
                                        NULL);

        g_return_val_if_fail (shell_songinfos->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "song-infos.glade",
                                "vbox",
                                shell_songinfos);

        widget = glade_xml_get_widget (xml, "vbox");

        shell_songinfos->priv->title_label = 
                glade_xml_get_widget (xml, "title_label");
        shell_songinfos->priv->artist_label = 
                glade_xml_get_widget (xml, "artist_label");
        shell_songinfos->priv->album_label = 
                glade_xml_get_widget (xml, "album_label");
        shell_songinfos->priv->track_label = 
                glade_xml_get_widget (xml, "track_label");
        shell_songinfos->priv->length_label = 
                glade_xml_get_widget (xml, "length_label");
        shell_songinfos->priv->date_label = 
                glade_xml_get_widget (xml, "date_label");
        shell_songinfos->priv->file_label = 
                glade_xml_get_widget (xml, "file_label");
        shell_songinfos->priv->genre_label = 
                glade_xml_get_widget (xml, "genre_label");
        shell_songinfos->priv->composer_label = 
                glade_xml_get_widget (xml, "composer_label");
        shell_songinfos->priv->performer_label = 
                glade_xml_get_widget (xml, "performer_label");
        shell_songinfos->priv->disc_label = 
                glade_xml_get_widget (xml, "disc_label");
        shell_songinfos->priv->comment_label = 
                glade_xml_get_widget (xml, "comment_label");

        gtk_widget_set_size_request(shell_songinfos->priv->title_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->artist_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->album_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->track_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->length_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->date_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->file_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->genre_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->composer_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->performer_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->disc_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->comment_label, 280, -1);

        gtk_notebook_append_page (GTK_NOTEBOOK (shell_songinfos->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Song Properties")));

        vbox = gtk_vbox_new (FALSE, 5);
        hbox = gtk_hbox_new (FALSE, 5);
        shell_songinfos->priv->save_button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
        shell_songinfos->priv->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

        shell_songinfos->priv->textview = gtk_text_view_new ();
        shell_songinfos->priv->textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (shell_songinfos->priv->textview));

        gtk_container_add (GTK_CONTAINER (scrolledwindow),
                           shell_songinfos->priv->textview);

        gtk_box_pack_start (GTK_BOX (vbox),
                            scrolledwindow,
                            TRUE, TRUE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          shell_songinfos->priv->save_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          shell_songinfos->priv->search_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            hbox,
                            FALSE, FALSE, 0);

        g_signal_connect_object (G_OBJECT (shell_songinfos->priv->textbuffer),
                                 "changed",
                                 G_CALLBACK (ario_shell_songinfos_textbuffer_changed_cb),
                                 shell_songinfos, 0);

        g_signal_connect_object (G_OBJECT (shell_songinfos->priv->save_button),
                                 "clicked",
                                 G_CALLBACK (ario_shell_songinfos_save_cb),
                                 shell_songinfos, 0);
                                 
        g_signal_connect_object (G_OBJECT (shell_songinfos->priv->search_button),
                                 "clicked",
                                 G_CALLBACK (ario_shell_songinfos_search_cb),
                                 shell_songinfos, 0);
                                 
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_songinfos->priv->notebook),
                                  vbox,
                                  gtk_label_new (_("Lyrics")));

        shell_songinfos->priv->queue = g_async_queue_new ();

        shell_songinfos->priv->thread = g_thread_create ((GThreadFunc) ario_shell_songinfos_get_lyrics_thread,
                                                         shell_songinfos,
                                                         TRUE,
                                                         NULL);

        shell_songinfos->priv->songs = songs;
        ario_shell_set_current_song (shell_songinfos);

        return GTK_WIDGET (shell_songinfos);
}

static void
ario_shell_songinfos_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos;
        ArioShellSonginfosData *data;
        
        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_SONGINFOS (object));

        shell_songinfos = ARIO_SHELL_SONGINFOS (object);

        g_return_if_fail (shell_songinfos->priv != NULL);

        shell_songinfos->priv->songs = g_list_first (shell_songinfos->priv->songs);
        g_list_foreach (shell_songinfos->priv->songs, (GFunc) ario_mpd_free_song, NULL);
        g_list_free (shell_songinfos->priv->songs);

        ario_lyrics_free (shell_songinfos->priv->lyrics);
        shell_songinfos->priv->lyrics = NULL;
        data = (ArioShellSonginfosData *) g_malloc0 (sizeof (ArioShellSonginfosData));
        data->finalize = TRUE;
        g_async_queue_push (shell_songinfos->priv->queue, data);
        g_thread_join (shell_songinfos->priv->thread);
        g_async_queue_unref (shell_songinfos->priv->queue);

        g_free (shell_songinfos->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_shell_songinfos_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos = ARIO_SHELL_SONGINFOS (object);

        switch (prop_id) {
        case PROP_MPD:
                shell_songinfos->priv->mpd = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_shell_songinfos_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos = ARIO_SHELL_SONGINFOS (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, shell_songinfos->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
ario_shell_songinfos_window_delete_cb (GtkWidget *window,
                                       GdkEventAny *event,
                                       ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (shell_songinfos));
        gtk_widget_destroy (GTK_WIDGET (shell_songinfos));

        return TRUE;
}

static void
ario_shell_songinfos_response_cb (GtkDialog *dialog,
                                  int response_id,
                                  ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        switch (response_id) {
        case GTK_RESPONSE_CLOSE:
                gtk_widget_hide (GTK_WIDGET (shell_songinfos));
                gtk_widget_destroy (GTK_WIDGET (shell_songinfos));
                break;
        case ARIO_PREVIOUS:
                if (g_list_previous (shell_songinfos->priv->songs)) {
                        shell_songinfos->priv->songs = g_list_previous (shell_songinfos->priv->songs);
                        ario_shell_set_current_song (shell_songinfos);
                }
                break;
        case ARIO_NEXT:
                if (g_list_next (shell_songinfos->priv->songs)) {
                        shell_songinfos->priv->songs = g_list_next (shell_songinfos->priv->songs);
                        ario_shell_set_current_song (shell_songinfos);
                }
                break;
        }
}

static void
ario_shell_set_current_song (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdSong *song = shell_songinfos->priv->songs->data;
        gchar *length;
        gchar *window_title;
        ArioShellSonginfosData *data;

        if (song) {
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->title_label), song->title);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->artist_label), song->artist);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->album_label), song->album);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->track_label), song->track);
                length = ario_util_format_time (song->time);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->length_label), length);
                g_free (length);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->date_label), song->date);                
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->file_label), song->file);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->genre_label), song->genre);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->composer_label), song->composer);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->performer_label), song->performer);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->disc_label), song->disc);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->comment_label), song->comment);
        }

        gtk_widget_set_sensitive (shell_songinfos->priv->previous_button, g_list_previous (shell_songinfos->priv->songs) != NULL);
        gtk_widget_set_sensitive (shell_songinfos->priv->next_button, g_list_next (shell_songinfos->priv->songs) != NULL);

        data = (ArioShellSonginfosData *) g_malloc0 (sizeof (ArioShellSonginfosData));
        data->artist = g_strdup (song->artist);
        data->title = ario_util_format_title (song);

        window_title = g_strdup_printf (_("Song Properties - %s"), data->title);
        gtk_window_set_title (GTK_WINDOW (shell_songinfos), window_title);
        
        g_async_queue_push (shell_songinfos->priv->queue, data);
}

static void
ario_shell_songinfos_save_cb (GtkButton *button,
                              ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        GtkTextIter start;
        GtkTextIter end;
        gchar *lyrics;
        gchar *title;
        ArioMpdSong *song = shell_songinfos->priv->songs->data;

        gtk_text_buffer_get_bounds (shell_songinfos->priv->textbuffer,
                                    &start, &end);

        lyrics = gtk_text_buffer_get_text (shell_songinfos->priv->textbuffer,
                                           &start, &end,
                                           TRUE);

        title = ario_util_format_title (song);
        ario_lyrics_save_lyrics (song->artist,
                                 title,
                                 lyrics);

        gtk_widget_set_sensitive (shell_songinfos->priv->save_button, FALSE);
        g_free (title);
}

static void
ario_shell_songinfos_search_cb (GtkButton *button,
                                ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *lyricsselect;
        ArioLyricsCandidate *candidate;
        ArioShellSonginfosData *data;
        char *title;
        ArioMpdSong *song = shell_songinfos->priv->songs->data;

        title = ario_util_format_title (song);

        lyricsselect = ario_shell_lyricsselect_new (song->artist, title);

        if (gtk_dialog_run (GTK_DIALOG (lyricsselect)) == GTK_RESPONSE_OK) {
                candidate = ario_shell_lyricsselect_get_lyrics_candidate (ARIO_SHELL_LYRICSSELECT (lyricsselect));
                if (candidate) {
                        data = (ArioShellSonginfosData *) g_malloc0 (sizeof (ArioShellSonginfosData));
                        data->artist = g_strdup (song->artist);
                        data->title = g_strdup (title);
                        data->hid = g_strdup (candidate->hid);

                        g_async_queue_push (shell_songinfos->priv->queue, data);

                        ario_lyrics_candidates_free (candidate);
                }
        }
        gtk_widget_destroy (lyricsselect);
        g_free (title);
}

static void
ario_shell_songinfos_free_data (ArioShellSonginfosData *data)
{
        ARIO_LOG_FUNCTION_START
        g_free (data->artist);
        g_free (data->title);
        g_free (data->hid);
        g_free (data);
}

static void
ario_shell_songinfos_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                            ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_set_sensitive (shell_songinfos->priv->save_button, TRUE);
}

static void
ario_shell_songinfos_get_lyrics_thread (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfosData *data;

        g_async_queue_ref (shell_songinfos->priv->queue);

        while (TRUE) {
                data = (ArioShellSonginfosData *) g_async_queue_pop (shell_songinfos->priv->queue);
                if (data->finalize) {
                        ario_shell_songinfos_free_data (data);
                        break;
                }
                gtk_widget_set_sensitive (shell_songinfos->priv->save_button, FALSE);
                g_signal_handlers_block_by_func (G_OBJECT (shell_songinfos->priv->textbuffer),
                                                 G_CALLBACK (ario_shell_songinfos_textbuffer_changed_cb),
                                                 shell_songinfos);

                shell_songinfos->priv->textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (shell_songinfos->priv->textview));
                gtk_text_buffer_set_text (shell_songinfos->priv->textbuffer, _("Downloading lyrics..."), -1);

                ario_lyrics_free (shell_songinfos->priv->lyrics);
                if (data->hid) {
                        shell_songinfos->priv->lyrics = ario_lyrics_get_lyrics_from_hid (data->artist,
                                                                                         data->title,
                                                                                         data->hid);
                } else {
                        shell_songinfos->priv->lyrics = ario_lyrics_get_lyrics (data->artist,
                                                                                data->title);
                }

                if (shell_songinfos->priv->lyrics
                    && shell_songinfos->priv->lyrics->lyrics
                    && strlen (shell_songinfos->priv->lyrics->lyrics)) {
                        gtk_text_buffer_set_text (shell_songinfos->priv->textbuffer, shell_songinfos->priv->lyrics->lyrics, -1);
                } else {
                        gtk_text_buffer_set_text (shell_songinfos->priv->textbuffer, _("Lyrics not found"), -1);
                }
                ario_shell_songinfos_free_data (data);
                g_signal_handlers_unblock_by_func (G_OBJECT (shell_songinfos->priv->textbuffer),
                                                   G_CALLBACK (ario_shell_songinfos_textbuffer_changed_cb),
                                                   shell_songinfos);
        }

        g_async_queue_unref (shell_songinfos->priv->queue);
}

