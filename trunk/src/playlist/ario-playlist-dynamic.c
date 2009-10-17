/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "playlist/ario-playlist-dynamic.h"
#include "shell/ario-shell-similarartists.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_playlist_dynamic_class_init (ArioPlaylistDynamicClass *klass);
static void ario_playlist_dynamic_init (ArioPlaylistDynamic *playlist_dynamic);
static void ario_playlist_dynamic_last_song (ArioPlaylistMode *playlist_mode,
                                             ArioPlaylist *playlist);
static GtkWidget* ario_playlist_dynamic_get_config (ArioPlaylistMode *playlist_mode);

static GObjectClass *parent_class = NULL;

typedef enum
{
        SONGS_FROM_SAME_ARTIST,
        SONGS_FROM_SAME_ALBUM,
        SONGS_FROM_SIMILAR_ARTISTS,
        ALBUMS_FROM_SAME_ARTIST,
        ALBUMS_FROM_SIMILAR_ARTISTS
} ArioDynamicType;

static const char *dynamic_type[] = {
        N_("songs of same artist"),
        N_("songs of same album"),
        N_("songs of similar artists"),
        N_("albums of same artists"),
        N_("albums of similar artists"),
        NULL
};

#define ARIO_PLAYLIST_DYNAMIC_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PLAYLIST_DYNAMIC, ArioPlaylistDynamicPrivate))
G_DEFINE_TYPE (ArioPlaylistDynamic, ario_playlist_dynamic, ARIO_TYPE_PLAYLIST_MODE)

static gchar *
ario_playlist_dynamic_get_id (ArioPlaylistMode *playlist_mode)
{
        return "dynamic";
}

static gchar *
ario_playlist_dynamic_get_name (ArioPlaylistMode *playlist_mode)
{
        return _("Dynamic Playlist");
}

static void
ario_playlist_dynamic_class_init (ArioPlaylistDynamicClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistModeClass *playlist_mode_class = ARIO_PLAYLIST_MODE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        playlist_mode_class->get_id = ario_playlist_dynamic_get_id;
        playlist_mode_class->get_name = ario_playlist_dynamic_get_name;
        playlist_mode_class->last_song = ario_playlist_dynamic_last_song;
        playlist_mode_class->get_config = ario_playlist_dynamic_get_config;
}

static void
ario_playlist_dynamic_init (ArioPlaylistDynamic *playlist_dynamic)
{
        ARIO_LOG_FUNCTION_START;
}

ArioPlaylistMode*
ario_playlist_dynamic_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistDynamic *dynamic;

        dynamic = g_object_new (TYPE_ARIO_PLAYLIST_DYNAMIC,
                                NULL);

        return ARIO_PLAYLIST_MODE (dynamic);
}

static void
ario_playlist_dynamic_last_song (ArioPlaylistMode *playlist_mode,
                                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        GSList *artists = NULL;
        GSList *albums = NULL, *tmp, *tmp_artist;
        ArioServerAtomicCriteria atomic_criteria1;
        ArioServerAtomicCriteria atomic_criteria2;
        ArioServerCriteria *criteria = NULL;
        GSList *criterias = NULL;
        int nbitems = ario_conf_get_integer (PREF_DYNAMIC_NBITEMS, PREF_DYNAMIC_NBITEMS_DEFAULT);

        switch (ario_conf_get_integer (PREF_DYNAMIC_TYPE, PREF_DYNAMIC_TYPE_DEFAULT)) {
        case SONGS_FROM_SAME_ARTIST:
                artists = g_slist_append (artists, ario_server_get_current_artist ());
                ario_server_playlist_append_artists (artists, PLAYLIST_ADD, nbitems);
                g_slist_free (artists);
                break;
        case SONGS_FROM_SAME_ALBUM:
                atomic_criteria1.tag = MPD_TAG_ITEM_ARTIST;
                atomic_criteria1.value = ario_server_get_current_artist ();
                atomic_criteria2.tag = MPD_TAG_ITEM_ALBUM;
                atomic_criteria2.value = ario_server_get_current_album ();

                criteria = g_slist_append (criteria, &atomic_criteria1);
                criteria = g_slist_append (criteria, &atomic_criteria2);

                criterias = g_slist_append (criterias, criteria);

                ario_server_playlist_append_criterias (criterias, PLAYLIST_ADD, nbitems);

                g_slist_free (criteria);
                g_slist_free (criterias);
                break;
        case SONGS_FROM_SIMILAR_ARTISTS:
                ario_shell_similarartists_add_similar_to_playlist (ario_server_get_current_artist (),
                                                                   nbitems);
                break;
        case ALBUMS_FROM_SAME_ARTIST:
                atomic_criteria1.tag = MPD_TAG_ITEM_ARTIST;
                atomic_criteria1.value = ario_server_get_current_artist ();

                criteria = g_slist_append (criteria, &atomic_criteria1);
                albums = ario_server_get_albums (criteria);
                g_slist_free (criteria);

                tmp = ario_util_gslist_randomize (&albums, nbitems);
                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
                albums = tmp;

                for (tmp = albums; tmp; tmp = g_slist_next (tmp)) {
                        ArioServerAlbum *album = tmp->data;

                        atomic_criteria1.tag = MPD_TAG_ITEM_ARTIST;
                        atomic_criteria1.value = album->artist;
                        atomic_criteria2.tag = MPD_TAG_ITEM_ALBUM;
                        atomic_criteria2.value = album->album;

                        criteria = NULL;
                        criteria = g_slist_append (criteria, &atomic_criteria1);
                        criteria = g_slist_append (criteria, &atomic_criteria2);

                        criterias = NULL;
                        criterias = g_slist_append (criterias, criteria);

                        ario_server_playlist_append_criterias (criterias, PLAYLIST_ADD, -1);

                        g_slist_free (criteria);
                        g_slist_free (criterias);
                }

                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
                break;
        case ALBUMS_FROM_SIMILAR_ARTISTS:
                artists = ario_shell_similarartists_get_similar_artists (ario_server_get_current_artist ());
                for (tmp_artist = artists; tmp_artist; tmp_artist = g_slist_next (tmp_artist)) {
                        ArioSimilarArtist *artist = tmp_artist->data;
                        atomic_criteria1.tag = MPD_TAG_ITEM_ARTIST;
                        atomic_criteria1.value = (gchar*) artist->name;

                        criteria = NULL;
                        criteria = g_slist_append (criteria, &atomic_criteria1);
                        albums = g_slist_concat (albums, ario_server_get_albums (criteria));
                        g_slist_free (criteria);
                }
                g_slist_foreach (artists, (GFunc) ario_shell_similarartists_free_similarartist, NULL);
                g_slist_free (artists);

                tmp = ario_util_gslist_randomize (&albums, nbitems);
                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
                albums = tmp;

                for (tmp = albums; tmp; tmp = g_slist_next (tmp)) {
                        ArioServerAlbum *album = tmp->data;

                        atomic_criteria1.tag = MPD_TAG_ITEM_ARTIST;
                        atomic_criteria1.value = album->artist;
                        atomic_criteria2.tag = MPD_TAG_ITEM_ALBUM;
                        atomic_criteria2.value = album->album;

                        criteria = NULL;
                        criteria = g_slist_append (criteria, &atomic_criteria1);
                        criteria = g_slist_append (criteria, &atomic_criteria2);

                        criterias = NULL;
                        criterias = g_slist_append (criterias, criteria);

                        ario_server_playlist_append_criterias (criterias, PLAYLIST_ADD, -1);

                        g_slist_free (criteria);
                        g_slist_free (criterias);
                }
                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
                break;
        default:
                break;
        }
}

