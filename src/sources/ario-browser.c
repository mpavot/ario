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
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "sources/ario-browser.h"
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "shell/ario-shell-coverselect.h"
#include "shell/ario-shell-coverdownloader.h"
#include "shell/ario-shell-songinfos.h"
#include "preferences/ario-preferences.h"
#include "covers/ario-cover-handler.h"
#include "ario-debug.h"

#define DRAG_THRESHOLD 1

static void ario_browser_class_init (ArioBrowserClass *klass);
static void ario_browser_init (ArioBrowser *browser);
static void ario_browser_finalize (GObject *object);
static void ario_browser_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void ario_browser_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);
static void ario_browser_state_changed_cb (ArioMpd *mpd,
                                           ArioBrowser *browser);
static void ario_browser_dbtime_changed_cb (ArioMpd *mpd,
                                            ArioBrowser *browser);
static void ario_browser_fill_artists (ArioBrowser *browser);
static void ario_browser_artists_selection_changed_cb (GtkTreeSelection * selection,
                                                       ArioBrowser *browser);
static void ario_browser_albums_selection_changed_cb (GtkTreeSelection * selection,
                                                      ArioBrowser *browser);
static void ario_browser_artists_drag_data_get_cb (GtkWidget * widget,
                                                   GdkDragContext * context,
                                                   GtkSelectionData * selection_data,
                                                   guint info, guint time, gpointer data);
static void ario_browser_albums_drag_data_get_cb (GtkWidget * widget,
                                                  GdkDragContext * context,
                                                  GtkSelectionData * selection_data,
                                                  guint info, guint time, gpointer data);
static void ario_browser_songs_drag_data_get_cb (GtkWidget * widget,
                                                 GdkDragContext * context,
                                                 GtkSelectionData * selection_data,
                                                 guint info, guint time, gpointer data);
static gboolean ario_browser_button_press_cb (GtkWidget *widget,
                                              GdkEventButton *event,
                                              ArioBrowser *browser);
static gboolean ario_browser_button_release_cb (GtkWidget *widget,
                                                GdkEventButton *event,
                                                ArioBrowser *browser);
static gboolean ario_browser_motion_notify (GtkWidget *widget, 
                                            GdkEventMotion *event,
                                            ArioBrowser *browser);
static void ario_browser_cmd_add_artists (GtkAction *action,
                                          ArioBrowser *browser);
static void ario_browser_cmd_add_albums (GtkAction *action,
                                         ArioBrowser *browser);
static void ario_browser_cmd_add_songs (GtkAction *action,
                                        ArioBrowser *browser);
static void ario_browser_cmd_albums_properties (GtkAction *action,
                                                ArioBrowser *browser);
static void ario_browser_cmd_songs_properties (GtkAction *action,
                                               ArioBrowser *browser);                              
static void ario_browser_add_in_playlist (ArioBrowser *browser);
static void ario_browser_cmd_get_artist_cover (GtkAction *action,
                                               ArioBrowser *browser);
static void ario_browser_cmd_remove_artist_cover (GtkAction *action,
                                                  ArioBrowser *browser);
static void ario_browser_cmd_get_album_cover (GtkAction *action,
                                              ArioBrowser *browser);
static void ario_browser_cmd_remove_album_cover (GtkAction *action,
                                                 ArioBrowser *browser);
static void ario_browser_covertree_visible_changed_cb (guint notification_id,
                                                       ArioBrowser *browser);
static void ario_browser_cover_changed_cb (ArioCoverHandler *cover_handler,
                                           ArioBrowser *browser);

struct ArioBrowserPrivate
{        
        GtkWidget *artists;
        GtkWidget *albums;
        GtkWidget *songs;

        GtkListStore *artists_model;
        GtkListStore *albums_model;
        GtkListStore *songs_model;

