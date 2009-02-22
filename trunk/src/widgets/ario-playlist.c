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

#include "widgets/ario-playlist.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "servers/ario-server.h"
#include "sources/ario-source-manager.h"
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
static void ario_playlist_changed_cb (ArioServer *server,
                                      ArioPlaylist *playlist);
static void ario_playlist_connectivity_changed_cb (ArioServer *server,
                                                   ArioPlaylist *playlist);
static void ario_playlist_song_changed_cb (ArioServer *server,
                                           ArioPlaylist *playlist);
static void ario_playlist_state_changed_cb (ArioServer *server,
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
static void ario_playlist_drag_drop_cb (GtkWidget * widget,
                                        gint x, gint y,
                                        guint time,
                                        ArioPlaylist *playlist);
static void ario_playlist_cmd_clear (GtkAction *action,
                                     ArioPlaylist *playlist);
static void ario_playlist_cmd_shuffle (GtkAction *action,
                                       ArioPlaylist *playlist);
static void ario_playlist_cmd_remove (GtkAction *action,
                                      ArioPlaylist *playlist);
static void ario_playlist_cmd_crop (GtkAction *action,
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
        GtkScrolledWindow *scrolled_window;
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;
        GtkTreeModelFilter *filter;

        GtkWidget *search_hbox;
        GtkWidget *search_entry;
        gboolean in_search;

        int playlist_id;
        int playlist_length;
        gboolean ignore;

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
        { "PlaylistShuffle", GTK_STOCK_REFRESH, N_("_Shuffle"), NULL,
                NULL,
                G_CALLBACK (ario_playlist_cmd_shuffle) },
        { "PlaylistCrop", GTK_STOCK_CUT, N_("Cr_op"), NULL,
                NULL,
                G_CALLBACK (ario_playlist_cmd_crop) },
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
G_DEFINE_TYPE (ArioPlaylist, ario_playlist, GTK_TYPE_VBOX)

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

static gboolean
ario_playlist_filter_func (GtkTreeModel *model,
                           GtkTreeIter  *iter,
                           ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        gchar *title, *artist, *album, *genre;
        gboolean visible = TRUE, filter;
        const gchar *cmp = gtk_entry_get_text (GTK_ENTRY (playlist->priv->search_entry));
        int i;
        gchar **cmp_str;

        if (!cmp || !g_utf8_collate (cmp, ""))
                return TRUE;

        cmp_str = g_strsplit (cmp, " ", -1); 

        if (!cmp_str)
                return TRUE;

        gtk_tree_model_get (model, iter,
                            TITLE_COLUMN, &title,
                            ARTIST_COLUMN, &artist,
                            ALBUM_COLUMN, &album,
                            GENRE_COLUMN, &genre,
                            -1);

        for (i = 0; cmp_str[i] && g_utf8_collate (cmp_str[i], "") && visible; ++i) {
                filter = FALSE;
                if (title
                    && ario_conf_get_boolean (PREF_TITLE_COLUMN_VISIBLE, PREF_TITLE_COLUMN_VISIBLE_DEFAULT)
                    && ario_util_stristr (title, cmp_str[i])) {
                        filter = TRUE;
                } else if (artist
                           && ario_conf_get_boolean (PREF_ARTIST_COLUMN_VISIBLE, PREF_ARTIST_COLUMN_VISIBLE_DEFAULT)
                           && ario_util_stristr (artist, cmp_str[i])) {
                        filter = TRUE;
                } else if (album
                           && ario_conf_get_boolean (PREF_ALBUM_COLUMN_VISIBLE, PREF_ALBUM_COLUMN_VISIBLE_DEFAULT)
                           && ario_util_stristr (album, cmp_str[i])) {
                        filter = TRUE;
                } else if (genre
                           && ario_conf_get_boolean (PREF_GENRE_COLUMN_VISIBLE, PREF_GENRE_COLUMN_VISIBLE_DEFAULT)
                           && ario_util_stristr (genre, cmp_str[i])) {
                        filter = TRUE;
                }
                visible &= filter;
        }

        g_strfreev (cmp_str);
        g_free (title);
        g_free (artist);
        g_free (album);
        g_free (genre);

        return visible;
}

static void
ario_playlist_search_close (GtkButton *button,
                            ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        gtk_entry_set_text (GTK_ENTRY (playlist->priv->search_entry), "");
        gtk_widget_hide (playlist->priv->search_hbox);
        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                 GTK_TREE_MODEL (playlist->priv->model));
        gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (playlist->priv->tree), TRUE);
        playlist->priv->in_search = FALSE;
}

