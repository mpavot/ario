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

#include "sources/ario-browser.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "sources/ario-tree.h"
#include "sources/ario-tree-albums.h"
#include "sources/ario-tree-songs.h"
#include "widgets/ario-playlist.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_browser_finalize (GObject *object);
static void ario_browser_reload_trees (ArioBrowser *browser);
static void ario_browser_trees_changed_cb (guint notification_id,
                                           ArioBrowser *browser);
static void ario_browser_connectivity_changed_cb (ArioServer *server,
                                                  ArioBrowser *browser);
static void ario_browser_dbtime_changed_cb (ArioServer *server,
                                            ArioBrowser *browser);
static void ario_browser_fill_first (ArioBrowser *browser);
static void ario_browser_tree_selection_changed_cb (ArioTree *tree,
                                                    ArioBrowser *browser);
static void ario_browser_menu_popup_cb (ArioTree *tree,
                                        ArioBrowser *browser);
static void ario_browser_cmd_add (GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data);
static void ario_browser_cmd_add_play (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer data);
static void ario_browser_cmd_clear_add_play (GSimpleAction *action,
                                             GVariant *parameter,
                                             gpointer data);
static void ario_browser_cmd_get_cover (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data);
static void ario_browser_cmd_remove_cover (GSimpleAction *action,
                                           GVariant *parameter,
                                           gpointer data);
static void ario_browser_cmd_albums_properties (GSimpleAction *action,
                                                GVariant *parameter,
                                                gpointer data);
static void ario_browser_cmd_songs_properties (GSimpleAction *action,
                                               GVariant *parameter,
                                               gpointer data);

struct ArioBrowserPrivate
{
        GSList *trees;

        ArioTree *popup_tree;
};

/* Actions */
static const GActionEntry ario_browser_actions[] = {
        { "add-to-pl", ario_browser_cmd_add },
        { "add-play", ario_browser_cmd_add_play },
        { "clear-add-play", ario_browser_cmd_clear_add_play },
        { "get-covers", ario_browser_cmd_get_cover },
        { "remove-covers", ario_browser_cmd_remove_cover },
        { "albums-properties", ario_browser_cmd_albums_properties },
        { "songs-properties", ario_browser_cmd_songs_properties },
};
static guint ario_browser_n_actions = G_N_ELEMENTS (ario_browser_actions);

/* Object properties */
enum
{
        PROP_0,
};

G_DEFINE_TYPE_WITH_CODE (ArioBrowser, ario_browser, ARIO_TYPE_SOURCE, G_ADD_PRIVATE(ArioBrowser))

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
        return "go-home";
}

static void ario_browser_goto_playling_song (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (source);
        GSList *tmp;
        ArioServerSong *song = ario_server_get_current_song ();

        /* Not playing, do nothing */
        if (!song)
                return;

        /* Go to playing song in each tree */
        for (tmp = browser->priv->trees; tmp; tmp = g_slist_next (tmp)) {
                ario_tree_goto_playling_song (ARIO_TREE (tmp->data), song);
        }
}

static void
ario_browser_class_init (ArioBrowserClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* Virtual GObject methods */
        object_class->finalize = ario_browser_finalize;

        /* Virtual ArioSource methods */
        source_class->get_id = ario_browser_get_id;
        source_class->get_name = ario_browser_get_name;
        source_class->get_icon = ario_browser_get_icon;
        source_class->goto_playling_song = ario_browser_goto_playling_song;
}

static void
ario_browser_init (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        browser->priv = ario_browser_get_instance_private (browser);
        browser->priv->trees = NULL;
}

static void
ario_browser_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_BROWSER (object));

        browser = ARIO_BROWSER (object);

        g_return_if_fail (browser->priv != NULL);
        g_slist_free (browser->priv->trees);

        G_OBJECT_CLASS (ario_browser_parent_class)->finalize (object);
}

GtkWidget *
ario_browser_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser;
        ArioServer *server = ario_server_get_instance ();

        browser = ARIO_BROWSER (g_object_new (TYPE_ARIO_BROWSER,
                                              NULL));

        g_return_val_if_fail (browser->priv != NULL, NULL);

        /* Signals to synchronize the browser with server */
        g_signal_connect_object (server,
                                 "connectivity_changed", G_CALLBACK (ario_browser_connectivity_changed_cb),
                                 browser, 0);

        g_signal_connect_object (server,
                                 "updatingdb_changed", G_CALLBACK (ario_browser_dbtime_changed_cb),
                                 browser, 0);

        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_browser_actions,
                                         ario_browser_n_actions,
                                         browser);

        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (browser), TRUE);
        gtk_box_set_spacing (GTK_BOX (browser), 4);

        /* Load all trees */
        ario_browser_reload_trees (browser);

        /* Notification for trees configuration changes */
        ario_conf_notification_add (PREF_BROWSER_TREES,
                                    (ArioNotifyFunc) ario_browser_trees_changed_cb,
                                    browser);

        return GTK_WIDGET (browser);
}

