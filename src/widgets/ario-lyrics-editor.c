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

#include "widgets/ario-lyrics-editor.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "shell/ario-shell-lyricsselect.h"
#include "lyrics/ario-lyrics.h"
#include "lyrics/ario-lyrics-manager.h"

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

G_DEFINE_TYPE_WITH_CODE (ArioLyricsEditor, ario_lyrics_editor, GTK_TYPE_BOX, G_ADD_PRIVATE(ArioLyricsEditor))

static void
ario_lyrics_editor_class_init (ArioLyricsEditorClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* Virtual methods */
        object_class->finalize = ario_lyrics_editor_finalize;
}

static void
ario_lyrics_editor_init (ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START;
        lyrics_editor->priv = ario_lyrics_editor_get_instance_private (lyrics_editor);
}

GtkWidget *
ario_lyrics_editor_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsEditor *lyrics_editor;
        GtkWidget *scrolledwindow;
        GtkWidget *separator;
        GtkWidget *hbox;

        lyrics_editor = g_object_new (TYPE_ARIO_LYRICS_EDITOR,
                                      NULL);
        g_return_val_if_fail (lyrics_editor->priv != NULL, NULL);

        gtk_orientable_set_orientation (GTK_ORIENTABLE (lyrics_editor), GTK_ORIENTATION_VERTICAL);
        gtk_box_set_spacing (GTK_BOX (lyrics_editor), 5);

        /* Create button hbox */
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

        /* Create separator */
        separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

        /* Create buttons */
        lyrics_editor->priv->save_button = gtk_button_new_from_icon_name ("document-save", GTK_ICON_SIZE_BUTTON);
        lyrics_editor->priv->search_button = gtk_button_new_from_icon_name ("edit-find", GTK_ICON_SIZE_BUTTON);

        /* Create scrolled window */
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

        /* Create text view */
        lyrics_editor->priv->textview = gtk_text_view_new ();
        lyrics_editor->priv->textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (lyrics_editor->priv->textview));

        gtk_container_add (GTK_CONTAINER (scrolledwindow),
                           lyrics_editor->priv->textview);

        /* Add buttons to hbox */
        gtk_box_pack_end (GTK_BOX (hbox),
                          lyrics_editor->priv->save_button,
                          FALSE, FALSE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          lyrics_editor->priv->search_button,
                          FALSE, FALSE, 0);

        /* Add widgets to ArioLyricsEditor */
        gtk_box_pack_start (GTK_BOX (lyrics_editor),
                            scrolledwindow,
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (lyrics_editor),
                            separator,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (lyrics_editor),
                            hbox,
                            FALSE, FALSE, 0);

        /* Connect signals */
        g_signal_connect (lyrics_editor->priv->textbuffer,
                          "changed",
                          G_CALLBACK (ario_lyrics_editor_textbuffer_changed_cb),
                          lyrics_editor);

        g_signal_connect (lyrics_editor->priv->save_button,
                          "clicked",
                          G_CALLBACK (ario_lyrics_editor_save_cb),
                          lyrics_editor);

        g_signal_connect (lyrics_editor->priv->search_button,
                          "clicked",
                          G_CALLBACK (ario_lyrics_editor_search_cb),
                          lyrics_editor);
        g_object_ref (lyrics_editor->priv->textbuffer);
        g_object_ref (lyrics_editor->priv->textview);

        /* Create asynchronous queue for lyrics loads */
        lyrics_editor->priv->queue = g_async_queue_new ();

        /* Thread for lyrics download */
        lyrics_editor->priv->thread = g_thread_new ("lyricsdl",
                                                    (GThreadFunc) ario_lyrics_editor_get_lyrics_thread,
                                                    lyrics_editor);
        return GTK_WIDGET (lyrics_editor);
}

static void
ario_lyrics_editor_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsEditor *lyrics_editor;
        ArioLyricsEditorData *data;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_LYRICS_EDITOR (object));

        lyrics_editor = ARIO_LYRICS_EDITOR (object);

        g_return_if_fail (lyrics_editor->priv != NULL);

        /* Delete all data in queue */
        while ((data = (ArioLyricsEditorData *) g_async_queue_try_pop (lyrics_editor->priv->queue))) {
                ario_lyrics_editor_free_data (data);
        }
        /* Add a special ArioLyricsEditorData to finalize the lyrics download thread */
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

        G_OBJECT_CLASS (ario_lyrics_editor_parent_class)->finalize (object);
}

static void
ario_lyrics_editor_save_cb (GtkButton *button,
                            ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START;
        GtkTextIter start;
        GtkTextIter end;
        gchar *lyrics;

        if (!lyrics_editor->priv->data)
                return;

        /* Get lyrics from text view */
        gtk_text_buffer_get_bounds (lyrics_editor->priv->textbuffer,
                                    &start, &end);

        lyrics = gtk_text_buffer_get_text (lyrics_editor->priv->textbuffer,
                                           &start, &end,
                                           TRUE);

        /* Save lyrics */
        ario_lyrics_save_lyrics (lyrics_editor->priv->data->artist,
                                 lyrics_editor->priv->data->title,
                                 lyrics);

        /* Set save button insensitive */
        gtk_widget_set_sensitive (lyrics_editor->priv->save_button, FALSE);
}

