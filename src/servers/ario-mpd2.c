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
#include "servers/ario-server.h"
#include "mpd/client.h"

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
                                     guint timeout);
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
// Return TRUE on error
static gboolean ario_mpd_command_preinvoke (void);
static void ario_mpd_command_postinvoke (void);
static void ario_mpd_idle_start (void);
static void ario_mpd_server_state_changed_cb (ArioServer *server,
                                              gpointer data);
/* Private attributes */
struct ArioMpdPrivate
{
        struct mpd_status *status;
        struct mpd_connection *connection;
        ArioServerStats *stats;

        guint timeout_id;

        gboolean support_empty_tags;
        gboolean support_idle;

        gboolean is_updating;

        int elapsed;
        int reconnect_time;
        int idle;
        int source_id;
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
                mpd_connection_free (mpd->priv->connection);

        /* Free a few data */
        if (mpd->priv->status)
                mpd_status_free (mpd->priv->status);

        if (mpd->priv->stats)
                g_free (mpd->priv->stats);

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
        struct mpd_pair * pair;

        /* Get list of supported commands */
        mpd_send_allowed_commands (mpd->priv->connection);
        while ((pair = mpd_recv_pair_named (mpd->priv->connection, "command"))) {
                /* Detect if idle command is supported */
                if (!strcmp (pair->value, "idle"))
                        mpd->priv->support_idle = TRUE;
                mpd_return_pair (mpd->priv->connection, pair);
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

static gboolean
ario_mpd_update_elapsed (gpointer data)
{
        ARIO_LOG_FUNCTION_START;

        /* If in idle mode: update elapsed time every seconds when MPD is playing */
        ++instance->priv->elapsed;
        g_object_set (G_OBJECT (instance), "elapsed", instance->priv->elapsed, NULL);
        ario_server_interface_emit (ARIO_SERVER_INTERFACE (instance), server_instance);

        return TRUE;
}

static void
ario_mpd_idle_read (void)
{
        ARIO_LOG_FUNCTION_START;

        enum mpd_idle flags = mpd_recv_idle (instance->priv->connection, FALSE);
        ario_mpd_check_errors ();

        /* Update MPD status */
        /* TODO: Be more selective depending on flags */
        if (flags & MPD_IDLE_DATABASE
            || flags & MPD_IDLE_QUEUE
            || flags & MPD_IDLE_PLAYER
            || flags & MPD_IDLE_MIXER
            || flags & MPD_IDLE_OPTIONS)
                ario_mpd_update_status ();

        /* Stored playlists changed, update list */
        if (flags & MPD_IDLE_STORED_PLAYLIST)
                g_signal_emit_by_name (G_OBJECT (server_instance), "storedplaylists_changed");
}

static gboolean
ario_mpd_idle_read_cb (GIOChannel *iochan,
                       GIOCondition cond,
                       gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        if (!instance->priv->idle) {
                instance->priv->source_id = 0;
                return FALSE;
        }

        if (cond & G_IO_IN) {
                instance->priv->idle = FALSE;
                if (instance->priv->source_id) {
                        g_source_remove (instance->priv->source_id);
                        instance->priv->source_id = 0;
                }
                ario_mpd_idle_read ();
        }

        return TRUE;
}

static void
ario_mpd_idle_start (void)
{
        ARIO_LOG_FUNCTION_START;
        static GIOChannel* iochan = NULL;

        if (!instance->priv->connection)
                return;

        if (!iochan) {
#ifdef WIN32
                iochan = g_io_channel_win32_new_socket (mpd_connection_get_fd (instance->priv->connection));
#else
                iochan = g_io_channel_unix_new (mpd_connection_get_fd (instance->priv->connection));
#endif
        }

        if (!instance->priv->idle) {
                instance->priv->source_id = g_io_add_watch (iochan,
                                                            G_IO_IN | G_IO_ERR | G_IO_HUP,
                                                            ario_mpd_idle_read_cb,
                                                            NULL);
                instance->priv->idle = TRUE;
                mpd_send_idle (instance->priv->connection);
        }
}

static void
ario_mpd_idle_stop (void)
{
        ARIO_LOG_FUNCTION_START;
        if (instance->priv->source_id) {
                g_source_remove (instance->priv->source_id);
                instance->priv->source_id = 0;
        }

        if (instance->priv->idle) {
                instance->priv->idle = FALSE;
                mpd_send_noidle (instance->priv->connection);
                ario_mpd_idle_read ();
        }
}

static void
ario_mpd_idle_init (void)
{
}

static void
ario_mpd_idle_free (void)
{
        ARIO_LOG_FUNCTION_START;
        ario_mpd_idle_stop ();

        if (instance->priv->source_id) {
                g_source_remove (instance->priv->source_id);
                instance->priv->source_id = 0;
        }
}

static gboolean
ario_mpd_connect_to (ArioMpd *mpd,
                     gchar *hostname,
                     int port,
                     guint timeout)
{
        ARIO_LOG_FUNCTION_START;
        gchar *password;
        struct mpd_connection *connection;

        /* Connect to MPD */
        connection = mpd_connection_new (hostname, port, timeout);
        if (!connection)
                return FALSE;

        /* Check connection errors */
        if  (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                ARIO_LOG_ERROR("%s", mpd_connection_get_error_message (connection));
                mpd_connection_clear_error (connection);
                mpd_connection_free (connection);
                return FALSE;
        }

        /* Send password if one is set in profile */
        password = ario_profiles_get_current (ario_profiles_get ())->password;
        if (password) {
                mpd_run_password (connection, password);
        }

        mpd->priv->connection = connection;

        /* Check if idle is supported by MPD server */
        ario_mpd_check_idle (mpd);

        if (instance->priv->support_idle && instance->priv->connection) {
                /* Initialise Idle mode */
                ario_mpd_idle_init ();
                ario_mpd_idle_start ();
                g_idle_add ((GSourceFunc) ario_mpd_update_status, NULL);

                /* Connect signal to launch timeout to update elapsed time */
                g_signal_connect_object (ario_server_get_instance (),
                                         "state_changed",
                                         G_CALLBACK (ario_mpd_server_state_changed_cb),
                                         NULL, 0);
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
        guint timeout;
        ArioProfile *profile;

        profile = ario_profiles_get_current (ario_profiles_get ());
        hostname = profile->host;
        port = profile->port;
        timeout = 5000;

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
                ario_mpd_idle_free ();
        mpd_connection_free (instance->priv->connection);
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
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_update (instance->priv->connection, NULL);

        ario_mpd_command_postinvoke ();
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

        if  (mpd_connection_get_error (instance->priv->connection) != MPD_ERROR_SUCCESS) {
                ARIO_LOG_ERROR("%s", mpd_connection_get_error_message (instance->priv->connection));
                mpd_connection_clear_error (instance->priv->connection);
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
        const GSList *tmp;
        GSList *values = NULL;
        ArioServerAtomicCriteria *atomic_criteria;
        struct mpd_pair *pair;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        mpd_search_db_tags (instance->priv->connection, tag);
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (instance->priv->support_empty_tags
                    && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        mpd_search_add_tag_constraint (instance->priv->connection,
                                                       MPD_OPERATOR_DEFAULT,
                                                       atomic_criteria->tag, "");
                else
                        mpd_search_add_tag_constraint (instance->priv->connection,
                                                       MPD_OPERATOR_DEFAULT,
                                                       atomic_criteria->tag, atomic_criteria->value);
        }
        mpd_search_commit (instance->priv->connection);

        while ((pair = mpd_recv_pair_tag (instance->priv->connection, tag))) {
                if (*pair->value)
                        values = g_slist_append (values, g_strdup(pair->value));
                else {
                        values = g_slist_append (values, g_strdup (ARIO_SERVER_UNKNOWN));
                        instance->priv->support_empty_tags = TRUE;
                }
                mpd_return_pair (instance->priv->connection, pair);
        }

        ario_mpd_command_postinvoke ();

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
        struct mpd_song *song;
        ArioServerAlbum *mpd_album;
        ArioServerAtomicCriteria *atomic_criteria;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        if (!criteria) {
                mpd_send_list_all_meta (instance->priv->connection, "/");
        } else {
                mpd_search_db_songs (instance->priv->connection, TRUE);
                for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                        atomic_criteria = tmp->data;

                        if (instance->priv->support_empty_tags
                            && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                                mpd_search_add_tag_constraint (instance->priv->connection,
                                                               MPD_OPERATOR_DEFAULT,
                                                               atomic_criteria->tag,
                                                               "");
                        else
                                mpd_search_add_tag_constraint (instance->priv->connection,
                                                               MPD_OPERATOR_DEFAULT,
                                                               atomic_criteria->tag,
                                                               atomic_criteria->value);
                }
                mpd_search_commit (instance->priv->connection);
        }

        while ((song = mpd_recv_song (instance->priv->connection))) {
                const char *artist;
                const char *album;
                const char *file;
                const char *date;
                artist = mpd_song_get_tag (song, MPD_TAG_ARTIST, 0);
                album = mpd_song_get_tag (song, MPD_TAG_ALBUM, 0);
                file = mpd_song_get_uri (song);
                date = mpd_song_get_tag (song, MPD_TAG_DATE, 0);

                if (!album) {
                        if (ario_mpd_album_is_present (albums, ARIO_SERVER_UNKNOWN)) {
                                mpd_song_free (song);
                                continue;
                        }
                } else {
                        if (ario_mpd_album_is_present (albums, album)) {
                                mpd_song_free (song);
                                continue;
                        }
                }

                mpd_album = (ArioServerAlbum *) g_malloc (sizeof (ArioServerAlbum));
                if (album)
                        mpd_album->album = g_strdup (album);
                else
                        mpd_album->album = g_strdup (ARIO_SERVER_UNKNOWN);

                if (artist)
                        mpd_album->artist = g_strdup (artist);
                else
                        mpd_album->artist = g_strdup (ARIO_SERVER_UNKNOWN);

                if (file)
                        mpd_album->path = g_strdup (file);
                else
                        mpd_album->path = NULL;

                if (date)
                        mpd_album->date = g_strdup (date);
                else
                        mpd_album->date = NULL;

                albums = g_slist_append (albums, mpd_album);

                mpd_song_free (song);
        }
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

        return albums;
}

static ArioServerSong *
ario_mpd_build_ario_song (const struct mpd_song *song)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerSong * ario_song;

        ario_song = (ArioServerSong *) g_malloc0 (sizeof (ArioServerSong));
        ario_song->file = g_strdup (mpd_song_get_uri (song));
        ario_song->artist = g_strdup (mpd_song_get_tag (song, MPD_TAG_ARTIST, 0));
        ario_song->title = g_strdup (mpd_song_get_tag (song, MPD_TAG_TITLE, 0));
        ario_song->album = g_strdup (mpd_song_get_tag (song, MPD_TAG_ALBUM, 0));
        ario_song->album_artist  = g_strdup (mpd_song_get_tag (song, MPD_TAG_ALBUM_ARTIST, 0));
        ario_song->track = g_strdup (mpd_song_get_tag (song, MPD_TAG_TRACK, 0));
        ario_song->name = g_strdup (mpd_song_get_tag (song, MPD_TAG_NAME, 0));
        ario_song->date = g_strdup (mpd_song_get_tag (song, MPD_TAG_DATE, 0));
        ario_song->genre = g_strdup (mpd_song_get_tag (song, MPD_TAG_GENRE, 0));
        ario_song->composer = g_strdup (mpd_song_get_tag (song, MPD_TAG_COMPOSER, 0));
        ario_song->performer = g_strdup (mpd_song_get_tag (song, MPD_TAG_PERFORMER, 0));
        ario_song->disc = g_strdup (mpd_song_get_tag (song, MPD_TAG_DISC, 0));
        ario_song->comment = g_strdup (mpd_song_get_tag (song, MPD_TAG_COMMENT, 0));
        ario_song->time = mpd_song_get_duration (song);
        ario_song->pos = mpd_song_get_pos (song);
        ario_song->id = mpd_song_get_id (song);

        return ario_song;
}

static ArioServerStats *
ario_mpd_build_ario_stats (struct mpd_stats *stats)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerStats * ario_stats;

        ario_stats = (ArioServerStats *) g_malloc0 (sizeof (ArioServerStats));
        ario_stats->numberOfArtists = mpd_stats_get_number_of_artists (stats);
        ario_stats->numberOfAlbums = mpd_stats_get_number_of_albums (stats);
        ario_stats->numberOfSongs = mpd_stats_get_number_of_songs (stats);
        ario_stats->uptime = mpd_stats_get_uptime (stats);
        ario_stats->dbUpdateTime = mpd_stats_get_db_update_time (stats);
        ario_stats->playTime = mpd_stats_get_play_time (stats);
        ario_stats->dbPlayTime = mpd_stats_get_db_play_time (stats);

        return ario_stats;
}

static ArioServerOutput *
ario_mpd_build_ario_output (struct mpd_output *output)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerOutput * ario_output;

        ario_output = (ArioServerOutput *) g_malloc0 (sizeof (ArioServerOutput));
        ario_output->id = mpd_output_get_id (output);
        ario_output->name = g_strdup (mpd_output_get_name (output));
        ario_output->enabled = mpd_output_get_enabled (output);

        return ario_output;
}

static GSList *
ario_mpd_get_songs (const ArioServerCriteria *criteria,
                    const gboolean exact)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;
        struct mpd_song *song;
        const GSList *tmp;
        gboolean is_album_unknown = FALSE;
        ArioServerAtomicCriteria *atomic_criteria;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (atomic_criteria->tag == ARIO_TAG_ALBUM
                    && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        is_album_unknown = TRUE;
        }

