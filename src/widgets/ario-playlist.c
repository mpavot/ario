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

#include "ario-util.h"
#include "ario-debug.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "servers/ario-server.h"
#include "shell/ario-shell-songinfos.h"
#include "sources/ario-source-manager.h"
#include "widgets/ario-dnd-tree.h"

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
static gboolean ario_playlist_drag_drop_cb (GtkWidget * widget,
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
static void ario_playlist_search (ArioPlaylist *playlist,
                                  const char* text);
static void ario_playlist_cmd_search (GtkAction *action,
                                      ArioPlaylist *playlist);
static void ario_playlist_cmd_songs_properties (GtkAction *action,
                                                ArioPlaylist *playlist);
static void ario_playlist_cmd_goto_playing_song (GtkAction *action,
                                                 ArioPlaylist *playlist);
static void ario_playlist_cmd_save (GtkAction *action,
                                    ArioPlaylist *playlist);
static gboolean ario_playlist_view_key_press_cb (GtkWidget *widget,
                                                 GdkEventKey *event,
                                                 ArioPlaylist *playlist);
static void ario_playlist_activate_selected ();
static void ario_playlist_column_visible_changed_cb (guint notification_id,
                                                     ArioPlaylistColumn *ario_column);
static void ario_playlist_popup_menu_cb (ArioDndTree* tree,
                                         ArioPlaylist *playlist);
static void ario_playlist_activate_cb (ArioDndTree* tree,
                                       ArioPlaylist *playlist);

static ArioPlaylist *instance = NULL;

struct ArioPlaylistPrivate
{
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;
        GtkTreeModelFilter *filter;

        GtkWidget *search_hbox;
        GtkWidget *search_entry;
        gboolean in_search;
        const gchar *search_text;
        gulong dnd_handler;

        gint64 playlist_id;
        int playlist_length;
        gint pos;

        GdkPixbuf *play_pixbuf;

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
        { "PlaylistCrop", GTK_STOCK_CUT, N_("Cr_op"), "<control>P",
                NULL,
                G_CALLBACK (ario_playlist_cmd_crop) },
        { "PlaylistSearch", GTK_STOCK_FIND, N_("_Search in playlist"), NULL,
                NULL,
                G_CALLBACK (ario_playlist_cmd_search) },
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

/* Object properties */
enum
{
        PROP_0,
        PROP_UI_MANAGER
};

/* Treeview columns */
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
        DISC_COLUMN,
        ID_COLUMN,
        TIME_COLUMN,
        N_COLUMN
};

/*
 * ArioPlaylistColumn is used to initialise a column in the
 * playlist treeview and defines various column properties
 */
struct ArioPlaylistColumn {
        /* Column number */
        const int columnnb;

        /* Identification of column size preference in Ario
         * configuration system
         */
        const gchar *pref_size;

        /* Default size if it is not set in preferences */
        const int default_size;

        /* Identification of column order preference in Ario
         * configuration system
         */
        const gchar *pref_order;

        /* Default order if it is not set in preferences */
        const int default_order;

        /* Identification of column visibility preference in Ario
         * configuration system
         */
        const gchar *pref_is_visible;

        /* Default visibility if it is not set in preferences */
        const gboolean default_is_visible;

        /* Whether the column is a pixbuf column or a text column */
        const gboolean is_pixbuf;

        /* Whether the column is resizable or not */
        const gboolean is_resizable;

        /* Whether the column is sortable or not */
        const gboolean is_sortable;

        /* Pointer to the column once it is initialized */
        GtkTreeViewColumn *column;
};

/* Definition of all columns with the preperties */
static ArioPlaylistColumn all_columns []  = {
        { PIXBUF_COLUMN, NULL, 20, PREF_PIXBUF_COLUMN_ORDER, PREF_PIXBUF_COLUMN_ORDER_DEFAULT, NULL, TRUE, TRUE, FALSE, FALSE, NULL },
        { TRACK_COLUMN, PREF_TRACK_COLUMN_SIZE, PREF_TRACK_COLUMN_SIZE_DEFAULT, PREF_TRACK_COLUMN_ORDER, PREF_TRACK_COLUMN_ORDER_DEFAULT, PREF_TRACK_COLUMN_VISIBLE, PREF_TRACK_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { TITLE_COLUMN, PREF_TITLE_COLUMN_SIZE, PREF_TITLE_COLUMN_SIZE_DEFAULT, PREF_TITLE_COLUMN_ORDER, PREF_TITLE_COLUMN_ORDER_DEFAULT, PREF_TITLE_COLUMN_VISIBLE, PREF_TITLE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { ARTIST_COLUMN, PREF_ARTIST_COLUMN_SIZE, PREF_ARTIST_COLUMN_SIZE_DEFAULT, PREF_ARTIST_COLUMN_ORDER, PREF_ARTIST_COLUMN_ORDER_DEFAULT, PREF_ARTIST_COLUMN_VISIBLE, PREF_ARTIST_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { ALBUM_COLUMN, PREF_ALBUM_COLUMN_SIZE, PREF_ALBUM_COLUMN_SIZE_DEFAULT, PREF_ALBUM_COLUMN_ORDER, PREF_ALBUM_COLUMN_ORDER_DEFAULT, PREF_ALBUM_COLUMN_VISIBLE, PREF_ALBUM_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { DURATION_COLUMN, PREF_DURATION_COLUMN_SIZE, PREF_DURATION_COLUMN_SIZE_DEFAULT, PREF_DURATION_COLUMN_ORDER, PREF_DURATION_COLUMN_ORDER_DEFAULT, PREF_DURATION_COLUMN_VISIBLE, PREF_DURATION_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { FILE_COLUMN, PREF_FILE_COLUMN_SIZE, PREF_FILE_COLUMN_SIZE_DEFAULT, PREF_FILE_COLUMN_ORDER, PREF_FILE_COLUMN_ORDER_DEFAULT, PREF_FILE_COLUMN_VISIBLE, PREF_FILE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { GENRE_COLUMN, PREF_GENRE_COLUMN_SIZE, PREF_GENRE_COLUMN_SIZE_DEFAULT, PREF_GENRE_COLUMN_ORDER, PREF_GENRE_COLUMN_ORDER_DEFAULT, PREF_GENRE_COLUMN_VISIBLE, PREF_GENRE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { DATE_COLUMN, PREF_DATE_COLUMN_SIZE, PREF_DATE_COLUMN_SIZE_DEFAULT, PREF_DATE_COLUMN_ORDER, PREF_DATE_COLUMN_ORDER_DEFAULT, PREF_DATE_COLUMN_VISIBLE, PREF_DATE_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { DISC_COLUMN, PREF_DISC_COLUMN_SIZE, PREF_DISC_COLUMN_SIZE_DEFAULT, PREF_DISC_COLUMN_ORDER, PREF_DISC_COLUMN_ORDER_DEFAULT, PREF_DISC_COLUMN_VISIBLE, PREF_DISC_COLUMN_VISIBLE_DEFAULT, FALSE, TRUE, TRUE, NULL },
        { -1, NULL, 0, NULL, 0, NULL, FALSE, FALSE, FALSE, FALSE, NULL }
};

/* Targets for drag & drop from other widgets */
static const GtkTargetEntry targets  [] = {
        { "text/internal-list", 0, 10},
        { "text/songs-list", 0, 20 },
        { "text/radios-list", 0, 30 },
        { "text/directory", 0, 40 },
        { "text/criterias-list", 0, 50 },
};

/* Targets for internal drag & drop */
static const GtkTargetEntry internal_targets  [] = {
        { "text/internal-list", 0, 10 },
};

#define ARIO_PLAYLIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PLAYLIST, ArioPlaylistPrivate))
G_DEFINE_TYPE (ArioPlaylist, ario_playlist, ARIO_TYPE_SOURCE)

static gchar *
ario_playlist_get_id (ArioSource *source)
{
        return "playlist";
}

static gchar *
ario_playlist_get_name (ArioSource *source)
{
        return _("Playlist");
}

static gchar *
ario_playlist_get_icon (ArioSource *source)
{
        return GTK_STOCK_INDEX;
}

static void
ario_playlist_class_init (ArioPlaylistClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* Virtual methods */
        object_class->finalize = ario_playlist_finalize;
        object_class->set_property = ario_playlist_set_property;
        object_class->get_property = ario_playlist_get_property;

        /* Virtual ArioSource methods */
        source_class->get_id = ario_playlist_get_id;
        source_class->get_name = ario_playlist_get_name;
        source_class->get_icon = ario_playlist_get_icon;

        /* Properties */
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioPlaylistPrivate));
}

static void
ario_playlist_append_column (ArioPlaylistColumn *ario_column,
                             const gchar *column_name)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        /* Create the appropriate renderer */
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

        /* Set column size */
        gtk_tree_view_column_set_resizable (column, ario_column->is_resizable);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        if (ario_column->pref_size)
                gtk_tree_view_column_set_fixed_width (column, ario_conf_get_integer (ario_column->pref_size, ario_column->default_size));
        else
                gtk_tree_view_column_set_fixed_width (column, ario_column->default_size);

        /* Set column sortation */
        if (ario_column->is_sortable) {
                gtk_tree_view_column_set_sort_indicator (column, TRUE);
                gtk_tree_view_column_set_sort_column_id (column, ario_column->columnnb);
        }

        /* All columns are reorderable */
        gtk_tree_view_column_set_reorderable (column, TRUE);

        /* Column visibility */
        if (ario_column->pref_is_visible)
                gtk_tree_view_column_set_visible (column, ario_conf_get_integer (ario_column->pref_is_visible, ario_column->default_is_visible));
        else
                gtk_tree_view_column_set_visible (column, ario_column->default_is_visible);

        /* Add column to the treeview */
        gtk_tree_view_append_column (GTK_TREE_VIEW (instance->priv->tree), column);
        ario_column->column = column;

        /* Notification if user changes column visibility in preferences */
        if (ario_column->pref_is_visible)
                ario_conf_notification_add (ario_column->pref_is_visible,
                                            (ArioNotifyFunc) ario_playlist_column_visible_changed_cb,
                                            ario_column);
}

static void
ario_playlist_reorder_columns (void)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *orders[N_COLUMN] = {NULL};
        GtkTreeViewColumn *current, *prev = NULL;
        int i, order;

        /* Get an ordered list of column in orders[] thanks to the preferences */
        for (i = 0; all_columns[i].columnnb != -1; ++i)
        {
                order = ario_conf_get_integer (all_columns[i].pref_order, all_columns[i].default_order);
                if (order < N_COLUMN)
                        orders[order] = all_columns[i].column;
        }

        /* Move columns in the order computed in orders[] */
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
ario_playlist_no_sort (G_GNUC_UNUSED GtkTreeModel *model,
                       G_GNUC_UNUSED GtkTreeIter *a,
                       G_GNUC_UNUSED GtkTreeIter *b,
                       G_GNUC_UNUSED gpointer user_data)
{
        /* Default sort always return 0 */
        return 0;
}

static gboolean
ario_playlist_filter_func (GtkTreeModel *model,
                           GtkTreeIter  *iter,
                           ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        gchar *title, *artist, *album, *genre;
        gboolean visible = TRUE, filter;
        int i;
        gchar **cmp_str;

        /* There is no filter if the search box is empty */
        if (!playlist->priv->search_text || *playlist->priv->search_text == '\0')
                return TRUE;

        /* Split on spaces to have multiple filters */
        cmp_str = g_strsplit (playlist->priv->search_text, " ", -1);
        if (!cmp_str)
                return TRUE;

        /* Get data needed to filter */
        gtk_tree_model_get (model, iter,
                            TITLE_COLUMN, &title,
                            ARTIST_COLUMN, &artist,
                            ALBUM_COLUMN, &album,
                            GENRE_COLUMN, &genre,
                            -1);

        /* Loop on every filter */
        for (i = 0; cmp_str[i] && visible; ++i) {
                if (g_utf8_collate (cmp_str[i], ""))
                {
                        /* By default the row doesn't match the filter */
                        filter = FALSE;

                        /* The row match the filter if one of the visible column contains
                         * the filter */
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

                        /* The row must match all the filters to be shown */
                        visible &= filter;
                }
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
        ARIO_LOG_FUNCTION_START;
        /* Hide the search box */
        gtk_entry_set_text (GTK_ENTRY (playlist->priv->search_entry), "");
        gtk_widget_hide (playlist->priv->search_hbox);
        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                 GTK_TREE_MODEL (playlist->priv->model));
        gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (playlist->priv->tree), TRUE);
        playlist->priv->in_search = FALSE;

        /* Stop handling drag & drop differently */
        if (playlist->priv->dnd_handler) {
                g_signal_handler_disconnect (playlist->priv->tree,
                                             playlist->priv->dnd_handler);
                playlist->priv->dnd_handler = 0;
        }
}

static void
ario_playlist_search_entry_changed (GtkEntry *entry,
                                    ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *cmp = gtk_entry_get_text (GTK_ENTRY (playlist->priv->search_entry));
        playlist->priv->search_text = cmp;

        if (!cmp || *cmp == '\0') {
                /* Close search window if text box is empty */
                ario_playlist_search_close (NULL, playlist);
                gtk_widget_grab_focus (playlist->priv->tree);
        } else {
                /* Refilter all rows if filter has changed */
                gtk_tree_model_filter_refilter (playlist->priv->filter);
        }
}

static gboolean
ario_playlist_search_entry_key_press_cb (GtkWidget *widget,
                                         GdkEventKey *event,
                                         ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Escape key closes the search box */
        if (event->keyval == GDK_Escape) {
                ario_playlist_search_close (NULL, playlist);
                return TRUE;
        }

        return FALSE;
}

static void
ario_playlist_init (ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        int i;
        const gchar *column_names []  = { " ", _("Track"), _("Title"), _("Artist"), _("Album"), _("Duration"), _("File"), _("Genre"), _("Date"), _("Disc") };
        GtkWidget *image, *close_button, *vbox;
        GtkScrolledWindow *scrolled_window;

        /* Attributes initialization */
        instance = playlist;
        playlist->priv = ARIO_PLAYLIST_GET_PRIVATE (playlist);
        playlist->priv->playlist_id = -1;
        playlist->priv->pos = -1;
        playlist->priv->playlist_length = 0;
        playlist->priv->play_pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "play.png", NULL);

        /* Create main vbox */
        vbox = gtk_vbox_new (FALSE, 0);

        /* Create scrolled window */
        scrolled_window = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
        gtk_scrolled_window_set_policy (scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (scrolled_window, GTK_SHADOW_IN);

        /* Create the drag & drop tree */
        playlist->priv->tree = ario_dnd_tree_new (internal_targets,
                                                  G_N_ELEMENTS (internal_targets),
                                                  FALSE);
        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (playlist->priv->tree), TRUE);

        /* Append all columns to treeview */
        for (i = 0; all_columns[i].columnnb != -1; ++i)
                ario_playlist_append_column (&all_columns[i], column_names[i]);

        /* Reorder columns */
        ario_playlist_reorder_columns ();

        /* Create tree model */
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
                                                    G_TYPE_STRING,
                                                    G_TYPE_INT,
                                                    G_TYPE_INT);

        /* Create the filter used when the search box is activated */
        playlist->priv->filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (playlist->priv->model), NULL));
        gtk_tree_model_filter_set_visible_func (playlist->priv->filter,
                                                (GtkTreeModelFilterVisibleFunc) ario_playlist_filter_func,
                                                playlist,
                                                NULL);

        /* Set various treeview properties */
        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                 GTK_TREE_MODEL (playlist->priv->model));
        gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (playlist->priv->model),
                                                 ario_playlist_no_sort,
                                                 NULL, NULL);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (playlist->priv->tree),
                                      TRUE);
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (playlist->priv->tree), FALSE);
        playlist->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist->priv->tree));
        gtk_tree_selection_set_mode (playlist->priv->selection,
                                     GTK_SELECTION_MULTIPLE);

        /* Connect signal to detect clicks on columns header for reordering */
        g_signal_connect (playlist->priv->model,
                          "sort-column-changed",
                          G_CALLBACK (ario_playlist_sort_changed_cb),
                          playlist);

        /* Set playlist as drag & drop destination */
        gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (playlist->priv->tree),
                                              targets, G_N_ELEMENTS (targets),
                                              GDK_ACTION_MOVE | GDK_ACTION_COPY);

        /* Add the tree in the scrolled window */
        gtk_container_add (GTK_CONTAINER (scrolled_window), playlist->priv->tree);

        /* Connect various signals of the tree */
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
                          "popup",
                          G_CALLBACK (ario_playlist_popup_menu_cb),
                          playlist);

        g_signal_connect (playlist->priv->tree,
                          "activate",
                          G_CALLBACK (ario_playlist_activate_cb),
                          playlist);

        /* Creation of search box */
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

        /* Add scrolled window to playlist */
        gtk_box_pack_start (GTK_BOX (vbox),
                            GTK_WIDGET (scrolled_window),
                            TRUE, TRUE, 0);

        /* Add search box to playlist */
        gtk_box_pack_start (GTK_BOX (vbox),
                            playlist->priv->search_hbox,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (playlist),
                            vbox,
                            TRUE, TRUE, 0);
}