        GtkTreeSelection *artists_selection;
        GtkTreeSelection *albums_selection;
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

static GtkActionEntry ario_browser_actions [] =
{
        { "BrowserAddArtists", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_add_artists) },
        { "BrowserAddAlbums", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_add_albums) },
        { "BrowserAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_add_songs) },

        { "BrowserAlbumsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_albums_properties) },              
        { "BrowserSongsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_songs_properties) },

        { "BrowserGetCoverArtist", GTK_STOCK_CDROM, N_("Get the covers"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_get_artist_cover) },
        { "BrowserRemoveCoverArtist", GTK_STOCK_DELETE, N_("_Delete the covers"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_remove_artist_cover) },

        { "BrowserGetCoverAlbum", GTK_STOCK_CDROM, N_("Get the covers"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_get_album_cover) },
        { "BrowserRemoveCoverAlbum", GTK_STOCK_DELETE, N_("_Delete the covers"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_remove_album_cover) },
};
static guint ario_browser_n_actions = G_N_ELEMENTS (ario_browser_actions);

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
        ALBUM_ALBUM_COLUMN,
        ALBUM_ARTIST_COLUMN,
        ALBUM_COVER_COLUMN,
        ALBUM_PATH_COLUMN,
        ALBUM_N_COLUMN
};

enum
{
        TRACK_COLUMN,
        TITLE_COLUMN,
        FILENAME_COLUMN,
        N_COLUMN
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

static const GtkTargetEntry albums_targets  [] = {
        { "text/albums-list", 0, 0 },
};

static GObjectClass *parent_class = NULL;

GType
ario_browser_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioBrowserClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_browser_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioBrowser),
                        0,
                        (GInstanceInitFunc) ario_browser_init
                };

                type = g_type_register_static (ARIO_TYPE_SOURCE,
                                               "ArioBrowser",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_browser_get_id (ArioSource *source)
{
        return "library";
}

static gchar *
ario_browser_get_name (ArioSource *source)
{
        return _("Library");
}

static gchar *
ario_browser_get_icon (ArioSource *source)
{
        return GTK_STOCK_HOME;
}

static void
ario_browser_class_init (ArioBrowserClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_browser_finalize;

        object_class->set_property = ario_browser_set_property;
        object_class->get_property = ario_browser_get_property;

        source_class->get_id = ario_browser_get_id;
        source_class->get_name = ario_browser_get_name;
        source_class->get_icon = ario_browser_get_icon;

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
ario_browser_init (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        GtkWidget *scrolledwindow_artists;
        GtkWidget *scrolledwindow_albums;
        GtkWidget *scrolledwindow_songs;

        browser->priv = g_new0 (ArioBrowserPrivate, 1);

        browser->priv->connected = FALSE;

        /* Artists list */
        scrolledwindow_artists = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_artists);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_artists), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_artists), GTK_SHADOW_IN);
        browser->priv->artists = gtk_tree_view_new ();
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->artists), column);
        browser->priv->artists_model = gtk_list_store_new (1, G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (browser->priv->artists_model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->priv->artists),
                                 GTK_TREE_MODEL (browser->priv->artists_model));
        browser->priv->artists_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->priv->artists));
        gtk_tree_selection_set_mode (browser->priv->artists_selection,
                                     GTK_SELECTION_MULTIPLE);
        g_signal_connect_object (G_OBJECT (browser->priv->artists_selection),
                                 "changed",
                                 G_CALLBACK (ario_browser_artists_selection_changed_cb),
                                 browser, 0);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_artists), browser->priv->artists);

        gtk_drag_source_set (browser->priv->artists,
                             GDK_BUTTON1_MASK,
                             albums_targets,
                             G_N_ELEMENTS (albums_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (browser->priv->artists),
                          "drag_data_get", 
                          G_CALLBACK (ario_browser_artists_drag_data_get_cb), browser);
        g_signal_connect_object (G_OBJECT (browser->priv->artists),
                                 "button_press_event",
                                 G_CALLBACK (ario_browser_button_press_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->artists),
                                 "button_release_event",
                                 G_CALLBACK (ario_browser_button_release_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->artists),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_browser_motion_notify),
                                 browser,
                                 0);

        /* Albums list */
        scrolledwindow_albums = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_albums);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_albums), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_albums), GTK_SHADOW_IN);
        browser->priv->albums = gtk_tree_view_new ();
        //gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (browser->priv->albums), TRUE);
        /* Text column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                           renderer,
                                                           "text", ALBUM_ALBUM_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->albums), column);
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
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->albums), 
                                     column);
        gtk_tree_view_column_set_visible (column,
                                          !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));
        /* Model */
        browser->priv->albums_model = gtk_list_store_new (ALBUM_N_COLUMN,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          GDK_TYPE_PIXBUF,
                                                          G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (browser->priv->albums_model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->priv->albums),
                                 GTK_TREE_MODEL (browser->priv->albums_model));
        browser->priv->albums_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->priv->albums));
        gtk_tree_selection_set_mode (browser->priv->albums_selection,
                                     GTK_SELECTION_MULTIPLE);
        g_signal_connect_object (G_OBJECT (browser->priv->albums_selection),
                                 "changed",
                                 G_CALLBACK (ario_browser_albums_selection_changed_cb),
                                 browser, 0);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_albums), browser->priv->albums);

        gtk_drag_source_set (browser->priv->albums,
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (browser->priv->albums),
                          "drag_data_get", 
                          G_CALLBACK (ario_browser_albums_drag_data_get_cb), browser);
        g_signal_connect_object (G_OBJECT (browser->priv->albums),
                                 "button_press_event",
                                 G_CALLBACK (ario_browser_button_press_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->albums),
                                 "button_release_event",
                                 G_CALLBACK (ario_browser_button_release_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->albums),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_browser_motion_notify),
                                 browser,
                                 0);

        ario_conf_notification_add (PREF_COVER_TREE_HIDDEN,
                                    (ArioNotifyFunc) ario_browser_covertree_visible_changed_cb,
                                    browser);

        /* Songs list */
        scrolledwindow_songs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_songs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_SHADOW_IN);
        browser->priv->songs = gtk_tree_view_new ();
        /* Track column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Track"),
                                                           renderer,
                                                           "text", TRACK_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->songs), column);
        /* Title column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           renderer,
                                                           "text", TITLE_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->songs), column);

        browser->priv->songs_model = gtk_list_store_new (N_COLUMN,
                                                         G_TYPE_STRING,
                                                         G_TYPE_STRING,
                                                         G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->priv->songs),
                                 GTK_TREE_MODEL (browser->priv->songs_model));
        browser->priv->songs_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->priv->songs));
        gtk_tree_selection_set_mode (browser->priv->songs_selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_songs), browser->priv->songs);

        gtk_drag_source_set (browser->priv->songs,
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (browser->priv->songs),
                          "drag_data_get", 
                          G_CALLBACK (ario_browser_songs_drag_data_get_cb), browser);
        g_signal_connect_object (G_OBJECT (browser->priv->songs),
                                 "button_press_event",
                                 G_CALLBACK (ario_browser_button_press_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->songs),
                                 "button_release_event",
                                 G_CALLBACK (ario_browser_button_release_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->songs),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_browser_motion_notify),
                                 browser,
                                 0);

        gtk_tree_selection_set_mode (browser->priv->songs_selection,
                                     GTK_SELECTION_MULTIPLE);
        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (browser), TRUE);
        gtk_box_set_spacing (GTK_BOX (browser), 4);

        gtk_box_pack_start (GTK_BOX (browser), scrolledwindow_artists, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (browser), scrolledwindow_albums, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (browser), scrolledwindow_songs, TRUE, TRUE, 0);

        g_signal_connect_object (G_OBJECT (ario_cover_handler_get_instance ()),
                                 "cover_changed", G_CALLBACK (ario_browser_cover_changed_cb),
                                 browser, 0);
}

static void
ario_browser_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_BROWSER (object));

        browser = ARIO_BROWSER (object);

        g_return_if_fail (browser->priv != NULL);

        g_free (browser->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_browser_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser = ARIO_BROWSER (object);

        switch (prop_id) {
        case PROP_MPD:
                browser->priv->mpd = g_value_get_object (value);

                /* Signals to synchronize the browser with mpd */
                g_signal_connect_object (G_OBJECT (browser->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_browser_state_changed_cb),
                                         browser, 0);

                g_signal_connect_object (G_OBJECT (browser->priv->mpd),
                                         "updatingdb_changed", G_CALLBACK (ario_browser_dbtime_changed_cb),
                                         browser, 0);
                break;
        case PROP_PLAYLIST:
                browser->priv->playlist = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                browser->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                browser->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (browser->priv->actiongroup,
                                              ario_browser_actions,
                                              ario_browser_n_actions, browser);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_browser_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser = ARIO_BROWSER (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, browser->priv->mpd);
                break;
        case PROP_PLAYLIST:
                g_value_set_object (value, browser->priv->playlist);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, browser->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, browser->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_browser_new (GtkUIManager *mgr,
                  GtkActionGroup *group,
                  ArioMpd *mpd,
                  ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser;

        browser = g_object_new (TYPE_ARIO_BROWSER,
                                "ui-manager", mgr,
                                "action-group", group,
                                "mpd", mpd,
                                "playlist", playlist,
                                NULL);

        g_return_val_if_fail (browser->priv != NULL, NULL);

        return GTK_WIDGET (browser);
}

