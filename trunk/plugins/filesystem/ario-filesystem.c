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
#include <string.h>
#include <config.h>
#include <libxml/parser.h>
#include <glib/gi18n.h>
#include "lib/eel-gconf-extensions.h"
#include "ario-filesystem.h"
#include "widgets/ario-songlist.h"
#include "shell/ario-shell-songinfos.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "preferences/ario-preferences.h"

#define DRAG_THRESHOLD 1

static void ario_filesystem_class_init (ArioFilesystemClass *klass);
static void ario_filesystem_init (ArioFilesystem *filesystem);
static void ario_filesystem_finalize (GObject *object);
static void ario_filesystem_shutdown (ArioSource *source);
static void ario_filesystem_set_property (GObject *object,
                                               guint prop_id,
                                               const GValue *value,
                                               GParamSpec *pspec);
static void ario_filesystem_get_property (GObject *object,
                                               guint prop_id,
                                               GValue *value,
                                               GParamSpec *pspec);
static void ario_filesystem_state_changed_cb (ArioMpd *mpd,
                                                   ArioFilesystem *filesystem);
static void ario_filesystem_filesystem_changed_cb (ArioMpd *mpd,
                                                             ArioFilesystem *filesystem);
static void ario_filesystem_cmd_add_filesystem (GtkAction *action,
                                                          ArioFilesystem *filesystem);
static void ario_filesystem_popup_menu (ArioFilesystem *filesystem);
static gboolean ario_filesystem_button_press_cb (GtkWidget *widget,
                                                      GdkEventButton *event,
                                                      ArioFilesystem *filesystem);
static gboolean ario_filesystem_button_release_cb (GtkWidget *widget,
                                                        GdkEventButton *event,
                                                        ArioFilesystem *filesystem);
static gboolean ario_filesystem_motion_notify (GtkWidget *widget, 
                                                    GdkEventMotion *event,
                                                    ArioFilesystem *filesystem);
static void ario_filesystem_playlists_selection_drag_foreach (GtkTreeModel *model,
                                                                   GtkTreePath *path,
                                                                   GtkTreeIter *iter,
                                                                   gpointer userdata);
static void ario_filesystem_playlists_drag_data_get_cb (GtkWidget * widget,
                                                             GdkDragContext * context,
                                                             GtkSelectionData * selection_data,
                                                             guint info, guint time, gpointer data);
static void ario_filesystem_playlists_row_activated_cb (GtkTreeView *tree_view,
                                                        GtkTreePath *path,
                                                        GtkTreeViewColumn *column,
                                                        ArioFilesystem *filesystem);
static void ario_filesystem_playlists_row_expanded_cb (GtkTreeView *tree_view,
                                                       GtkTreeIter *iter,
                                                       GtkTreePath *path,
                                                       ArioFilesystem *filesystem);
static void ario_filesystem_cursor_moved_cb (GtkTreeView *tree_view,
                                             ArioFilesystem *filesystem);
static void ario_filesystem_fill_filesystem (ArioFilesystem *filesystem);

struct ArioFilesystemPrivate
{
        GtkWidget *filesystem;
        GtkWidget *paned;
        GtkTreeStore *filesystem_model;
        GtkTreeSelection *filesystem_selection;

        GtkWidget *songs;

        gboolean connected;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        ArioMpd *mpd;
        ArioPlaylist *playlist;
        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;
};

static GtkActionEntry ario_filesystem_actions [] =
{
        { "FilesystemAddDir", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_filesystem_cmd_add_filesystem) }
};
static guint ario_filesystem_n_actions = G_N_ELEMENTS (ario_filesystem_actions);

static GtkActionEntry ario_filesystem_songs_actions [] =
{
        { "FilesystemAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_add_songlists) },
        { "FilesystemSongsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_songs_properties) }
};
static guint ario_filesystem_n_songs_actions = G_N_ELEMENTS (ario_filesystem_songs_actions);

enum
{
        PROP_0,
        PROP_MPD,
        PROP_PLAYLIST,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
};

enum
{
        PLAYLISTS_ICON_COLUMN,
        PLAYLISTS_ICONSIZE_COLUMN,
        PLAYLISTS_NAME_COLUMN,
        PLAYLISTS_DIR_COLUMN,
        PLAYLISTS_N_COLUMN
};

static const GtkTargetEntry dirs_targets  [] = {
        { "text/directory", 0, 0 },
};

static GObjectClass *parent_class = NULL;

GType
ario_filesystem_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioFilesystemClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_filesystem_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioFilesystem),
                        0,
                        (GInstanceInitFunc) ario_filesystem_init
                };

                type = g_type_register_static (ARIO_TYPE_SOURCE,
                                               "ArioFilesystem",
                                               &our_info, 0);
        }
        return type;
}

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
        return GTK_STOCK_HARDDISK;
}