static void
ario_playlist_dynamic_type_combobox_changed_cb (GtkComboBox *combobox,
                                                ArioPlaylistMode *playlist_mode)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        int type;

        gtk_combo_box_get_active_iter (combobox, &iter);
        gtk_tree_model_get (gtk_combo_box_get_model (combobox),
                            &iter,
                            1, &type,
                            -1);
        ario_conf_set_integer (PREF_DYNAMIC_TYPE, type);
}

static void
ario_playlist_dynamic_nbitems_changed_cb (GtkWidget *widget,
                                          ArioPlaylistMode *playlist_mode)
{
        ARIO_LOG_FUNCTION_START;
        gdouble nbitems = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
        ario_conf_set_integer (PREF_DYNAMIC_NBITEMS, (int) nbitems);
}

static GtkWidget*
ario_playlist_dynamic_get_config (ArioPlaylistMode *playlist_mode)
{
        GtkWidget *hbox;
        GtkWidget *spinbutton;
        GtkWidget *combobox;
        GtkAdjustment *adj;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;
        int nbitems;

        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new ("Automatically add"),
                            FALSE, FALSE,
                            0);

        adj = GTK_ADJUSTMENT (gtk_adjustment_new (10,
                                                  0.0,
                                                  1000.0,
                                                  1.0,
                                                  10.0,
                                                  0.0));

        spinbutton = gtk_spin_button_new (adj,
                                          1.0, 0);
        nbitems = ario_conf_get_integer (PREF_DYNAMIC_NBITEMS, PREF_DYNAMIC_NBITEMS_DEFAULT);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton), (double) nbitems);

        g_signal_connect (G_OBJECT (spinbutton),
                          "value_changed",
                          G_CALLBACK (ario_playlist_dynamic_nbitems_changed_cb), playlist_mode);

        gtk_box_pack_start (GTK_BOX (hbox),
                            spinbutton,
                            FALSE, FALSE,
                            0);

        combobox = gtk_combo_box_new ();

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
        for (i = 0; dynamic_type[i]; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (dynamic_type[i]),
                                    1, i,
                                    -1);
        }
        gtk_combo_box_set_model (GTK_COMBO_BOX (combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0, NULL);

        gtk_combo_box_set_active (GTK_COMBO_BOX (combobox),
                                  ario_conf_get_integer (PREF_DYNAMIC_TYPE, PREF_DYNAMIC_TYPE_DEFAULT));

        g_signal_connect (G_OBJECT (combobox),
                          "changed",
                          G_CALLBACK (ario_playlist_dynamic_type_combobox_changed_cb), playlist_mode);

        gtk_box_pack_start (GTK_BOX (hbox),
                            combobox,
                            FALSE, FALSE,
                            0);

        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new ("to playlist."),
                            FALSE, FALSE,
                            0);
        return hbox;
}
