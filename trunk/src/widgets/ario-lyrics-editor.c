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
#include "widgets/ario-lyrics-editor.h"
#include "shell/ario-shell-lyricsselect.h"
#include "ario-debug.h"
#include "ario-lyrics.h"
#include "ario-util.h"

static void ario_lyrics_editor_class_init (ArioLyricsEditorClass *klass);
static void ario_lyrics_editor_init (ArioLyricsEditor *lyrics_editor);
static void ario_lyrics_editor_finalize (GObject *object);
static void ario_lyrics_editor_save_cb (GtkButton *button,
                                        ArioLyricsEditor *lyrics_editor);
static void ario_lyrics_editor_search_cb (GtkButton *button,
                                          ArioLyricsEditor *lyrics_editor);
static void ario_lyrics_editor_free_data (ArioLyricsEditorData *data);
static void ario_lyrics_editor_get_lyrics_thread (ArioLyricsEditor *lyrics_editor);
static void ario_lyrics_editor_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                                      ArioLyricsEditor *lyrics_editor);

struct ArioLyricsEditorPrivate
{
        GtkTextBuffer *textbuffer;
        GtkWidget *textview;
        GtkWidget *save_button;
        GtkWidget *search_button;

        GThread *thread;
        GAsyncQueue *queue;

        ArioLyricsEditorData *data;
};

static GObjectClass *parent_class = NULL;

GType
ario_lyrics_editor_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_lyrics_editor_type = 0;

        if (ario_lyrics_editor_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioLyricsEditorClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_lyrics_editor_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioLyricsEditor),
                        0,
                        (GInstanceInitFunc) ario_lyrics_editor_init
                };

                ario_lyrics_editor_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                  "ArioLyricsEditor",
                                                                  &our_info, 0);
        }

        return ario_lyrics_editor_type;
}

static void
ario_lyrics_editor_class_init (ArioLyricsEditorClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_lyrics_editor_finalize;
}

static void
ario_lyrics_editor_init (ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START
        lyrics_editor->priv = g_new0 (ArioLyricsEditorPrivate, 1);
}

GtkWidget *
ario_lyrics_editor_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsEditor *lyrics_editor;
        GtkWidget *scrolledwindow;
        GtkWidget *separator;
        GtkWidget *hbox;

        lyrics_editor = g_object_new (TYPE_ARIO_LYRICS_EDITOR,
                                      NULL);

        g_return_val_if_fail (lyrics_editor->priv != NULL, NULL);

        gtk_box_set_spacing (GTK_BOX (lyrics_editor), 5);
        hbox = gtk_hbox_new (FALSE, 5);
        separator = gtk_hseparator_new ();
        lyrics_editor->priv->save_button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
        lyrics_editor->priv->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

        lyrics_editor->priv->textview = gtk_text_view_new ();
        lyrics_editor->priv->textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (lyrics_editor->priv->textview));

        gtk_container_add (GTK_CONTAINER (scrolledwindow),
                           lyrics_editor->priv->textview);

        gtk_box_pack_start (GTK_BOX (lyrics_editor),
                            scrolledwindow,
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (lyrics_editor),
                            separator,
                            FALSE, FALSE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          lyrics_editor->priv->save_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          lyrics_editor->priv->search_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (lyrics_editor),
                            hbox,
                            FALSE, FALSE, 0);

        g_signal_connect_object (G_OBJECT (lyrics_editor->priv->textbuffer),
                                 "changed",
                                 G_CALLBACK (ario_lyrics_editor_textbuffer_changed_cb),
                                 lyrics_editor, 0);

        g_signal_connect_object (G_OBJECT (lyrics_editor->priv->save_button),
                                 "clicked",
                                 G_CALLBACK (ario_lyrics_editor_save_cb),
                                 lyrics_editor, 0);

        g_signal_connect_object (G_OBJECT (lyrics_editor->priv->search_button),
                                 "clicked",
                                 G_CALLBACK (ario_lyrics_editor_search_cb),
                                 lyrics_editor, 0);
        g_object_ref (lyrics_editor->priv->textbuffer);
        g_object_ref (lyrics_editor->priv->textview);
        lyrics_editor->priv->queue = g_async_queue_new ();

        lyrics_editor->priv->thread = g_thread_create ((GThreadFunc) ario_lyrics_editor_get_lyrics_thread,
                                                       lyrics_editor,
                                                       TRUE,
                                                       NULL);
        return GTK_WIDGET (lyrics_editor);
}

