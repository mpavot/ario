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
#include <limits.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "servers/ario-xmms.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

#define NORMAL_TIMEOUT 500
#define LAZY_TIMEOUT 12000

static void ario_xmms_finalize (GObject *object);
static gboolean ario_xmms_connect_to (ArioXmms *xmms,
                                      gchar *hostname,
                                      int port,
                                      float timeout);
static gpointer ario_xmms_connect (void);
static void ario_xmms_disconnect (void);
static void ario_xmms_update_db (void);
static gboolean ario_xmms_is_connected (void);
static GSList * ario_xmms_list_tags (const ArioServerTag tag,
                                     const ArioServerCriteria *criteria);
static GSList * ario_xmms_get_albums (const ArioServerCriteria *criteria);
static GSList * ario_xmms_get_songs (const ArioServerCriteria *criteria,
                                     const gboolean exact);
static GSList * ario_xmms_get_songs_from_playlist (char *playlist);
static GSList * ario_xmms_get_playlists (void);
static GSList * ario_xmms_get_playlist_changes (int playlist_id);
static ArioServerSong * ario_xmms_get_current_song_on_server (void);
static int ario_xmms_get_current_playlist_total_time (void);
static unsigned long ario_xmms_get_last_update (void);
static void ario_xmms_do_next (void);
static void ario_xmms_do_prev (void);
static void ario_xmms_do_play (void);
static void ario_xmms_do_play_pos (gint id);
static void ario_xmms_do_pause (void);
static void ario_xmms_do_stop (void);
static void ario_xmms_set_current_elapsed (const gint elapsed);
static void ario_xmms_set_current_volume (const gint volume);
static void ario_xmms_set_current_random (const gboolean random);
static void ario_xmms_set_current_repeat (const gboolean repeat);
static void ario_xmms_set_crossfadetime (const int crossfadetime);
static void ario_xmms_clear (void);
static void ario_xmms_queue_commit (void);
static void ario_xmms_insert_at (const GSList *songs,
                                 const gint pos);
static int ario_xmms_save_playlist (const char *name);
static void ario_xmms_delete_playlist (const char *name);
static GSList * ario_xmms_get_outputs (void);
static void ario_xmms_enable_output (int id,
                                     gboolean enabled);
static ArioServerStats * ario_xmms_get_stats (void);
static GList * ario_xmms_get_songs_info (GSList *paths);
static ArioServerFileList * ario_xmms_list_files (const char *path,
                                                  gboolean recursive);

struct ArioXmmsPrivate
{
        xmmsc_connection_t *connection;
        int total_time;
};

char * ArioXmmsPattern[MPD_TAG_NUM_OF_ITEM_TYPES] =
{
        "artist",    // MPD_TAG_ITEM_ARTIST
        "album",     // MPD_TAG_ITEM_ALBUM
        "title",     // MPD_TAG_ITEM_TITLE
        "track",     // MPD_TAG_ITEM_TRACK
        NULL,            // MPD_TAG_ITEM_NAME
        "genre",     // MPD_TAG_ITEM_GENRE
        "date",      // MPD_TAG_ITEM_DATE
        "composer",  // MPD_TAG_ITEM_COMPOSER
        "performer", // MPD_TAG_ITEM_PERFORMER
        NULL,            // MPD_TAG_ITEM_COMMENT
        NULL,            // MPD_TAG_ITEM_DISC
        "filename",  // MPD_TAG_ITEM_FILENAME
        "any"        // MPD_TAG_ITEM_ANY
};

#define ARIO_XMMS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_XMMS, ArioXmmsPrivate))
G_DEFINE_TYPE (ArioXmms, ario_xmms, TYPE_ARIO_SERVER)