static void
ario_playlist_search_entry_changed (GtkEntry *entry,
                                    ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        const gchar *cmp = gtk_entry_get_text (GTK_ENTRY (playlist->priv->search_entry));

        if (!cmp || !g_utf8_collate (cmp, "")) {
                ario_playlist_search_close (NULL, playlist);
                gtk_widget_grab_focus (playlist->priv->tree);
        } else {
                gtk_tree_model_filter_refilter (playlist->priv->filter);
        }
}

static gboolean
ario_playlist_search_entry_key_press_cb (GtkWidget *widget,
                                         GdkEventKey *event,
                                         ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        if (event->keyval == GDK_Escape) {
                ario_playlist_search_close (NULL, playlist);
                return TRUE;
        }

        return FALSE;
}

static void
ario_playlist_init (ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        int i;
        const gchar *column_names []  = { " ", _("Track"), _("Title"), _("Artist"), _("Album"), _("Duration"), _("File"), _("Genre"), _("Date") };
        GtkWidget *image, *close_button;

        instance = playlist;
        playlist->priv = ARIO_PLAYLIST_GET_PRIVATE (playlist);
        playlist->priv->playlist_id = -1;
        playlist->priv->playlist_length = 0;
        playlist->priv->play_pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "play.png", NULL);

        playlist->priv->scrolled_window = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
        gtk_scrolled_window_set_policy (playlist->priv->scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (playlist->priv->scrolled_window, GTK_SHADOW_IN);

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

        playlist->priv->filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (playlist->priv->model), NULL));
        gtk_tree_model_filter_set_visible_func (playlist->priv->filter,
                                                (GtkTreeModelFilterVisibleFunc) ario_playlist_filter_func,
                                                playlist,
                                                NULL);
        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                 GTK_TREE_MODEL (playlist->priv->model));

        gtk_tree_view_set_reorderable (GTK_TREE_VIEW (playlist->priv->tree),
                                       TRUE);
        gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (playlist->priv->model),
                                                 ario_playlist_no_sort,
                                                 NULL, NULL);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (playlist->priv->tree),
                                      TRUE);
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (playlist->priv->tree), FALSE);

        g_signal_connect (playlist->priv->model,
                          "sort-column-changed",
                          G_CALLBACK (ario_playlist_sort_changed_cb),
                          playlist);

        playlist->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist->priv->tree));
        gtk_tree_selection_set_mode (playlist->priv->selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (playlist->priv->scrolled_window), playlist->priv->tree);
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

        g_signal_connect (playlist->priv->tree,
                          "drag_drop",
                          G_CALLBACK (ario_playlist_drag_drop_cb),
                          playlist);

        playlist->priv->search_hbox = gtk_hbox_new (FALSE, 6);

        image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
                                          GTK_ICON_SIZE_MENU);
        close_button = gtk_button_new ();
        gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
        gtk_container_add (GTK_CONTAINER (close_button), image);
        g_signal_connect (close_button,
                          "clicked",
                          G_CALLBACK (ario_playlist_search_close),
                          playlist);

        gtk_box_pack_start (GTK_BOX (playlist->priv->search_hbox),
                            close_button,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (playlist->priv->search_hbox),
                            gtk_label_new (_("Filter:")),
                            FALSE, FALSE, 0);
        playlist->priv->search_entry = gtk_entry_new ();
        gtk_box_pack_start (GTK_BOX (playlist->priv->search_hbox),
                            playlist->priv->search_entry,
                            FALSE, FALSE, 0);
        g_signal_connect (playlist->priv->search_entry,
                          "changed",
                          G_CALLBACK (ario_playlist_search_entry_changed),
                          playlist);
        g_signal_connect (playlist->priv->search_entry,
                          "key_press_event",
                          G_CALLBACK (ario_playlist_search_entry_key_press_cb),
                          playlist);


        gtk_widget_show_all (playlist->priv->search_hbox);
        gtk_widget_set_no_show_all (playlist->priv->search_hbox, TRUE);
        gtk_widget_hide (playlist->priv->search_hbox);

        gtk_box_set_homogeneous (GTK_BOX (playlist), FALSE);
        gtk_box_pack_start (GTK_BOX (playlist),
                            GTK_WIDGET (playlist->priv->scrolled_window),
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (playlist),
                            playlist->priv->search_hbox,
                            FALSE, FALSE, 0);
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
                                 int *pos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerSong *song;
        int state;

        state = ario_server_get_current_state ();
        song = ario_server_get_current_song  ();

        if (state != MPD_STATUS_STATE_UNKNOWN
            && state != MPD_STATUS_STATE_STOP
            && song
            && song->pos == *pos)
                gtk_list_store_set (GTK_LIST_STORE (model), iter,
                                    PIXBUF_COLUMN, instance->priv->play_pixbuf,
                                    -1);
        else
                gtk_list_store_set (GTK_LIST_STORE (model), iter,
                                    PIXBUF_COLUMN, NULL,
                                    -1);

        ++*pos;

        return FALSE;
}

