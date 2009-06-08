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

#include "widgets/ario-songlist.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>

#include "ario-util.h"
#include "ario-debug.h"
#include "shell/ario-shell-songinfos.h"
#include "widgets/ario-dnd-tree.h"
#include "widgets/ario-playlist.h"

static void ario_songlist_finalize (GObject *object);
static void ario_songlist_set_property (GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
static void ario_songlist_get_property (GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec);
static void ario_songlist_add_in_playlist (ArioSonglist *songlist,
                                           gboolean play);
static void ario_songlist_popup_menu_cb (ArioDndTree* tree,
                                         ArioSonglist *songlist);
static void ario_songlist_activate_cb (ArioDndTree* tree,
                                       ArioSonglist *songlist);
static void ario_songlist_songlists_selection_drag_foreach (GtkTreeModel *model,
                                                            GtkTreePath *path,
                                                            GtkTreeIter *iter,
                                                            gpointer userdata);

static void ario_songlist_drag_data_get_cb (GtkWidget * widget,
                                            GdkDragContext * context,
                                            GtkSelectionData * selection_data,
                                            guint info, guint time, gpointer data);
struct ArioSonglistPrivate
{
        GtkTreeView* tree;
        GtkListStore *model;
        GtkTreeSelection *selection;

        GtkUIManager *ui_manager;

        gchar *popup;
};

/* Properties */
enum
{
        PROP_0,
        PROP_UI_MANAGER
};

/* Drag and drop targets */
static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

#define ARIO_SONGLIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SONGLIST, ArioSonglistPrivate))
G_DEFINE_TYPE (ArioSonglist, ario_songlist, GTK_TYPE_SCROLLED_WINDOW)

static void
ario_songlist_class_init (ArioSonglistClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* Virtual Methods */
        object_class->finalize = ario_songlist_finalize;
        object_class->set_property = ario_songlist_set_property;
        object_class->get_property = ario_songlist_get_property;

        /* Object properties */
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioSonglistPrivate));
}

static void
ario_songlist_init (ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;

        songlist->priv = ARIO_SONGLIST_GET_PRIVATE (songlist);
}

static void
ario_songlist_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioSonglist *songlist;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SONGLIST (object));

        songlist = ARIO_SONGLIST (object);

        g_return_if_fail (songlist->priv != NULL);
        g_free (songlist->priv->popup);

        G_OBJECT_CLASS (ario_songlist_parent_class)->finalize (object);
}

