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
#include "lib/ario-conf.h"
#include "sources/ario-browser.h"
#include "sources/ario-search.h"
#include "sources/ario-storedplaylists.h"
#include "preferences/ario-preferences.h"
#include "widgets/ario-playlist.h"
#include "ario-debug.h"

static void ario_sourcemanager_sync (ArioSourceManager *sourcemanager);
static void ario_sourcemanager_showtabs_changed_cb (guint notification_id,
                                                    ArioSourceManager *sourcemanager);
static gboolean ario_sourcemanager_button_press_cb (GtkWidget *widget,
                                                    GdkEventButton *event,
                                                    ArioSourceManager *sourcemanager);
static gboolean ario_sourcemanager_switch_page_cb (GtkNotebook *notebook,
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

#define ARIO_SOURCEMANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SOURCEMANAGER, ArioSourceManagerPrivate))
G_DEFINE_TYPE (ArioSourceManager, ario_sourcemanager, GTK_TYPE_NOTEBOOK)

static void
ario_sourcemanager_class_init (ArioSourceManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioSourceManagerPrivate));
}

static void
ario_sourcemanager_init (ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START

        sourcemanager->priv = ARIO_SOURCEMANAGER_GET_PRIVATE (sourcemanager);
}

GtkWidget *
ario_sourcemanager_get_instance (GtkUIManager *mgr,
                                 GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *source;

        if (instance)
                return GTK_WIDGET (instance);

        instance = g_object_new (TYPE_ARIO_SOURCEMANAGER,
                                 NULL);
        g_return_val_if_fail (instance->priv != NULL, NULL);

        instance->priv->ui_manager = mgr;
        instance->priv->group = group;

        source = ario_browser_new (mgr,
                                   group);
        ario_sourcemanager_append (ARIO_SOURCE (source));

#ifdef ENABLE_SEARCH
        source = ario_search_new (mgr,
                                  group);
        ario_sourcemanager_append (ARIO_SOURCE (source));
#endif  /* ENABLE_SEARCH */
#ifdef ENABLE_STOREDPLAYLISTS
        source = ario_storedplaylists_new (mgr,
                                           group);
        ario_sourcemanager_append (ARIO_SOURCE (source));
#endif  /* ENABLE_STOREDPLAYLISTS */

        g_signal_connect (instance,
                          "button_press_event",
                          G_CALLBACK (ario_sourcemanager_button_press_cb),
                          instance);

        g_signal_connect_after (instance,
                                "switch-page",
                                G_CALLBACK (ario_sourcemanager_switch_page_cb),
                                instance);

        ario_sourcemanager_reorder ();

        ario_conf_notification_add (PREF_SHOW_TABS,
                                    (ArioNotifyFunc) ario_sourcemanager_showtabs_changed_cb,
                                    instance);

        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (instance),
                                    ario_conf_get_boolean (PREF_SHOW_TABS, PREF_SHOW_TABS_DEFAULT));

        return GTK_WIDGET (instance);
}

static void
ario_sourcemanager_shutdown_foreach (ArioSource *source,
                                     GSList **ordered_sources)
{
        ARIO_LOG_FUNCTION_START
        ario_source_shutdown (source);
        *ordered_sources = g_slist_append (*ordered_sources, ario_source_get_id (source));
}

void
ario_sourcemanager_shutdown (void)
{
        ARIO_LOG_FUNCTION_START
        GSList *ordered_sources = NULL;

        ario_conf_set_integer (PREF_SOURCE,
                               gtk_notebook_get_current_page (GTK_NOTEBOOK (instance)));

        gtk_container_foreach (GTK_CONTAINER (instance), (GtkCallback) ario_sourcemanager_shutdown_foreach, &ordered_sources);
        ario_conf_set_string_slist (PREF_SOURCE_LIST, ordered_sources);
        g_slist_free (ordered_sources);
}

void
ario_sourcemanager_goto_playling_song (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->source) {
                ario_source_goto_playling_song (instance->priv->source);
        }
}

void
ario_sourcemanager_reorder (void)
{
        ARIO_LOG_FUNCTION_START
        int i = 0;
        ArioSourceData *data;
        GSList *ordered_tmp;
        GSList *sources_tmp;
        GSList *ordered_sources = ario_conf_get_string_slist (PREF_SOURCE_LIST, PREF_SOURCE_LIST_DEFAULT);

        for (ordered_tmp = ordered_sources; ordered_tmp; ordered_tmp = g_slist_next (ordered_tmp)) {
                for (sources_tmp = instance->priv->sources; sources_tmp; sources_tmp = g_slist_next (sources_tmp)) {
                        data = sources_tmp->data;
                        if (!strcmp (ario_source_get_id (data->source), ordered_tmp->data)) {
                                gtk_notebook_reorder_child (GTK_NOTEBOOK (instance),
                                                            GTK_WIDGET (data->source), i);
                                break;
                        }
                }
                ++i;
        }

        g_slist_foreach (ordered_sources, (GFunc) g_free, NULL);
        g_slist_free (ordered_sources);

        ario_sourcemanager_sync (instance);
}

static void
ario_sourcemanager_sync (ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START
#ifdef MULTIPLE_VIEW
        gint page;

        page = ario_conf_get_integer (PREF_SOURCE, PREF_SOURCE_DEFAULT);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (sourcemanager), page);
#endif  /* MULTIPLE_VIEW */
}

