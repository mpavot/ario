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
#include "sources/ario-storedplaylists.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "preferences/ario-preferences.h"

#ifdef ENABLE_STOREDPLAYLISTS

#define DRAG_THRESHOLD 1

static void ario_storedplaylists_class_init (ArioStoredplaylistsClass *klass);
static void ario_storedplaylists_init (ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_finalize (GObject *object);
static void ario_storedplaylists_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void ario_storedplaylists_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);
static void ario_storedplaylists_state_changed_cb (ArioMpd *mpd,
                                         ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_storedplaylists_changed_cb (ArioMpd *mpd,
                                                             ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_add_in_playlist (ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_add_storedplaylists (GtkAction *action,
                                                          ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_delete_storedplaylists (GtkAction *action,
                                                             ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_add_songs (GtkAction *action,
                                                ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_popup_menu (ArioStoredplaylists *storedplaylists);
static gboolean ario_storedplaylists_button_press_cb (GtkWidget *widget,
                                            GdkEventButton *event,
                                            ArioStoredplaylists *storedplaylists);
static gboolean ario_storedplaylists_button_release_cb (GtkWidget *widget,
                                              GdkEventButton *event,
                                              ArioStoredplaylists *storedplaylists);
static gboolean ario_storedplaylists_motion_notify (GtkWidget *widget, 
                                          GdkEventMotion *event,
                                          ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_playlists_selection_drag_foreach (GtkTreeModel *model,
                                                      GtkTreePath *path,
                                                      GtkTreeIter *iter,
                                                      gpointer userdata);
static void ario_storedplaylists_playlists_drag_data_get_cb (GtkWidget * widget,
                                         GdkDragContext * context,
                                         GtkSelectionData * selection_data,
                                         guint info, guint time, gpointer data);
static void ario_storedplaylists_songs_drag_data_get_cb (GtkWidget * widget,
                                         GdkDragContext * context,
                                         GtkSelectionData * selection_data,
                                         guint info, guint time, gpointer data);
static void ario_storedplaylists_playlists_selection_changed_cb (GtkTreeSelection *selection,
                                                                 ArioStoredplaylists *storedplaylists);

struct ArioStoredplaylistsPrivate
{        
        GtkWidget *storedplaylists;
        GtkListStore *storedplaylists_model;
        GtkTreeSelection *storedplaylists_selection;


        GtkWidget *songs;
        GtkListStore *songs_model;
        GtkTreeSelection *songs_selection;

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

static GtkActionEntry ario_storedplaylists_actions [] =
{
        { "StoredplaylistsAddPlaylists", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
          N_("Add to the playlist"),
          G_CALLBACK (ario_storedplaylists_cmd_add_storedplaylists) },
        { "StoredplaylistsAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
          N_("Add to the playlist"),
          G_CALLBACK (ario_storedplaylists_cmd_add_songs) },
        { "StoredplaylistsDelete", GTK_STOCK_DELETE, N_("_Delete"), NULL,
          N_("Delete this playlists"),
          G_CALLBACK (ario_storedplaylists_cmd_delete_storedplaylists) }
};
static guint ario_storedplaylists_n_actions = G_N_ELEMENTS (ario_storedplaylists_actions);

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
        PLAYLISTS_NAME_COLUMN,
        PLAYLISTS_N_COLUMN
};


enum
{
        SONGS_TITLE_COLUMN,
        SONGS_ARTIST_COLUMN,
        SONGS_ALBUM_COLUMN,
        SONGS_FILENAME_COLUMN,
        SONGS_N_COLUMN
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

static GObjectClass *parent_class = NULL;

GType
ario_storedplaylists_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioStoredplaylistsClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_storedplaylists_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioStoredplaylists),
                        0,
                        (GInstanceInitFunc) ario_storedplaylists_init
                };

                type = g_type_register_static (GTK_TYPE_HPANED,
                                               "ArioStoredplaylists",
                                                &our_info, 0);
        }
        return type;
}

static void
ario_storedplaylists_class_init (ArioStoredplaylistsClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_storedplaylists_finalize;

        object_class->set_property = ario_storedplaylists_set_property;
        object_class->get_property = ario_storedplaylists_get_property;

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
ario_storedplaylists_init (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        int pos;
        GtkWidget *scrolledwindow_storedplaylists, *scrolledwindow_songs;

        storedplaylists->priv = g_new0 (ArioStoredplaylistsPrivate, 1);

        storedplaylists->priv->connected = FALSE;

        /* Storedplaylists list */
        scrolledwindow_storedplaylists = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_storedplaylists);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_storedplaylists), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_storedplaylists), GTK_SHADOW_IN);
        storedplaylists->priv->storedplaylists = gtk_tree_view_new ();
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Playlist"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 50);
        gtk_tree_view_append_column (GTK_TREE_VIEW (storedplaylists->priv->storedplaylists), column);
        storedplaylists->priv->storedplaylists_model = gtk_list_store_new (PLAYLISTS_N_COLUMN,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (storedplaylists->priv->storedplaylists_model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (storedplaylists->priv->storedplaylists),
                                 GTK_TREE_MODEL (storedplaylists->priv->storedplaylists_model));
        storedplaylists->priv->storedplaylists_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (storedplaylists->priv->storedplaylists));
        gtk_tree_selection_set_mode (storedplaylists->priv->storedplaylists_selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_storedplaylists), storedplaylists->priv->storedplaylists);

        gtk_drag_source_set (storedplaylists->priv->storedplaylists,
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (storedplaylists->priv->storedplaylists),
                          "drag_data_get", 
                          G_CALLBACK (ario_storedplaylists_playlists_drag_data_get_cb), storedplaylists);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->storedplaylists),
                                 "button_press_event",
                                 G_CALLBACK (ario_storedplaylists_button_press_cb),
                                 storedplaylists,
                                 0);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->storedplaylists),
                                 "button_release_event",
                                 G_CALLBACK (ario_storedplaylists_button_release_cb),
                                 storedplaylists,
                                 0);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->storedplaylists),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_storedplaylists_motion_notify),
                                 storedplaylists,
                                 0);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->storedplaylists_selection),
                                 "changed",
                                 G_CALLBACK (ario_storedplaylists_playlists_selection_changed_cb),
                                 storedplaylists, 0);

        /* Songs list */
        scrolledwindow_songs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_songs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_SHADOW_IN);
        storedplaylists->priv->songs = gtk_tree_view_new ();

        /* Titles */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           renderer,
                                                           "text", SONGS_TITLE_COLUMN,
                                                           NULL);
	gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, SONGS_TITLE_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (storedplaylists->priv->songs), column);

        /* Artists */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           renderer,
                                                           "text", SONGS_ARTIST_COLUMN,
                                                           NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, SONGS_ARTIST_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (storedplaylists->priv->songs), column);

        /* Albums */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                           renderer,
                                                           "text", SONGS_ALBUM_COLUMN,
                                                           NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, SONGS_ALBUM_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (storedplaylists->priv->songs), column);

        storedplaylists->priv->songs_model = gtk_list_store_new (SONGS_N_COLUMN,
                                                                 G_TYPE_STRING,
                                                                 G_TYPE_STRING,
                                                                 G_TYPE_STRING,
                                                                 G_TYPE_STRING);

        gtk_tree_view_set_model (GTK_TREE_VIEW (storedplaylists->priv->songs),
                                 GTK_TREE_MODEL (storedplaylists->priv->songs_model));
        storedplaylists->priv->songs_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (storedplaylists->priv->songs));
        gtk_tree_selection_set_mode (storedplaylists->priv->songs_selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_songs), storedplaylists->priv->songs);

        gtk_drag_source_set (storedplaylists->priv->songs,
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (storedplaylists->priv->songs),
                          "drag_data_get", 
                          G_CALLBACK (ario_storedplaylists_songs_drag_data_get_cb), storedplaylists);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->songs),
                                 "button_press_event",
                                 G_CALLBACK (ario_storedplaylists_button_press_cb),
                                 storedplaylists,
                                 0);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->songs),
                                 "button_release_event",
                                 G_CALLBACK (ario_storedplaylists_button_release_cb),
                                 storedplaylists,
                                 0);
        g_signal_connect_object (G_OBJECT (storedplaylists->priv->songs),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_storedplaylists_motion_notify),
                                 storedplaylists,
                                 0);

        /* Hpaned properties */        gtk_paned_pack1 (GTK_PANED (storedplaylists), scrolledwindow_storedplaylists, FALSE, FALSE);
        gtk_paned_pack2 (GTK_PANED (storedplaylists), scrolledwindow_songs, TRUE, FALSE);

        pos = eel_gconf_get_integer (CONF_PLAYLISTS_HPANED_SIZE, 250);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (storedplaylists),
                                        pos);
}