        mpd_search_db_songs (instance->priv->connection, exact);
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (instance->priv->support_empty_tags
                    && !g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        mpd_search_add_tag_constraint (instance->priv->connection,
                                                       MPD_OPERATOR_DEFAULT,
                                                       atomic_criteria->tag,
                                                       "");
                else if (atomic_criteria->tag != ARIO_TAG_ALBUM
                         || g_utf8_collate (atomic_criteria->value, ARIO_SERVER_UNKNOWN))
                        mpd_search_add_tag_constraint (instance->priv->connection,
                                                       MPD_OPERATOR_DEFAULT,
                                                       atomic_criteria->tag,
                                                       atomic_criteria->value);
        }
        mpd_search_commit (instance->priv->connection);

        while ((song = mpd_recv_song (instance->priv->connection))) {
                if (instance->priv->support_empty_tags
                    || !is_album_unknown
                    || !mpd_song_get_tag (song, MPD_TAG_ALBUM, 0)) {
                        songs = g_slist_append (songs, ario_mpd_build_ario_song (song));
                }
                mpd_song_free (song);
        }
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

        return songs;
}

static GSList *
ario_mpd_get_songs_from_playlist (char *playlist)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;
        struct mpd_song *song;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        mpd_send_list_playlist_meta (instance->priv->connection, playlist);
        while ((song = mpd_recv_song (instance->priv->connection))) {
                songs = g_slist_append (songs, ario_mpd_build_ario_song (song));
                mpd_song_free (song);
        }
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

        return songs;
}

