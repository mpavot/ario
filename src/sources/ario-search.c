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

#include "sources/ario-search.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include "widgets/ario-songlist.h"
#include "widgets/ario-playlist.h"
#include "shell/ario-shell-songinfos.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "servers/ario-server.h"

#ifdef ENABLE_SEARCH

static void ario_search_finalize (GObject *object);
static void ario_search_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec);
static void ario_search_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec);
static void ario_search_connectivity_changed_cb (ArioServer *server,
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

        GtkWidget *vbox;
        GtkWidget *plus_button;
        GtkWidget *search_button;

        gboolean connected;
        GtkUIManager *ui_manager;

        GSList *search_constraints;
        GtkListStore *list_store;
};

/**
 * Wisgets added when user adds a new search
 * constraint
 */
typedef struct ArioSearchConstraint
{
        GtkWidget *hbox;
        GtkWidget *combo_box;
        GtkWidget *entry;
        GtkWidget *minus_button;
} ArioSearchConstraint;

/* Actions */
static GtkActionEntry ario_search_actions [] =
{
        { "SearchAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_add_songlists) },
        { "SearchAddPlaySongs", GTK_STOCK_MEDIA_PLAY, N_("Add and _play"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_add_play_songlists) },
        { "SearchClearAddPlaySongs", GTK_STOCK_REFRESH, N_("_Replace in playlist"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_clear_add_play_songlists) },
        { "SearchSongsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_songlist_cmd_songs_properties) }
};
static guint ario_search_n_actions = G_N_ELEMENTS (ario_search_actions);

/* Object properties */
enum
{
        PROP_0,
        PROP_UI_MANAGER
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

#define ARIO_SEARCH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SEARCH, ArioSearchPrivate))
G_DEFINE_TYPE (ArioSearch, ario_search, ARIO_TYPE_SOURCE)

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
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* Virtual GObject methods */
        object_class->finalize = ario_search_finalize;
        object_class->set_property = ario_search_set_property;
        object_class->get_property = ario_search_get_property;

        /* Virtual ArioSource methods */
        source_class->get_id = ario_search_get_id;
        source_class->get_name = ario_search_get_name;
        source_class->get_icon = ario_search_get_icon;

        /* Object properties */
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioSearchPrivate));
}

static void
ario_search_init (ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *hbox;
        GtkTreeIter iter;
        GtkWidget *image;
        gchar **items;
        int i;

        search->priv = ARIO_SEARCH_GET_PRIVATE (search);

        search->priv->connected = FALSE;

        /* Main vbox */
        search->priv->vbox = gtk_vbox_new (FALSE, 5);
        gtk_container_set_border_width (GTK_CONTAINER (search->priv->vbox), 10);

        /* Buttons for hbox */
        hbox = gtk_hbox_new (FALSE, 0);

        /* Plus button */
        image = gtk_image_new_from_stock (GTK_STOCK_ADD,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        search->priv->plus_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (search->priv->plus_button), image);
        gtk_widget_set_tooltip_text (GTK_WIDGET (search->priv->plus_button),
                                     _("Add a search criteria"));

        /* Connect signal for plus button */
        g_signal_connect (search->priv->plus_button,
                          "clicked",
                          G_CALLBACK (ario_search_do_plus),
                          search);

        /* Search button */
        search->priv->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
        gtk_widget_set_tooltip_text (GTK_WIDGET (search->priv->search_button),
                                     _("Search songs in the library"));

        /* Connect signal for search button */
        g_signal_connect (search->priv->search_button,
                          "clicked",
                          G_CALLBACK (ario_search_do_search),
                          search);

        /* Add buttons for hbox */
        gtk_box_pack_start (GTK_BOX (hbox),
                            search->priv->plus_button,
                            TRUE, TRUE, 0);

        gtk_box_pack_end (GTK_BOX (hbox),
                          search->priv->search_button,
                          TRUE, TRUE, 0);

        /* Add widgets to main vbox */
        gtk_box_pack_end (GTK_BOX (search->priv->vbox),
                          hbox,
                          FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search),
                            search->priv->vbox,
                            FALSE, FALSE, 0);

        /* Create model */
        search->priv->list_store = gtk_list_store_new (2,
                                                       G_TYPE_STRING,
                                                       G_TYPE_INT);

        /* Add tags to model */
        items = ario_server_get_items_names ();
        for (i = 0; i < ARIO_TAG_COUNT; ++i) {
                if (items[i]) {
                        gtk_list_store_append (search->priv->list_store, &iter);
                        gtk_list_store_set (search->priv->list_store, &iter,
                                            0, gettext (items[i]),
                                            1, i,
                                            -1);
                }
        }

        /* Add a search criteria box */
        ario_search_do_plus (NULL, search);

        /* Search hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (search), FALSE);
        gtk_box_set_spacing (GTK_BOX (search), 4);
}

static void
ario_search_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioSearch *search;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SEARCH (object));

        search = ARIO_SEARCH (object);

        g_return_if_fail (search->priv != NULL);
        g_object_unref (search->priv->list_store);

        G_OBJECT_CLASS (ario_search_parent_class)->finalize (object);
}

static void
ario_search_set_property (GObject *object,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioSearch *search = ARIO_SEARCH (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                search->priv->ui_manager = g_value_get_object (value);
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
        ARIO_LOG_FUNCTION_START;
        ArioSearch *search = ARIO_SEARCH (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                g_value_set_object (value, search->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_search_new (GtkUIManager *mgr,
                 GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START;
        ArioSearch *search;

        search = g_object_new (TYPE_ARIO_SEARCH,
                               "ui-manager", mgr,
                               NULL);

        g_return_val_if_fail (search->priv != NULL, NULL);

        /* Signals to synchronize the search with server */
        g_signal_connect_object (ario_server_get_instance (),
                                 "state_changed", G_CALLBACK (ario_search_connectivity_changed_cb),
                                 search, 0);

        /* Search songs list */
        search->priv->searchs = ario_songlist_new (mgr,
                                                   "/SearchPopup",
                                                   TRUE);

        /* Songs list widget */
        gtk_box_pack_start (GTK_BOX (search),
                            search->priv->searchs,
                            TRUE, TRUE, 0);

        /* Add actions */
        gtk_action_group_add_actions (group,
                                      ario_search_actions,
                                      ario_search_n_actions, search->priv->searchs);

        return GTK_WIDGET (search);
}