static void
ario_lyrics_editor_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsEditor *lyrics_editor;
        ArioLyricsEditorData *data;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_LYRICS_EDITOR (object));

        lyrics_editor = ARIO_LYRICS_EDITOR (object);

        g_return_if_fail (lyrics_editor->priv != NULL);

        while ((data = (ArioLyricsEditorData *) g_async_queue_try_pop (lyrics_editor->priv->queue))) {
                ario_lyrics_editor_free_data (data);
        }
        data = (ArioLyricsEditorData *) g_malloc0 (sizeof (ArioLyricsEditorData));
        data->finalize = TRUE;
        g_async_queue_push (lyrics_editor->priv->queue, data);
        g_thread_join (lyrics_editor->priv->thread);
        g_async_queue_unref (lyrics_editor->priv->queue);
        g_object_unref (lyrics_editor->priv->textview);
        g_object_unref (lyrics_editor->priv->textbuffer);
        if (lyrics_editor->priv->data) {
                ario_lyrics_editor_free_data (lyrics_editor->priv->data);
                lyrics_editor->priv->data = NULL;
        }

        g_free (lyrics_editor->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_lyrics_editor_save_cb (GtkButton *button,
                            ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START
        GtkTextIter start;
        GtkTextIter end;
        gchar *lyrics;

        gtk_text_buffer_get_bounds (lyrics_editor->priv->textbuffer,
                                    &start, &end);

        lyrics = gtk_text_buffer_get_text (lyrics_editor->priv->textbuffer,
                                           &start, &end,
                                           TRUE);

        ario_lyrics_save_lyrics (lyrics_editor->priv->data->artist,
                                 lyrics_editor->priv->data->title,
                                 lyrics);

        gtk_widget_set_sensitive (lyrics_editor->priv->save_button, FALSE);
}

static void
ario_lyrics_editor_search_cb (GtkButton *button,
                              ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *lyricsselect;
        ArioLyricsCandidate *candidate;
        ArioLyricsEditorData *data;
        char *artist;

        artist = lyrics_editor->priv->data->artist;

        lyricsselect = ario_shell_lyricsselect_new (artist, lyrics_editor->priv->data->title);

        if (gtk_dialog_run (GTK_DIALOG (lyricsselect)) == GTK_RESPONSE_OK) {
                candidate = ario_shell_lyricsselect_get_lyrics_candidate (ARIO_SHELL_LYRICSSELECT (lyricsselect));
                if (candidate) {
                        data = (ArioLyricsEditorData *) g_malloc0 (sizeof (ArioLyricsEditorData));
                        data->artist = g_strdup (artist);
                        data->title = g_strdup (lyrics_editor->priv->data->title);
                        data->hid = g_strdup (candidate->hid);

                        g_async_queue_push (lyrics_editor->priv->queue, data);

                        ario_lyrics_candidates_free (candidate);
                }
        }
        gtk_widget_destroy (lyricsselect);
}

static void
ario_lyrics_editor_free_data (ArioLyricsEditorData *data)
{
        ARIO_LOG_FUNCTION_START
        if (data) {
                g_free (data->artist);
                g_free (data->title);
                g_free (data->hid);
                g_free (data);
        }
}

static void
ario_lyrics_editor_get_lyrics_thread (ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsEditorData *data;
        ArioLyrics *lyrics;

        g_async_queue_ref (lyrics_editor->priv->queue);

        while (TRUE) {
                data = (ArioLyricsEditorData *) g_async_queue_pop (lyrics_editor->priv->queue);
                if (data->finalize) {
                        ario_lyrics_editor_free_data (data);
                        break;
                }

                gtk_widget_set_sensitive (lyrics_editor->priv->save_button, FALSE);
                g_signal_handlers_block_by_func (G_OBJECT (lyrics_editor->priv->textbuffer),
                                                 G_CALLBACK (ario_lyrics_editor_textbuffer_changed_cb),
                                                 lyrics_editor);

                gtk_text_buffer_set_text (lyrics_editor->priv->textbuffer, _("Downloading lyrics..."), -1);

                if (data->hid) {
                        lyrics = ario_lyrics_get_lyrics_from_hid (data->artist,
                                                                  data->title,
                                                                  data->hid);
                } else {
                        lyrics = ario_lyrics_get_lyrics (data->artist,
                                                         data->title);
                }

                if (lyrics
                    && lyrics->lyrics
                    && strlen (lyrics->lyrics)) {
                        gtk_text_buffer_set_text (lyrics_editor->priv->textbuffer, lyrics->lyrics, -1);
                } else {
                        gtk_text_buffer_set_text (lyrics_editor->priv->textbuffer, _("Lyrics not found"), -1);
                }
                ario_lyrics_free (lyrics);
                ario_lyrics_editor_free_data (lyrics_editor->priv->data);
                lyrics_editor->priv->data = data;
                g_signal_handlers_unblock_by_func (G_OBJECT (lyrics_editor->priv->textbuffer),
                                                   G_CALLBACK (ario_lyrics_editor_textbuffer_changed_cb),
                                                   lyrics_editor);
        }

        g_async_queue_unref (lyrics_editor->priv->queue);
}

void
ario_lyrics_editor_push (ArioLyricsEditor *lyrics_editor,
                         ArioLyricsEditorData *data)
{
        ARIO_LOG_FUNCTION_START
        g_async_queue_push (lyrics_editor->priv->queue, data);
}

static void
ario_lyrics_editor_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                          ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_set_sensitive (lyrics_editor->priv->save_button, TRUE);
}

