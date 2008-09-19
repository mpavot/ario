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

static void ario_mpd_finalize (GObject *object);
static void ario_mpd_set_default (ArioMpd *mpd);
void ario_mpd_check_errors (void);
static void ario_mpd_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
static void ario_mpd_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec);
void ario_mpd_launch_timeout (void);

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
                const char *path;             // For ARIO_MPD_ACTION_ADD
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
        gboolean support_empty_tags;
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

char * ArioMpdItemNames[MPD_TAG_NUM_OF_ITEM_TYPES] =
{
        N_("Artist"),    // MPD_TAG_ITEM_ARTIST
        N_("Album"),     // MPD_TAG_ITEM_ALBUM
        N_("Title"),     // MPD_TAG_ITEM_TITLE
        N_("Track"),     // MPD_TAG_ITEM_TRACK
        NULL,            // MPD_TAG_ITEM_NAME
        N_("Genre"),     // MPD_TAG_ITEM_GENRE
        N_("Date"),      // MPD_TAG_ITEM_DATE
        N_("Composer"),  // MPD_TAG_ITEM_COMPOSER
        N_("Performer"), // MPD_TAG_ITEM_PERFORMER
        NULL,            // MPD_TAG_ITEM_COMMENT
        NULL,            // MPD_TAG_ITEM_DISC
        N_("Filename"),  // MPD_TAG_ITEM_FILENAME
        N_("Any")        // MPD_TAG_ITEM_ANY
};

#define ARIO_MPD_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_MPD, ArioMpdPrivate))
G_DEFINE_TYPE (ArioMpd, ario_mpd, G_TYPE_OBJECT)

static ArioMpd *instance = NULL;

static void
ario_mpd_class_init (ArioMpdClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

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
        g_type_class_add_private (klass, sizeof (ArioMpdPrivate));
}

static void
ario_mpd_init (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        mpd->priv = ARIO_MPD_GET_PRIVATE (mpd);

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

        if (mpd->priv->connection)
                mpd_closeConnection (mpd->priv->connection);

        if (mpd->priv->ario_mpd_song)
                ario_mpd_free_song (mpd->priv->ario_mpd_song);

        if (mpd->priv->status)
                mpd_freeStatus (mpd->priv->status);

        if (mpd->priv->stats)
                mpd_freeStats (mpd->priv->stats);

        G_OBJECT_CLASS (ario_mpd_parent_class)->finalize (object);
}

static void
ario_mpd_set_property (GObject *object,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd = ARIO_MPD (object);
        int song_id;

        switch (prop_id) {
        case PROP_SONGID:
                song_id = g_value_get_int (value);

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

                if (mpd->priv->song_id != song_id)
                        mpd->priv->signals_to_emit |= SONG_CHANGED_FLAG;
                mpd->priv->song_id = song_id;
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
ario_mpd_get_instance (void)
{
        ARIO_LOG_FUNCTION_START
        if (!instance) {
                instance = g_object_new (TYPE_ARIO_MPD, NULL);
                g_return_val_if_fail (instance->priv != NULL, NULL);
        }
        return instance;
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

        if  (connection->error) {
                ARIO_LOG_ERROR("%s", connection->errorStr);
                mpd_clearError (connection);
                mpd_closeConnection (connection);
                return FALSE;
        }

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
                ario_mpd_disconnect ();
        }

        g_free (hostname);
        mpd->priv->support_empty_tags = FALSE;
        mpd->priv->connecting = FALSE;

        return NULL;
}

gboolean
ario_mpd_connect (void)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *win, *vbox,*label, *bar;
        GThread* thread;
        GtkWidget *dialog;

        /* check if there is a connection */
        if (instance->priv->connection || instance->priv->connecting)
                return FALSE;

        instance->priv->connecting = TRUE;

        thread = g_thread_create ((GThreadFunc) ario_mpd_connect_thread,
                                  instance, TRUE, NULL);

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

        while (instance->priv->connecting) {
                gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));
                while (gtk_events_pending ())
                        gtk_main_iteration ();
                g_usleep (200000);
        }

        g_thread_join (thread);

        if (!ario_mpd_is_connected ()) {
                dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, 
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 _("Impossible to connect to mpd. Check the connection options."));
                if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_NONE)
                        gtk_widget_destroy (dialog);
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[STATE_CHANGED], 0);
        }

        gtk_widget_hide (win);
        gtk_widget_destroy (win);

        return FALSE;
}

