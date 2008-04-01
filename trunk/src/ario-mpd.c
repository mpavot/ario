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
#include "ario-mpd.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define NORMAL_TIMEOUT 500
#define LAZY_TIMEOUT 12000

static void ario_mpd_class_init (ArioMpdClass *klass);
static void ario_mpd_init (ArioMpd *mpd);
static void ario_mpd_finalize (GObject *object);
static void ario_mpd_set_default (ArioMpd *mpd);
void ario_mpd_check_errors (ArioMpd *mpd);
static void ario_mpd_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
static void ario_mpd_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec);
void ario_mpd_launch_timeout (ArioMpd *mpd);

typedef enum
{
        ARIO_MPD_ACTION_ADD,
        ARIO_MPD_ACTION_DELETE_ID,
        ARIO_MPD_ACTION_DELETE_POS,
        ARIO_MPD_ACTION_MOVE
}ArioMpdActionType;

typedef struct ArioMpdQueueAction {
        ArioMpdActionType type;
        union {
                char *path;             // For ARIO_MPD_ACTION_ADD
                int id;                 // For ARIO_MPD_ACTION_DELETE_ID
                int pos;                // For ARIO_MPD_ACTION_DELETE_POS
                struct {                // For ARIO_MPD_ACTION_MOVE
                        int old_pos;
                        int new_pos;
                };
        };
} ArioMpdQueueAction;

struct ArioMpdPrivate
{        
        mpd_Status *status;
        mpd_Connection *connection;
        mpd_Stats *stats;

        int song_id;
        int state;
        int volume;
        int elapsed;

        int total_time;

        ArioMpdSong *ario_mpd_song;
        int playlist_id;
        int playlist_length;

        gboolean random;
        gboolean repeat;

        int updatingdb;

        GSList *queue;
        gboolean is_updating; 

        int signals_to_emit;

        int use_count;
        guint timeout_id;

        gboolean connecting;
};

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

enum
{
        SONG_CHANGED,
        ALBUM_CHANGED,
        STATE_CHANGED,
        VOLUME_CHANGED,
        ELAPSED_CHANGED,
        PLAYLIST_CHANGED,
        RANDOM_CHANGED,
        REPEAT_CHANGED,
        UPDATINGDB_CHANGED,
        STOREDPLAYLISTS_CHANGED,
        LAST_SIGNAL
};
static guint ario_mpd_signals[LAST_SIGNAL] = { 0 };

enum
{
        SONG_CHANGED_FLAG = 2 << SONG_CHANGED,
        ALBUM_CHANGED_FLAG = 2 << ALBUM_CHANGED,
        STATE_CHANGED_FLAG = 2 << STATE_CHANGED,
        VOLUME_CHANGED_FLAG = 2 << VOLUME_CHANGED,
        ELAPSED_CHANGED_FLAG = 2 << ELAPSED_CHANGED,
        PLAYLIST_CHANGED_FLAG = 2 << PLAYLIST_CHANGED,
        RANDOM_CHANGED_FLAG = 2 << RANDOM_CHANGED,
        REPEAT_CHANGED_FLAG = 2 << REPEAT_CHANGED,
        UPDATINGDB_CHANGED_FLAG = 2 << UPDATINGDB_CHANGED,
        STOREDPLAYLISTS_CHANGED_FLAG = 2 << STOREDPLAYLISTS_CHANGED
};

static GObjectClass *parent_class = NULL;

GType
ario_mpd_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioMpdClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_mpd_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioMpd),
                        0,
                        (GInstanceInitFunc) ario_mpd_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "ArioMpd",
                                               &our_info, 0);
        }
        return type;
}