void
ario_storedplaylists_shutdown (ArioStoredplaylists *storedplaylists)
{
        int pos;
        pos = gtk_paned_get_position (GTK_PANED (storedplaylists));
        if (pos > 0)
                eel_gconf_set_integer (CONF_PLAYLISTS_HPANED_SIZE,
                                       pos);
}

static void
ario_storedplaylists_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (object));

        storedplaylists = ARIO_STOREDPLAYLISTS (object);

        g_return_if_fail (storedplaylists->priv != NULL);

        g_free (storedplaylists->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_storedplaylists_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (object);
        
        switch (prop_id) {
        case PROP_MPD:
                storedplaylists->priv->mpd = g_value_get_object (value);

                /* Signals to synchronize the storedplaylists with mpd */
                g_signal_connect_object (G_OBJECT (storedplaylists->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_storedplaylists_state_changed_cb),
                                         storedplaylists, 0);
                g_signal_connect_object (G_OBJECT (storedplaylists->priv->mpd),
                                         "storedplaylists_changed", G_CALLBACK (ario_storedplaylists_storedplaylists_changed_cb),
                                         storedplaylists, 0);
                break;
        case PROP_PLAYLIST:
                storedplaylists->priv->playlist = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                storedplaylists->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                storedplaylists->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (storedplaylists->priv->actiongroup,
                                              ario_storedplaylists_actions,
                                              ario_storedplaylists_n_actions, storedplaylists);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_storedplaylists_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, storedplaylists->priv->mpd);
                break;
        case PROP_PLAYLIST:
                g_value_set_object (value, storedplaylists->priv->playlist);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, storedplaylists->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, storedplaylists->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_storedplaylists_new (GtkUIManager *mgr,
                  GtkActionGroup *group,
                  ArioMpd *mpd,
                  ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists;

        storedplaylists = g_object_new (TYPE_ARIO_STOREDPLAYLISTS,
                              "ui-manager", mgr,
                              "action-group", group,
                              "mpd", mpd,
                              "playlist", playlist,
                              NULL);

        g_return_val_if_fail (storedplaylists->priv != NULL, NULL);

        return GTK_WIDGET (storedplaylists);
}

static void
ario_storedplaylists_fill_storedplaylists (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter storedplaylists_iter;
        GList *playlists;
        GList *tmp;

        gtk_list_store_clear (storedplaylists->priv->storedplaylists_model);

        if (!storedplaylists->priv->connected)
                return;

        playlists = ario_mpd_get_playlists (storedplaylists->priv->mpd);
        for (tmp = playlists; tmp; tmp = g_list_next (tmp)) {
                gtk_list_store_append (storedplaylists->priv->storedplaylists_model, &storedplaylists_iter);
                gtk_list_store_set (storedplaylists->priv->storedplaylists_model, &storedplaylists_iter, 0,
                                    tmp->data, -1);
        }

        g_list_foreach(playlists, (GFunc) g_free, NULL);
        g_list_free (playlists);

        gtk_tree_selection_unselect_all (storedplaylists->priv->storedplaylists_selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (storedplaylists->priv->storedplaylists_model), &storedplaylists_iter))
                gtk_tree_selection_select_iter (storedplaylists->priv->storedplaylists_selection, &storedplaylists_iter);
}