static void
ario_playlist_sync_song (void)
{
        ARIO_LOG_FUNCTION_START
        int pos = 0;

        gtk_tree_model_foreach (GTK_TREE_MODEL (instance->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_sync_song_foreach,
                                &pos);
}

static void
ario_playlist_changed_cb (ArioServer *server,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        gint old_length;
        GtkTreeIter iter;
        gchar *time, *track;
        gchar *title;
        GSList *songs, *tmp;
        ArioServerSong *song, *current_song;
        gboolean need_set;
        gboolean need_sync = FALSE;
        GtkTreePath *path;
        GdkPixbuf *pixbuf;
        int state;

        if (playlist->priv->ignore) {
                playlist->priv->ignore = FALSE;
                playlist->priv->playlist_id = ario_server_get_current_playlist_id ();
                return;
        }

        if (!ario_server_is_connected ()) {
                playlist->priv->playlist_length = 0;
                playlist->priv->playlist_id = -1;
                gtk_list_store_clear (playlist->priv->model);
                return;
        }

        songs = ario_server_get_playlist_changes (playlist->priv->playlist_id);
        playlist->priv->playlist_id = ario_server_get_current_playlist_id ();

        old_length = playlist->priv->playlist_length;

        state = ario_server_get_current_state ();
        if (state == MPD_STATUS_STATE_UNKNOWN || state == MPD_STATUS_STATE_STOP)
                current_song = NULL;
        else
                current_song = ario_server_get_current_song ();

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
                        if (current_song && (song->pos == current_song->pos))
                                pixbuf = instance->priv->play_pixbuf;
                        else
                                pixbuf = NULL;
                        gtk_list_store_set (playlist->priv->model, &iter,
                                            PIXBUF_COLUMN, pixbuf,
                                            TRACK_COLUMN, track,
                                            TITLE_COLUMN, title,
                                            ARTIST_COLUMN, song->artist,
                                            ALBUM_COLUMN, song->album ? song->album : ARIO_SERVER_UNKNOWN,
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

        g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
        g_slist_free (songs);

        playlist->priv->playlist_length = ario_server_get_current_playlist_length ();

        if (playlist->priv->playlist_length < old_length) {
                path = gtk_tree_path_new ();
                gtk_tree_path_append_index (path, playlist->priv->playlist_length);

                if (gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                        while (gtk_list_store_remove (playlist->priv->model, &iter)) { }
                }

                gtk_tree_path_free (path);
                need_sync = TRUE;
        }

        if (need_sync)
                ario_playlist_sync_song ();
}

static void
ario_playlist_connectivity_changed_cb (ArioServer *server,
                                       ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        if (!ario_server_is_connected ())
                ario_playlist_changed_cb (server, playlist);
}

static void
ario_playlist_song_changed_cb (ArioServer *server,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_sync_song ();

        if (ario_conf_get_boolean (PREF_PLAYLIST_AUTOSCROLL, PREF_PLAYLIST_AUTOSCROLL_DEFAULT))
                ario_playlist_cmd_goto_playing_song (NULL, playlist);
}

static void
ario_playlist_state_changed_cb (ArioServer *server,
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

static gboolean
ario_playlist_rows_reordered_foreach (GtkTreeModel *model,
                                      GtkTreePath *p,
                                      GtkTreeIter *iter,
                                      GSList **ids)
{
        gint *id;

        id = g_malloc (sizeof (gint));
        gtk_tree_model_get (model, iter, ID_COLUMN, id, -1);

        *ids = g_slist_append (*ids, id);

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
        GSList *tmp, *ids = NULL;
        int i = 0;

        g_signal_handlers_disconnect_by_func (G_OBJECT (playlist->priv->model),
                                              G_CALLBACK (ario_playlist_rows_reordered_cb),
                                              playlist);

        gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_rows_reordered_foreach,
                                &ids);

        for (tmp = ids; tmp; tmp = g_slist_next (tmp)) {
                ario_server_queue_moveid (*((gint *)tmp->data), i);
                ++i;
        }

        playlist->priv->ignore = TRUE;
        ario_server_queue_commit ();

        g_slist_foreach (ids, (GFunc) g_free, NULL);
        g_slist_free (ids);


        g_signal_handlers_block_by_func (playlist->priv->model,
                                         G_CALLBACK (ario_playlist_sort_changed_cb),
                                         playlist);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (playlist->priv->model),
                                              GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                              GTK_SORT_ASCENDING);
        g_signal_handlers_unblock_by_func (playlist->priv->model,
                                           G_CALLBACK (ario_playlist_sort_changed_cb),
                                           playlist);
}

GtkWidget *
ario_playlist_new (GtkUIManager *mgr,
                   GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START
        ArioServer *server = ario_server_get_instance ();

        g_return_val_if_fail (instance == NULL, NULL);

        instance = g_object_new (TYPE_ARIO_PLAYLIST,
                                 "ui-manager", mgr,
                                 NULL);

        g_return_val_if_fail (instance->priv != NULL, NULL);

        g_signal_connect_object (server,
                                 "playlist_changed",
                                 G_CALLBACK (ario_playlist_changed_cb),
                                 instance, 0);
        g_signal_connect_object (server,
                                 "song_changed",
                                 G_CALLBACK (ario_playlist_song_changed_cb),
                                 instance, 0);
        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_playlist_state_changed_cb),
                                 instance, 0);
        g_signal_connect_object (server,
                                 "connectivity_changed",
                                 G_CALLBACK (ario_playlist_connectivity_changed_cb),
                                 instance, 0);

        gtk_action_group_add_actions (group,
                                      ario_playlist_actions,
                                      ario_playlist_n_actions, instance);

        return GTK_WIDGET (instance);
}