static ArioXmms *instance = NULL;
/*
static void
abcde (const void *key, xmmsc_result_value_type_t type, const void *value, void *user_data)
{
printf ("%s\n", key);
}
*/
static void
ario_xmms_class_init (ArioXmmsClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioServerClass *server_class = ARIO_SERVER_CLASS (klass);

        object_class->finalize = ario_xmms_finalize;

        server_class->connect = ario_xmms_connect;
        server_class->disconnect = ario_xmms_disconnect;
        server_class->is_connected = ario_xmms_is_connected;
        server_class->update_db = ario_xmms_update_db;
        server_class->list_tags = ario_xmms_list_tags;
        server_class->get_albums = ario_xmms_get_albums;
        server_class->get_songs = ario_xmms_get_songs;
        server_class->get_songs_from_playlist = ario_xmms_get_songs_from_playlist;
        server_class->get_playlists = ario_xmms_get_playlists;
        server_class->get_playlist_changes = ario_xmms_get_playlist_changes;
        server_class->get_current_song_on_server = ario_xmms_get_current_song_on_server;
        server_class->get_current_playlist_total_time = ario_xmms_get_current_playlist_total_time;
        server_class->get_last_update = ario_xmms_get_last_update;
        server_class->do_next = ario_xmms_do_next;
        server_class->do_prev = ario_xmms_do_prev;
        server_class->do_play = ario_xmms_do_play;
        server_class->do_play_pos = ario_xmms_do_play_pos;
        server_class->do_pause = ario_xmms_do_pause;
        server_class->do_stop = ario_xmms_do_stop;
        server_class->set_current_elapsed = ario_xmms_set_current_elapsed;
        server_class->set_current_volume = ario_xmms_set_current_volume;
        server_class->set_current_random = ario_xmms_set_current_random;
        server_class->set_current_repeat = ario_xmms_set_current_repeat;
        server_class->set_crossfadetime = ario_xmms_set_crossfadetime;
        server_class->clear = ario_xmms_clear;
        server_class->queue_commit = ario_xmms_queue_commit;
        server_class->insert_at = ario_xmms_insert_at;
        server_class->save_playlist = ario_xmms_save_playlist;
        server_class->delete_playlist = ario_xmms_delete_playlist;
        server_class->get_outputs = ario_xmms_get_outputs;
        server_class->enable_output = ario_xmms_enable_output;
        server_class->get_stats = ario_xmms_get_stats;
        server_class->get_songs_info = ario_xmms_get_songs_info;
        server_class->list_files = ario_xmms_list_files;

        g_type_class_add_private (klass, sizeof (ArioXmmsPrivate));
}

static void
ario_xmms_init (ArioXmms *xmms)
{
        ARIO_LOG_FUNCTION_START
        xmms->priv = ARIO_XMMS_GET_PRIVATE (xmms);
}

static void
ario_xmms_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioXmms *xmms;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_XMMS (object));

        xmms = ARIO_XMMS (object);
        g_return_if_fail (xmms->priv != NULL);

        if (xmms->priv->connection)
                xmmsc_unref (xmms->priv->connection);

        instance = NULL;
        G_OBJECT_CLASS (ario_xmms_parent_class)->finalize (object);
}

static ArioServerSong *
ario_xmms_get_song_from_res (xmmsc_result_t *result)
{
        ARIO_LOG_FUNCTION_START
        ArioServerSong *song;
        const gchar *tmp;
        int tracknr;

        if (!result)
                return NULL;

        song = (ArioServerSong *) g_malloc0 (sizeof (ArioServerSong));
        xmmsc_result_get_dict_entry_string (result, "genre", &tmp);
        song->genre = g_strdup (tmp);
        xmmsc_result_get_dict_entry_string (result, "artist", &tmp);
        song->artist = g_strdup (tmp);
        xmmsc_result_get_dict_entry_string (result, "album", &tmp);
        song->album = g_strdup (tmp);
        xmmsc_result_get_dict_entry_string (result, "title", &tmp);
        song->title = g_strdup (tmp);
        xmmsc_result_get_dict_entry_string (result, "url", &tmp);
        song->file = g_strdup (tmp);
        xmmsc_result_get_dict_entry_string (result, "track", &tmp);
        song->track = g_strdup (tmp);
        xmmsc_result_get_dict_entry_int (result, "id", &song->id);
        xmmsc_result_get_dict_entry_int (result, "duration", &song->time);
        song->time = song->time / 1000;
        xmmsc_result_get_dict_entry_int (result, "tracknr", &tracknr);
        song->track = g_strdup_printf ("%d", tracknr);

        return song;
}

static void
wait_cb (xmmsc_result_t *res,
         gboolean *wait)
{
        *wait = FALSE;
}

