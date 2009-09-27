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

#include "sources/ario-source-manager.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "sources/ario-browser.h"
#include "sources/ario-search.h"
#include "sources/ario-storedplaylists.h"
#include "widgets/ario-playlist.h"

static void ario_source_manager_sync (ArioSourceManager *sourcemanager);
static void ario_source_manager_showtabs_changed_cb (guint notification_id,
                                                    ArioSourceManager *sourcemanager);
static gboolean ario_source_manager_button_press_cb (GtkWidget *widget,
                                                    GdkEventButton *event,
                                                    ArioSourceManager *sourcemanager);
static gboolean ario_source_manager_switch_page_cb (GtkNotebook *notebook,
                                                   GtkNotebookPage *notebook_page,
                                                   gint page,
                                                   ArioSourceManager *sourcemanager);

struct ArioSourceManagerPrivate
{
        GSList *sources;
        GtkUIManager *ui_manager;

        ArioSource *source;
        GtkActionGroup *group;
};

static ArioSourceManager *instance = NULL;

typedef struct ArioSourceData
{
        ArioSource *source;
        guint ui_merge_id;
} ArioSourceData;

#define ARIO_SOURCE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ARIO_TYPE_SOURCE_MANAGER, ArioSourceManagerPrivate))
G_DEFINE_TYPE (ArioSourceManager, ario_source_manager, GTK_TYPE_NOTEBOOK)

static void
ario_source_manager_class_init (ArioSourceManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioSourceManagerPrivate));
}

static void
ario_source_manager_init (ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START;
        sourcemanager->priv = ARIO_SOURCE_MANAGER_GET_PRIVATE (sourcemanager);
}

GtkWidget *
ario_source_manager_get_instance (GtkUIManager *mgr,
                                 GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *source;

        /* Returns singleton if already instantiated */
        if (instance)
                return GTK_WIDGET (instance);

        instance = g_object_new (ARIO_TYPE_SOURCE_MANAGER,
                                 NULL);
        g_return_val_if_fail (instance->priv != NULL, NULL);

        instance->priv->ui_manager = mgr;
        instance->priv->group = group;

        /* Create browser */
        source = ario_browser_new (mgr,
                                   group);
        ario_source_manager_append (ARIO_SOURCE (source));

#ifdef ENABLE_SEARCH
        /* Create search */
        source = ario_search_new (mgr,
                                  group);
        ario_source_manager_append (ARIO_SOURCE (source));
#endif  /* ENABLE_SEARCH */
#ifdef ENABLE_STOREDPLAYLISTS
        /* Create stored playlists source */
        source = ario_storedplaylists_new (mgr,
                                           group);
        ario_source_manager_append (ARIO_SOURCE (source));
#endif  /* ENABLE_STOREDPLAYLISTS */

        /* Connect signlas for actions on notebook */
        g_signal_connect (instance,
                          "button_press_event",
                          G_CALLBACK (ario_source_manager_button_press_cb),
                          instance);

        g_signal_connect_after (instance,
                                "switch-page",
                                G_CALLBACK (ario_source_manager_switch_page_cb),
                                instance);

        /* Reorder sources according to preferences */
        ario_source_manager_reorder ();

        /* Notification for preference changes */
        ario_conf_notification_add (PREF_SHOW_TABS,
                                    (ArioNotifyFunc) ario_source_manager_showtabs_changed_cb,
                                    instance);

        /* Set tabs visibility according to preferences */
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (instance),
                                    ario_conf_get_boolean (PREF_SHOW_TABS, PREF_SHOW_TABS_DEFAULT));

        return GTK_WIDGET (instance);
}

static void
ario_source_manager_shutdown_foreach (ArioSource *source,
                                     GSList **ordered_sources)
{
        ARIO_LOG_FUNCTION_START;
        /* Shutdown source */
        ario_source_shutdown (source);

        /* Add source to ordered list */
        *ordered_sources = g_slist_append (*ordered_sources, ario_source_get_id (source));
}

void
ario_source_manager_shutdown (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *ordered_sources = NULL;

        /* Save current active page */
        ario_conf_set_integer (PREF_SOURCE,
                               gtk_notebook_get_current_page (GTK_NOTEBOOK (instance)));

        /* Shutdown each source and get an ordered list of sources */
        gtk_container_foreach (GTK_CONTAINER (instance),
                               (GtkCallback) ario_source_manager_shutdown_foreach,
                               &ordered_sources);

        /* Save ordered list of sources */
        ario_conf_set_string_slist (PREF_SOURCE_LIST, ordered_sources);
        g_slist_free (ordered_sources);
}