static void
ario_browser_reload_trees (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *tree;
        gboolean is_first = TRUE;
        int i;
        gchar **splited_conf;
        const gchar *conf;
        GSList *tmp;
        gint tag;

        /* Remove all trees */
        for (tmp = browser->priv->trees; tmp; tmp = g_slist_next (tmp)) {
                gtk_container_remove (GTK_CONTAINER (browser), GTK_WIDGET (tmp->data));
        }
        g_slist_free (browser->priv->trees);
        browser->priv->trees = NULL;

        /* Get trees configuration */
        conf = ario_conf_get_string (PREF_BROWSER_TREES, PREF_BROWSER_TREES_DEFAULT);
        splited_conf = g_strsplit (conf, ",", MAX_TREE_NB);
        /* For each configured tree */
        for (i = 0; splited_conf[i]; ++i) {
                /* Create new tree */
                tag = atoi (splited_conf[i]);
                tree = ario_tree_new (tag,
                                      is_first);

                /* Append tree to the list */
                browser->priv->trees = g_slist_append (browser->priv->trees, tree);
                is_first = FALSE;

                /* Connect signal for tree selection changes */
                if (splited_conf[i+1]) { /* No signal for the last tree */
                        g_signal_connect (tree,
                                          "selection_changed",
                                          G_CALLBACK (ario_browser_tree_selection_changed_cb), browser);
                }
                /* Connect signal for popup menu */
                g_signal_connect (tree,
                                  "menu_popup",
                                  G_CALLBACK (ario_browser_menu_popup_cb), browser);

                /* Add tree to browser */
                gtk_box_pack_start (GTK_BOX (browser), tree, TRUE, TRUE, 0);
                gtk_widget_show_all (GTK_WIDGET (tree));
        }
        g_strfreev (splited_conf);
}

static void
ario_browser_trees_changed_cb (guint notification_id,
                               ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        /* Reload all trees */
        ario_browser_reload_trees (browser);

        /* Fill first tree */
        ario_browser_fill_first (browser);
}

static void
ario_browser_connectivity_changed_cb (ArioServer *server,
                                      ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill first tree */
        ario_browser_fill_first (browser);
}

static void
ario_browser_dbtime_changed_cb (ArioServer *server,
                                ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill first tree at end of update */
        if (!ario_server_get_updating ())
                ario_browser_fill_first (browser);
}

static void
ario_browser_fill_first (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        /* Fill first tree */
        if (browser->priv->trees && browser->priv->trees->data)
                ario_tree_fill (ARIO_TREE (browser->priv->trees->data));
}

static void
ario_browser_tree_selection_changed_cb (ArioTree *tree,
                                        ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *next_tree = NULL;
        GSList *tmp, *criterias;

        /* Get next tree in the list */
        for (tmp = browser->priv->trees; tmp; tmp = g_slist_next (tmp)) {
                if (tmp->data == tree && g_list_next (tmp)) {
                        next_tree = g_slist_next (tmp)->data;
                }
        }
        g_return_if_fail (next_tree);

        /* Clear criteria of next tree */
        ario_tree_clear_criterias (next_tree);

        /* Add criteria of current tree to next tree */
        criterias = ario_tree_get_criterias (tree);
        for (tmp = criterias; tmp; tmp = g_slist_next (tmp)) {
                ario_tree_add_criteria (next_tree, tmp->data);
        }

        /* Fill next tree */
        ario_tree_fill (next_tree);
}

static void
ario_browser_menu_popup_cb (ArioTree *tree,
                            ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START;
        /* Remember on which tree popup menu must be shown */
        browser->priv->popup_tree = tree;
}

static void
ario_browser_cmd_add (GSimpleAction *action,
                      GVariant *parameter,
                      gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (data);
        /* Add songs to playlist */
        if (browser->priv->popup_tree)
                ario_tree_cmd_add (browser->priv->popup_tree, PLAYLIST_ADD);
}

static void
ario_browser_cmd_add_play (GSimpleAction *action,
                           GVariant *parameter,
                           gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (data);
        /* Add songs to playlist and play */
        if (browser->priv->popup_tree)
                ario_tree_cmd_add (browser->priv->popup_tree, PLAYLIST_ADD_PLAY);
}

static void
ario_browser_cmd_clear_add_play (GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (data);
        if (browser->priv->popup_tree) {
                /* Add songs to playlist and play */
                ario_tree_cmd_add (browser->priv->popup_tree, PLAYLIST_REPLACE);
        }
}

static void
ario_browser_cmd_get_cover (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (data);
        /* Get cover arts for selected entries */
        if (browser->priv->popup_tree)
                ario_tree_get_cover (browser->priv->popup_tree, GET_COVERS);
}

static void
ario_browser_cmd_remove_cover (GSimpleAction *action,
                               GVariant *parameter,
                               gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        /* Remove cover arts for selected entries */
        ArioBrowser *browser = ARIO_BROWSER (data);
        if (browser->priv->popup_tree)
                ario_tree_get_cover (browser->priv->popup_tree, REMOVE_COVERS);
}

static void
ario_browser_cmd_albums_properties (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (data);
        g_return_if_fail (IS_ARIO_TREE_ALBUMS (browser->priv->popup_tree));

        /* Show album properties */
        if (browser->priv->popup_tree)
                ario_tree_albums_cmd_albums_properties (ARIO_TREE_ALBUMS (browser->priv->popup_tree));
}

static void
ario_browser_cmd_songs_properties (GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowser *browser = ARIO_BROWSER (data);
        g_return_if_fail (IS_ARIO_TREE_SONGS (browser->priv->popup_tree));

        /* Show songs properties */
        if (browser->priv->popup_tree)
                ario_tree_songs_cmd_songs_properties (ARIO_TREE_SONGS (browser->priv->popup_tree));
}
