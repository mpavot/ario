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
#include "shell/ario-shell-lyrics.h"
#include "ario-debug.h"
#include "ario-lyrics.h"
#include "ario-util.h"

typedef struct ArioShellLyricsData
{
        gchar *artist;
        gchar *title;
        gboolean finalize;
} ArioShellLyricsData;

static void ario_shell_lyrics_class_init (ArioShellLyricsClass *klass);
static void ario_shell_lyrics_init (ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_finalize (GObject *object);
static void ario_shell_lyrics_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void ario_shell_lyrics_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);
static gboolean ario_shell_lyrics_window_delete_cb (GtkWidget *window,
                                                   GdkEventAny *event,
                                                   ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_close_cb (GtkButton *button,
                                        ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_save_cb (GtkButton *button,
                                       ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_free_data (ArioShellLyricsData *data);
static void ario_shell_lyrics_get_lyrics_thread (ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_add_to_queue (ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_song_changed_cb (ArioMpd *mpd,
                                               ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                                     ArioShellLyrics *shell_lyrics);

enum
{
        PROP_0,
        PROP_MPD
};

struct ArioShellLyricsPrivate
{
        GtkTextBuffer *textbuffer;
        GtkWidget *textview;
        GtkWidget *save_button;

        ArioMpd *mpd;

        ArioLyrics *lyrics;

        GThread *thread;
        GAsyncQueue *queue;
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_lyrics_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_lyrics_type = 0;

        if (ario_shell_lyrics_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellLyricsClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_lyrics_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellLyrics),
                        0,
                        (GInstanceInitFunc) ario_shell_lyrics_init
                };

                ario_shell_lyrics_type = g_type_register_static (GTK_TYPE_WINDOW,
                                                                 "ArioShellLyrics",
                                                                 &our_info, 0);
        }

        return ario_shell_lyrics_type;
}

static void
ario_shell_lyrics_class_init (ArioShellLyricsClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_lyrics_finalize;
        object_class->set_property = ario_shell_lyrics_set_property;
        object_class->get_property = ario_shell_lyrics_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_shell_lyrics_init (ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        shell_lyrics->priv = g_new0 (ArioShellLyricsPrivate, 1);

        g_signal_connect_object (G_OBJECT (shell_lyrics),
                                 "delete_event",
                                 G_CALLBACK (ario_shell_lyrics_window_delete_cb),
                                 shell_lyrics, 0);

        gtk_window_set_title (GTK_WINDOW (shell_lyrics), _("Ario Lyrics"));
        gtk_window_set_resizable (GTK_WINDOW (shell_lyrics), FALSE);

        gtk_container_set_border_width (GTK_CONTAINER (shell_lyrics), 5);
}

GtkWidget *
ario_shell_lyrics_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyrics *shell_lyrics;
        GtkWidget *scrolledwindow;
        GtkWidget *separator;
        GtkWidget *vbox;
        GtkWidget *hbox;
        GtkWidget *close_button;

        shell_lyrics = g_object_new (TYPE_ARIO_SHELL_LYRICS,
                                   "mpd", mpd,
                                   NULL);

        g_return_val_if_fail (shell_lyrics->priv != NULL, NULL);

        vbox = gtk_vbox_new (FALSE, 5);
        hbox = gtk_hbox_new (FALSE, 5);
        separator = gtk_hseparator_new ();
        close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
        shell_lyrics->priv->save_button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

        shell_lyrics->priv->textview = gtk_text_view_new ();
        shell_lyrics->priv->textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (shell_lyrics->priv->textview));

        gtk_container_add (GTK_CONTAINER (scrolledwindow),
                           shell_lyrics->priv->textview);

        gtk_box_pack_start (GTK_BOX (vbox),
                            scrolledwindow,
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            separator,
                            FALSE, FALSE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          close_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          shell_lyrics->priv->save_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            hbox,
                            FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (shell_lyrics),
                           vbox);

        gtk_window_set_resizable (GTK_WINDOW (shell_lyrics), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell_lyrics), 350, 500);
        gtk_window_set_position (GTK_WINDOW (shell_lyrics), GTK_WIN_POS_CENTER);

        g_signal_connect_object (G_OBJECT (shell_lyrics->priv->textbuffer),
                                 "changed",
                                 G_CALLBACK (ario_shell_lyrics_textbuffer_changed_cb),
                                 shell_lyrics, 0);

        g_signal_connect_object (G_OBJECT (close_button),
                                 "clicked",
                                 G_CALLBACK (ario_shell_lyrics_close_cb),
                                 shell_lyrics, 0);

        g_signal_connect_object (G_OBJECT (shell_lyrics->priv->save_button),
                                 "clicked",
                                 G_CALLBACK (ario_shell_lyrics_save_cb),
                                 shell_lyrics, 0);

        shell_lyrics->priv->queue = g_async_queue_new ();

        shell_lyrics->priv->thread = g_thread_create ((GThreadFunc) ario_shell_lyrics_get_lyrics_thread,
                                                      shell_lyrics,
                                                      TRUE,
                                                      NULL);

        ario_shell_lyrics_add_to_queue (shell_lyrics);

        return GTK_WIDGET (shell_lyrics);
}

static void
ario_shell_lyrics_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyrics *shell_lyrics;
        ArioShellLyricsData *data;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_LYRICS (object));

        shell_lyrics = ARIO_SHELL_LYRICS (object);

        g_return_if_fail (shell_lyrics->priv != NULL);

        ario_lyrics_free (shell_lyrics->priv->lyrics);
        shell_lyrics->priv->lyrics = NULL;
        data = (ArioShellLyricsData *) g_malloc0 (sizeof (ArioShellLyricsData));
        data->finalize = TRUE;
        g_async_queue_push (shell_lyrics->priv->queue, data);
        g_thread_join (shell_lyrics->priv->thread);

        g_free (shell_lyrics->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_shell_lyrics_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyrics *shell_lyrics = ARIO_SHELL_LYRICS (object);
        
        switch (prop_id) {
        case PROP_MPD:
                shell_lyrics->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (shell_lyrics->priv->mpd),
                                         "song_changed", G_CALLBACK (ario_shell_lyrics_song_changed_cb),
                                         shell_lyrics, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_shell_lyrics_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyrics *shell_lyrics = ARIO_SHELL_LYRICS (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, shell_lyrics->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
ario_shell_lyrics_window_delete_cb (GtkWidget *window,
                                    GdkEventAny *event,
                                    ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (shell_lyrics));
        gtk_widget_destroy (GTK_WIDGET (shell_lyrics));

        return TRUE;
}

static void
ario_shell_lyrics_close_cb (GtkButton *button,
                            ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (shell_lyrics));
        gtk_widget_destroy (GTK_WIDGET (shell_lyrics));
}

static void
ario_shell_lyrics_save_cb (GtkButton *button,
                           ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        GtkTextIter start;
        GtkTextIter end;
        gchar *lyrics;
        gchar *title;

        gtk_text_buffer_get_bounds (shell_lyrics->priv->textbuffer,
                                    &start, &end);

        lyrics = gtk_text_buffer_get_text (shell_lyrics->priv->textbuffer,
                                           &start, &end,
                                           TRUE);

        title = ario_util_format_title (ario_mpd_get_current_song (shell_lyrics->priv->mpd));
        ario_lyrics_save_lyrics (ario_mpd_get_current_artist (shell_lyrics->priv->mpd),
                                 title,
                                 lyrics);
        g_free (title);
}

static void
ario_shell_lyrics_free_data (ArioShellLyricsData *data)
{
        ARIO_LOG_FUNCTION_START
        g_free (data->artist);
        g_free (data->title);
        g_free (data);
}

static void
ario_shell_lyrics_get_lyrics_thread (ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyricsData *data;

        g_async_queue_ref (shell_lyrics->priv->queue);

        while (TRUE) {
                data = (ArioShellLyricsData *) g_async_queue_pop (shell_lyrics->priv->queue);
                if (data->finalize) {
                        break;
                }
                gtk_widget_set_sensitive (shell_lyrics->priv->save_button, FALSE);
                g_signal_handlers_block_by_func (G_OBJECT (shell_lyrics->priv->textbuffer),
                                                 G_CALLBACK (ario_shell_lyrics_textbuffer_changed_cb),
                                                 shell_lyrics);

                shell_lyrics->priv->textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (shell_lyrics->priv->textview));
                gtk_text_buffer_set_text (shell_lyrics->priv->textbuffer, _("Downloading lyrics..."), -1);

                ario_lyrics_free (shell_lyrics->priv->lyrics);
                shell_lyrics->priv->lyrics = ario_lyrics_get_lyrics (data->artist,
                                                                     data->title);

                if (shell_lyrics->priv->lyrics
                    && shell_lyrics->priv->lyrics->lyrics
                    && strlen (shell_lyrics->priv->lyrics->lyrics)) {
                        gtk_text_buffer_set_text (shell_lyrics->priv->textbuffer, shell_lyrics->priv->lyrics->lyrics, -1);
                } else {
                        gtk_text_buffer_set_text (shell_lyrics->priv->textbuffer, _("Lyrics not found"), -1);
                }
                ario_shell_lyrics_free_data (data);
                g_signal_handlers_unblock_by_func (G_OBJECT (shell_lyrics->priv->textbuffer),
                                                   G_CALLBACK (ario_shell_lyrics_textbuffer_changed_cb),
                                                   shell_lyrics);
        }

        g_async_queue_unref (shell_lyrics->priv->queue);
}

static void
ario_shell_lyrics_add_to_queue (ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyricsData *data;

        data = (ArioShellLyricsData *) g_malloc0 (sizeof (ArioShellLyricsData));

        data->artist = g_strdup (ario_mpd_get_current_artist (shell_lyrics->priv->mpd));
        data->title = ario_util_format_title (ario_mpd_get_current_song (shell_lyrics->priv->mpd));

        g_async_queue_push (shell_lyrics->priv->queue, data);
}

static void
ario_shell_lyrics_song_changed_cb (ArioMpd *mpd,
                                   ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_lyrics_add_to_queue (shell_lyrics);
}

static void
ario_shell_lyrics_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                         ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_set_sensitive (shell_lyrics->priv->save_button, TRUE);
}

