/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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

#include "ario-filesystem.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <libxml/parser.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "plugins/ario-plugin.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "servers/ario-server.h"
#include "shell/ario-shell-songinfos.h"
#include "widgets/ario-dnd-tree.h"
#include "widgets/ario-songlist.h"

#define ROOT "/"

static void ario_filesystem_shutdown (ArioSource *source);
static void ario_filesystem_state_changed_cb (ArioServer *server,
                                              ArioFilesystem *filesystem);
static void ario_filesystem_filesystem_changed_cb (ArioServer *server,
                                                   ArioFilesystem *filesystem);
static void ario_filesystem_cmd_add_filesystem (GSimpleAction *action,
                                                GVariant *parameter,
                                                gpointer data);
static void ario_filesystem_cmd_add_play_filesystem (GSimpleAction *action,
                                                     GVariant *parameter,
                                                     gpointer data);
static void ario_filesystem_cmd_clear_add_play_filesystem (GSimpleAction *action,
                                                           GVariant *parameter,
                                                           gpointer data);
static void ario_filesystem_popup_menu_cb (ArioDndTree* tree,
                                           ArioFilesystem *filesystem);
static void ario_filesystem_filetree_drag_data_get_cb (GtkWidget * widget,
                                                       GdkDragContext * context,
                                                       GtkSelectionData * selection_data,
                                                       guint info, guint time, gpointer data);
static void ario_filesystem_filetree_row_activated_cb (GtkTreeView *tree_view,
                                                       GtkTreePath *path,
                                                       GtkTreeViewColumn *column,
                                                       ArioFilesystem *filesystem);
static gboolean ario_filesystem_filetree_row_expanded_cb (GtkTreeView *tree_view,
                                                          GtkTreeIter *iter,
                                                          GtkTreePath *path,
                                                          ArioFilesystem *filesystem);
static void ario_filesystem_cursor_moved_cb (GtkTreeView *tree_view,
                                             ArioFilesystem *filesystem);
static void ario_filesystem_fill_filesystem (ArioFilesystem *filesystem);

struct ArioFilesystemPrivate
{
        GtkWidget *tree;
        GtkTreeStore *model;
        GtkTreeSelection *selection;

        GtkWidget *paned;
        GtkWidget *songs;

        gboolean connected;
        gboolean empty;

        GtkWidget *menu;
};

/* Actions on directories */
static const GActionEntry ario_filesystem_actions [] =
{
        { "filesystem-add-to-pl", ario_filesystem_cmd_add_filesystem },
        { "filesystem-add-play", ario_filesystem_cmd_add_play_filesystem },
        { "filesystem-clear-add-play", ario_filesystem_cmd_clear_add_play_filesystem },
};
static guint ario_filesystem_n_actions = G_N_ELEMENTS (ario_filesystem_actions);

/* Actions on songs */
static const GActionEntry ario_filesystem_songs_actions[] = {
        { "filesystem-add-to-pl-songs", ario_songlist_cmd_add_songlists },
        { "filesystem-add-play-songs", ario_songlist_cmd_add_play_songlists },
        { "filesystem-clear-add-play-songs", ario_songlist_cmd_clear_add_play_songlists },
        { "filesystem-properties-songs", ario_songlist_cmd_songs_properties },
};
static guint ario_filesystem_songs_n_actions = G_N_ELEMENTS (ario_filesystem_songs_actions);

/* Object properties */
enum
{
        PROP_0,
};

/* Directory tree columns */
enum
{
        FILETREE_ICON_COLUMN,
        FILETREE_ICONSIZE_COLUMN,
        FILETREE_NAME_COLUMN,
        FILETREE_DIR_COLUMN,
        FILETREE_N_COLUMN
};

/* Drag and drop targets */
static const GtkTargetEntry dirs_targets  [] = {
        { "text/directory", 0, 0 },
};