void
ario_playlist_shutdown (void)
{
        ARIO_LOG_FUNCTION_START;
        int width;
        int orders[N_COLUMN];
        GtkTreeViewColumn *column;
        int i, j = 1;
        GList *columns, *tmp;

        /* Get ordered list of columns in orders[] */
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

        /* Save preferences */
        for (i = 0; all_columns[i].columnnb != -1; ++i) {
                /* Save column size */
                width = gtk_tree_view_column_get_width (all_columns[i].column);
                if (width > 10 && all_columns[i].pref_size)
                        ario_conf_set_integer (all_columns[i].pref_size,
                                               width);
                /* Save column order */
                ario_conf_set_integer (all_columns[i].pref_order, orders[all_columns[i].columnnb]);
        }
}

static void
ario_playlist_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
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
        ARIO_LOG_FUNCTION_START;
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
        ARIO_LOG_FUNCTION_START;
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

/**
 * This method removes the 'playing' pixbuf from the previous played
 * song and adds it to the new song
 */
static void
ario_playlist_sync_song (void)
{
        ARIO_LOG_FUNCTION_START;
        int state = ario_server_get_current_state ();
        ArioServerSong *song = ario_server_get_current_song ();
        GtkTreePath *path;
        GtkTreeIter iter;

        /* If we are still playing and the song has not changed we don't do anything */
        if (song
            && instance->priv->pos == song->pos
            && (state == ARIO_STATE_PLAY
                || state == ARIO_STATE_PAUSE))
                return;

        /* If there is no song playing and it was already the case before, we don't
         * do anything */
        if ((instance->priv->pos == -1
             && (!song
                 || state == ARIO_STATE_UNKNOWN
                 || state == ARIO_STATE_STOP)))
                return;

        /* Remove the 'playing' icon from previous song */
        if (instance->priv->pos >= 0) {
                path = gtk_tree_path_new_from_indices (instance->priv->pos, -1);
                if (gtk_tree_model_get_iter (GTK_TREE_MODEL (instance->priv->model), &iter, path)) {
                        gtk_list_store_set (instance->priv->model, &iter,
                                            PIXBUF_COLUMN, NULL,
                                            -1);
                        instance->priv->pos = -1;
                }
                gtk_tree_path_free (path);
        }

        /* Add 'playing' icon to new song */
        if (song
            && state != ARIO_STATE_UNKNOWN
            && state != ARIO_STATE_STOP) {
                path = gtk_tree_path_new_from_indices (song->pos, -1);
                if (gtk_tree_model_get_iter (GTK_TREE_MODEL (instance->priv->model), &iter, path)) {
                        gtk_list_store_set (instance->priv->model, &iter,
                                            PIXBUF_COLUMN, instance->priv->play_pixbuf,
                                            -1);
                        instance->priv->pos = song->pos;
                }
                gtk_tree_path_free (path);
        }
}

