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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "widgets/ario-playlist.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "preferences/ario-preferences.h"
#include "shell/ario-shell-songinfos.h"

#define DRAG_THRESHOLD 1

typedef struct ArioPlaylistColumn ArioPlaylistColumn;

static void ario_playlist_finalize (GObject *object);
static void ario_playlist_set_property (GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
static void ario_playlist_get_property (GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec);
static void ario_playlist_changed_cb (ArioMpd *mpd,
                                      ArioPlaylist *playlist);
static void ario_playlist_song_changed_cb (ArioMpd *mpd,
                                           ArioPlaylist *playlist);
static void ario_playlist_state_changed_cb (ArioMpd *mpd,
                                            ArioPlaylist *playlist);
static void ario_playlist_sort_changed_cb (GtkTreeSortable *treesortable,
                                           ArioPlaylist *playlist);
static void ario_playlist_rows_reordered_cb (GtkTreeModel *tree_model,
                                             GtkTreePath *path,
                                             GtkTreeIter *iter,
                                             gpointer arg3,
                                             ArioPlaylist *playlist);
static void ario_playlist_drag_leave_cb (GtkWidget *widget,
                                         GdkDragContext *context,
                                         gint x, gint y,
                                         GtkSelectionData *data,
                                         guint info,
                                         guint time,
                                         gpointer user_data);
static void ario_playlist_drag_data_get_cb (GtkWidget * widget,
                                            GdkDragContext * context,
                                            GtkSelectionData * selection_data,
                                            guint info, guint time, gpointer data);
static void ario_playlist_cmd_clear (GtkAction *action,
                                     ArioPlaylist *playlist);
static void ario_playlist_cmd_remove (GtkAction *action,
                                      ArioPlaylist *playlist);
static void ario_playlist_cmd_songs_properties (GtkAction *action,
                                                ArioPlaylist *playlist);
static void ario_playlist_cmd_goto_playing_song (GtkAction *action,
                                                 ArioPlaylist *playlist);
static void ario_playlist_cmd_save (GtkAction *action,
                                    ArioPlaylist *playlist);
static gboolean ario_playlist_view_button_press_cb (GtkWidget *widget,
                                                    GdkEventButton *event,
                                                    ArioPlaylist *playlist);
static gboolean ario_playlist_view_key_press_cb (GtkWidget *widget,
                                                 GdkEventKey *event,
                                                 ArioPlaylist *playlist);
static gboolean ario_playlist_view_button_release_cb (GtkWidget *widget,
                                                      GdkEventButton *event,
                                                      ArioPlaylist *playlist);
static gboolean ario_playlist_view_motion_notify (GtkWidget *widget,
                                                  GdkEventMotion *event,
                                                  ArioPlaylist *playlist);
static void ario_playlist_activate_row (GtkTreePath *path);
static void ario_playlist_column_visible_changed_cb (guint notification_id,
                                                     ArioPlaylistColumn *ario_column);

static ArioPlaylist *instance = NULL;

struct ArioPlaylistPrivate
{
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;

        int playlist_id;
        int playlist_length;

        GdkPixbuf *play_pixbuf;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        GtkUIManager *ui_manager;
};

static GtkActionEntry ario_playlist_actions [] =
{
        { "PlaylistClear", GTK_STOCK_CLEAR, N_("_Clear"), NULL,
                NULL,
                G_CALLBACK (ario_playlist_cmd_clear) },
        { "PlaylistRemove", GTK_STOCK_REMOVE, N_("_Remove"), NULL,
                NULL,
                G_CALLBACK (ario_playlist_cmd_remove) },
        { "PlaylistSave", GTK_STOCK_SAVE, N_("_Save"), "<control>S",
                NULL,
                G_CALLBACK (ario_playlist_cmd_save) },
        { "PlaylistSongProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_playlist_cmd_songs_properties) },
        { "PlaylistGotoPlaying", GTK_STOCK_JUMP_TO, N_("_Go to playing song"), "<control>L",
                NULL,
                G_CALLBACK (ario_playlist_cmd_goto_playing_song) }
};

static guint ario_playlist_n_actions = G_N_ELEMENTS (ario_playlist_actions);

enum
{
        PROP_0,
        PROP_UI_MANAGER
};

enum
{
        PIXBUF_COLUMN,
        TRACK_COLUMN,
        TITLE_COLUMN,
        ARTIST_COLUMN,
        ALBUM_COLUMN,
        DURATION_COLUMN,
        FILE_COLUMN,
        GENRE_COLUMN,
        DATE_COLUMN,
        ID_COLUMN,
        N_COLUMN
};

struct ArioPlaylistColumn {
        const int columnnb;
        const gchar *pref_size;
        const int default_size;

        const gchar *pref_order;
        const int default_order;

        const gchar *pref_is_visible;
        const gboolean default_is_visible;

        const gboolean is_pixbuf;
        const gboolean is_resizable;
        const gboolean is_sortable;
        GtkTreeViewColumn *column;
};

static ArioPlaylistColumn all_columns []  = {
        { PIXBUF_COLUMN, NULL, 20, PREF_PIXBUF_COLUMN_ORDER, PREF_PIXBUF_COLUMN_ORDER_DEFAULT, NULL, TRUE, TRUE, FALSE, FALSE },
        { TRACK_COLUMN, PREF_TRACK_COLUMN_SIZE, PREF_TRACK_COLUMN_SIZE_DEFAULT, PREF_TRACK_COLUMN_ORDER, PREF_TRACK_COLUMN_ORDER_DEFAULT, PREF_TRACK_COLUMN_VISIBLE, PREF_TRACK_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { TITLE_COLUMN, PREF_TITLE_COLUMN_SIZE, PREF_TITLE_COLUMN_SIZE_DEFAULT, PREF_TITLE_COLUMN_ORDER, PREF_TITLE_COLUMN_ORDER_DEFAULT, PREF_TITLE_COLUMN_VISIBLE, PREF_TITLE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { ARTIST_COLUMN, PREF_ARTIST_COLUMN_SIZE, PREF_ARTIST_COLUMN_SIZE_DEFAULT, PREF_ARTIST_COLUMN_ORDER, PREF_ARTIST_COLUMN_ORDER_DEFAULT, PREF_ARTIST_COLUMN_VISIBLE, PREF_ARTIST_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { ALBUM_COLUMN, PREF_ALBUM_COLUMN_SIZE, PREF_ALBUM_COLUMN_SIZE_DEFAULT, PREF_ALBUM_COLUMN_ORDER, PREF_ALBUM_COLUMN_ORDER_DEFAULT, PREF_ALBUM_COLUMN_VISIBLE, PREF_ALBUM_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { DURATION_COLUMN, PREF_DURATION_COLUMN_SIZE, PREF_DURATION_COLUMN_SIZE_DEFAULT, PREF_DURATION_COLUMN_ORDER, PREF_DURATION_COLUMN_ORDER_DEFAULT, PREF_DURATION_COLUMN_VISIBLE, PREF_DURATION_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { FILE_COLUMN, PREF_FILE_COLUMN_SIZE, PREF_FILE_COLUMN_SIZE_DEFAULT, PREF_FILE_COLUMN_ORDER, PREF_FILE_COLUMN_ORDER_DEFAULT, PREF_FILE_COLUMN_VISIBLE, PREF_FILE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { GENRE_COLUMN, PREF_GENRE_COLUMN_SIZE, PREF_GENRE_COLUMN_SIZE_DEFAULT, PREF_GENRE_COLUMN_ORDER, PREF_GENRE_COLUMN_ORDER_DEFAULT, PREF_GENRE_COLUMN_VISIBLE, PREF_GENRE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { DATE_COLUMN, PREF_DATE_COLUMN_SIZE, PREF_DATE_COLUMN_SIZE_DEFAULT, PREF_DATE_COLUMN_ORDER, PREF_DATE_COLUMN_ORDER_DEFAULT, PREF_DATE_COLUMN_VISIBLE, PREF_DATE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE },
        { -1 }
};

static const GtkTargetEntry targets  [] = {
        { "text/internal-list", 0, 10},
        { "text/songs-list", 0, 20 },
        { "text/radios-list", 0, 30 },
        { "text/directory", 0, 40 },
        { "text/criterias-list", 0, 50 },
};

static const GtkTargetEntry internal_targets  [] = {
        { "text/internal-list", 0, 10 },
};

#define ARIO_PLAYLIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PLAYLIST, ArioPlaylistPrivate))
G_DEFINE_TYPE (ArioPlaylist, ario_playlist, GTK_TYPE_SCROLLED_WINDOW)

static void
ario_playlist_class_init (ArioPlaylistClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_playlist_finalize;

        object_class->set_property = ario_playlist_set_property;
        object_class->get_property = ario_playlist_get_property;

        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (ArioPlaylistPrivate));
}

static void
ario_playlist_append_column (ArioPlaylistColumn *ario_column,
                             const gchar *column_name)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        if (ario_column->is_pixbuf) {
                renderer = gtk_cell_renderer_pixbuf_new ();
                column = gtk_tree_view_column_new_with_attributes (column_name,
                                                                   renderer,
                                                                   "pixbuf", ario_column->columnnb,
                                                                   NULL);
        } else {
                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes (column_name,
                                                                   renderer,
                                                                   "text", ario_column->columnnb,
                                                                   NULL);
        }
        gtk_tree_view_column_set_resizable (column, ario_column->is_resizable);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);

        if (ario_column->is_sortable) {
                gtk_tree_view_column_set_sort_indicator (column, TRUE);
                gtk_tree_view_column_set_sort_column_id (column, ario_column->columnnb);
        }

        if (ario_column->pref_size)
                gtk_tree_view_column_set_fixed_width (column, ario_conf_get_integer (ario_column->pref_size, ario_column->default_size));
        else
                gtk_tree_view_column_set_fixed_width (column, ario_column->default_size);
        gtk_tree_view_column_set_reorderable (column, TRUE);
        if (ario_column->pref_is_visible)
                gtk_tree_view_column_set_visible (column, ario_conf_get_integer (ario_column->pref_is_visible, ario_column->default_is_visible));
        else
                gtk_tree_view_column_set_visible (column, ario_column->default_is_visible);
        gtk_tree_view_append_column (GTK_TREE_VIEW (instance->priv->tree), column);
        ario_column->column = column;

        if (ario_column->pref_is_visible)
                ario_conf_notification_add (ario_column->pref_is_visible,
                                            (ArioNotifyFunc) ario_playlist_column_visible_changed_cb,
                                            ario_column);
}

static void
ario_playlist_reorder_columns (void)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewColumn *orders[N_COLUMN];
        GtkTreeViewColumn *current, *prev = NULL;
        int i;

        for (i = 0; i < N_COLUMN; ++i)
                orders[i] = NULL;

        for (i = 0; all_columns[i].columnnb != -1; ++i)
                orders[ario_conf_get_integer (all_columns[i].pref_order, all_columns[i].default_order)] = all_columns[i].column;

        for (i = 0; i < N_COLUMN; ++i) {
                current = orders[i];
                if (current)
                        gtk_tree_view_move_column_after (GTK_TREE_VIEW (instance->priv->tree), current, prev);
                prev = current;
        }

        /* Resize the last visible column */
        for (i = N_COLUMN - 1; i >= 0; --i) {
                if (!orders[i])
                        continue;
                if (gtk_tree_view_column_get_visible (orders[i])) {
                        gtk_tree_view_column_set_fixed_width (orders[i], ario_util_min (gtk_tree_view_column_get_fixed_width (orders[i]), 50));
                        break;
                }
        }
}

static gint
ario_playlist_no_sort (GtkTreeModel *model,
                       GtkTreeIter *a,
                       GtkTreeIter *b,
                       gpointer user_data)
{
        return 0;
}

static void
ario_playlist_init (ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        int i;
        const gchar *column_names []  = { " ", _("Track"), _("Title"), _("Artist"), _("Album"), _("Duration"), _("File"), _("Genre"), _("Date") };

        instance = playlist;
        playlist->priv = ARIO_PLAYLIST_GET_PRIVATE (playlist);
        playlist->priv->playlist_id = -1;
        playlist->priv->playlist_length = 0;
        playlist->priv->play_pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "play.png", NULL);

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (playlist), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (playlist), GTK_SHADOW_IN);
        gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (playlist), NULL);
        gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (playlist), NULL);

        playlist->priv->tree = gtk_tree_view_new ();
        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (playlist->priv->tree), TRUE);

        for (i = 0; all_columns[i].columnnb != -1; ++i)
                ario_playlist_append_column (&all_columns[i], column_names[i]);

        ario_playlist_reorder_columns ();
        /* Model */
        playlist->priv->model = gtk_list_store_new (N_COLUMN,
                                                    GDK_TYPE_PIXBUF,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_INT);

        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                 GTK_TREE_MODEL (playlist->priv->model));

        gtk_tree_view_set_reorderable (GTK_TREE_VIEW (playlist->priv->tree),
                                       TRUE);
        gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (playlist->priv->model),
                                                 ario_playlist_no_sort,
                                                 NULL, NULL);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (playlist->priv->tree),
                                      TRUE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (playlist->priv->tree),
                                         TITLE_COLUMN);

        g_signal_connect (playlist->priv->model,
                          "sort-column-changed",
                          G_CALLBACK (ario_playlist_sort_changed_cb),
                          playlist);

        playlist->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist->priv->tree));
        gtk_tree_selection_set_mode (playlist->priv->selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (playlist), playlist->priv->tree);
        gtk_drag_source_set (playlist->priv->tree,
                             GDK_BUTTON1_MASK,
                             internal_targets,
                             G_N_ELEMENTS (internal_targets),
                             GDK_ACTION_MOVE);

        gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (playlist->priv->tree),
                                              targets, G_N_ELEMENTS (targets),
                                              GDK_ACTION_MOVE);

        g_signal_connect (playlist->priv->tree,
                          "button_press_event",
                          G_CALLBACK (ario_playlist_view_button_press_cb),
                          playlist);
        g_signal_connect (playlist->priv->tree,
                          "button_release_event",
                          G_CALLBACK (ario_playlist_view_button_release_cb),
                          playlist);
        g_signal_connect (playlist->priv->tree,
                          "motion_notify_event",
                          G_CALLBACK (ario_playlist_view_motion_notify),
                          playlist);
        g_signal_connect (playlist->priv->tree,
                          "key_press_event",
                          G_CALLBACK (ario_playlist_view_key_press_cb),
                          playlist);

        g_signal_connect (playlist->priv->tree,
                          "drag_data_received",
                          G_CALLBACK (ario_playlist_drag_leave_cb),
                          playlist);

        g_signal_connect (playlist->priv->tree,
                          "drag_data_get",
                          G_CALLBACK (ario_playlist_drag_data_get_cb),
                          playlist);
}