static void
ario_sourcemanager_showtabs_changed_cb (guint notification_id,
                                        ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sourcemanager),
                                    ario_conf_get_boolean (PREF_SHOW_TABS, PREF_SHOW_TABS_DEFAULT));
}

static void
ario_sourcemanager_set_source_active (ArioSource *source,
                                      gboolean active)
{
        ARIO_LOG_FUNCTION_START
        gchar *conf_name;
        GtkAction *action;

        conf_name = g_strconcat (ario_source_get_id (source), "-active", NULL);
        ario_conf_set_boolean (conf_name, active);
        g_free (conf_name);

        if (active) {
                gtk_widget_set_no_show_all (GTK_WIDGET (source), FALSE);
                gtk_widget_show_all (GTK_WIDGET (source));
                gtk_widget_set_no_show_all (GTK_WIDGET (source), TRUE);
        } else {
                gtk_widget_hide (GTK_WIDGET (source));
        }

        action = gtk_action_group_get_action (instance->priv->group, ario_source_get_id (source));
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);
}

static void
ario_sourcemanager_menu_source_cb (GtkToggleAction *action,
                                   ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        ario_sourcemanager_set_source_active (source, gtk_toggle_action_get_active (action));
}

static void
ario_sourcemanager_menu_cb (GtkCheckMenuItem *checkmenuitem,
                            ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        ario_sourcemanager_set_source_active (source, gtk_check_menu_item_get_active (checkmenuitem));
}

void
ario_sourcemanager_append (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *hbox;
        gchar *conf_name;
        ArioSourceData *data;
        guint ui_merge_id;

        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (ario_source_get_icon (source), GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (ario_source_get_name (source)),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (instance),
                                  GTK_WIDGET (source),
                                  hbox);

        gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (instance),
                                          GTK_WIDGET (source),
                                          TRUE);

        conf_name = g_strconcat (ario_source_get_id (source), "-active", NULL);

        if (ario_conf_get_boolean (conf_name, TRUE))
                gtk_widget_show_all (GTK_WIDGET (source));
        else
                gtk_widget_hide (GTK_WIDGET (source));
        gtk_widget_set_no_show_all (GTK_WIDGET (source), TRUE);

        ui_merge_id = gtk_ui_manager_new_merge_id (instance->priv->ui_manager);
        gtk_ui_manager_add_ui (instance->priv->ui_manager,
                               ui_merge_id,
                               "/MenuBar/ViewMenu/ViewMenuSourcesPlaceholder",
                               ario_source_get_id (source),
                               ario_source_get_id (source),
                               GTK_UI_MANAGER_MENUITEM,
                               FALSE);

        GtkToggleActionEntry actions [] =
        {
                { ario_source_get_id (source), NULL, ario_source_get_name (source), NULL,
                        NULL, G_CALLBACK (ario_sourcemanager_menu_source_cb), ario_conf_get_boolean (conf_name, TRUE) }
        };
        g_free (conf_name);
        gtk_action_group_add_toggle_actions (instance->priv->group,
                                             actions, 1,
                                             source);

        data = (ArioSourceData *) g_malloc (sizeof (ArioSourceData));
        data->source = source;
        data->ui_merge_id = ui_merge_id;

        instance->priv->sources = g_slist_append (instance->priv->sources, data);
}

void
ario_sourcemanager_remove (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioSourceData *data;

        ario_source_shutdown (source);

        if (instance->priv->source == source)
                instance->priv->source = NULL;

        for (tmp = instance->priv->sources; tmp; tmp = g_slist_next (tmp)) {
                data = tmp->data;
                if (data->source == source) {
                        instance->priv->sources = g_slist_remove (instance->priv->sources, data);
                        gtk_ui_manager_remove_ui (instance->priv->ui_manager, data->ui_merge_id);

                        gtk_action_group_remove_action (instance->priv->group,
                                                        gtk_action_group_get_action (instance->priv->group, ario_source_get_id (source)));
                        g_free (data);
                        break;
                }
        }
        gtk_container_remove (GTK_CONTAINER (instance), GTK_WIDGET (source));
}

static gboolean
ario_sourcemanager_button_press_cb (GtkWidget *widget,
                                    GdkEventButton *event,
                                    ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menu;
        GtkWidget *item;
        GSList *tmp;
        ArioSourceData *data;
        gchar *conf_name;

        if (event->button == 3) {
                menu = gtk_menu_new ();

                for (tmp = sourcemanager->priv->sources; tmp; tmp = g_slist_next (tmp)) {
                        data = tmp->data;

                        item = gtk_check_menu_item_new_with_label (ario_source_get_name (data->source));
                        conf_name = g_strconcat (ario_source_get_id (data->source), "-active", NULL);
                        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                                        ario_conf_get_boolean (conf_name, TRUE));
                        g_free (conf_name);
                        g_signal_connect (item, "toggled",
                                          G_CALLBACK (ario_sourcemanager_menu_cb), data->source);
                        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
                }

                gtk_widget_show_all (menu);
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }

        return FALSE;
}

static gboolean
ario_sourcemanager_switch_page_cb (GtkNotebook *notebook,
                                   GtkNotebookPage *notebook_page,
                                   gint page,
                                   ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START
        ArioSource *new_source;

        if (sourcemanager->priv->source) {
                ario_source_unselect (sourcemanager->priv->source);
        }

        new_source = ARIO_SOURCE (gtk_notebook_get_nth_page (notebook, page));
        if (new_source)
                ario_source_select (new_source);

        sourcemanager->priv->source = new_source;

        return FALSE;
}
