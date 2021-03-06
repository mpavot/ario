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

#include "sources/ario-storedplaylists.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "servers/ario-server.h"
#include "shell/ario-shell-songinfos.h"
#include "widgets/ario-dnd-tree.h"
#include "widgets/ario-playlist.h"
#include "widgets/ario-songlist.h"

#ifdef ENABLE_STOREDPLAYLISTS

static void ario_storedplaylists_shutdown (ArioSource *source);
static void ario_storedplaylists_connectivity_changed_cb (ArioServer *server,
                                                          ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_storedplaylists_changed_cb (ArioServer *server,
                                                             ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_add_storedplaylists (GSimpleAction *action,
                                                          GVariant *parameter,
                                                          gpointer data);
static void ario_storedplaylists_cmd_add_play_storedplaylists (GSimpleAction *action,
                                                               GVariant *parameter,
                                                               gpointer data);
static void ario_storedplaylists_cmd_clear_add_play_storedplaylists (GSimpleAction *action,
                                                                     GVariant *parameter,
                                                                     gpointer data);
static void ario_storedplaylists_cmd_delete_storedplaylists (GSimpleAction *action,
                                                             GVariant *parameter,
                                                             gpointer data);
static void ario_storedplaylists_popup_menu_cb (ArioDndTree* tree,
                                                ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_playlists_activate_cb (ArioDndTree* tree,
                                                        ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_playlists_drag_data_get_cb (GtkWidget * widget,
                                                             GdkDragContext * context,
                                                             GtkSelectionData * selection_data,
                                                             guint info, guint time, gpointer data);
static void ario_storedplaylists_playlists_selection_changed_cb (GtkTreeSelection *selection,
                                                                 ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_fill_storedplaylists (ArioStoredplaylists *storedplaylists);

struct ArioStoredplaylistsPrivate
{
        GtkListStore *model;
        GtkTreeSelection *selection;

        GtkWidget *songs;
        GtkWidget *paned;

        gboolean connected;
        gboolean empty;

        GtkWidget *popup;
};


/* Actions on playlists list */
static const GActionEntry ario_storedplaylists_actions [] = {
        { "storedpl-add-to-pl", ario_storedplaylists_cmd_add_storedplaylists },
        { "storedpl-add-play", ario_storedplaylists_cmd_add_play_storedplaylists },
        { "storedpl-clear-add-play", ario_storedplaylists_cmd_clear_add_play_storedplaylists },
        { "storedpl-delete", ario_storedplaylists_cmd_delete_storedplaylists },
};
static guint ario_storedplaylists_n_actions = G_N_ELEMENTS (ario_storedplaylists_actions);

static const GActionEntry ario_storedplaylists_songs_actions [] = {
        { "storedpl-add-to-pl-songs", ario_songlist_cmd_add_songlists },
        { "storedpl-add-play-songs", ario_songlist_cmd_add_play_songlists },
        { "storedpl-clear-add-play-songs", ario_songlist_cmd_clear_add_play_songlists },
        { "storedpl-songs-properties", ario_songlist_cmd_songs_properties },
};
static guint ario_storedplaylists_n_songs_actions = G_N_ELEMENTS (ario_storedplaylists_songs_actions);

/* Object properties */
enum
{
        PROP_0,
};

enum
{
        PLAYLISTS_NAME_COLUMN,
        PLAYLISTS_N_COLUMN
};

/* Drag and drop targets */
static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

G_DEFINE_TYPE_WITH_CODE (ArioStoredplaylists, ario_storedplaylists, ARIO_TYPE_SOURCE, G_ADD_PRIVATE(ArioStoredplaylists))

static gchar *
ario_storedplaylists_get_id (ArioSource *source)
{
        return "storedplaylists";
}

static gchar *
ario_storedplaylists_get_name (ArioSource *source)
{
        return _("Playlists");
}

static gchar *
ario_storedplaylists_get_icon (ArioSource *source)
{
        return "multimedia-player";
}

static void
ario_storedplaylists_select (ArioSource *source)
{
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (source);

        /* Fill playlists list if it's empty */
        if (storedplaylists->priv->empty)
                ario_storedplaylists_fill_storedplaylists (storedplaylists);
}

static void
ario_storedplaylists_class_init (ArioStoredplaylistsClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* ArioSource virtual methods */
        source_class->get_id = ario_storedplaylists_get_id;
        source_class->get_name = ario_storedplaylists_get_name;
        source_class->get_icon = ario_storedplaylists_get_icon;
        source_class->shutdown = ario_storedplaylists_shutdown;
        source_class->select = ario_storedplaylists_select;
}

static void
ario_storedplaylists_init (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GtkWidget *scrolledwindow_storedplaylists;
        GtkWidget *tree;
        GtkBuilder *builder;
        GMenuModel *menu;
        int pos;

        storedplaylists->priv = ario_storedplaylists_get_instance_private (storedplaylists);

        storedplaylists->priv->connected = FALSE;
        storedplaylists->priv->empty = TRUE;

        /* Create scrolled window for playlists list */
        scrolledwindow_storedplaylists = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_storedplaylists);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_storedplaylists), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_storedplaylists), GTK_SHADOW_IN);

        /* Drag and drop tree for playlists list */
        tree = ario_dnd_tree_new (songs_targets,
                                  G_N_ELEMENTS (songs_targets),
                                  FALSE);

        /* Add column for playlists list */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Playlist"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 50);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

        /* Create model for playlists list */
        storedplaylists->priv->model = gtk_list_store_new (PLAYLISTS_N_COLUMN,
                                                           G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (storedplaylists->priv->model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                                 GTK_TREE_MODEL (storedplaylists->priv->model));

        /* Get selection for playlists list */
        storedplaylists->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
        gtk_tree_selection_set_mode (storedplaylists->priv->selection,
                                     GTK_SELECTION_MULTIPLE);

        gtk_container_add (GTK_CONTAINER (scrolledwindow_storedplaylists), tree);

        /* Signals for actions on playlists list */
        g_signal_connect (GTK_TREE_VIEW (tree),
                          "drag_data_get",
                          G_CALLBACK (ario_storedplaylists_playlists_drag_data_get_cb), storedplaylists);
        g_signal_connect (GTK_TREE_VIEW (tree),
                          "popup",
                          G_CALLBACK (ario_storedplaylists_popup_menu_cb), storedplaylists);
        g_signal_connect (GTK_TREE_VIEW (tree),
                          "activate",
                          G_CALLBACK (ario_storedplaylists_playlists_activate_cb), storedplaylists);
        g_signal_connect (storedplaylists->priv->selection,
                          "changed",
                          G_CALLBACK (ario_storedplaylists_playlists_selection_changed_cb),
                          storedplaylists);

        /* Hpaned properties */
        storedplaylists->priv->paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_paned_pack1 (GTK_PANED (storedplaylists->priv->paned), scrolledwindow_storedplaylists, FALSE, FALSE);

        pos = ario_conf_get_integer (PREF_PLAYLISTS_HPANED_SIZE, PREF_PLAYLISTS_HPANED_SIZE_DEFAULT);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (storedplaylists->priv->paned),
                                        pos);

        gtk_box_pack_start (GTK_BOX (storedplaylists), storedplaylists->priv->paned, TRUE, TRUE, 0);

        /* Popup menu */
        builder = gtk_builder_new_from_file (UI_PATH "ario-songlist-menu.ui");
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "storedpl-menu"));
        storedplaylists->priv->popup = gtk_menu_new_from_model (menu);
        gtk_menu_attach_to_widget  (GTK_MENU (storedplaylists->priv->popup),
                                    GTK_WIDGET (storedplaylists),
                                    NULL);
        g_object_unref (builder);
}