static void
ario_playlist_changed_cb (ArioServer *server,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        gint old_length;
        GtkTreeIter iter;
        gchar track[ARIO_MAX_TRACK_SIZE];
        gchar time[ARIO_MAX_TIME_SIZE];
        gchar *title;
        GSList *songs, *tmp;
        ArioServerSong *song;
        gboolean need_set;
        GtkTreePath *path;

        /* Clear the playlist if ario is not connected to the server */
        if (!ario_server_is_connected ()) {
                playlist->priv->playlist_length = 0;
                playlist->priv->playlist_id = -1;
                gtk_list_store_clear (playlist->priv->model);
                return;
        }

        /* Get changes on server */
        songs = ario_server_get_playlist_changes (playlist->priv->playlist_id);
        playlist->priv->playlist_id = ario_server_get_current_playlist_id ();

        old_length = playlist->priv->playlist_length;

        /* For each change in playlist */
        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                need_set = FALSE;
                /* Decide whether to update or to add */
                if (song->pos < old_length) {
                        /* Update */
                        path = gtk_tree_path_new_from_indices (song->pos, -1);
                        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                                need_set = TRUE;
                        }
                        gtk_tree_path_free (path);
                } else {
                        /* Add */
                        gtk_list_store_append (playlist->priv->model, &iter);
                        need_set = TRUE;
                }
                if (need_set) {
                        /* Set appropriate data in playlist row */
                        ario_util_format_time_buf (song->time, time, ARIO_MAX_TIME_SIZE);
                        ario_util_format_track_buf (song->track, track, ARIO_MAX_TRACK_SIZE);
                        title = ario_util_format_title (song);
                        gtk_list_store_set (playlist->priv->model, &iter,
                                            TRACK_COLUMN, track,
                                            TITLE_COLUMN, title,
                                            ARTIST_COLUMN, song->artist,
                                            ALBUM_COLUMN, song->album ? song->album : ARIO_SERVER_UNKNOWN,
                                            DURATION_COLUMN, time,
                                            FILE_COLUMN, song->file,
                                            GENRE_COLUMN, song->genre,
                                            DATE_COLUMN, song->date,
                                            ID_COLUMN, song->id,
                                            TIME_COLUMN, song->time,
                                            DISC_COLUMN, song->disc,
                                            -1);
                }
        }

        g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
        g_slist_free (songs);

        playlist->priv->playlist_length = ario_server_get_current_playlist_length ();

        /* Remove rows at the end of playlist if playlist size has decreased */
        if (playlist->priv->playlist_length < old_length) {
                path = gtk_tree_path_new_from_indices (playlist->priv->playlist_length, -1);

                if (gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                        while (gtk_list_store_remove (playlist->priv->model, &iter)) { }
                }

                gtk_tree_path_free (path);
        }

        /* Synchronize 'playing' pixbuf in playlist */
        ario_playlist_sync_song ();
}