void
ario_source_manager_goto_playling_song (void)
{
        ARIO_LOG_FUNCTION_START;
        /* Go to playing song on active source */
        if (instance->priv->source) {
                ario_source_goto_playling_song (instance->priv->source);
        }
}

void
ario_source_manager_reorder (void)
{
        ARIO_LOG_FUNCTION_START;
        int i = 0;
        ArioSourceData *data;
        GSList *ordered_tmp;
        GSList *sources_tmp;
        GSList *ordered_sources = ario_conf_get_string_slist (PREF_SOURCE_LIST, PREF_SOURCE_LIST_DEFAULT);

        /* For each source in preferences */
        for (ordered_tmp = ordered_sources; ordered_tmp; ordered_tmp = g_slist_next (ordered_tmp)) {
                /* For each registered source */
                for (sources_tmp = instance->priv->sources; sources_tmp; sources_tmp = g_slist_next (sources_tmp)) {
                        data = sources_tmp->data;
                        if (!strcmp (ario_source_get_id (data->source), ordered_tmp->data)) {
                                /* Move source tab according to preferences */
                                gtk_notebook_reorder_child (GTK_NOTEBOOK (instance),
                                                            GTK_WIDGET (data->source), i);
                                break;
                        }
                }
                ++i;
        }

        g_slist_foreach (ordered_sources, (GFunc) g_free, NULL);
        g_slist_free (ordered_sources);

        ario_source_manager_sync (instance);
}

static void
ario_source_manager_sync (ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START;
        gint page;

        /* Select active page according to preferences */
        page = ario_conf_get_integer (PREF_SOURCE, PREF_SOURCE_DEFAULT);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (sourcemanager), page);
}

static void
ario_source_manager_showtabs_changed_cb (guint notification_id,
                                        ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START;
        /* Set tabs visibility */
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sourcemanager),
                                    ario_conf_get_boolean (PREF_SHOW_TABS, PREF_SHOW_TABS_DEFAULT));
}

static void
ario_source_manager_set_source_active (ArioSource *source,
                                      gboolean active)
{
        ARIO_LOG_FUNCTION_START;
        gchar *conf_name;
        GtkAction *action;

        /* Set source activity in preferences */
        conf_name = g_strconcat (ario_source_get_id (source), "-active", NULL);
        ario_conf_set_boolean (conf_name, active);
        g_free (conf_name);

        if (active) {
                /* Show source */
                gtk_widget_set_no_show_all (GTK_WIDGET (source), FALSE);
                gtk_widget_show_all (GTK_WIDGET (source));
                gtk_widget_set_no_show_all (GTK_WIDGET (source), TRUE);
        } else {
                /* Hide source */
                gtk_widget_hide (GTK_WIDGET (source));
        }

        /* Select source in menu */
        action = gtk_action_group_get_action (instance->priv->group, ario_source_get_id (source));
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);
}

static void
ario_source_manager_menu_source_cb (GtkToggleAction *action,
                                   ArioSource *source)
{
        ARIO_LOG_FUNCTION_START;
        /* Select source as in menu */
        ario_source_manager_set_source_active (source, gtk_toggle_action_get_active (action));
}

static void
ario_source_manager_menu_cb (GtkCheckMenuItem *checkmenuitem,
                            ArioSource *source)
{
        ARIO_LOG_FUNCTION_START;
        /* Select source as in menu */
        ario_source_manager_set_source_active (source, gtk_check_menu_item_get_active (checkmenuitem));
}

