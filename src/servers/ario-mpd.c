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

#include "servers/ario-mpd.h"
#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-profiles.h"
#include "preferences/ario-preferences.h"
#include "lib/ario-conf.h"
#include "widgets/ario-playlist.h"

/* Number of milliseconds in 1 second */
#define ONE_SECOND 1000

/* Timeout for retrieve of data on MPD */
#define NORMAL_TIMEOUT 500

/* Reconnect after 0.5 second in case of error */
#define RECONNECT_INIT_TIMEOUT 500
/* Multiply reconnection timeout by 2 after each tentative */
#define RECONNECT_FACTOR 2
/* Try to reconnect 5 times */
#define RECONNECT_TENTATIVES 5

static void ario_mpd_finalize (GObject *object);
static gboolean ario_mpd_connect_to (ArioMpd *mpd,
                                     gchar *hostname,
                                     int port,
                                     float timeout);
static void ario_mpd_connect (void);
static void ario_mpd_disconnect (void);
static void ario_mpd_update_db (const gchar *path);
static gboolean ario_mpd_check_errors (void);
static gboolean ario_mpd_is_connected (void);
static GSList * ario_mpd_list_tags (const ArioServerTag tag,
                                    const ArioServerCriteria *criteria);
static gboolean ario_mpd_album_is_present (const GSList *albums,
                                           const char *album);
static GSList * ario_mpd_get_albums (const ArioServerCriteria *criteria);
static GSList * ario_mpd_get_songs (const ArioServerCriteria *criteria,
                                    const gboolean exact);
static GSList * ario_mpd_get_songs_from_playlist (char *playlist);
static GSList * ario_mpd_get_playlists (void);
static GSList * ario_mpd_get_playlist_changes (gint64 playlist_id);
static gboolean ario_mpd_update_status (void);
static ArioServerSong * ario_mpd_get_current_song_on_server (void);
static int ario_mpd_get_current_playlist_total_time (void);
static unsigned long ario_mpd_get_last_update (void);
static void ario_mpd_do_next (void);
static void ario_mpd_do_prev (void);
static void ario_mpd_do_play (void);
static void ario_mpd_do_play_pos (gint id);
static void ario_mpd_do_pause (void);
static void ario_mpd_do_stop (void);
static void ario_mpd_set_current_elapsed (const gint elapsed);
static void ario_mpd_set_current_volume (const gint volume);
static void ario_mpd_set_current_random (const gboolean random);
static void ario_mpd_set_current_repeat (const gboolean repeat);
static void ario_mpd_set_crossfadetime (const int crossfadetime);
static void ario_mpd_clear (void);
static void ario_mpd_shuffle (void);
static void ario_mpd_queue_commit (void);
static void ario_mpd_insert_at (const GSList *songs,
                                const gint pos);
static int ario_mpd_save_playlist (const char *name);
static void ario_mpd_delete_playlist (const char *name);
static void ario_mpd_launch_timeout (void);
static GSList * ario_mpd_get_outputs (void);
static void ario_mpd_enable_output (int id,
                                    gboolean enabled);
static ArioServerStats * ario_mpd_get_stats (void);
static GList * ario_mpd_get_songs_info (GSList *paths);
static ArioServerFileList * ario_mpd_list_files (const char *path,
                                                 gboolean recursive);

/* Private attributes */
struct ArioMpdPrivate
{
        mpd_Status *status;
        mpd_Connection *connection;
        mpd_Stats *stats;

        guint timeout_id;

        gboolean support_empty_tags;
        gboolean support_idle;

        gboolean is_updating;

        int elapsed;
        int reconnect_time;
};

#define ARIO_MPD_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_MPD, ArioMpdPrivate))
G_DEFINE_TYPE (ArioMpd, ario_mpd, TYPE_ARIO_SERVER_INTERFACE)

static ArioMpd *instance = NULL;
static ArioServer *server_instance = NULL;