void
ario_mpd_disconnect (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_closeConnection (instance->priv->connection);
        instance->priv->connection = NULL;

        ario_mpd_update_status ();
}

void
ario_mpd_update_db (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendUpdateCommand (instance->priv->connection, "");
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_check_errors (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START
        if (!instance->priv->connection)
                return;

        if  (instance->priv->connection->error) {
                ARIO_LOG_ERROR("%s", instance->priv->connection->errorStr);
                mpd_clearError (instance->priv->connection);
                ario_mpd_disconnect ();
                /* Try to reconnect */
                ario_mpd_connect ();
        }
}

gboolean
ario_mpd_is_connected (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START
        return (instance->priv->connection != NULL);
}

GSList *
ario_mpd_list_tags (const ArioMpdTag tag,
                    const ArioMpdCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        gchar *value;
        const GSList *tmp;
        GSList *values = NULL;
        ArioMpdAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_startFieldSearch (instance->priv->connection, tag);
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (instance->priv->support_empty_tags
                    && !g_utf8_collate (atomic_criteria->value, ARIO_MPD_UNKNOWN))
                        mpd_addConstraintSearch (instance->priv->connection, atomic_criteria->tag, "");
                else
                        mpd_addConstraintSearch (instance->priv->connection, atomic_criteria->tag, atomic_criteria->value);
        }
        mpd_commitSearch (instance->priv->connection);

        while ((value = mpd_getNextTag (instance->priv->connection, tag))) {
                if (*value)
                        values = g_slist_append (values, value);
                else {
                        values = g_slist_append (values, g_strdup (ARIO_MPD_UNKNOWN));
                        instance->priv->support_empty_tags = TRUE;
                }
        }

        return values;
}

GSList *
ario_mpd_get_artists (void)
{
        ARIO_LOG_FUNCTION_START
        GSList *artists = NULL;
        gchar *artist_char;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendListCommand (instance->priv->connection, MPD_TABLE_ARTIST, NULL);

        while ((artist_char = mpd_getNextArtist (instance->priv->connection)))
                artists = g_slist_append (artists, artist_char);

        mpd_finishCommand (instance->priv->connection);

        return artists;
}

static gboolean
ario_mpd_album_is_present (const GSList *albums,
                           const char *album)
{
        const GSList *tmp;
        ArioMpdAlbum *mpd_album;

        for (tmp = albums; tmp; tmp = g_slist_next (tmp)) {
                mpd_album = tmp->data;
                if (!g_utf8_collate (album, mpd_album->album)) {
                        return TRUE;
                }
        }
        return FALSE;
}