static void
ario_filesystem_class_init (ArioFilesystemClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
	ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_filesystem_finalize;

        object_class->set_property = ario_filesystem_set_property;
        object_class->get_property = ario_filesystem_get_property;

        source_class->get_id = ario_filesystem_get_id;
        source_class->get_name = ario_filesystem_get_name;
        source_class->get_icon = ario_filesystem_get_icon;
        source_class->shutdown = ario_filesystem_shutdown;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_PLAYLIST,
                                         g_param_spec_object ("playlist",
                                                              "playlist",
                                                              "playlist",
                                                              TYPE_ARIO_PLAYLIST,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_ACTION_GROUP,
                                         g_param_spec_object ("action-group",
                                                              "GtkActionGroup",
                                                              "GtkActionGroup object",
                                                              GTK_TYPE_ACTION_GROUP,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ario_filesystem_init (ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        int pos;
        GtkWidget *scrolledwindow_filesystem;

        filesystem->priv = g_new0 (ArioFilesystemPrivate, 1);

        filesystem->priv->connected = FALSE;

        /* Filesystem tree */
        scrolledwindow_filesystem = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_filesystem);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_filesystem), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_filesystem), GTK_SHADOW_IN);
        filesystem->priv->filesystem = gtk_tree_view_new ();

        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "icon-name", PLAYLISTS_ICON_COLUMN, "stock-size", PLAYLISTS_ICONSIZE_COLUMN, NULL);

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer, "text", PLAYLISTS_NAME_COLUMN, NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 50);
        gtk_tree_view_append_column (GTK_TREE_VIEW (filesystem->priv->filesystem), column);
        filesystem->priv->filesystem_model = gtk_tree_store_new (PLAYLISTS_N_COLUMN,
                                                                           G_TYPE_STRING,
                                                                           G_TYPE_UINT,
                                                                           G_TYPE_STRING,
                                                                           G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (filesystem->priv->filesystem_model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (filesystem->priv->filesystem),
                                 GTK_TREE_MODEL (filesystem->priv->filesystem_model));
        filesystem->priv->filesystem_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (filesystem->priv->filesystem));
        gtk_tree_selection_set_mode (filesystem->priv->filesystem_selection,
                                     GTK_SELECTION_BROWSE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_filesystem), filesystem->priv->filesystem);

        gtk_drag_source_set (filesystem->priv->filesystem,
                             GDK_BUTTON1_MASK,
                             dirs_targets,
                             G_N_ELEMENTS (dirs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (filesystem->priv->filesystem),
                          "drag_data_get", 
                          G_CALLBACK (ario_filesystem_playlists_drag_data_get_cb), filesystem);
        g_signal_connect_object (G_OBJECT (filesystem->priv->filesystem),
                                 "button_press_event",
                                 G_CALLBACK (ario_filesystem_button_press_cb),
                                 filesystem,
                                 0);
        g_signal_connect_object (G_OBJECT (filesystem->priv->filesystem),
                                 "button_release_event",
                                 G_CALLBACK (ario_filesystem_button_release_cb),
                                 filesystem,
                                 0);
        g_signal_connect_object (G_OBJECT (filesystem->priv->filesystem),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_filesystem_motion_notify),
                                 filesystem,
                                 0);
        g_signal_connect_object (G_OBJECT (filesystem->priv->filesystem),
                                 "row-activated",
                                 G_CALLBACK (ario_filesystem_playlists_row_activated_cb),
                                 filesystem, 0);
        g_signal_connect_object (G_OBJECT (filesystem->priv->filesystem),
                                 "test-expand-row",
                                 G_CALLBACK (ario_filesystem_playlists_row_expanded_cb),
                                 filesystem, 0);
        g_signal_connect_object (G_OBJECT (filesystem->priv->filesystem),
                                 "cursor-changed",
                                 G_CALLBACK (ario_filesystem_cursor_moved_cb),
                                 filesystem, 0);

        /* Hpaned properties */
        filesystem->priv->paned = gtk_hpaned_new ();
        gtk_paned_pack1 (GTK_PANED (filesystem->priv->paned), scrolledwindow_filesystem, FALSE, FALSE);

        pos = eel_gconf_get_integer (CONF_FILSYSTEM_HPANED_SIZE, 250);
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

        pos = gtk_paned_get_position (GTK_PANED (filesystem->priv->paned));
        if (pos > 0)
                eel_gconf_set_integer (CONF_FILSYSTEM_HPANED_SIZE,
                                       pos);
}