static GSList *
ario_mpd_get_playlists (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *playlists = NULL;
        struct mpd_entity *ent;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        mpd_send_list_meta (instance->priv->connection, "/");

        while ((ent = mpd_recv_entity (instance->priv->connection))) {
                if (mpd_entity_get_type (ent) == MPD_ENTITY_TYPE_PLAYLIST) {
                        const struct mpd_playlist * playlist = mpd_entity_get_playlist (ent);
                        playlists = g_slist_append (playlists, g_strdup (mpd_playlist_get_path (playlist)));
                }
                mpd_entity_free (ent);
        }
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

        return playlists;
}

static GSList *
ario_mpd_get_playlist_changes (gint64 playlist_id)
{
        ARIO_LOG_FUNCTION_START;
        GSList *songs = NULL;
        struct mpd_song *song;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        mpd_send_queue_changes_meta (instance->priv->connection, (unsigned) playlist_id);
        while ((song = mpd_recv_song (instance->priv->connection))) {
                songs = g_slist_append (songs, ario_mpd_build_ario_song (song));
                mpd_song_free (song);
        }
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

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
        if (ario_mpd_command_preinvoke ()) {
                ario_server_interface_set_default (ARIO_SERVER_INTERFACE (instance));
        } else {
                if (instance->priv->status)
                        mpd_status_free (instance->priv->status);
                instance->priv->status = mpd_run_status (instance->priv->connection);

                if (ario_mpd_check_errors ()) {
                        ario_server_interface_set_default (ARIO_SERVER_INTERFACE (instance));
                } else if (instance->priv->status) {
                        if (instance->parent.song_id != mpd_status_get_song_id (instance->priv->status)
                            || instance->parent.playlist_id != (gint64) mpd_status_get_queue_version (instance->priv->status))
                                g_object_set (G_OBJECT (instance), "song_id", mpd_status_get_song_id (instance->priv->status), NULL);

                        if (instance->parent.state != mpd_status_get_state (instance->priv->status))
                                g_object_set (G_OBJECT (instance), "state", mpd_status_get_state (instance->priv->status), NULL);

                        if (instance->parent.volume != mpd_status_get_volume (instance->priv->status))
                                g_object_set (G_OBJECT (instance), "volume", mpd_status_get_volume (instance->priv->status), NULL);

                        if (instance->parent.elapsed != mpd_status_get_elapsed_time (instance->priv->status)) {
                                g_object_set (G_OBJECT (instance), "elapsed", mpd_status_get_elapsed_time (instance->priv->status), NULL);
                                instance->priv->elapsed = mpd_status_get_elapsed_time (instance->priv->status);
                        }

                        if (instance->parent.playlist_id != (gint64) mpd_status_get_queue_version (instance->priv->status)) {
                                g_object_set (G_OBJECT (instance), "playlist_id", (gint64) mpd_status_get_queue_version (instance->priv->status), NULL);
                                instance->parent.playlist_length = mpd_status_get_queue_length (instance->priv->status);
                        }

                        if (instance->parent.random != mpd_status_get_random (instance->priv->status))
                                g_object_set (G_OBJECT (instance), "random", mpd_status_get_random (instance->priv->status), NULL);

                        if (instance->parent.repeat != mpd_status_get_repeat (instance->priv->status))
                                g_object_set (G_OBJECT (instance), "repeat", mpd_status_get_repeat (instance->priv->status), NULL);

                        if (instance->parent.updatingdb != mpd_status_get_update_id (instance->priv->status))
                                g_object_set (G_OBJECT (instance), "updatingdb", mpd_status_get_update_id (instance->priv->status), NULL);
                        instance->parent.crossfade = mpd_status_get_crossfade (instance->priv->status);
                }
        }
        ario_server_interface_emit (ARIO_SERVER_INTERFACE (instance), server_instance);

        instance->priv->is_updating = FALSE;

        ario_mpd_command_postinvoke ();

        return !instance->priv->support_idle;
}