static void
ario_browser_state_changed_cb (ArioMpd *mpd,
                               ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->connected != ario_mpd_is_connected (mpd))
                ario_browser_fill_artists (browser);

        browser->priv->connected = ario_mpd_is_connected (mpd);
}

static void
ario_browser_dbtime_changed_cb (ArioMpd *mpd,
                                ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (!ario_mpd_get_updating (browser->priv->mpd))
                ario_browser_fill_artists (browser);
}

static void
ario_browser_fill_artists (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GSList *artists;
        GSList *temp;
        GtkTreeIter artist_iter;
        GList* paths;
        GtkTreePath *path;
        GtkTreeModel *models = GTK_TREE_MODEL (browser->priv->artists_model);

        g_signal_handlers_block_by_func (G_OBJECT (browser->priv->artists_selection),
                                         G_CALLBACK (ario_browser_artists_selection_changed_cb),
                                         browser);

        paths = gtk_tree_selection_get_selected_rows (browser->priv->artists_selection, &models);

        gtk_list_store_clear (browser->priv->artists_model);

        artists = ario_mpd_get_artists (browser->priv->mpd);

        for (temp = artists; temp; temp = g_slist_next (temp)) {
                gtk_list_store_append (browser->priv->artists_model, &artist_iter);
                gtk_list_store_set (browser->priv->artists_model, &artist_iter, 0,
                                    temp->data, -1);
        }
        g_slist_foreach (artists, (GFunc) g_free, NULL);
        g_slist_free (artists);

        gtk_tree_selection_unselect_all (browser->priv->artists_selection);
        if (paths) {
                path = paths->data;
                if (path) {
                        gtk_tree_selection_select_path (browser->priv->artists_selection, path);
                }
        } else {
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->priv->artists_model), &artist_iter))
                        gtk_tree_selection_select_iter (browser->priv->artists_selection, &artist_iter);
        }

        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        g_signal_handlers_unblock_by_func (G_OBJECT (browser->priv->artists_selection),
                                           G_CALLBACK (ario_browser_artists_selection_changed_cb),
                                           browser);

        g_signal_emit_by_name (G_OBJECT (browser->priv->artists_selection), "changed", 0);
}

