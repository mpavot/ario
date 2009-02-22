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

#include "sources/ario-tree.h"
#include <gtk/gtk.h>
#include <string.h>
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "shell/ario-shell-coverselect.h"
#include "shell/ario-shell-coverdownloader.h"
#include "shell/ario-shell-songinfos.h"
#include "preferences/ario-preferences.h"
#include "widgets/ario-playlist.h"
#include "covers/ario-cover-handler.h"
#include "ario-debug.h"

#define DRAG_THRESHOLD 1
#define LIMIT_FOR_IDLE 800

static void ario_tree_finalize (GObject *object);
static void ario_tree_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void ario_tree_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void ario_tree_selection_changed_cb (GtkTreeSelection * selection,
                                            ArioTree *tree);
static void ario_tree_popup_menu (ArioTree *tree);
static gboolean ario_tree_button_press_cb (GtkWidget *widget,
                                           GdkEventButton *event,
                                           ArioTree *tree);
static gboolean ario_tree_button_release_cb (GtkWidget *widget,
                                             GdkEventButton *event,
                                             ArioTree *tree);
static gboolean ario_tree_motion_notify (GtkWidget *widget, 
                                         GdkEventMotion *event,
                                         ArioTree *tree);
static void get_selected_songs_foreach (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer userdata);
static void get_selected_albums_foreach (GtkTreeModel *model,
                                         GtkTreePath *path,
                                         GtkTreeIter *iter,
                                         gpointer userdata);
static gboolean ario_tree_covers_update (GtkTreeModel *model,
                                         GtkTreePath *path,
                                         GtkTreeIter *iter,
                                         gpointer userdata);
static void ario_tree_cover_changed_cb (ArioCoverHandler *cover_handler,
                                        ArioTree *tree);
static void ario_tree_selection_drag_foreach (GtkTreeModel *model,
                                              GtkTreePath *path,
                                              GtkTreeIter *iter,
                                              gpointer userdata);
static void ario_tree_drag_data_get_cb (GtkWidget *widget,
                                        GdkDragContext *context,
                                        GtkSelectionData *selection_data,
                                        guint info, guint time, gpointer userdata);
static void ario_tree_drag_begin_cb (GtkWidget *widget,
                                     GdkDragContext *context,
                                     ArioTree *tree);
static void ario_tree_covertree_visible_changed_cb (guint notification_id,
                                                    ArioTree *tree);
static void ario_tree_album_sort_changed_cb (guint notification_id,
                                             ArioTree *tree);

struct ArioTreePrivate
{
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        GtkUIManager *ui_manager;

        int album_sort;

        ArioServerTag tag;
        gboolean is_first;

        /* List of ArioServerCriteria */
        GSList *criterias;

        guint covertree_notif;
        guint sort_notif;
};

typedef struct
{
        GSList **criterias;
        ArioServerTag tag;
} ArioServerCriteriaData;

typedef struct
{
        GString *string;
        ArioServerTag tag;
} ArioTreeStringData;

enum
{
        PROP_0,
        PROP_UI_MANAGER
};

enum
{
        SELECTION_CHANGED,
        MENU_POPUP,
        LAST_SIGNAL
};
static guint ario_tree_signals[LAST_SIGNAL] = { 0 };

enum
{
        VALUE_COLUMN,
        CRITERIA_COLUMN,
        N_COLUMN
};

enum
{
        ALBUM_VALUE_COLUMN,
        ALBUM_CRITERIA_COLUMN,
        ALBUM_TEXT_COLUMN,
        ALBUM_ALBUM_COLUMN,
        ALBUM_COVER_COLUMN,
        ALBUM_N_COLUMN
};

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

static const GtkTargetEntry criterias_targets  [] = {
        { "text/criterias-list", 0, 0 },
};

#define ARIO_TREE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TREE, ArioTreePrivate))
G_DEFINE_TYPE (ArioTree, ario_tree, GTK_TYPE_SCROLLED_WINDOW)

