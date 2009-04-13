/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "shell/ario-shell-lyricsselect.h"
#include <gtk/gtk.h>
#include <string.h>

#include <glib/gi18n.h>
#include "lyrics/ario-lyrics-manager.h"
#include "lib/gtk-builder-helpers.h"
#include "ario-debug.h"

#define CURRENT_LYRICS_SIZE 130

static void ario_shell_lyricsselect_finalize (GObject *object);
static GObject * ario_shell_lyricsselect_constructor (GType type, guint n_construct_properties,
                                                      GObjectConstructParam *construct_properties);
static gboolean ario_shell_lyricsselect_window_delete_cb (GtkWidget *window,
                                                          GdkEventAny *event,
                                                          ArioShellLyricsselect *shell_lyricsselect);
static void ario_shell_lyricsselect_search_cb (GtkWidget *widget,
                                               ArioShellLyricsselect *shell_lyricsselect);
static void ario_shell_lyricsselect_show_lyrics (ArioShellLyricsselect *shell_lyricsselect);

enum
{
        ARTIST_COLUMN,
        TITLE_COLUMN,
        PROVIDER_COLUMN,
        CANDIDATE_COLUMN,
        N_COLUMN
};

struct ArioShellLyricsselectPrivate
{
        GtkWidget *artist_entry;
        GtkWidget *title_entry;

        GtkWidget *artist_label;
        GtkWidget *title_label;

        GtkWidget *search_button;

        GtkWidget *treeview;
        GtkListStore *liststore;

        const gchar *file_artist;
        const gchar *file_title;

        GSList *lyrics;
};

#define ARIO_SHELL_LYRICSSELECT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_LYRICSSELECT, ArioShellLyricsselectPrivate))
G_DEFINE_TYPE (ArioShellLyricsselect, ario_shell_lyricsselect, GTK_TYPE_DIALOG)

static void
ario_shell_lyricsselect_class_init (ArioShellLyricsselectClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_shell_lyricsselect_finalize;
        object_class->constructor = ario_shell_lyricsselect_constructor;

        g_type_class_add_private (klass, sizeof (ArioShellLyricsselectPrivate));
}

static void
ario_shell_lyricsselect_init (ArioShellLyricsselect *shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START;
        shell_lyricsselect->priv = ARIO_SHELL_LYRICSSELECT_GET_PRIVATE (shell_lyricsselect);
        shell_lyricsselect->priv->lyrics = NULL;
}

static void
ario_shell_lyricsselect_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellLyricsselect *shell_lyricsselect;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_LYRICSSELECT (object));

        shell_lyricsselect = ARIO_SHELL_LYRICSSELECT (object);

        g_return_if_fail (shell_lyricsselect->priv != NULL);

        g_slist_foreach (shell_lyricsselect->priv->lyrics, (GFunc) ario_lyrics_candidate_free, NULL);
        g_slist_free (shell_lyricsselect->priv->lyrics);

        G_OBJECT_CLASS (ario_shell_lyricsselect_parent_class)->finalize (object);
}

static GObject *
ario_shell_lyricsselect_constructor (GType type, guint n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellLyricsselect *shell_lyricsselect;
        ArioShellLyricsselectClass *klass;
        GObjectClass *parent_class;
        GtkBuilder *builder;
        GtkWidget *vbox;

        klass = ARIO_SHELL_LYRICSSELECT_CLASS (g_type_class_peek (TYPE_ARIO_SHELL_LYRICSSELECT));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        shell_lyricsselect = ARIO_SHELL_LYRICSSELECT (parent_class->constructor (type, n_construct_properties,
                                                                                 construct_properties));

        builder = gtk_builder_helpers_new (UI_PATH "lyrics-select.ui",
                                           NULL);
        vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));
        shell_lyricsselect->priv->artist_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "artist_label"));
        shell_lyricsselect->priv->title_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "title_label"));
        shell_lyricsselect->priv->artist_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "artist_entry"));
        shell_lyricsselect->priv->title_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
        shell_lyricsselect->priv->search_button =
                GTK_WIDGET (gtk_builder_get_object (builder, "search_button"));
        shell_lyricsselect->priv->treeview =
                GTK_WIDGET (gtk_builder_get_object (builder, "treeview"));
        shell_lyricsselect->priv->liststore =
                GTK_LIST_STORE (gtk_builder_get_object (builder, "liststore"));

        gtk_builder_helpers_boldify_label (builder, "static_artist_label");
        gtk_builder_helpers_boldify_label (builder, "static_title_label");

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell_lyricsselect)->vbox),
                           vbox);

        gtk_window_set_title (GTK_WINDOW (shell_lyricsselect), _("Lyrics Download"));
        gtk_window_set_default_size (GTK_WINDOW (shell_lyricsselect), 520, 350);
        gtk_dialog_add_button (GTK_DIALOG (shell_lyricsselect),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (shell_lyricsselect),
                               GTK_STOCK_OK,
                               GTK_RESPONSE_OK);
        gtk_dialog_set_default_response (GTK_DIALOG (shell_lyricsselect),
                                         GTK_RESPONSE_OK);

        g_signal_connect (shell_lyricsselect,
                          "delete_event",
                          G_CALLBACK (ario_shell_lyricsselect_window_delete_cb),
                          shell_lyricsselect);
        g_signal_connect (shell_lyricsselect->priv->search_button,
                          "clicked", G_CALLBACK (ario_shell_lyricsselect_search_cb),
                          shell_lyricsselect);

        g_object_unref (builder);

        return G_OBJECT (shell_lyricsselect);
}