static void
ario_browser_artists_selection_foreach (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser = ARIO_BROWSER (userdata);
        gchar* artist = NULL;
        GSList *albums = NULL, *temp;
        ArioMpdAlbum *ario_mpd_album;
        GtkTreeIter album_iter;
        gchar *ario_cover_path;
        GdkPixbuf *cover;

        g_return_if_fail (IS_ARIO_BROWSER (browser));

        gtk_tree_model_get (model, iter, 0, &artist, -1);

        if (!artist)
                return;

        albums = ario_mpd_get_albums (browser->priv->mpd, artist);
        g_free (artist);

        for (temp = albums; temp; temp = g_slist_next (temp)) {
                ario_mpd_album = temp->data;

                ario_cover_path = ario_cover_make_ario_cover_path (ario_mpd_album->artist, ario_mpd_album->album, SMALL_COVER);

                /* The small cover exists, we show it */
                cover = gdk_pixbuf_new_from_file_at_size (ario_cover_path, COVER_SIZE, COVER_SIZE, NULL);
                g_free (ario_cover_path);

                if (!GDK_IS_PIXBUF (cover)) {
                        /* There is no cover, we show a transparent picture */
                        cover = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COVER_SIZE, COVER_SIZE);
                        gdk_pixbuf_fill (cover, 0);
                }

                gtk_list_store_append (browser->priv->albums_model, &album_iter);

                gtk_list_store_set (browser->priv->albums_model, &album_iter,
                                    ALBUM_ALBUM_COLUMN, ario_mpd_album->album,
                                    ALBUM_ARTIST_COLUMN, ario_mpd_album->artist,
                                    ALBUM_COVER_COLUMN, cover,
                                    ALBUM_PATH_COLUMN, ario_mpd_album->path,
                                    -1);
                g_object_unref (G_OBJECT (cover));
        }

        g_slist_foreach (albums, (GFunc) ario_mpd_free_album, NULL);
        g_slist_free (albums);
}