GSList *
ario_mpd_get_albums (const ArioMpdCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL;
        const GSList *tmp;
        mpd_InfoEntity *entity = NULL;
        ArioMpdAlbum *mpd_album;
        ArioMpdAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        if (!criteria) {
                mpd_sendListallInfoCommand(instance->priv->connection, "/");
        } else {
                mpd_startSearch (instance->priv->connection, TRUE);
                for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                        atomic_criteria = tmp->data;

                        if (instance->priv->support_empty_tags
                            && !g_utf8_collate (atomic_criteria->value, ARIO_MPD_UNKNOWN))
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
                        if (ario_mpd_album_is_present (albums, ARIO_MPD_UNKNOWN)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                } else {
                        if (ario_mpd_album_is_present (albums, entity->info.song->album)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                }

                mpd_album = (ArioMpdAlbum *) g_malloc (sizeof (ArioMpdAlbum));
                if (entity->info.song->album) {
                        mpd_album->album = entity->info.song->album;
                        entity->info.song->album = NULL;
                } else {
                        mpd_album->album = g_strdup (ARIO_MPD_UNKNOWN);
                }

                if (entity->info.song->artist) {
                        mpd_album->artist = entity->info.song->artist;
                        entity->info.song->artist = NULL;
                } else {
                        mpd_album->artist = g_strdup (ARIO_MPD_UNKNOWN);
                }

                if (entity->info.song->file) {
                        mpd_album->path = g_path_get_dirname (entity->info.song->file);
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

        return albums;
}

GSList *
ario_mpd_get_songs (const ArioMpdCriteria *criteria,
                    const gboolean exact)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        mpd_InfoEntity *entity = NULL;
        const GSList *tmp;
        gboolean is_album_unknown = FALSE;
        ArioMpdAtomicCriteria *atomic_criteria;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (atomic_criteria->tag == MPD_TAG_ITEM_ALBUM
                    && !g_utf8_collate (atomic_criteria->value, ARIO_MPD_UNKNOWN))
                        is_album_unknown = TRUE;
        }

        mpd_startSearch (instance->priv->connection, exact);
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (instance->priv->support_empty_tags
                    && !g_utf8_collate (atomic_criteria->value, ARIO_MPD_UNKNOWN))
                        mpd_addConstraintSearch (instance->priv->connection,
                                                 atomic_criteria->tag,
                                                 "");
                else if (atomic_criteria->tag != MPD_TAG_ITEM_ALBUM
                    || g_utf8_collate (atomic_criteria->value, ARIO_MPD_UNKNOWN))
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

        return songs;
}

GSList *
ario_mpd_get_songs_from_playlist (char *playlist)
{
        ARIO_LOG_FUNCTION_START
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

        return songs;
}

GSList *
ario_mpd_get_playlists (void)
{
        ARIO_LOG_FUNCTION_START
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

        return playlists;
}

GSList *
ario_mpd_get_playlist_changes (int playlist_id)
{
        ARIO_LOG_FUNCTION_START
        GSList *songs = NULL;
        mpd_InfoEntity *entity;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendPlChangesCommand (instance->priv->connection, playlist_id);

        while ((entity = mpd_getNextInfoEntity (instance->priv->connection))) {
                if (entity->info.song) {
                        songs = g_slist_append (songs, entity->info.song);
                        entity->info.song = NULL;
                }
                mpd_freeInfoEntity (entity);
        }
        mpd_finishCommand (instance->priv->connection);

        return songs;
}

ArioMpdSong *
ario_mpd_get_playlist_info (int song_pos)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdSong *song = NULL;
        mpd_InfoEntity *entity;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendPlaylistInfoCommand (instance->priv->connection, song_pos);

        entity = mpd_getNextInfoEntity (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);

        if (entity) {
                song = entity->info.song;
                entity->info.song = NULL;
                mpd_freeInfoEntity (entity);
        }

        return song;
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
ario_mpd_update_status (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START

        if (instance->priv->is_updating)
                return TRUE;
        instance->priv->is_updating = TRUE;
        instance->priv->signals_to_emit = 0;

        /* check if there is a connection */
        if (!instance->priv->connection) {
                ario_mpd_set_default (instance);
        } else {
                if (instance->priv->status)
                        mpd_freeStatus (instance->priv->status);
                mpd_sendStatusCommand (instance->priv->connection);
                instance->priv->status = mpd_getStatus (instance->priv->connection);

                ario_mpd_check_errors ();

                if (instance->priv->status) {
                        if (instance->priv->song_id != instance->priv->status->songid)
                                g_object_set (G_OBJECT (instance), "song_id", instance->priv->status->songid, NULL);

                        if (instance->priv->state != instance->priv->status->state)
                                g_object_set (G_OBJECT (instance), "state", instance->priv->status->state, NULL);

                        if (instance->priv->volume != instance->priv->status->volume)
                                g_object_set (G_OBJECT (instance), "volume", instance->priv->status->volume, NULL);

                        if (instance->priv->elapsed != instance->priv->status->elapsedTime)
                                g_object_set (G_OBJECT (instance), "elapsed", instance->priv->status->elapsedTime, NULL);

                        if (instance->priv->playlist_id != (int) instance->priv->status->playlist) {
                                g_object_set (G_OBJECT (instance), "song_id", instance->priv->status->songid, NULL);
                                g_object_set (G_OBJECT (instance), "playlist_id", instance->priv->status->playlist, NULL);
                        }

                        if (instance->priv->random != (gboolean) instance->priv->status->random)
                                g_object_set (G_OBJECT (instance), "random", instance->priv->status->random, NULL);

                        if (instance->priv->repeat != (gboolean) instance->priv->status->repeat)
                                g_object_set (G_OBJECT (instance), "repeat", instance->priv->status->repeat, NULL);

                        if (instance->priv->updatingdb != instance->priv->status->updatingDb)
                                g_object_set (G_OBJECT (instance), "updatingdb", instance->priv->status->updatingDb, NULL);
                }
        }

        if (instance->priv->signals_to_emit & SONG_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[SONG_CHANGED], 0);
        if (instance->priv->signals_to_emit & ALBUM_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[ALBUM_CHANGED], 0);
        if (instance->priv->signals_to_emit & STATE_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[STATE_CHANGED], 0);
        if (instance->priv->signals_to_emit & VOLUME_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[VOLUME_CHANGED], 0);
        if (instance->priv->signals_to_emit & ELAPSED_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[ELAPSED_CHANGED], 0);
        if (instance->priv->signals_to_emit & PLAYLIST_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[PLAYLIST_CHANGED], 0);
        if (instance->priv->signals_to_emit & RANDOM_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[RANDOM_CHANGED], 0);
        if (instance->priv->signals_to_emit & REPEAT_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[REPEAT_CHANGED], 0);
        if (instance->priv->signals_to_emit & UPDATINGDB_CHANGED_FLAG)
                g_signal_emit (G_OBJECT (instance), ario_mpd_signals[UPDATINGDB_CHANGED], 0);

        instance->priv->is_updating = FALSE;
        return TRUE;
}

char *
ario_mpd_get_current_title (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->ario_mpd_song)
                return instance->priv->ario_mpd_song->title;
        else
                return NULL;
}

