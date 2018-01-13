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

#define SEARCH_DELAY 250

static void ario_search_connectivity_changed_cb (ArioServer *server,
                                                 ArioSearch *search);
static void ario_search_entry_changed (GtkEntry *entry,
                                       ArioSearch *search);
static void ario_search_entry_clear (GtkEntry *entry,
                                     GtkEntryIconPosition icon_pos,
                                     GdkEvent *event,
                                     ArioSearch *search);
static gboolean ario_search_do_search (ArioSearch *search);

struct ArioSearchPrivate
{
        GtkWidget *searchs;

        GtkWidget *entry;
        GtkWidget *vbox;

        gboolean connected;

        guint event_id;
};

/* Actions */
static const GActionEntry ario_search_actions[] = {
        { "search-add-to-pl", ario_songlist_cmd_add_songlists },
        { "search-add-play", ario_songlist_cmd_add_play_songlists },
        { "search-replace-in-pl", ario_songlist_cmd_clear_add_play_songlists },
        { "search-properties", ario_songlist_cmd_songs_properties },
};
static guint ario_search_n_actions = G_N_ELEMENTS (ario_search_actions);

/* Object properties */
enum
{
        PROP_0,
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
        return "edit-find";
}

static void
ario_search_class_init (ArioSearchClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* Virtual ArioSource methods */
        source_class->get_id = ario_search_get_id;
        source_class->get_name = ario_search_get_name;
        source_class->get_icon = ario_search_get_icon;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioSearchPrivate));
}

static void
ario_search_init (ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        search->priv = ARIO_SEARCH_GET_PRIVATE (search);

        search->priv->connected = FALSE;

        /* Main vbox */
        search->priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width (GTK_CONTAINER (search->priv->vbox), 10);

        /* Search entry */
        search->priv->entry = gtk_entry_new ();
        gtk_entry_set_icon_from_icon_name (GTK_ENTRY (search->priv->entry),
                                           GTK_ENTRY_ICON_PRIMARY,
                                           "edit-clear");

        g_signal_connect (search->priv->entry,
                          "changed",
                          G_CALLBACK (ario_search_entry_changed),
                          search);

        g_signal_connect (search->priv->entry,
                          "icon-press",
                          G_CALLBACK (ario_search_entry_clear),
                          search);

        gtk_box_pack_start (GTK_BOX (search->priv->vbox),
                            search->priv->entry,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (search),
                            search->priv->vbox,
                            TRUE, TRUE, 0);
}

GtkWidget *
ario_search_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioSearch *search;

        search = g_object_new (TYPE_ARIO_SEARCH,
                               NULL);

        g_return_val_if_fail (search->priv != NULL, NULL);

        /* Signals to synchronize the search with server */
        g_signal_connect_object (ario_server_get_instance (),
                                 "state_changed", G_CALLBACK (ario_search_connectivity_changed_cb),
                                 search, 0);

        /* Search songs list */
        search->priv->searchs = ario_songlist_new (UI_PATH "ario-songlist-menu.ui",
                                                   "search-menu",
                                                   TRUE);

        /* Songs list widget */
        gtk_box_pack_start (GTK_BOX (search->priv->vbox),
                            search->priv->searchs,
                            TRUE, TRUE, 0);

        /* Add actions */
        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_search_actions,
                                         ario_search_n_actions,
                                         search->priv->searchs);

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
ario_search_entry_changed (GtkEntry *entry,
                           ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        if (search->priv->event_id > 0)
                g_source_remove (search->priv->event_id);
        search->priv->event_id = g_timeout_add (SEARCH_DELAY, (GSourceFunc) ario_search_do_search, search);
}

static void
ario_search_entry_clear (GtkEntry *entry,
                         GtkEntryIconPosition icon_pos,
                         GdkEvent *event,
                         ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        /* Clear search entry */
        gtk_entry_set_text (GTK_ENTRY (search->priv->entry), "");
}

static gboolean
ario_search_do_search (ArioSearch *search)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerAtomicCriteria *atomic_criteria;
        GSList *criteria = NULL;
        GSList *tmp;
        GSList *songs;
        ArioServerSong *song;
        GtkTreeIter iter;
        gchar *title;
        GtkListStore *liststore;
        int i, j;
        gchar **cmp_str;
        gboolean tagged_search;
        gchar *sep, *tag, *value;
        gchar **items;
        gint len;

        /* Split on spaces to have multiple filters */
        cmp_str = g_strsplit (gtk_entry_get_text (GTK_ENTRY (search->priv->entry)), " ", -1);
        if (!cmp_str)
                return FALSE;

        /* Loop on every filter */
        for (i = 0; cmp_str[i]; ++i) {
                len = strlen(cmp_str[i]);
                /* Only keep words of at least 3 chars for performance reasons */
                if (g_utf8_collate (cmp_str[i], "")
                    && len > 2)
                {
                        /* Check if we are in the case of a search by tag (like title:foo or artist:bar) */
                        tagged_search = FALSE;
                        sep = g_strstr_len (cmp_str[i], len, ":");
                        if (sep)
                        {
                                if (sep - cmp_str[i] == len - 1)
                                {
                                        /* separator is the last character (for example: 'title:') 
                                         * We don't take this string into account */
                                        tagged_search = TRUE;
                                }
                                else
                                {
                                        tag = g_strndup (cmp_str[i], sep - cmp_str[i]);
                                        value = g_strdup (sep + 1);

                                        items = ario_server_get_items_names ();
                                        for (j = 0; j < ARIO_TAG_COUNT; ++j) {
                                                if (items[j] && tag
                                                    && (! g_ascii_strcasecmp (tag, items[j])
                                                        || ! g_ascii_strcasecmp (tag, gettext (items[j]))))
                                                {
                                                        atomic_criteria = (ArioServerAtomicCriteria *) g_malloc (sizeof (ArioServerAtomicCriteria));
                                                        atomic_criteria->tag = j;
                                                        atomic_criteria->value = g_strdup (value);
                                                        criteria = g_slist_append (criteria, atomic_criteria);
                                                        tagged_search = TRUE;
                                                        break;
                                                }
                                        }
                                }
                        }

                        /* Not a search by tag: search in any tag */
                        if (!tagged_search)
                        {
                                atomic_criteria = (ArioServerAtomicCriteria *) g_malloc (sizeof (ArioServerAtomicCriteria));
                                atomic_criteria->tag = ARIO_TAG_ANY;
                                atomic_criteria->value = g_strdup (cmp_str[i]);
                                criteria = g_slist_append (criteria, atomic_criteria);
                        }
                }
        }
        g_strfreev (cmp_str);

        /* Clear song list */
        liststore = ario_songlist_get_liststore (ARIO_SONGLIST (search->priv->searchs));
        gtk_list_store_clear (liststore);

        if (!criteria)
                return FALSE;

        /* Get songs corresponding to criteria */
        songs = ario_server_get_songs (criteria, FALSE);

        g_slist_foreach (criteria, (GFunc) g_free, NULL);
        g_slist_free (criteria);

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

        return FALSE;
}
#endif  /* ENABLE_SEARCH */