static void
ario_tree_class_init (ArioTreeClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_tree_finalize;

        object_class->set_property = ario_tree_set_property;
        object_class->get_property = ario_tree_get_property;

        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        ario_tree_signals[SELECTION_CHANGED] =
                g_signal_new ("selection_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioTreeClass, selection_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_tree_signals[MENU_POPUP] =
                g_signal_new ("menu_popup",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioTreeClass, menu_popup),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_type_class_add_private (klass, sizeof (ArioTreePrivate));
}

static gint
ario_tree_albums_sort_func (GtkTreeModel *model,
                            GtkTreeIter *a,
                            GtkTreeIter *b,
                            ArioTree *tree)
{
        ArioServerAlbum *aalbum;
        ArioServerAlbum *balbum;
        int ret;

        gtk_tree_model_get (model, a,
                            ALBUM_ALBUM_COLUMN, &aalbum,
                            -1);
        gtk_tree_model_get (model, b,
                            ALBUM_ALBUM_COLUMN, &balbum,
                            -1);

        if (tree->priv->album_sort == SORT_YEAR) {
                if (aalbum->date && !balbum->date)
                        ret = -1;
                else if (balbum->date && !aalbum->date)
                        ret = 1;
                else if (aalbum->date && balbum->date) {
                        ret = g_utf8_collate (aalbum->date, balbum->date);
                        if (ret == 0)
                                ret = g_utf8_collate (aalbum->album, balbum->album);
                } else {
                        ret = g_utf8_collate (aalbum->album, balbum->album);
                }
        } else {
                ret = g_utf8_collate (aalbum->album, balbum->album);
        }

        return ret;
}

static void
ario_tree_init (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        tree->priv = ARIO_TREE_GET_PRIVATE (tree);
}

static gboolean
ario_tree_is_album_tree (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        return (tree->priv->tag == MPD_TAG_ITEM_ALBUM) && !tree->priv->is_first;
}

static gboolean
ario_tree_is_song_tree (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        return (tree->priv->tag == MPD_TAG_ITEM_TITLE) && !tree->priv->is_first;
}

static gboolean
ario_tree_album_free (GtkTreeModel *model,
                      GtkTreePath *path,
                      GtkTreeIter *iter,
                      gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree = ARIO_TREE (userdata);
        ArioServerAlbum *album;
        g_return_val_if_fail (IS_ARIO_TREE (tree), FALSE);

        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &album, -1);

        ario_server_free_album (album);
        return FALSE;
}

static void
ario_tree_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_TREE (object));

        tree = ARIO_TREE (object);

        g_return_if_fail (tree->priv != NULL);
        if (tree->priv->covertree_notif)
                ario_conf_notification_remove (tree->priv->covertree_notif);

        if (tree->priv->sort_notif)
                ario_conf_notification_remove (tree->priv->sort_notif);

        if (ario_tree_is_album_tree (tree)) {
                gtk_tree_model_foreach (GTK_TREE_MODEL (tree->priv->model),
                                        (GtkTreeModelForeachFunc) ario_tree_album_free,
                                        tree);
        }
 
        gtk_list_store_clear (tree->priv->model);
        g_slist_foreach (tree->priv->criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (tree->priv->criterias);

        G_OBJECT_CLASS (ario_tree_parent_class)->finalize (object);
}

