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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <glib/gi18n.h>
#include "servers/ario-server-interface.h"
#include "ario-debug.h"

#define NORMAL_TIMEOUT 500
#define LAZY_TIMEOUT 12000

static void ario_server_interface_finalize (GObject *object);
static void ario_server_interface_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec);
static void ario_server_interface_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec);

enum
{
        PROP_0,
        PROP_SONGID,
        PROP_STATE,
        PROP_VOLUME,
        PROP_ELAPSED,
        PROP_PLAYLISTID,
        PROP_RANDOM,
        PROP_UPDATINGDB,
        PROP_REPEAT
};

G_DEFINE_TYPE (ArioServerInterface, ario_server_interface, G_TYPE_OBJECT)

static void
dummy_void_void (void)
{
}

static void
dummy_void_int (const int a)
{
}

static void
dummy_void_int_int (const int a,
                    const int b)
{
}

static void
dummy_void_pointer (const gpointer *a)
{
}

static void
dummy_void_pointer_int (const gpointer *a,
                        const int b)
{
}

static int
dummy_int_void (void)
{
        return 0;
}

static int
dummy_int_pointer (const gpointer *a)
{
        return 0;
}

static unsigned long
dummy_ulong_void (void)
{
        return 0;
}

static gpointer
dummy_pointer_void (void)
{
        return NULL;
}

static gpointer
dummy_pointer_pointer (const gpointer *a)
{
        return NULL;
}

static gpointer
dummy_pointer_pointer_int (const gpointer *a,
                           const int b)
{
        return NULL;
}

static gpointer
dummy_pointer_tag_pointer (const ArioServerTag a,
                           const gpointer *b)
{
        return NULL;
}

static  gpointer
dummy_pointer_int  (int playlist_id)
{
        return NULL;
}