static void
ario_xmms_result_wait (xmmsc_result_t *result)
{
        ARIO_LOG_FUNCTION_START
        gboolean wait = TRUE;
        xmmsc_result_notifier_set (result, (xmmsc_result_notifier_t) wait_cb, &wait);

        while (wait)
                gtk_main_iteration ();
}

ArioXmms *
ario_xmms_get_instance (void)
{
        ARIO_LOG_FUNCTION_START
        if (!instance) {
                instance = g_object_new (TYPE_ARIO_XMMS, NULL);
                g_return_val_if_fail (instance->priv != NULL, NULL);
        }
        return instance;
}

static gboolean
playlist_not_idle (xmmsc_result_t *res)
{
        g_signal_emit_by_name (G_OBJECT (instance), "playlist_changed");
        return FALSE;
}

static void
playlist_not (xmmsc_result_t *res,
              ArioXmms *xmms)
{
        g_idle_add ((GSourceFunc) playlist_not_idle, res);
}

static gboolean
playback_status_not_idle (xmmsc_result_t *res)
{
        guint state = 0;

        if (xmmsc_result_iserror (res)) {
                ARIO_LOG_ERROR ("Result has error");
                return FALSE;
        }
        if (!xmmsc_result_get_uint (res, &state)) {
                ARIO_LOG_ERROR ("Result didn't contain right type!");
                return FALSE;
        }

        /* horrible hack: mpd state = xmms state + 1*/
        ++state;
        g_object_set (G_OBJECT (instance), "state", state, NULL);
        g_signal_emit_by_name (G_OBJECT (instance), "state_changed");

        return FALSE;
}

static void
playback_status_not (xmmsc_result_t *res,
                     ArioXmms *xmms)
{
        g_idle_add ((GSourceFunc) playback_status_not_idle, res);
}

static gboolean
playback_current_id_not_idle (xmmsc_result_t *res)
{
        guint id;

        if (xmmsc_result_iserror (res)) {
                ARIO_LOG_ERROR ("Result has error");
                return FALSE;
        }
        if (!xmmsc_result_get_uint (res, &id)) {
                ARIO_LOG_ERROR ("Result didn't contain right type!");
                return FALSE;
        }

        if (instance->parent.song_id != id)
                g_object_set (G_OBJECT (instance), "song_id", id, NULL);

        if (instance->parent.signals_to_emit & SERVER_SONG_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (instance), "song_changed");

        if (instance->parent.signals_to_emit & SERVER_ALBUM_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (instance), "album_changed");

        return FALSE;
}

static void
playback_current_id_not (xmmsc_result_t *res,
                         ArioXmms *xmms)
{
        g_idle_add ((GSourceFunc) playback_current_id_not_idle, res);
}

static gboolean
playback_volume_changed_not_idle (xmmsc_result_t *result)
{
        guint volume;
        xmmsc_result_t *res;

        res = xmmsc_playback_volume_get (instance->priv->connection);
        ario_xmms_result_wait (res);

        if (!xmmsc_result_get_dict_entry_uint (res, "left", &volume)) {
                ARIO_LOG_ERROR ("Result didn't contain right type!");
                return FALSE;
        }
        xmmsc_result_unref (res);
        if (instance->parent.volume != volume)
                g_object_set (G_OBJECT (instance), "volume", volume, NULL);

        if (instance->parent.signals_to_emit & SERVER_VOLUME_CHANGED_FLAG)
                g_signal_emit_by_name (G_OBJECT (instance), "volume_changed", volume);

        return FALSE;
}

static void
playback_volume_changed_not (xmmsc_result_t *res,
                             ArioXmms *xmms)
{
        g_idle_add ((GSourceFunc) playback_volume_changed_not_idle, res);
}

static void
playback_playtime_not (xmmsc_result_t *res,
                       ArioXmms *xmms)
{
        guint playtime;
        xmmsc_result_t *res2;

        if (xmmsc_result_iserror (res)) {
                ARIO_LOG_ERROR ("Result has error");
                return;
        }
        if (!xmmsc_result_get_uint (res, &playtime)) {
                ARIO_LOG_ERROR ("Result didn't contain right type!");
                return;
        }

        /* TODO: Delay this restart (1 second?) */
        res2 = xmmsc_result_restart (res);
        xmmsc_result_unref (res2);
        xmmsc_result_unref (res);

        playtime = playtime / 1000;
        if (xmms->parent.elapsed != playtime) {
                g_object_set (G_OBJECT (instance), "elapsed", playtime, NULL);
                g_signal_emit_by_name (G_OBJECT (xmms), "elapsed_changed", playtime);
        }
}