static void
ario_tree_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree = ARIO_TREE (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                tree->priv->ui_manager = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_tree_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree = ARIO_TREE (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                g_value_set_object (value, tree->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_tree_new (GtkUIManager *mgr,
               ArioServerTag tag,
               gboolean is_first)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        tree = ARIO_TREE (g_object_new (TYPE_ARIO_TREE,
                                        "ui-manager", mgr,
                                        NULL));

        g_return_val_if_fail (tree->priv != NULL, NULL);
        tree->priv->tag = tag;

        tree->priv->is_first = is_first;

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree), GTK_SHADOW_IN);

        tree->priv->tree = gtk_tree_view_new ();

        if (!ario_tree_is_album_tree (tree) && !ario_tree_is_song_tree (tree))
                gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (tree->priv->tree), TRUE);

        if (ario_tree_is_album_tree (tree)) {
                /* Cover column */
                renderer = gtk_cell_renderer_pixbuf_new ();
                /* Translators - This "Cover" refers to an album cover art */
                column = gtk_tree_view_column_new_with_attributes (_("Cover"), 
                                                                   renderer, 
                                                                   "pixbuf", 
                                                                   ALBUM_COVER_COLUMN, NULL);        
                gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
                gtk_tree_view_column_set_fixed_width (column, COVER_SIZE + 30);
                gtk_tree_view_column_set_spacing (column, 0);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tree->priv->tree), 
                                             column);
                gtk_tree_view_column_set_visible (column,
                                                  !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));
                /* Text column */
                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                                   renderer,
                                                                   "text", ALBUM_TEXT_COLUMN,
                                                                   NULL);
                gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
                gtk_tree_view_column_set_expand (column, TRUE);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tree->priv->tree), column);
                /* Model */
                tree->priv->model = gtk_list_store_new (ALBUM_N_COLUMN,
                                                        G_TYPE_STRING,
                                                        G_TYPE_POINTER,
                                                        G_TYPE_STRING,
                                                        G_TYPE_POINTER,
                                                        GDK_TYPE_PIXBUF);
                gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->priv->model),
                                                      ALBUM_TEXT_COLUMN, GTK_SORT_ASCENDING);
                gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (tree->priv->model),
                                                 ALBUM_TEXT_COLUMN,
                                                 (GtkTreeIterCompareFunc) ario_tree_albums_sort_func,
                                                 tree,
                                                 NULL);

                g_signal_connect_object (ario_cover_handler_get_instance (),
                                         "cover_changed", G_CALLBACK (ario_tree_cover_changed_cb),
                                         tree, 0);

                tree->priv->covertree_notif = ario_conf_notification_add (PREF_COVER_TREE_HIDDEN,
                                                                          (ArioNotifyFunc) ario_tree_covertree_visible_changed_cb,
                                                                          tree);

                tree->priv->album_sort = ario_conf_get_integer (PREF_ALBUM_SORT, PREF_ALBUM_SORT_DEFAULT);
                tree->priv->sort_notif = ario_conf_notification_add (PREF_ALBUM_SORT,
                                                                     (ArioNotifyFunc) ario_tree_album_sort_changed_cb,
                                                                     tree);
        } else if (ario_tree_is_song_tree (tree)) {
                /* Track column */
                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes (_("Track"),
                                                                   renderer,
                                                                   "text", SONG_TRACK_COLUMN,
                                                                   NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tree->priv->tree), column);
                /* Title column */
                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                                   renderer,
                                                                   "text", SONG_VALUE_COLUMN,
                                                                   NULL);
                gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tree->priv->tree), column);

                tree->priv->model = gtk_list_store_new (SONG_N_COLUMN,
                                                        G_TYPE_STRING,
                                                        G_TYPE_POINTER,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING);
                gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->priv->model),
                                                      SONG_TRACK_COLUMN,
                                                      GTK_SORT_ASCENDING);
        } else {
                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes (ario_server_get_items_names ()[tree->priv->tag],
                                                                   renderer,
                                                                   "text", 0,
                                                                   NULL);
                gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tree->priv->tree), column);
                tree->priv->model = gtk_list_store_new (N_COLUMN,
                                                        G_TYPE_STRING,
                                                        G_TYPE_POINTER);
                gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->priv->model),
                                                      0, GTK_SORT_ASCENDING);
        }
        gtk_tree_view_set_model (GTK_TREE_VIEW (tree->priv->tree),
                                 GTK_TREE_MODEL (tree->priv->model));
        tree->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->priv->tree));
        gtk_tree_selection_set_mode (tree->priv->selection,
                                     GTK_SELECTION_MULTIPLE);
        g_signal_connect (tree->priv->selection,
                          "changed",
                          G_CALLBACK (ario_tree_selection_changed_cb),
                          tree);

        gtk_container_add (GTK_CONTAINER (tree), tree->priv->tree);

        if (ario_tree_is_song_tree (tree)) {
                gtk_drag_source_set (tree->priv->tree,
                                     GDK_BUTTON1_MASK,
                                     songs_targets,
                                     G_N_ELEMENTS (songs_targets),
                                     GDK_ACTION_COPY);
        } else {
                gtk_drag_source_set (tree->priv->tree,
                                     GDK_BUTTON1_MASK,
                                     criterias_targets,
                                     G_N_ELEMENTS (criterias_targets),
                                     GDK_ACTION_COPY);
        }

        g_signal_connect (tree->priv->tree,
                          "drag_data_get", 
                          G_CALLBACK (ario_tree_drag_data_get_cb), tree);

        g_signal_connect (tree->priv->tree,
                          "drag_begin", 
                          G_CALLBACK (ario_tree_drag_begin_cb), tree);

        g_signal_connect (tree->priv->tree,
                          "button_press_event",
                          G_CALLBACK (ario_tree_button_press_cb),
                          tree);
        g_signal_connect (tree->priv->tree,
                          "button_release_event",
                          G_CALLBACK (ario_tree_button_release_cb),
                          tree);
        g_signal_connect (tree->priv->tree,
                          "motion_notify_event",
                          G_CALLBACK (ario_tree_motion_notify),
                          tree);

        return GTK_WIDGET (tree);
}