G_DEFINE_TYPE_WITH_CODE (ArioFilesystem, ario_filesystem, ARIO_TYPE_SOURCE, G_ADD_PRIVATE(ArioFilesystem))

static gchar *
ario_filesystem_get_id (ArioSource *source)
{
        return "filesystem";
}

static gchar *
ario_filesystem_get_name (ArioSource *source)
{
        return _("File System");
}

static gchar *
ario_filesystem_get_icon (ArioSource *source)
{
        return "drive-harddisk";
}

static void
ario_filesystem_select (ArioSource *source)
{
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (source);

        /* Fill directory tree on the first time filesystem tab is selected */
        if (filesystem->priv->empty)
                ario_filesystem_fill_filesystem (filesystem);
}

static void
ario_filesystem_class_init (ArioFilesystemClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* ArioSource virtual methods */
        source_class->get_id = ario_filesystem_get_id;
        source_class->get_name = ario_filesystem_get_name;
        source_class->get_icon = ario_filesystem_get_icon;
        source_class->shutdown = ario_filesystem_shutdown;
        source_class->select = ario_filesystem_select;
}

static void
ario_filesystem_init (ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        int pos;
        GtkWidget *scrolledwindow_filesystem;

        filesystem->priv = ario_filesystem_get_instance_private (filesystem);

        filesystem->priv->connected = FALSE;
        filesystem->priv->empty = TRUE;

        /* Create scrolled window */
        scrolledwindow_filesystem = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_filesystem);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_filesystem), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_filesystem), GTK_SHADOW_IN);

        /* Create drag and drop tree */
        filesystem->priv->tree = ario_dnd_tree_new (dirs_targets,
                                                    G_N_ELEMENTS (dirs_targets),
                                                    TRUE);
        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (filesystem->priv->tree), TRUE);

        /* Add folder icon column */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "icon-name", FILETREE_ICON_COLUMN, "stock-size", FILETREE_ICONSIZE_COLUMN, NULL);

        /* Add directory name column */
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer, "text", FILETREE_NAME_COLUMN, NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 50);
        gtk_tree_view_append_column (GTK_TREE_VIEW (filesystem->priv->tree), column);

        /* Create tree model */
        filesystem->priv->model = gtk_tree_store_new (FILETREE_N_COLUMN,
                                                      G_TYPE_STRING,
                                                      G_TYPE_UINT,
                                                      G_TYPE_STRING,
                                                      G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (filesystem->priv->model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (filesystem->priv->tree),
                                 GTK_TREE_MODEL (filesystem->priv->model));

        /* Enable search */
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (filesystem->priv->tree), TRUE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (filesystem->priv->tree), FILETREE_NAME_COLUMN);

        /* Get tree selection */
        filesystem->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (filesystem->priv->tree));
        gtk_tree_selection_set_mode (filesystem->priv->selection,
                                     GTK_SELECTION_BROWSE);

        /* Add tree to scrolled window */
        gtk_container_add (GTK_CONTAINER (scrolledwindow_filesystem), filesystem->priv->tree);

        /* Connect signals for actions on folder tree */
        g_signal_connect (filesystem->priv->tree,
                          "drag_data_get",
                          G_CALLBACK (ario_filesystem_filetree_drag_data_get_cb), filesystem);
        g_signal_connect (filesystem->priv->tree,
                          "popup",
                          G_CALLBACK (ario_filesystem_popup_menu_cb), filesystem);
        g_signal_connect (filesystem->priv->tree,
                          "row-activated",
                          G_CALLBACK (ario_filesystem_filetree_row_activated_cb),
                          filesystem);
        g_signal_connect (filesystem->priv->tree,
                          "test-expand-row",
                          G_CALLBACK (ario_filesystem_filetree_row_expanded_cb),
                          filesystem);
        g_signal_connect (filesystem->priv->tree,
                          "cursor-changed",
                          G_CALLBACK (ario_filesystem_cursor_moved_cb),
                          filesystem);

        /* Create hpaned */
        filesystem->priv->paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_paned_pack1 (GTK_PANED (filesystem->priv->paned), scrolledwindow_filesystem, FALSE, FALSE);

        pos = ario_conf_get_integer (PREF_FILSYSTEM_HPANED_SIZE, PREF_FILSYSTEM_HPANED_SIZE_DEFAULT);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (filesystem->priv->paned),
                                        pos);
        gtk_box_pack_start (GTK_BOX (filesystem), filesystem->priv->paned, TRUE, TRUE, 0);
}