static void
ario_storedplaylists_playlists_selection_foreach (GtkTreeModel *model,
                                                  GtkTreePath *path,
                                                  GtkTreeIter *iter,
                                                  gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (userdata);
        gchar* playlist = NULL;
        GList *songs = NULL, *temp;
        ArioMpdSong *song;
        GtkTreeIter song_iter;
        gchar *title;

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &playlist, -1);

        if (!playlist)
                return;

        songs = ario_mpd_get_songs_from_playlist (storedplaylists->priv->mpd, playlist);
        g_free (playlist);

        for (temp = songs; temp; temp = g_list_next (temp)) {
                song = temp->data;
                gtk_list_store_append (storedplaylists->priv->songs_model, &song_iter);

                title = ario_util_format_title (song);
                gtk_list_store_set (storedplaylists->priv->songs_model, &song_iter,
                                    SONGS_TITLE_COLUMN, title,
                                    SONGS_ARTIST_COLUMN, song->artist,
                                    SONGS_ALBUM_COLUMN, song->album,
                                    SONGS_FILENAME_COLUMN, song->file,
                                    -1);
                g_free (title);
        }

        g_list_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
        g_list_free (songs);
}

static void
ario_storedplaylists_playlists_selection_update (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter song_iter;

        gtk_list_store_clear (storedplaylists->priv->songs_model);

        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             ario_storedplaylists_playlists_selection_foreach,
                                             storedplaylists);

        gtk_tree_selection_unselect_all (storedplaylists->priv->songs_selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (storedplaylists->priv->songs_model), &song_iter))
                gtk_tree_selection_select_iter (storedplaylists->priv->songs_selection, &song_iter);
}