static void
ario_playlist_connectivity_changed_cb (ArioServer *server,
                                       ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        if (!ario_server_is_connected ())
                ario_playlist_changed_cb (server, playlist);
}

static void
ario_playlist_song_changed_cb (ArioServer *server,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ario_playlist_sync_song ();

        /* Autoscroll in playlist on song chang if option is activated */
        if (ario_conf_get_boolean (PREF_PLAYLIST_AUTOSCROLL, PREF_PLAYLIST_AUTOSCROLL_DEFAULT))
                ario_playlist_cmd_goto_playing_song (NULL, playlist);
}

static void
ario_playlist_state_changed_cb (ArioServer *server,
                                ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        static gboolean first_run = TRUE;

        /* Synchronise song information */
        ario_playlist_sync_song ();

        /* Set focus to playlist at first start */
        if (first_run
            && ario_server_is_connected()) {
                gtk_widget_grab_focus (playlist->priv->tree);
                first_run = FALSE;
        }
}

static void
ario_playlist_sort_changed_cb (GtkTreeSortable *treesortable,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
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
        ARIO_LOG_FUNCTION_START;
        GSList *tmp, *ids = NULL;
        int i = 0;

        g_signal_handlers_disconnect_by_func (G_OBJECT (playlist->priv->model),
                                              G_CALLBACK (ario_playlist_rows_reordered_cb),
                                              playlist);

        /* Get the list of songs ids in the playlist */
        gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_rows_reordered_foreach,
                                &ids);

        /* Move songs on server according to the order in the playlist */
        for (tmp = ids; tmp; tmp = g_slist_next (tmp)) {
                ario_server_queue_moveid (*((gint *)tmp->data), i);
                ++i;
        }

        /* Force a full synchronization of playlist in next update */
        playlist->priv->playlist_id = -1;

        /* Commit songs moves */
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
        ARIO_LOG_FUNCTION_START;
        ArioServer *server = ario_server_get_instance ();

        g_return_val_if_fail (instance == NULL, NULL);

        instance = g_object_new (TYPE_ARIO_PLAYLIST,
                                 "ui-manager", mgr,
                                 NULL);

        g_return_val_if_fail (instance->priv != NULL, NULL);

        /* Connect signals to remain synchronized with music server */
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

        /* Add contextual menu actions */
        gtk_action_group_add_actions (group,
                                      ario_playlist_actions,
                                      ario_playlist_n_actions, instance);

        return GTK_WIDGET (instance);
}