void
ario_storedplaylists_shutdown (ArioSource *source)
{
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (source);
        int pos;

        /* Save hpaned position */
        pos = gtk_paned_get_position (GTK_PANED (storedplaylists->priv->paned));
        if (pos > 0)
                ario_conf_set_integer (PREF_PLAYLISTS_HPANED_SIZE,
                                       pos);
}

GtkWidget *
ario_storedplaylists_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioStoredplaylists *storedplaylists;
        ArioServer *server = ario_server_get_instance ();

        storedplaylists = g_object_new (TYPE_ARIO_STOREDPLAYLISTS,
                                        NULL);

        g_return_val_if_fail (storedplaylists->priv != NULL, NULL);

        /* Signals to synchronize the storedplaylists with server */
        g_signal_connect_object (server,
                                 "connectivity_changed", G_CALLBACK (ario_storedplaylists_connectivity_changed_cb),
                                 storedplaylists, 0);
        g_signal_connect_object (server,
                                 "storedplaylists_changed", G_CALLBACK (ario_storedplaylists_storedplaylists_changed_cb),
                                 storedplaylists, 0);

        /* Create songs list */
        storedplaylists->priv->songs = ario_songlist_new (UI_PATH "ario-songlist-menu.ui",
                                                          "storedpl-songs-menu",
                                                          FALSE);
        gtk_paned_pack2 (GTK_PANED (storedplaylists->priv->paned), storedplaylists->priv->songs, TRUE, FALSE);

        /* Register actions */
        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_storedplaylists_actions,
                                         ario_storedplaylists_n_actions,
                                         storedplaylists);
        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_storedplaylists_songs_actions,
                                         ario_storedplaylists_n_songs_actions,
                                         storedplaylists->priv->songs);

        return GTK_WIDGET (storedplaylists);
}

