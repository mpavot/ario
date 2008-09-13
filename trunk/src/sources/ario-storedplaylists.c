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
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "sources/ario-storedplaylists.h"
#include "widgets/ario-songlist.h"
#include "shell/ario-shell-songinfos.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "preferences/ario-preferences.h"
#include "widgets/ario-playlist.h"

#ifdef ENABLE_STOREDPLAYLISTS

#define DRAG_THRESHOLD 1

static void ario_storedplaylists_class_init (ArioStoredplaylistsClass *klass);
static void ario_storedplaylists_init (ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_finalize (GObject *object);
static void ario_storedplaylists_shutdown (ArioSource *source);
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
static void ario_storedplaylists_cmd_add_storedplaylists (GtkAction *action,
                                                          ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_add_play_storedplaylists (GtkAction *action,
                                                               ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_clear_add_play_storedplaylists (GtkAction *action,
                                                                     ArioStoredplaylists *storedplaylists);
static void ario_storedplaylists_cmd_delete_storedplaylists (GtkAction *action,
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
static void ario_storedplaylists_playlists_selection_changed_cb (GtkTreeSelection *selection,
                                                                 ArioStoredplaylists *storedplaylists);

struct ArioStoredplaylistsPrivate
{
        GtkWidget *storedplaylists;
        GtkListStore *storedplaylists_model;
        GtkTreeSelection *storedplaylists_selection;

        GtkWidget *songs;
        GtkWidget *paned;

        gboolean connected;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        ArioMpd *mpd;
        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;
};

static GtkActionEntry ario_storedplaylists_actions [] =
{
        { "StoredplaylistsAddPlaylists", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_storedplaylists_cmd_add_storedplaylists) },
        { "StoredplaylistsAddPlayPlaylists", GTK_STOCK_MEDIA_PLAY, N_("Add and _play"), NULL,
                NULL,
                G_CALLBACK (ario_storedplaylists_cmd_add_play_storedplaylists) },
        { "StoredplaylistsClearAddPlayPlaylists", GTK_STOCK_REFRESH, N_("_Replace in playlist"), NULL,
                NULL,
                G_CALLBACK (ario_storedplaylists_cmd_clear_add_play_storedplaylists) },
        { "StoredplaylistsDelete", GTK_STOCK_DELETE, N_("_Delete"), NULL,
                NULL,
                G_CALLBACK (ario_storedplaylists_cmd_delete_storedplaylists) }
};
static guint ario_storedplaylists_n_actions = G_N_ELEMENTS (ario_storedplaylists_actions);

static GtkActionEntry ario_storedplaylists_songs_actions [] =
{
        { "StoredplaylistsAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_add_songlists) },
        { "StoredplaylistsAddPlaySongs", GTK_STOCK_MEDIA_PLAY, N_("Add and _play"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_add_play_songlists) },
        { "StoredplaylistsClearAddPlaySongs", GTK_STOCK_REFRESH, N_("_Replace in playlist"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_clear_add_play_songlists) },
        { "StoredplaylistsSongsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_songs_properties) }
};
static guint ario_storedplaylists_n_songs_actions = G_N_ELEMENTS (ario_storedplaylists_songs_actions);

enum
{
        PROP_0,
        PROP_MPD,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
};

enum
{
        PLAYLISTS_NAME_COLUMN,
        PLAYLISTS_N_COLUMN
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

                type = g_type_register_static (ARIO_TYPE_SOURCE,
                                               "ArioStoredplaylists",
                                               &our_info, 0);
        }
        return type;
}

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
        return GTK_STOCK_INDEX;
}

static void
ario_storedplaylists_class_init (ArioStoredplaylistsClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_storedplaylists_finalize;

        object_class->set_property = ario_storedplaylists_set_property;
        object_class->get_property = ario_storedplaylists_get_property;

        source_class->get_id = ario_storedplaylists_get_id;
        source_class->get_name = ario_storedplaylists_get_name;
        source_class->get_icon = ario_storedplaylists_get_icon;
        source_class->shutdown = ario_storedplaylists_shutdown;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
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
        GtkWidget *scrolledwindow_storedplaylists;

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
        g_signal_connect (storedplaylists->priv->storedplaylists,
                          "button_press_event",
                          G_CALLBACK (ario_storedplaylists_button_press_cb),
                          storedplaylists);
        g_signal_connect (storedplaylists->priv->storedplaylists,
                          "button_release_event",
                          G_CALLBACK (ario_storedplaylists_button_release_cb),
                          storedplaylists);
        g_signal_connect (storedplaylists->priv->storedplaylists,
                          "motion_notify_event",
                          G_CALLBACK (ario_storedplaylists_motion_notify),
                          storedplaylists);
        g_signal_connect (storedplaylists->priv->storedplaylists_selection,
                          "changed",
                          G_CALLBACK (ario_storedplaylists_playlists_selection_changed_cb),
                          storedplaylists);

        /* Hpaned properties */
        storedplaylists->priv->paned = gtk_hpaned_new ();
        gtk_paned_pack1 (GTK_PANED (storedplaylists->priv->paned), scrolledwindow_storedplaylists, FALSE, FALSE);

        pos = ario_conf_get_integer (PREF_PLAYLISTS_HPANED_SIZE, PREF_PLAYLISTS_HPANED_SIZE_DEFAULT);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (storedplaylists->priv->paned),
                                        pos);

        gtk_box_pack_start (GTK_BOX (storedplaylists), storedplaylists->priv->paned, TRUE, TRUE, 0);
}

void
ario_storedplaylists_shutdown (ArioSource *source)
{
        ArioStoredplaylists *storedplaylists = ARIO_STOREDPLAYLISTS (source);
        int pos;

        pos = gtk_paned_get_position (GTK_PANED (storedplaylists->priv->paned));
        if (pos > 0)
                ario_conf_set_integer (PREF_PLAYLISTS_HPANED_SIZE,
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
                g_signal_connect (storedplaylists->priv->mpd,
                                  "state_changed", G_CALLBACK (ario_storedplaylists_state_changed_cb),
                                  storedplaylists);
                g_signal_connect (storedplaylists->priv->mpd,
                                  "storedplaylists_changed", G_CALLBACK (ario_storedplaylists_storedplaylists_changed_cb),
                                  storedplaylists);
                break;
        case PROP_UI_MANAGER:
                storedplaylists->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                storedplaylists->priv->actiongroup = g_value_get_object (value);
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
                          ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioStoredplaylists *storedplaylists;
        GtkWidget *scrolledwindow_songs;

        storedplaylists = g_object_new (TYPE_ARIO_STOREDPLAYLISTS,
                                        "ui-manager", mgr,
                                        "action-group", group,
                                        "mpd", mpd,
                                        NULL);

        g_return_val_if_fail (storedplaylists->priv != NULL, NULL);

        /* Songs list */
        scrolledwindow_songs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_songs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_SHADOW_IN);
        storedplaylists->priv->songs = ario_songlist_new (mgr,
                                                          mpd,
                                                          "/StoredplaylistsSongsPopup",
                                                          FALSE);
        gtk_paned_pack2 (GTK_PANED (storedplaylists->priv->paned), scrolledwindow_songs, TRUE, FALSE);

        gtk_container_add (GTK_CONTAINER (scrolledwindow_songs), storedplaylists->priv->songs);

        gtk_action_group_add_actions (storedplaylists->priv->actiongroup,
                                      ario_storedplaylists_actions,
                                      ario_storedplaylists_n_actions, storedplaylists);
        gtk_action_group_add_actions (storedplaylists->priv->actiongroup,
                                      ario_storedplaylists_songs_actions,
                                      ario_storedplaylists_n_songs_actions, storedplaylists->priv->songs);

        return GTK_WIDGET (storedplaylists);
}

static void
ario_storedplaylists_fill_storedplaylists (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter storedplaylists_iter;
        GSList *playlists;
        GSList *tmp;

        gtk_list_store_clear (storedplaylists->priv->storedplaylists_model);

        if (!storedplaylists->priv->connected)
                return;

        playlists = ario_mpd_get_playlists (storedplaylists->priv->mpd);
        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                gtk_list_store_append (storedplaylists->priv->storedplaylists_model, &storedplaylists_iter);
                gtk_list_store_set (storedplaylists->priv->storedplaylists_model, &storedplaylists_iter, 0,
                                    tmp->data, -1);
        }

        g_slist_foreach(playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);

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
        GSList *songs = NULL, *temp;
        ArioMpdSong *song;
        GtkTreeIter song_iter;
        gchar *title;
        GtkListStore *liststore = ario_songlist_get_liststore (ARIO_SONGLIST (storedplaylists->priv->songs));

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &playlist, -1);

        if (!playlist)
                return;

        songs = ario_mpd_get_songs_from_playlist (storedplaylists->priv->mpd, playlist);
        g_free (playlist);

        for (temp = songs; temp; temp = g_slist_next (temp)) {
                song = temp->data;
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

        g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
        g_slist_free (songs);
}

static void
ario_storedplaylists_playlists_selection_update (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter song_iter;
        GtkListStore *liststore = ario_songlist_get_liststore (ARIO_SONGLIST (storedplaylists->priv->songs));
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (storedplaylists->priv->songs));

        gtk_list_store_clear (liststore);

        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             ario_storedplaylists_playlists_selection_foreach,
                                             storedplaylists);

        gtk_tree_selection_unselect_all (selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (liststore), &song_iter))
                gtk_tree_selection_select_iter (selection, &song_iter);
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
        GSList **storedplaylists = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &val, -1);
        *storedplaylists = g_slist_append (*storedplaylists, val);
}