char *
ario_mpd_get_current_name (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->ario_mpd_song)
                return instance->priv->ario_mpd_song->name;
        else
                return NULL;
}

ArioMpdSong *
ario_mpd_get_current_song (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->ario_mpd_song)
                return instance->priv->ario_mpd_song;
        else
                return NULL;
}

char *
ario_mpd_get_current_artist (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->ario_mpd_song)
                return instance->priv->ario_mpd_song->artist;
        else
                return NULL;
}

char *
ario_mpd_get_current_album (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->ario_mpd_song)
                return instance->priv->ario_mpd_song->album;
        else
                return NULL;
}

char *
ario_mpd_get_current_song_path (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->ario_mpd_song)
                return instance->priv->ario_mpd_song->file;
        else
                return NULL;
}

int
ario_mpd_get_current_song_id (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->song_id;
}

int
ario_mpd_get_current_state (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->state;
}

int
ario_mpd_get_current_elapsed (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->elapsed;
}

int
ario_mpd_get_current_volume (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->volume;
}

int
ario_mpd_get_current_total_time (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->total_time;
}

int
ario_mpd_get_current_playlist_id (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->playlist_id;
}

int
ario_mpd_get_current_playlist_length (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->playlist_length;
}

int
ario_mpd_get_current_playlist_total_time (void)
{
        ARIO_LOG_FUNCTION_START
        int total_time = 0;
        ArioMpdSong *song;
        mpd_InfoEntity *ent = NULL;

        if (!instance->priv->connection)
                return 0;

        // We go to MPD server for each call to this function but it is not a problem as it is
        // called only once for each playlist change (by status bar). I may change this implementation
        // if this function is called more than once for performance reasons.
        mpd_sendPlaylistInfoCommand(instance->priv->connection, -1);
        while ((ent = mpd_getNextInfoEntity (instance->priv->connection))) {
                song = ent->info.song;
                if (song->time != MPD_SONG_NO_TIME)
                        total_time = total_time + song->time;
                mpd_freeInfoEntity(ent);
        }
        mpd_finishCommand (instance->priv->connection);

        return total_time;
}

int
ario_mpd_get_crossfadetime (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->status)
                return instance->priv->status->crossfade;
        else
                return 0;
}

gboolean
ario_mpd_get_current_random (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->random;
}

gboolean
ario_mpd_get_current_repeat (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->priv->repeat;
}

gboolean
ario_mpd_get_updating (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->status)
                return instance->priv->updatingdb;
        else
                return FALSE;
}

unsigned long
ario_mpd_get_last_update (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return 0;

        if (instance->priv->stats)
                mpd_freeStats (instance->priv->stats);
        mpd_sendStatsCommand (instance->priv->connection);
        instance->priv->stats = mpd_getStats (instance->priv->connection);

        ario_mpd_check_errors ();

        if (instance->priv->stats)
                return instance->priv->stats->dbUpdateTime;
        else
                return 0;
}

gboolean
ario_mpd_is_paused (void)
{
        ARIO_LOG_FUNCTION_START
        return (instance->priv->state == MPD_STATUS_STATE_PAUSE) || (instance->priv->state == MPD_STATUS_STATE_STOP);
}