static void
ario_tree_popup_menu (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        if ((tree->priv->tag == MPD_TAG_ITEM_ALBUM)
            && (gtk_tree_selection_count_selected_rows (tree->priv->selection) == 1)) {
                menu = gtk_ui_manager_get_widget (tree->priv->ui_manager, "/BrowserAlbumsPopupSingle");
        } else if (tree->priv->tag == MPD_TAG_ITEM_TITLE) {
                menu = gtk_ui_manager_get_widget (tree->priv->ui_manager, "/BrowserSongsPopup");
        } else {
                menu = gtk_ui_manager_get_widget (tree->priv->ui_manager, "/BrowserPopup");
        }
        g_signal_emit (G_OBJECT (tree), ario_tree_signals[MENU_POPUP], 0);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                        gtk_get_current_event_time ());
}

static gboolean
ario_tree_button_press_cb (GtkWidget *widget,
                           GdkEventButton *event,
                           ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y;
        gboolean selected;

        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (tree->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                ario_tree_cmd_add (tree, FALSE);
                return FALSE;
        }

        if (event->button == 1) {
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                tree->priv->drag_start_x = x;
                tree->priv->drag_start_y = y;
                tree->priv->pressed = TRUE;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        selected = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), path);
                        gtk_tree_path_free (path);

                        return selected;
                }

                return TRUE;
        }

        if (event->button == 3) {
                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        ario_tree_popup_menu (tree);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_tree_button_release_cb (GtkWidget *widget,
                             GdkEventButton *event,
                             ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        if (!tree->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

                        if (gtk_tree_selection_path_is_selected (selection, path)
                            && (gtk_tree_selection_count_selected_rows (selection) != 1)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        gtk_tree_path_free (path);
                }
        }

        tree->priv->dragging = FALSE;
        tree->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_tree_motion_notify (GtkWidget *widget, 
                         GdkEventMotion *event,
                         ArioTree *tree)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((tree->priv->dragging) || !(tree->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - tree->priv->drag_start_x;
        dy = y - tree->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                tree->priv->dragging = TRUE;

        return FALSE;
}

static void
get_selected_songs_foreach (GtkTreeModel *model,
                            GtkTreePath *path,
                            GtkTreeIter *iter,
                            gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GSList **songs = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, SONG_FILENAME_COLUMN, &val, -1);

        *songs = g_slist_append (*songs, val);
}

static void
get_selected_albums_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GSList **albums = (GSList **) userdata;

        ArioServerAlbum *server_album;

        gtk_tree_model_get (model, iter,
                            ALBUM_ALBUM_COLUMN, &server_album, -1);

        *albums = g_slist_append (*albums, server_album);
}

static gboolean
ario_tree_covers_update (GtkTreeModel *model,
                         GtkTreePath *path,
                         GtkTreeIter *iter,
                         gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree = ARIO_TREE (userdata);
        ArioServerAlbum *album;
        gchar *cover_path;
        GdkPixbuf *cover;

        g_return_val_if_fail (IS_ARIO_TREE (tree), FALSE);

        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &album, -1);

        cover_path = ario_cover_make_ario_cover_path (album->artist, album->album, SMALL_COVER);

        /* The small cover exists, we show it */
        cover = gdk_pixbuf_new_from_file_at_size (cover_path, COVER_SIZE, COVER_SIZE, NULL);
        g_free (cover_path);

        if (!GDK_IS_PIXBUF (cover)) {
                /* There is no cover, we show a transparent picture */
                cover = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COVER_SIZE, COVER_SIZE);
                gdk_pixbuf_fill (cover, 0);
        }

        gtk_list_store_set (tree->priv->model, iter,
                            ALBUM_COVER_COLUMN, cover,
                            -1);

        g_object_unref (G_OBJECT (cover));

        return FALSE;
}