void
ario_source_manager_append (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *hbox;
        gchar *conf_name;
        ArioSourceData *data;
        guint ui_merge_id;

        /* Create hbox for tab header */
        hbox = gtk_hbox_new (FALSE, 4);

        /* Add source icon to hbox */
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (ario_source_get_icon (source), GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);

        /* Add source nqme to hbox */
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (ario_source_get_name (source)),
                            TRUE, TRUE, 0);

        /* Append source to source-manager */
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (instance),
                                  GTK_WIDGET (source),
                                  hbox);

        gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (instance),
                                          GTK_WIDGET (source),
                                          TRUE);

        /* Show/hide source depending on preferences */
        conf_name = g_strconcat (ario_source_get_id (source), "-active", NULL);
        if (ario_conf_get_boolean (conf_name, TRUE))
                gtk_widget_show_all (GTK_WIDGET (source));
        else
                gtk_widget_hide (GTK_WIDGET (source));
        gtk_widget_set_no_show_all (GTK_WIDGET (source), TRUE);

        /* Add source in menu for selection */
        ui_merge_id = gtk_ui_manager_new_merge_id (instance->priv->ui_manager);
        gtk_ui_manager_add_ui (instance->priv->ui_manager,
                               ui_merge_id,
                               "/MenuBar/ViewMenu/ViewMenuSourcesPlaceholder",
                               ario_source_get_id (source),
                               ario_source_get_id (source),
                               GTK_UI_MANAGER_MENUITEM,
                               FALSE);

        /* Add source to popup menu for selection */
        GtkToggleActionEntry actions [] =
        {
                { ario_source_get_id (source), NULL, ario_source_get_name (source), NULL,
                        NULL, G_CALLBACK (ario_source_manager_menu_source_cb), ario_conf_get_boolean (conf_name, TRUE) }
        };
        g_free (conf_name);
        gtk_action_group_add_toggle_actions (instance->priv->group,
                                             actions, 1,
                                             source);

        /* Add source data to list */
        data = (ArioSourceData *) g_malloc (sizeof (ArioSourceData));
        data->source = source;
        data->ui_merge_id = ui_merge_id;

        instance->priv->sources = g_slist_append (instance->priv->sources, data);
}

void
ario_source_manager_remove (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp;
        ArioSourceData *data;

        /* Shutdown source */
        ario_source_shutdown (source);

        if (instance->priv->source == source)
                instance->priv->source = NULL;

        /* For each source */
        for (tmp = instance->priv->sources; tmp; tmp = g_slist_next (tmp)) {
                data = tmp->data;
                /* Get source to remove */
                if (data->source == source) {
                        /* Remove the source from the list */
                        instance->priv->sources = g_slist_remove (instance->priv->sources, data);

                        /* Remove the source from UI manager */
                        gtk_ui_manager_remove_ui (instance->priv->ui_manager, data->ui_merge_id);

                        /* Remove the source from popup menu */
                        gtk_action_group_remove_action (instance->priv->group,
                                                        gtk_action_group_get_action (instance->priv->group, ario_source_get_id (source)));
                        g_free (data);
                        break;
                }
        }
        /* Remove source from notebook */
        gtk_container_remove (GTK_CONTAINER (instance), GTK_WIDGET (source));
}

static gboolean
ario_source_manager_button_press_cb (GtkWidget *widget,
                                    GdkEventButton *event,
                                    ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;
        GtkWidget *item;
        GSList *tmp;
        ArioSourceData *data;
        gchar *conf_name;

        if (event->button == 3) {
                /* Third button: Show popup menu */
                menu = gtk_menu_new ();

                for (tmp = sourcemanager->priv->sources; tmp; tmp = g_slist_next (tmp)) {
                        /* Build menu with each source */
                        data = tmp->data;

                        item = gtk_check_menu_item_new_with_label (ario_source_get_name (data->source));
                        conf_name = g_strconcat (ario_source_get_id (data->source), "-active", NULL);
                        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                                        ario_conf_get_boolean (conf_name, TRUE));
                        g_free (conf_name);

                        /* Connect signal for activation/deactivation of sources in popup menu */
                        g_signal_connect (item, "toggled",
                                          G_CALLBACK (ario_source_manager_menu_cb), data->source);
                        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
                }

                /* Show popup menu */
                gtk_widget_show_all (menu);
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                                gtk_get_current_event_time ());
        }

        return FALSE;
}

static gboolean
ario_source_manager_switch_page_cb (GtkNotebook *notebook,
                                   GtkNotebookPage *notebook_page,
                                   gint page,
                                   ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START;
        ArioSource *new_source;

        /* Call unselect on previous source */
        if (sourcemanager->priv->source) {
                ario_source_unselect (sourcemanager->priv->source);
        }

        /* Call select on new source */
        new_source = ARIO_SOURCE (gtk_notebook_get_nth_page (notebook, page));
        if (new_source)
                ario_source_select (new_source);

        sourcemanager->priv->source = new_source;

        return FALSE;
}