static void
ario_search_connectivity_changed_cb (ArioServer *server,
                                     ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        search->priv->connected = ario_server_is_connected ();
}

static void
ario_search_entry_grab_focus (GtkEntry *entry,
                              ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        GTK_WIDGET_SET_FLAGS (search->priv->search_button, GTK_CAN_DEFAULT);
        gtk_widget_grab_default (search->priv->search_button);
}

static void
ario_search_do_plus (GtkButton *button,
                     ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        GtkCellRenderer *renderer;
        GtkTreeIter iter, prev_iter;
        GtkWidget *image;
        int len;

        /* Create new search constraint */
        ArioSearchConstraint *search_constraint = (ArioSearchConstraint *) g_malloc (sizeof (ArioSearchConstraint));

        /* Create search constraint widgets */
        search_constraint->hbox = gtk_hbox_new (FALSE, 3);
        search_constraint->combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (search->priv->list_store));
        search_constraint->entry = gtk_entry_new ();
        gtk_entry_set_activates_default (GTK_ENTRY (search_constraint->entry), TRUE);

        /* Create minus button */
        image = gtk_image_new_from_stock (GTK_STOCK_REMOVE,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        search_constraint->minus_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (search_constraint->minus_button), image);
        gtk_widget_set_tooltip_text (GTK_WIDGET (search_constraint->minus_button),
                                     _("Remove a search criteria"));

        /* Connect signal for minus signal */
        g_signal_connect (search_constraint->minus_button,
                          "clicked",
                          G_CALLBACK (ario_search_do_minus),
                          search);

        /* Create renderer */
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (search_constraint->combo_box));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (search_constraint->combo_box), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (search_constraint->combo_box), renderer,
                                        "text", 0, NULL);

        /* Select last item in combobox (Any) */
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (search->priv->list_store), &iter);
        do {
                prev_iter = iter;
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (search->priv->list_store), &iter));
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (search_constraint->combo_box),
                                       &prev_iter);

        /* Add widgets to hbox */
        gtk_box_pack_start (GTK_BOX (search_constraint->hbox),
                            search_constraint->combo_box,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search_constraint->hbox),
                            search_constraint->entry,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search_constraint->hbox),
                            search_constraint->minus_button,
                            FALSE, FALSE, 0);

        /* Add hbox to main vbox */
        gtk_box_pack_start (GTK_BOX (search->priv->vbox),
                            search_constraint->hbox,
                            FALSE, FALSE, 2);

        gtk_widget_show_all (search_constraint->hbox);

        /* Append search constraint to the list */
        search->priv->search_constraints = g_slist_append (search->priv->search_constraints,
                                                           search_constraint);

        len = g_slist_length (search->priv->search_constraints);
        if (len == 1) {
                /* Forbid removal of last search constraint */
                search_constraint = search->priv->search_constraints->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, FALSE);
        } else if (len > 4) {
                /* Limit number of search constraints to 5 */
                gtk_widget_set_sensitive (search->priv->plus_button, FALSE);
        } else if (len == 2) {
                /* Reactivate minus button */
                search_constraint = search->priv->search_constraints->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, TRUE);
        }
        g_signal_connect (search_constraint->entry,
                          "grab-focus",
                           G_CALLBACK (ario_search_entry_grab_focus),
                           search);
}