void
ario_mpd_do_next (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendNextCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_do_prev (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendPrevCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_do_play (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendPlayCommand (instance->priv->connection, -1);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_do_play_id (gint id)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        /* send mpd the play command */
        mpd_sendPlayIdCommand (instance->priv->connection, id);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_do_pause (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendPauseCommand (instance->priv->connection, TRUE);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_do_stop (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendStopCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_free_album (ArioMpdAlbum *ario_mpd_album)
{
        ARIO_LOG_FUNCTION_START
        if (ario_mpd_album) {
                g_free (ario_mpd_album->album);
                g_free (ario_mpd_album->artist);
                g_free (ario_mpd_album->path);
                g_free (ario_mpd_album->date);
                g_free (ario_mpd_album);
        }
}

ArioMpdAlbum*
ario_mpd_copy_album (const ArioMpdAlbum *ario_mpd_album)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdAlbum *ret = NULL;

        if (ario_mpd_album) {
                ret = (ArioMpdAlbum *) g_malloc (sizeof (ArioMpdAlbum));
                ret->album = g_strdup (ario_mpd_album->album);
                ret->artist = g_strdup (ario_mpd_album->artist);
                ret->path = g_strdup (ario_mpd_album->path);
                ret->date = g_strdup (ario_mpd_album->date);
        }

        return ret;
}

void
ario_mpd_set_current_elapsed (const gint elapsed)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendSeekCommand (instance->priv->connection, instance->priv->status->song, elapsed);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_set_current_volume (const gint volume)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendSetvolCommand (instance->priv->connection, volume);
        mpd_finishCommand (instance->priv->connection);
        ario_mpd_update_status ();
}

void
ario_mpd_set_current_random (const gboolean random)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendRandomCommand (instance->priv->connection, random);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_set_current_repeat (const gboolean repeat)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendRepeatCommand (instance->priv->connection, repeat);
        mpd_finishCommand (instance->priv->connection);
}

void
ario_mpd_set_crossfadetime (const int crossfadetime)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendCrossfadeCommand (instance->priv->connection, crossfadetime);        
        mpd_finishCommand (instance->priv->connection);          
}

void
ario_mpd_clear (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendClearCommand (instance->priv->connection);
        mpd_finishCommand (instance->priv->connection);
        ario_mpd_update_status ();
}

void
ario_mpd_queue_add (const char *path)
{
        ARIO_LOG_FUNCTION_START

        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_ADD;
        queue_action->path = path;

        instance->priv->queue = g_slist_append (instance->priv->queue, queue_action);
}

void
ario_mpd_queue_delete_id (const int id)
{
        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_DELETE_ID;
        queue_action->id = id;

        instance->priv->queue = g_slist_append (instance->priv->queue, queue_action);
}

void
ario_mpd_queue_delete_pos (const int pos)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_DELETE_POS;
        queue_action->pos = pos;

        instance->priv->queue = g_slist_append (instance->priv->queue, queue_action);
}

void
ario_mpd_queue_move (const int old_pos,
                     const int new_pos)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) g_malloc (sizeof (ArioMpdQueueAction));
        queue_action->type = ARIO_MPD_ACTION_MOVE;
        queue_action->old_pos = old_pos;
        queue_action->new_pos = new_pos;

        instance->priv->queue = g_slist_append (instance->priv->queue, queue_action);
}


void
ario_mpd_queue_commit (void)
{
        ARIO_LOG_FUNCTION_START
        GSList *temp;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return;

        mpd_sendCommandListBegin(instance->priv->connection);

        for (temp = instance->priv->queue; temp; temp = g_slist_next (temp)) {
                ArioMpdQueueAction *queue_action = (ArioMpdQueueAction *) temp->data;
                if(queue_action->type == ARIO_MPD_ACTION_ADD) {
                        if(queue_action->path) {
                                mpd_sendAddCommand(instance->priv->connection, queue_action->path);
                        }
                } else if (queue_action->type == ARIO_MPD_ACTION_DELETE_ID) {
                        if(queue_action->id >= 0) {
                                mpd_sendDeleteIdCommand(instance->priv->connection, queue_action->id);
                        }
                } else if (queue_action->type == ARIO_MPD_ACTION_DELETE_POS) {                                                                                      
                        if(queue_action->id >= 0) {
                                mpd_sendDeleteCommand(instance->priv->connection, queue_action->pos);
                        }
                } else if (queue_action->type == ARIO_MPD_ACTION_MOVE) {
                        if(queue_action->id >= 0) {
                                mpd_sendMoveCommand(instance->priv->connection, queue_action->old_pos, queue_action->new_pos);
                        }
                }
        }
        mpd_sendCommandListEnd(instance->priv->connection);
        mpd_finishCommand(instance->priv->connection);
        ario_mpd_update_status ();

        g_slist_foreach(instance->priv->queue, (GFunc) g_free, NULL);
        g_slist_free (instance->priv->queue);
        instance->priv->queue = NULL;
}