void
ario_filesystem_shutdown (ArioSource *source)
{
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (source);
        int pos;
        guint i;

        /* Save hpaned position */
        pos = gtk_paned_get_position (GTK_PANED (filesystem->priv->paned));
        if (pos > 0)
                ario_conf_set_integer (PREF_FILSYSTEM_HPANED_SIZE,
                                       pos);

        /* Unregister actions */
        for (i = 0; i < ario_filesystem_n_actions; ++i) {
                g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                                            ario_filesystem_actions[i].name);
        }
        for (i = 0; i < ario_filesystem_songs_n_actions; ++i) {
                g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                                            ario_filesystem_songs_actions[i].name);
        }
}

GtkWidget *
ario_filesystem_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioFilesystem *filesystem;
        GtkBuilder *builder;
        GMenuModel *menu;
        gchar *file;
        ArioServer *server = ario_server_get_instance ();

        filesystem = g_object_new (TYPE_ARIO_FILESYSTEM,
                                   NULL);

        g_return_val_if_fail (filesystem->priv != NULL, NULL);

        /* Signals to synchronize the filesystem with server */
        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_filesystem_state_changed_cb),
                                 filesystem, 0);
        g_signal_connect_object (server,
                                 "updatingdb_changed",
                                 G_CALLBACK (ario_filesystem_filesystem_changed_cb),
                                 filesystem, 0);

        /* Create songs list */
        file = ario_plugin_find_file ("ario-filesystem-menu.ui");
        g_return_val_if_fail (file != NULL, NULL);

        filesystem->priv->songs = ario_songlist_new (file,
                                                     "songs-menu",
                                                     FALSE);
        gtk_paned_pack2 (GTK_PANED (filesystem->priv->paned), filesystem->priv->songs, TRUE, FALSE);

        /* Create menu */
        builder = gtk_builder_new_from_file (file);
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
        filesystem->priv->menu = gtk_menu_new_from_model (menu);
        gtk_menu_attach_to_widget  (GTK_MENU (filesystem->priv->menu),
                                    GTK_WIDGET (filesystem),
                                    NULL);
        g_free (file);

        /* Register actions */
        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_filesystem_actions,
                                         ario_filesystem_n_actions, filesystem);
        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_filesystem_songs_actions,
                                         ario_filesystem_songs_n_actions, filesystem->priv->songs);

        filesystem->priv->connected = ario_server_is_connected ();

        return GTK_WIDGET (filesystem);
}

static void
ario_filesystem_fill_filesystem (ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter, fake_child;

        /* Empty folder tree */
        gtk_tree_store_clear (filesystem->priv->model);

        /* Append Root folder */
        gtk_tree_store_append (filesystem->priv->model, &iter, NULL);
        gtk_tree_store_set (filesystem->priv->model, &iter,
                            FILETREE_ICON_COLUMN, "drive-harddisk",
                            FILETREE_ICONSIZE_COLUMN, 1,
                            FILETREE_NAME_COLUMN, _("Music"),
                            FILETREE_DIR_COLUMN, ROOT, -1);

        /* Append fake child to allow expand */
        gtk_tree_store_append(GTK_TREE_STORE (filesystem->priv->model), &fake_child, &iter);

        /* Select first row */
        gtk_tree_selection_unselect_all (filesystem->priv->selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filesystem->priv->model), &iter)) {
                gtk_tree_selection_select_iter (filesystem->priv->selection, &iter);
                ario_filesystem_cursor_moved_cb (GTK_TREE_VIEW (filesystem->priv->tree), filesystem);
        }
        filesystem->priv->empty = FALSE;
}