static void
ario_storedplaylists_add_playlists (ArioStoredplaylists *storedplaylists,
                                    gboolean play)
{
        ARIO_LOG_FUNCTION_START
        GSList *playlists = NULL;
        GSList *songs = NULL;
        GSList *tmp;

        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             storedplaylists_foreach,
                                             &playlists);

        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                songs = ario_mpd_get_songs_from_playlist (storedplaylists->priv->mpd, tmp->data);
                ario_playlist_append_mpd_songs (songs, play);

                g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
                g_slist_free (songs);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);
}

static void
ario_storedplaylists_clear_add_play_playlists (ArioStoredplaylists *storedplaylists)
{
        ario_mpd_clear(storedplaylists->priv->mpd);
        ario_storedplaylists_add_playlists(storedplaylists, TRUE);
}

static void
ario_storedplaylists_cmd_add_storedplaylists (GtkAction *action,
                                              ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_add_playlists (storedplaylists, FALSE);
}

static void
ario_storedplaylists_cmd_add_play_storedplaylists (GtkAction *action,
                                                   ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_add_playlists (storedplaylists, TRUE);
}

static void
ario_storedplaylists_cmd_clear_add_play_storedplaylists (GtkAction *action,
                                                         ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        ario_storedplaylists_clear_add_play_playlists (storedplaylists);
}

