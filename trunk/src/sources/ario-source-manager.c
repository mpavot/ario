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
#include "lib/eel-gconf-extensions.h"
#include "sources/ario-source-manager.h"
#include "sources/ario-browser.h"
#include "sources/ario-radio.h"
#include "sources/ario-search.h"
#include "sources/ario-storedplaylists.h"
#include "sources/ario-filesystem.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_sourcemanager_class_init (ArioSourceManagerClass *klass);
static void ario_sourcemanager_init (ArioSourceManager *sourcemanager);
static void ario_sourcemanager_finalize (GObject *object);
static void ario_sourcemanager_order (ArioSourceManager *sourcemanager);
static void ario_sourcemanager_sync (ArioSourceManager *sourcemanager, GtkActionGroup *group);
static void ario_sourcemanager_showtabs_changed_cb (GConfClient *client,
                                             guint cnxn_id,
                                             GConfEntry *entry,
                                             ArioSourceManager *sourcemanager);

struct ArioSourceManagerPrivate
{
        GSList *sources;
        GtkActionGroup *group;
};

static GObjectClass *parent_class = NULL;

GType
ario_sourcemanager_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioSourceManagerClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_sourcemanager_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioSourceManager),
                        0,
                        (GInstanceInitFunc) ario_sourcemanager_init
                };

                type = g_type_register_static (GTK_TYPE_NOTEBOOK,
                                               "ArioSourceManager",
                                               &our_info, 0);
        }
        return type;
}

static void
ario_sourcemanager_class_init (ArioSourceManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_sourcemanager_finalize;
}

static void
ario_sourcemanager_init (ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START

        sourcemanager->priv = g_new0 (ArioSourceManagerPrivate, 1);
}

static void
ario_sourcemanager_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioSourceManager *sourcemanager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SOURCEMANAGER (object));

        sourcemanager = ARIO_SOURCEMANAGER (object);

        g_return_if_fail (sourcemanager->priv != NULL);
        g_free (sourcemanager->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
ario_sourcemanager_new (GtkUIManager *mgr,
                 GtkActionGroup *group,
                 ArioMpd *mpd,
                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioSourceManager *sourcemanager;
        GtkWidget *source;

        sourcemanager = g_object_new (TYPE_ARIO_SOURCEMANAGER,
                               NULL);
        g_return_val_if_fail (sourcemanager->priv != NULL, NULL);

        sourcemanager->priv->group = group;

        source = ario_browser_new (mgr,
                                   group,
                                   mpd,
                                   playlist);
        ario_sourcemanager_append (sourcemanager,
                                   ARIO_SOURCE (source));

#ifdef ENABLE_RADIOS
        source = ario_radio_new (mgr,
                                 group,
                                 mpd,
                                 playlist);
        ario_sourcemanager_append (sourcemanager,
                                   ARIO_SOURCE (source));
#endif  /* ENABLE_RADIOS */
#ifdef ENABLE_SEARCH
        source = ario_search_new (mgr,
                                  group,
                                  mpd,
                                  playlist);
        ario_sourcemanager_append (sourcemanager,
                                   ARIO_SOURCE (source));
#endif  /* ENABLE_SEARCH */
#ifdef ENABLE_STOREDPLAYLISTS
        source = ario_storedplaylists_new (mgr,
                                           group,
                                           mpd,
                                           playlist);
        ario_sourcemanager_append (sourcemanager,
                                   ARIO_SOURCE (source));
#endif  /* ENABLE_STOREDPLAYLISTS */

        source = ario_filesystem_new (mgr,
                                      group,
                                      mpd,
                                      playlist);
        ario_sourcemanager_append (sourcemanager,
                                   ARIO_SOURCE (source));

        gtk_widget_show_all (GTK_WIDGET (sourcemanager));

        ario_sourcemanager_order (sourcemanager);
        ario_sourcemanager_sync (sourcemanager, group);

        eel_gconf_notification_add (CONF_SHOW_TABS,
                                    (GConfClientNotifyFunc) ario_sourcemanager_showtabs_changed_cb,
                                    sourcemanager);

        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sourcemanager),
                                    eel_gconf_get_boolean (CONF_SHOW_TABS, TRUE));

        return GTK_WIDGET (sourcemanager);
}

static void
ario_sourcemanager_shutdown_foreach (ArioSource *source,
                                     GSList **ordered_sources)
{
        ario_source_shutdown (source);
        *ordered_sources = g_slist_append (*ordered_sources, ario_source_get_id (source));
}

void
ario_sourcemanager_shutdown (ArioSourceManager *sourcemanager)
{
        GSList *ordered_sources = NULL;

        eel_gconf_set_integer (CONF_SOURCE,
                               gtk_notebook_get_current_page (GTK_NOTEBOOK (sourcemanager)));

        gtk_container_foreach (GTK_CONTAINER (sourcemanager), (GFunc) ario_sourcemanager_shutdown_foreach, &ordered_sources);
        eel_gconf_set_string_slist (CONF_SOURCE_LIST, ordered_sources);
        g_slist_free (ordered_sources);
}

static void
ario_sourcemanager_order (ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START
        int i = 0;
        ArioSource *source;
        GSList *ordered_tmp;
        GSList *sources_tmp;
        GSList *ordered_sources = eel_gconf_get_string_slist (CONF_SOURCE_LIST);

        for (ordered_tmp = ordered_sources; ordered_tmp; ordered_tmp = g_slist_next (ordered_tmp)) {
                for (sources_tmp = sourcemanager->priv->sources; sources_tmp; sources_tmp = g_slist_next (sources_tmp)) {
                        source = sources_tmp->data;
                        if (!strcmp (ario_source_get_id (source), ordered_tmp->data)) {
                                gtk_notebook_reorder_child (GTK_NOTEBOOK (sourcemanager),
                                                            GTK_WIDGET (source), i);
                        }
                }
                ++i;
        }

        g_slist_foreach (ordered_sources, (GFunc) g_free, NULL);
        g_slist_free (ordered_sources);
}

static void
ario_sourcemanager_sync (ArioSourceManager *sourcemanager,
                                       GtkActionGroup *group)
{
        ARIO_LOG_FUNCTION_START
#ifdef MULTIPLE_VIEW
        gint page;

        page = eel_gconf_get_integer (CONF_SOURCE, ARIO_SOURCE_BROWSER);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (sourcemanager), page);
#endif  /* MULTIPLE_VIEW */
}

static void
ario_sourcemanager_showtabs_changed_cb (GConfClient *client,
                                 guint cnxn_id,
                                 GConfEntry *entry,
                                 ArioSourceManager *sourcemanager)
{
        ARIO_LOG_FUNCTION_START
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sourcemanager),
                                    eel_gconf_get_boolean (CONF_SHOW_TABS, TRUE));
}

void
ario_sourcemanager_append (ArioSourceManager *sourcemanager,
                           ArioSource *source)
{
        GtkWidget *hbox;
        sourcemanager->priv->sources = g_slist_append (sourcemanager->priv->sources, source);

        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (ario_source_get_icon (source), GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (ario_source_get_name (source)),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (sourcemanager),
                                  GTK_WIDGET (source),
                                  hbox);
        gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (sourcemanager),
                                          GTK_WIDGET (source),
                                          TRUE);
}