static void
ario_mpd_class_init (ArioMpdClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_mpd_finalize;

        object_class->set_property = ario_mpd_set_property;
        object_class->get_property = ario_mpd_get_property;

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

        ario_mpd_signals[SONG_CHANGED] =
                g_signal_new ("song_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, song_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[ALBUM_CHANGED] =
                g_signal_new ("album_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, album_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[STATE_CHANGED] =
                g_signal_new ("state_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, state_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[VOLUME_CHANGED] =
                g_signal_new ("volume_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, volume_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[ELAPSED_CHANGED] =
                g_signal_new ("elapsed_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, elapsed_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[PLAYLIST_CHANGED] =
                g_signal_new ("playlist_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[RANDOM_CHANGED] =
                g_signal_new ("random_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[REPEAT_CHANGED] =
                g_signal_new ("repeat_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[UPDATINGDB_CHANGED] =
                g_signal_new ("updatingdb_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, updatingdb_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[STOREDPLAYLISTS_CHANGED] =
                g_signal_new ("storedplaylists_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, storedplaylists_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
ario_mpd_init (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        mpd->priv = g_new0 (ArioMpdPrivate, 1);

        mpd->priv->song_id = -1;
        mpd->priv->playlist_id = -1;
        mpd->priv->volume = 0;
        mpd->priv->is_updating = FALSE;
        mpd->priv->use_count = 0;
        mpd->priv->timeout_id = 0;
}

static void
ario_mpd_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_MPD (object));

        mpd = ARIO_MPD (object);
        g_return_if_fail (mpd->priv != NULL);

        ario_mpd_disconnect (mpd);

        if (mpd->priv->ario_mpd_song)
                ario_mpd_free_song (mpd->priv->ario_mpd_song);

        if (mpd->priv->status)
                mpd_freeStatus (mpd->priv->status);

        if (mpd->priv->stats)
                mpd_freeStats (mpd->priv->stats);

        g_free (mpd->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_mpd_set_property (GObject *object,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd = ARIO_MPD (object);

        switch (prop_id) {
        case PROP_SONGID:
                mpd->priv->song_id = g_value_get_int (value);

                /* check if there is a connection */
                if (mpd->priv->connection) {
                        mpd_InfoEntity *ent ;
                        ArioMpdSong *new_song = NULL;
                        ArioMpdSong *old_song = mpd->priv->ario_mpd_song;
                        gboolean state_changed;
                        gboolean artist_changed = FALSE;
                        gboolean album_changed = FALSE;

                        mpd_sendCurrentSongCommand (mpd->priv->connection);
                        ent = mpd_getNextInfoEntity (mpd->priv->connection);
                        if (ent)
                                new_song = ent->info.song;
                        mpd_finishCommand (mpd->priv->connection);
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
                                mpd->priv->signals_to_emit |= ALBUM_CHANGED_FLAG;

                        if (mpd->priv->ario_mpd_song)
                                ario_mpd_free_song (mpd->priv->ario_mpd_song);

                        if (new_song) {
                                mpd->priv->ario_mpd_song = new_song;
                                ent->info.song = NULL;
                        } else {
                                mpd->priv->ario_mpd_song = NULL;
                        }

                        if (ent)
                                mpd_freeInfoEntity (ent);

                        if (mpd->priv->status)
                                mpd->priv->total_time = mpd->priv->status->totalTime;
                } else {
                        if (mpd->priv->ario_mpd_song) {
                                ario_mpd_free_song (mpd->priv->ario_mpd_song);
                                mpd->priv->ario_mpd_song = NULL;
                        }
                }

                mpd->priv->signals_to_emit |= SONG_CHANGED_FLAG;
                break;
        case PROP_STATE:
                mpd->priv->state = g_value_get_int (value);
                if (mpd->priv->status)
                        mpd->priv->total_time = mpd->priv->status->totalTime;
                else
                        mpd->priv->total_time = 0;
                mpd->priv->signals_to_emit |= STATE_CHANGED_FLAG;
                break;
        case PROP_VOLUME:
                mpd->priv->volume = g_value_get_int (value);
                mpd->priv->signals_to_emit |= VOLUME_CHANGED_FLAG;
                break;
        case PROP_ELAPSED:
                mpd->priv->elapsed = g_value_get_int (value);
                mpd->priv->signals_to_emit |= ELAPSED_CHANGED_FLAG;
                break;
        case PROP_PLAYLISTID:
                mpd->priv->playlist_id = g_value_get_int (value);
                if (mpd->priv->connection)
                        mpd->priv->playlist_length = mpd->priv->status->playlistLength;
                else
                        mpd->priv->playlist_length = 0;
                mpd->priv->signals_to_emit |= PLAYLIST_CHANGED_FLAG;
                break;
        case PROP_RANDOM:
                mpd->priv->random = g_value_get_boolean (value);
                mpd->priv->signals_to_emit |= RANDOM_CHANGED_FLAG;
                break;
        case PROP_REPEAT:
                mpd->priv->repeat = g_value_get_boolean (value);
                mpd->priv->signals_to_emit |= REPEAT_CHANGED_FLAG;
                break;
        case PROP_UPDATINGDB:
                mpd->priv->updatingdb = g_value_get_int (value);
                mpd->priv->signals_to_emit |= UPDATINGDB_CHANGED_FLAG;
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_mpd_get_property (GObject *object,
                       guint prop_id,
                       GValue *value,
                       GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd = ARIO_MPD (object);

        switch (prop_id) {
        case PROP_SONGID:
                g_value_set_int (value, mpd->priv->song_id);
                break;
        case PROP_STATE:
                g_value_set_int (value, mpd->priv->state);
                break;
        case PROP_VOLUME:
                g_value_set_int (value, mpd->priv->volume);
                break;
        case PROP_ELAPSED:
                g_value_set_int (value, mpd->priv->elapsed);
                break;
        case PROP_PLAYLISTID:
                g_value_set_int (value, mpd->priv->playlist_id);
                break;
        case PROP_RANDOM:
                g_value_set_boolean (value, mpd->priv->random);
                break;
        case PROP_REPEAT:
                g_value_set_boolean (value, mpd->priv->repeat);
                break;
        case PROP_UPDATINGDB:
                g_value_set_int (value, mpd->priv->updatingdb);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

ArioMpd *
ario_mpd_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd;

        mpd = g_object_new (TYPE_ARIO_MPD,
                            NULL);

        g_return_val_if_fail (mpd->priv != NULL, NULL);

        return ARIO_MPD (mpd);
}

static gboolean
ario_mpd_connect_to (ArioMpd *mpd,
                     gchar *hostname,
                     int port,
                     float timeout)
{
        ARIO_LOG_FUNCTION_START
        mpd_Stats *stats;
        gchar *password;
        mpd_Connection *connection;

        connection = mpd_newConnection (hostname, port, timeout);

        if (!connection)
                return FALSE;

        password = ario_conf_get_string (PREF_PASSWORD, PREF_PASSWORD_DEFAULT);
        if (password) {
                mpd_sendPasswordCommand (connection, password);
                mpd_finishCommand (connection);
                g_free (password);
        }

        mpd_sendStatsCommand (connection);
        stats = mpd_getStats (connection);
        mpd_finishCommand (connection);
        if (stats == NULL) {
                mpd_closeConnection (connection);
                mpd->priv->connection = NULL;
                return FALSE;
        }

        mpd_freeStats(stats);

        mpd->priv->connection = connection;

        return TRUE;
}

static gpointer
ario_mpd_connect_thread (ArioMpd *mpd)
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

        if (!ario_mpd_connect_to (mpd, hostname, port, timeout)) {
                ario_mpd_disconnect (mpd);
        }

        g_free (hostname);
        mpd->priv->connecting = FALSE;

        return NULL;
}

gboolean
ario_mpd_connect (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *win, *vbox,*label, *bar;
        GThread* thread;
        GtkWidget *dialog;

        /* check if there is a connection */
        if (mpd->priv->connection || mpd->priv->connecting)
                return FALSE;

        mpd->priv->connecting = TRUE;

        thread = g_thread_create ((GThreadFunc) ario_mpd_connect_thread,
                                  mpd, TRUE, NULL);

        win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_modal (GTK_WINDOW (win), TRUE);
        vbox = gtk_vbox_new (FALSE, 0);
        label = gtk_label_new (_("Connecting to MPD server..."));
        bar = gtk_progress_bar_new ();

        gtk_container_add (GTK_CONTAINER (win), vbox);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 6);
        gtk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 6);

        gtk_window_set_resizable (GTK_WINDOW (win), FALSE);
        gtk_window_set_title (GTK_WINDOW (win), "Ario");
        gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER);
        gtk_widget_show_all (win);

        while (mpd->priv->connecting) {
                gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));
                while (gtk_events_pending ())
                        gtk_main_iteration ();
                g_usleep (200000);
        }

        g_thread_join (thread);

        if (!ario_mpd_is_connected (mpd)) {
                dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, 
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 _("Impossible to connect to mpd. Check the connection options."));
                if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_NONE)
                        gtk_widget_destroy (dialog);
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[STATE_CHANGED], 0);
        }
                
        gtk_widget_hide (win);
        gtk_widget_destroy (win);

        return FALSE;
}

void
ario_mpd_disconnect (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_closeConnection (mpd->priv->connection);
        mpd->priv->connection = NULL;
}

void
ario_mpd_update_db (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendUpdateCommand (mpd->priv->connection, "");
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_check_errors (ArioMpd *mpd)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START
        if (!mpd->priv->connection)
                return;

        if  (mpd->priv->connection->error) {
                ARIO_LOG_DBG("Error nb: %d, Msg:%s\n", mpd->priv->connection->errorCode, mpd->priv->connection->errorStr);
                ario_mpd_disconnect(mpd);
        }
}

gboolean
ario_mpd_is_connected (ArioMpd *mpd)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START
        return (mpd->priv->connection != NULL);
}

GSList *
ario_mpd_get_artists (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GSList *artists = NULL;
        gchar *artist_char;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        mpd_sendListCommand (mpd->priv->connection, MPD_TABLE_ARTIST, NULL);

        while ((artist_char = mpd_getNextArtist (mpd->priv->connection)))
                artists = g_slist_append (artists, artist_char);

        mpd_finishCommand (mpd->priv->connection);

        return artists;
}

GSList *
ario_mpd_get_albums (ArioMpd *mpd,
                     const char *artist)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL;
        mpd_InfoEntity *entity = NULL;
        gchar *prev_album = "";
        ArioMpdAlbum *ario_mpd_album;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        mpd_sendFindCommand (mpd->priv->connection, MPD_TABLE_ARTIST, artist);
        while ((entity = mpd_getNextInfoEntity (mpd->priv->connection))) {
                if (!entity->info.song->album) {
                        if (!g_utf8_collate (prev_album, ARIO_MPD_UNKNOWN)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                } else {
                        if (!g_utf8_collate (entity->info.song->album, prev_album)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                }

                ario_mpd_album = (ArioMpdAlbum *) g_malloc (sizeof (ArioMpdAlbum));
                if (entity->info.song->album) {
                        ario_mpd_album->album = entity->info.song->album;
                        entity->info.song->album = NULL;
                } else {
                        ario_mpd_album->album = g_strdup (ARIO_MPD_UNKNOWN);
                }

                if (entity->info.song->artist) {
                        ario_mpd_album->artist = entity->info.song->artist;
                        entity->info.song->artist = NULL;
                } else {
                        ario_mpd_album->artist = g_strdup (ARIO_MPD_UNKNOWN);
                }

                if (entity->info.song->file) {
                        ario_mpd_album->path = g_path_get_dirname (entity->info.song->file);
                } else {
                        ario_mpd_album->path = NULL;
                }

                prev_album = ario_mpd_album->album;
                albums = g_slist_append (albums, ario_mpd_album);

                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (mpd->priv->connection);

        return albums;
}

GSList *
ario_mpd_get_songs (ArioMpd *mpd,
                    const char *artist, 
                    const char *album)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        mpd_InfoEntity *entity = NULL;
        gboolean is_album_unknown;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        is_album_unknown = !g_utf8_collate (album, ARIO_MPD_UNKNOWN);

        if (is_album_unknown)
                mpd_sendFindCommand (mpd->priv->connection, MPD_TABLE_ARTIST, artist);
        else
                mpd_sendFindCommand (mpd->priv->connection, MPD_TABLE_ALBUM, album);

        while ((entity = mpd_getNextInfoEntity (mpd->priv->connection))) {
                if (!g_utf8_collate (entity->info.song->artist, artist)) {
                        if (entity->info.song)
                                if (!is_album_unknown || !entity->info.song->album) {
                                        songs = g_slist_append (songs, entity->info.song);
                                        entity->info.song = NULL;
                                }
                }
                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (mpd->priv->connection);

        return songs;
}

GSList *
ario_mpd_get_songs_from_playlist (ArioMpd *mpd,
                                  char *playlist)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        mpd_InfoEntity *ent = NULL;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        mpd_sendListPlaylistInfoCommand(mpd->priv->connection, playlist);
        while ((ent = mpd_getNextInfoEntity(mpd->priv->connection))) {
                songs = g_slist_append (songs, ent->info.song);
                ent->info.song = NULL;
                mpd_freeInfoEntity(ent);
        }
        mpd_finishCommand (mpd->priv->connection);

        return songs;
}

GSList *
ario_mpd_get_playlists (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GSList *playlists = NULL;
        mpd_InfoEntity *ent = NULL;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        mpd_sendLsInfoCommand(mpd->priv->connection, "/");

        while ((ent = mpd_getNextInfoEntity (mpd->priv->connection))) {
                if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
                        playlists = g_slist_append (playlists, g_strdup (ent->info.playlistFile->path));
                }
                mpd_freeInfoEntity (ent);
        }
        mpd_finishCommand (mpd->priv->connection);

        return playlists;
}

GSList *
ario_mpd_get_playlist_changes (ArioMpd *mpd,
                               int playlist_id)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        mpd_InfoEntity *entity = NULL;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        mpd_sendPlChangesCommand (mpd->priv->connection, playlist_id);

        while ((entity = mpd_getNextInfoEntity (mpd->priv->connection))) {
                if (entity->info.song) {
                        songs = g_slist_append (songs, entity->info.song);
                        entity->info.song = NULL;
                }
                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (mpd->priv->connection);

        return songs;
}

mpd_Connection *
ario_mpd_get_connection (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->connection;
}

void
ario_mpd_set_default (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->song_id != 0)
                g_object_set (G_OBJECT (mpd), "song_id", 0, NULL);

        if (mpd->priv->state != MPD_STATUS_STATE_UNKNOWN)
                g_object_set (G_OBJECT (mpd), "state", MPD_STATUS_STATE_UNKNOWN, NULL);

        if (mpd->priv->volume != MPD_STATUS_NO_VOLUME)
                g_object_set (G_OBJECT (mpd), "volume", MPD_STATUS_NO_VOLUME, NULL);

        if (mpd->priv->elapsed != 0)
                g_object_set (G_OBJECT (mpd), "elapsed", 0, NULL);

        if (mpd->priv->playlist_id != -1)
                g_object_set (G_OBJECT (mpd), "playlist_id", -1, NULL);

        if (mpd->priv->random != FALSE)
                g_object_set (G_OBJECT (mpd), "random", FALSE, NULL);

        if (mpd->priv->repeat != FALSE)
                g_object_set (G_OBJECT (mpd), "repeat", FALSE, NULL);

        if (mpd->priv->updatingdb != 0)
                g_object_set (G_OBJECT (mpd), "updatingdb", 0, NULL);
}

gboolean
ario_mpd_update_status (ArioMpd *mpd)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START

        if (mpd->priv->is_updating)
                return TRUE;
        mpd->priv->is_updating = TRUE;
        mpd->priv->signals_to_emit = 0;

        /* check if there is a connection */
        if (!mpd->priv->connection) {
                ario_mpd_set_default (mpd);
        } else {
                if (mpd->priv->status)
                        mpd_freeStatus (mpd->priv->status);
                mpd_sendStatusCommand (mpd->priv->connection);
                mpd->priv->status = mpd_getStatus (mpd->priv->connection);

                ario_mpd_check_errors(mpd);

                if (mpd->priv->status) {
                        if (mpd->priv->song_id != mpd->priv->status->songid)
                                g_object_set (G_OBJECT (mpd), "song_id", mpd->priv->status->songid, NULL);

                        if (mpd->priv->state != mpd->priv->status->state)
                                g_object_set (G_OBJECT (mpd), "state", mpd->priv->status->state, NULL);

                        if (mpd->priv->volume != mpd->priv->status->volume)
                                g_object_set (G_OBJECT (mpd), "volume", mpd->priv->status->volume, NULL);

                        if (mpd->priv->elapsed != mpd->priv->status->elapsedTime)
                                g_object_set (G_OBJECT (mpd), "elapsed", mpd->priv->status->elapsedTime, NULL);

                        if (mpd->priv->playlist_id != (int) mpd->priv->status->playlist) {
                                g_object_set (G_OBJECT (mpd), "song_id", mpd->priv->status->songid, NULL);
                                g_object_set (G_OBJECT (mpd), "playlist_id", mpd->priv->status->playlist, NULL);
                        }

                        if (mpd->priv->random != (gboolean) mpd->priv->status->random)
                                g_object_set (G_OBJECT (mpd), "random", mpd->priv->status->random, NULL);

                        if (mpd->priv->repeat != (gboolean) mpd->priv->status->repeat)
                                g_object_set (G_OBJECT (mpd), "repeat", mpd->priv->status->repeat, NULL);

                        if (mpd->priv->updatingdb != mpd->priv->status->updatingDb)
                                g_object_set (G_OBJECT (mpd), "updatingdb", mpd->priv->status->updatingDb, NULL);
                }
        }

        if (mpd->priv->signals_to_emit & SONG_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[SONG_CHANGED], 0);
        if (mpd->priv->signals_to_emit & ALBUM_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[ALBUM_CHANGED], 0);
        if (mpd->priv->signals_to_emit & STATE_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[STATE_CHANGED], 0);
        if (mpd->priv->signals_to_emit & VOLUME_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[VOLUME_CHANGED], 0);
        if (mpd->priv->signals_to_emit & ELAPSED_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[ELAPSED_CHANGED], 0);
        if (mpd->priv->signals_to_emit & PLAYLIST_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[PLAYLIST_CHANGED], 0);
        if (mpd->priv->signals_to_emit & RANDOM_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[RANDOM_CHANGED], 0);
        if (mpd->priv->signals_to_emit & REPEAT_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[REPEAT_CHANGED], 0);
        if (mpd->priv->signals_to_emit & UPDATINGDB_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[UPDATINGDB_CHANGED], 0);

        mpd->priv->is_updating = FALSE;
        return TRUE;
}

char *
ario_mpd_get_current_title (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->title;
        else
                return NULL;
}

char *
ario_mpd_get_current_name (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->name;
        else
                return NULL;
}

ArioMpdSong *
ario_mpd_get_current_song (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song;
        else
                return NULL;
}

char *
ario_mpd_get_current_artist (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->artist;
        else
                return NULL;
}

char *
ario_mpd_get_current_album (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->album;
        else
                return NULL;
}

int
ario_mpd_get_current_song_id (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->song_id;
}

int
ario_mpd_get_current_state (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->state;
}

int
ario_mpd_get_current_elapsed (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->elapsed;
}

int
ario_mpd_get_current_volume (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->volume;
}

int
ario_mpd_get_current_total_time (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->total_time;
}

int
ario_mpd_get_current_playlist_id (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->playlist_id;
}

int
ario_mpd_get_current_playlist_length (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->playlist_length;
}

int
ario_mpd_get_current_playlist_total_time (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        int total_time = 0;
        ArioMpdSong *song;
        mpd_InfoEntity *ent = NULL;

        if (!mpd->priv->connection)
                return 0;

        // We go to MPD server for each call to this function but it is not a problem as it is
        // called only once for each playlist change (by status bar). I may change this implementation
        // if this function is called more than once for performance reasons.
        mpd_sendPlaylistInfoCommand(mpd->priv->connection, -1);
        while ((ent = mpd_getNextInfoEntity (mpd->priv->connection))) {
                song = ent->info.song;
                if (song->time != MPD_SONG_NO_TIME)
                        total_time = total_time + song->time;
                mpd_freeInfoEntity(ent);
        }
        mpd_finishCommand (mpd->priv->connection);

        return total_time;
}

int
ario_mpd_get_crossfadetime (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->status)
                return mpd->priv->status->crossfade;
        else
                return 0;
}

gboolean
ario_mpd_get_current_random (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->random;
}

gboolean
ario_mpd_get_current_repeat (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->repeat;
}

gboolean
ario_mpd_get_updating (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->status)
                return mpd->priv->updatingdb;
        else
                return FALSE;
}