static void
ario_storedplaylists_cmd_delete_storedplaylists (GtkAction *action,
                                                 ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;
        GSList *playlists = NULL;        GSList *tmp;

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

        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                ario_mpd_delete_playlist (storedplaylists->priv->mpd, tmp->data);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);
}

static void
ario_storedplaylists_popup_menu (ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        menu = gtk_ui_manager_get_widget (storedplaylists->priv->ui_manager, "/StoredplaylistsPopup");
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                        gtk_get_current_event_time ());
}

static gboolean
ario_storedplaylists_button_press_cb (GtkWidget *widget,
                                      GdkEventButton *event,
                                      ArioStoredplaylists *storedplaylists)
{
        ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y;
        gboolean selected;

        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (storedplaylists->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                ario_storedplaylists_add_playlists (storedplaylists, FALSE);
                return FALSE;
        }

        if (event->button == 1) {
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                storedplaylists->priv->drag_start_x = x;
                storedplaylists->priv->drag_start_y = y;
                storedplaylists->priv->pressed = TRUE;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        selected = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), path);
                        gtk_tree_path_free (path);

                        return selected;
                }

                return TRUE;
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
        GSList **playlists = (GSList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, PLAYLISTS_NAME_COLUMN, &val, -1);

        *playlists = g_slist_append (*playlists, val);
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
        GSList *playlists = NULL;
        GSList *songs;
        GSList *tmp, *tmp2;
        GString *str_playlists;

        storedplaylists = ARIO_STOREDPLAYLISTS (data);

        g_return_if_fail (IS_ARIO_STOREDPLAYLISTS (storedplaylists));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);


        gtk_tree_selection_selected_foreach (storedplaylists->priv->storedplaylists_selection,
                                             ario_storedplaylists_playlists_selection_drag_foreach,
                                             &playlists);

        str_playlists = g_string_new("");
        for (tmp = playlists; tmp; tmp = g_slist_next (tmp)) {
                songs = ario_mpd_get_songs_from_playlist (storedplaylists->priv->mpd, tmp->data);

                for (tmp2 = songs; tmp2; tmp2 = g_slist_next (tmp2)) {
                        song = tmp2->data;
                        g_string_append (str_playlists, song->file);
                        g_string_append (str_playlists, "\n");
                }

                g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
                g_slist_free (songs);
        }

        g_slist_foreach (playlists, (GFunc) g_free, NULL);
        g_slist_free (playlists);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) str_playlists->str,
                                strlen (str_playlists->str) * sizeof(guchar));

        g_string_free (str_playlists, TRUE);
}

#endif  /* ENABLE_STOREDPLAYLISTS */