static void
ario_lyrics_editor_search_cb (GtkButton *button,
                              ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *lyricsselect;
        ArioLyricsCandidate *candidate;
        ArioLyricsEditorData *data;
        char *artist;
        char *title;

        if (!lyrics_editor->priv->data)
                return;

        artist = lyrics_editor->priv->data->artist;
        title = lyrics_editor->priv->data->title;

        /* Launch lyrics selection dialog */
        lyricsselect = ario_shell_lyricsselect_new (artist, title);
        if (gtk_dialog_run (GTK_DIALOG (lyricsselect)) == GTK_RESPONSE_OK) {
                candidate = ario_shell_lyricsselect_get_lyrics_candidate (ARIO_SHELL_LYRICSSELECT (lyricsselect));
                if (candidate) {
                        /* Push lyrics candidate to queue */
                        data = (ArioLyricsEditorData *) g_malloc0 (sizeof (ArioLyricsEditorData));
                        data->artist = g_strdup (artist);
                        data->title = g_strdup (title);
                        data->candidate = candidate;

                        g_async_queue_push (lyrics_editor->priv->queue, data);
                }
        }
        gtk_widget_destroy (lyricsselect);
}

static void
ario_lyrics_editor_free_data (ArioLyricsEditorData *data)
{
        ARIO_LOG_FUNCTION_START;
        if (data) {
                g_free (data->artist);
                g_free (data->title);
                ario_lyrics_candidate_free (data->candidate);
                g_free (data);
        }
}

typedef struct
{
        ArioLyricsEditor *lyrics_editor;
        gchar *text;
} ArioLyricsEditorTextData;

static gboolean
ario_lyrics_editor_set_text (ArioLyricsEditorTextData * text_data)
{
        gtk_text_buffer_set_text (text_data->lyrics_editor->priv->textbuffer, text_data->text, -1);
        g_free (text_data->text);
        g_free (text_data);

        return FALSE;
}

static void
ario_lyrics_editor_get_lyrics_thread (ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsEditorData *data;
        ArioLyrics *lyrics;

        g_async_queue_ref (lyrics_editor->priv->queue);

        /* Main loop of lyrics download thread */
        while (TRUE) {
                /* Wait for data in the queue */
                data = (ArioLyricsEditorData *) g_async_queue_pop (lyrics_editor->priv->queue);
                if (data->finalize) {
                        /* Last element in queue. Exit from main loop */
                        ario_lyrics_editor_free_data (data);
                        break;
                }

                gtk_widget_set_sensitive (lyrics_editor->priv->save_button, FALSE);

                /* Block signal to modify the text view */
                g_signal_handlers_block_by_func (G_OBJECT (lyrics_editor->priv->textbuffer),
                                                 G_CALLBACK (ario_lyrics_editor_textbuffer_changed_cb),
                                                 lyrics_editor);

                /* Set temporary text for lyrics download */
                ArioLyricsEditorTextData * text_data = (ArioLyricsEditorTextData *) g_malloc0 (sizeof (ArioLyricsEditorTextData));
                text_data->lyrics_editor = lyrics_editor;
                text_data->text = g_strdup(_("Downloading lyrics..."));
                g_idle_add ((GSourceFunc) ario_lyrics_editor_set_text, text_data);

                if (data->candidate) {
                        /* We already know which lyrics to use */
                        lyrics = ario_lyrics_provider_get_lyrics_from_candidate (data->candidate->lyrics_provider,
                                                                                 data->candidate);
                } else {
                        /* We need to download the lyrics using the lyrics manager */
                        lyrics = ario_lyrics_manager_get_lyrics (ario_lyrics_manager_get_instance (),
                                                                 data->artist,
                                                                 data->title,
                                                                 NULL);
                }

                if (lyrics
                    && lyrics->lyrics
                    && strlen (lyrics->lyrics)) {
                        /* Lyrics found */
                        ArioLyricsEditorTextData * text_data2 = (ArioLyricsEditorTextData *) g_malloc0 (sizeof (ArioLyricsEditorTextData));
                        text_data2->lyrics_editor = lyrics_editor;
                        text_data2->text = lyrics->lyrics;
                        lyrics->lyrics = NULL;
                        g_idle_add ((GSourceFunc) ario_lyrics_editor_set_text, text_data2);
                } else {
                        /* Lyrics not found */
                        ArioLyricsEditorTextData * text_data2 = (ArioLyricsEditorTextData *) g_malloc0 (sizeof (ArioLyricsEditorTextData));
                        text_data2->lyrics_editor = lyrics_editor;
                        text_data2->text = g_strdup(_("Lyrics not found"));
                        g_idle_add ((GSourceFunc) ario_lyrics_editor_set_text, text_data2);
                }
                ario_lyrics_free (lyrics);
                ario_lyrics_editor_free_data (lyrics_editor->priv->data);

                /* Set lyrics as current data */
                lyrics_editor->priv->data = data;

                /* Unblock signal of text view modification */
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
        ARIO_LOG_FUNCTION_START;
        g_async_queue_push (lyrics_editor->priv->queue, data);
}

static void
ario_lyrics_editor_textbuffer_changed_cb (GtkTextBuffer *textbuffer,
                                          ArioLyricsEditor *lyrics_editor)
{
        ARIO_LOG_FUNCTION_START;
        /* Set save button sensitive */
        gtk_widget_set_sensitive (lyrics_editor->priv->save_button, TRUE);
}