static int
ario_playlist_get_indice (GtkTreePath *path)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreePath *parent_path = NULL;
        int *indices = NULL;
        int indice = -1;

        /* Get indice from a path depending on if search is activated or not */
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
ario_playlist_activate_selected (void)
{
        ARIO_LOG_FUNCTION_START;
        GList *paths;
        GtkTreePath *path = NULL;
        GtkTreeModel *model = GTK_TREE_MODEL (instance->priv->model);

        /* Start playing a song when a row is activated */
        paths = gtk_tree_selection_get_selected_rows (instance->priv->selection, &model);
        if (paths)
                path = paths->data;
        if (path)
                ario_server_do_play_pos (ario_playlist_get_indice (path));

        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);
}

static void
ario_playlist_move_rows (const int x, const int y)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreePath *path = NULL;
        GtkTreeViewDropPosition drop_pos;
        gint pos;
        gint *indice;
        GList *list;
        int offset = 0;
        GtkTreePath *path_to_select;
        GtkTreeModel *model = GTK_TREE_MODEL (instance->priv->model);

        /* Get drop location */
        gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (instance->priv->tree), x, y, &path, &drop_pos);

        if (path == NULL) {
                pos = instance->priv->playlist_length;
        } else {
                indice = gtk_tree_path_get_indices (path);
                pos = indice[0];

                /* Adjust position acording to drop after */
                if ((drop_pos == GTK_TREE_VIEW_DROP_AFTER
                     || drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
                    && pos < instance->priv->playlist_length)
                        ++pos;

                gtk_tree_path_free (path);
        }

        /* Get all selected rows */
        list = gtk_tree_selection_get_selected_rows (instance->priv->selection, &model);
        if (!list)
                return;

        /* Unselect all rows */
        gtk_tree_selection_unselect_all (instance->priv->selection);

        /* For each selected row (starting from the end) */
        for (list = g_list_last (list); list; list = g_list_previous (list)) {
                /* Get start pos */
                indice = gtk_tree_path_get_indices ((GtkTreePath *) list->data);

                /* Compensate */
                if (pos > indice[0])
                        --pos;

                /* Move the song */
                ario_server_queue_move (indice[0] + offset, pos);

                /* Adjust offset to take the move into account */
                if (pos < indice[0])
                        ++offset;

                path_to_select = gtk_tree_path_new_from_indices (pos, -1);
                gtk_tree_selection_select_path (instance->priv->selection, path_to_select);
                gtk_tree_path_free (path_to_select);
        }

        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);

        /* Commit queue moves */
        ario_server_queue_commit ();
}

