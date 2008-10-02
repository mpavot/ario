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
#include "widgets/ario-lyrics-editor.h"

#define ARIO_PREVIOUS 1
#define ARIO_NEXT 2

static void ario_shell_songinfos_finalize (GObject *object);
static gboolean ario_shell_songinfos_window_delete_cb (GtkWidget *window,
                                                       GdkEventAny *event,
                                                       ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_response_cb (GtkDialog *dialog,
                                              int response_id,
                                              ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_set_current_song (ArioShellSonginfos *shell_songinfos);

struct ArioShellSonginfosPrivate
{
        GtkWidget *notebook;

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

        GtkWidget *lyrics_editor;

        GtkWidget *previous_button;
        GtkWidget *next_button;
};

#define ARIO_SHELL_SONGINFOS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_SONGINFOS, ArioShellSonginfosPrivate))
G_DEFINE_TYPE (ArioShellSonginfos, ario_shell_songinfos, GTK_TYPE_DIALOG)

static void
ario_shell_songinfos_class_init (ArioShellSonginfosClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_shell_songinfos_finalize;
        
        g_type_class_add_private (klass, sizeof (ArioShellSonginfosPrivate));
}

static void
ario_shell_songinfos_init (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        shell_songinfos->priv = ARIO_SHELL_SONGINFOS_GET_PRIVATE (shell_songinfos);

        g_signal_connect (shell_songinfos,
                          "delete_event",
                          G_CALLBACK (ario_shell_songinfos_window_delete_cb),
                          shell_songinfos);
        g_signal_connect (shell_songinfos,
                          "response",
                          G_CALLBACK (ario_shell_songinfos_response_cb),
                          shell_songinfos);

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
ario_shell_songinfos_new (GList *songs)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos;
        GtkWidget *widget;
        GladeXML *xml;

        shell_songinfos = g_object_new (TYPE_ARIO_SHELL_SONGINFOS, NULL);

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

        rb_glade_boldify_label (xml, "frame_label");
        rb_glade_boldify_label (xml, "title_const_label");
        rb_glade_boldify_label (xml, "artist_const_label");
        rb_glade_boldify_label (xml, "album_const_label");
        rb_glade_boldify_label (xml, "track_const_label");
        rb_glade_boldify_label (xml, "length_const_label");
        rb_glade_boldify_label (xml, "date_const_label");
        rb_glade_boldify_label (xml, "file_const_label");
        rb_glade_boldify_label (xml, "genre_const_label");
        rb_glade_boldify_label (xml, "composer_const_label");
        rb_glade_boldify_label (xml, "performer_const_label");
        rb_glade_boldify_label (xml, "disc_const_label");
        rb_glade_boldify_label (xml, "comment_const_label");

        g_object_unref (G_OBJECT (xml));

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

        shell_songinfos->priv->lyrics_editor = ario_lyrics_editor_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_songinfos->priv->notebook),
                                  shell_songinfos->priv->lyrics_editor,
                                  gtk_label_new (_("Lyrics")));

        shell_songinfos->priv->songs = songs;
        ario_shell_songinfos_set_current_song (shell_songinfos);

        return GTK_WIDGET (shell_songinfos);
}

static void
ario_shell_songinfos_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_SONGINFOS (object));

        shell_songinfos = ARIO_SHELL_SONGINFOS (object);

        g_return_if_fail (shell_songinfos->priv != NULL);

        shell_songinfos->priv->songs = g_list_first (shell_songinfos->priv->songs);
        g_list_foreach (shell_songinfos->priv->songs, (GFunc) ario_server_free_song, NULL);
        g_list_free (shell_songinfos->priv->songs);

        G_OBJECT_CLASS (ario_shell_songinfos_parent_class)->finalize (object);
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
                        ario_shell_songinfos_set_current_song (shell_songinfos);
                }
                break;
        case ARIO_NEXT:
                if (g_list_next (shell_songinfos->priv->songs)) {
                        shell_songinfos->priv->songs = g_list_next (shell_songinfos->priv->songs);
                        ario_shell_songinfos_set_current_song (shell_songinfos);
                }
                break;
        }
}

static void
ario_shell_songinfos_set_current_song (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerSong *song;
        gchar *length;
        gchar *window_title;
        ArioLyricsEditorData *data;

        if (!shell_songinfos->priv->songs)
                return;

        song = shell_songinfos->priv->songs->data;
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

        data = (ArioLyricsEditorData *) g_malloc0 (sizeof (ArioLyricsEditorData));
        data->artist = g_strdup (song->artist);
        data->title = ario_util_format_title (song);
        ario_lyrics_editor_push (ARIO_LYRICS_EDITOR (shell_songinfos->priv->lyrics_editor), data);

        window_title = g_strdup_printf ("%s - %s", _("Song Properties"), data->title);
        gtk_window_set_title (GTK_WINDOW (shell_songinfos), window_title);
        g_free (window_title);
}