static void
ario_tree_cover_changed_cb (ArioCoverHandler *cover_handler,
                            ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->priv->model),
                                (GtkTreeModelForeachFunc) ario_tree_covers_update,
                                tree);
}

static void
ario_tree_selection_drag_foreach (GtkTreeModel *model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioTreeStringData *data = (ArioTreeStringData *) userdata;
        gchar *val, *buf;
        g_return_if_fail (data != NULL);
        ArioServerCriteria *criteria, *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        if (data->tag == MPD_TAG_ITEM_TITLE) {
                gtk_tree_model_get (model, iter, SONG_FILENAME_COLUMN, &val, -1);
                g_string_append (data->string, val);
                g_string_append (data->string, "\n");

                g_free (val);
        } else {
                gtk_tree_model_get (model, iter, CRITERIA_COLUMN, &criteria, VALUE_COLUMN, &val, -1);
                buf = g_strdup_printf ("%d", g_slist_length (criteria) + 1);
                g_string_append (data->string, buf);
                g_string_append (data->string, "\n");
                g_free (buf);

                for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                        atomic_criteria = tmp->data;
                        buf = g_strdup_printf ("%d", atomic_criteria->tag);
                        g_string_append (data->string, buf);
                        g_string_append (data->string, "\n");
                        g_free (buf);
                        g_string_append (data->string, atomic_criteria->value);
                        g_string_append (data->string, "\n");
                }

                buf = g_strdup_printf ("%d", data->tag);
                g_string_append (data->string, buf);
                g_string_append (data->string, "\n");
                g_free (buf);
                g_string_append (data->string, val);

                g_free (val);
        }
}

static void
ario_tree_drag_data_get_cb (GtkWidget *widget,
                            GdkDragContext *context,
                            GtkSelectionData *selection_data,
                            guint info, guint time, gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *tree;
        GString* string = NULL;
        ArioTreeStringData data;

        tree = ARIO_TREE (userdata);

        g_return_if_fail (IS_ARIO_TREE (tree));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        string = g_string_new ("");
        data.string = string;
        data.tag = tree->priv->tag;
        gtk_tree_selection_selected_foreach (tree->priv->selection,
                                             ario_tree_selection_drag_foreach,
                                             &data);
        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) string->str,
                                strlen (string->str) * sizeof(guchar));

        g_string_free (string, TRUE);
}

static
void ario_tree_drag_begin_cb (GtkWidget *widget,
                              GdkDragContext *context,
                              ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL;
        GSList *criterias = NULL;
        GdkPixbuf *pixbuf;

        if (tree->priv->tag == MPD_TAG_ITEM_ALBUM) {
                gtk_tree_selection_selected_foreach (tree->priv->selection,
                                                     get_selected_albums_foreach,
                                                     &albums);
                pixbuf = ario_util_get_dnd_pixbuf_from_albums (albums);
                g_slist_free (albums);
        } else {
                criterias = ario_tree_get_criterias (tree);
                pixbuf = ario_util_get_dnd_pixbuf (criterias);
                g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
                g_slist_free (criterias);
        }

        if (pixbuf) {
                gtk_drag_source_set_icon_pixbuf (widget, pixbuf);
                g_object_unref (pixbuf);
        } else {
                gtk_drag_source_set_icon_stock (widget, GTK_STOCK_DND);
        }
}

static void
ario_tree_covertree_visible_changed_cb (guint notification_id,
                                        ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        gtk_tree_view_column_set_visible (gtk_tree_view_get_column (GTK_TREE_VIEW (tree->priv->tree), 0),
                                          !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));
        /* Update display */
        ario_tree_fill (tree);
}

static void
ario_tree_album_sort_changed_cb (guint notification_id,
                                 ArioTree *tree)
{
        tree->priv->album_sort = ario_conf_get_integer (PREF_ALBUM_SORT, PREF_ALBUM_SORT_DEFAULT);
        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (tree->priv->model),
                                         ALBUM_TEXT_COLUMN,
                                         (GtkTreeIterCompareFunc) ario_tree_albums_sort_func,
                                         tree,
                                         NULL);
}

