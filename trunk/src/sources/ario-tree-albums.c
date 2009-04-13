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

#include "sources/ario-tree-albums.h"
#include <gtk/gtk.h>
#include <string.h>
#include "lib/ario-conf.h"
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "covers/ario-cover-handler.h"
#include "preferences/ario-preferences.h"
#include "shell/ario-shell-coverselect.h"

static void ario_tree_albums_finalize (GObject *object);
static void ario_tree_albums_build_tree (ArioTree *parent_tree,
                                         GtkTreeView *treeview);
static void ario_tree_albums_fill_tree (ArioTree *parent_tree);
static GdkPixbuf* ario_tree_albums_get_dnd_pixbuf (ArioTree *tree);
static void ario_tree_albums_cover_changed_cb (ArioCoverHandler *cover_handler,
                                               ArioTreeAlbums *tree);
static void ario_tree_albums_album_sort_changed_cb (guint notification_id,
                                                    ArioTreeAlbums *tree);
static void ario_tree_albums_covertree_visible_changed_cb (guint notification_id,
                                                           ArioTree *tree);

struct ArioTreeAlbumsPrivate
{
        int album_sort;

        guint covertree_notif;
        guint sort_notif;
};

/* Tree view columns */
enum
{
        ALBUM_VALUE_COLUMN,
        ALBUM_CRITERIA_COLUMN,
        ALBUM_TEXT_COLUMN,
        ALBUM_ALBUM_COLUMN,
        ALBUM_COVER_COLUMN,
        ALBUM_N_COLUMN
};

#define ARIO_TREE_ALBUMS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TREE_ALBUMS, ArioTreeAlbumsPrivate))
G_DEFINE_TYPE (ArioTreeAlbums, ario_tree_albums, TYPE_ARIO_TREE)

static void
ario_tree_albums_class_init (ArioTreeAlbumsClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioTreeClass *tree_class = ARIO_TREE_CLASS (klass);

        /* GObject virtual methods */
        object_class->finalize = ario_tree_albums_finalize;

        /* ArioTree virtual methods */
        tree_class->build_tree = ario_tree_albums_build_tree;
        tree_class->fill_tree = ario_tree_albums_fill_tree;
        tree_class->get_dnd_pixbuf = ario_tree_albums_get_dnd_pixbuf;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioTreeAlbumsPrivate));
}

static gint
ario_tree_albums_sort_func (GtkTreeModel *model,
                            GtkTreeIter *a,
                            GtkTreeIter *b,
                            ArioTreeAlbums *tree)
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
ario_tree_albums_init (ArioTreeAlbums *tree)
{
        ARIO_LOG_FUNCTION_START;
        tree->priv = ARIO_TREE_ALBUMS_GET_PRIVATE (tree);
}

static gboolean
ario_tree_albums_album_free (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeAlbums *tree;
        ArioServerAlbum *album;
        g_return_val_if_fail (IS_ARIO_TREE_ALBUMS (userdata), FALSE);

        tree = ARIO_TREE_ALBUMS (userdata);

        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &album, -1);

        ario_server_free_album (album);
        return FALSE;
}

static void
ario_tree_albums_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeAlbums *tree;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_TREE_ALBUMS (object));

        tree = ARIO_TREE_ALBUMS (object);

        g_return_if_fail (tree->priv != NULL);
        if (tree->priv->covertree_notif)
                ario_conf_notification_remove (tree->priv->covertree_notif);

        if (tree->priv->sort_notif)
                ario_conf_notification_remove (tree->priv->sort_notif);

        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->parent.model),
                                (GtkTreeModelForeachFunc) ario_tree_albums_album_free,
                                tree);

        G_OBJECT_CLASS (ario_tree_albums_parent_class)->finalize (object);
}

static void
ario_tree_albums_build_tree (ArioTree *parent_tree,
                             GtkTreeView *treeview)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeAlbums *tree;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        g_return_if_fail (IS_ARIO_TREE_ALBUMS (parent_tree));
        tree = ARIO_TREE_ALBUMS (parent_tree);

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
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree->parent.tree),
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
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree->parent.tree), column);
        /* Model */
        tree->parent.model = gtk_list_store_new (ALBUM_N_COLUMN,
                                                 G_TYPE_STRING,
                                                 G_TYPE_POINTER,
                                                 G_TYPE_STRING,
                                                 G_TYPE_POINTER,
                                                 GDK_TYPE_PIXBUF);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->parent.model),
                                              ALBUM_TEXT_COLUMN, GTK_SORT_ASCENDING);
        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (tree->parent.model),
                                         ALBUM_TEXT_COLUMN,
                                         (GtkTreeIterCompareFunc) ario_tree_albums_sort_func,
                                         tree,
                                         NULL);

        g_signal_connect_object (ario_cover_handler_get_instance (),
                                 "cover_changed", G_CALLBACK (ario_tree_albums_cover_changed_cb),
                                 tree, 0);

        tree->priv->covertree_notif = ario_conf_notification_add (PREF_COVER_TREE_HIDDEN,
                                                                  (ArioNotifyFunc) ario_tree_albums_covertree_visible_changed_cb,
                                                                  tree);

        tree->priv->album_sort = ario_conf_get_integer (PREF_ALBUM_SORT, PREF_ALBUM_SORT_DEFAULT);
        tree->priv->sort_notif = ario_conf_notification_add (PREF_ALBUM_SORT,
                                                             (ArioNotifyFunc) ario_tree_albums_album_sort_changed_cb,
                                                             tree);
}