static void
ario_mpd_class_init (ArioMpdClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioServerInterfaceClass *server_class = ARIO_SERVER_INTERFACE_CLASS (klass);

        /* GObject virtual methods */
        object_class->finalize = ario_mpd_finalize;

        /* ArioServerInterface virtual methods */
        server_class->connect = ario_mpd_connect;
        server_class->disconnect = ario_mpd_disconnect;
        server_class->is_connected = ario_mpd_is_connected;
        server_class->update_status = ario_mpd_update_status;
        server_class->update_db = ario_mpd_update_db;
        server_class->list_tags = ario_mpd_list_tags;
        server_class->get_albums = ario_mpd_get_albums;
        server_class->get_songs = ario_mpd_get_songs;
        server_class->get_songs_from_playlist = ario_mpd_get_songs_from_playlist;
        server_class->get_playlists = ario_mpd_get_playlists;
        server_class->get_playlist_changes = ario_mpd_get_playlist_changes;
        server_class->get_current_song_on_server = ario_mpd_get_current_song_on_server;
        server_class->get_current_playlist_total_time = ario_mpd_get_current_playlist_total_time;
        server_class->get_last_update = ario_mpd_get_last_update;
        server_class->do_next = ario_mpd_do_next;
        server_class->do_prev = ario_mpd_do_prev;
        server_class->do_play = ario_mpd_do_play;
        server_class->do_play_pos = ario_mpd_do_play_pos;
        server_class->do_pause = ario_mpd_do_pause;
        server_class->do_stop = ario_mpd_do_stop;
        server_class->set_current_elapsed = ario_mpd_set_current_elapsed;
        server_class->set_current_volume = ario_mpd_set_current_volume;
        server_class->set_current_random = ario_mpd_set_current_random;
        server_class->set_current_repeat = ario_mpd_set_current_repeat;
        server_class->set_crossfadetime = ario_mpd_set_crossfadetime;
        server_class->clear = ario_mpd_clear;
        server_class->shuffle = ario_mpd_shuffle;
        server_class->queue_commit = ario_mpd_queue_commit;
        server_class->insert_at = ario_mpd_insert_at;
        server_class->save_playlist = ario_mpd_save_playlist;
        server_class->delete_playlist = ario_mpd_delete_playlist;
        server_class->get_outputs = ario_mpd_get_outputs;
        server_class->enable_output = ario_mpd_enable_output;
        server_class->get_stats = ario_mpd_get_stats;
        server_class->get_songs_info = ario_mpd_get_songs_info;
        server_class->list_files = ario_mpd_list_files;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioMpdPrivate));
}

static void
ario_mpd_init (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START;
        mpd->priv = ARIO_MPD_GET_PRIVATE (mpd);

        mpd->priv->timeout_id = 0;
}

static void
ario_mpd_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioMpd *mpd;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_MPD (object));

        mpd = ARIO_MPD (object);
        g_return_if_fail (mpd->priv != NULL);

        /* Close connection to MPD */
        if (mpd->priv->connection)
                mpd_closeConnection (mpd->priv->connection);

        /* Free a few data */
        if (mpd->priv->status)
                mpd_freeStatus (mpd->priv->status);

        if (mpd->priv->stats)
                mpd_freeStats (mpd->priv->stats);

        /* Stop retrieving data from MPD */
        if (mpd->priv->timeout_id)
                g_source_remove (mpd->priv->timeout_id);

        instance = NULL;

        G_OBJECT_CLASS (ario_mpd_parent_class)->finalize (object);
}

ArioMpd *
ario_mpd_get_instance (ArioServer *server)
{
        ARIO_LOG_FUNCTION_START;
        /* Create instance if needed */
        if (!instance) {
                instance = g_object_new (TYPE_ARIO_MPD, NULL);
                g_return_val_if_fail (instance->priv != NULL, NULL);
                server_instance = server;
        }
        return instance;
}

static void
ario_mpd_check_idle (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START;
        mpd->priv->support_idle = FALSE;
#ifdef ENABLE_MPDIDLE
        char *command;

        /* Get list of supported commands */
        mpd_sendCommandsCommand (mpd->priv->connection);
        while ((command = mpd_getNextCommand (mpd->priv->connection))) {
                /* Detect if idle command is supported */
                if (!strcmp (command, "idle"))
                        mpd->priv->support_idle = TRUE;
                g_free (command);
        }
#endif
}