static void
ario_tree_selection_changed_cb (GtkTreeSelection *selection,
                                ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        g_signal_emit (G_OBJECT (tree), ario_tree_signals[SELECTION_CHANGED], 0);
}

static void
ario_tree_add_next_albums (ArioTree *tree,
                           const GSList *albums,
                           ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        const GSList *tmp;
        ArioServerAlbum *server_album;
        GtkTreeIter album_iter;
        gchar *cover_path;
        gchar *album;
        gchar *album_date;
        GdkPixbuf *cover;

        for (tmp = albums; tmp; tmp = g_slist_next (tmp)) {
                server_album = tmp->data;
                album_date = NULL;

                cover_path = ario_cover_make_ario_cover_path (server_album->artist, server_album->album, SMALL_COVER);

                /* The small cover exists, we show it */
                cover = gdk_pixbuf_new_from_file_at_size (cover_path, COVER_SIZE, COVER_SIZE, NULL);
                g_free (cover_path);

                if (!GDK_IS_PIXBUF (cover)) {
                        /* There is no cover, we show a transparent picture */
                        cover = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COVER_SIZE, COVER_SIZE);
                        gdk_pixbuf_fill (cover, 0);
                }

                if (server_album->date) {
                        album_date = g_strdup_printf ("%s (%s)", server_album->album, server_album->date);
                        album = album_date;
                } else {
                        album = server_album->album;
                }

                gtk_list_store_append (tree->priv->model, &album_iter);
                gtk_list_store_set (tree->priv->model, &album_iter,
                                    ALBUM_VALUE_COLUMN, server_album->album,
                                    ALBUM_CRITERIA_COLUMN, criteria,
                                    ALBUM_TEXT_COLUMN, album,
                                    ALBUM_ALBUM_COLUMN, server_album,
                                    ALBUM_COVER_COLUMN, cover,
                                    -1);
                g_object_unref (G_OBJECT (cover));
                g_free (album_date);
        }
}

static void
ario_tree_fill_albums (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums, *tmp;

        for (tmp = tree->priv->criterias; tmp; tmp = g_slist_next (tmp)) {
                albums = ario_server_get_albums (tmp->data);
                ario_tree_add_next_albums (tree, albums, tmp->data);
                g_slist_free (albums);
        }
}

static void
ario_tree_add_next_songs (ArioTree *tree,
                          const GSList *songs,
                          ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        const GSList *tmp;
        ArioServerSong *song;
        GtkTreeIter iter;
        gchar *track;
        gchar *title;

        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                gtk_list_store_append (tree->priv->model, &iter);

                track = ario_util_format_track (song->track);
                title = ario_util_format_title (song);
                gtk_list_store_set (tree->priv->model, &iter,
                                    SONG_VALUE_COLUMN, title,
                                    SONG_CRITERIA_COLUMN, criteria,
                                    SONG_TRACK_COLUMN, track,
                                    SONG_FILENAME_COLUMN, song->file,
                                    -1);
                g_free (title);
                g_free (track);
        }
}

static void
ario_tree_fill_songs (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs, *tmp;

        for (tmp = tree->priv->criterias; tmp; tmp = g_slist_next (tmp)) {
                songs = ario_server_get_songs (tmp->data, TRUE);
                ario_tree_add_next_songs (tree, songs, tmp->data);
                g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (songs);
        }
}

typedef struct ArioTreeAddData
{
        ArioTree *tree;
        ArioServerCriteria *criteria;
        GSList *tags;
        GSList *tmp;
}ArioTreeAddData;

static gboolean
ario_tree_add_tags_idle (ArioTreeAddData *data)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;

        if (!data->tmp) {
                g_slist_foreach (data->tags, (GFunc) g_free, NULL);
                g_slist_free (data->tags);
                ario_server_criteria_free (data->criteria);
                g_free (data);
                return FALSE;
        }

        gtk_list_store_append (data->tree->priv->model, &iter);
        gtk_list_store_set (data->tree->priv->model, &iter,
                            VALUE_COLUMN, data->tmp->data,
                            CRITERIA_COLUMN, data->criteria,
                            -1);

        if (data->tmp == data->tags) {
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (data->tree->priv->model), &iter))
                        gtk_tree_selection_select_iter (data->tree->priv->selection, &iter);
        }

        data->tmp = g_slist_next (data->tmp);

        return TRUE;
}