static void
ario_browser_artists_selection_update (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter album_iter;

        g_signal_handlers_block_by_func (G_OBJECT (browser->priv->albums_selection),
                                         G_CALLBACK (ario_browser_albums_selection_changed_cb),
                                         browser);

        gtk_list_store_clear (browser->priv->albums_model);

        gtk_tree_selection_selected_foreach (browser->priv->artists_selection,
                                             ario_browser_artists_selection_foreach,
                                             browser);

        gtk_tree_selection_unselect_all (browser->priv->albums_selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->priv->albums_model), &album_iter))
                gtk_tree_selection_select_iter (browser->priv->albums_selection, &album_iter);
        g_signal_handlers_unblock_by_func (G_OBJECT (browser->priv->albums_selection),
                                           G_CALLBACK (ario_browser_albums_selection_changed_cb),
                                           browser);

        g_signal_emit_by_name (G_OBJECT (browser->priv->albums_selection), "changed", 0);
}

static void
ario_browser_artists_selection_changed_cb (GtkTreeSelection *selection,
                                           ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_artists_selection_update (browser);
}

static void
ario_browser_albums_selection_foreach (GtkTreeModel *model,
                                       GtkTreePath *path,
                                       GtkTreeIter *iter,
                                       gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser = ARIO_BROWSER (userdata);
        gchar *artist = NULL, *album = NULL;
        GSList *songs = NULL, *temp;
        ArioMpdSong *song;
        GtkTreeIter songs_iter;
        gchar *track;
        gchar *title;

        g_return_if_fail (IS_ARIO_BROWSER (browser));

        gtk_tree_model_get (model, iter,
                            ALBUM_ARTIST_COLUMN, &artist,
                            ALBUM_ALBUM_COLUMN, &album, -1);

        if (!artist || !album)
                return;

        songs = ario_mpd_get_songs (browser->priv->mpd, artist, album);
        g_free (artist);
        g_free (album);

        for (temp = songs; temp; temp = g_slist_next (temp)) {
                song = temp->data;
                gtk_list_store_append (browser->priv->songs_model, &songs_iter);

                track = ario_util_format_track (song->track);
                title = ario_util_format_title (song);
                gtk_list_store_set (browser->priv->songs_model, &songs_iter,
                                    TRACK_COLUMN, track,
                                    TITLE_COLUMN, title,
                                    FILENAME_COLUMN, song->file,
                                    -1);
                g_free (title);
                g_free (track);
        }

        g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
        g_slist_free (songs);
}

static void
ario_browser_albums_selection_update (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter song_iter;

        gtk_list_store_clear (browser->priv->songs_model);

        gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                             ario_browser_albums_selection_foreach,
                                             browser);

        gtk_tree_selection_unselect_all (browser->priv->songs_selection);

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->priv->songs_model), &song_iter))
                gtk_tree_selection_select_iter (browser->priv->songs_selection, &song_iter);
}

static void
ario_browser_albums_selection_changed_cb (GtkTreeSelection * selection,
                                          ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_albums_selection_update (browser);
}

static void
ario_browser_albums_selection_drag_foreach (GtkTreeModel *model,
                                            GtkTreePath *path,
                                            GtkTreeIter *iter,
                                            gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GString *albums = (GString *) userdata;
        gchar* val = NULL;
        g_return_if_fail (albums != NULL);

        gtk_tree_model_get (model, iter, ALBUM_ARTIST_COLUMN, &val, -1);
        g_string_append (albums, val);
        g_string_append (albums, "\n");
        g_free (val);

        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &val, -1);
        g_string_append (albums, val);
        g_string_append (albums, "\n");
        g_free (val);
}