unsigned long
ario_mpd_get_last_update (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return 0;

        if (mpd->priv->stats)
                mpd_freeStats (mpd->priv->stats);
        mpd_sendStatsCommand (mpd->priv->connection);
        mpd->priv->stats = mpd_getStats (mpd->priv->connection);

        ario_mpd_check_errors(mpd);

        if (mpd->priv->stats)
                return mpd->priv->stats->dbUpdateTime;
        else
                return 0;
}

gboolean
ario_mpd_is_paused (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return (mpd->priv->state == MPD_STATUS_STATE_PAUSE) || (mpd->priv->state == MPD_STATUS_STATE_STOP);
}

void
ario_mpd_do_next (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendNextCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_prev (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendPrevCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_play (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendPlayCommand (mpd->priv->connection, -1);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_play_id (ArioMpd *mpd,
                     gint id)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        /* send mpd the play command */
        mpd_sendPlayIdCommand (mpd->priv->connection, id);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_pause (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendPauseCommand (mpd->priv->connection, TRUE);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_stop (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendStopCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_free_album (ArioMpdAlbum *ario_mpd_album)
{
        ARIO_LOG_FUNCTION_START
        if (ario_mpd_album) {
                g_free (ario_mpd_album->album);
                g_free (ario_mpd_album->artist);
                g_free (ario_mpd_album->path);
                g_free (ario_mpd_album);
        }
}

ArioMpdAlbum*
ario_mpd_copy_album (ArioMpdAlbum *ario_mpd_album)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdAlbum *ret = NULL;

        if (ario_mpd_album) {
                ret = (ArioMpdAlbum *) g_malloc (sizeof (ArioMpdAlbum));
                ret->album = g_strdup (ario_mpd_album->album);
                ret->artist = g_strdup (ario_mpd_album->artist);
                ret->path = g_strdup (ario_mpd_album->path);
        }

        return ret;
}

void
ario_mpd_set_current_elapsed (ArioMpd *mpd,
                              gint elapsed)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendSeekCommand (mpd->priv->connection, mpd->priv->status->song, elapsed);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_current_volume (ArioMpd *mpd,
                             gint volume)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendSetvolCommand (mpd->priv->connection, volume);
        mpd_finishCommand (mpd->priv->connection);
        ario_mpd_update_status (mpd);
}

void
ario_mpd_set_current_random (ArioMpd *mpd,
                             gboolean random)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendRandomCommand (mpd->priv->connection, random);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_current_repeat (ArioMpd *mpd,
                             gboolean repeat)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendRepeatCommand (mpd->priv->connection, repeat);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_crossfadetime (ArioMpd *mpd,
                            int crossfadetime)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendCrossfadeCommand (mpd->priv->connection, crossfadetime);        
        mpd_finishCommand (mpd->priv->connection);          
}

void
ario_mpd_clear (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendClearCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
        ario_mpd_update_status (mpd);
}

void
ario_mpd_remove (ArioMpd *mpd,
                 GArray *song)
{
        ARIO_LOG_FUNCTION_START
        guint i;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendCommandListBegin (mpd->priv->connection);

        for (i=0; i<song->len; ++i)
                if ((g_array_index (song, int, i) - i) >= 0)
                        mpd_sendDeleteCommand (mpd->priv->connection, g_array_index (song, int, i) - i);

        mpd_sendCommandListEnd (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_queue_add (ArioMpd *mpd,
                    char* path)
{
        ARIO_LOG_FUNCTION_START

        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_ADD;
        queue_action->path = path;

        mpd->priv->queue = g_slist_append (mpd->priv->queue, queue_action);
}

void
ario_mpd_queue_delete_id (ArioMpd *mpd,
                          int id)
{
        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_DELETE_ID;
        queue_action->id = id;

        mpd->priv->queue = g_slist_append (mpd->priv->queue, queue_action);
}

void
ario_mpd_queue_delete_pos (ArioMpd *mpd,
                           int pos)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_DELETE_POS;
        queue_action->pos = pos;

        mpd->priv->queue = g_slist_append (mpd->priv->queue, queue_action);
}

void
ario_mpd_queue_move (ArioMpd *mpd,
                     int old_pos,
                     int new_pos)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_MOVE;
        queue_action->old_pos = old_pos;
        queue_action->new_pos = new_pos;

        mpd->priv->queue = g_slist_append (mpd->priv->queue, queue_action);
}


void
ario_mpd_queue_commit (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GSList *temp;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return;

        mpd_sendCommandListBegin(mpd->priv->connection);

        for (temp = mpd->priv->queue; temp; temp = g_slist_next (temp)) {
                ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) temp->data;
                if(queue_action->type == ARIO_MPD_ACTION_ADD) {
                        if(queue_action->path) {
                                mpd_sendAddCommand(mpd->priv->connection, queue_action->path);
                        }
                } else if (queue_action->type == ARIO_MPD_ACTION_DELETE_ID) {
                        if(queue_action->id >= 0) {
                                mpd_sendDeleteIdCommand(mpd->priv->connection, queue_action->id);
                        }
                } else if (queue_action->type == ARIO_MPD_ACTION_DELETE_POS) {                                                                                      
                        if(queue_action->id >= 0) {
                                mpd_sendDeleteCommand(mpd->priv->connection, queue_action->pos);
                        }
                } else if (queue_action->type == ARIO_MPD_ACTION_MOVE) {
                        if(queue_action->id >= 0) {
                                mpd_sendMoveCommand(mpd->priv->connection, queue_action->old_pos, queue_action->new_pos);
                        }
                }
        }
        mpd_sendCommandListEnd(mpd->priv->connection);
        mpd_finishCommand(mpd->priv->connection);
        ario_mpd_update_status (mpd);

        g_slist_foreach(mpd->priv->queue, (GFunc) g_free, NULL);
        g_slist_free (mpd->priv->queue);
        mpd->priv->queue = NULL;
}


GSList*
ario_mpd_search (ArioMpd *mpd,
                 GSList *search_criterias)
{
        ARIO_LOG_FUNCTION_START
        mpd_InfoEntity *ent = NULL;
        GSList *tmp;
        ArioMpdSearchCriteria *search_criteria;
        GSList *songs = NULL;

        if (!mpd->priv->connection)
                return NULL;

        mpd_startSearch(mpd->priv->connection, FALSE);

        for (tmp = search_criterias; tmp; tmp = g_slist_next (tmp)) {
                search_criteria = tmp->data;
                mpd_addConstraintSearch(mpd->priv->connection,
                                        search_criteria->type,
                                        search_criteria->value);
        }
        mpd_commitSearch(mpd->priv->connection);

        while ((ent = mpd_getNextInfoEntity (mpd->priv->connection))) {
                songs = g_slist_append (songs, ent->info.song);
                ent->info.song = NULL;
                mpd_freeInfoEntity(ent);
        }
        mpd_finishCommand (mpd->priv->connection);

        return songs;
}

int
ario_mpd_save_playlist (ArioMpd *mpd,
                        const char *name)
{
        ARIO_LOG_FUNCTION_START
        mpd_sendSaveCommand (mpd->priv->connection, name);
        mpd_finishCommand (mpd->priv->connection);

        if (mpd->priv->connection->error == MPD_ERROR_ACK && mpd->priv->connection->errorCode == MPD_ACK_ERROR_EXIST)
                return 1;

        g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[STOREDPLAYLISTS_CHANGED], 0);

        return 0;
}