static void
ario_xmms_sync (ArioXmms *xmms)
{
        ARIO_LOG_FUNCTION_START
        xmmsc_result_t *res;

        playlist_not (NULL, xmms);
        res = xmmsc_playback_status (xmms->priv->connection);
        ario_xmms_result_wait (res);
        playback_status_not (res, xmms);
        xmmsc_result_unref (res);

        res = xmmsc_playback_current_id (xmms->priv->connection);
        ario_xmms_result_wait (res);
        playback_current_id_not (res, xmms);
        xmmsc_result_unref (res);

        playback_volume_changed_not (NULL, xmms);


        /*
           res = xmmsc_configval_list (xmms->priv->connection);
           ario_xmms_result_wait (res);
           xmmsc_result_dict_foreach (res, abcde, NULL);
           xmmsc_result_unref (res);
           */
}

static gboolean
ario_xmms_connect_to (ArioXmms *xmms,
                      gchar *hostname,
                      int port,
                      float timeout)
{
        ARIO_LOG_FUNCTION_START
        xmmsc_connection_t *connection;
        const gchar *xmms_path;

        connection = xmmsc_init ("ario");

        if (!connection)
                return FALSE;

        xmms_path = g_getenv ("XMMS_PATH");

        if (!xmms_path)
                return FALSE;

        if (!xmmsc_connect (connection, xmms_path))
                return FALSE;

        xmms->priv->connection = connection;

        XMMS_CALLBACK_SET (connection, xmmsc_broadcast_playlist_changed, 
                           (xmmsc_result_notifier_t) playlist_not, xmms);
        XMMS_CALLBACK_SET (connection, xmmsc_broadcast_playback_status, 
                           (xmmsc_result_notifier_t) playback_status_not, xmms);
        XMMS_CALLBACK_SET (connection, xmmsc_broadcast_playback_current_id, 
                           (xmmsc_result_notifier_t) playback_current_id_not, xmms);
        XMMS_CALLBACK_SET (connection, xmmsc_broadcast_playback_volume_changed, 
                           (xmmsc_result_notifier_t) playback_volume_changed_not, xmms);
        XMMS_CALLBACK_SET (connection, xmmsc_signal_playback_playtime, 
                           (xmmsc_result_notifier_t) playback_playtime_not, xmms);

        /* TODO: Disconnect callback */
        xmmsc_mainloop_gmain_init (connection);

        ario_xmms_sync (xmms);

        return TRUE;
}

static gpointer
ario_xmms_connect (void)
{
        ARIO_LOG_FUNCTION_START
        gchar *hostname;
        int port;
        float timeout;

        hostname = ario_conf_get_string (PREF_HOST, PREF_HOST_DEFAULT);
        port = ario_conf_get_integer (PREF_PORT, PREF_PORT_DEFAULT);
        timeout = 5.0;

        if (hostname == NULL)
                hostname = g_strdup ("localhost");

        if (port == 0)
                port = 6600;

        if (!ario_xmms_connect_to (instance, hostname, port, timeout)) {
                ario_xmms_disconnect ();
        }

        g_free (hostname);
        instance->parent.connecting = FALSE;

        return NULL;
}

static void
ario_xmms_disconnect (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_unref (instance->priv->connection);
        instance->priv->connection = NULL;
}

static void
ario_xmms_update_db (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_medialib_rehash (instance->priv->connection, 0));
}

static gboolean
ario_xmms_is_connected (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START
        return (instance->priv->connection != NULL);
}

