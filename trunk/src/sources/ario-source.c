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
#include "sources/ario-source.h"
#include "sources/ario-browser.h"
#include "sources/ario-radio.h"
#include "sources/ario-search.h"
#include "sources/ario-storedplaylists.h"
#include "sources/ario-filesystem.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_source_class_init (ArioSourceClass *klass);
static void ario_source_init (ArioSource *source);
static void ario_source_finalize (GObject *object);
static void ario_source_sync_source (ArioSource *source);
static gboolean ario_source_page_changed_cb (GtkNotebook *notebook,
                                             GtkNotebookPage *page,
                                             gint page_nb,
                                             ArioSource *source);
static void ario_source_showtabs_changed_cb (GConfClient *client,
                                             guint cnxn_id,
                                             GConfEntry *entry,
                                             ArioSource *source);
struct ArioSourcePrivate
{
        GtkWidget *browser;
        GtkWidget *radio;
        GtkWidget *search;
        GtkWidget *storedplaylists;
        GtkWidget *filesystem;

        gint page;
};

static GObjectClass *parent_class = NULL;

enum
{
        SOURCE_CHANGED,
        LAST_SIGNAL
};
static guint ario_source_signals[LAST_SIGNAL] = { 0 };

GType
ario_source_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioSourceClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_source_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioSource),
                        0,
                        (GInstanceInitFunc) ario_source_init
                };

                type = g_type_register_static (GTK_TYPE_NOTEBOOK,
                                               "ArioSource",
                                               &our_info, 0);
        }
        return type;
}

static void
ario_source_class_init (ArioSourceClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_source_finalize;

        ario_source_signals[SOURCE_CHANGED] =
                g_signal_new ("source_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioSourceClass, source_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
ario_source_init (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START

        source->priv = g_new0 (ArioSourcePrivate, 1);
}

static void
ario_source_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioSource *source;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SOURCE (object));

        source = ARIO_SOURCE (object);

        g_return_if_fail (source->priv != NULL);
        g_free (source->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
ario_source_new (GtkUIManager *mgr,
                 GtkActionGroup *group,
                 ArioMpd *mpd,
                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioSource *source;
        GtkWidget *hbox;

        source = g_object_new (TYPE_ARIO_SOURCE,
                               NULL);
        g_return_val_if_fail (source->priv != NULL, NULL);

        source->priv->browser = ario_browser_new (mgr,
                                                  group,
                                                  mpd,
                                                  playlist);
        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (GTK_STOCK_HOME, GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (_("Library")),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (source),
                                  source->priv->browser,
                                  hbox);
#ifdef ENABLE_RADIOS
        source->priv->radio = ario_radio_new (mgr,
                                              group,
                                              mpd,
                                              playlist);
        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (GTK_STOCK_NETWORK, GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (_("Web Radios")),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (source),
                                  source->priv->radio,
                                  hbox);
#endif  /* ENABLE_RADIOS */
#ifdef ENABLE_SEARCH
        source->priv->search = ario_search_new (mgr,
                                                group,
                                                mpd,
                                                playlist);
        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            /* Translators - This "Search" is a name (like in "a search"), not a verb */
                            gtk_label_new (_("Search")),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (source),
                                  source->priv->search,                                  
                                  hbox);
#endif  /* ENABLE_SEARCH */
#ifdef ENABLE_STOREDPLAYLISTS
        source->priv->storedplaylists = ario_storedplaylists_new (mgr,
                                                                  group,
                                                                  mpd,
                                                                  playlist);
        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (_("Playlists")),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (source),
                                  source->priv->storedplaylists,
                                  hbox);
#endif  /* ENABLE_STOREDPLAYLISTS */
        source->priv->filesystem = ario_filesystem_new (mgr,
                                                        group,
                                                        mpd,
                                                        playlist);
        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_image_new_from_stock (GTK_STOCK_HARDDISK, GTK_ICON_SIZE_MENU),
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new (_("File System")),
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);
        gtk_notebook_append_page (GTK_NOTEBOOK (source),
                                  source->priv->filesystem,
                                  hbox);

        gtk_widget_show_all (GTK_WIDGET (source));
        ario_source_sync_source (source);

        eel_gconf_notification_add (CONF_SHOW_TABS,
                                    (GConfClientNotifyFunc) ario_source_showtabs_changed_cb,
                                    source);
        g_signal_connect_object (G_OBJECT (source),
                                 "switch_page",
                                 G_CALLBACK (ario_source_page_changed_cb),
                                 source, 0);

        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (source),
                                    eel_gconf_get_boolean (CONF_SHOW_TABS, TRUE));

        return GTK_WIDGET (source);
}

void
ario_source_shutdown (ArioSource *source)
{
        eel_gconf_set_integer (CONF_SOURCE,
                               gtk_notebook_get_current_page (GTK_NOTEBOOK (source)));
#ifdef ENABLE_STOREDPLAYLISTS
        ario_storedplaylists_shutdown (ARIO_STOREDPLAYLISTS (source->priv->storedplaylists));
#endif  /* ENABLE_STOREDPLAYLISTS */
        ario_filesystem_shutdown (ARIO_FILESYSTEM (source->priv->filesystem));
}

static void
ario_source_sync_source (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
#ifdef MULTIPLE_VIEW
        source->priv->page = eel_gconf_get_integer (CONF_SOURCE, ARIO_SOURCE_BROWSER);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (source), source->priv->page);
#endif  /* MULTIPLE_VIEW */
}

void
ario_source_set_page (ArioSource *source,
                      gint page)
{
        ARIO_LOG_FUNCTION_START
        gtk_notebook_set_current_page (GTK_NOTEBOOK (source), page);
}

gint
ario_source_get_page (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        return source->priv->page;
}

static gboolean
ario_source_page_changed_cb (GtkNotebook *notebook,
                             GtkNotebookPage *page,
                             gint page_nb,
                             ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        source->priv->page = page_nb;
        g_signal_emit (G_OBJECT (source), ario_source_signals[SOURCE_CHANGED], 0);

        return TRUE;
}

static void
ario_source_showtabs_changed_cb (GConfClient *client,
                                 guint cnxn_id,
                                 GConfEntry *entry,
                                 ArioSource *source)
{
        ARIO_LOG_FUNCTION_START
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (source),
                                    eel_gconf_get_boolean (CONF_SHOW_TABS, TRUE));
}