void
ario_mpd_delete_playlist (ArioMpd *mpd,
                          const char *name)
{
        ARIO_LOG_FUNCTION_START
        mpd_sendRmCommand (mpd->priv->connection, name);
        mpd_finishCommand (mpd->priv->connection);

        g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[STOREDPLAYLISTS_CHANGED], 0);
}

void
ario_mpd_use_count_inc (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ++mpd->priv->use_count;
        if (mpd->priv->use_count == 1) {
                if (mpd->priv->timeout_id)
                        g_source_remove (mpd->priv->timeout_id);
                ario_mpd_update_status (mpd);
                ario_mpd_launch_timeout (mpd);
        }
}

void
ario_mpd_use_count_dec (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->use_count > 0)
                --mpd->priv->use_count;
        if (mpd->priv->use_count == 0) {
                if (mpd->priv->timeout_id)
                        g_source_remove (mpd->priv->timeout_id);
                ario_mpd_launch_timeout (mpd);
        }
}

void
ario_mpd_launch_timeout (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        mpd->priv->timeout_id = g_timeout_add ((mpd->priv->use_count) ? NORMAL_TIMEOUT : LAZY_TIMEOUT,
                                               (GSourceFunc) ario_mpd_update_status,
                                               mpd);
}

GSList *
ario_mpd_get_outputs (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GSList *outputs = NULL;
        mpd_OutputEntity *output_ent;

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        mpd_sendOutputsCommand (mpd->priv->connection);

        while ((output_ent = mpd_getNextOutput (mpd->priv->connection)))
                outputs = g_slist_append (outputs, output_ent);

        mpd_finishCommand (mpd->priv->connection);

        return outputs;
}

