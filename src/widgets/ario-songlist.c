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
#include "widgets/ario-playlist.h"
#include "shell/ario-shell-songinfos.h"
#include "ario-util.h"
#include "ario-debug.h"

#define DRAG_THRESHOLD 1

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
static void ario_songlist_popup_menu (ArioSonglist *songlist);
static gboolean ario_songlist_button_press_cb (GtkWidget *widget,
                                               GdkEventButton *event,
                                               ArioSonglist *songlist);
static gboolean ario_songlist_button_release_cb (GtkWidget *widget,
                                                 GdkEventButton *event,
                                                 ArioSonglist *songlist);
static gboolean ario_songlist_motion_notify_cb (GtkWidget *widget,
                                                GdkEventMotion *event,
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
        GtkListStore *songlists_model;
        GtkTreeSelection *songlists_selection;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        GtkUIManager *ui_manager;

        gchar *popup;
};

enum
{
        PROP_0,
        PROP_UI_MANAGER
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

#define ARIO_SONGLIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SONGLIST, ArioSonglistPrivate))
G_DEFINE_TYPE (ArioSonglist, ario_songlist, GTK_TYPE_TREE_VIEW)

static void
ario_songlist_class_init (ArioSonglistClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_songlist_finalize;

        object_class->set_property = ario_songlist_set_property;
        object_class->get_property = ario_songlist_get_property;

        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (ArioSonglistPrivate));
}

static void
ario_songlist_init (ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START

        songlist->priv = ARIO_SONGLIST_GET_PRIVATE (songlist);
}

static void
ario_songlist_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
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
        ARIO_LOG_FUNCTION_START
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
        ARIO_LOG_FUNCTION_START
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
        ARIO_LOG_FUNCTION_START
        ArioSonglist *songlist;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        songlist = g_object_new (TYPE_ARIO_SONGLIST,
                                 "ui-manager", mgr,
                                 NULL);

        g_return_val_if_fail (songlist->priv != NULL, NULL);

        /* Titles */
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
        gtk_tree_view_append_column (GTK_TREE_VIEW (songlist), column);

        /* Artists */
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
        gtk_tree_view_append_column (GTK_TREE_VIEW (songlist), column);

        /* Albums */
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
        gtk_tree_view_append_column (GTK_TREE_VIEW (songlist), column);

        songlist->priv->songlists_model = gtk_list_store_new (SONGS_N_COLUMN,
                                                              G_TYPE_STRING,
                                                              G_TYPE_STRING,
                                                              G_TYPE_STRING,
                                                              G_TYPE_STRING);

        gtk_tree_view_set_model (GTK_TREE_VIEW (songlist),
                                 GTK_TREE_MODEL (songlist->priv->songlists_model));
        songlist->priv->songlists_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (songlist));
        gtk_tree_selection_set_mode (songlist->priv->songlists_selection,
                                     GTK_SELECTION_MULTIPLE);

        gtk_drag_source_set (GTK_WIDGET (songlist),
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (songlist),
                          "drag_data_get",
                          G_CALLBACK (ario_songlist_drag_data_get_cb), songlist);

        g_signal_connect (songlist,
                          "button_press_event",
                          G_CALLBACK (ario_songlist_button_press_cb),
                          songlist);
        g_signal_connect (songlist,
                          "button_release_event",
                          G_CALLBACK (ario_songlist_button_release_cb),
                          songlist);
        g_signal_connect (songlist,
                          "motion_notify_event",
                          G_CALLBACK (ario_songlist_motion_notify_cb),
                          songlist);

        songlist->priv->popup = g_strdup (popup);

        return GTK_WIDGET (songlist);
}

static void
songlists_foreach (GtkTreeModel *model,
                   GtkTreePath *path,
                   GtkTreeIter *iter,
                   gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GSList **songlists = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, SONGS_FILENAME_COLUMN, &val, -1);

        *songlists = g_slist_append (*songlists, val);
}

static void
ario_songlist_add_in_playlist (ArioSonglist *songlist,
                               gboolean play)
{
        ARIO_LOG_FUNCTION_START
        GSList *songlists = NULL;

        gtk_tree_selection_selected_foreach (songlist->priv->songlists_selection,
                                             songlists_foreach,
                                             &songlists);
        ario_playlist_append_songs (songlists, play);

        g_slist_foreach (songlists, (GFunc) g_free, NULL);
        g_slist_free (songlists);
}

void
ario_songlist_cmd_add_songlists (GtkAction *action,
                                 ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        ario_songlist_add_in_playlist (songlist, FALSE);
}

void
ario_songlist_cmd_add_play_songlists (GtkAction *action,
                                      ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        ario_songlist_add_in_playlist (songlist, TRUE);
}

void
ario_songlist_cmd_clear_add_play_songlists (GtkAction *action,
                                            ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        ario_server_clear ();
        ario_songlist_add_in_playlist (songlist, TRUE);
}

void
ario_songlist_cmd_songs_properties (GtkAction *action,
                                    ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        GSList *paths = NULL;
        GtkWidget *songinfos;

        gtk_tree_selection_selected_foreach (songlist->priv->songlists_selection,
                                             songlists_foreach,
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
ario_songlist_popup_menu (ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        if (songlist->priv->popup) {
                menu = gtk_ui_manager_get_widget (songlist->priv->ui_manager, songlist->priv->popup);

                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                                gtk_get_current_event_time ());
        }
}

static gboolean
ario_songlist_button_press_cb (GtkWidget *widget,
                               GdkEventButton *event,
                               ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y, bx, by;
        gboolean selected;

        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (songlist->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1) {
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (widget), x, y, &bx, &by);

                if (bx >= 0 && by >= 0) {
                        if (event->type == GDK_2BUTTON_PRESS) {
                                ario_songlist_add_in_playlist (songlist, FALSE);
                                return FALSE;
                        }

                        songlist->priv->drag_start_x = x;
                        songlist->priv->drag_start_y = y;
                        songlist->priv->pressed = TRUE;

                        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                        if (path) {
                                selected = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), path);
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
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        ario_songlist_popup_menu (songlist);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_songlist_button_release_cb (GtkWidget *widget,
                                 GdkEventButton *event,
                                 ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        if (!songlist->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
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

        songlist->priv->dragging = FALSE;
        songlist->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_songlist_motion_notify_cb (GtkWidget *widget,
                                GdkEventMotion *event,
                                ArioSonglist *songlist)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((songlist->priv->dragging) || !(songlist->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - songlist->priv->drag_start_x;
        dy = y - songlist->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                songlist->priv->dragging = TRUE;

        return FALSE;
}

static void
ario_songlist_songlists_selection_drag_foreach (GtkTreeModel *model,
                                                GtkTreePath *path,
                                                GtkTreeIter *iter,
                                                gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GString *songlists = (GString *) userdata;
        g_return_if_fail (songlists != NULL);

        gchar* val = NULL;

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
        ARIO_LOG_FUNCTION_START
        ArioSonglist *songlist;
        GString* songlists = NULL;

        songlist = ARIO_SONGLIST (data);

        g_return_if_fail (IS_ARIO_SONGLIST (songlist));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        songlists = g_string_new("");
        gtk_tree_selection_selected_foreach (songlist->priv->songlists_selection,
                                             ario_songlist_songlists_selection_drag_foreach,
                                             songlists);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) songlists->str,
                                strlen (songlists->str) * sizeof(guchar));

        g_string_free (songlists, TRUE);
}

GtkListStore *
ario_songlist_get_liststore (ArioSonglist *songlist)
{
        ARIO_LOG_FUNCTION_START
        return songlist->priv->songlists_model;
}