GtkWidget *
ario_shell_lyricsselect_new (const char *artist,
                             const char *title)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellLyricsselect *shell_lyricsselect;

        shell_lyricsselect = g_object_new (TYPE_ARIO_SHELL_LYRICSSELECT,
                                           NULL);

        shell_lyricsselect->priv->file_artist = artist;
        shell_lyricsselect->priv->file_title = title;

        gtk_entry_set_text (GTK_ENTRY (shell_lyricsselect->priv->artist_entry),
                            shell_lyricsselect->priv->file_artist);
        gtk_entry_set_text (GTK_ENTRY (shell_lyricsselect->priv->title_entry),
                            shell_lyricsselect->priv->file_title);

        gtk_label_set_label (GTK_LABEL (shell_lyricsselect->priv->artist_label),
                             shell_lyricsselect->priv->file_artist);
        gtk_label_set_label (GTK_LABEL (shell_lyricsselect->priv->title_label),
                             shell_lyricsselect->priv->file_title);

        g_return_val_if_fail (shell_lyricsselect->priv != NULL, NULL);

        return GTK_WIDGET (shell_lyricsselect);
}

static gboolean
ario_shell_lyricsselect_window_delete_cb (GtkWidget *window,
                                          GdkEventAny *event,
                                          ArioShellLyricsselect *shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START;
        gtk_widget_hide (GTK_WIDGET (shell_lyricsselect));
        return FALSE;
}

static void
ario_shell_lyricsselect_set_sensitive (ArioShellLyricsselect *shell_lyricsselect,
                                       gboolean sensitive)
{
        ARIO_LOG_FUNCTION_START;
        gtk_dialog_set_response_sensitive (GTK_DIALOG (shell_lyricsselect),
                                           GTK_RESPONSE_CLOSE,
                                           sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_lyricsselect->priv->artist_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_lyricsselect->priv->title_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_lyricsselect->priv->search_button), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_lyricsselect->priv->treeview), sensitive);

        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void
ario_shell_lyricsselect_search_cb (GtkWidget *widget,
                                   ArioShellLyricsselect *shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *artist;
        const gchar *title;

        ario_shell_lyricsselect_set_sensitive (shell_lyricsselect, FALSE);

        artist = gtk_entry_get_text (GTK_ENTRY (shell_lyricsselect->priv->artist_entry));
        title = gtk_entry_get_text (GTK_ENTRY (shell_lyricsselect->priv->title_entry));

        g_slist_foreach (shell_lyricsselect->priv->lyrics, (GFunc) ario_lyrics_candidate_free, NULL);
        g_slist_free (shell_lyricsselect->priv->lyrics);
        shell_lyricsselect->priv->lyrics = NULL;

        ario_lyrics_manager_get_lyrics_candidates (ario_lyrics_manager_get_instance(),
                                                   artist, title,
                                                   &shell_lyricsselect->priv->lyrics);

        ario_shell_lyricsselect_show_lyrics (shell_lyricsselect);

        ario_shell_lyricsselect_set_sensitive (shell_lyricsselect, TRUE);
}

static void
ario_shell_lyricsselect_show_lyrics (ArioShellLyricsselect *shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GSList *tmp;
        ArioLyricsCandidate *candidate;

        gtk_list_store_clear (shell_lyricsselect->priv->liststore);

        for (tmp = shell_lyricsselect->priv->lyrics; tmp; tmp = g_slist_next (tmp)) {
                candidate = (ArioLyricsCandidate *) tmp->data;
                gtk_list_store_append(shell_lyricsselect->priv->liststore,
                                      &iter);
                gtk_list_store_set (shell_lyricsselect->priv->liststore,
                                    &iter,
                                    ARTIST_COLUMN,
                                    candidate->artist,
                                    TITLE_COLUMN,
                                    candidate->title,
                                    PROVIDER_COLUMN,
                                    ario_lyrics_provider_get_name (candidate->lyrics_provider),
                                    CANDIDATE_COLUMN,
                                    candidate, -1);
        }

        /* Select first item in list */
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (shell_lyricsselect->priv->liststore), &iter))
                gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (shell_lyricsselect->priv->treeview)), &iter);
}

ArioLyricsCandidate *
ario_shell_lyricsselect_get_lyrics_candidate (ArioShellLyricsselect *shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START;
        GList* paths;
        GtkTreeIter iter;
        ArioLyricsCandidate *candidate;
        GtkTreeModel *tree_model = GTK_TREE_MODEL (shell_lyricsselect->priv->liststore);
        GtkTreePath *path;
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (shell_lyricsselect->priv->treeview));

        paths = gtk_tree_selection_get_selected_rows (selection,
                                                      &tree_model);
        if (!paths)
                return NULL;

        path = g_list_first(paths)->data;
        if (!path)
                return NULL;

        gtk_tree_model_get_iter (tree_model,
                                 &iter,
                                 path);
        gtk_tree_model_get (tree_model, &iter, CANDIDATE_COLUMN, &candidate, -1);

        return ario_lyrics_candidate_copy (candidate);
}

