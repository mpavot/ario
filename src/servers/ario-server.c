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
#include "servers/ario-mpd.h"
#include "servers/ario-server.h"
#ifdef ENABLE_XMMS2
#include "servers/ario-xmms.h"
#endif
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define NORMAL_TIMEOUT 500
#define LAZY_TIMEOUT 12000

static void ario_server_finalize (GObject *object);
static void ario_server_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec);
static void ario_server_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec);

struct ArioServerPrivate
{
        /* TODO: Remove */
        gboolean dummy;
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

static guint ario_server_signals[SERVER_LAST_SIGNAL] = { 0 };

char * ArioServerItemNames[MPD_TAG_NUM_OF_ITEM_TYPES] =
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

#define ARIO_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SERVER, ArioServerPrivate))
G_DEFINE_TYPE (ArioServer, ario_server, G_TYPE_OBJECT)

static ArioServer *instance = NULL;
#ifdef ENABLE_XMMS2
static ArioServerType instance_type = ArioServerXmms;
#else
static ArioServerType instance_type = ArioServerMpd;
#endif
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
ario_server_class_init (ArioServerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_server_finalize;

        object_class->set_property = ario_server_set_property;
        object_class->get_property = ario_server_get_property;

        klass->connect = dummy_pointer_void;
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
        klass->use_count_inc = dummy_void_void;
        klass->use_count_dec = dummy_void_void;
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

        ario_server_signals[SERVER_SONG_CHANGED] =
                g_signal_new ("song_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, song_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_ALBUM_CHANGED] =
                g_signal_new ("album_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, album_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_STATE_CHANGED] =
                g_signal_new ("state_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, state_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_VOLUME_CHANGED] =
                g_signal_new ("volume_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, volume_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);

        ario_server_signals[SERVER_ELAPSED_CHANGED] =
                g_signal_new ("elapsed_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, elapsed_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);

        ario_server_signals[SERVER_PLAYLIST_CHANGED] =
                g_signal_new ("playlist_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_RANDOM_CHANGED] =
                g_signal_new ("random_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_REPEAT_CHANGED] =
                g_signal_new ("repeat_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_UPDATINGDB_CHANGED] =
                g_signal_new ("updatingdb_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, updatingdb_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_STOREDPLAYLISTS_CHANGED] =
                g_signal_new ("storedplaylists_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, storedplaylists_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        g_type_class_add_private (klass, sizeof (ArioServerPrivate));
}

static void
ario_server_init (ArioServer *server)
{
        ARIO_LOG_FUNCTION_START
        server->priv = ARIO_SERVER_GET_PRIVATE (server);

        server->song_id = -1;
        server->playlist_id = -1;
        server->volume = -1;
}

static void
ario_server_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioServer *server;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SERVER (object));

        server = ARIO_SERVER (object);
        g_return_if_fail (server->priv != NULL);

        if (server->server_song)
                ario_server_free_song (server->server_song);

        G_OBJECT_CLASS (ario_server_parent_class)->finalize (object);
}

static void
ario_server_set_property (GObject *object,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioServer *server = ARIO_SERVER (object);

        switch (prop_id) {
        case PROP_SONGID:
                server->song_id = g_value_get_int (value);
                server->signals_to_emit |= SERVER_SONG_CHANGED_FLAG;

                /* check if there is a connection */
                if (ario_server_is_connected ()) {
                        ArioServerSong *new_song;
                        ArioServerSong *old_song = server->server_song;
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
                                server->signals_to_emit |= SERVER_ALBUM_CHANGED_FLAG;

                        if (server->server_song)
                                ario_server_free_song (server->server_song);

                        if (new_song) {
                                server->server_song = new_song;
                        } else {
                                server->server_song = NULL;
                        }
                } else {
                        if (server->server_song) {
                                ario_server_free_song (server->server_song);
                                server->server_song = NULL;
                        }
                }
                break;
        case PROP_STATE:
                server->state = g_value_get_int (value);
                server->signals_to_emit |= SERVER_STATE_CHANGED_FLAG;
                break;
        case PROP_VOLUME:
                server->volume = g_value_get_int (value);
                server->signals_to_emit |= SERVER_VOLUME_CHANGED_FLAG;
                break;
        case PROP_ELAPSED:
                server->elapsed = g_value_get_int (value);
                server->signals_to_emit |= SERVER_ELAPSED_CHANGED_FLAG;
                break;
        case PROP_PLAYLISTID:
                server->playlist_id = g_value_get_int (value);
                if (!ario_server_is_connected ())
                        server->playlist_length = 0;
                server->signals_to_emit |= SERVER_PLAYLIST_CHANGED_FLAG;
                break;
        case PROP_RANDOM:
                server->random = g_value_get_boolean (value);
                server->signals_to_emit |= SERVER_RANDOM_CHANGED_FLAG;
                break;
        case PROP_REPEAT:
                server->repeat = g_value_get_boolean (value);
                server->signals_to_emit |= SERVER_REPEAT_CHANGED_FLAG;
                break;
        case PROP_UPDATINGDB:
                server->updatingdb = g_value_get_int (value);
                server->signals_to_emit |= SERVER_UPDATINGDB_CHANGED_FLAG;
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_server_get_property (GObject *object,
                          guint prop_id,
                          GValue *value,
                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioServer *server = ARIO_SERVER (object);

        switch (prop_id) {
        case PROP_SONGID:
                g_value_set_int (value, server->song_id);
                break;
        case PROP_STATE:
                g_value_set_int (value, server->state);
                break;
        case PROP_VOLUME:
                g_value_set_int (value, server->volume);
                break;
        case PROP_ELAPSED:
                g_value_set_int (value, server->elapsed);
                break;
        case PROP_PLAYLISTID:
                g_value_set_int (value, server->playlist_id);
                break;
        case PROP_RANDOM:
                g_value_set_boolean (value, server->random);
                break;
        case PROP_REPEAT:
                g_value_set_boolean (value, server->repeat);
                break;
        case PROP_UPDATINGDB:
                g_value_set_int (value, server->updatingdb);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_server_create_instance (void)
{
        ARIO_LOG_FUNCTION_START

        if (instance_type == ArioServerMpd) {
                instance = ARIO_SERVER (ario_mpd_get_instance ());
#ifdef ENABLE_XMMS2
        } else if (instance_type == ArioServerXmms) {
                instance = ARIO_SERVER (ario_xmms_get_instance ());
#endif
        } else {
                ARIO_LOG_ERROR ("Unknown server type: %d", instance_type);
        }
}

ArioServer *
ario_server_get_instance (void)
{
        ARIO_LOG_FUNCTION_START
        if (!instance) {
                ario_server_create_instance ();
        }
        return instance;
}

void
ario_server_set_instance_type (ArioServerType type)
{
        if (type != instance_type) {
                instance_type = type;
                ario_server_create_instance ();
        }
}

static gpointer
ario_server_connect_thread (ArioServer *server)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (server)->connect ();
}

gboolean
ario_server_connect (void)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *win, *vbox,*label, *bar;
        GThread* thread;
        GtkWidget *dialog;

        /* check if there is a connection */
        if (ario_server_is_connected () || instance->connecting)
                return FALSE;

        instance->connecting = TRUE;

        thread = g_thread_create ((GThreadFunc) ario_server_connect_thread,
                                  instance, TRUE, NULL);

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

        while (instance->connecting) {
                gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));
                while (gtk_events_pending ())
                        gtk_main_iteration ();
                g_usleep (200000);
        }

        g_thread_join (thread);

        if (!ario_server_is_connected ()) {
                dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, 
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 _("Impossible to connect to server. Check the connection options."));
                if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_NONE)
                        gtk_widget_destroy (dialog);
                g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_STATE_CHANGED], 0);
        }

        gtk_widget_hide (win);
        gtk_widget_destroy (win);

        return FALSE;
}

void
ario_server_disconnect (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->disconnect ();
}

void
ario_server_update_db (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->update_db ();
}

gboolean
ario_server_is_connected (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START

        return ARIO_SERVER_GET_CLASS (instance)->is_connected ();
}

GSList *
ario_server_list_tags (const ArioServerTag tag,
                       const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->list_tags (tag, criteria);
}

GSList *
ario_server_get_albums (const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_albums (criteria);
}

GSList *
ario_server_get_songs (const ArioServerCriteria *criteria,
                       const gboolean exact)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_songs (criteria, exact);
}

GSList *
ario_server_get_songs_from_playlist (char *playlist)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_songs_from_playlist (playlist);
}

GSList *
ario_server_get_playlists (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_playlists ();
}

GSList *
ario_server_get_playlist_changes (int playlist_id)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_playlist_changes (playlist_id);
}

void
ario_server_set_default (ArioServer *server)
{
        ARIO_LOG_FUNCTION_START
        if (server->song_id != 0)
                g_object_set (G_OBJECT (server), "song_id", 0, NULL);

        if (server->state != MPD_STATUS_STATE_UNKNOWN)
                g_object_set (G_OBJECT (server), "state", MPD_STATUS_STATE_UNKNOWN, NULL);

        if (server->volume != MPD_STATUS_NO_VOLUME)
                g_object_set (G_OBJECT (server), "volume", MPD_STATUS_NO_VOLUME, NULL);

        if (server->elapsed != 0)
                g_object_set (G_OBJECT (server), "elapsed", 0, NULL);

        if (server->playlist_id != -1)
                g_object_set (G_OBJECT (server), "playlist_id", -1, NULL);

        if (server->random != FALSE)
                g_object_set (G_OBJECT (server), "random", FALSE, NULL);

        if (server->repeat != FALSE)
                g_object_set (G_OBJECT (server), "repeat", FALSE, NULL);

        if (server->updatingdb != 0)
                g_object_set (G_OBJECT (server), "updatingdb", 0, NULL);
}

gboolean
ario_server_update_status (void)
{
        return ARIO_SERVER_GET_CLASS (instance)->update_status ();
}

char *
ario_server_get_current_title (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song->title;
        else
                return NULL;
}

char *
ario_server_get_current_name (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song->name;
        else
                return NULL;
}

ArioServerSong *
ario_server_get_current_song_on_server (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_current_song_on_server ();
}

ArioServerSong *
ario_server_get_current_song (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song;
        else
                return NULL;
}

char *
ario_server_get_current_artist (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song->artist;
        else
                return NULL;
}

char *
ario_server_get_current_album (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song->album;
        else
                return NULL;
}

char *
ario_server_get_current_song_path (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song->file;
        else
                return NULL;
}

int
ario_server_get_current_song_id (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->song_id;
}

int
ario_server_get_current_state (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->state;
}

int
ario_server_get_current_elapsed (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->elapsed;
}

int
ario_server_get_current_volume (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->volume;
}

int
ario_server_get_current_total_time (void)
{
        ARIO_LOG_FUNCTION_START
        if (instance->server_song)
                return instance->server_song->time;
        else
                return 0;
}

int
ario_server_get_current_playlist_id (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->playlist_id;
}

int
ario_server_get_current_playlist_length (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->playlist_length;
}

int
ario_server_get_current_playlist_total_time (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_current_playlist_total_time ();
}

int
ario_server_get_crossfadetime (void)
{
        ARIO_LOG_FUNCTION_START
        if (ario_server_is_connected ())
                return instance->crossfade;
        else
                return 0;
}

gboolean
ario_server_get_current_random (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->random;
}

gboolean
ario_server_get_current_repeat (void)
{
        ARIO_LOG_FUNCTION_START
        return instance->repeat;
}

gboolean
ario_server_get_updating (void)
{
        ARIO_LOG_FUNCTION_START
        return ario_server_is_connected () && instance->updatingdb;
}

unsigned long
ario_server_get_last_update (void)
{
        return ARIO_SERVER_GET_CLASS (instance)->get_last_update ();
}

gboolean
ario_server_is_paused (void)
{
        ARIO_LOG_FUNCTION_START
        return (instance->state == MPD_STATUS_STATE_PAUSE) || (instance->state == MPD_STATUS_STATE_STOP);
}

void
ario_server_do_next (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->do_next ();
}

void
ario_server_do_prev (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->do_prev ();
}

void
ario_server_do_play (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->do_play ();
}

void
ario_server_do_play_pos (gint id)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->do_play_pos (id);
}

void
ario_server_do_pause (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->do_pause ();
}

void
ario_server_do_stop (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->do_stop ();
}

void
ario_server_free_album (ArioServerAlbum *server_album)
{
        ARIO_LOG_FUNCTION_START
        if (server_album) {
                g_free (server_album->album);
                g_free (server_album->artist);
                g_free (server_album->path);
                g_free (server_album->date);
                g_free (server_album);
        }
}

ArioServerAlbum*
ario_server_copy_album (const ArioServerAlbum *server_album)
{
        ARIO_LOG_FUNCTION_START
        ArioServerAlbum *ret = NULL;

        if (server_album) {
                ret = (ArioServerAlbum *) g_malloc (sizeof (ArioServerAlbum));
                ret->album = g_strdup (server_album->album);
                ret->artist = g_strdup (server_album->artist);
                ret->path = g_strdup (server_album->path);
                ret->date = g_strdup (server_album->date);
        }

        return ret;
}

void
ario_server_set_current_elapsed (const gint elapsed)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->set_current_elapsed (elapsed);
}