void
ario_playlist_shutdown (void)
{
        ARIO_LOG_FUNCTION_START
        int width;
        int orders[N_COLUMN];
        GtkTreeViewColumn *column;
        int i, j = 1;

        GList *columns, *tmp;
        columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (instance->priv->tree));
        for (tmp = columns; tmp; tmp = g_list_next (tmp)) {
                column = tmp->data;
                for (i = 0; i < N_COLUMN; ++i) {
                        if (all_columns[i].column == column)
                                orders[i] = j;
                }
                ++j;
        }
        g_list_free (columns);

        for (i = 0; all_columns[i].columnnb != -1; ++i) {
                width = gtk_tree_view_column_get_width (all_columns[i].column);
                if (width > 10 && all_columns[i].pref_size)
                        ario_conf_set_integer (all_columns[i].pref_size,
                                               width);
                ario_conf_set_integer (all_columns[i].pref_order, orders[all_columns[i].columnnb]);
        }
}

static void
ario_playlist_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylist *playlist;
        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_PLAYLIST (object));

        playlist = ARIO_PLAYLIST (object);

        g_return_if_fail (playlist->priv != NULL);
        g_object_unref (playlist->priv->play_pixbuf);

        G_OBJECT_CLASS (ario_playlist_parent_class)->finalize (object);
}