static void
ario_storedplaylists_playlists_selection_changed_cb (GtkTreeSelection *selection,
                                                     ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_playlists_selection_update (storedplaylists);
}

static void
ario_storedplaylists_state_changed_cb (ArioMpd *mpd,
                                       ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        if (storedplaylists->priv->connected != ario_mpd_is_connected (mpd)) {
                storedplaylists->priv->connected = ario_mpd_is_connected (mpd);
                ario_storedplaylists_fill_storedplaylists (storedplaylists);
        }
}

static void
ario_storedplaylists_storedplaylists_changed_cb (ArioMpd *mpd,
                                                 ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_fill_storedplaylists (storedplaylists);
}

static void
storedplaylists_foreach (GtkTreeModel *model,
                         GtkTreePath *path,
                         GtkTreeIter *iter,
                         gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GList **storedplaylists = (GList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &val, -1);
        *storedplaylists = g_list_append (*storedplaylists, val);
}

static void
ario_storedplaylists_add_playlists (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GList *playlists = NULL;
        GList *songs = NULL;
        GList *tmp;

        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             storedplaylists_foreach,
                                             &playlists);

        for (tmp = playlists; tmp; tmp = g_list_next (tmp)) {
                songs = ario_mpd_get_songs_from_playlist (storedplaylists->priv->mpd, tmp->data);
                ario_playlist_append_mpd_songs (storedplaylists->priv->playlist, songs);

                g_list_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
                g_list_free (songs);
        }

        g_list_foreach (playlists, (GFunc) g_free, NULL);
        g_list_free (playlists);
}

static void
songs_foreach (GtkTreeModel *model,
                GtkTreePath *path,
                GtkTreeIter *iter,
                gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GList **storedplaylists = (GList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, SONGS_FILENAME_COLUMN, &val, -1);

        *storedplaylists = g_list_append (*storedplaylists, val);
}

static void
ario_storedplaylists_add_songs (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GList *songs = NULL;

        gtk_tree_selection_selected_foreach (storedplaylists->priv->songs_selection,
                                             songs_foreach,
                                             &songs);

        ario_playlist_append_songs (storedplaylists->priv->playlist, songs);

        g_list_foreach (songs, (GFunc) g_free, NULL);
        g_list_free (songs);
}

static void
ario_storedplaylists_add_in_playlist (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (storedplaylists->priv->storedplaylists)))
                ario_storedplaylists_add_playlists (storedplaylists);

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (storedplaylists->priv->songs)))
                ario_storedplaylists_add_songs (storedplaylists);
}

static void
ario_storedplaylists_cmd_add_storedplaylists (GtkAction *action,
                                              ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_add_playlists (storedplaylists);
}