static void
ario_filesystem_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioFilesystem *filesystem;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_FILESYSTEM (object));
        filesystem = ARIO_FILESYSTEM (object);

        g_return_if_fail (filesystem->priv != NULL);

        g_free (filesystem->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_filesystem_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (object);

        switch (prop_id) {
        case PROP_MPD:
                filesystem->priv->mpd = g_value_get_object (value);

                /* Signals to synchronize the filesystem with mpd */
                g_signal_connect_object (G_OBJECT (filesystem->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_filesystem_state_changed_cb),
                                         filesystem, 0);
                g_signal_connect_object (G_OBJECT (filesystem->priv->mpd),
                                         "updatingdb_changed", G_CALLBACK (ario_filesystem_filesystem_changed_cb),
                                         filesystem, 0);
                break;
        case PROP_PLAYLIST:
                filesystem->priv->playlist = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                filesystem->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                filesystem->priv->actiongroup = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_filesystem_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioFilesystem *filesystem = ARIO_FILESYSTEM (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, filesystem->priv->mpd);
                break;
        case PROP_PLAYLIST:
                g_value_set_object (value, filesystem->priv->playlist);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, filesystem->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, filesystem->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_filesystem_new (GtkUIManager *mgr,
                          GtkActionGroup *group,
                          ArioMpd *mpd,
                          ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioFilesystem *filesystem;
        GtkWidget *scrolledwindow_songs;
        static gboolean is_loaded = FALSE;
        
        filesystem = g_object_new (TYPE_ARIO_FILESYSTEM,
                                        "ui-manager", mgr,
                                        "action-group", group,
                                        "mpd", mpd,
                                        "playlist", playlist,
                                        NULL);
                                        
        g_return_val_if_fail (filesystem->priv != NULL, NULL);   
             
        /* Songs list */
        scrolledwindow_songs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_songs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_SHADOW_IN);
        filesystem->priv->songs = ario_songlist_new (mgr,
                                                          mpd,
                                                          playlist,
                                                          "/FilesystemSongsPopup",
                                                          FALSE);
        gtk_paned_pack2 (GTK_PANED (filesystem->priv->paned), scrolledwindow_songs, TRUE, FALSE);

        gtk_container_add (GTK_CONTAINER (scrolledwindow_songs), filesystem->priv->songs);

       if (!is_loaded) {
                gtk_action_group_add_actions (filesystem->priv->actiongroup,
                                              ario_filesystem_actions,
                                              ario_filesystem_n_actions, filesystem);
                gtk_action_group_add_actions (filesystem->priv->actiongroup,
                                              ario_filesystem_songs_actions,
                                              ario_filesystem_n_songs_actions, filesystem->priv->songs);
                is_loaded = TRUE;
        }

        ario_filesystem_fill_filesystem (filesystem);

        return GTK_WIDGET (filesystem);
}

static void
ario_filesystem_fill_filesystem (ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter filesystem_iter, fake_child;
        GSList *tmp;
        ArioMpdFileList *files;
        gchar *path;

        gtk_tree_store_clear (filesystem->priv->filesystem_model);

        files = ario_mpd_list_files (filesystem->priv->mpd, "/", FALSE);
        for (tmp = files->directories; tmp; tmp = g_slist_next (tmp)) {
                path = tmp->data;
                gtk_tree_store_append (filesystem->priv->filesystem_model, &filesystem_iter, NULL);
                gtk_tree_store_set (filesystem->priv->filesystem_model, &filesystem_iter,
                                    PLAYLISTS_ICON_COLUMN, GTK_STOCK_DIRECTORY,
                                    PLAYLISTS_ICONSIZE_COLUMN, 1,
                                    PLAYLISTS_NAME_COLUMN, path,
                                    PLAYLISTS_DIR_COLUMN, path, -1);
                gtk_tree_store_append(GTK_TREE_STORE (filesystem->priv->filesystem_model), &fake_child, &filesystem_iter);
        }

	ario_mpd_free_file_list (files);

        gtk_tree_selection_unselect_all (filesystem->priv->filesystem_selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filesystem->priv->filesystem_model), &filesystem_iter))
                gtk_tree_selection_select_iter (filesystem->priv->filesystem_selection, &filesystem_iter);
}