static gboolean
ario_browser_albums_drag_foreach (GtkTreeModel *model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_albums_selection_drag_foreach (model,
                                                    path,
                                                    iter,
                                                    userdata);
        return FALSE;
}

static void
ario_browser_artists_drag_data_get_cb (GtkWidget * widget,
                                       GdkDragContext * context,
                                       GtkSelectionData * selection_data,
                                       guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser;
        GString* albums = NULL;

        browser = ARIO_BROWSER (data);

        g_return_if_fail (IS_ARIO_BROWSER (browser));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        albums = g_string_new("");
        gtk_tree_model_foreach (GTK_TREE_MODEL (browser->priv->albums_model),
                                ario_browser_albums_drag_foreach,
                                albums);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) albums->str,
                                strlen (albums->str) * sizeof(guchar));

        g_string_free (albums, TRUE);
}

static void
ario_browser_songs_selection_drag_foreach (GtkTreeModel *model,
                                           GtkTreePath *path,
                                           GtkTreeIter *iter,
                                           gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GString *filenames = (GString *) userdata;
        gchar* val = NULL;
        g_return_if_fail (filenames != NULL);

        gtk_tree_model_get (model, iter, FILENAME_COLUMN, &val, -1);
        g_string_append (filenames, val);
        g_string_append (filenames, "\n");

        g_free (val);
}

static gboolean
ario_browser_songs_drag_foreach (GtkTreeModel *model,
                                 GtkTreePath *path,
                                 GtkTreeIter *iter,
                                 gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_songs_selection_drag_foreach (model,
                                                   path,
                                                   iter,
                                                   userdata);
        return FALSE;
}

static void
ario_browser_albums_drag_data_get_cb (GtkWidget * widget,
                                      GdkDragContext * context,
                                      GtkSelectionData * selection_data,
                                      guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser;
        GString* songs = NULL;

        browser = ARIO_BROWSER (data);

        g_return_if_fail (IS_ARIO_BROWSER (browser));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        songs = g_string_new("");
        gtk_tree_model_foreach (GTK_TREE_MODEL (browser->priv->songs_model),
                                ario_browser_songs_drag_foreach,
                                songs);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) songs->str,
                                strlen (songs->str) * sizeof(guchar));

        g_string_free (songs, TRUE);
}

static void
ario_browser_songs_drag_data_get_cb (GtkWidget * widget,
                                     GdkDragContext * context,
                                     GtkSelectionData * selection_data,
                                     guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser;
        GString* filenames = NULL;

        browser = ARIO_BROWSER (data);

        g_return_if_fail (IS_ARIO_BROWSER (browser));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        filenames = g_string_new("");
        gtk_tree_selection_selected_foreach (browser->priv->songs_selection,
                                             ario_browser_songs_selection_drag_foreach,
                                             filenames);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) filenames->str,
                                strlen (filenames->str) * sizeof(guchar));

        g_string_free (filenames, TRUE);
}

static void
ario_browser_popup_menu (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->artists))) {
                menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserArtistsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->albums))) {
                if (gtk_tree_selection_count_selected_rows (browser->priv->albums_selection) == 1)
                        menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserAlbumsPopupSingle");
                else
                        menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserAlbumsPopupMultiple");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->songs))) {
                menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserSongsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }
}

