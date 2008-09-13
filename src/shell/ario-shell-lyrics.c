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
#include "widgets/ario-lyrics-editor.h"
#include "ario-util.h"

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
static void ario_shell_lyrics_add_to_queue (ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_song_changed_cb (ArioMpd *mpd,
                                               ArioShellLyrics *shell_lyrics);
static void ario_shell_lyrics_state_changed_cb (ArioMpd *mpd,
                                                ArioShellLyrics *shell_lyrics);

#define BASE_TITLE _("Lyrics")

enum
{
        PROP_0,
        PROP_MPD
};

struct ArioShellLyricsPrivate
{      
        GtkWidget *lyrics_editor;

        ArioMpd *mpd;
};

static gboolean is_instantiated = FALSE;

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

        g_signal_connect(shell_lyrics,
                         "delete_event",
                         G_CALLBACK (ario_shell_lyrics_window_delete_cb),
                         shell_lyrics);

        gtk_window_set_title (GTK_WINDOW (shell_lyrics), BASE_TITLE);
        gtk_window_set_resizable (GTK_WINDOW (shell_lyrics), FALSE);

        gtk_container_set_border_width (GTK_CONTAINER (shell_lyrics), 5);
}

GtkWidget *
ario_shell_lyrics_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyrics *shell_lyrics;
        GtkWidget *close_button;
        GList *childs_list;
        GtkWidget *hbox;

        if (is_instantiated)
                return NULL;
        else
                is_instantiated = TRUE;

        shell_lyrics = g_object_new (TYPE_ARIO_SHELL_LYRICS,
                                     "mpd", mpd,
                                     NULL);

        g_return_val_if_fail (shell_lyrics->priv != NULL, NULL);

        close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
        shell_lyrics->priv->lyrics_editor = ario_lyrics_editor_new ();

        childs_list = gtk_container_get_children (GTK_CONTAINER (shell_lyrics->priv->lyrics_editor));
        hbox = g_list_last (childs_list)->data;
        gtk_box_pack_end (GTK_BOX (hbox),
                          close_button,
                          FALSE, FALSE, 0);
        gtk_box_reorder_child (GTK_BOX (hbox), close_button, 0);

        gtk_container_add (GTK_CONTAINER (shell_lyrics),
                           shell_lyrics->priv->lyrics_editor);

        gtk_window_set_resizable (GTK_WINDOW (shell_lyrics), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell_lyrics), 350, 500);
        gtk_window_set_position (GTK_WINDOW (shell_lyrics), GTK_WIN_POS_CENTER);

        g_signal_connect (close_button,
                          "clicked",
                          G_CALLBACK (ario_shell_lyrics_close_cb),
                          shell_lyrics);

        ario_shell_lyrics_add_to_queue (shell_lyrics);
        ario_mpd_use_count_inc (shell_lyrics->priv->mpd);

        return GTK_WIDGET (shell_lyrics);
}

static void
ario_shell_lyrics_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyrics *shell_lyrics;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_LYRICS (object));

        shell_lyrics = ARIO_SHELL_LYRICS (object);

        g_return_if_fail (shell_lyrics->priv != NULL);
        ario_mpd_use_count_dec (shell_lyrics->priv->mpd);
        g_free (shell_lyrics->priv);

        is_instantiated = FALSE;

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
                g_signal_connect_object (shell_lyrics->priv->mpd,
                                         "song_changed",
                                         G_CALLBACK (ario_shell_lyrics_song_changed_cb),
                                         shell_lyrics, 0);
                g_signal_connect_object (shell_lyrics->priv->mpd,
                                         "state_changed",
                                         G_CALLBACK (ario_shell_lyrics_state_changed_cb),
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
ario_shell_lyrics_add_to_queue (ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsEditorData *data;
        gchar *window_title;

        data = (ArioLyricsEditorData *) g_malloc0 (sizeof (ArioLyricsEditorData));

        if (!ario_mpd_is_connected (shell_lyrics->priv->mpd)
            || ario_mpd_get_current_state (shell_lyrics->priv->mpd) == MPD_STATUS_STATE_STOP
            || ario_mpd_get_current_state (shell_lyrics->priv->mpd) == MPD_STATUS_STATE_UNKNOWN) {
                data->artist = NULL;
                data->title = NULL;
                window_title = g_strdup (BASE_TITLE);
        } else {
                data->artist = g_strdup (ario_mpd_get_current_artist (shell_lyrics->priv->mpd));
                data->title = ario_util_format_title (ario_mpd_get_current_song (shell_lyrics->priv->mpd));
                window_title = g_strdup_printf ("%s - %s", BASE_TITLE, data->title);
        }

        ario_lyrics_editor_push (ARIO_LYRICS_EDITOR (shell_lyrics->priv->lyrics_editor), data);
        gtk_window_set_title (GTK_WINDOW (shell_lyrics), window_title);
        g_free (window_title);
}

static void
ario_shell_lyrics_song_changed_cb (ArioMpd *mpd,
                                   ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_lyrics_add_to_queue (shell_lyrics);
}

static void
ario_shell_lyrics_state_changed_cb (ArioMpd *mpd,
                                    ArioShellLyrics *shell_lyrics)
{
        ARIO_LOG_FUNCTION_START

        ario_shell_lyrics_add_to_queue (shell_lyrics);
}