static void
ario_storedplaylists_cmd_delete_storedplaylists (GtkAction *action,
                                                 ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;
        GList *playlists = NULL;        GList *tmp;

        dialog = gtk_message_dialog_new (NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_YES_NO,
                                        _("Are you sure that you want to delete all the selected playlists?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             storedplaylists_foreach,
                                             &playlists);

        for (tmp = playlists; tmp; tmp = g_list_next (tmp)) {
        	ario_mpd_delete_playlist (storedplaylists->priv->mpd, tmp->data);
        }

        g_list_foreach (playlists, (GFunc) g_free, NULL);
        g_list_free (playlists);
}

static void
ario_storedplaylists_cmd_add_songs (GtkAction *action,
                                    ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_add_songs (storedplaylists);
}

static void
ario_storedplaylists_popup_menu (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (storedplaylists->priv->storedplaylists))) {
                menu = gtk_ui_manager_get_widget (storedplaylists->priv->ui_manager, "/StoredplaylistsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (storedplaylists->priv->songs))) {
                menu = gtk_ui_manager_get_widget (storedplaylists->priv->ui_manager, "/StoredplaylistsSongsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }
}

static gboolean
ario_storedplaylists_button_press_cb (GtkWidget *widget,
                            GdkEventButton *event,
                            ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (storedplaylists->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
                ario_storedplaylists_add_in_playlist (storedplaylists);

        if (event->button == 1) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
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
                        storedplaylists->priv->drag_start_x = x;
                        storedplaylists->priv->drag_start_y = y;
                        storedplaylists->priv->pressed = TRUE;

                        gtk_tree_path_free (path);
                        if (selected)
                                return TRUE;
                        else
                                return FALSE;
                }
        }

        if (event->button == 3) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        ario_storedplaylists_popup_menu (storedplaylists);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_storedplaylists_button_release_cb (GtkWidget *widget,
                                GdkEventButton *event,
                                ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        if (!storedplaylists->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        gtk_tree_selection_unselect_all (selection);
                        gtk_tree_selection_select_path (selection, path);
                        gtk_tree_path_free (path);
                }
        }

        storedplaylists->priv->dragging = FALSE;
        storedplaylists->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_storedplaylists_motion_notify (GtkWidget *widget, 
                            GdkEventMotion *event,
                            ArioStoredplaylists *storedplaylists)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((storedplaylists->priv->dragging) || !(storedplaylists->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - storedplaylists->priv->drag_start_x;
        dy = y - storedplaylists->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                storedplaylists->priv->dragging = TRUE;

        return FALSE;
}

static void
ario_storedplaylists_playlists_selection_drag_foreach (GtkTreeModel *model,
                                          GtkTreePath *path,
                                          GtkTreeIter *iter,
                                          gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GList **playlists = (GList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &val, -1);

        *playlists = g_list_append (*playlists, val);
}

static void
ario_storedplaylists_playlists_drag_data_get_cb (GtkWidget * widget,
                             GdkDragContext * context,
                             GtkSelectionData * selection_data,
                             guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists;
        ArioMpdSong *song;
        GList *playlists = NULL;
        GList *songs;
        GList *tmp, *tmp2;
        GString *str_playlists;

        storedplaylists = ARIO_STOREDPLAYLISTS (data);

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);


        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             ario_storedplaylists_playlists_selection_drag_foreach,
                                             &playlists);

        str_playlists = g_string_new("");
        for (tmp = playlists; tmp; tmp = g_list_next (tmp)) {
                songs = ario_mpd_get_songs_from_playlist (storedplaylists->priv->mpd, tmp->data);

                for (tmp2 = songs; tmp2; tmp2 = g_list_next (tmp2)) {
                        song = tmp2->data;
                        g_string_append (str_playlists, song->file);
                        g_string_append (str_playlists, "\n");
        	}

                g_list_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
                g_list_free (songs);
        }

        g_list_foreach (playlists, (GFunc) g_free, NULL);
        g_list_free (playlists);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) str_playlists->str,
                                strlen (str_playlists->str) * sizeof(guchar));

        g_string_free (str_playlists, TRUE);
}
static void
ario_storedplaylists_songs_selection_drag_foreach (GtkTreeModel *model,
                                          GtkTreePath *path,
                                          GtkTreeIter *iter,
                                          gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GString *filenames = (GString *) userdata;
        g_return_if_fail (filenames != NULL);

        gchar* val = NULL;

        gtk_tree_model_get (model, iter, SONGS_FILENAME_COLUMN, &val, -1);
        g_string_append (filenames, val);
        g_string_append (filenames, "\n");

        g_free (val);
}


static void
ario_storedplaylists_songs_drag_data_get_cb (GtkWidget * widget,
                             GdkDragContext * context,
                             GtkSelectionData * selection_data,
                             guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists;
        GString* filenames = NULL;

        storedplaylists = ARIO_STOREDPLAYLISTS (data);

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        filenames = g_string_new("");
        gtk_tree_selection_selected_foreach (storedplaylists->priv->songs_selection,
                                             ario_storedplaylists_songs_selection_drag_foreach,
                                             filenames);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) filenames->str,
                                strlen (filenames->str) * sizeof(guchar));
        
        g_string_free (filenames, TRUE);
}

#endif  /* ENABLE_STOREDPLAYLISTS */