static void
ario_search_do_minus (GtkButton *button,
                      ArioSearch *search)

{
        ARIO_LOG_FUNCTION_START;
        ArioSearchConstraint *search_constraint;
        GSList *tmp;

        /* Remove search constraint corresponding to pressed minus button */
        for (tmp = search->priv->search_constraints; tmp; tmp = g_slist_next (tmp)) {
                search_constraint = tmp->data;
                if (search_constraint->minus_button == (GtkWidget* ) button)
                        break;
        }
        g_return_if_fail (tmp);

        /* Destroy search constraint widgets */
        gtk_widget_destroy (search_constraint->combo_box);
        gtk_widget_destroy (search_constraint->entry);
        gtk_widget_destroy (search_constraint->minus_button);
        gtk_widget_destroy (search_constraint->hbox);

        /* Remove search constraint from the list */
        search->priv->search_constraints = g_slist_remove (search->priv->search_constraints,
                                                           search_constraint);
        g_free (search_constraint);

        /* Reactivate plus button if needed */
        gtk_widget_set_sensitive (search->priv->plus_button, TRUE);

        /* Deactivate minus button if only one search constraint remaining */
        if (g_slist_length (search->priv->search_constraints) == 1) {
                search_constraint = search->priv->search_constraints->data;
                gtk_widget_set_sensitive (search_constraint->minus_button, FALSE);
        }
}

static void
ario_search_do_search (GtkButton *button,
                       ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        ArioSearchConstraint *search_constraint;
        ArioServerAtomicCriteria *atomic_criteria;
        GSList *criteria = NULL;
        GSList *tmp;
        GSList *songs;
        ArioServerSong *song;
        GtkTreeIter iter;
        gchar *title;
        GtkListStore *liststore;

        /* Generate a list of ArioServerCriteria thanks to search criteria */
        for (tmp = search->priv->search_constraints; tmp; tmp = g_slist_next (tmp)) {
                search_constraint = tmp->data;

                atomic_criteria = (ArioServerAtomicCriteria *) g_malloc (sizeof (ArioServerAtomicCriteria));
                gtk_combo_box_get_active_iter (GTK_COMBO_BOX (search_constraint->combo_box), &iter);
                gtk_tree_model_get (GTK_TREE_MODEL (search->priv->list_store),
                                    &iter,
                                    1, &atomic_criteria->tag,
                                    -1);
                atomic_criteria->value = (gchar *) gtk_entry_get_text (GTK_ENTRY (search_constraint->entry));
                criteria = g_slist_append (criteria, atomic_criteria);
        }

        /* Get songs corresponding to criteria */
        songs = ario_server_get_songs (criteria, FALSE);

        g_slist_foreach (criteria, (GFunc) g_free, NULL);
        g_slist_free (criteria);

        /* Clear song list */
        liststore = ario_songlist_get_liststore (ARIO_SONGLIST (search->priv->searchs));
        gtk_list_store_clear (liststore);

        /* For each retrieved song */
        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                song = tmp->data;

                /* Add song to song list */
                gtk_list_store_append (liststore, &iter);
                title = ario_util_format_title (song);
                gtk_list_store_set (liststore, &iter,
                                    SONGS_TITLE_COLUMN, title,
                                    SONGS_ARTIST_COLUMN, song->artist,
                                    SONGS_ALBUM_COLUMN, song->album,
                                    SONGS_FILENAME_COLUMN, song->file,
                                    -1);
        }
        g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
        g_slist_free (songs);
}
#endif  /* ENABLE_SEARCH */

