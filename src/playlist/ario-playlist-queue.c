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
#include "playlist/ario-playlist-queue.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_playlist_queue_class_init (ArioPlaylistQueueClass *klass);
static void ario_playlist_queue_init (ArioPlaylistQueue *playlist_queue);
static void ario_playlist_queue_next_song (ArioPlaylistMode *playlist_mode,
                                           ArioPlaylist *playlist);

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (ArioPlaylistQueue, ario_playlist_queue, ARIO_TYPE_PLAYLIST_MODE)

static gchar *
ario_playlist_queue_get_id (ArioPlaylistMode *playlist_mode)
{
        return "queue";
}

static gchar *
ario_playlist_queue_get_name (ArioPlaylistMode *playlist_mode)
{
        return _("Queue Mode");
}

static void
ario_playlist_queue_class_init (ArioPlaylistQueueClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistModeClass *playlist_mode_class = ARIO_PLAYLIST_MODE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        playlist_mode_class->get_id = ario_playlist_queue_get_id;
        playlist_mode_class->get_name = ario_playlist_queue_get_name;
        playlist_mode_class->next_song = ario_playlist_queue_next_song;
}

static void
ario_playlist_queue_init (ArioPlaylistQueue *playlist_queue)
{
        ARIO_LOG_FUNCTION_START;
}

ArioPlaylistMode*
ario_playlist_queue_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistQueue *queue;

        queue = g_object_new (TYPE_ARIO_PLAYLIST_QUEUE,
                              NULL);

        return ARIO_PLAYLIST_MODE (queue);
}

static void
ario_playlist_queue_next_song (ArioPlaylistMode *playlist_mode,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerSong *song = ario_server_get_current_song ();
        int i;
        int state = ario_server_get_current_state ();

        if (state != ARIO_STATE_PLAY
            && state != ARIO_STATE_PAUSE)
                return;
        if (!song)
                return;
        if (!song->pos)
                return;
        for (i=0; i<song->pos; ++i) {
                ario_server_queue_delete_pos (0);
        }
        ario_server_queue_commit ();
}