static GSList *
ario_xmms_list_tags (const ArioServerTag tag,
                     const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        GSList *tags = NULL;
        xmmsc_result_t *res;
        xmmsc_coll_t *coll;
        const char *properties[] = { ArioXmmsPattern[tag], NULL };
        const char *group_by[]   = { ArioXmmsPattern[tag], NULL };
        const char *char_tag;
        char *pattern = NULL, *pattern_tmp;
        const GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;

                if (!g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN)) {
                        if (pattern) {
                                pattern_tmp = g_strdup_printf ("%s AND NOT +%s", pattern, ArioXmmsPattern[atomic_criteria->tag]);
                                g_free (pattern);
                                pattern = pattern_tmp;
                        } else
                                pattern = g_strdup_printf ("NOT +%s", ArioXmmsPattern[atomic_criteria->tag]);
                } else {
                        if (pattern) {
                                pattern_tmp = g_strdup_printf ("%s AND %s:\"%s\"", pattern, ArioXmmsPattern[atomic_criteria->tag], atomic_criteria->value);
                                g_free (pattern);
                                pattern = pattern_tmp;
                        } else
                                pattern = g_strdup_printf ("%s:\"%s\"", ArioXmmsPattern[atomic_criteria->tag], atomic_criteria->value);
                }
        }

        if (pattern) {
                if (!xmmsc_coll_parse (pattern, &coll))
                        return NULL;
        } else {
                coll = xmmsc_coll_universe ();
        }

        res = xmmsc_coll_query_infos (instance->priv->connection, coll, properties,
                                      0, 0, properties, group_by);
        ario_xmms_result_wait (res);
        for (; xmmsc_result_list_valid (res); xmmsc_result_list_next (res)) {
                xmmsc_result_get_dict_entry_string (res, ArioXmmsPattern[tag], &char_tag);
                if (!char_tag)
                        char_tag = ARIO_SERVER_UNKNOWN;

                tags = g_slist_append (tags, g_strdup (char_tag));
        }

        g_free (pattern);
        xmmsc_coll_unref (coll);
        xmmsc_result_unref (res);

        return tags;
}

static GSList *
ario_xmms_get_albums (const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL;
        ArioServerAlbum *ario_xmms_album;
        xmmsc_result_t *res;
        xmmsc_coll_t *coll;
        const char *properties[] = { "artist", "album", NULL };
        const char *group_by[]   = { "artist", "album", NULL };
        const char *album, *artist;
        char *pattern = NULL, *pattern_tmp;
        const GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;

                if (!g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN)) {
                        if (pattern) {
                                pattern_tmp = g_strdup_printf ("%s AND NOT +%s", pattern, ArioXmmsPattern[atomic_criteria->tag]);
                                g_free (pattern);
                                pattern = pattern_tmp;
                        } else
                                pattern = g_strdup_printf ("NOT +%s", ArioXmmsPattern[atomic_criteria->tag]);
                } else {
                        if (pattern) {
                                pattern_tmp = g_strdup_printf ("%s AND %s:\"%s\"", pattern, ArioXmmsPattern[atomic_criteria->tag], atomic_criteria->value);
                                g_free (pattern);
                                pattern = pattern_tmp;
                        } else
                                pattern = g_strdup_printf ("%s:\"%s\"", ArioXmmsPattern[atomic_criteria->tag], atomic_criteria->value);
                }
        }

        if (pattern) {
                if (!xmmsc_coll_parse (pattern, &coll))
                        return NULL;
        } else {
                coll = xmmsc_coll_universe ();
        }

        res = xmmsc_coll_query_infos(instance->priv->connection, coll, properties,
                                     0, 0, properties, group_by);
        ario_xmms_result_wait (res);
        for (; xmmsc_result_list_valid (res); xmmsc_result_list_next (res)) {
                ario_xmms_album = (ArioServerAlbum *) g_malloc0 (sizeof (ArioServerAlbum));

                xmmsc_result_get_dict_entry_string (res, "artist", &artist);
                xmmsc_result_get_dict_entry_string (res, "album", &album);
                if (!album)
                        album = ARIO_SERVER_UNKNOWN;

                if (!artist)
                        artist = ARIO_SERVER_UNKNOWN;

                ario_xmms_album->artist = g_strdup (artist);
                ario_xmms_album->album = g_strdup (album);

                albums = g_slist_append (albums, ario_xmms_album);
        }

        g_free (pattern);
        xmmsc_coll_unref(coll);
        xmmsc_result_unref (res);

        return albums;
}