int
ario_mpd_save_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        mpd_sendSaveCommand (instance->priv->connection, name);
        mpd_finishCommand (instance->priv->connection);

        if (instance->priv->connection->error == MPD_ERROR_ACK && instance->priv->connection->errorCode == MPD_ACK_ERROR_EXIST)
                return 1;

        g_signal_emit (G_OBJECT (instance), ario_mpd_signals[STOREDPLAYLISTS_CHANGED], 0);

        return 0;
}

void
ario_mpd_delete_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        mpd_sendRmCommand (instance->priv->connection, name);
        mpd_finishCommand (instance->priv->connection);

        g_signal_emit (G_OBJECT (instance), ario_mpd_signals[STOREDPLAYLISTS_CHANGED], 0);
}

void
ario_mpd_use_count_inc (void)
{
        ARIO_LOG_FUNCTION_START
        ++instance->priv->use_count;
        if (instance->priv->use_count == 1) {
                if (instance->priv->timeout_id)
                        g_source_remove (instance->priv->timeout_id);
                ario_mpd_update_status ();
                ario_mpd_launch_timeout ();
        }
}

void
ario_mpd_use_count_dec (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->priv->use_count > 0)
                --instance->priv->use_count;
        if (instance->priv->use_count == 0) {
                if (instance->priv->timeout_id)
                        g_source_remove (instance->priv->timeout_id);
                ario_mpd_launch_timeout ();
        }
}

void
ario_mpd_launch_timeout (void)
{
        ARIO_LOG_FUNCTION_START
        instance->priv->timeout_id = g_timeout_add ((instance->priv->use_count) ? NORMAL_TIMEOUT : LAZY_TIMEOUT,
                                               (GSourceFunc) ario_mpd_update_status,
                                               NULL);
}

GSList *
ario_mpd_get_outputs (void)
{
        ARIO_LOG_FUNCTION_START
        GSList *outputs = NULL;
        mpd_OutputEntity *output_ent;

        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        mpd_sendOutputsCommand (instance->priv->connection);

        while ((output_ent = mpd_getNextOutput (instance->priv->connection)))
                outputs = g_slist_append (outputs, output_ent);

        mpd_finishCommand (instance->priv->connection);

        return outputs;
}

void
ario_mpd_enable_output (int id,
                        gboolean enabled)
{
        ARIO_LOG_FUNCTION_START

        if (enabled) {
                mpd_sendEnableOutputCommand(instance->priv->connection, id);
        } else {
                mpd_sendDisableOutputCommand(instance->priv->connection, id);
        }

        mpd_finishCommand (instance->priv->connection);
}

ArioMpdStats *
ario_mpd_get_stats (void)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!instance->priv->connection)
                return NULL;

        if (instance->priv->stats)
                mpd_freeStats (instance->priv->stats);
        mpd_sendStatsCommand (instance->priv->connection);
        instance->priv->stats = mpd_getStats (instance->priv->connection);

        ario_mpd_check_errors ();

        return instance->priv->stats;
}

GList *
ario_mpd_get_songs_info (GSList *paths)
{
        ARIO_LOG_FUNCTION_START
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

        return songs;
}

ArioMpdFileList *
ario_mpd_list_files (const char *path,
                     gboolean recursive)
{
        ARIO_LOG_FUNCTION_START
        mpd_InfoEntity *entity;
        ArioMpdFileList *files = (ArioMpdFileList *) g_malloc0 (sizeof (ArioMpdFileList));

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

void
ario_mpd_criteria_free (ArioMpdCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioMpdAtomicCriteria *atomic_criteria;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                g_free (atomic_criteria->value);
                g_free (atomic_criteria);
        }
        g_slist_free (criteria);
}

ArioMpdCriteria *
ario_mpd_criteria_copy (const ArioMpdCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdCriteria *ret = NULL;
        const GSList *tmp;
        ArioMpdAtomicCriteria *atomic_criteria;
        ArioMpdAtomicCriteria *new_atomic_criteria;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (criteria) {
                        new_atomic_criteria = (ArioMpdAtomicCriteria *) g_malloc0 (sizeof (ArioMpdAtomicCriteria));
                        new_atomic_criteria->tag = atomic_criteria->tag;
                        new_atomic_criteria->value = g_strdup (atomic_criteria->value);
                        ret = g_slist_append (ret, new_atomic_criteria);
                }
        }
        return ret;
}

gchar **
ario_mpd_get_items_names (void)
{
        return ArioMpdItemNames;
}