static void
ario_playlist_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylist *playlist = ARIO_PLAYLIST (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                playlist->priv->ui_manager = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_playlist_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylist *playlist = ARIO_PLAYLIST (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                g_value_set_object (value, playlist->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
ario_playlist_sync_song_foreach (GtkTreeModel *model,
                                 GtkTreePath *path,
                                 GtkTreeIter *iter,
                                 int *new_song_id)
{
        ARIO_LOG_FUNCTION_START
        int song_id;

        gtk_tree_model_get (model, iter, ID_COLUMN, &song_id, -1);
        if (song_id == *new_song_id)
                gtk_list_store_set (GTK_LIST_STORE (model), iter,
                                    PIXBUF_COLUMN, instance->priv->play_pixbuf,
                                    -1);
        else
                gtk_list_store_set (GTK_LIST_STORE (model), iter,
                                    PIXBUF_COLUMN, NULL,
                                    -1);

        return FALSE;
}

static void
ario_playlist_sync_song (void)
{
        ARIO_LOG_FUNCTION_START
        int new_song_id;
        int state;

        state = ario_mpd_get_current_state ();

        if (state == MPD_STATUS_STATE_UNKNOWN || state == MPD_STATUS_STATE_STOP)
                new_song_id = -1;
        else
                new_song_id = ario_mpd_get_current_song_id ();

        gtk_tree_model_foreach (GTK_TREE_MODEL (instance->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_sync_song_foreach,
                                &new_song_id);
}

static void
ario_playlist_changed_cb (ArioMpd *mpd,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        gint old_length;
        GtkTreeIter iter;
        gchar *time, *track;
        gchar *title;
        GSList *songs, *tmp;
        ArioMpdSong *song;
        gboolean need_set;
        gboolean need_sync = FALSE;
        GtkTreePath *path;
        GdkPixbuf *pixbuf;
        int song_id;
        int state;


        if (!ario_mpd_is_connected ()) {
                playlist->priv->playlist_length = 0;
                playlist->priv->playlist_id = -1;
                gtk_list_store_clear (playlist->priv->model);
                return;
        }

        old_length = playlist->priv->playlist_length;

        songs = ario_mpd_get_playlist_changes (playlist->priv->playlist_id);

        state = ario_mpd_get_current_state ();
        if (state == MPD_STATUS_STATE_UNKNOWN || state == MPD_STATUS_STATE_STOP)
                song_id = -1;
        else
                song_id = ario_mpd_get_current_song_id ();

        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                need_set = FALSE;
                /* decide whether to update or to add */
                if (song->pos < old_length) {
                        path = gtk_tree_path_new ();
                        gtk_tree_path_append_index (path, song->pos);
                        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                                need_set = TRUE;
                        }
                        gtk_tree_path_free (path);
                } else {
                        gtk_list_store_append (playlist->priv->model, &iter);
                        need_set = TRUE;
                }
                if (need_set) {
                        time = ario_util_format_time (song->time);
                        track = ario_util_format_track (song->track);
                        title = ario_util_format_title (song);
                        if (song_id == song->id)
                                pixbuf = instance->priv->play_pixbuf;
                        else
                                pixbuf = NULL;
                        gtk_list_store_set (playlist->priv->model, &iter,
                                            PIXBUF_COLUMN, pixbuf,
                                            TRACK_COLUMN, track,
                                            TITLE_COLUMN, title,
                                            ARTIST_COLUMN, song->artist,
                                            ALBUM_COLUMN, song->album ? song->album : ARIO_MPD_UNKNOWN,
                                            DURATION_COLUMN, time,
                                            FILE_COLUMN, song->file,
                                            GENRE_COLUMN, song->genre,
                                            DATE_COLUMN, song->date,
                                            ID_COLUMN, song->id,
                                            -1);
                        g_free (title);
                        g_free (time);
                        g_free (track);
                }
        }

        g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
        g_slist_free (songs);

        playlist->priv->playlist_length = ario_mpd_get_current_playlist_length ();
        playlist->priv->playlist_id = ario_mpd_get_current_playlist_id ();

        while (playlist->priv->playlist_length < old_length) {

                path = gtk_tree_path_new ();
                gtk_tree_path_append_index (path, old_length - 1);

                if (gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist->priv->model), &iter, path))
                        gtk_list_store_remove (playlist->priv->model, &iter);

                gtk_tree_path_free (path);
                --old_length;
                need_sync = TRUE;
        }

        if (need_sync)
                ario_playlist_sync_song ();
}