static void
get_selected_albums_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **albums = (GSList **) userdata;

        ArioServerAlbum *server_album;

        gtk_tree_model_get (model, iter,
                            ALBUM_ALBUM_COLUMN, &server_album, -1);

        *albums = g_slist_append (*albums, server_album);
}

static gboolean
ario_tree_albums_covers_update (GtkTreeModel *model,
                                GtkTreePath *path,
                                GtkTreeIter *iter,
                                gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeAlbums *tree = ARIO_TREE_ALBUMS (userdata);
        ArioServerAlbum *album;
        gchar *cover_path;
        GdkPixbuf *cover;

        g_return_val_if_fail (IS_ARIO_TREE_ALBUMS (tree), FALSE);

        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &album, -1);

        cover_path = ario_cover_make_cover_path (album->artist, album->album, SMALL_COVER);

        /* The small cover exists, we show it */
        cover = gdk_pixbuf_new_from_file_at_size (cover_path, COVER_SIZE, COVER_SIZE, NULL);
        g_free (cover_path);

        if (!GDK_IS_PIXBUF (cover)) {
                /* There is no cover, we show a transparent picture */
                cover = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COVER_SIZE, COVER_SIZE);
                gdk_pixbuf_fill (cover, 0);
        }

        gtk_list_store_set (tree->parent.model, iter,
                            ALBUM_COVER_COLUMN, cover,
                            -1);

        g_object_unref (G_OBJECT (cover));

        return FALSE;
}

static void
ario_tree_albums_cover_changed_cb (ArioCoverHandler *cover_handler,
                                   ArioTreeAlbums *tree)
{
        ARIO_LOG_FUNCTION_START;
        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->parent.model),
                                (GtkTreeModelForeachFunc) ario_tree_albums_covers_update,
                                tree);
}

static void
ario_tree_albums_album_sort_changed_cb (guint notification_id,
                                        ArioTreeAlbums *tree)
{
        tree->priv->album_sort = ario_conf_get_integer (PREF_ALBUM_SORT, PREF_ALBUM_SORT_DEFAULT);
        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (tree->parent.model),
                                         ALBUM_TEXT_COLUMN,
                                         (GtkTreeIterCompareFunc) ario_tree_albums_sort_func,
                                         tree,
                                         NULL);
}

static void
ario_tree_albums_add_next_albums (ArioTreeAlbums *tree,
                                  const GSList *albums,
                                  ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START;
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

                cover_path = ario_cover_make_cover_path (server_album->artist, server_album->album, SMALL_COVER);

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

                gtk_list_store_append (tree->parent.model, &album_iter);
                gtk_list_store_set (tree->parent.model, &album_iter,
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
ario_tree_albums_fill_tree (ArioTree *parent_tree)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeAlbums *tree;
        GSList *albums, *tmp;

        g_return_if_fail (IS_ARIO_TREE_ALBUMS (parent_tree));
        tree = ARIO_TREE_ALBUMS (parent_tree);

        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->parent.model),
                                (GtkTreeModelForeachFunc) ario_tree_albums_album_free,
                                tree);
        gtk_list_store_clear (tree->parent.model);

        for (tmp = tree->parent.criterias; tmp; tmp = g_slist_next (tmp)) {
                albums = ario_server_get_albums (tmp->data);
                ario_tree_albums_add_next_albums (tree, albums, tmp->data);
                g_slist_free (albums);
        }
}

static GdkPixbuf*
ario_tree_albums_get_dnd_pixbuf (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GSList *albums = NULL;
        GdkPixbuf *pixbuf;

        gtk_tree_selection_selected_foreach (tree->selection,
                                             get_selected_albums_foreach,
                                             &albums);
        pixbuf = ario_util_get_dnd_pixbuf_from_albums (albums);
        g_slist_free (albums);

        return pixbuf;
}

void
ario_tree_albums_cmd_albums_properties (ArioTreeAlbums *tree)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *coverselect;
        GSList *albums = NULL;
        ArioServerAlbum *ario_server_album;

        gtk_tree_selection_selected_foreach (tree->parent.selection,
                                             get_selected_albums_foreach,
                                             &albums);

        ario_server_album = albums->data;
        coverselect = ario_shell_coverselect_new (ario_server_album);
        gtk_dialog_run (GTK_DIALOG (coverselect));
        gtk_widget_destroy (coverselect);

        g_slist_free (albums);
}

static void
ario_tree_albums_covertree_visible_changed_cb (guint notification_id,
                                               ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        gtk_tree_view_column_set_visible (gtk_tree_view_get_column (GTK_TREE_VIEW (tree->tree), 0),
                                          !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));
        /* Update display */
        ario_tree_fill (tree);
}