static GSList *
ario_xmms_get_songs (const ArioServerCriteria *criteria,
                     const gboolean exact)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        xmmsc_result_t *res;
        xmmsc_coll_t *coll;
        const char *properties[] = { "tracknr", "title", "url", NULL };
        const char *group_by[]   = { "tracknr", "title", NULL };
        const char *title;
        const char *url;
        int tracknr;
        ArioServerSong *xmms_song;
        char *pattern = NULL, *pattern_tmp;
        const GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;

                if (!g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN)) {
                        if (pattern) {
                                pattern_tmp = g_strdup_printf ("%s AND NOT +%s", pattern, ArioXmmsPattern[atomic_criteria->tag]);
                                g_free (pattern);
                                pattern = pattern_tmp;
                        } else
                                pattern = g_strdup_printf ("NOT +%s", ArioXmmsPattern[atomic_criteria->tag]);
                } else {
                        if (pattern) {
                                pattern_tmp = g_strdup_printf ("%s AND %s:\"%s\"", pattern, ArioXmmsPattern[atomic_criteria->tag], atomic_criteria->value);
                                g_free (pattern);
                                pattern = pattern_tmp;
                        } else
                                pattern = g_strdup_printf ("%s:\"%s\"", ArioXmmsPattern[atomic_criteria->tag], atomic_criteria->value);
                }
        }

        if (pattern) {
                if (!xmmsc_coll_parse (pattern, &coll))
                        return NULL;
        } else {
                coll = xmmsc_coll_universe ();
        }

        res = xmmsc_coll_query_infos(instance->priv->connection, coll, properties,
                                     0, 0, properties, group_by);
        ario_xmms_result_wait (res);
        for (; xmmsc_result_list_valid (res); xmmsc_result_list_next (res)) {
                xmms_song = (ArioServerSong *) g_malloc0 (sizeof (ArioServerSong));

                xmmsc_result_get_dict_entry_string (res, "title", &title);
                xmmsc_result_get_dict_entry_string (res, "url", &url);
                xmmsc_result_get_dict_entry_int (res, "tracknr", &tracknr);

                xmms_song->title = g_strdup (title);
                xmms_song->track = g_strdup_printf ("%d", tracknr);
                xmms_song->file = g_strdup (url);

                songs = g_slist_append (songs, xmms_song);
        }

        g_free (pattern);
        xmmsc_coll_unref(coll);
        xmmsc_result_unref (res);

        return songs;
}

static GSList *
ario_xmms_get_songs_from_playlist (char *playlist)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        xmmsc_result_t *res;
        xmmsc_result_t *res2;
        guint i;
        int pos = 0;
        ArioServerSong *song;

        res = xmmsc_playlist_list_entries (instance->priv->connection, playlist);

        ario_xmms_result_wait (res);

        instance->priv->total_time = 0;
        for (; xmmsc_result_list_valid (res); xmmsc_result_list_next (res)) {
                if (!xmmsc_result_get_uint(res, &i))
                        ARIO_LOG_ERROR ("Broken result");
                res2 = xmmsc_medialib_get_info (instance->priv->connection, i);
                ario_xmms_result_wait (res2);

                song = ario_xmms_get_song_from_res (res2);

                song->pos = pos;
                ++pos;
                xmmsc_result_unref (res2);
                songs = g_slist_append (songs, song);
                instance->priv->total_time += song->time;
        }
        instance->parent.playlist_length = pos;

        xmmsc_result_unref (res);

        return songs;
}

static GSList *
ario_xmms_get_playlists (void)
{
        ARIO_LOG_FUNCTION_START
        GSList *playlists = NULL;
        const gchar *playlist;
        xmmsc_result_t *res;

        res = xmmsc_playlist_list (instance->priv->connection);
        ario_xmms_result_wait (res);

        for (; xmmsc_result_list_valid (res); xmmsc_result_list_next (res)) {
                xmmsc_result_get_string (res, &playlist);
                playlists = g_slist_append (playlists, g_strdup (playlist));
        }
        xmmsc_result_unref (res);

        return playlists;
}

static GSList *
ario_xmms_get_playlist_changes (int playlist_id)
{
        ARIO_LOG_FUNCTION_START
        return ario_xmms_get_songs_from_playlist (NULL);
}