static void
ario_mpd_launch_timeout (void)
{
        ARIO_LOG_FUNCTION_START;
        instance->priv->timeout_id = g_timeout_add (NORMAL_TIMEOUT,
                                                    (GSourceFunc) ario_mpd_update_status,
                                                    NULL);
}

#ifdef ENABLE_MPDIDLE
static gboolean
ario_mpd_update_elapsed (gpointer data)
{
        ARIO_LOG_FUNCTION_START;

        /* If in idle mode: update elapsed time every seconds when MPD is playing */
        if (ario_server_get_current_state() == MPD_STATUS_STATE_PLAY) {
                ++instance->priv->elapsed;
                g_object_set (G_OBJECT (instance), "elapsed", instance->priv->elapsed, NULL);
                ario_server_interface_emit (ARIO_SERVER_INTERFACE (instance), server_instance);
        }

        return TRUE;
}

static void
ario_mpd_launch_idle_timeout (void)
{
        ARIO_LOG_FUNCTION_START;
        instance->priv->timeout_id = g_timeout_add (ONE_SECOND,
                                                    (GSourceFunc) ario_mpd_update_elapsed,
                                                    NULL);
}
#endif

static void
ario_mpd_idle_cb (mpd_Connection *connection,
                  unsigned flags,
                  void *userdata)
{
        ARIO_LOG_FUNCTION_START;

        /* Update MPD status */
        /* TODO: Be more selective depending on flags */
        if (flags & IDLE_DATABASE
            || flags & IDLE_PLAYLIST
            || flags & IDLE_PLAYER
            || flags & IDLE_MIXER
            || flags & IDLE_OPTIONS)
                ario_mpd_update_status ();

        /* Stored playlists changed, update list */
        if (flags & IDLE_STORED_PLAYLIST)
                g_signal_emit_by_name (G_OBJECT (server_instance), "storedplaylists_changed");

        /* Diconnected from MPD: check errors */
        if (flags & IDLE_DISCONNECT)
                ario_mpd_check_errors ();

        /* Restart idle */
        if (instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static gboolean
ario_mpd_connect_to (ArioMpd *mpd,
                     gchar *hostname,
                     int port,
                     float timeout)
{
        ARIO_LOG_FUNCTION_START;
        gchar *password;
        mpd_Connection *connection;

        /* Connect to MPD */
        connection = mpd_newConnection (hostname, port, timeout);
        if (!connection)
                return FALSE;

        /* Check connection errors */
        if  (connection->error) {
                ARIO_LOG_ERROR("%s", connection->errorStr);
                mpd_clearError (connection);
                mpd_closeConnection (connection);
                return FALSE;
        }

        /* Send password if one is set in profile */
        password = ario_profiles_get_current (ario_profiles_get ())->password;
        if (password) {
                mpd_sendPasswordCommand (connection, password);
                mpd_finishCommand (connection);
        }

        mpd->priv->connection = connection;

        /* Check if idle is supported by MPD server */
        ario_mpd_check_idle (mpd);

        if (instance->priv->support_idle && instance->priv->connection) {
#ifdef ENABLE_MPDIDLE
                /* Initialise Idle mode */
                mpd_glibInit (instance->priv->connection);
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
                g_idle_add ((GSourceFunc) ario_mpd_update_status, NULL);

                /* Launch timeout to update elapsed time */
                ario_mpd_launch_idle_timeout ();
#endif
        } else {
                /* Launch timeout for data retrieve from MPD */
                ario_mpd_launch_timeout ();
        }

        return TRUE;
}

static gpointer
ario_mpd_connect_thread (ArioServer *server)
{
        ARIO_LOG_FUNCTION_START;
        gchar *hostname;
        int port;
        float timeout;
        ArioProfile *profile;

        profile = ario_profiles_get_current (ario_profiles_get ());
        hostname = profile->host;
        port = profile->port;
        timeout = 5.0;

        if (hostname == NULL)
                hostname = "localhost";

        if (port == 0)
                port = 6600;

        if (!ario_mpd_connect_to (instance, hostname, port, timeout)) {
                ario_mpd_disconnect ();
        }

        instance->priv->support_empty_tags = FALSE;
        instance->parent.connecting = FALSE;

        return NULL;
}

static void
ario_mpd_connect (void)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *win = NULL, *vbox,*label, *bar;
        GThread* thread;
        GtkWidget *dialog;
        gboolean is_in_error = (instance->priv->reconnect_time > 0);

        thread = g_thread_create ((GThreadFunc) ario_mpd_connect_thread,
                                  instance, TRUE, NULL);

        if (!is_in_error) {
                win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
                gtk_window_set_modal (GTK_WINDOW (win), TRUE);
                vbox = gtk_vbox_new (FALSE, 0);
                label = gtk_label_new (_("Connecting to server..."));
                bar = gtk_progress_bar_new ();

                gtk_container_add (GTK_CONTAINER (win), vbox);
                gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 6);
                gtk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 6);

                gtk_window_set_resizable (GTK_WINDOW (win), FALSE);
                gtk_window_set_title (GTK_WINDOW (win), "Ario");
                gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER);
                gtk_widget_show_all (win);

                while (instance->parent.connecting) {
                        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));
                        while (gtk_events_pending ())
                                gtk_main_iteration ();
                        g_usleep (200000);
                }
        }

        g_thread_join (thread);

        if (ario_server_is_connected ()) {
                instance->priv->reconnect_time = 0;
        } else if (!is_in_error) {
                dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 _("Impossible to connect to server. Check the connection options."));
                if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_NONE)
                        gtk_widget_destroy (dialog);
                g_signal_emit_by_name (G_OBJECT (server_instance), "state_changed");
        }

        if (win) {
                gtk_widget_hide (win);
                gtk_widget_destroy (win);
        }
}