static int
ario_playlist_get_indice (GtkTreePath *path)
{
        ARIO_LOG_FUNCTION_START
        GtkTreePath *parent_path = NULL;
        int *indices = NULL;
        int indice = -1;

        if (instance->priv->in_search) {
                parent_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (instance->priv->filter), path);
                if (parent_path)
                        indices = gtk_tree_path_get_indices (parent_path);
        } else {
                indices = gtk_tree_path_get_indices (path);
        }

        if (indices)
                indice = indices[0];

        if (parent_path)
                gtk_tree_path_free (parent_path);

        return indice;
}

static void
ario_playlist_activate_row (GtkTreePath *path)
{
        ARIO_LOG_FUNCTION_START
        ario_server_do_play_pos (ario_playlist_get_indice (path));
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
                    && pos < instance->priv->playlist_length)
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

                ario_server_queue_move (indice[0] + offset, pos);

                if (pos < indice[0])
                        ++offset;

                path_to_select = gtk_tree_path_new_from_indices (pos, -1);
                gtk_tree_selection_select_path (instance->priv->selection, path_to_select);
                gtk_tree_path_free (path_to_select);
        } while ((list = g_list_previous (list)));

        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);

        ario_server_queue_commit ();
}

static gint
ario_playlist_get_drop_position (const int x,
                                 const int y)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewDropPosition pos;
        gint *indice;
        GtkTreePath *path = NULL;
        gint drop = 1;

        if (x < 0 || y < 0)
                return -1;

        /* get drop location */
        gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (instance->priv->tree), x, y, &path, &pos);

        if (!path)
                return -1;

        /* grab drop localtion */
        indice = gtk_tree_path_get_indices (path);
        drop = indice[0];
        /* adjust position */
        if ((pos == GTK_TREE_VIEW_DROP_BEFORE
             || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
            && drop > 0)
                --drop;
        gtk_tree_path_free (path);

        return drop;
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

        ario_server_playlist_add_songs (filenames,
                                        ario_playlist_get_drop_position (x, y),
                                        FALSE);

        g_strfreev (songs);
        g_slist_free (filenames);
}