static void
ario_playlist_song_changed_cb (ArioMpd *mpd,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_sync_song ();

        if (ario_conf_get_boolean (PREF_PLAYLIST_AUTOSCROLL, PREF_PLAYLIST_AUTOSCROLL_DEFAULT))
                ario_playlist_cmd_goto_playing_song (NULL, playlist);
}

static void
ario_playlist_state_changed_cb (ArioMpd *mpd,
                                ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_sync_song ();
}

static void
ario_playlist_sort_changed_cb (GtkTreeSortable *treesortable,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START

        g_signal_connect (playlist->priv->model,
                          "rows-reordered",
                          G_CALLBACK (ario_playlist_rows_reordered_cb),
                          playlist);
}

typedef struct ArioPlaylistReorderData {
        GSList *paths;
        GSList *ids;
} ArioPlaylistReorderData;

static gboolean
ario_playlist_rows_reordered_foreach (GtkTreeModel *model,
                                      GtkTreePath *p,
                                      GtkTreeIter *iter,
                                      ArioPlaylistReorderData *data)
{
        gchar *path;
        gint *id;

        id = g_malloc (sizeof (gint));
        gtk_tree_model_get (model, iter, FILE_COLUMN, &path, ID_COLUMN, id, -1);

        data->paths = g_slist_append (data->paths, path);
        data->ids = g_slist_append (data->ids, id);

        return FALSE;
}

