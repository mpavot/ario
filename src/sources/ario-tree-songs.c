/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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

#include "sources/ario-tree-songs.h"
#include <gtk/gtk.h>
#include <string.h>
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "shell/ario-shell-songinfos.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_tree_songs_build_tree (ArioTree *parent_tree,
                                        GtkTreeView *treeview);
static void ario_tree_songs_fill_tree (ArioTree *parent_tree);
static void ario_tree_songs_get_drag_source (const GtkTargetEntry** targets,
                                             int* n_targets);
static void ario_tree_songs_append_drag_data (ArioTree *tree,
                                              GtkTreeModel *model,
                                              GtkTreeIter *iter,
                                              ArioTreeStringData *data);
static void ario_tree_songs_add_to_playlist (ArioTree *tree,
                                             const gboolean play);

enum
{
        SONG_VALUE_COLUMN,
        SONG_CRITERIA_COLUMN,
        SONG_TRACK_COLUMN,
        SONG_FILENAME_COLUMN,
        SONG_N_COLUMN
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

G_DEFINE_TYPE (ArioTreeSongs, ario_tree_songs, TYPE_ARIO_TREE)

static void
ario_tree_songs_class_init (ArioTreeSongsClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeClass *tree_class = ARIO_TREE_CLASS (klass);

        tree_class->build_tree = ario_tree_songs_build_tree;
        tree_class->fill_tree = ario_tree_songs_fill_tree;
        tree_class->get_drag_source = ario_tree_songs_get_drag_source;
        tree_class->append_drag_data = ario_tree_songs_append_drag_data;
        tree_class->add_to_playlist = ario_tree_songs_add_to_playlist;
}

static void
ario_tree_songs_init (ArioTreeSongs *tree)
{
        ARIO_LOG_FUNCTION_START;
}

static void
ario_tree_songs_build_tree (ArioTree *parent_tree,
                            GtkTreeView *treeview)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeSongs *tree;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        g_return_if_fail (IS_ARIO_TREE_SONGS (parent_tree));
        tree = ARIO_TREE_SONGS (parent_tree);

        /* Track column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Track"),
                                                           renderer,
                                                           "text", SONG_TRACK_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree->parent.tree), column);
        /* Title column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           renderer,
                                                           "text", SONG_VALUE_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree->parent.tree), column);

        tree->parent.model = gtk_list_store_new (SONG_N_COLUMN,
                                                 G_TYPE_STRING,
                                                 G_TYPE_POINTER,
                                                 G_TYPE_STRING,
                                                 G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->parent.model),
                                              SONG_TRACK_COLUMN,
                                              GTK_SORT_ASCENDING);
}

static void
ario_tree_songs_add_next_songs (ArioTreeSongs *tree,
                                const GSList *songs,
                                ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START;
        const GSList *tmp;
        ArioServerSong *song;
        GtkTreeIter iter;
        gchar track[ARIO_MAX_TRACK_SIZE];
        gchar *title;

        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                gtk_list_store_append (tree->parent.model, &iter);

                ario_util_format_track_buf (song->track, track, ARIO_MAX_TRACK_SIZE);
                title = ario_util_format_title (song);
                gtk_list_store_set (tree->parent.model, &iter,
                                    SONG_VALUE_COLUMN, title,
                                    SONG_CRITERIA_COLUMN, criteria,
                                    SONG_TRACK_COLUMN, track,
                                    SONG_FILENAME_COLUMN, song->file,
                                    -1);
        }
}

static void
ario_tree_songs_fill_tree (ArioTree *parent_tree)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeSongs *tree;
        GSList *songs, *tmp;

        g_return_if_fail (IS_ARIO_TREE_SONGS (parent_tree));
        tree = ARIO_TREE_SONGS (parent_tree);

        gtk_list_store_clear (tree->parent.model);

        for (tmp = tree->parent.criterias; tmp; tmp = g_slist_next (tmp)) {
                songs = ario_server_get_songs (tmp->data, TRUE);
                ario_tree_songs_add_next_songs (tree, songs, tmp->data);
                g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (songs);
        }
}

static void
ario_tree_songs_get_drag_source (const GtkTargetEntry** targets,
                                 int* n_targets)
{
        ARIO_LOG_FUNCTION_START;
        *targets = songs_targets;
        *n_targets = G_N_ELEMENTS (songs_targets);
}

static void
ario_tree_songs_append_drag_data (ArioTree *tree,
                                  GtkTreeModel *model,
                                  GtkTreeIter *iter,
                                  ArioTreeStringData *data)
{
        ARIO_LOG_FUNCTION_START;
        gchar *val;

        gtk_tree_model_get (model, iter, SONG_FILENAME_COLUMN, &val, -1);
        g_string_append (data->string, val);
        g_string_append (data->string, "\n");

        g_free (val);
}

static void
get_selected_songs_foreach (GtkTreeModel *model,
                            GtkTreePath *path,
                            GtkTreeIter *iter,
                            gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **songs = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, SONG_FILENAME_COLUMN, &val, -1);

        *songs = g_slist_append (*songs, val);
}

void
ario_tree_songs_cmd_songs_properties (ArioTreeSongs *tree)
{
        ARIO_LOG_FUNCTION_START;
        GSList *paths = NULL;
        GtkWidget *songinfos;

        gtk_tree_selection_selected_foreach (tree->parent.selection,
                                             get_selected_songs_foreach,
                                             &paths);

        if (paths) {
                songinfos = ario_shell_songinfos_new (paths);
                if (songinfos)
                        gtk_widget_show_all (songinfos);

                g_slist_foreach (paths, (GFunc) g_free, NULL);
                g_slist_free (paths);
        }
}

static void
ario_tree_songs_add_to_playlist (ArioTree *tree,
                                 const gboolean play)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;

        gtk_tree_selection_selected_foreach (tree->selection,
                                             get_selected_songs_foreach,
                                             &songs);

        if (songs) {
                ario_server_playlist_append_songs (songs, play);

                g_slist_foreach (songs, (GFunc) g_free, NULL);
                g_slist_free (songs);
        }

}