static ArioServerSong *
ario_xmms_get_current_song_on_server (void)
{
        ARIO_LOG_FUNCTION_START
        ArioServerSong *song = NULL;
        xmmsc_result_t *result;

        result = xmmsc_medialib_get_info (instance->priv->connection, instance->parent.song_id);
        ario_xmms_result_wait (result);
        song = ario_xmms_get_song_from_res (result);
        xmmsc_result_unref (result);

        return song;
}

static int
ario_xmms_get_current_playlist_total_time (void)
{
        ARIO_LOG_FUNCTION_START

        return instance->priv->total_time;
}

static unsigned long
ario_xmms_get_last_update (void)
{
        ARIO_LOG_FUNCTION_START

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");

        return 0;
}

void
ario_xmms_do_next (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playlist_set_next_rel (instance->priv->connection, 1));
        xmmsc_result_unref (xmmsc_playback_tickle (instance->priv->connection));
}

void
ario_xmms_do_prev (void)
{
        ARIO_LOG_FUNCTION_START

        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playlist_set_next_rel (instance->priv->connection, -1));
        xmmsc_result_unref (xmmsc_playback_tickle (instance->priv->connection));
}

void
ario_xmms_do_play (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playback_start (instance->priv->connection));
}

void
ario_xmms_do_play_pos (gint id)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playback_start (instance->priv->connection));
        xmmsc_result_unref (xmmsc_playlist_set_next (instance->priv->connection, id));
        xmmsc_result_unref (xmmsc_playback_tickle (instance->priv->connection));
}

void
ario_xmms_do_pause (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playback_pause (instance->priv->connection));
}

void
ario_xmms_do_stop (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playback_stop (instance->priv->connection));
}

void
ario_xmms_set_current_elapsed (const gint elapsed)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playback_seek_ms (instance->priv->connection, elapsed * 1000));
}

void
ario_xmms_set_current_volume (const gint volume)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_result_unref (xmmsc_playback_volume_set (instance->priv->connection, "left", volume));
        xmmsc_result_unref (xmmsc_playback_volume_set (instance->priv->connection, "right", volume));
}

void
ario_xmms_set_current_random (const gboolean random)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");
}

void
ario_xmms_set_current_repeat (const gboolean repeat)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");
}

void
ario_xmms_set_crossfadetime (const int crossfadetime)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");
}

void
ario_xmms_clear (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        xmmsc_playlist_clear (instance->priv->connection, NULL);
}
/*
   static gchar
   hex2char (gchar first_digit,
   gchar second_digit)
   {
   ARIO_LOG_FUNCTION_START
   gchar result = 0;

   if (first_digit >= '0' && first_digit <= '9')
   result = (first_digit - '0') * 16;
   else if (first_digit >= 'a' && first_digit <= 'f')
   result = (first_digit - 'a' + 10) * 16;
   if (second_digit >= '0' && second_digit <= '9')
   result += second_digit - '0';
   else if (second_digit >= 'a' && second_digit <= 'f')
   result += second_digit - 'a' + 10;
   return result;
   }

   static gchar *
   decode_string (const gchar *string)
   {
   ARIO_LOG_FUNCTION_START
   int i, j;
   gchar *result = g_malloc ((g_utf8_strlen (string, -1) + 1) * sizeof (gchar));

   for (i = 0, j = 0; i < g_utf8_strlen (string, -1); i++) {
   if (string[i] == '%') {
   result[j] = hex2char(string[i+1], string[i+2]);
   i+=2;
   j++; 
   } else if (string[i] == '+') {
   result[j] = ' ';
   j++;
   } else {
   result[j] = string[i];
   j++;
   }
   }
   result[j] = '\0';
   return result;
   }
   */