static void
ario_playlist_drop_dir (const int x, const int y,
                        const GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START
        const gchar *dir = (const gchar *) data->data;

        ario_server_playlist_add_dir (dir,
                                      ario_playlist_get_drop_position (x, y),
                                      FALSE);
}

static void
ario_playlist_drop_criterias (const int x, const int y,
                              const GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START
        gchar **criterias_str;
        ArioServerCriteria *criteria;
        ArioServerAtomicCriteria *atomic_criteria;
        int i = 0, j;
        int nb;
        GSList *filenames = NULL, *criterias = NULL;

        criterias_str = g_strsplit ((const gchar *) data->data, "\n", 0);

        while (criterias_str[i]) {
                nb = atoi (criterias_str[i]);
                criteria = NULL;
                filenames = NULL;

                for (j=0; j<nb; ++j) {
                        atomic_criteria = (ArioServerAtomicCriteria *) g_malloc0 (sizeof (ArioServerAtomicCriteria));
                        atomic_criteria->tag = atoi (criterias_str[i+2*j+1]);
                        atomic_criteria->value = g_strdup (criterias_str[i+2*j+2]);
                        criteria = g_slist_append (criteria, atomic_criteria);
                }
                i += 2*nb + 1;

                criterias = g_slist_append (criterias, criteria);
        }
        g_strfreev (criterias_str);

        ario_server_playlist_add_criterias (criterias,
                                            ario_playlist_get_drop_position (x, y),
                                            FALSE, -1);

        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
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
ario_playlist_drag_drop_cb (GtkWidget * widget,
                            gint x, gint y,
                            guint time,
                            ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->in_search)
                g_signal_stop_emission_by_name (widget, "drag_drop");
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
        ario_server_clear ();
}

static void
ario_playlist_cmd_shuffle (GtkAction *action,
                           ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_server_shuffle ();
}

static void
ario_playlist_selection_remove_foreach (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        guint *deleted)
{
        ARIO_LOG_FUNCTION_START
        gint indice = ario_playlist_get_indice (path);

        if (indice >= 0) {
                ario_server_queue_delete_pos (indice - *deleted);
                ++(*deleted);
        }
}