static ArioServerSong *
ario_mpd_get_current_song_on_server (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerSong *ario_song = NULL;
        struct mpd_song *song;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        mpd_send_current_song (instance->priv->connection);
        song = mpd_recv_song (instance->priv->connection);
        if (song) {
                ario_song = ario_mpd_build_ario_song (song);
                mpd_song_free (song);
        }
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

        return ario_song;
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

        ario_mpd_get_stats ();

        if (instance->priv->stats)
                return instance->priv->stats->dbUpdateTime;
        else
                return 0;
}

static void
ario_mpd_do_next (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_next (instance->priv->connection);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_do_prev (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_previous (instance->priv->connection);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_do_play (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_play (instance->priv->connection);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_do_play_pos (gint id)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        /* send mpd the play command */
        mpd_run_play_pos (instance->priv->connection, id);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_do_pause (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_pause (instance->priv->connection, TRUE);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_do_stop (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_stop (instance->priv->connection);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_set_current_elapsed (const gint elapsed)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_seek_id (instance->priv->connection, mpd_status_get_song_id (instance->priv->status), elapsed);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_set_current_volume (const gint volume)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_set_volume (instance->priv->connection, volume);
        ario_mpd_update_status ();

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_set_current_random (const gboolean random)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_random (instance->priv->connection, random);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_set_current_repeat (const gboolean repeat)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_repeat (instance->priv->connection, repeat);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_set_crossfadetime (const int crossfadetime)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_crossfade (instance->priv->connection, crossfadetime);

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_clear (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_clear (instance->priv->connection);
        ario_mpd_update_status ();

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_shuffle (void)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_shuffle (instance->priv->connection);
        ario_mpd_update_status ();

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_queue_commit (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *temp;
        ArioServerQueueAction *queue_action;

        if (ario_mpd_command_preinvoke ())
                return;

        mpd_command_list_begin (instance->priv->connection, FALSE);

        for (temp = instance->parent.queue; temp; temp = g_slist_next (temp)) {
                queue_action = (ArioServerQueueAction *) temp->data;
                if (queue_action->type == ARIO_SERVER_ACTION_ADD) {
                        if (queue_action->path) {
                                mpd_send_add (instance->priv->connection, queue_action->path);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_DELETE_ID) {
                        if (queue_action->id >= 0) {
                                mpd_send_delete_id (instance->priv->connection, queue_action->id);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_DELETE_POS) {
                        if (queue_action->pos >= 0) {
                                mpd_send_delete (instance->priv->connection, queue_action->pos);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_MOVE) {
                        if (queue_action->id >= 0) {
                                mpd_send_move (instance->priv->connection, queue_action->old_pos, queue_action->new_pos);
                        }
                } else if (queue_action->type == ARIO_SERVER_ACTION_MOVEID) {
                        if (queue_action->id >= 0) {
                                mpd_send_move_id (instance->priv->connection, queue_action->old_pos, queue_action->new_pos);
                        }
                }
        }
        mpd_command_list_end (instance->priv->connection);
        mpd_response_finish (instance->priv->connection);
        ario_mpd_update_status ();

        g_slist_foreach (instance->parent.queue, (GFunc) g_free, NULL);
        g_slist_free (instance->parent.queue);
        instance->parent.queue = NULL;

        ario_mpd_command_postinvoke ();
}

static void
ario_mpd_insert_at (const GSList *songs,
                    const gint pos)
{
        ARIO_LOG_FUNCTION_START;
        const GSList *tmp;

        if (ario_mpd_command_preinvoke ())
                return;

        mpd_command_list_begin (instance->priv->connection, FALSE);

        /* For each filename :*/
        for (tmp = songs; tmp; tmp = g_slist_next (tmp)) {
                /* Add it in the playlist*/
                mpd_send_add_id_to (instance->priv->connection, tmp->data, pos + 1);
        }

        mpd_command_list_end (instance->priv->connection);
        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();
}

static int
ario_mpd_save_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START;
        int ret;

        if (ario_mpd_command_preinvoke ())
                return 1;

        ret = mpd_run_save (instance->priv->connection, name);

        ario_mpd_command_postinvoke ();

        return ret;
}

static void
ario_mpd_delete_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_mpd_command_preinvoke ())
                return;

        mpd_run_rm (instance->priv->connection, name);

        ario_mpd_command_postinvoke ();
}

static GSList *
ario_mpd_get_outputs (void)
{
        ARIO_LOG_FUNCTION_START;
        GSList *outputs = NULL;
        struct mpd_output *output_ent;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        mpd_send_outputs (instance->priv->connection);

        while ((output_ent = mpd_recv_output (instance->priv->connection))) {
                outputs = g_slist_append (outputs, ario_mpd_build_ario_output (output_ent));
                mpd_output_free (output_ent);
        }

        mpd_response_finish (instance->priv->connection);

        ario_mpd_command_postinvoke ();

        return outputs;
}

static void
ario_mpd_enable_output (int id,
                        gboolean enabled)
{
        ARIO_LOG_FUNCTION_START;

        if (ario_mpd_command_preinvoke ())
                return;

        if (enabled) {
                mpd_run_enable_output (instance->priv->connection, id);
        } else {
                mpd_run_disable_output (instance->priv->connection, id);
        }

        ario_mpd_command_postinvoke ();
}

static ArioServerStats *
ario_mpd_get_stats (void)
{
        ARIO_LOG_FUNCTION_START;
        struct mpd_stats *stats;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        stats = mpd_run_stats (instance->priv->connection);

        if (instance->priv->stats)
                g_free (instance->priv->stats);
        instance->priv->stats  = ario_mpd_build_ario_stats (stats);

        mpd_stats_free (stats);

        ario_mpd_check_errors ();

        ario_mpd_command_postinvoke ();

        return instance->priv->stats;
}

static GList *
ario_mpd_get_songs_info (GSList *paths)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *path = NULL;
        GSList *temp;
        GList *songs = NULL;
        struct mpd_song *song;

        if (ario_mpd_command_preinvoke ())
                return NULL;

        for (temp = paths; temp; temp = g_slist_next (temp)) {
                path = temp->data;

                mpd_send_list_all_meta (instance->priv->connection, path);

                song = mpd_recv_song (instance->priv->connection);

                mpd_response_finish (instance->priv->connection);
                if (!song)
                        continue;

                songs = g_list_append (songs, ario_mpd_build_ario_song (song));
                mpd_song_free (song);

                ario_mpd_check_errors ();
        }

        ario_mpd_command_postinvoke ();

        return songs;
}

static ArioServerFileList *
ario_mpd_list_files (const char *path,
                     gboolean recursive)
{
        ARIO_LOG_FUNCTION_START;
        struct mpd_entity *entity;
        ArioServerFileList *files = (ArioServerFileList *) g_malloc0 (sizeof (ArioServerFileList));

        if (ario_mpd_command_preinvoke ())
                return files;

        if (recursive)
                mpd_send_list_all_meta (instance->priv->connection, path);
        else
                mpd_send_list_meta (instance->priv->connection, path);

        while ((entity = mpd_recv_entity (instance->priv->connection))) {
                enum mpd_entity_type type = mpd_entity_get_type (entity);
                if (type == MPD_ENTITY_TYPE_DIRECTORY) {
                        const struct mpd_directory * directory = mpd_entity_get_directory (entity);
                        files->directories = g_slist_append (files->directories, g_strdup (mpd_directory_get_path (directory)));
                } else if (type == MPD_ENTITY_TYPE_SONG) {
                        const struct mpd_song * song = mpd_entity_get_song (entity);
                        files->songs = g_slist_append (files->songs, ario_mpd_build_ario_song (song));
                }

                mpd_entity_free(entity);
        }

        ario_mpd_command_postinvoke ();

        return files;
}

static gboolean
ario_mpd_command_preinvoke (void)
{
        ARIO_LOG_FUNCTION_START;
        /* check if there is a connection */
        if (!instance->priv->connection)
                return TRUE;

        if (instance->priv->support_idle) {
                ario_mpd_idle_stop ();

                if (!instance->priv->connection)
                        return TRUE;
        }

        return FALSE;
}

static void
ario_mpd_command_postinvoke (void)
{
        ARIO_LOG_FUNCTION_START;
        if (instance->priv->support_idle && instance->priv->connection) {
                ario_mpd_idle_start ();
        }
}

static void
ario_mpd_server_state_changed_cb (ArioServer *server,
                                  gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        if (instance->priv->timeout_id) {
                g_source_remove (instance->priv->timeout_id);
                instance->priv->timeout_id = 0;
        }

        if (ario_server_get_current_state () == ARIO_STATE_PLAY) {
                instance->priv->timeout_id = g_timeout_add (ONE_SECOND,
                                                            (GSourceFunc) ario_mpd_update_elapsed,
                                                            NULL);
        }
}