static void
ario_filesystem_state_changed_cb (ArioMpd *mpd,
                                  ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        if (filesystem->priv->connected != ario_mpd_is_connected (mpd)) {
                filesystem->priv->connected = ario_mpd_is_connected (mpd);
                ario_filesystem_fill_filesystem (filesystem);
        }
}

static void
ario_filesystem_filesystem_changed_cb (ArioMpd *mpd,
                                                 ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        ario_filesystem_fill_filesystem (filesystem);
}

static void
ario_filesystem_playlists_row_activated_cb (GtkTreeView *tree_view,
                                                     GtkTreePath *path,
                                                     GtkTreeViewColumn *column,
                                                     ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START

        if (!gtk_tree_view_row_expanded (tree_view, path)) {
                gtk_tree_view_expand_row (tree_view, path, FALSE);
        } else {
                gtk_tree_view_collapse_row (tree_view, path);
        }
}

static void ario_filesystem_playlists_row_expanded_cb (GtkTreeView *tree_view,
                                                       GtkTreeIter *iter,
                                                       GtkTreePath *path,
                                                       ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        ario_filesystem_cursor_moved_cb (tree_view,
                                         filesystem);
}

static void
ario_filesystem_cursor_moved_cb (GtkTreeView *tree_view,
                                 ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
	GtkTreeIter iter, child, fake_child, song_iter;
        GtkTreeModel *model = GTK_TREE_MODEL (filesystem->priv->filesystem_model);
        GtkListStore *liststore = ario_songlist_get_liststore (ARIO_SONGLIST (filesystem->priv->songs));
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (filesystem->priv->songs));
        gchar *dir;
        gchar *title;
        GSList *tmp;
        ArioMpdFileList *files;
        ArioMpdSong *song;
        gchar *path;

        if (!gtk_tree_selection_get_selected (filesystem->priv->filesystem_selection,
                                              &model,
                                              &iter))
                return;

        if (gtk_tree_model_iter_children (GTK_TREE_MODEL (filesystem->priv->filesystem_model),
                                          &child,
                                          &iter)) {
                while (gtk_tree_store_remove (GTK_TREE_STORE (filesystem->priv->filesystem_model), &child)) { }
        }
        gtk_list_store_clear (liststore);
        gtk_tree_model_get (GTK_TREE_MODEL (filesystem->priv->filesystem_model), &iter, PLAYLISTS_DIR_COLUMN, &dir, -1);
        g_return_if_fail (dir);

        files = ario_mpd_list_files (filesystem->priv->mpd, dir, FALSE);
        for (tmp = files->directories; tmp; tmp = g_slist_next (tmp)) {
                path = tmp->data;
                gtk_tree_store_append (filesystem->priv->filesystem_model, &child, &iter);
                gtk_tree_store_set (filesystem->priv->filesystem_model, &child,
                                    PLAYLISTS_ICON_COLUMN, GTK_STOCK_DIRECTORY,
                                    PLAYLISTS_ICONSIZE_COLUMN, 1,
                                    PLAYLISTS_NAME_COLUMN, path + strlen (dir) + 1,
                                    PLAYLISTS_DIR_COLUMN, path, -1);
                gtk_tree_store_append(GTK_TREE_STORE (filesystem->priv->filesystem_model), &fake_child, &child);
        }
        for (tmp = files->songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;
                gtk_list_store_append (liststore, &song_iter);

                title = ario_util_format_title (song);
                gtk_list_store_set (liststore, &song_iter,
                                    SONGS_TITLE_COLUMN, title,
                                    SONGS_ARTIST_COLUMN, song->artist,
                                    SONGS_ALBUM_COLUMN, song->album,
                                    SONGS_FILENAME_COLUMN, song->file,
                                    -1);
                g_free (title);
        }
	ario_mpd_free_file_list (files);
        g_free (dir);

        gtk_tree_selection_unselect_all (selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (liststore), &song_iter))
                gtk_tree_selection_select_iter (selection, &song_iter);
}