static void
ario_playlist_rows_reordered_cb (GtkTreeModel *tree_model,
                                 GtkTreePath *path,
                                 GtkTreeIter *iter,
                                 gpointer arg3,
                                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START

        ArioPlaylistReorderData data;
        data.paths = NULL;
        data.ids = NULL;
        GSList *tmp;

        gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_rows_reordered_foreach,
                                &data);

        for (tmp = data.ids; tmp; tmp = g_slist_next (tmp)) {
                ario_mpd_queue_delete_id (*((gint *)tmp->data));
        }

        for (tmp = data.paths; tmp; tmp = g_slist_next (tmp)) {
                ario_mpd_queue_add (tmp->data);
        }
        ario_mpd_queue_commit ();

        g_slist_foreach (data.ids, (GFunc) g_free, NULL);
        g_slist_free (data.ids);

        g_slist_foreach (data.paths, (GFunc) g_free, NULL);
        g_slist_free (data.paths);

        g_signal_handlers_disconnect_by_func (G_OBJECT (playlist->priv->model),
                                              G_CALLBACK (ario_playlist_rows_reordered_cb),
                                              playlist);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (playlist->priv->model),
                                              GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                              GTK_SORT_ASCENDING);

}

GtkWidget *
ario_playlist_new (GtkUIManager *mgr,
                   GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd = ario_mpd_get_instance ();

        g_return_val_if_fail (instance == NULL, NULL);

        instance = g_object_new (TYPE_ARIO_PLAYLIST,
                                 "ui-manager", mgr,
                                 NULL);

        g_return_val_if_fail (instance->priv != NULL, NULL);

        g_signal_connect_object (mpd,
                                 "playlist_changed",
                                 G_CALLBACK (ario_playlist_changed_cb),
                                 instance, 0);
        g_signal_connect_object (mpd,
                                 "song_changed",
                                 G_CALLBACK (ario_playlist_song_changed_cb),
                                 instance, 0);
        g_signal_connect_object (mpd,
                                 "state_changed",
                                 G_CALLBACK (ario_playlist_state_changed_cb),
                                 instance, 0);

        gtk_action_group_add_actions (group,
                                      ario_playlist_actions,
                                      ario_playlist_n_actions, instance);

        return GTK_WIDGET (instance);
}

        static void
ario_playlist_activate_row (GtkTreePath *path)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        gint id;

        /* get the iter from the path */
        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (instance->priv->model), &iter, path)) {
                /* get the song id */
                gtk_tree_model_get (GTK_TREE_MODEL (instance->priv->model), &iter, ID_COLUMN, &id, -1);
                ario_mpd_do_play_id (id);
        }
}

static void
ario_playlist_move_rows (const int x, const int y)
{
        ARIO_LOG_FUNCTION_START
        GtkTreePath *path = NULL;
        GtkTreeViewDropPosition drop_pos;
        gint pos;
        gint *indice;
        GList *list;
        int offset = 0;
        GtkTreePath *path_to_select;
        GtkTreeModel *model = GTK_TREE_MODEL (instance->priv->model);

        /* get drop location */
        gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (instance->priv->tree), x, y, &path, &drop_pos);

        if (path == NULL) {
                pos = instance->priv->playlist_length;
        } else {
                indice = gtk_tree_path_get_indices (path);
                pos = indice[0];

                /* adjust position acording to drop after */
                if ((drop_pos == GTK_TREE_VIEW_DROP_AFTER
                     || drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
                    && pos < instance->priv->playlist_length - 1)
                        ++pos;

                gtk_tree_path_free (path);
        }

        /* move every dragged row */
        list = gtk_tree_selection_get_selected_rows (instance->priv->selection, &model);
        if (!list)
                return;

        gtk_tree_selection_unselect_all (instance->priv->selection);
        list = g_list_last (list);
        do {
                /* get start pos */
                indice = gtk_tree_path_get_indices ((GtkTreePath *) list->data);

                /* compensate */
                if (pos > indice[0])
                        --pos;

                ario_mpd_queue_move (indice[0] + offset, pos);

                if (pos < indice[0])
                        ++offset;

                path_to_select = gtk_tree_path_new_from_indices (pos, -1);
                gtk_tree_selection_select_path (instance->priv->selection, path_to_select);
                gtk_tree_path_free (path_to_select);
        } while ((list = g_list_previous (list)));

        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);

        ario_mpd_queue_commit ();
}