static void
ario_mpd_disconnect (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        if (instance->priv->support_idle)
                mpd_stopIdle (instance->priv->connection);
        mpd_closeConnection (instance->priv->connection);
        instance->priv->connection = NULL;

        if (instance->priv->timeout_id) {
                g_source_remove (instance->priv->timeout_id);
                instance->priv->timeout_id = 0;
        }

        ario_mpd_update_status ();
}

static void
ario_mpd_update_db (const gchar *path)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        if (!path)
                mpd_sendUpdateCommand (instance->priv->connection, "");
        else
                mpd_sendUpdateCommand (instance->priv->connection, path);

        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static gboolean
ario_mpd_try_reconnect (gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_connect ();

        if (!instance->priv->connection
            && instance->priv->reconnect_time <= RECONNECT_TENTATIVES) {
                /* Try to reconnect */
                ++instance->priv->reconnect_time;
                g_timeout_add (RECONNECT_INIT_TIMEOUT * instance->priv->reconnect_time * RECONNECT_FACTOR,
                               ario_mpd_try_reconnect, NULL);
        }

        return FALSE;
}

static gboolean
ario_mpd_check_errors (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START;
        if (!instance->priv->connection)
                return FALSE;

        if  (instance->priv->connection->error) {
                ARIO_LOG_ERROR("%s", instance->priv->connection->errorStr);
                mpd_clearError (instance->priv->connection);
                ario_server_disconnect ();

                /* Try to reconnect */
                instance->priv->reconnect_time = 1;
                g_timeout_add (RECONNECT_INIT_TIMEOUT * instance->priv->reconnect_time * RECONNECT_FACTOR,
                               ario_mpd_try_reconnect, NULL);
                return TRUE;
        }
        return FALSE;
}

static gboolean
ario_mpd_is_connected (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START;
        return (instance->priv->connection != NULL);
}