static gboolean
ario_browser_button_press_cb (GtkWidget *widget,
                              GdkEventButton *event,
                              ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y;
        gboolean selected;

        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (browser->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                ario_browser_add_in_playlist (browser);
                return FALSE;
        }

        if (event->button == 1) {
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                browser->priv->drag_start_x = x;
                browser->priv->drag_start_y = y;
                browser->priv->pressed = TRUE;

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
                        ario_browser_popup_menu (browser);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_browser_button_release_cb (GtkWidget *widget,
                                GdkEventButton *event,
                                ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (!browser->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
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

        browser->priv->dragging = FALSE;
        browser->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_browser_motion_notify (GtkWidget *widget, 
                            GdkEventMotion *event,
                            ArioBrowser *browser)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((browser->priv->dragging) || !(browser->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - browser->priv->drag_start_x;
        dy = y - browser->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                browser->priv->dragging = TRUE;

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

        gtk_tree_model_get (model, iter, FILENAME_COLUMN, &val, -1);

        *songs = g_slist_append (*songs, val);
}

static gboolean
get_songs_foreach (GtkTreeModel *model,
                   GtkTreePath *path,
                   GtkTreeIter *iter,
                   gpointer userdata)
{
        ARIO_LOG_FUNCTION_START

        get_selected_songs_foreach (model, path, iter, userdata);

        return FALSE;
}

static void
get_selected_albums_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GSList **albums = (GSList **) userdata;

        ArioMpdAlbum *ario_mpd_album;

        ario_mpd_album = (ArioMpdAlbum *) g_malloc (sizeof (ArioMpdAlbum));
        gtk_tree_model_get (model, iter,
                            ALBUM_ARTIST_COLUMN, &ario_mpd_album->artist,
                            ALBUM_ALBUM_COLUMN, &ario_mpd_album->album,
                            ALBUM_PATH_COLUMN, &ario_mpd_album->path, -1);

        *albums = g_slist_append (*albums, ario_mpd_album);
}

static gboolean
get_albums_foreach (GtkTreeModel *model,
                    GtkTreePath *path,
                    GtkTreeIter *iter,
                    gpointer userdata)
{
        ARIO_LOG_FUNCTION_START

        get_selected_albums_foreach (model, path, iter, userdata);

        return FALSE;
}

static void
ario_browser_add_artists (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL;

        gtk_tree_model_foreach (GTK_TREE_MODEL (browser->priv->albums_model),
                                get_albums_foreach,
                                &albums);
        ario_playlist_append_albums (browser->priv->playlist, albums);

        g_slist_foreach (albums, (GFunc) ario_mpd_free_album, NULL);
        g_slist_free (albums);
}

static void
ario_browser_cmd_add_artists (GtkAction *action,
                              ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_add_artists (browser);
}

static void
ario_browser_add_albums (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;

        gtk_tree_model_foreach (GTK_TREE_MODEL (browser->priv->songs_model),
                                get_songs_foreach,
                                &songs);
        ario_playlist_append_songs (browser->priv->playlist, songs);

        g_slist_foreach (songs, (GFunc) g_free, NULL);
        g_slist_free (songs);
}

static void
ario_browser_cmd_add_albums (GtkAction *action,
                             ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_add_albums (browser);
}

static void
ario_browser_add_songs (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;

        gtk_tree_selection_selected_foreach (browser->priv->songs_selection,
                                             get_selected_songs_foreach,
                                             &songs);
        ario_playlist_append_songs (browser->priv->playlist, songs);

        g_slist_foreach (songs, (GFunc) g_free, NULL);
        g_slist_free (songs);
}

static void
ario_browser_cmd_add_songs (GtkAction *action,
                            ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_add_songs (browser);
}

static gboolean
ario_browser_covers_update (GtkTreeModel *model,
                            GtkTreePath *path,
                            GtkTreeIter *iter,
                            gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser = ARIO_BROWSER (userdata);
        gchar* artist;
        gchar *album;
        gchar *ario_cover_path;
        GdkPixbuf *cover;

        g_return_val_if_fail (IS_ARIO_BROWSER (browser), FALSE);

        gtk_tree_model_get (model, iter, ALBUM_ARTIST_COLUMN, &artist, ALBUM_ALBUM_COLUMN, &album, -1);

        ario_cover_path = ario_cover_make_ario_cover_path (artist, album, SMALL_COVER);

        /* The small cover exists, we show it */
        cover = gdk_pixbuf_new_from_file_at_size (ario_cover_path, COVER_SIZE, COVER_SIZE, NULL);
        g_free (ario_cover_path);

        if (!GDK_IS_PIXBUF (cover)) {
                /* There is no cover, we show a transparent picture */
                cover = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COVER_SIZE, COVER_SIZE);
                gdk_pixbuf_fill (cover, 0);
        }

        gtk_list_store_set (browser->priv->albums_model, iter,
                            ALBUM_ARTIST_COLUMN, artist, ALBUM_ALBUM_COLUMN, album,
                            ALBUM_COVER_COLUMN, cover,
                            -1);

        g_free (artist);
        g_free (album);
        g_object_unref (G_OBJECT (cover));

        return FALSE;
}

static void
ario_browser_cover_changed_cb (ArioCoverHandler *cover_handler,
                               ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        gtk_tree_model_foreach (GTK_TREE_MODEL (browser->priv->albums_model),
                                (GtkTreeModelForeachFunc) ario_browser_covers_update,
                                browser);
}

static void
ario_browser_cmd_albums_properties (GtkAction *action,
                                    ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverselect;
        GSList *albums = NULL;
        ArioMpdAlbum *ario_mpd_album;

        gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                             get_selected_albums_foreach,
                                             &albums);

        ario_mpd_album = albums->data;
        coverselect = ario_shell_coverselect_new (ario_mpd_album);
        gtk_dialog_run (GTK_DIALOG(coverselect));
        gtk_widget_destroy (coverselect);

        g_slist_foreach (albums, (GFunc) ario_mpd_free_album, NULL);
        g_slist_free (albums);
}

static void
ario_browser_cmd_songs_properties (GtkAction *action,
                                   ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GSList *paths = NULL;
        GList *songs;
        GtkWidget *songinfos;

        gtk_tree_selection_selected_foreach (browser->priv->songs_selection,
                                             get_selected_songs_foreach,
                                             &paths);

        songs = ario_mpd_get_songs_info (browser->priv->mpd,
                                         paths);
        g_slist_foreach (paths, (GFunc) g_free, NULL);
        g_slist_free (paths);

        songinfos = ario_shell_songinfos_new (browser->priv->mpd,
                                              songs);
        if (songinfos)
                gtk_widget_show_all (songinfos);
}

static void
ario_browser_add_in_playlist (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->artists)))
                ario_browser_add_artists (browser);

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->albums)))
                ario_browser_add_albums (browser);

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->songs)))
                ario_browser_add_songs (browser);
}

