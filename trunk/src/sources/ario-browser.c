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
#include <stdlib.h>
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "sources/ario-browser.h"
#include "sources/ario-tree.h"
#include "widgets/ario-playlist.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

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
static void ario_browser_reload_trees (ArioBrowser *browser);
static void ario_browser_trees_changed_cb (guint notification_id,
                                           ArioBrowser *browser);
static void ario_browser_state_changed_cb (ArioMpd *mpd,
                                           ArioBrowser *browser);
static void ario_browser_dbtime_changed_cb (ArioMpd *mpd,
                                            ArioBrowser *browser);
static void ario_browser_fill_first (ArioBrowser *browser);
static void ario_browser_tree_selection_changed_cb (ArioTree *tree,
                                                    ArioBrowser *browser);
static void ario_browser_menu_popup_cb (ArioTree *tree,
                                        ArioBrowser *browser);
static void ario_browser_cmd_add (GtkAction *action,
                                  ArioBrowser *browser);
static void ario_browser_cmd_add_play (GtkAction *action,
                                       ArioBrowser *browser);
static void ario_browser_cmd_clear_add_play (GtkAction *action,
                                             ArioBrowser *browser);
static void ario_browser_cmd_get_cover (GtkAction *action,
                                        ArioBrowser *browser);
static void ario_browser_cmd_remove_cover (GtkAction *action,
                                           ArioBrowser *browser);
static void ario_browser_cmd_albums_properties (GtkAction *action,
                                                ArioBrowser *browser);
static void ario_browser_cmd_songs_properties (GtkAction *action,
                                               ArioBrowser *browser);

struct ArioBrowserPrivate
{
        GSList *trees;

        gboolean connected;

        ArioMpd *mpd;
        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        ArioTree *popup_tree;
};

static GtkActionEntry ario_browser_actions [] =
{
        { "BrowserAdd", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_add) },
        { "BrowserAddPlay", GTK_STOCK_MEDIA_PLAY, N_("Add and _play"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_add_play) },
        { "BrowserClearAddPlay", GTK_STOCK_REFRESH, N_("_Replace in playlist"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_clear_add_play) },
        { "BrowserGetCover", GTK_STOCK_CDROM, N_("Get the covers"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_get_cover) },
        { "BrowserRemoveCover", GTK_STOCK_DELETE, N_("_Delete the covers"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_remove_cover) },
        { "BrowserAlbumsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_albums_properties) },
        { "BrowserSongsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL,
                NULL,
                G_CALLBACK (ario_browser_cmd_songs_properties) },
};
static guint ario_browser_n_actions = G_N_ELEMENTS (ario_browser_actions);

enum
{
        PROP_0,
        PROP_MPD,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
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

        browser->priv = g_new0 (ArioBrowserPrivate, 1);

        browser->priv->connected = FALSE;
        browser->priv->trees = NULL;
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
        g_slist_free (browser->priv->trees);
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
                g_signal_connect (browser->priv->mpd,
                                  "state_changed", G_CALLBACK (ario_browser_state_changed_cb),
                                  browser);

                g_signal_connect (browser->priv->mpd,
                                  "updatingdb_changed", G_CALLBACK (ario_browser_dbtime_changed_cb),
                                  browser);
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
                  ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowser *browser;

        browser = ARIO_BROWSER (g_object_new (TYPE_ARIO_BROWSER,
                                              "ui-manager", mgr,
                                              "action-group", group,
                                              "mpd", mpd,
                                              NULL));

        g_return_val_if_fail (browser->priv != NULL, NULL);

        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (browser), TRUE);
        gtk_box_set_spacing (GTK_BOX (browser), 4);

        /* Trees */
        ario_browser_reload_trees (browser);

        ario_conf_notification_add (PREF_BROWSER_TREES,
                                    (ArioNotifyFunc) ario_browser_trees_changed_cb,
                                    browser);

        return GTK_WIDGET (browser);
}