static void
ario_storedplaylists_fill_storedplaylists (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter storedplaylists_iter;
        GSList *playlists;
        GSList *tmp;

        storedplaylists->priv->empty = FALSE;

        /* Empty playlists list */
        gtk_list_store_clear (storedplaylists->priv->model);

        if (!storedplaylists->priv->connected)
                return;

        /* Get playlists list on server */
        playlists = ario_server_get_playlists ();
        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                /* Add playlist to list */
                gtk_list_store_append (storedplaylists->priv->model, &storedplaylists_iter);
                gtk_list_store_set (storedplaylists->priv->model, &storedplaylists_iter, 0,
                                    tmp->data, -1);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);

        /* Select first playlist */
        gtk_tree_selection_unselect_all (storedplaylists->priv->selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (storedplaylists->priv->model), &storedplaylists_iter))
                gtk_tree_selection_select_iter (storedplaylists->priv->selection, &storedplaylists_iter);
}

static void
ario_storedplaylists_playlists_selection_foreach (GtkTreeModel *model,
                                                  GtkTreePath *path,
                                                  GtkTreeIter *iter,
                                                  gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (userdata);
        gchar* playlist = NULL;
        GSList *songs = NULL, *temp;
        ArioServerSong *song;
        GtkTreeIter song_iter;
        gchar *title;
        GtkListStore *liststore;

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &playlist, -1);

        if (!playlist)
                return;

        /* Get list of songs of selected playlist */
        songs = ario_server_get_songs_from_playlist (playlist);
        g_free (playlist);

        liststore = ario_songlist_get_liststore (ARIO_SONGLIST (storedplaylists->priv->songs));
        for (temp = songs; temp; temp = g_slist_next (temp)) {
                /* Append each song to the list */
                song = temp->data;
                gtk_list_store_append (liststore, &song_iter);

                title = ario_util_format_title (song);
                gtk_list_store_set (liststore, &song_iter,
                                    SONGS_TITLE_COLUMN, title,
                                    SONGS_ARTIST_COLUMN, song->artist,
                                    SONGS_ALBUM_COLUMN, song->album,
                                    SONGS_FILENAME_COLUMN, song->file,
                                    -1);
        }

        g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
        g_slist_free (songs);
}

static void
ario_storedplaylists_playlists_selection_update (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter song_iter;
        ArioSonglist *songlist = ARIO_SONGLIST (storedplaylists->priv->songs);
        GtkListStore *liststore = ario_songlist_get_liststore (songlist);
        GtkTreeSelection *selection = ario_songlist_get_selection (songlist);

        /* Empty songs list */
        gtk_list_store_clear (liststore);

        /* Add songs of each selected playlist */
        gtk_tree_selection_selected_foreach (storedplaylists->priv->selection,
                                             ario_storedplaylists_playlists_selection_foreach,
                                             storedplaylists);

        /* Select first song */
        gtk_tree_selection_unselect_all (selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (liststore), &song_iter))
                gtk_tree_selection_select_iter (selection, &song_iter);
}

static void
ario_storedplaylists_playlists_selection_changed_cb (GtkTreeSelection *selection,
                                                     ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        ario_storedplaylists_playlists_selection_update (storedplaylists);
}

static void
ario_storedplaylists_connectivity_changed_cb (ArioServer *server,
                                              ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        storedplaylists->priv->connected = ario_server_is_connected ();

        /* Fill playlists list */
        if (!storedplaylists->priv->empty)
                ario_storedplaylists_fill_storedplaylists (storedplaylists);
}

static void
ario_storedplaylists_storedplaylists_changed_cb (ArioServer *server,
                                                 ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill playlists list */
        ario_storedplaylists_fill_storedplaylists (storedplaylists);
}

static void
storedplaylists_foreach (GtkTreeModel *model,
                         GtkTreePath *path,
                         GtkTreeIter *iter,
                         gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **storedplaylists = (GSList **) userdata;
        gchar *val = NULL;

        /* Append playlist name to the list */
        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &val, -1);
        *storedplaylists = g_slist_append (*storedplaylists, val);
}