void
ario_mpd_enable_output (ArioMpd *mpd,
                        int id,
                        gboolean enabled)
{
        ARIO_LOG_FUNCTION_START

        if (enabled) {
                mpd_sendEnableOutputCommand(mpd->priv->connection, id);
        } else {
                mpd_sendDisableOutputCommand(mpd->priv->connection, id);
        }

        mpd_finishCommand (mpd->priv->connection);
}

ArioMpdStats *
ario_mpd_get_stats (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        if (mpd->priv->stats)
                mpd_freeStats (mpd->priv->stats);
        mpd_sendStatsCommand (mpd->priv->connection);
        mpd->priv->stats = mpd_getStats (mpd->priv->connection);

        ario_mpd_check_errors (mpd);

        return mpd->priv->stats;
}

GList *
ario_mpd_get_songs_info (ArioMpd *mpd,
                         GSList *paths)
{
        ARIO_LOG_FUNCTION_START
        const gchar *path = NULL;
        GSList *temp;
        GList *songs = NULL;
        mpd_InfoEntity *ent;
        
        /* check if there is a connection */
        if (!mpd->priv->connection)
                return NULL;

        for (temp = paths; temp; temp = g_slist_next (temp)) {
                path = temp->data;

                mpd_sendListallInfoCommand (mpd->priv->connection, path);

	        ent = mpd_getNextInfoEntity (mpd->priv->connection);

	        mpd_finishCommand (mpd->priv->connection);
                if (!ent)
                        continue;

	        songs = g_list_append (songs, ent->info.song);
	        ent->info.song = NULL;

	        mpd_freeInfoEntity (ent);
                ario_mpd_check_errors (mpd);
        }

        return songs;
}