void
ario_server_set_current_volume (const gint volume)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->set_current_volume (volume);
}

void
ario_server_set_current_random (const gboolean random)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->set_current_random (random);
}

void
ario_server_set_current_repeat (const gboolean repeat)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->set_current_repeat (repeat);
}

void
ario_server_set_crossfadetime (const int crossfadetime)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->set_crossfadetime (crossfadetime);
}

void
ario_server_clear (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->clear ();
}

void
ario_server_queue_add (const char *path)
{
        ARIO_LOG_FUNCTION_START

        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_ADD;
        queue_action->path = path;

        instance->queue = g_slist_append (instance->queue, queue_action);
}

void
ario_server_queue_delete_id (const int id)
{
        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_DELETE_ID;
        queue_action->id = id;

        instance->queue = g_slist_append (instance->queue, queue_action);
}

void
ario_server_queue_delete_pos (const int pos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_DELETE_POS;
        queue_action->pos = pos;

        instance->queue = g_slist_append (instance->queue, queue_action);
}

void
ario_server_queue_move (const int old_pos,
                        const int new_pos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_MOVE;
        queue_action->old_pos = old_pos;
        queue_action->new_pos = new_pos;

        instance->queue = g_slist_append (instance->queue, queue_action);
}


void
ario_server_queue_commit (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->queue_commit ();
}