static void
ario_storedplaylists_add_playlists (ArioStoredplaylists *storedplaylists,
                                    PlaylistAction action)
{
        ARIO_LOG_FUNCTION_START;
        GSList *playlists = NULL;
        GSList *songs = NULL;
        GSList *tmp;

        /* Get a list of playlists names */
        gtk_tree_selection_selected_foreach (storedplaylists->priv->selection,
                                             storedplaylists_foreach,
                                             &playlists);

        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                /* Get songs from playlist */
                songs = ario_server_get_songs_from_playlist (tmp->data);

                /* Append songs to main playlist */
                ario_server_playlist_append_server_songs (songs, action);

                g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (songs);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);
}

static void
ario_storedplaylists_clear_add_play_playlists (ArioStoredplaylists *storedplaylists)
{
        /* Empty playlist, add songs to playlist and play */
        ario_storedplaylists_add_playlists (storedplaylists, PLAYLIST_REPLACE);
}

static void
ario_storedplaylists_cmd_add_storedplaylists (GSimpleAction *action,
                                              GVariant *parameter,
                                              gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (data);
        /* Add songs to playlist */
        ario_storedplaylists_add_playlists (storedplaylists, PLAYLIST_ADD);
}

static void
ario_storedplaylists_cmd_add_play_storedplaylists (GSimpleAction *action,
                                                   GVariant *parameter,
                                                   gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (data);
        /* Add songs to playlist and play */
        ario_storedplaylists_add_playlists (storedplaylists, PLAYLIST_ADD_PLAY);
}

static void
ario_storedplaylists_cmd_clear_add_play_storedplaylists (GSimpleAction *action,
                                                         GVariant *parameter,
                                                         gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (data);
        ario_storedplaylists_clear_add_play_playlists (storedplaylists);
}

static void
ario_storedplaylists_cmd_delete_storedplaylists (GSimpleAction *action,
                                                 GVariant *parameter,
                                                 gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;
        GSList *playlists = NULL;
        GSList *tmp;
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (data);

        /* Create confirmation dialog for playlist deletion */
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure that you want to delete all the selected playlists?"));

        /* Run dialog */
        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        /* Get list of selected playlists */
        gtk_tree_selection_selected_foreach (storedplaylists->priv->selection,
                                             storedplaylists_foreach,
                                             &playlists);

        /* Delete each selected playlist */
        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                ario_server_delete_playlist (tmp->data);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);
}

static void
ario_storedplaylists_popup_menu_cb (ArioDndTree* tree,
                                    ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;

        if (gtk_tree_selection_count_selected_rows (storedplaylists->priv->selection) > 0) {
                /* Show popup menu */
                gtk_menu_popup_at_pointer (GTK_MENU (storedplaylists->priv->popup), NULL);
        }
}

static void
ario_storedplaylists_playlists_activate_cb (ArioDndTree* tree,
                                            ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START;
        ario_storedplaylists_add_playlists (storedplaylists,
                                            ario_conf_get_integer (PREF_DOUBLECLICK_BEHAVIOR,
                                                                   PREF_DOUBLECLICK_BEHAVIOR_DEFAULT));
}

static void
ario_storedplaylists_playlists_drag_data_get_cb (GtkWidget * widget,
                                                 GdkDragContext * context,
                                                 GtkSelectionData * selection_data,
                                                 guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioStoredplaylists *storedplaylists;
        ArioServerSong *song;
        GSList *playlists = NULL;
        GSList *songs;
        GSList *tmp, *tmp2;
        GString *str_playlists;

        storedplaylists = ARIO_STOREDPLAYLISTS (data);

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        /* Get list of selected playlists */
        gtk_tree_selection_selected_foreach (storedplaylists->priv->selection,
                                             storedplaylists_foreach,
                                             &playlists);

        /* Get string of playlist names concatenation */
        str_playlists = g_string_new("");
        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                songs = ario_server_get_songs_from_playlist (tmp->data);

                for (tmp2 = songs; tmp2; tmp2 = g_slist_next (tmp2)) {
                        song = tmp2->data;
                        g_string_append (str_playlists, song->file);
                        g_string_append (str_playlists, "\n");
                }

                g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (songs);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);

        /* Set drag data */
        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (const guchar *) str_playlists->str,
                                strlen (str_playlists->str) * sizeof(guchar));

        g_string_free (str_playlists, TRUE);
}

#endif  /* ENABLE_STOREDPLAYLISTS */