ArioMpdFileList *
ario_mpd_list_files (ArioMpd *mpd,
                     const char *path,
                     gboolean recursive)
{
        ARIO_LOG_FUNCTION_START
        mpd_InfoEntity *entity;
        ArioMpdFileList *files = (ArioMpdFileList *) g_malloc0 (sizeof (ArioMpdFileList));

        /* check if there is a connection */
        if (!mpd->priv->connection)
                return files;

        if (recursive)
                mpd_sendListallCommand (mpd->priv->connection, path);
        else
                mpd_sendLsInfoCommand (mpd->priv->connection, path);

        while ((entity = mpd_getNextInfoEntity (mpd->priv->connection))) {
	        if (entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
                        files->directories = g_slist_append (files->directories, entity->info.directory->path);
                        entity->info.directory->path = NULL;
	        } else if (entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
                        files->songs = g_slist_append (files->songs, entity->info.song);
                        entity->info.song = NULL;
                }

                mpd_freeInfoEntity(entity);
        }

        return files;
}

void
ario_mpd_free_file_list (ArioMpdFileList *files)
{
        ARIO_LOG_FUNCTION_START
        if (files) {
                g_slist_foreach(files->directories, (GFunc) g_free, NULL);
                g_slist_free (files->directories);
                g_slist_foreach(files->songs, (GFunc) ario_mpd_free_song, NULL);
                g_slist_free (files->songs);
                g_free (files);
        }
}