static void
ario_filesystem_add_playlists (ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        gchar *dir;	GtkTreeIter iter;
        GtkTreeModel *model = GTK_TREE_MODEL (filesystem->priv->filesystem_model);

        if (!gtk_tree_selection_get_selected (filesystem->priv->filesystem_selection,
                                              &model,
                                              &iter))
                return;

        gtk_tree_model_get (GTK_TREE_MODEL (filesystem->priv->filesystem_model), &iter,
                            PLAYLISTS_DIR_COLUMN, &dir, -1);

        g_return_if_fail (dir);

        ario_playlist_append_dir (filesystem->priv->playlist, dir);
        g_free (dir);
}

static void
ario_filesystem_cmd_add_filesystem (GtkAction *action,
                                              ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        ario_filesystem_add_playlists (filesystem);
}

static void
ario_filesystem_popup_menu (ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        menu = gtk_ui_manager_get_widget (filesystem->priv->ui_manager, "/FilesystemPopup");
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                        gtk_get_current_event_time ());
}

static gboolean
ario_filesystem_button_press_cb (GtkWidget *widget,
                                      GdkEventButton *event,
                                      ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (filesystem->priv->dragging)
                return FALSE;

        if (event->button == 1) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        gboolean selected;
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        selected = gtk_tree_selection_path_is_selected (selection, path);
                        if (!selected) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }

                        GdkModifierType mods;
                        int x, y;

                        gdk_window_get_pointer (widget->window, &x, &y, &mods);
                        filesystem->priv->drag_start_x = x;
                        filesystem->priv->drag_start_y = y;
                        filesystem->priv->pressed = TRUE;

                        gtk_tree_path_free (path);
                        return FALSE;
                }
        }

        if (event->button == 3) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        ario_filesystem_popup_menu (filesystem);
                        gtk_tree_path_free (path);
                        return FALSE;
                }
        }

        return FALSE;
}

static gboolean
ario_filesystem_button_release_cb (GtkWidget *widget,
                                        GdkEventButton *event,
                                        ArioFilesystem *filesystem)
{
        ARIO_LOG_FUNCTION_START
        if (!filesystem->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        gtk_tree_selection_unselect_all (selection);
                        gtk_tree_selection_select_path (selection, path);
                        gtk_tree_path_free (path);
                }
        }

        filesystem->priv->dragging = FALSE;
        filesystem->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_filesystem_motion_notify (GtkWidget *widget, 
                                    GdkEventMotion *event,
                                    ArioFilesystem *filesystem)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((filesystem->priv->dragging) || !(filesystem->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - filesystem->priv->drag_start_x;
        dy = y - filesystem->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                filesystem->priv->dragging = TRUE;

        return FALSE;
}

static void
ario_filesystem_playlists_drag_data_get_cb (GtkWidget * widget,
                                                 GdkDragContext * context,
                                                 GtkSelectionData * selection_data,
                                                 guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioFilesystem *filesystem;
        ArioMpdSong *song;
	GtkTreeIter iter;
        GtkTreeModel *model;
        guchar* dir;

        filesystem = ARIO_FILESYSTEM (data);

        g_return_if_fail (IS_ARIO_FILESYSTEM (filesystem));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        model = GTK_TREE_MODEL (filesystem->priv->filesystem_model);

        if (!gtk_tree_selection_get_selected (filesystem->priv->filesystem_selection,
                                              &model,
                                              &iter))
                return;

        gtk_tree_model_get (GTK_TREE_MODEL (filesystem->priv->filesystem_model), &iter,
                            PLAYLISTS_DIR_COLUMN, &dir, -1);

        gtk_selection_data_set (selection_data, selection_data->target, 8, dir,
                                strlen (dir) * sizeof(guchar));
        g_free (dir);
}