static void
ario_server_interface_class_init (ArioServerInterfaceClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_server_interface_finalize;

        object_class->set_property = ario_server_interface_set_property;
        object_class->get_property = ario_server_interface_get_property;

        klass->connect = dummy_void_void;
        klass->disconnect = dummy_void_void;
        klass->is_connected = dummy_int_void;
        klass->update_status = dummy_int_void;
        klass->update_db = dummy_void_void;
        klass->list_tags = (GSList* (*) (const ArioServerTag, const ArioServerCriteria *)) dummy_pointer_tag_pointer;
        klass->get_albums = (GSList* (*) (const ArioServerCriteria *)) dummy_pointer_pointer;
        klass->get_songs = (GSList* (*) (const ArioServerCriteria *, const gboolean)) dummy_pointer_pointer_int;
        klass->get_songs_from_playlist = (GSList* (*) (char *)) dummy_pointer_pointer;
        klass->get_playlists = (GSList* (*) (void)) dummy_pointer_void;
        klass->get_playlist_changes = (GSList* (*) (int))dummy_pointer_int;
        klass->get_current_song_on_server = (ArioServerSong* (*) (void)) dummy_pointer_void;
        klass->get_current_playlist_total_time = dummy_int_void;
        klass->get_last_update = dummy_ulong_void;
        klass->do_next = dummy_void_void;
        klass->do_prev = dummy_void_void;
        klass->do_play = dummy_void_void;
        klass->do_play_pos = dummy_void_int;
        klass->do_pause = dummy_void_void;
        klass->do_stop = dummy_void_void;
        klass->set_current_elapsed = dummy_void_int;
        klass->set_current_volume = dummy_void_int;
        klass->set_current_random = dummy_void_int;
        klass->set_current_repeat = dummy_void_int;
        klass->set_crossfadetime = dummy_void_int;
        klass->clear = dummy_void_void;
        klass->queue_commit = dummy_void_void;
        klass->insert_at = (void (*) (const GSList *, const gint)) dummy_void_pointer_int;
        klass->save_playlist = (int (*) (const char *)) dummy_int_pointer;
        klass->delete_playlist = (void (*) (const char *)) dummy_void_pointer;
        klass->get_outputs = (GSList* (*) (void)) dummy_pointer_void;
        klass->enable_output = dummy_void_int_int;
        klass->get_stats = (ArioServerStats* (*) (void)) dummy_pointer_void;
        klass->get_songs_info = (GList* (*) (GSList *)) dummy_pointer_pointer;
        klass->list_files = (ArioServerFileList* (*) (const char *, const int)) dummy_pointer_pointer_int;

        g_object_class_install_property (object_class,
                                         PROP_SONGID,
                                         g_param_spec_int ("song_id",
                                                           "song_id",
                                                           "song_id",
                                                           0, INT_MAX, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_STATE,
                                         g_param_spec_int ("state",
                                                           "state",
                                                           "state",
                                                           0, 3, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_VOLUME,
                                         g_param_spec_int ("volume",
                                                           "volume",
                                                           "volume",
                                                           -1, 100, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_ELAPSED,
                                         g_param_spec_int ("elapsed",
                                                           "elapsed",
                                                           "elapsed",
                                                           0, INT_MAX, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_PLAYLISTID,
                                         g_param_spec_int ("playlist_id",
                                                           "playlist_id",
                                                           "playlist_id",
                                                           -1, INT_MAX, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_RANDOM,
                                         g_param_spec_boolean ("random",
                                                               "random",
                                                               "random",
                                                               FALSE,
                                                               G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_REPEAT,
                                         g_param_spec_boolean ("repeat",
                                                               "repeat",
                                                               "repeat",
                                                               FALSE,
                                                               G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_UPDATINGDB,
                                         g_param_spec_int ("updatingdb",
                                                           "updatingdb",
                                                           "updatingdb",
                                                           -1, INT_MAX, 0,
                                                           G_PARAM_READWRITE));
}

static void
ario_server_interface_init (ArioServerInterface *server_interface)
{
        ARIO_LOG_FUNCTION_START
        server_interface->song_id = -1;
        server_interface->playlist_id = -1;
        server_interface->volume = -1;
}

static void
ario_server_interface_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioServerInterface *server_interface;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SERVER_INTERFACE (object));

        server_interface = ARIO_SERVER_INTERFACE (object);

        if (server_interface->server_song)
                ario_server_free_song (server_interface->server_song);

        G_OBJECT_CLASS (ario_server_interface_parent_class)->finalize (object);
}

static void
ario_server_interface_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioServerInterface *server_interface = ARIO_SERVER_INTERFACE (object);
        int song_id;

        switch (prop_id) {
        case PROP_SONGID:
                song_id = g_value_get_int (value);
                if (server_interface->song_id != song_id) {
                        server_interface->signals_to_emit |= SERVER_SONG_CHANGED_FLAG;
                        server_interface->song_id = song_id;
                }

                /* check if there is a connection */
                if (ario_server_is_connected ()) {
                        ArioServerSong *new_song;
                        ArioServerSong *old_song = server_interface->server_song;
                        gboolean state_changed;
                        gboolean artist_changed = FALSE;
                        gboolean album_changed = FALSE;

                        new_song = ario_server_get_current_song_on_server ();
                        state_changed = (!old_song || !new_song);
                        if (!state_changed) {
                                artist_changed = ( (old_song->artist && !new_song->artist)
                                                   || (!old_song->artist && new_song->artist)
                                                   || (old_song->artist && new_song->artist && g_utf8_collate (old_song->artist, new_song->artist)) );
                                if (!artist_changed) {
                                        album_changed = ( (old_song->album && !new_song->album)
                                                          || (!old_song->album && new_song->album)
                                                          || (old_song->album && new_song->album && g_utf8_collate (old_song->album, new_song->album)) );
                                }
                        }

                        if (state_changed || artist_changed || album_changed)
                                server_interface->signals_to_emit |= SERVER_ALBUM_CHANGED_FLAG;

                        if (server_interface->server_song)
                                ario_server_free_song (server_interface->server_song);

                        if (new_song) {
                                server_interface->server_song = new_song;
                        } else {
                                server_interface->server_song = NULL;
                        }
                } else {
                        if (server_interface->server_song) {
                                ario_server_free_song (server_interface->server_song);
                                server_interface->server_song = NULL;
                        }
                }
                break;
        case PROP_STATE:
                server_interface->state = g_value_get_int (value);
                server_interface->signals_to_emit |= SERVER_STATE_CHANGED_FLAG;
                break;
        case PROP_VOLUME:
                server_interface->volume = g_value_get_int (value);
                server_interface->signals_to_emit |= SERVER_VOLUME_CHANGED_FLAG;
                break;
        case PROP_ELAPSED:
                server_interface->elapsed = g_value_get_int (value);
                server_interface->signals_to_emit |= SERVER_ELAPSED_CHANGED_FLAG;
                break;
        case PROP_PLAYLISTID:
                server_interface->playlist_id = g_value_get_int (value);
                if (!ario_server_is_connected ())
                        server_interface->playlist_length = 0;
                server_interface->signals_to_emit |= SERVER_PLAYLIST_CHANGED_FLAG;
                break;
        case PROP_RANDOM:
                server_interface->random = g_value_get_boolean (value);
                server_interface->signals_to_emit |= SERVER_RANDOM_CHANGED_FLAG;
                break;
        case PROP_REPEAT:
                server_interface->repeat = g_value_get_boolean (value);
                server_interface->signals_to_emit |= SERVER_REPEAT_CHANGED_FLAG;
                break;
        case PROP_UPDATINGDB:
                server_interface->updatingdb = g_value_get_int (value);
                server_interface->signals_to_emit |= SERVER_UPDATINGDB_CHANGED_FLAG;
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_server_interface_get_property (GObject *object,
                          guint prop_id,
                          GValue *value,
                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioServerInterface *server_interface = ARIO_SERVER_INTERFACE (object);

        switch (prop_id) {
        case PROP_SONGID:
                g_value_set_int (value, server_interface->song_id);
                break;
        case PROP_STATE:
                g_value_set_int (value, server_interface->state);
                break;
        case PROP_VOLUME:
                g_value_set_int (value, server_interface->volume);
                break;
        case PROP_ELAPSED:
                g_value_set_int (value, server_interface->elapsed);
                break;
        case PROP_PLAYLISTID:
                g_value_set_int (value, server_interface->playlist_id);
                break;
        case PROP_RANDOM:
                g_value_set_boolean (value, server_interface->random);
                break;
        case PROP_REPEAT:
                g_value_set_boolean (value, server_interface->repeat);
                break;
        case PROP_UPDATINGDB:
                g_value_set_int (value, server_interface->updatingdb);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

void
ario_server_interface_set_default (ArioServerInterface *server_interface)
{
        ARIO_LOG_FUNCTION_START
        if (server_interface->song_id != 0)
                g_object_set (G_OBJECT (server_interface), "song_id", 0, NULL);

        if (server_interface->state != MPD_STATUS_STATE_UNKNOWN)
                g_object_set (G_OBJECT (server_interface), "state", MPD_STATUS_STATE_UNKNOWN, NULL);

        if (server_interface->volume != MPD_STATUS_NO_VOLUME)
                g_object_set (G_OBJECT (server_interface), "volume", MPD_STATUS_NO_VOLUME, NULL);

        if (server_interface->elapsed != 0)
                g_object_set (G_OBJECT (server_interface), "elapsed", 0, NULL);

        g_object_set (G_OBJECT (server_interface), "playlist_id", -1, NULL);

        if (server_interface->random != FALSE)
                g_object_set (G_OBJECT (server_interface), "random", FALSE, NULL);

        if (server_interface->repeat != FALSE)
                g_object_set (G_OBJECT (server_interface), "repeat", FALSE, NULL);

        if (server_interface->updatingdb != 0)
                g_object_set (G_OBJECT (server_interface), "updatingdb", 0, NULL);
}

void
ario_server_interface_emit (ArioServerInterface *server_interface,
                            ArioServer *server)
{
        ARIO_LOG_FUNCTION_START
        if (server_interface->signals_to_emit & SERVER_SONG_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "song_changed");
        if (server_interface->signals_to_emit & SERVER_ALBUM_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "album_changed");
        if (server_interface->signals_to_emit & SERVER_STATE_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "state_changed");
        if (server_interface->signals_to_emit & SERVER_VOLUME_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "volume_changed", server_interface->volume);
        if (server_interface->signals_to_emit & SERVER_ELAPSED_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "elapsed_changed", server_interface->elapsed);
        if (server_interface->signals_to_emit & SERVER_PLAYLIST_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "playlist_changed");
        if (server_interface->signals_to_emit & SERVER_RANDOM_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "random_changed");
        if (server_interface->signals_to_emit & SERVER_REPEAT_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "repeat_changed");
        if (server_interface->signals_to_emit & SERVER_UPDATINGDB_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (server), "updatingdb_changed");
        server_interface->signals_to_emit = 0;
}
