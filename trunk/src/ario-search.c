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
#include "libmpdclient.h"
#include <glib/gi18n.h>
#include "ario-search.h"
#include "ario-util.h"
#include "ario-debug.h"

#ifdef ENABLE_SEARCH

#define DRAG_THRESHOLD 1

typedef struct ArioSearchConstraint
{
        GtkWidget *hbox;
        GtkWidget *combo_box;
        GtkWidget *entry;
        GtkWidget *minus_button;
} ArioSearchConstraint;

char * ArioSearchItemKeys[MPD_TAG_NUM_OF_ITEM_TYPES] =
{
	N_("Artist"),    // MPD_TAG_ITEM_ARTIST
	N_("Album"),     // MPD_TAG_ITEM_ALBUM
	N_("Title"),     // MPD_TAG_ITEM_TITLE
	N_("Track"),     // MPD_TAG_ITEM_TRACK
	NULL,            // MPD_TAG_ITEM_NAME
	NULL,            // MPD_TAG_ITEM_GENRE
	NULL,            // MPD_TAG_ITEM_DATE
	NULL,            // MPD_TAG_ITEM_COMPOSER
	NULL,            // MPD_TAG_ITEM_PERFORMER
	NULL,            // MPD_TAG_ITEM_COMMENT
	NULL,            // MPD_TAG_ITEM_DISC
	N_("Filename"),  // MPD_TAG_ITEM_FILENAME
	N_("Any")        // MPD_TAG_ITEM_ANY
};

static void ario_search_class_init (ArioSearchClass *klass);
static void ario_search_init (ArioSearch *search);
static void ario_search_finalize (GObject *object);
static void ario_search_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void ario_search_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);
static void ario_search_state_changed_cb (ArioMpd *mpd,
                                         ArioSearch *search);
static void ario_search_add_in_playlist (ArioSearch *search);
static void ario_search_cmd_add_searchs (GtkAction *action,
                                       ArioSearch *search);
static void ario_search_popup_menu (ArioSearch *search);
static gboolean ario_search_button_press_cb (GtkWidget *widget,
                                            GdkEventButton *event,
                                            ArioSearch *search);
static gboolean ario_search_button_release_cb (GtkWidget *widget,
                                              GdkEventButton *event,
                                              ArioSearch *search);
static gboolean ario_search_motion_notify_cb (GtkWidget *widget, 
                                          GdkEventMotion *event,
                                          ArioSearch *search);
static void ario_search_realize_cb (GtkWidget *widget, 
                                    ArioSearch *search);
static void ario_search_searchs_selection_drag_foreach (GtkTreeModel *model,
                                                      GtkTreePath *path,
                                                      GtkTreeIter *iter,
                                                      gpointer userdata);

static void ario_search_drag_data_get_cb (GtkWidget * widget,
                                         GdkDragContext * context,
                                         GtkSelectionData * selection_data,
                                         guint info, guint time, gpointer data);
static void ario_search_do_plus (GtkButton *button,
                                 ArioSearch *search);
static void ario_search_do_minus (GtkButton *button,
                                  ArioSearch *search);
static void ario_search_do_search (GtkButton *button,
                                   ArioSearch *search);
struct ArioSearchPrivate
{        
        GtkWidget *searchs;
        GtkListStore *searchs_model;
        GtkTreeSelection *searchs_selection;

        GtkTooltips *tooltips;

        GtkWidget *vbox;
        GtkWidget *plus_button;
        GtkWidget *search_button;

        gboolean connected;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        ArioMpd *mpd;
        ArioPlaylist *playlist;
        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        GList *search_constraints;
        GtkListStore *list_store;
};

static GtkActionEntry ario_search_actions [] =
{
        { "SearchAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
          N_("Add to the playlist"),
          G_CALLBACK (ario_search_cmd_add_searchs) }
};
static guint ario_search_n_actions = G_N_ELEMENTS (ario_search_actions);

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
        TITLE_COLUMN,
        ARTIST_COLUMN,
        ALBUM_COLUMN,
        FILENAME_COLUMN,
        N_COLUMN
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