static void
ario_songlist_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioSonglist *songlist = ARIO_SONGLIST (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                songlist->priv->ui_manager = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_songlist_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioSonglist *songlist = ARIO_SONGLIST (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                g_value_set_object (value, songlist->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_songlist_new (GtkUIManager *mgr,
                   gchar *popup,
                   gboolean is_sortable)
{
        ARIO_LOG_FUNCTION_START;
        ArioSonglist *songlist;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        songlist = g_object_new (TYPE_ARIO_SONGLIST,
                                 "ui-manager", mgr,
                                 NULL);

        g_return_val_if_fail (songlist->priv != NULL, NULL);

        /* Scrolled window properties */
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (songlist), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (songlist), GTK_SHADOW_IN);

        /* Create treeview */
        songlist->priv->tree = GTK_TREE_VIEW (ario_dnd_tree_new (songs_targets,
                                                                 G_N_ELEMENTS (songs_targets),
                                                                 FALSE));

        /* Titles column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           renderer,
                                                           "text", SONGS_TITLE_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        if (is_sortable) {
                gtk_tree_view_column_set_sort_indicator (column, TRUE);
                gtk_tree_view_column_set_sort_column_id (column, SONGS_TITLE_COLUMN);
        }
        gtk_tree_view_append_column (songlist->priv->tree, column);

        /* Artists column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           renderer,
                                                           "text", SONGS_ARTIST_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        if (is_sortable) {
                gtk_tree_view_column_set_sort_indicator (column, TRUE);
                gtk_tree_view_column_set_sort_column_id (column, SONGS_TITLE_COLUMN);
        }
        gtk_tree_view_append_column (songlist->priv->tree, column);

        /* Albums column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                           renderer,
                                                           "text", SONGS_ALBUM_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        if (is_sortable) {
                gtk_tree_view_column_set_sort_indicator (column, TRUE);
                gtk_tree_view_column_set_sort_column_id (column, SONGS_TITLE_COLUMN);
        }
        gtk_tree_view_append_column (songlist->priv->tree, column);

        /* Create model */
        songlist->priv->model = gtk_list_store_new (SONGS_N_COLUMN,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING);
        gtk_tree_view_set_model (songlist->priv->tree,
                                 GTK_TREE_MODEL (songlist->priv->model));

        /* Get selection */
        songlist->priv->selection = gtk_tree_view_get_selection (songlist->priv->tree);
        gtk_tree_selection_set_mode (songlist->priv->selection,
                                     GTK_SELECTION_MULTIPLE);

        /* Connect a few signals for interaction with tree view */
        g_signal_connect (songlist->priv->tree,
                          "drag_data_get",
                          G_CALLBACK (ario_songlist_drag_data_get_cb), songlist);
        g_signal_connect (GTK_TREE_VIEW (songlist->priv->tree),
                          "popup",
                          G_CALLBACK (ario_songlist_popup_menu_cb), songlist);
        g_signal_connect (GTK_TREE_VIEW (songlist->priv->tree),
                          "activate",
                          G_CALLBACK (ario_songlist_activate_cb), songlist);

        songlist->priv->popup = g_strdup (popup);

        gtk_container_add (GTK_CONTAINER (songlist), GTK_WIDGET (songlist->priv->tree));

        return GTK_WIDGET (songlist);
}

static void
songlists_foreach (GtkTreeModel *model,
                   GtkTreePath *path,
                   GtkTreeIter *iter,
                   gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **songlists = (GSList **) userdata;
        gchar *val = NULL;

        /* Append filename to the list */
        gtk_tree_model_get (model, iter, SONGS_FILENAME_COLUMN, &val, -1);
        *songlists = g_slist_append (*songlists, val);
}

static void
ario_songlist_add_in_playlist (ArioSonglist *songlist,
                               gboolean play)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songlists = NULL;

        /* List of selected songs */
        gtk_tree_selection_selected_foreach (songlist->priv->selection,
                                             songlists_foreach,
                                             &songlists);

        /* Append songs to playlist */
        ario_server_playlist_append_songs (songlists, play);

        g_slist_foreach (songlists, (GFunc) g_free, NULL);
        g_slist_free (songlists);
}

void
ario_songlist_cmd_add_songlists (GtkAction *action,
                                 ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Add songs to playlist */
        ario_songlist_add_in_playlist (songlist, FALSE);
}

void
ario_songlist_cmd_add_play_songlists (GtkAction *action,
                                      ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Add songs to playlist and play */
        ario_songlist_add_in_playlist (songlist, TRUE);
}

void
ario_songlist_cmd_clear_add_play_songlists (GtkAction *action,
                                            ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Cleat playlist */
        ario_server_clear ();

        /* Add songs to playlist and play */
        ario_songlist_add_in_playlist (songlist, TRUE);
}

void
ario_songlist_cmd_songs_properties (GtkAction *action,
                                    ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        GSList *paths = NULL;
        GtkWidget *songinfos;

        /* Get list of selected songs */
        gtk_tree_selection_selected_foreach (songlist->priv->selection,
                                             songlists_foreach,
                                             &paths);

        if (paths) {
                /* Launch songinfos dialog for selected songs */
                songinfos = ario_shell_songinfos_new (paths);
                if (songinfos)
                        gtk_widget_show_all (songinfos);

                g_slist_foreach (paths, (GFunc) g_free, NULL);
                g_slist_free (paths);
        }
}

static void
ario_songlist_popup_menu_cb (ArioDndTree* tree,
                             ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;

        /* Show popup */
        if (songlist->priv->popup
            && gtk_tree_selection_count_selected_rows (songlist->priv->selection) > 0) {
                menu = gtk_ui_manager_get_widget (songlist->priv->ui_manager, songlist->priv->popup);

                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                                gtk_get_current_event_time ());
        }
}

static void
ario_songlist_activate_cb (ArioDndTree* tree,
                           ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        /* Add selected songs to playlist */
        ario_songlist_add_in_playlist (songlist, FALSE);
}

static void
ario_songlist_songlists_selection_drag_foreach (GtkTreeModel *model,
                                                GtkTreePath *path,
                                                GtkTreeIter *iter,
                                                gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GString *songlists = (GString *) userdata;
        gchar* val = NULL;

        g_return_if_fail (songlists != NULL);

        /* Append filename to dnd string */
        gtk_tree_model_get (model, iter, SONGS_FILENAME_COLUMN, &val, -1);
        g_string_append (songlists, val);
        g_string_append (songlists, "\n");
        g_free (val);
}

static void
ario_songlist_drag_data_get_cb (GtkWidget * widget,
                                GdkDragContext * context,
                                GtkSelectionData * selection_data,
                                guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioSonglist *songlist;
        GString* songlists = NULL;

        songlist = ARIO_SONGLIST (data);

        g_return_if_fail (IS_ARIO_SONGLIST (songlist));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        /* Get dnd string with all selected songs */
        songlists = g_string_new("");
        gtk_tree_selection_selected_foreach (songlist->priv->selection,
                                             ario_songlist_songlists_selection_drag_foreach,
                                             songlists);

        /* Set drag data */
        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (const guchar *) songlists->str,
                                strlen (songlists->str) * sizeof(guchar));

        g_string_free (songlists, TRUE);
}

GtkListStore *
ario_songlist_get_liststore (ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        return songlist->priv->model;
}

GtkTreeSelection *
ario_songlist_get_selection (ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START;
        return songlist->priv->selection;
}
