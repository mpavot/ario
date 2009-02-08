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
static void ario_playlist_queue_finalize (GObject *object);
static void ario_playlist_queue_next_song (ArioPlaylistMode *playlist_mode,
                                           ArioPlaylist *playlist);

struct ArioPlaylistQueuePrivate
{
        gboolean dummy;
};

static GObjectClass *parent_class = NULL;

GType
ario_playlist_queue_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioPlaylistQueueClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_playlist_queue_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioPlaylistQueue),
                        0,
                        (GInstanceInitFunc) ario_playlist_queue_init
                };

                type = g_type_register_static (ARIO_TYPE_PLAYLIST_MODE,
                                               "ArioPlaylistQueue",
                                               &our_info, 0);
        }
        return type;
}

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
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPlaylistModeClass *playlist_mode_class = ARIO_PLAYLIST_MODE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_playlist_queue_finalize;

        playlist_mode_class->get_id = ario_playlist_queue_get_id;
        playlist_mode_class->get_name = ario_playlist_queue_get_name;
        playlist_mode_class->next_song = ario_playlist_queue_next_song;
}

static void
ario_playlist_queue_init (ArioPlaylistQueue *playlist_queue)
{
        ARIO_LOG_FUNCTION_START
        playlist_queue->priv = g_new0 (ArioPlaylistQueuePrivate, 1);
}

static void
ario_playlist_queue_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistQueue *playlist_queue;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_PLAYLIST_QUEUE (object));

        playlist_queue = ARIO_PLAYLIST_QUEUE (object);

        g_return_if_fail (playlist_queue->priv != NULL);
        g_free (playlist_queue->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioPlaylistMode*
ario_playlist_queue_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistQueue *queue;

        queue = g_object_new (TYPE_ARIO_PLAYLIST_QUEUE,
                              NULL);

        g_return_val_if_fail (queue->priv != NULL, NULL);

        return ARIO_PLAYLIST_MODE (queue);
}

static void
ario_playlist_queue_next_song (ArioPlaylistMode *playlist_mode,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioServerSong *song = ario_server_get_current_song ();
        int i;
        int state = ario_server_get_current_state ();

        if (state != MPD_STATUS_STATE_PLAY
            && state != MPD_STATUS_STATE_PAUSE)
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