static void
ario_filesystem_state_changed_cb (ArioServer *server,
                                  ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        if (filesystem->priv->connected != ario_server_is_connected ()) {
                filesystem->priv->connected = ario_server_is_connected ();
                /* Fill folder tree */
                ario_filesystem_fill_filesystem (filesystem);
        }
}

static void
ario_filesystem_filesystem_changed_cb (ArioServer *server,
                                       ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill folder tree */
        ario_filesystem_fill_filesystem (filesystem);
}

static gboolean
ario_filesystem_filetree_row_expanded_cb (GtkTreeView *tree_view,
                                          GtkTreeIter *iter,
                                          GtkTreePath *path,
                                          ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        ario_filesystem_cursor_moved_cb (tree_view,
                                         filesystem);

        return FALSE;
}

static void
ario_filesystem_filetree_row_activated_cb (GtkTreeView *tree_view,
                                           GtkTreePath *path,
                                           GtkTreeViewColumn *column,
                                           ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;

        if (!gtk_tree_view_row_expanded (tree_view, path)) {
                /* Expand row */
                gtk_tree_view_expand_row (tree_view, path, FALSE);
        } else {
                /* Collapse row */
                gtk_tree_view_collapse_row (tree_view, path);
        }
}

static void
ario_filesystem_cursor_moved_cb (GtkTreeView *tree_view,
                                 ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter, child, fake_child, song_iter;
        GtkTreeModel *model = GTK_TREE_MODEL (filesystem->priv->model);
        ArioSonglist *songlist = ARIO_SONGLIST (filesystem->priv->songs);
        GtkListStore *liststore = ario_songlist_get_liststore (songlist);
        GtkTreeSelection *selection = ario_songlist_get_selection (songlist);
        gchar *dir;
        gchar *title;
        GSList *tmp;
        ArioServerFileList *files;
        ArioServerSong *song;
        gchar *path, *display_path;
        GtkTreePath *treepath;
        gboolean was_expanded;

        /* Do nothing if no folder is selected */
        if (!gtk_tree_selection_get_selected (filesystem->priv->selection,
                                              &model,
                                              &iter))
                return;

        /* Get treepath of selected folder */
        treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (filesystem->priv->model), &iter);

        /* Remember if row was expanded or not */
        was_expanded = gtk_tree_view_row_expanded (tree_view, treepath);

        /* Remove all childs */
        if (gtk_tree_model_iter_children (GTK_TREE_MODEL (filesystem->priv->model),
                                          &child,
                                          &iter)) {
                while (gtk_tree_store_remove (GTK_TREE_STORE (filesystem->priv->model), &child)) { }
        }

        /* Empty songs list */
        gtk_list_store_clear (liststore);

        /* Get path of selected dir */
        gtk_tree_model_get (GTK_TREE_MODEL (filesystem->priv->model), &iter, FILETREE_DIR_COLUMN, &dir, -1);
        g_return_if_fail (dir);

        /* Get files/directories in path */
        files = ario_server_list_files (dir, FALSE);

        /* For each directory */
        for (tmp = files->directories; tmp; tmp = g_slist_next (tmp)) {
                path = tmp->data;
                /* Append directory to folder tree */
                gtk_tree_store_append (filesystem->priv->model, &child, &iter);
                if (!strcmp (dir, ROOT)) {
                        display_path = path;
                } else {
                        /* Do no display parent hierarchy in tree path */
                        display_path = path + strlen (dir) + 1;
                }

                /* Set tree values */
                gtk_tree_store_set (filesystem->priv->model, &child,
                                    FILETREE_ICON_COLUMN, "folder",
                                    FILETREE_ICONSIZE_COLUMN, 1,
                                    FILETREE_NAME_COLUMN, display_path,
                                    FILETREE_DIR_COLUMN, path, -1);

                /* Append fake child to allow expand */
                gtk_tree_store_append(GTK_TREE_STORE (filesystem->priv->model), &fake_child, &child);
        }

        /* For each file */
        for (tmp = files->songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;

                /* Append song to songs list */
                gtk_list_store_append (liststore, &song_iter);

                title = ario_util_format_title (song);
                gtk_list_store_set (liststore, &song_iter,
                                    SONGS_TITLE_COLUMN, title,
                                    SONGS_ARTIST_COLUMN, song->artist,
                                    SONGS_ALBUM_COLUMN, song->album,
                                    SONGS_FILENAME_COLUMN, song->file,
                                    -1);
        }
        ario_server_free_file_list (files);
        g_free (dir);

        /* Select first song */
        gtk_tree_selection_unselect_all (selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (liststore), &song_iter))
                gtk_tree_selection_select_iter (selection, &song_iter);

        /* Re-expand row if needed */
        if (was_expanded)
                gtk_tree_view_expand_row (tree_view, treepath, FALSE);
        gtk_tree_path_free (treepath);
}