static void
ario_playlist_remove (void)
{
        ARIO_LOG_FUNCTION_START
        guint deleted = 0;

        gtk_tree_selection_selected_foreach (instance->priv->selection,
                                             (GtkTreeSelectionForeachFunc) ario_playlist_selection_remove_foreach,
                                             &deleted);

        ario_server_queue_commit ();

        gtk_tree_selection_unselect_all (instance->priv->selection);
}

static void
ario_playlist_cmd_remove (GtkAction *action,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_remove ();
}

typedef struct ArioPlaylistCropData
{
        guint kept;
        guint deleted;
} ArioPlaylistCropData;

static void
ario_playlist_selection_crop_foreach (GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      ArioPlaylistCropData *data)
{
        ARIO_LOG_FUNCTION_START
        gint indice = ario_playlist_get_indice (path);

        if (indice >= 0) {
                while (data->deleted + data->kept < indice) {
                        ario_server_queue_delete_pos (data->kept);
                        ++data->deleted;
                }
                ++data->kept;
        }
}

static void
ario_playlist_cmd_crop (GtkAction *action,
                        ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistCropData data;

        data.kept = 0;
        data.deleted = 0;

        gtk_tree_selection_selected_foreach (instance->priv->selection,
                                             (GtkTreeSelectionForeachFunc) ario_playlist_selection_crop_foreach,
                                             &data);

        while (data.deleted + data.kept < instance->priv->playlist_length) {
                ario_server_queue_delete_pos (data.kept);
                ++data.deleted;
        }

        ario_server_queue_commit ();

        gtk_tree_selection_unselect_all (instance->priv->selection);
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
        GtkWidget *songinfos;

        gtk_tree_selection_selected_foreach (playlist->priv->selection,
                                             ario_playlist_selection_properties_foreach,
                                             &paths);

        if (paths) {
                songinfos = ario_shell_songinfos_new (paths);
                if (songinfos)
                        gtk_widget_show_all (songinfos);

                g_slist_foreach (paths, (GFunc) g_free, NULL);
                g_slist_free (paths);
        }
}

static gboolean
ario_playlist_model_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             ArioPlaylist *playlist)
{
        int song_id;

        gtk_tree_model_get (model, iter, ID_COLUMN, &song_id, -1);

        if (song_id == ario_server_get_current_song_id ()) {
                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (playlist->priv->tree),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5, 0);
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

        if (instance->priv->in_search) {
                gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->filter),
                                        (GtkTreeModelForeachFunc) ario_playlist_model_foreach,
                                        playlist);
        } else {
                gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->model),
                                        (GtkTreeModelForeachFunc) ario_playlist_model_foreach,
                                        playlist);
        }
        ario_sourcemanager_goto_playling_song ();
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

        if (ario_server_save_playlist (name)) {
                dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("Playlist already exists. Do you want to ovewrite it?"));

                retval = gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                if (retval == GTK_RESPONSE_YES) {
                        ario_server_delete_playlist (name);
                        ario_server_save_playlist (name);
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

        if (event->keyval == GDK_Delete) {
                ario_playlist_remove ();
        } else if (event->keyval == GDK_Return) {
                paths = gtk_tree_selection_get_selected_rows (playlist->priv->selection, &model);
                if (paths)
                        path = paths->data;
                if (path)
                        ario_playlist_activate_row (path);

                g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
                g_list_free (paths);

                return TRUE;
        } else if (event->string
                   && event->length > 0
                   && event->keyval != GDK_Escape
                   && !(event->state & GDK_CONTROL_MASK)) {
                if (!playlist->priv->in_search) {
                        gtk_widget_show (playlist->priv->search_hbox);
                        gtk_widget_grab_focus (playlist->priv->search_entry);
                        gtk_entry_set_text (GTK_ENTRY (playlist->priv->search_entry), event->string);
                        gtk_editable_set_position (GTK_EDITABLE (playlist->priv->search_entry), -1);
                        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                                 GTK_TREE_MODEL (playlist->priv->filter));
                        gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (playlist->priv->tree), FALSE);
                        playlist->priv->in_search = TRUE;
                }
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