static void
ario_tree_add_tags (ArioTree *tree,
                    ArioServerCriteria *criteria,
                    GSList *tags)
{
        ARIO_LOG_FUNCTION_START
        ArioTreeAddData *data;
        GSList *tmp;
        GtkTreeIter iter;

        if (tree->priv->is_first && g_slist_length (tags) > LIMIT_FOR_IDLE) {
                /* Asynchronous */
                data = (ArioTreeAddData *) g_malloc0 (sizeof (ArioTreeAddData));
                data->tree = tree;
                data->criteria = ario_server_criteria_copy (criteria);
                data->tags = tags;
                data->tmp = tags;

                g_idle_add ((GSourceFunc) ario_tree_add_tags_idle, data);
        } else {
                /* Synchronous */
                for (tmp = tags; tmp; tmp = g_slist_next (tmp)) {
                        gtk_list_store_append (tree->priv->model, &iter);
                        gtk_list_store_set (tree->priv->model, &iter,
                                            VALUE_COLUMN, tmp->data,
                                            CRITERIA_COLUMN, criteria,
                                            -1);
                }
                g_slist_foreach (tags, (GFunc) g_free, NULL);
                g_slist_free (tags);
        }
}

void
ario_tree_fill (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp, *tags;
        GtkTreeIter iter;
        GList *paths = NULL;
        GtkTreePath *path;
        GtkTreeModel *model = GTK_TREE_MODEL (tree->priv->model);
        ArioServerCriteria *criteria;

        g_signal_handlers_block_by_func (G_OBJECT (tree->priv->selection),
                                         G_CALLBACK (ario_tree_selection_changed_cb),
                                         tree);

        if (tree->priv->is_first)
                paths = gtk_tree_selection_get_selected_rows (tree->priv->selection, &model);

        if (ario_tree_is_album_tree (tree)) {
                gtk_tree_model_foreach (GTK_TREE_MODEL (tree->priv->model),
                                        (GtkTreeModelForeachFunc) ario_tree_album_free,
                                        tree);
                gtk_list_store_clear (tree->priv->model);
                ario_tree_fill_albums (tree);
        } else if (ario_tree_is_song_tree (tree)) {
                gtk_list_store_clear (tree->priv->model);
                ario_tree_fill_songs (tree);
        } else {
                gtk_list_store_clear (tree->priv->model);
                if (tree->priv->is_first) {
                        tags = ario_server_list_tags (tree->priv->tag, NULL);
                        ario_tree_add_tags (tree, NULL, tags);
                } else {
                        for (tmp = tree->priv->criterias; tmp; tmp = g_slist_next (tmp)) {
                                criteria = tmp->data;
                                tags = ario_server_list_tags (tree->priv->tag, criteria);
                                ario_tree_add_tags (tree, criteria, tags);
                        }
                }
        }

        gtk_tree_selection_unselect_all (tree->priv->selection);
        if (paths) {
                path = paths->data;
                if (path) {
                        gtk_tree_selection_select_path (tree->priv->selection, path);
                }
        } else {
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree->priv->model), &iter)) {
                        gtk_tree_selection_select_iter (tree->priv->selection, &iter);
                        path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree->priv->model), &iter);
                        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree->priv->tree),
                                                      path,
                                                      NULL,
                                                      TRUE,
                                                      0, 0);
                        gtk_tree_path_free (path);
                }
        }

        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        g_signal_handlers_unblock_by_func (G_OBJECT (tree->priv->selection),
                                           G_CALLBACK (ario_tree_selection_changed_cb),
                                           tree);

        g_signal_emit_by_name (G_OBJECT (tree->priv->selection), "changed", 0);
}