static GObjectClass *parent_class = NULL;

GType
ario_search_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioSearchClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_search_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioSearch),
                        0,
                        (GInstanceInitFunc) ario_search_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "ArioSearch",
                                                &our_info, 0);
        }
        return type;
}

static void
ario_search_class_init (ArioSearchClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_search_finalize;

        object_class->set_property = ario_search_set_property;
        object_class->get_property = ario_search_get_property;

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
ario_search_init (ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GtkWidget *hbox;
        GtkWidget *scrolledwindow_searchs;
        GtkTreeIter iter;
        GtkWidget *image;
        int i;

        search->priv = g_new0 (ArioSearchPrivate, 1);

        search->priv->connected = FALSE;

        search->priv->vbox = gtk_vbox_new (FALSE, 5);
        gtk_container_set_border_width (GTK_CONTAINER (search->priv->vbox), 10);

        hbox = gtk_hbox_new (FALSE, 0);
        search->priv->tooltips = gtk_tooltips_new ();
        gtk_tooltips_enable (search->priv->tooltips);

        /* Plus button */
        image = gtk_image_new_from_stock (GTK_STOCK_ADD,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);

        search->priv->plus_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (search->priv->plus_button), image);
        g_signal_connect (G_OBJECT (search->priv->plus_button),
                          "clicked", G_CALLBACK (ario_search_do_plus), search);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (search->priv->tooltips), 
                              GTK_WIDGET (search->priv->plus_button), 
                              _("Add a search criteria"), NULL);
        /* Search button */
        search->priv->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
        g_signal_connect (G_OBJECT (search->priv->search_button),
                          "clicked", G_CALLBACK (ario_search_do_search), search);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (search->priv->tooltips), 
                              GTK_WIDGET (search->priv->search_button), 
                              _("Search songs in the library"), NULL);

        gtk_box_pack_start (GTK_BOX (hbox),
                            search->priv->plus_button,
                            TRUE, TRUE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          search->priv->search_button,
                          TRUE, TRUE, 0);

        gtk_box_pack_end (GTK_BOX (search->priv->vbox),
                          hbox,
                          FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search),
                            search->priv->vbox,
                            FALSE, FALSE, 0);

        search->priv->list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

        for (i = 0; i < MPD_TAG_NUM_OF_ITEM_TYPES; ++i) {
                if (ArioSearchItemKeys[i]) {
                        gtk_list_store_append (search->priv->list_store, &iter);
                        gtk_list_store_set (search->priv->list_store, &iter,
                                            0, gettext (ArioSearchItemKeys[i]),
                                            1, i,
                                            -1);
                }
        }

        ario_search_do_plus (NULL, search);

        /* Searchs list */
        scrolledwindow_searchs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_searchs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_searchs), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_searchs), GTK_SHADOW_IN);
        search->priv->searchs = gtk_tree_view_new ();

        /* Titles */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           renderer,
                                                           "text", TITLE_COLUMN,
                                                           NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, TITLE_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (search->priv->searchs), column);

        /* Artists */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           renderer,
                                                           "text", ARTIST_COLUMN,
                                                           NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, ARTIST_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (search->priv->searchs), column);

        /* Albums */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                           renderer,
                                                           "text", ALBUM_COLUMN,
                                                           NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 200);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, ALBUM_COLUMN);
        gtk_tree_view_append_column (GTK_TREE_VIEW (search->priv->searchs), column);

        search->priv->searchs_model = gtk_list_store_new (N_COLUMN,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING);

        gtk_tree_view_set_model (GTK_TREE_VIEW (search->priv->searchs),
                                 GTK_TREE_MODEL (search->priv->searchs_model));
        search->priv->searchs_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (search->priv->searchs));
        gtk_tree_selection_set_mode (search->priv->searchs_selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_searchs), search->priv->searchs);

        gtk_drag_source_set (search->priv->searchs,
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (search->priv->searchs),
                          "drag_data_get", 
                          G_CALLBACK (ario_search_drag_data_get_cb), search);

        g_signal_connect_object (G_OBJECT (search->priv->searchs),
                                 "button_press_event",
                                 G_CALLBACK (ario_search_button_press_cb),
                                 search,
                                 0);
        g_signal_connect_object (G_OBJECT (search->priv->searchs),
                                 "button_release_event",
                                 G_CALLBACK (ario_search_button_release_cb),
                                 search,
                                 0);
        g_signal_connect_object (G_OBJECT (search->priv->searchs),
                                 "motion_notify_event",
                                 G_CALLBACK (ario_search_motion_notify_cb),
                                 search,
                                 0);
        g_signal_connect_object (G_OBJECT (search),
                                 "realize",
                                 G_CALLBACK (ario_search_realize_cb),
                                 search,
                                 0);
        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (search), FALSE);
        gtk_box_set_spacing (GTK_BOX (search), 4);

        gtk_box_pack_start (GTK_BOX (search),
                            scrolledwindow_searchs,
                            TRUE, TRUE, 0);
}