void
ario_server_insert_at (const GSList *songs,
                       const gint pos)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->insert_at (songs, pos);
}

int
ario_server_save_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        int ret = ARIO_SERVER_GET_CLASS (instance)->save_playlist (name);

        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_STOREDPLAYLISTS_CHANGED], 0);

        return ret;
}

void
ario_server_delete_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->delete_playlist (name);

        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_STOREDPLAYLISTS_CHANGED], 0);
}

void
ario_server_use_count_inc (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->use_count_inc ();
}

void
ario_server_use_count_dec (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->use_count_dec ();
}

GSList *
ario_server_get_outputs (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_outputs ();
}

void
ario_server_enable_output (int id,
                           gboolean enabled)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_GET_CLASS (instance)->enable_output (id, enabled);
}

ArioServerStats *
ario_server_get_stats (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_stats ();
}

GList *
ario_server_get_songs_info (GSList *paths)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->get_songs_info (paths);
}

ArioServerFileList *
ario_server_list_files (const char *path,
                        gboolean recursive)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_GET_CLASS (instance)->list_files (path, recursive);
}

void
ario_server_free_file_list (ArioServerFileList *files)
{
        ARIO_LOG_FUNCTION_START
        if (files) {
                g_slist_foreach(files->directories, (GFunc) g_free, NULL);
                g_slist_free (files->directories);
                g_slist_foreach(files->songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (files->songs);
                g_free (files);
        }
}

void
ario_server_criteria_free (ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                g_free (atomic_criteria->value);
                g_free (atomic_criteria);
        }
        g_slist_free (criteria);
}

ArioServerCriteria *
ario_server_criteria_copy (const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        ArioServerCriteria *ret = NULL;
        const GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;
        ArioServerAtomicCriteria *new_atomic_criteria;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (criteria) {
                        new_atomic_criteria = (ArioServerAtomicCriteria *) g_malloc0 (sizeof (ArioServerAtomicCriteria));
                        new_atomic_criteria->tag = atomic_criteria->tag;
                        new_atomic_criteria->value = g_strdup (atomic_criteria->value);
                        ret = g_slist_append (ret, new_atomic_criteria);
                }
        }
        return ret;
}

gchar **
ario_server_get_items_names (void)
{
        return ArioServerItemNames;
}