void
ario_tree_clear_criterias (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START

        g_slist_foreach (tree->priv->criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (tree->priv->criterias);
        tree->priv->criterias = NULL;
}

void
ario_tree_add_criteria (ArioTree *tree,
                        ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        tree->priv->criterias = g_slist_append (tree->priv->criterias, criteria);
}

static void
ario_tree_selection_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             ArioServerCriteriaData *data)
{
        ARIO_LOG_FUNCTION_START
        gchar *value;
        ArioServerCriteria *criteria;
        ArioServerCriteria *ret;
        ArioServerAtomicCriteria *atomic_criteria;

        gtk_tree_model_get (model, iter,
                            VALUE_COLUMN, &value,
                            CRITERIA_COLUMN, &criteria,
                            -1);

        ret = ario_server_criteria_copy (criteria);
        atomic_criteria = (ArioServerAtomicCriteria *) g_malloc0 (sizeof (ArioServerAtomicCriteria));
        atomic_criteria->tag = data->tag;
        atomic_criteria->value = value;
        ret = g_slist_append (ret, atomic_criteria);

        *data->criterias = g_slist_append (*data->criterias, ret);
}

GSList *
ario_tree_get_criterias (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        ArioServerCriteriaData data;
        GSList *criterias = NULL;

        data.criterias = &criterias;
        data.tag = tree->priv->tag;

        gtk_tree_selection_selected_foreach (tree->priv->selection,
                                             (GtkTreeSelectionForeachFunc) ario_tree_selection_foreach,
                                             &data);

        return criterias;
}

void
ario_tree_cmd_add (ArioTree *tree,
                   const gboolean play)
{
        ARIO_LOG_FUNCTION_START
        GSList *criterias = NULL;

        criterias = ario_tree_get_criterias (tree);

        ario_server_playlist_append_criterias (criterias, play, -1);

        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
}

static void
get_cover (ArioTree *tree,
           const guint operation)
{
        ARIO_LOG_FUNCTION_START
        GSList *criterias = NULL, *tmp;
        GtkWidget *coverdownloader;
        GSList *albums = NULL;

        criterias = ario_tree_get_criterias (tree);

        coverdownloader = ario_shell_coverdownloader_new ();
        if (coverdownloader) {
                for (tmp = criterias; tmp; tmp = g_slist_next (tmp)) {
                        albums = g_slist_concat (albums, ario_server_get_albums (tmp->data));
                }
                ario_shell_coverdownloader_get_covers_from_albums (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                                   albums,
                                                                   operation);

                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
        }
        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
}

void 
ario_tree_cmd_get_cover (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        get_cover (tree, GET_COVERS);
}

void 
ario_tree_cmd_remove_cover (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure that you want to remove all the selected covers?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        get_cover (tree, REMOVE_COVERS);
}

void
ario_tree_cmd_albums_properties (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverselect;
        GSList *albums = NULL;
        ArioServerAlbum *ario_server_album;

        gtk_tree_selection_selected_foreach (tree->priv->selection,
                                             get_selected_albums_foreach,
                                             &albums);

        ario_server_album = albums->data;
        coverselect = ario_shell_coverselect_new (ario_server_album);
        gtk_dialog_run (GTK_DIALOG (coverselect));
        gtk_widget_destroy (coverselect);

        g_slist_free (albums);
}

void
ario_tree_cmd_songs_properties (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START
        GSList *paths = NULL;
        GtkWidget *songinfos;

        gtk_tree_selection_selected_foreach (tree->priv->selection,
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

typedef struct
{
        const char *tag;
        ArioTree *tree;
} ArioTreeForeachData;

static gboolean
ario_tree_model_foreach (GtkTreeModel *model,
                         GtkTreePath *path,
                         GtkTreeIter *iter,
                         ArioTreeForeachData *data)
{
        gchar *value;

        gtk_tree_model_get (model, iter, VALUE_COLUMN, &value, -1);

        if (ario_util_strcmp (data->tag, value) == 0) {
                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (data->tree->priv->tree),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5, 0);
                gtk_tree_view_set_cursor (GTK_TREE_VIEW (data->tree->priv->tree),
                                          path,
                                          NULL, FALSE);
                g_free (value);
                return TRUE;
        }
        g_free (value);

        return FALSE;
}

void
ario_tree_goto_playling_song (ArioTree *tree,
                              const ArioServerSong *song)
{
        ARIO_LOG_FUNCTION_START
        ArioTreeForeachData data;
        data.tree = tree;
        data.tag = ario_server_song_get_tag (song, tree->priv->tag);
        if (!data.tag)
                data.tag = ARIO_SERVER_UNKNOWN;

        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->priv->model),
                                (GtkTreeModelForeachFunc) ario_tree_model_foreach,
                                &data);
}