static void
ario_playlist_copy_rows (const int x, const int y)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreePath *path = NULL;
        GtkTreeViewDropPosition drop_pos;
        gint pos;
        gint *indice;
        GList *list;
        GSList *songs = NULL;
        GtkTreeModel *model = GTK_TREE_MODEL (instance->priv->model);
        GtkTreeIter iter;
        gchar *filename;

        /* Get drop location */
        gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (instance->priv->tree), x, y, &path, &drop_pos);
        if (path == NULL) {
                pos = instance->priv->playlist_length;
        } else {
                indice = gtk_tree_path_get_indices (path);
                pos = indice[0];

                /* Adjust position acording to drop after */
                if ((drop_pos == GTK_TREE_VIEW_DROP_AFTER
                     || drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
                    && pos < instance->priv->playlist_length)
                        ++pos;

                gtk_tree_path_free (path);
        }

        /* Get all selected rows */
        list = gtk_tree_selection_get_selected_rows (instance->priv->selection, &model);

        /* Unselect all rows */
        gtk_tree_selection_unselect_all (instance->priv->selection);

        /* For each selected row (starting from the end) */
        for (; list; list = g_list_next (list)) {
                /* Get start pos */
                gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
                gtk_tree_model_get (model, &iter, FILE_COLUMN, &filename, -1);
                songs = g_slist_append (songs, filename);
        }
        /* Insert songs in playlist */
        ario_server_insert_at (songs, pos - 1);

        g_slist_foreach (songs, (GFunc) g_free, NULL);
        g_slist_free (songs);

        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
}

static gint
ario_playlist_get_drop_position (const int x,
                                 const int y)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewDropPosition pos;
        gint *indice;
        GtkTreePath *path = NULL;
        gint drop = 1;

        if (x < 0 || y < 0)
                return -1;

        /* Get drop location */
        gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (instance->priv->tree), x, y, &path, &pos);

        if (!path)
                return -1;

        /* Grab drop localtion */
        indice = gtk_tree_path_get_indices (path);
        drop = indice[0];
        /* Adjust position */
        if ((pos == GTK_TREE_VIEW_DROP_BEFORE
             || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
            && drop > 0)
                --drop;
        gtk_tree_path_free (path);

        return drop;
}

static void
ario_playlist_drop_songs (const int x, const int y,
                          GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START;
        gchar **songs;
        GSList *filenames = NULL;
        int i;

        songs = g_strsplit ((const gchar *) gtk_selection_data_get_data (data), "\n", 0);

        /* Get a list of filenames */
        for (i=0; songs[i]!=NULL && g_utf8_collate (songs[i], ""); ++i)
                filenames = g_slist_append (filenames, songs[i]);

        /* Add songs to the playlist */
        ario_server_playlist_add_songs (filenames,
                                        ario_playlist_get_drop_position (x, y),
                                        PLAYLIST_ADD);

        g_strfreev (songs);
        g_slist_free (filenames);
}

static void
ario_playlist_drop_dir (const int x, const int y,
                        GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *dir = (const gchar *) gtk_selection_data_get_data (data);

        /* Add a whole directory to the playlist */
        ario_server_playlist_add_dir (dir,
                                      ario_playlist_get_drop_position (x, y),
                                      PLAYLIST_ADD);
}