static GSList *
ario_mpd_list_tags (const ArioServerTag tag,
                    const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START;
        gchar *value;
        const GSList *tmp;
        GSList *values = NULL;
        ArioServerAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_startFieldSearch (instance->priv->connection, tag);
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (instance->priv->support_empty_tags
                    && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        mpd_addConstraintSearch (instance->priv->connection, atomic_criteria->tag, "");
                else
                        mpd_addConstraintSearch (instance->priv->connection, atomic_criteria->tag, atomic_criteria->value);
        }
        mpd_commitSearch (instance->priv->connection);

        while ((value = mpd_getNextTag (instance->priv->connection, tag))) {
                if (*value)
                        values = g_slist_append (values, value);
                else {
                        g_free (value);
                        values = g_slist_append (values, g_strdup (ARIO_SERVER_UNKNOWN));
                        instance->priv->support_empty_tags = TRUE;
                }
        }

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return values;
}

static gboolean
ario_mpd_album_is_present (const GSList *albums,
                           const char *album)
{
        ARIO_LOG_FUNCTION_START;
        const GSList *tmp;
        ArioServerAlbum *mpd_album;

        for (tmp = albums; tmp; tmp = g_slist_next (tmp)) {
                mpd_album = tmp->data;
                if (!g_utf8_collate (album, mpd_album->album)) {
                        return TRUE;
                }
        }
        return FALSE;
}

