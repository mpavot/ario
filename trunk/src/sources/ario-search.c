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
#include "lib/libmpdclient.h"
#include "widgets/ario-songlist.h"
#include "sources/ario-search.h"
#include "shell/ario-shell-songinfos.h"
#include "ario-util.h"
#include "ario-debug.h"

#ifdef ENABLE_SEARCH

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
static void ario_search_do_plus (GtkButton *button,
                                 ArioSearch *search);
static void ario_search_do_minus (GtkButton *button,
                                  ArioSearch *search);
static void ario_search_do_search (GtkButton *button,
                                   ArioSearch *search);
struct ArioSearchPrivate
{        
        GtkWidget *searchs;

        GtkTooltips *tooltips;

        GtkWidget *vbox;
        GtkWidget *plus_button;
        GtkWidget *search_button;

        gboolean connected;

        ArioMpd *mpd;
        ArioPlaylist *playlist;
        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        GSList *search_constraints;
        GtkListStore *list_store;
};

static GtkActionEntry ario_search_actions [] =
{
        { "SearchAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_add_songlists) },
        { "SearchSongsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_songs_properties) }
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

                type = g_type_register_static (ARIO_TYPE_SOURCE,
                                               "ArioSearch",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_search_get_id (ArioSource *source)
{
        return "search";
}

static gchar *
ario_search_get_name (ArioSource *source)
{
        return _("Search");
}

static gchar *
ario_search_get_icon (ArioSource *source)
{
        return GTK_STOCK_FIND;
}

static void
ario_search_class_init (ArioSearchClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_search_finalize;

        object_class->set_property = ario_search_set_property;
        object_class->get_property = ario_search_get_property;

        source_class->get_id = ario_search_get_id;
        source_class->get_name = ario_search_get_name;
        source_class->get_icon = ario_search_get_icon;

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
        GtkWidget *hbox;
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

        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (search), FALSE);
        gtk_box_set_spacing (GTK_BOX (search), 4);
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
        GtkWidget *scrolledwindow_searchs;

        search = g_object_new (TYPE_ARIO_SEARCH,
                               "ui-manager", mgr,
                               "action-group", group,
                               "mpd", mpd,
                               "playlist", playlist,
                               NULL);

        g_return_val_if_fail (search->priv != NULL, NULL);

        /* Searchs list */
        scrolledwindow_searchs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_searchs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_searchs), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_searchs), GTK_SHADOW_IN);
        search->priv->searchs = ario_songlist_new (mgr,
                                                   mpd,
                                                   playlist,
                                                   "/SearchPopup",
                                                   TRUE);

        gtk_container_add (GTK_CONTAINER (scrolledwindow_searchs), search->priv->searchs);
        gtk_box_pack_start (GTK_BOX (search),
                            scrolledwindow_searchs,
                            TRUE, TRUE, 0);

        gtk_action_group_add_actions (search->priv->actiongroup,
                                      ario_search_actions,
                                      ario_search_n_actions, search->priv->searchs);

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
ario_search_entry_grab_focus (GtkEntry *entry, 
                              ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START
        GTK_WIDGET_SET_FLAGS (search->priv->search_button, GTK_CAN_DEFAULT);
        gtk_widget_grab_default (search->priv->search_button);
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

        search->priv->search_constraints = g_slist_append (search->priv->search_constraints,
                                                           search_constraint);

        len = g_slist_length (search->priv->search_constraints);
        if (len == 1) {
                search_constraint = search->priv->search_constraints->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, FALSE);
        } else if ( len > 4) {
                gtk_widget_set_sensitive (search->priv->plus_button, FALSE);
        } else if (len == 2) {
                search_constraint = search->priv->search_constraints->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, TRUE);
        }
        g_signal_connect_object (G_OBJECT (search_constraint->entry),
                                 "grab-focus",
                                 G_CALLBACK (ario_search_entry_grab_focus),
                                 search, 0);
}

static void
ario_search_do_minus (GtkButton *button,
                      ArioSearch *search)

{
        ARIO_LOG_FUNCTION_START
        ArioSearchConstraint *search_constraint;
        GSList *tmp;

        for (tmp = search->priv->search_constraints; tmp; tmp = g_slist_next (tmp)) {
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

        search->priv->search_constraints = g_slist_remove (search->priv->search_constraints,
                                                           search_constraint);
        g_free (search_constraint);

        gtk_widget_set_sensitive (search->priv->plus_button, TRUE);

        if (g_slist_length (search->priv->search_constraints) == 1) {
                search_constraint = search->priv->search_constraints->data;
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
        GSList *search_criterias = NULL;
        GSList *tmp;
        GSList *songs;
        ArioMpdSong *song;
        GtkTreeIter iter;
        gchar *title;
        GValue *value;
        GtkListStore *liststore = ario_songlist_get_liststore (ARIO_SONGLIST (search->priv->searchs));

        for (tmp = search->priv->search_constraints; tmp; tmp = g_slist_next (tmp)) {
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
                search_criterias = g_slist_append (search_criterias, search_criteria);
        }

        songs = ario_mpd_search (search->priv->mpd, search_criterias);
        g_slist_foreach (search_criterias, (GFunc) g_free, NULL);
        g_slist_free (search_criterias);

        gtk_list_store_clear (liststore);

        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;

                gtk_list_store_append (liststore, &iter);
                title = ario_util_format_title (song);
                gtk_list_store_set (liststore, &iter,
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
#endif  /* ENABLE_SEARCH */