static void
ario_playlist_drop_criterias (const int x, const int y,
                              GtkSelectionData *data)
{
        ARIO_LOG_FUNCTION_START;
        gchar **criterias_str;
        ArioServerCriteria *criteria;
        ArioServerAtomicCriteria *atomic_criteria;
        int i = 0, j;
        int nb;
        GSList *criterias = NULL;

        /* Get a list of criteria from drag data */
        criterias_str = g_strsplit ((const gchar *) gtk_selection_data_get_data (data), "\n", 0);
        while (criterias_str[i] && *criterias_str[i] != '\0') {
                nb = atoi (criterias_str[i]);
                criteria = NULL;

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

        /* Add all songs matching the list of criteria to the playlist */
        ario_server_playlist_add_criterias (criterias,
                                            ario_playlist_get_drop_position (x, y),
                                            PLAYLIST_ADD, -1);

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
        ARIO_LOG_FUNCTION_START;

        /* Call the appropriate functions depending on data type */
        if (gtk_selection_data_get_data_type (data) == gdk_atom_intern ("text/internal-list", TRUE)) {
#if GTK_CHECK_VERSION(2, 22, 0)
                if (gdk_drag_context_get_actions (context) & GDK_ACTION_COPY)
#else
                if (context->action & GDK_ACTION_COPY)
#endif
                        ario_playlist_copy_rows (x, y);
                else
                        ario_playlist_move_rows (x, y);
        } else if (gtk_selection_data_get_data_type (data) == gdk_atom_intern ("text/songs-list", TRUE)) {
                ario_playlist_drop_songs (x, y, data);
        } else if (gtk_selection_data_get_data_type (data) == gdk_atom_intern ("text/radios-list", TRUE)) {
                ario_playlist_drop_songs (x, y, data);
        } else if (gtk_selection_data_get_data_type (data) == gdk_atom_intern ("text/directory", TRUE)) {
                ario_playlist_drop_dir (x, y, data);
        } else if (gtk_selection_data_get_data_type (data) == gdk_atom_intern ("text/criterias-list", TRUE)) {
                ario_playlist_drop_criterias (x, y, data);
        }

        /* Finish the drag */
        gtk_drag_finish (context, TRUE, FALSE, time);
}

static void
ario_playlist_drag_data_get_cb (GtkWidget * widget,
                                GdkDragContext * context,
                                GtkSelectionData * selection_data,
                                guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START;

        g_return_if_fail (selection_data != NULL);

        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8,
                                NULL, 0);
}

static gboolean
ario_playlist_drag_drop_cb (GtkWidget * widget,
                            gint x, gint y,
                            guint time,
                            ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;

        /* Drag and drop is deactivated when search is activated */
        if (instance->priv->in_search)
                g_signal_stop_emission_by_name (widget, "drag_drop");

        return FALSE;
}

static void
ario_playlist_popup_menu_cb (ArioDndTree* tree,
                             ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;

        /* Show popup menu */
        menu = gtk_ui_manager_get_widget (instance->priv->ui_manager, "/PlaylistPopup");
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                        gtk_get_current_event_time ());
}

static void
ario_playlist_activate_cb (ArioDndTree* tree,
                           ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ario_playlist_activate_selected ();
}

static void
ario_playlist_cmd_clear (GtkAction *action,
                         ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Clear playlist */
        ario_server_clear ();
}

static void
ario_playlist_cmd_shuffle (GtkAction *action,
                           ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Shuffle playlist */
        ario_server_shuffle ();
}

static void
ario_playlist_selection_remove_foreach (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        guint *deleted)
{
        ARIO_LOG_FUNCTION_START;
        gint indice = ario_playlist_get_indice (path);

        if (indice >= 0) {
                /* Remove song from playlist on server */
                ario_server_queue_delete_pos (indice - *deleted);
                ++(*deleted);
        }
}

static void
ario_playlist_remove (void)
{
        ARIO_LOG_FUNCTION_START;
        guint deleted = 0;

        /* Delete every selected song */
        gtk_tree_selection_selected_foreach (instance->priv->selection,
                                             (GtkTreeSelectionForeachFunc) ario_playlist_selection_remove_foreach,
                                             &deleted);

        /* Commit song deletions */
        ario_server_queue_commit ();

        /* Unselect all rows */
        gtk_tree_selection_unselect_all (instance->priv->selection);
}

static void
ario_playlist_cmd_remove (GtkAction *action,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ario_playlist_remove ();
}

typedef struct ArioPlaylistCropData
{
        gint kept;
        gint deleted;
} ArioPlaylistCropData;

static void
ario_playlist_selection_crop_foreach (GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      ArioPlaylistCropData *data)
{
        ARIO_LOG_FUNCTION_START;
        gint indice = ario_playlist_get_indice (path);

        if (indice >= 0) {
                /* Remove all rows between the last kept row and the current row */
                while (data->deleted + data->kept < indice) {
                        ario_server_queue_delete_pos (data->kept);
                        ++data->deleted;
                }

                /* Keep the current row */
                ++data->kept;
        }
}

static void
ario_playlist_cmd_crop (GtkAction *action,
                        ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistCropData data;

        data.kept = 0;
        data.deleted = 0;

        /* Call ario_playlist_selection_crop_foreach, for each selected row */
        gtk_tree_selection_selected_foreach (instance->priv->selection,
                                             (GtkTreeSelectionForeachFunc) ario_playlist_selection_crop_foreach,
                                             &data);

        /* Delete all songs after the last selected one */
        while (data.deleted + data.kept < instance->priv->playlist_length) {
                ario_server_queue_delete_pos (data.kept);
                ++data.deleted;
        }

        /* Commit song deletions */
        ario_server_queue_commit ();

        /* Unselect all rows */
        gtk_tree_selection_unselect_all (instance->priv->selection);
}