static void
get_artist_cover (ArioBrowser *browser,
                  guint operation)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverdownloader;
        GSList *albums = NULL;

        coverdownloader = ario_shell_coverdownloader_new (browser->priv->mpd);
        if (coverdownloader) {
                gtk_tree_model_foreach (GTK_TREE_MODEL (browser->priv->albums_model),
                                        get_albums_foreach,
                                        &albums);
                ario_shell_coverdownloader_get_covers_from_albums (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                                   albums,
                                                                   operation);

                g_slist_foreach (albums, (GFunc) ario_mpd_free_album, NULL);
                g_slist_free (albums);
        }
}

static void 
ario_browser_cmd_get_artist_cover (GtkAction *action,
                                   ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        get_artist_cover (browser, GET_COVERS);
}

static void 
ario_browser_cmd_remove_artist_cover (GtkAction *action,
                                      ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure that you want to remove all the covers of the selected artists?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        get_artist_cover (browser, REMOVE_COVERS);
}

static void
get_album_cover (ArioBrowser *browser,
                 guint operation)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL;
        GtkWidget *coverdownloader;

        coverdownloader = ario_shell_coverdownloader_new (browser->priv->mpd);
        if (coverdownloader) {
                gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                                     get_selected_albums_foreach,
                                                     &albums);
                ario_shell_coverdownloader_get_covers_from_albums (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                                   albums,
                                                                   operation);
                g_slist_foreach (albums, (GFunc) ario_mpd_free_album, NULL);
                g_slist_free (albums);
        }
}

static void 
ario_browser_cmd_get_album_cover (GtkAction *action,
                                  ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        get_album_cover (browser, GET_COVERS);
}

static void 
ario_browser_cmd_remove_album_cover (GtkAction *action,
                                     ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure that you want to remove all the covers of the selected albums?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        get_album_cover (browser, REMOVE_COVERS);
}

static void
ario_browser_covertree_visible_changed_cb (guint notification_id,
                                           ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        gtk_tree_view_column_set_visible (gtk_tree_view_get_column (GTK_TREE_VIEW (browser->priv->albums), 1),
                                          !ario_conf_get_boolean (PREF_COVER_TREE_HIDDEN, PREF_COVER_TREE_HIDDEN_DEFAULT));
        ario_browser_artists_selection_update (browser);
}