static void
ario_playlist_add_songs (const GSList *songs,
                         const gint x, const gint y,
                         const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        const GSList *tmp_songs;
        gint *indice;
        int end, offset = 0, drop = 0;
        GtkTreePath *path = NULL;
        GtkTreeViewDropPosition pos;
        gboolean move = TRUE;
        ArioMpdSong *song;

        end = instance->priv->playlist_length;

        if (x < 0 || y < 0) {
                move = FALSE;
        } else {
                /* get drop location */
                gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (instance->priv->tree), x, y, &path, &pos);

                if (path) {
                        /* grab drop localtion */
                        indice = gtk_tree_path_get_indices (path);
                        drop = indice[0];
                        /* adjust position */
                        if ((pos == GTK_TREE_VIEW_DROP_BEFORE
                            || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
                            && drop > 0)
                                --drop;
                        gtk_tree_path_free (path);
                } else {
                        move = FALSE;
                }
        }

        /* For each filename :*/
        for (tmp_songs = songs; tmp_songs; tmp_songs = g_slist_next (tmp_songs)) {
                /* Add it in the playlist*/
                ario_mpd_queue_add (tmp_songs->data);
                ++offset;
                /* move it in the right place */
                if (move)
                        ario_mpd_queue_move (end + offset - 1, drop + offset);
        }

        ario_mpd_queue_commit ();

        if (play) {
                song = ario_mpd_get_playlist_info (end);
                if (song) {
                        ario_mpd_do_play_id (song->id);
                        ario_mpd_free_song (song);
                }
        }
}

static void
ario_playlist_add_dir (const gchar *dir,
                       const gint x, const gint y,
                       const gboolean play)
{
        GSList *tmp;
        ArioMpdFileList *files;
        ArioMpdSong *song;
        GSList *char_songs = NULL;

        files = ario_mpd_list_files (dir, TRUE);
        for (tmp = files->songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                char_songs = g_slist_append (char_songs, song->file);
        }

        ario_playlist_add_songs (char_songs, x, y, play);
        g_slist_free (char_songs);
        ario_mpd_free_file_list (files);
}

static void
ario_playlist_add_criterias (const GSList *criterias,
                             const gint x, const gint y,
                             const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        GSList *filenames = NULL, *songs = NULL;
        const GSList *tmp_criteria, *tmp_songs;
        const ArioMpdCriteria *criteria;
        ArioMpdSong *mpd_song;

        /* For each criteria :*/
        for (tmp_criteria = criterias; tmp_criteria; tmp_criteria = g_slist_next (tmp_criteria)) {
                criteria = tmp_criteria->data;
                songs = ario_mpd_get_songs (criteria, TRUE);

                /* For each song */
                for (tmp_songs = songs; tmp_songs; tmp_songs = g_slist_next (tmp_songs)) {
                        mpd_song = tmp_songs->data;
                        filenames = g_slist_append (filenames, mpd_song->file);
                        mpd_song->file = NULL;
                }

                g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
                g_slist_free (songs);
        }

        ario_playlist_add_songs (filenames,
                                 x, y,
                                 play);

        g_slist_foreach (filenames, (GFunc) g_free, NULL);
        g_slist_free (filenames);
}

static void
ario_playlist_drop_songs (const int x, const int y,
                          const GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START
        gchar **songs;
        GSList *filenames = NULL;
        int i;

        songs = g_strsplit ((const gchar *) data->data, "\n", 0);

        /* For each filename :*/
        for (i=0; songs[i]!=NULL && g_utf8_collate (songs[i], ""); ++i)
                filenames = g_slist_append (filenames, songs[i]);

        ario_playlist_add_songs (filenames,
                                 x, y, FALSE);

        g_strfreev (songs);
        g_slist_free (filenames);
}

static void
ario_playlist_drop_dir (const int x, const int y,
                        const GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START
        const gchar *dir = (const gchar *) data->data;

        ario_playlist_add_dir (dir,
                               x, y, FALSE);
}

static void
ario_playlist_drop_criterias (const int x, const int y,
                              const GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START
        gchar **criterias_str;
        ArioMpdCriteria *criteria;
        ArioMpdAtomicCriteria *atomic_criteria;
        int i = 0, j;
        int nb;
        GSList *filenames = NULL, *criterias = NULL;

        criterias_str = g_strsplit ((const gchar *) data->data, "\n", 0);

        while (criterias_str[i]) {
                nb = atoi (criterias_str[i]);
                criteria = NULL;
                filenames = NULL;

                for (j=0; j<nb; ++j) {
                        atomic_criteria = (ArioMpdAtomicCriteria *) g_malloc0 (sizeof (ArioMpdAtomicCriteria));
                        atomic_criteria->tag = atoi (criterias_str[i+2*j+1]);
                        atomic_criteria->value = g_strdup (criterias_str[i+2*j+2]);
                        criteria = g_slist_append (criteria, atomic_criteria);
                }
                i += 2*nb + 1;

                criterias = g_slist_append (criterias, criteria);
        }
        g_strfreev (criterias_str);

        ario_playlist_add_criterias (criterias,
                                     x, y, FALSE);

        g_slist_foreach (criterias, (GFunc) ario_mpd_criteria_free, NULL);
        g_slist_free (criterias);
}