static void
ario_playlist_search (ArioPlaylist *playlist,
                      const char* text)
{
        ARIO_LOG_FUNCTION_START;
        if (!playlist->priv->in_search) {
                /* Show search box */
                gtk_widget_show (playlist->priv->search_hbox);
                gtk_widget_grab_focus (playlist->priv->search_entry);
                gtk_entry_set_text (GTK_ENTRY (playlist->priv->search_entry), text);
                gtk_editable_set_position (GTK_EDITABLE (playlist->priv->search_entry), -1);
                gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                         GTK_TREE_MODEL (playlist->priv->filter));
                gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (playlist->priv->tree), FALSE);
                playlist->priv->in_search = TRUE;

                /* Handles drag & drop differently while filters are activated */
                playlist->priv->dnd_handler = g_signal_connect (playlist->priv->tree,
                                                                "drag_drop",
                                                                G_CALLBACK (ario_playlist_drag_drop_cb),
                                                                playlist);
        }
}

static void
ario_playlist_cmd_search (GtkAction *action,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ario_playlist_search (playlist, "");
        gtk_tree_model_filter_refilter (playlist->priv->filter);
}

static void
ario_playlist_selection_properties_foreach (GtkTreeModel *model,
                                            GtkTreePath *path,
                                            GtkTreeIter *iter,
                                            gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **paths = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, FILE_COLUMN, &val, -1);

        *paths = g_slist_append (*paths, val);
}

static void
ario_playlist_cmd_songs_properties (GtkAction *action,
                                    ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        GSList *paths = NULL;
        GtkWidget *songinfos;

        /* Get a list of selected song paths */
        gtk_tree_selection_selected_foreach (playlist->priv->selection,
                                             ario_playlist_selection_properties_foreach,
                                             &paths);

        /* Show the songinfos dialog for all selected songs */
        if (paths) {
                songinfos = ario_shell_songinfos_new (paths);
                if (songinfos)
                        gtk_widget_show_all (songinfos);

                g_slist_foreach (paths, (GFunc) g_free, NULL);
                g_slist_free (paths);
        }
}

static gboolean
ario_playlist_cmd_goto_playing_song_foreach (GtkTreeModel *model,
                                             GtkTreePath *path,
                                             GtkTreeIter *iter,
                                             ArioPlaylist *playlist)
{
        int song_id;

        gtk_tree_model_get (model, iter, ID_COLUMN, &song_id, -1);

        /* The song is the currently playing song */
        if (song_id == ario_server_get_current_song_id ()) {
                /* Scroll to current song */
                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (playlist->priv->tree),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5, 0);
                /* Select current song */
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
        ARIO_LOG_FUNCTION_START;

        /* Call ario_playlist_cmd_goto_playing_song_foreach for each song in playlist */
        if (instance->priv->in_search) {
                gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->filter),
                                        (GtkTreeModelForeachFunc) ario_playlist_cmd_goto_playing_song_foreach,
                                        playlist);
        } else {
                gtk_tree_model_foreach (GTK_TREE_MODEL (playlist->priv->model),
                                        (GtkTreeModelForeachFunc) ario_playlist_cmd_goto_playing_song_foreach,
                                        playlist);
        }

        /* Also go to the playing song in the souces */
        ario_source_manager_goto_playling_song ();
}

void
ario_playlist_cmd_save (GtkAction *action,
                        ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
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

        gtk_box_pack_start (GTK_BOX (hbox),
                            label,
                            TRUE, TRUE,
                            0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            entry,
                            TRUE, TRUE,
                            0);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
        gtk_box_set_spacing (GTK_BOX (hbox), 4);
        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                           hbox);
        gtk_widget_show_all (dialog);

        /* Launch dialog */
        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        if (retval != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                return;
        }

        /* Get playlist name */
        name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
        gtk_widget_destroy (dialog);

        /* Save playlist */
        if (ario_server_save_playlist (name)) {
                /* Playlist exists: ask for a confirmation before replacement */
                dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("Playlist already exists. Do you want to overwrite it?"));

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
        ARIO_LOG_FUNCTION_START;

        if (event->keyval == GDK_Delete) {
                /* Del key removes songs from playlist */
                ario_playlist_remove ();
        } else if (event->keyval == GDK_Return) {
                /* Enter key activate a song */
                ario_playlist_activate_selected ();
                return TRUE;
        } else if (event->string
                   && event->length > 0
                   && event->keyval != GDK_Escape
                   && !(event->state & GDK_CONTROL_MASK)) {
                /* Other keys start the search in playlist */
                ario_playlist_search (playlist, event->string);
        }

        return FALSE;
}

static void
ario_playlist_column_visible_changed_cb (guint notification_id,
                                         ArioPlaylistColumn *ario_column)
{
        ARIO_LOG_FUNCTION_START;
        /* Called when user changes column visibility in preferences */
        gtk_tree_view_column_set_visible (ario_column->column,
                                          ario_conf_get_integer (ario_column->pref_is_visible, ario_column->default_is_visible));
}

static gboolean
ario_playlist_get_total_time_foreach (GtkTreeModel *model,
                                      GtkTreePath *p,
                                      GtkTreeIter *iter,
                                      int *total_time)
{
        gint time;

        gtk_tree_model_get (model, iter, TIME_COLUMN, &time, -1);
        *total_time += time;

        return FALSE;
}

gint
ario_playlist_get_total_time (void)
{
        ARIO_LOG_FUNCTION_START;
        int total_time = 0;

        /* Compute total time by adding all song times */
        gtk_tree_model_foreach (GTK_TREE_MODEL (instance->priv->model),
                                (GtkTreeModelForeachFunc) ario_playlist_get_total_time_foreach,
                                &total_time);

        return total_time;
}