static void
ario_browser_reload_trees (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *tree;
        gboolean is_first = TRUE;
        int i;
        gchar **splited_conf;
        gchar *conf;
        GSList *tmp;

        /* Remove all trees */
        for (tmp = browser->priv->trees; tmp; tmp = g_slist_next (tmp)) {
                gtk_container_remove (GTK_CONTAINER (browser), GTK_WIDGET (tmp->data));
        }
        g_slist_free (browser->priv->trees);
        browser->priv->trees = NULL;

        conf = ario_conf_get_string (PREF_BROWSER_TREES, PREF_BROWSER_TREES_DEFAULT);
        splited_conf = g_strsplit (conf, ",", MAX_TREE_NB);
        g_free (conf);
        for (i = 0; splited_conf[i]; ++i) {
                tree = ario_tree_new (browser->priv->ui_manager,
                                      browser->priv->mpd,
                                      atoi (splited_conf[i]),
                                      is_first);
                browser->priv->trees = g_slist_append (browser->priv->trees, tree);
                is_first = FALSE;

                if (splited_conf[i+1]) { /* No signal for the last tree */
                        g_signal_connect (tree,
                                          "selection_changed", 
                                          G_CALLBACK (ario_browser_tree_selection_changed_cb), browser);
                }
                g_signal_connect (tree,
                                  "menu_popup", 
                                  G_CALLBACK (ario_browser_menu_popup_cb), browser);

                gtk_box_pack_start (GTK_BOX (browser), tree, TRUE, TRUE, 0);
                gtk_widget_show_all (GTK_WIDGET (tree));
        }
        g_strfreev (splited_conf);
}

static void
ario_browser_trees_changed_cb (guint notification_id,
                               ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ario_browser_reload_trees (browser);
        ario_browser_fill_first (browser);
}

static void
ario_browser_state_changed_cb (ArioMpd *mpd,
                               ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->connected != ario_mpd_is_connected (mpd))
                ario_browser_fill_first (browser);

        browser->priv->connected = ario_mpd_is_connected (mpd);
}

static void
ario_browser_dbtime_changed_cb (ArioMpd *mpd,
                                ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (!ario_mpd_get_updating (browser->priv->mpd))
                ario_browser_fill_first (browser);
}

static void
ario_browser_fill_first (ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START

        if (browser->priv->trees && browser->priv->trees->data)
                ario_tree_fill (ARIO_TREE (browser->priv->trees->data));
}

static void
ario_browser_tree_selection_changed_cb (ArioTree *tree,
                                        ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        ArioTree *next_tree = NULL;
        GSList *tmp, *criterias;

        for (tmp = browser->priv->trees; tmp; tmp = g_slist_next (tmp)) {
                if (tmp->data == tree && g_list_next (tmp)) {
                        next_tree = g_slist_next (tmp)->data;
                }
        }
        g_return_if_fail (next_tree);

        ario_tree_clear_criterias (next_tree);
        criterias = ario_tree_get_criterias (tree);
        for (tmp = criterias; tmp; tmp = g_slist_next (tmp)) {
                ario_tree_add_criteria (next_tree, tmp->data);
        }

        ario_tree_fill (next_tree);
}

static void
ario_browser_menu_popup_cb (ArioTree *tree,
                            ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        browser->priv->popup_tree = tree;
}

static void
ario_browser_cmd_add (GtkAction *action,
                      ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree)
                ario_tree_cmd_add (browser->priv->popup_tree, FALSE);
}

static void
ario_browser_cmd_add_play (GtkAction *action,
                           ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree)
                ario_tree_cmd_add (browser->priv->popup_tree, TRUE);
}

static void
ario_browser_cmd_clear_add_play (GtkAction *action,
                                 ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree) {
                ario_mpd_clear (browser->priv->mpd);
                ario_tree_cmd_add (browser->priv->popup_tree, TRUE);
        }
}

static void
ario_browser_cmd_get_cover (GtkAction *action,
                            ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree)
                ario_tree_cmd_get_cover (browser->priv->popup_tree);
}

static void
ario_browser_cmd_remove_cover (GtkAction *action,
                               ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree)
                ario_tree_cmd_remove_cover (browser->priv->popup_tree);
}

static void
ario_browser_cmd_albums_properties (GtkAction *action,
                                    ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree)
                ario_tree_cmd_albums_properties (browser->priv->popup_tree);
}

static void
ario_browser_cmd_songs_properties (GtkAction *action,
                                   ArioBrowser *browser)
{
        ARIO_LOG_FUNCTION_START
        if (browser->priv->popup_tree)
                ario_tree_cmd_songs_properties (browser->priv->popup_tree);
}