void
ario_playlist_append_songs (const GSList *songs,
                            const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_add_songs (songs, -1, -1, play);
}


void
ario_playlist_append_mpd_songs (const GSList *songs,
                                const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        const GSList *tmp;
        GSList *char_songs = NULL;
        ArioMpdSong *song;

        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                char_songs = g_slist_append (char_songs, song->file);
        }

        ario_playlist_add_songs (char_songs, -1, -1, play);
        g_slist_free (char_songs);
}

void
ario_playlist_append_artists (const GSList *artists,
                              const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdAtomicCriteria *atomic_criteria;
        ArioMpdCriteria *criteria;
        GSList *criterias = NULL;
        const GSList *tmp;

        for (tmp = artists; tmp; tmp = g_slist_next (tmp)) {
                criteria = NULL;
                atomic_criteria = (ArioMpdAtomicCriteria *) g_malloc0 (sizeof (ArioMpdAtomicCriteria));
                atomic_criteria->tag = MPD_TAG_ITEM_ARTIST;
                atomic_criteria->value = g_strdup (tmp->data);

                criteria = g_slist_append (criteria, atomic_criteria);
                criterias = g_slist_append (criterias, criteria);
        }

        ario_playlist_append_criterias (criterias, play);

        g_slist_foreach (criterias, (GFunc) ario_mpd_criteria_free, NULL);
        g_slist_free (criterias);
}

void
ario_playlist_append_dir (const gchar *dir,
                          const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_add_dir (dir, -1, -1, play);
}

void
ario_playlist_append_criterias (const GSList *criterias,
                                const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_add_criterias (criterias, -1, -1, play);
}

static void
ario_playlist_drag_leave_cb (GtkWidget *widget,
                             GdkDragContext *context,
                             gint x, gint y,
                             GtkSelectionData *data,
                             guint info,
                             guint time,
                             gpointer user_data)
{
        ARIO_LOG_FUNCTION_START

        if (data->type == gdk_atom_intern ("text/internal-list", TRUE))
                ario_playlist_move_rows (x, y);
        else if (data->type == gdk_atom_intern ("text/songs-list", TRUE))
                ario_playlist_drop_songs (x, y, data);
        else if (data->type == gdk_atom_intern ("text/radios-list", TRUE))
                ario_playlist_drop_songs (x, y, data);
        else if (data->type == gdk_atom_intern ("text/directory", TRUE))
                ario_playlist_drop_dir (x, y, data);
        else if (data->type == gdk_atom_intern ("text/criterias-list", TRUE))
                ario_playlist_drop_criterias (x, y, data);

        /* finish the drag */
        gtk_drag_finish (context, TRUE, FALSE, time);
}

static void
ario_playlist_drag_data_get_cb (GtkWidget * widget,
                                GdkDragContext * context,
                                GtkSelectionData * selection_data,
                                guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START

        g_return_if_fail (selection_data != NULL);

        gtk_selection_data_set (selection_data, selection_data->target, 8,
                                NULL, 0);
}

static void
ario_playlist_popup_menu (void)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        menu = gtk_ui_manager_get_widget (instance->priv->ui_manager, "/PlaylistPopup");
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                        gtk_get_current_event_time ());
}

static void
ario_playlist_cmd_clear (GtkAction *action,
                         ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_clear ();
}

static void
ario_playlist_selection_remove_foreach (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        guint *deleted)
{
        ARIO_LOG_FUNCTION_START
        gint *indice;

        indice = gtk_tree_path_get_indices (path);
        ario_mpd_queue_delete_pos (indice[0] - *deleted);
        ++(*deleted);
}

static void
ario_playlist_remove (void)
{
        ARIO_LOG_FUNCTION_START
        guint deleted = 0;

        gtk_tree_selection_selected_foreach (instance->priv->selection,
                                             (GtkTreeSelectionForeachFunc) ario_playlist_selection_remove_foreach,
                                             &deleted);

        ario_mpd_queue_commit ();

        gtk_tree_selection_unselect_all (instance->priv->selection);
}

static void
ario_playlist_cmd_remove (GtkAction *action,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_remove ();
}

static void
ario_playlist_selection_properties_foreach (GtkTreeModel *model,
                                            GtkTreePath *path,
                                            GtkTreeIter *iter,
                                            gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GSList **paths = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, FILE_COLUMN, &val, -1);

        *paths = g_slist_append (*paths, val);
}