static void
ario_filesystem_add_filetree (ArioFilesystem *filesystem,
                              PlaylistAction action)
{
        ARIO_LOG_FUNCTION_START;
        gchar *dir;
        GtkTreeIter iter;
        GtkTreeModel *model = GTK_TREE_MODEL (filesystem->priv->model);

        /* Do nothing if no folder is selected */
        if (!gtk_tree_selection_get_selected (filesystem->priv->selection,
                                              &model,
                                              &iter))
                return;

        /* Get folder path */
        gtk_tree_model_get (GTK_TREE_MODEL (filesystem->priv->model), &iter,
                            FILETREE_DIR_COLUMN, &dir, -1);

        g_return_if_fail (dir);

        /* Append songs in folder to playlist */
        ario_server_playlist_append_dir (dir, action);
        g_free (dir);
}

static void
ario_filesystem_cmd_add_filesystem (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (data);
        /* Append songs in selected folder to playlist */
        ario_filesystem_add_filetree (filesystem, PLAYLIST_ADD);
}

static void
ario_filesystem_cmd_add_play_filesystem (GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (data);
        /* Append songs in selected folder to playlist */
        ario_filesystem_add_filetree (filesystem, PLAYLIST_ADD_PLAY);
}

static void
ario_filesystem_cmd_clear_add_play_filesystem (GSimpleAction *action,
                                               GVariant *parameter,
                                               gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (data);
        /* Clear playlist, append songs in selected folder to playlist */
        ario_filesystem_add_filetree (filesystem, PLAYLIST_REPLACE);
}

static void
ario_filesystem_popup_menu_cb (ArioDndTree* tree,
                               ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START;
        /* Show popup menu */
        gtk_menu_popup_at_pointer (GTK_MENU (filesystem->priv->menu), NULL);
}

static void
ario_filesystem_filetree_drag_data_get_cb (GtkWidget * widget,
                                           GdkDragContext * context,
                                           GtkSelectionData * selection_data,
                                           guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioFilesystem *filesystem;
        GtkTreeIter iter;
        GtkTreeModel *model;
        guchar* dir;

        filesystem = ARIO_FILESYSTEM (data);

        g_return_if_fail (IS_ARIO_FILESYSTEM (filesystem));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        model = GTK_TREE_MODEL (filesystem->priv->model);

        /* Do nothing if no folder is selected */
        if (!gtk_tree_selection_get_selected (filesystem->priv->selection,
                                              &model,
                                              &iter))
                return;

        /* Get folder path */
        gtk_tree_model_get (GTK_TREE_MODEL (filesystem->priv->model), &iter,
                            FILETREE_DIR_COLUMN, &dir, -1);

        /* Set drag data */
        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, dir,
                                strlen ((const gchar*) dir) * sizeof(guchar));
        g_free (dir);
}