static void
ario_search_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioSearch *search;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SEARCH (object));

        search = ARIO_SEARCH (object);

        g_return_if_fail (search->priv != NULL);
        g_object_unref (search->priv->list_store);
        g_free (search->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_search_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioSearch *search = ARIO_SEARCH (object);
        
        switch (prop_id) {
        case PROP_MPD:
                search->priv->mpd = g_value_get_object (value);

                /* Signals to synchronize the search with mpd */
                g_signal_connect_object (G_OBJECT (search->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_search_state_changed_cb),
                                         search, 0);
                break;
        case PROP_PLAYLIST:
                search->priv->playlist = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                search->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                search->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (search->priv->actiongroup,
                                              ario_search_actions,
                                              ario_search_n_actions, search);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_search_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioSearch *search = ARIO_SEARCH (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, search->priv->mpd);
                break;
        case PROP_PLAYLIST:
                g_value_set_object (value, search->priv->playlist);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, search->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, search->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_search_new (GtkUIManager *mgr,
                  GtkActionGroup *group,
                  ArioMpd *mpd,
                  ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioSearch *search;

        search = g_object_new (TYPE_ARIO_SEARCH,
                              "ui-manager", mgr,
                              "action-group", group,
                              "mpd", mpd,
                              "playlist", playlist,
                              NULL);

        g_return_val_if_fail (search->priv != NULL, NULL);

        return GTK_WIDGET (search);
}

static void
ario_search_state_changed_cb (ArioMpd *mpd,
                             ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        if (search->priv->connected != ario_mpd_is_connected (mpd)) {
                search->priv->connected = ario_mpd_is_connected (mpd);
                /* ario_search_set_active (search->priv->connected); */
        }
}

static void
searchs_foreach (GtkTreeModel *model,
                 GtkTreePath *path,
                 GtkTreeIter *iter,
                 gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GList **searchs = (GList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, FILENAME_COLUMN, &val, -1);

        *searchs = g_list_append (*searchs, val);
}

static void
ario_search_add_in_playlist (ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        GList *searchs = NULL;

        gtk_tree_selection_selected_foreach (search->priv->searchs_selection,
                                             searchs_foreach,
                                             &searchs);
        ario_playlist_append_songs (search->priv->playlist, searchs);

        g_list_foreach (searchs, (GFunc) g_free, NULL);
        g_list_free (searchs);
}

static void
ario_search_cmd_add_searchs (GtkAction *action,
                             ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        ario_search_add_in_playlist (search);
}

static void
ario_search_popup_menu (ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;

        menu = gtk_ui_manager_get_widget (search->priv->ui_manager, "/SearchPopup");

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                        gtk_get_current_event_time ());
}

static gboolean
ario_search_button_press_cb (GtkWidget *widget,
                            GdkEventButton *event,
                            ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (search->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
                ario_search_add_in_playlist (search);

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
                        search->priv->drag_start_x = x;
                        search->priv->drag_start_y = y;
                        search->priv->pressed = TRUE;

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
                        ario_search_popup_menu (search);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_search_button_release_cb (GtkWidget *widget,
                                GdkEventButton *event,
                                ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        if (!search->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        gtk_tree_selection_unselect_all (selection);
                        gtk_tree_selection_select_path (selection, path);
                        gtk_tree_path_free (path);
                }
        }

        search->priv->dragging = FALSE;
        search->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_search_motion_notify_cb (GtkWidget *widget, 
                            GdkEventMotion *event,
                            ArioSearch *search)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((search->priv->dragging) || !(search->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - search->priv->drag_start_x;
        dy = y - search->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                search->priv->dragging = TRUE;

        return FALSE;
}

static void
ario_search_realize_cb (GtkWidget *widget, 
                        ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START

        GTK_WIDGET_SET_FLAGS (search->priv->search_button, GTK_CAN_DEFAULT);
        gtk_widget_grab_default (search->priv->search_button);
}

static void
ario_search_searchs_selection_drag_foreach (GtkTreeModel *model,
                                          GtkTreePath *path,
                                          GtkTreeIter *iter,
                                          gpointer userdata)
{
        ARIO_LOG_FUNCTION_START
        GString *searchs = (GString *) userdata;
        g_return_if_fail (searchs != NULL);

        gchar* val = NULL;

        gtk_tree_model_get (model, iter, FILENAME_COLUMN, &val, -1);
        g_string_append (searchs, val);
        g_string_append (searchs, "\n");
        g_free (val);
}

static void
ario_search_drag_data_get_cb (GtkWidget * widget,
                             GdkDragContext * context,
                             GtkSelectionData * selection_data,
                             guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START
        ArioSearch *search;
        GString* searchs = NULL;

        search = ARIO_SEARCH (data);

        g_return_if_fail (IS_ARIO_SEARCH (search));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        searchs = g_string_new("");
        gtk_tree_selection_selected_foreach (search->priv->searchs_selection,
                                             ario_search_searchs_selection_drag_foreach,
                                             searchs);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) searchs->str,
                                strlen (searchs->str) * sizeof(guchar));
        
        g_string_free (searchs, TRUE);
}

static void
ario_search_do_plus (GtkButton *button,
                     ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        GtkCellRenderer *renderer;
        GtkTreeIter iter, prev_iter;
        GtkWidget *image;
        int len;

        ArioSearchConstraint *search_constraint = (ArioSearchConstraint *) g_malloc (sizeof (ArioSearchConstraint));

        search_constraint->hbox = gtk_hbox_new (FALSE, 3);
        search_constraint->combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (search->priv->list_store));
        search_constraint->entry = gtk_entry_new ();
        gtk_entry_set_activates_default (GTK_ENTRY (search_constraint->entry), TRUE);
        image = gtk_image_new_from_stock (GTK_STOCK_REMOVE,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        search_constraint->minus_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (search_constraint->minus_button), image);

        g_signal_connect (G_OBJECT (search_constraint->minus_button),
                          "clicked", G_CALLBACK (ario_search_do_minus), search);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (search->priv->tooltips), 
                              GTK_WIDGET (search_constraint->minus_button), 
                              _("Remove a search criteria"), NULL);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (search_constraint->combo_box));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (search_constraint->combo_box), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (search_constraint->combo_box), renderer,
                                        "text", 0, NULL);
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (search->priv->list_store), &iter);
        do
        {
                prev_iter = iter;
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (search->priv->list_store), &iter));
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (search_constraint->combo_box),
                                       &prev_iter);

        gtk_box_pack_start (GTK_BOX (search_constraint->hbox),
                            search_constraint->combo_box,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search_constraint->hbox),
                            search_constraint->entry,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search_constraint->hbox),
                            search_constraint->minus_button,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search->priv->vbox),
                            search_constraint->hbox,
                            FALSE, FALSE, 2);

        gtk_widget_show_all (search_constraint->hbox);

        search->priv->search_constraints = g_list_append (search->priv->search_constraints,
                                                          search_constraint);

        len = g_list_length (search->priv->search_constraints);
        if (len == 1) {
                search_constraint = (g_list_first (search->priv->search_constraints))->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, FALSE);
        } else if ( len > 4) {
                gtk_widget_set_sensitive (search->priv->plus_button, FALSE);
        } else if (len == 2) {
                search_constraint = (g_list_first (search->priv->search_constraints))->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, TRUE);
        }
}