static void
ario_playlist_cmd_songs_properties (GtkAction *action,
                                    ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        GSList *paths = NULL;
        GList *songs;
        GtkWidget *songinfos;

        gtk_tree_selection_selected_foreach (playlist->priv->selection,
                                             ario_playlist_selection_properties_foreach,
                                             &paths);

        songs = ario_mpd_get_songs_info (paths);
        g_slist_foreach (paths, (GFunc) g_free, NULL);
        g_slist_free (paths);

        songinfos = ario_shell_songinfos_new (songs);
        if (songinfos)
                gtk_widget_show_all (songinfos);
}

static gboolean
ario_playlist_model_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             ArioPlaylist *playlist)
{
        int song_id;

        gtk_tree_model_get (model, iter, ID_COLUMN, &song_id, -1);

        if (song_id == ario_mpd_get_current_song_id ()) {
                gtk_tree_view_set_cursor (GTK_TREE_VIEW (playlist->priv->tree),
                                          path,
                                          NULL, FALSE);
                return TRUE;
        }

        return FALSE;
}

static void
ario_playlist_cmd_goto_playing_song (GtkAction *action,
                                     ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START

        gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_model_foreach,
                                playlist);
}

void
ario_playlist_cmd_save (GtkAction *action,
                        ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        GtkWidget *hbox;
        GtkWidget *label, *entry;
        gint retval = GTK_RESPONSE_CANCEL;
        gchar *name;

        /* Create the widgets */
        dialog = gtk_dialog_new_with_buttons (_("Save playlist"),
                                              NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_OK);
        label = gtk_label_new (_("Playlist name :"));
        entry = gtk_entry_new ();
        hbox = gtk_hbox_new (FALSE, 5);

        gtk_box_pack_start_defaults (GTK_BOX (hbox), label);
        gtk_box_pack_start_defaults (GTK_BOX (hbox), entry);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
        gtk_box_set_spacing (GTK_BOX (hbox), 4);
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                           hbox);
        gtk_widget_show_all (dialog);

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        if (retval != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                return;
        }
        name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
        gtk_widget_destroy (dialog);

        if (ario_mpd_save_playlist (name)) {
                dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("Playlist already exists. Do you want to ovewrite it?"));

                retval = gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                if (retval == GTK_RESPONSE_YES) {
                        ario_mpd_delete_playlist (name);
                        ario_mpd_save_playlist (name);
                }
        }
        g_free (name);
}

static gboolean
ario_playlist_view_key_press_cb (GtkWidget *widget,
                                 GdkEventKey *event,
                                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        GList *paths;
        GtkTreePath *path = NULL;
        GtkTreeModel *model = GTK_TREE_MODEL (playlist->priv->model);

        if (event->keyval == GDK_Delete)
                ario_playlist_remove ();

        if (event->keyval == GDK_Return) {
                paths = gtk_tree_selection_get_selected_rows (playlist->priv->selection, &model);
                if (paths)
                        path = paths->data;
                if (path)
                        ario_playlist_activate_row (path);

                g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
                g_list_free (paths);

                return TRUE;
        }

        return FALSE;
}

static gboolean
ario_playlist_view_button_press_cb (GtkWidget *widget,
                                    GdkEventButton *event,
                                    ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y, bx, by;
        gboolean selected;

        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (playlist->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1) {
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (widget), x, y, &bx, &by);

                if (bx >= 0 && by >= 0) {
                        if (event->type == GDK_2BUTTON_PRESS) {
                                GtkTreePath *path;

                                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                                if (path) {
                                        ario_playlist_activate_row (path);
                                        gtk_tree_path_free (path);

                                        return FALSE;
                                }

                        }

                        playlist->priv->drag_start_x = x;
                        playlist->priv->drag_start_y = y;
                        playlist->priv->pressed = TRUE;

                        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                        if (path) {
                                selected = gtk_tree_selection_path_is_selected (playlist->priv->selection, path);
                                gtk_tree_path_free (path);

                                return selected;
                        }

                        return TRUE;
                } else {
                        return FALSE;
                }
        }

        if (event->button == 3) {
                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        if (!gtk_tree_selection_path_is_selected (playlist->priv->selection, path)) {
                                gtk_tree_selection_unselect_all (playlist->priv->selection);
                                gtk_tree_selection_select_path (playlist->priv->selection, path);
                        }
                        ario_playlist_popup_menu ();
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}


static gboolean
ario_playlist_view_button_release_cb (GtkWidget *widget,
                                      GdkEventButton *event,
                                      ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        if (!playlist->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                int bx, by;
                gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (widget), event->x, event->y, &bx, &by);
                if (bx >= 0 && by >= 0) {
                        GtkTreePath *path;

                        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                        if (path) {
                                GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                                gtk_tree_path_free (path);
                        }
                }
        }

        playlist->priv->dragging = FALSE;
        playlist->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_playlist_view_motion_notify (GtkWidget *widget,
                                  GdkEventMotion *event,
                                  ArioPlaylist *playlist)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((playlist->priv->dragging) || !(playlist->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - playlist->priv->drag_start_x;
        dy = y - playlist->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                playlist->priv->dragging = TRUE;

        return FALSE;
}

static void
ario_playlist_column_visible_changed_cb (guint notification_id,
                                         ArioPlaylistColumn *ario_column)
{
        ARIO_LOG_FUNCTION_START
        gtk_tree_view_column_set_visible (ario_column->column,
                                          ario_conf_get_integer (ario_column->pref_is_visible, ario_column->default_is_visible));
}