static void
ario_xmms_queue_commit (void)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioServerQueueAction *queue_action;
        gchar *pattern = NULL, *tmp_pattern;
        xmmsc_coll_t *coll;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        for (tmp = instance->parent.queue; tmp; tmp = g_slist_next (tmp)) {
                queue_action = (ArioServerQueueAction *) tmp->data;
                if (queue_action->type == ARIO_SERVER_ACTION_ADD) {
                        if (queue_action->path) {
                                if (!pattern) {
                                        pattern = g_strdup_printf ("url:\"%s\"", queue_action->path);
                                } else {
                                        tmp_pattern = g_strdup_printf ("%s OR url:\"%s\"", pattern, queue_action->path);
                                        g_free (pattern);
                                        pattern = tmp_pattern;
                                }
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_DELETE_ID) {
                        if(queue_action->id >= 0) {
                                /* TODO */
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_DELETE_POS) {
                        if(queue_action->pos >= 0) {
                                xmmsc_result_unref (xmmsc_playlist_remove_entry (instance->priv->connection, NULL, queue_action->pos));
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_MOVE) {
                        if (queue_action->id >= 0) {
                                xmmsc_result_unref (xmmsc_playlist_move_entry (instance->priv->connection, NULL, queue_action->old_pos, queue_action->new_pos));
                        }
                }

        }

        if (pattern) {
                if (xmmsc_coll_parse (pattern, &coll)) {
                        xmmsc_result_unref (xmmsc_playlist_add_collection (instance->priv->connection, NULL, coll, NULL));
                        xmmsc_coll_unref (coll);
                }
                g_free (pattern);
        }

        g_slist_foreach (instance->parent.queue, (GFunc) g_free, NULL);
        g_slist_free (instance->parent.queue);
        instance->parent.queue = NULL;
}

static void
ario_xmms_insert_at (const GSList *songs,
                     const gint pos)
{
        ARIO_LOG_FUNCTION_START
        const GSList *tmp;
        gchar *pattern = NULL, *tmp_pattern;
        xmmsc_coll_t *coll;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        /* For each filename :*/
        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                if (!pattern) {
                        pattern = g_strdup_printf ("url:\"%s\"", (char*) tmp->data);
                } else {
                        tmp_pattern = g_strdup_printf ("%s OR url:\"%s\"", pattern, (char *) tmp->data);
                        g_free (pattern);
                        pattern = tmp_pattern;
                }
        }

        if (pattern) {
                if (xmmsc_coll_parse (pattern, &coll)) {
                        xmmsc_result_unref (xmmsc_playlist_insert_collection (instance->priv->connection, NULL, pos + 1, coll, NULL));
                        xmmsc_coll_unref (coll);
                }
                g_free (pattern);
        }
}

static int
ario_xmms_save_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");

        return 0;
}

static void
ario_xmms_delete_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        xmmsc_result_unref (xmmsc_playlist_remove (instance->priv->connection, name));
}

static GSList *
ario_xmms_get_outputs (void)
{
        ARIO_LOG_FUNCTION_START

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");

        return NULL;
}

static void
ario_xmms_enable_output (int id,
                         gboolean enabled)
{
        ARIO_LOG_FUNCTION_START

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");
}

static ArioServerStats *
ario_xmms_get_stats (void)
{
        ARIO_LOG_FUNCTION_START

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");

        /* TODO :
         * xmmsc_result_t *         xmmsc_main_stats (xmmsc_connection_t *c)
         */
        return NULL;
}

static GList *
ario_xmms_get_songs_info (GSList *paths)
{
        ARIO_LOG_FUNCTION_START
        const gchar *path = NULL;
        GSList *temp;
        GList *songs = NULL;
        xmmsc_result_t *res;
        guint id;
        ArioServerSong *song;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (temp = paths; temp; temp = g_slist_next (temp)) {
                path = temp->data;

                res = xmmsc_medialib_get_id (instance->priv->connection, path);
                ario_xmms_result_wait (res);

                if (!xmmsc_result_get_uint(res, &id)) {
                        ARIO_LOG_ERROR ("Broken result");
                        xmmsc_result_unref (res);
                        continue;
                }
                xmmsc_result_unref (res);
                res = xmmsc_medialib_get_info (instance->priv->connection, id);
                ario_xmms_result_wait (res);

                song = ario_xmms_get_song_from_res (res);
                songs = g_list_append (songs, song);
        }

        return songs;
}

static ArioServerFileList *
ario_xmms_list_files (const char *path,
                      gboolean recursive)
{
        ARIO_LOG_FUNCTION_START
        ArioServerFileList *files = (ArioServerFileList *) g_malloc0 (sizeof (ArioServerFileList));

        ARIO_LOG_ERROR ("Not yet implemented for XMMS");
        /* TODO:
         * pattern: url ~? in:?
         * */

        return files;
}

