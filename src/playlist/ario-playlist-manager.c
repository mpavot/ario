/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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
#include "lib/ario-conf.h"
#include "playlist/ario-playlist-manager.h"
#include "playlist/ario-playlist-normal.h"
#include "playlist/ario-playlist-queue.h"
#include "playlist/ario-playlist-dynamic.h"
#include "servers/ario-server.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_playlist_manager_class_init (ArioPlaylistManagerClass *klass);
static void ario_playlist_manager_init (ArioPlaylistManager *playlist_manager);

struct ArioPlaylistManagerPrivate
{
        GSList *modes;
};

G_DEFINE_TYPE_WITH_CODE (ArioPlaylistManager, ario_playlist_manager, G_TYPE_OBJECT, G_ADD_PRIVATE(ArioPlaylistManager))

static void
ario_playlist_manager_class_init (ArioPlaylistManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START;
}

static void
ario_playlist_manager_init (ArioPlaylistManager *playlist_manager)
{
        ARIO_LOG_FUNCTION_START;
        playlist_manager->priv = ario_playlist_manager_get_instance_private (playlist_manager);
}

static void
ario_playlist_manager_song_changed_cb (ArioServer *server,
                                       ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistMode *mode;
        ArioServerSong *song = ario_server_get_current_song ();
        const gchar *id = ario_conf_get_string (PREF_PLAYLIST_MODE, PREF_PLAYLIST_MODE_DEFAULT);

        mode = ario_playlist_manager_get_mode_from_id (ario_playlist_manager_get_instance (), id);

        ario_playlist_mode_next_song (mode, playlist);

        if (song
            && (song->pos == ario_server_get_current_playlist_length () - 1)) {
                if (mode)
                        ario_playlist_mode_last_song (mode, playlist);
        }
}

ArioPlaylistManager *
ario_playlist_manager_get_instance (void)
{
        ARIO_LOG_FUNCTION_START;
        static ArioPlaylistManager *playlist_manager = NULL;

        if (!playlist_manager) {
                ArioPlaylistMode *playlist_mode;

                playlist_manager = g_object_new (TYPE_ARIO_PLAYLIST_MANAGER,
                                                 NULL);
                g_return_val_if_fail (playlist_manager->priv != NULL, NULL);

                playlist_mode = ario_playlist_normal_new ();
                ario_playlist_manager_add_mode (playlist_manager,
                                                playlist_mode);

                playlist_mode = ario_playlist_queue_new ();
                ario_playlist_manager_add_mode (playlist_manager,
                                                playlist_mode);

                playlist_mode = ario_playlist_dynamic_new ();
                ario_playlist_manager_add_mode (playlist_manager,
                                                playlist_mode);

                g_signal_connect (ario_server_get_instance (),
                                  "song_changed",
                                  G_CALLBACK (ario_playlist_manager_song_changed_cb),
                                  playlist_manager);
        }

        return ARIO_PLAYLIST_MANAGER (playlist_manager);
}

GSList*
ario_playlist_manager_get_modes (ArioPlaylistManager *playlist_manager)
{
        ARIO_LOG_FUNCTION_START;
        return playlist_manager->priv->modes;
}

static gint
ario_playlist_manager_compare_modes (ArioPlaylistMode *playlist_mode,
                                     const gchar *id)
{
        return strcmp (ario_playlist_mode_get_id (playlist_mode), id);
}

ArioPlaylistMode*
ario_playlist_manager_get_mode_from_id (ArioPlaylistManager *playlist_manager,
                                        const gchar *id)
{
        ARIO_LOG_FUNCTION_START;
        GSList *found;

        if (!id)
                return NULL;

        found = g_slist_find_custom (playlist_manager->priv->modes,
                                     id,
                                     (GCompareFunc) ario_playlist_manager_compare_modes);
        if (!found)
                return NULL;

        return ARIO_PLAYLIST_MODE (found->data);
}


void
ario_playlist_manager_add_mode (ArioPlaylistManager *playlist_manager,
                                ArioPlaylistMode *playlist_mode)
{
        ARIO_LOG_FUNCTION_START;
        playlist_manager->priv->modes = g_slist_append (playlist_manager->priv->modes, playlist_mode);
}

void
ario_playlist_manager_remove_mode (ArioPlaylistManager *playlist_manager,
                                   ArioPlaylistMode *playlist_mode)
{
        ARIO_LOG_FUNCTION_START;
        playlist_manager->priv->modes = g_slist_remove (playlist_manager->priv->modes, playlist_mode);
}