static GSList *
ario_mpd_get_albums (const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START;
        GSList *albums = NULL;
        const GSList *tmp;
        mpd_InfoEntity *entity = NULL;
        ArioServerAlbum *mpd_album;
        ArioServerAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        if (!criteria) {
                mpd_sendListallInfoCommand (instance->priv->connection, "/");
        } else {
                mpd_startSearch (instance->priv->connection, TRUE);
                for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                        atomic_criteria = tmp->data;

                        if (instance->priv->support_empty_tags
                            && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                                mpd_addConstraintSearch (instance->priv->connection,
                                                         atomic_criteria->tag,
                                                         "");
                        else
                                mpd_addConstraintSearch (instance->priv->connection,
                                                         atomic_criteria->tag,
                                                         atomic_criteria->value);
                }
                mpd_commitSearch (instance->priv->connection);
        }

        while ((entity = mpd_getNextInfoEntity (instance->priv->connection))) {
                if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
                        mpd_freeInfoEntity (entity);
                        continue;
                }

                if (!entity->info.song->album) {
                        if (ario_mpd_album_is_present (albums, ARIO_SERVER_UNKNOWN)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                } else {
                        if (ario_mpd_album_is_present (albums, entity->info.song->album)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                }

                mpd_album = (ArioServerAlbum *) g_malloc (sizeof (ArioServerAlbum));
                if (entity->info.song->album) {
                        mpd_album->album = entity->info.song->album;
                        entity->info.song->album = NULL;
                } else {
                        mpd_album->album = g_strdup (ARIO_SERVER_UNKNOWN);
                }

                if (entity->info.song->artist) {
                        mpd_album->artist = entity->info.song->artist;
                        entity->info.song->artist = NULL;
                } else {
                        mpd_album->artist = g_strdup (ARIO_SERVER_UNKNOWN);
                }

                if (entity->info.song->file) {
                        mpd_album->path = entity->info.song->file;
                        entity->info.song->file = NULL;
                } else {
                        mpd_album->path = NULL;
                }

                if (entity->info.song->date) {
                        mpd_album->date = entity->info.song->date;
                        entity->info.song->date = NULL;
                } else {
                        mpd_album->date = NULL;
                }

                albums = g_slist_append (albums, mpd_album);

                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return albums;
}

static GSList *
ario_mpd_get_songs (const ArioServerCriteria *criteria,
                    const gboolean exact)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;
        mpd_InfoEntity *entity = NULL;
        const GSList *tmp;
        gboolean is_album_unknown = FALSE;
        ArioServerAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (atomic_criteria->tag == MPD_TAG_ITEM_ALBUM
                    && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        is_album_unknown = TRUE;
        }

        mpd_startSearch (instance->priv->connection, exact);
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (instance->priv->support_empty_tags
                    && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        mpd_addConstraintSearch (instance->priv->connection,
                                                 atomic_criteria->tag,
                                                 "");
                else if (atomic_criteria->tag != MPD_TAG_ITEM_ALBUM
                         || g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        mpd_addConstraintSearch (instance->priv->connection,
                                                 atomic_criteria->tag,
                                                 atomic_criteria->value);
        }
        mpd_commitSearch (instance->priv->connection);

        while ((entity = mpd_getNextInfoEntity (instance->priv->connection))) {
                if (entity->type == MPD_INFO_ENTITY_TYPE_SONG && entity->info.song) {
                        if (instance->priv->support_empty_tags || !is_album_unknown || !entity->info.song->album) {
                                songs = g_slist_append (songs, entity->info.song);
                                entity->info.song = NULL;
                        }
                }
                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return songs;
}

static GSList *
ario_mpd_get_songs_from_playlist (char *playlist)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;
        mpd_InfoEntity *ent = NULL;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendListPlaylistInfoCommand (instance->priv->connection, playlist);
        while ((ent = mpd_getNextInfoEntity (instance->priv->connection))) {
                songs = g_slist_append (songs, ent->info.song);
                ent->info.song = NULL;
                mpd_freeInfoEntity (ent);
        }
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return songs;
}

static GSList *
ario_mpd_get_playlists (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *playlists = NULL;
        mpd_InfoEntity *ent = NULL;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendLsInfoCommand(instance->priv->connection, "/");

        while ((ent = mpd_getNextInfoEntity (instance->priv->connection))) {
                if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
                        playlists = g_slist_append (playlists, g_strdup (ent->info.playlistFile->path));
                }
                mpd_freeInfoEntity (ent);
        }
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return playlists;
}

static GSList *
ario_mpd_get_playlist_changes (gint64 playlist_id)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;
        mpd_InfoEntity *entity;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendPlChangesCommand (instance->priv->connection, (long long) playlist_id);
        while ((entity = mpd_getNextInfoEntity (instance->priv->connection))) {
                if (entity->info.song) {
                        songs = g_slist_append (songs, entity->info.song);
                        entity->info.song = NULL;
                }
                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return songs;
}

static gboolean
ario_mpd_update_status (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START;

        if (instance->priv->is_updating)
                return !instance->priv->support_idle;
        instance->priv->is_updating = TRUE;

        /* check if there is a connection */
        if (!instance->priv->connection) {
                ario_server_interface_set_default (ARIO_SERVER_INTERFACE (instance));
        } else {
                if (instance->priv->status)
                        mpd_freeStatus (instance->priv->status);
                mpd_sendStatusCommand (instance->priv->connection);
                instance->priv->status = mpd_getStatus (instance->priv->connection);

                if (ario_mpd_check_errors ()) {
                        ario_server_interface_set_default (ARIO_SERVER_INTERFACE (instance));
                } else if (instance->priv->status) {
                        if (instance->parent.song_id != instance->priv->status->songid
                            || instance->parent.playlist_id != (gint64) instance->priv->status->playlist)
                                g_object_set (G_OBJECT (instance), "song_id", instance->priv->status->songid, NULL);

                        if (instance->parent.state != instance->priv->status->state)
                                g_object_set (G_OBJECT (instance), "state", instance->priv->status->state, NULL);

                        if (instance->parent.volume != instance->priv->status->volume)
                                g_object_set (G_OBJECT (instance), "volume", instance->priv->status->volume, NULL);

                        if (instance->parent.elapsed != instance->priv->status->elapsedTime) {
                                g_object_set (G_OBJECT (instance), "elapsed", instance->priv->status->elapsedTime, NULL);
                                instance->priv->elapsed = instance->priv->status->elapsedTime;
                        }

                        if (instance->parent.playlist_id != (gint64) instance->priv->status->playlist) {
                                g_object_set (G_OBJECT (instance), "playlist_id", (gint64) instance->priv->status->playlist, NULL);
                                instance->parent.playlist_length = instance->priv->status->playlistLength;
                        }

                        if (instance->parent.random != (gboolean) instance->priv->status->random)
                                g_object_set (G_OBJECT (instance), "random", instance->priv->status->random, NULL);

                        if (instance->parent.repeat != (gboolean) instance->priv->status->repeat)
                                g_object_set (G_OBJECT (instance), "repeat", instance->priv->status->repeat, NULL);

                        if (instance->parent.updatingdb != instance->priv->status->updatingDb)
                                g_object_set (G_OBJECT (instance), "updatingdb", instance->priv->status->updatingDb, NULL);
                        instance->parent.crossfade = instance->priv->status->crossfade;
                }
        }
        ario_server_interface_emit (ARIO_SERVER_INTERFACE (instance), server_instance);

        instance->priv->is_updating = FALSE;

        if (instance->priv->support_idle
            && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return !instance->priv->support_idle;
}

static ArioServerSong *
ario_mpd_get_current_song_on_server (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerSong *song = NULL;
        mpd_InfoEntity *ent ;

        mpd_sendCurrentSongCommand (instance->priv->connection);
        ent = mpd_getNextInfoEntity (instance->priv->connection);
        if (ent) {
                song = ent->info.song;
                ent->info.song = NULL;
                mpd_freeInfoEntity (ent);
        }
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return song;
}

static int
ario_mpd_get_current_playlist_total_time (void)
{
        ARIO_LOG_FUNCTION_START;

        if (!instance->priv->connection)
                return 0;

        /* Compute it from playlist widget (quite coslty but not often called) */
        return ario_playlist_get_total_time ();
}

static unsigned long
ario_mpd_get_last_update (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return 0;

        if (instance->priv->stats)
                mpd_freeStats (instance->priv->stats);
        mpd_sendStatsCommand (instance->priv->connection);
        instance->priv->stats = mpd_getStats (instance->priv->connection);

        ario_mpd_check_errors ();

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        if (instance->priv->stats)
                return instance->priv->stats->dbUpdateTime;
        else
                return 0;
}

static void
ario_mpd_do_next (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendNextCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_do_prev (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendPrevCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_do_play (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendPlayCommand (instance->priv->connection, -1);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_do_play_pos (gint id)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        /* send mpd the play command */
        mpd_sendPlayCommand (instance->priv->connection, id);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_do_pause (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendPauseCommand (instance->priv->connection, TRUE);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_do_stop (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendStopCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_set_current_elapsed (const gint elapsed)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendSeekCommand (instance->priv->connection, instance->priv->status->song, elapsed);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_set_current_volume (const gint volume)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendSetvolCommand (instance->priv->connection, volume);
        mpd_finishCommand (instance->priv->connection);
        ario_mpd_update_status ();

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_set_current_random (const gboolean random)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendRandomCommand (instance->priv->connection, random);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_set_current_repeat (const gboolean repeat)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendRepeatCommand (instance->priv->connection, repeat);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_set_crossfadetime (const int crossfadetime)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendCrossfadeCommand (instance->priv->connection, crossfadetime);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_clear (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendClearCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);
        ario_mpd_update_status ();

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_shuffle (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendShuffleCommand (instance->priv->connection);

        mpd_finishCommand (instance->priv->connection);
        ario_mpd_update_status ();

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_queue_commit (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *temp;
        ArioServerQueueAction *queue_action;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendCommandListBegin(instance->priv->connection);

        for (temp = instance->parent.queue; temp; temp = g_slist_next (temp)) {
                queue_action = (ArioServerQueueAction *) temp->data;
                if (queue_action->type == ARIO_SERVER_ACTION_ADD) {
                        if (queue_action->path) {
                                mpd_sendAddCommand(instance->priv->connection, queue_action->path);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_DELETE_ID) {
                        if (queue_action->id >= 0) {
                                mpd_sendDeleteIdCommand(instance->priv->connection, queue_action->id);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_DELETE_POS) {
                        if (queue_action->pos >= 0) {
                                mpd_sendDeleteCommand(instance->priv->connection, queue_action->pos);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_MOVE) {
                        if (queue_action->id >= 0) {
                                mpd_sendMoveCommand(instance->priv->connection, queue_action->old_pos, queue_action->new_pos);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_MOVEID) {
                        if (queue_action->id >= 0) {
                                mpd_sendMoveIdCommand(instance->priv->connection, queue_action->old_pos, queue_action->new_pos);
                        }
                }
        }
        mpd_sendCommandListEnd (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);
        ario_mpd_update_status ();

        g_slist_foreach (instance->parent.queue, (GFunc) g_free, NULL);
        g_slist_free (instance->parent.queue);
        instance->parent.queue = NULL;

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static void
ario_mpd_insert_at (const GSList *songs,
                    const gint pos)
{
        ARIO_LOG_FUNCTION_START;
        int end, offset = 0;
        const GSList *tmp;

        end = instance->parent.playlist_length;

        /* For each filename :*/
        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                /* Add it in the playlist*/
                ario_server_queue_add (tmp->data);
                ++offset;
                /* move it in the right place */
                ario_server_queue_move (end + offset - 1, pos + offset);
        }

        ario_mpd_queue_commit ();
}

static int
ario_mpd_save_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START;
        mpd_sendSaveCommand (instance->priv->connection, name);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        if (instance->priv->connection->error == MPD_ERROR_ACK && instance->priv->connection->errorCode == MPD_ACK_ERROR_EXIST)
                return 1;
        return 0;
}

static void
ario_mpd_delete_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START;
        mpd_sendRmCommand (instance->priv->connection, name);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static GSList *
ario_mpd_get_outputs (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *outputs = NULL;
        mpd_OutputEntity *output_ent;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendOutputsCommand (instance->priv->connection);

        while ((output_ent = mpd_getNextOutput (instance->priv->connection)))
                outputs = g_slist_append (outputs, output_ent);

        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return outputs;
}

static void
ario_mpd_enable_output (int id,
                        gboolean enabled)
{
        ARIO_LOG_FUNCTION_START;

        if (enabled) {
                mpd_sendEnableOutputCommand(instance->priv->connection, id);
        } else {
                mpd_sendDisableOutputCommand(instance->priv->connection, id);
        }

        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);
}

static ArioServerStats *
ario_mpd_get_stats (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        if (instance->priv->stats)
                mpd_freeStats (instance->priv->stats);
        mpd_sendStatsCommand (instance->priv->connection);
        instance->priv->stats = mpd_getStats (instance->priv->connection);

        ario_mpd_check_errors ();

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return instance->priv->stats;
}

static GList *
ario_mpd_get_songs_info (GSList *paths)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *path = NULL;
        GSList *temp;
        GList *songs = NULL;
        mpd_InfoEntity *ent;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (temp = paths; temp; temp = g_slist_next (temp)) {
                path = temp->data;

                mpd_sendListallInfoCommand (instance->priv->connection, path);

                ent = mpd_getNextInfoEntity (instance->priv->connection);

                mpd_finishCommand (instance->priv->connection);
                if (!ent)
                        continue;

                songs = g_list_append (songs, ent->info.song);
                ent->info.song = NULL;

                mpd_freeInfoEntity (ent);
                ario_mpd_check_errors ();
        }

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return songs;
}

static ArioServerFileList *
ario_mpd_list_files (const char *path,
                     gboolean recursive)
{
        ARIO_LOG_FUNCTION_START;
        mpd_InfoEntity *entity;
        ArioServerFileList *files = (ArioServerFileList *) g_malloc0 (sizeof (ArioServerFileList));

        /* check if there is a connection */
        if (!instance->priv->connection)
                return files;

        if (recursive)
                mpd_sendListallCommand (instance->priv->connection, path);
        else
                mpd_sendLsInfoCommand (instance->priv->connection, path);

        while ((entity = mpd_getNextInfoEntity (instance->priv->connection))) {
                if (entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
                        files->directories = g_slist_append (files->directories, entity->info.directory->path);
                        entity->info.directory->path = NULL;
                } else if (entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
                        files->songs = g_slist_append (files->songs, entity->info.song);
                        entity->info.song = NULL;
                }

                mpd_freeInfoEntity(entity);
        }

        if (instance->priv->support_idle && instance->priv->connection)
                mpd_startIdle (instance->priv->connection, ario_mpd_idle_cb, NULL);

        return files;
}