static void
ario_search_do_minus (GtkButton *button,
                      ArioSearch *search)

{
        ARIO_LOG_FUNCTION_START
        ArioSearchConstraint *search_constraint;
        GList *tmp;

        for (tmp = search->priv->search_constraints; tmp; tmp = g_list_next (tmp)) {
                search_constraint = tmp->data;
                if (search_constraint->minus_button == (GtkWidget* ) button)
                        break;
        }
        if (!tmp)
                return;        

        gtk_widget_destroy (search_constraint->combo_box);
        gtk_widget_destroy (search_constraint->entry);
        gtk_widget_destroy (search_constraint->minus_button);
        gtk_widget_destroy (search_constraint->hbox);

        search->priv->search_constraints = g_list_remove (search->priv->search_constraints,
                                                          search_constraint);
        g_free (search_constraint);

        gtk_widget_set_sensitive (search->priv->plus_button, TRUE);

        if (g_list_length (search->priv->search_constraints) == 1) {
                search_constraint = (g_list_first (search->priv->search_constraints))->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, FALSE);
        }
}

static void
ario_search_do_search (GtkButton *button,
                       ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        ArioSearchConstraint *search_constraint;
        ArioMpdSearchCriteria *search_criteria;
        GList *search_criterias = NULL;
        GList *tmp;
        GList *songs;
        ArioMpdSong *song;
        GtkTreeIter iter;
        gchar *title;
        GValue *value;

        for (tmp = search->priv->search_constraints; tmp; tmp = g_list_next (tmp)) {
                search_constraint = tmp->data;

                search_criteria = (ArioMpdSearchCriteria *) g_malloc (sizeof (ArioMpdSearchCriteria));
                gtk_combo_box_get_active_iter (GTK_COMBO_BOX (search_constraint->combo_box), &iter);
                value = (GValue*)g_malloc(sizeof(GValue));
                value->g_type = 0;
                gtk_tree_model_get_value (GTK_TREE_MODEL (search->priv->list_store),
                                          &iter,
                                          1, value);
                search_criteria->type = g_value_get_int(value);
                g_free (value);
                search_criteria->value = gtk_entry_get_text (GTK_ENTRY (search_constraint->entry));
                search_criterias = g_list_append (search_criterias, search_criteria);
        }

        songs = ario_mpd_search (search->priv->mpd, search_criterias);
        g_list_foreach (search_criterias, (GFunc) g_free, NULL);
        g_list_free (search_criterias);
        
        gtk_list_store_clear (search->priv->searchs_model);

        for (tmp = songs; tmp; tmp = g_list_next (tmp)) {
                song = tmp->data;

                gtk_list_store_append (search->priv->searchs_model, &iter);
                title = ario_util_format_title (song);
                gtk_list_store_set (search->priv->searchs_model, &iter,
                                    TITLE_COLUMN, title,
                                    ARTIST_COLUMN, song->artist,
                                    ALBUM_COLUMN, song->album,
                                    FILENAME_COLUMN, song->file,
                                    -1);
                g_free (title);
        }
        g_list_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
        g_list_free (songs);
}
#endif  /* ENABLE_SEARCH */

