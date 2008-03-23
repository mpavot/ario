/*
 *  Based on Rhythmbox audioscrobbler plugin:
 *  Copyright (C) 2005 Alex Revo <xiphoidappendix@gmail.com>,
 *                       Ruben Vermeersch <ruben@Lambda1.be>
 *            (C) 2007 Christophe Fergeau <teuf@gnome.org>
 *
 *  Adapted to Ario:
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 */

#include "config.h"
#include <string.h>
#include <time.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libsoup/soup.h>
#include "ario-audioscrobbler.h"
#include "ario-debug.h"
#include "lib/ario-conf.h"
#include "ario-util.h"
#include "lib/rb-glade-helpers.h"
#include "preferences/ario-preferences.h"
#include "lib/md5.h"
#include "ario-plugin.h"

#define CLIENT_ID "tst" //"ari"
#define CLIENT_VERSION "1.0" //PACKAGE_VERSION
#define MAX_QUEUE_SIZE 1000
#define MAX_SUBMIT_SIZE        10
#define SCROBBLER_URL "http://post.audioscrobbler.com/"
#define SCROBBLER_VERSION "1.1"

#define USER_AGENT        "Ario/" PACKAGE_VERSION

#define SCROBBLER_DATE_FORMAT "%Y%%2D%m%%2D%d%%20%H%%3A%M%%3A%S"

#define EXTRA_URI_ENCODE_CHARS        "&+"

#define CONF_AUDIOSCROBBLER_USERNAME    "/apps/ario/audio-scrobbler-username"
#define CONF_AUDIOSCROBBLER_PASSWORD    "/apps/ario/audio-scrobbler-password"

/* compatibility junk for libsoup 2.2.
 * not intended to obviate the need for #ifdefs in code, but
 * should remove a lot of the trivial ones and make it easier
 * to drop libsoup 2.2
 */
#if defined(HAVE_LIBSOUP_2_2)

#include <libsoup/soup-uri.h>
#include <libsoup/soup-address.h>
#include <libsoup/soup-connection.h>
#include <libsoup/soup-headers.h>
#include <libsoup/soup-message.h>
#include <libsoup/soup-misc.h>
#include <libsoup/soup-session-sync.h>
#include <libsoup/soup-server.h>
#include <libsoup/soup-server-auth.h>
#include <libsoup/soup-server-message.h>


typedef SoupUri				SoupURI;
typedef SoupMessageCallbackFn		SoupSessionCallback;
typedef SoupServerContext		SoupClientContext;

#define SOUP_MEMORY_TAKE		SOUP_BUFFER_SYSTEM_OWNED
#define SOUP_MEMORY_TEMPORARY		SOUP_BUFFER_USER_OWNED

#define soup_message_headers_append	soup_message_add_header
#define soup_message_headers_get	soup_message_get_header

#define soup_client_context_get_host	soup_server_context_get_client_host

#endif	/* HAVE_LIBSOUP_2_2 */

typedef struct
{
        gchar *artist;
        gchar *album;
        gchar *title;
        guint length;
        time_t play_time;
} AudioscrobblerEntry;

typedef struct
{
        gchar *artist;
        gchar *album;
        gchar *title;
        guint length;
        gchar *timestamp;
} AudioscrobblerEncodedEntry;


struct _ArioAudioscrobblerPrivate
{
        ArioMpd *mpd;

        /* Widgets for the prefs pane */
        GtkWidget *preferences;
        GtkWidget *username_entry;
        GtkWidget *username_label;
        GtkWidget *password_entry;
        GtkWidget *password_label;
        GtkWidget *status_label;
        GtkWidget *submit_count_label;
        GtkWidget *submit_time_label;
        GtkWidget *queue_count_label;

        /* Data for the prefs pane */
        guint submit_count;
        char *submit_time;
        guint queue_count;
        enum {
                STATUS_OK = 0,
                HANDSHAKING,
                REQUEST_FAILED,
                BAD_USERNAME,
                BAD_PASSWORD,
                HANDSHAKE_FAILED,
                CLIENT_UPDATE_REQUIRED,
                SUBMIT_FAILED,
                QUEUE_TOO_LONG,
                GIVEN_UP,
        } status;
        char *status_msg;

        /* Submission queue */
        GQueue *queue;
        /* Entries currently being submitted */
        GQueue *submission;

        guint failures;
        /* Handshake has been done? */
        gboolean handshake;
        time_t handshake_next;
        time_t submit_next;
        time_t submit_interval;

        /* Only write the queue to a file if it has been changed */
        gboolean queue_changed;

        /* Authentication cookie + authentication info */
        gchar *md5_challenge;
        gchar *username;
        gchar *password;
        gchar *submit_url;

        /* Currently playing song info, if NULL this means the currently
         * playing song isn't eligible to be queued
         */
        AudioscrobblerEntry *currently_playing;
        guint current_elapsed;

        /* Preference notifications */
        guint notification_username_id;
        guint notification_password_id;

        guint timeout_id;

        /* HTTP requests session */
        SoupSession *soup_session;
};

#define ARIO_AUDIOSCROBBLER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ARIO_TYPE_AUDIOSCROBBLER, ArioAudioscrobblerPrivate))

static void audioscrobbler_entry_init (AudioscrobblerEntry *entry);
static void audioscrobbler_entry_free (AudioscrobblerEntry *entry);
static void audioscrobbler_encoded_entry_free (AudioscrobblerEncodedEntry *entry);
static AudioscrobblerEncodedEntry *audioscrobbler_entry_encode (AudioscrobblerEntry *entry);
static gboolean ario_audioscrobbler_load_queue (ArioAudioscrobbler *audioscrobbler);
static int ario_audioscrobbler_save_queue (ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_print_queue (ArioAudioscrobbler *audioscrobbler, gboolean submission);
static void ario_audioscrobbler_free_queue_entries (ArioAudioscrobbler *audioscrobbler, GQueue **queue);
static void ario_audioscrobbler_class_init (ArioAudioscrobblerClass *klass);
static void ario_audioscrobbler_init (ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_get_property (GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec);
static void ario_audioscrobbler_set_property (GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec);
static void ario_audioscrobbler_dispose (GObject *object);
static void ario_audioscrobbler_finalize (GObject *object);
static void ario_audioscrobbler_add_timeout (ArioAudioscrobbler *audioscrobbler);
static gboolean ario_audioscrobbler_timeout_cb (ArioAudioscrobbler *audioscrobbler);
static gchar *mkmd5 (char *string);
static void ario_audioscrobbler_parse_response (ArioAudioscrobbler *audioscrobbler, SoupMessage *msg);

static void ario_audioscrobbler_do_handshake (ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_submit_queue (ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_perform (ArioAudioscrobbler *audioscrobbler,
                                         char *url,
                                         char *post_data,
                                         SoupSessionCallback response_handler);
#if defined(HAVE_LIBSOUP_2_4)
static void ario_audioscrobbler_do_handshake_cb (SoupSession *session, SoupMessage *msg, gpointer user_data);
static void ario_audioscrobbler_submit_queue_cb (SoupSession *session, SoupMessage *msg, gpointer user_data);
#else
static void ario_audioscrobbler_do_handshake_cb (SoupMessage *msg, gpointer user_data);
static void ario_audioscrobbler_submit_queue_cb (SoupMessage *msg, gpointer user_data);
#endif
static void ario_audioscrobbler_import_settings (ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_preferences_sync (ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_conf_username_changed_cb (gpointer do_not_use1,
                                                          guint cnxn_id,
                                                          gpointer do_not_use2,
                                                          ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_conf_password_changed_cb (gpointer do_not_use1,
                                                          guint cnxn_id,
                                                          gpointer do_not_use2,
                                                          ArioAudioscrobbler *audioscrobbler);
static void ario_audioscrobbler_song_changed_cb (ArioMpd *mpd,
                                                 ArioAudioscrobbler *audioscrobbler);
G_MODULE_EXPORT void ario_audioscrobbler_username_entry_changed_cb (GtkEntry *entry,
                                                                    ArioAudioscrobbler *audioscrobbler);
G_MODULE_EXPORT void ario_audioscrobbler_username_entry_activate_cb (GtkEntry *entry,
                                                                     ArioAudioscrobbler *audioscrobbler);
G_MODULE_EXPORT void ario_audioscrobbler_password_entry_changed_cb (GtkEntry *entry,
                                                                    ArioAudioscrobbler *audioscrobbler);
G_MODULE_EXPORT void ario_audioscrobbler_password_entry_activate_cb (GtkEntry *entry,
                                                                     ArioAudioscrobbler *audioscrobbler);
G_MODULE_EXPORT void ario_audioscrobbler_enabled_check_changed_cb (GtkCheckButton *button,
                                                                   ArioAudioscrobbler *audioscrobbler);

enum
{
        PROP_0,
        PROP_MPD
};

G_DEFINE_TYPE (ArioAudioscrobbler, ario_audioscrobbler, G_TYPE_OBJECT)


static GObject *
ario_audioscrobbler_constructor (GType type,
                                 guint n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        GObject *obj;
        ArioAudioscrobbler *audioscrobbler;

        /* Invoke parent constructor. */
        ArioAudioscrobblerClass *klass;
        GObjectClass *parent_class;  
        klass = ARIO_AUDIOSCROBBLER_CLASS (g_type_class_peek (ARIO_TYPE_AUDIOSCROBBLER));
        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        obj = parent_class->constructor (type,
                                         n_construct_properties,
                                         construct_properties);

        audioscrobbler = ARIO_AUDIOSCROBBLER (obj);

        return obj;
}

/* Class-related functions: */
static void
ario_audioscrobbler_class_init (ArioAudioscrobblerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);


        object_class->constructor = ario_audioscrobbler_constructor;
        object_class->dispose = ario_audioscrobbler_dispose;
        object_class->finalize = ario_audioscrobbler_finalize;

        object_class->set_property = ario_audioscrobbler_set_property;
        object_class->get_property = ario_audioscrobbler_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "ArioMpd",
                                                              "ArioMpd object",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (ArioAudioscrobblerPrivate));
}

static void
ario_audioscrobbler_init (ArioAudioscrobbler *audioscrobbler)
{
        ARIO_LOG_DBG ("Initialising Audioscrobbler");
        ARIO_LOG_DBG ("Plugin ID: %s, Version %s (Protocol %s)",
                      CLIENT_ID, CLIENT_VERSION, SCROBBLER_VERSION);

        audioscrobbler->priv = ARIO_AUDIOSCROBBLER_GET_PRIVATE (audioscrobbler);

        audioscrobbler->priv->queue = g_queue_new();
        audioscrobbler->priv->submission = g_queue_new();
        audioscrobbler->priv->md5_challenge = g_strdup ("");
        audioscrobbler->priv->username = NULL;
        audioscrobbler->priv->password = NULL;
        audioscrobbler->priv->submit_url = g_strdup ("");

        ario_audioscrobbler_load_queue (audioscrobbler);

        ario_audioscrobbler_import_settings (audioscrobbler);

        /* conf notifications: */
        audioscrobbler->priv->notification_username_id =
                ario_conf_notification_add (CONF_AUDIOSCROBBLER_USERNAME,
                                            (ArioNotifyFunc) ario_audioscrobbler_conf_username_changed_cb,
                                            audioscrobbler);
        audioscrobbler->priv->notification_password_id =
                ario_conf_notification_add (CONF_AUDIOSCROBBLER_PASSWORD,
                                            (ArioNotifyFunc) ario_audioscrobbler_conf_password_changed_cb,
                                            audioscrobbler);

        ario_audioscrobbler_preferences_sync (audioscrobbler);
}

static void
ario_audioscrobbler_dispose (GObject *object)
{
        ArioAudioscrobbler *audioscrobbler;

        g_return_if_fail (object != NULL);
        g_return_if_fail (ARIO_IS_AUDIOSCROBBLER (object));

        audioscrobbler = ARIO_AUDIOSCROBBLER (object);

        ARIO_LOG_DBG ("disposing audioscrobbler");

        if (audioscrobbler->priv->notification_username_id != 0) {
                ario_conf_notification_remove (audioscrobbler->priv->notification_username_id);
                audioscrobbler->priv->notification_username_id = 0;
        }
        if (audioscrobbler->priv->notification_password_id != 0) {
                ario_conf_notification_remove (audioscrobbler->priv->notification_password_id);
                audioscrobbler->priv->notification_password_id = 0;
        }

        if (audioscrobbler->priv->timeout_id != 0) {
                g_source_remove (audioscrobbler->priv->timeout_id);
                audioscrobbler->priv->timeout_id = 0;
        }

        if (audioscrobbler->priv->soup_session != NULL) {
                soup_session_abort (audioscrobbler->priv->soup_session);
                g_object_unref (audioscrobbler->priv->soup_session);
                audioscrobbler->priv->soup_session = NULL;
        }

        audioscrobbler->priv->mpd = NULL;
        G_OBJECT_CLASS (ario_audioscrobbler_parent_class)->dispose (object);
}

static void
ario_audioscrobbler_finalize (GObject *object)
{
        ArioAudioscrobbler *audioscrobbler;

        ARIO_LOG_DBG ("Finalizing Audioscrobbler");

        g_return_if_fail (object != NULL);
        g_return_if_fail (ARIO_IS_AUDIOSCROBBLER (object));

        audioscrobbler = ARIO_AUDIOSCROBBLER (object);

        /* Save any remaining entries */
        ario_audioscrobbler_save_queue (audioscrobbler);

        g_free (audioscrobbler->priv->md5_challenge);
        g_free (audioscrobbler->priv->username);
        g_free (audioscrobbler->priv->password);
        g_free (audioscrobbler->priv->submit_url);
        if (audioscrobbler->priv->currently_playing != NULL) {
                audioscrobbler_entry_free (audioscrobbler->priv->currently_playing);
                audioscrobbler->priv->currently_playing = NULL;
        }

        if (audioscrobbler->priv->preferences)
                gtk_widget_destroy (audioscrobbler->priv->preferences);

        ario_audioscrobbler_free_queue_entries (audioscrobbler, &audioscrobbler->priv->queue);
        ario_audioscrobbler_free_queue_entries (audioscrobbler, &audioscrobbler->priv->submission);

        G_OBJECT_CLASS (ario_audioscrobbler_parent_class)->finalize (object);
}

ArioAudioscrobbler*
ario_audioscrobbler_new (ArioMpd *mpd)
{
        return g_object_new (ARIO_TYPE_AUDIOSCROBBLER,
                             "mpd", mpd,
                             NULL);
}

static void
ario_audioscrobbler_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
        ArioAudioscrobbler *audioscrobbler = ARIO_AUDIOSCROBBLER (object);

        switch (prop_id) {
        case PROP_MPD:
                audioscrobbler->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (audioscrobbler->priv->mpd),
                                         "song-changed",
                                         G_CALLBACK (ario_audioscrobbler_song_changed_cb),
                                         audioscrobbler, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_audioscrobbler_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
        ArioAudioscrobbler *audioscrobbler = ARIO_AUDIOSCROBBLER (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, audioscrobbler->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

#if defined(HAVE_LIBSOUP_2_4)
SoupURI *
ario_audioscrobbler_get_libsoup_uri (RBProxyConfig *config)
{
        SoupURI *uri = NULL;

        if (!config->enabled)
                return NULL;

        uri = soup_uri_new (NULL);
        soup_uri_set_scheme (uri, SOUP_URI_SCHEME_HTTP);
        soup_uri_set_host (uri, config->host);
        soup_uri_set_port (uri, config->port);

        if (config->auth_enabled) {
                soup_uri_set_user (uri, config->username);
                soup_uri_set_password (uri, config->password);
        }

        return uri;
}
#elif defined(HAVE_LIBSOUP_2_2)
static SoupUri *
ario_audioscrobbler_get_libsoup_uri ()
{
        SoupUri *uri = NULL;

        if (!ario_conf_get_boolean (CONF_USE_PROXY, FALSE))
                return NULL;

        uri = g_new0 (SoupUri, 1);
        uri->protocol = SOUP_PROTOCOL_HTTP;

        uri->host = ario_conf_get_string (CONF_PROXY_ADDRESS, "192.168.0.1");
        uri->port = ario_conf_get_integer (CONF_PROXY_PORT, 8080);

        return uri;
}
#endif

/* Add the audioscrobbler thread timer */
static void
ario_audioscrobbler_add_timeout (ArioAudioscrobbler *audioscrobbler)
{
        if (!audioscrobbler->priv->timeout_id) {
                ARIO_LOG_DBG ("Adding Audioscrobbler timer (15 seconds)");
                audioscrobbler->priv->timeout_id = 
                        g_timeout_add (15000, (GSourceFunc) ario_audioscrobbler_timeout_cb,
                                       audioscrobbler);
        }
}

static gboolean
ario_audioscrobbler_is_queueable (ArioMpdSong *song)
{
        const char *title;
        const char *artist;
        gulong duration;

        /* TODO: check if the entry is appropriate for sending to 
         * audioscrobbler
         */

        title = song->title;
        artist = song->artist;
        duration = song->time;

        /* The specification (v1.1) tells us to ignore entries that do not
         * meet these conditions
         */
        if (duration < 30) {
                ARIO_LOG_DBG ("entry not queueable: shorter than 30 seconds");
                return FALSE;
        }
        if (!artist) {
                ARIO_LOG_DBG ("entry not queueable: artist is unknown");
                return FALSE;
        }
        if (!title) {
                ARIO_LOG_DBG ("entry not queueable: title is unknown");
                return FALSE;
        }

        ARIO_LOG_DBG ("entry is queueable");
        return TRUE;
}

static AudioscrobblerEntry *
ario_audioscrobbler_create_entry (ArioMpdSong *song)
{
        AudioscrobblerEntry *as_entry = g_new0 (AudioscrobblerEntry, 1);

        if (song->title) {
                as_entry->title = g_strdup (song->title);
        } else {
                as_entry->title = g_strdup ("");
        }
        if (song->artist) {
                as_entry->artist = g_strdup (song->artist);
        } else {
                as_entry->artist = g_strdup ("");
        }
        if (song->album) {
                as_entry->album = g_strdup (song->album);
        } else {
                as_entry->album = g_strdup ("");
        }
        as_entry->length = song->time;

        return as_entry;
}

static gboolean
ario_audioscrobbler_add_to_queue (ArioAudioscrobbler *audioscrobbler,
                                  AudioscrobblerEntry *entry)
{        
        if (g_queue_get_length (audioscrobbler->priv->queue) < MAX_QUEUE_SIZE){
                g_queue_push_tail (audioscrobbler->priv->queue, entry);
                audioscrobbler->priv->queue_changed = TRUE;
                audioscrobbler->priv->queue_count++;
                return TRUE;
        } else {
                ARIO_LOG_DBG ("Queue is too long.  Not adding song to queue");
                g_free (audioscrobbler->priv->status_msg);
                audioscrobbler->priv->status = QUEUE_TOO_LONG;
                audioscrobbler->priv->status_msg = NULL;
                return FALSE;
        }
}

static void
maybe_add_current_song_to_queue (ArioAudioscrobbler *audioscrobbler)
{
        guint elapsed;
        AudioscrobblerEntry *cur_entry;

        cur_entry = audioscrobbler->priv->currently_playing;
        if (cur_entry == NULL) {
                return;
        }

        elapsed = ario_mpd_get_current_elapsed (audioscrobbler->priv->mpd);

        int elapsed_delta = elapsed - audioscrobbler->priv->current_elapsed;
        audioscrobbler->priv->current_elapsed = elapsed;
        ARIO_LOG_DBG ("elapsed=%d, cur_entry->length=%d, elapsed_delta=%d", elapsed, cur_entry->length, elapsed_delta);
        if ((elapsed >= cur_entry->length / 2 || elapsed >= 240) && elapsed_delta < 20) {
                ARIO_LOG_DBG ("Adding currently playing song to queue");
                time (&cur_entry->play_time);
                if (ario_audioscrobbler_add_to_queue (audioscrobbler, cur_entry)){
                        audioscrobbler->priv->currently_playing = NULL;
                }

                ario_audioscrobbler_preferences_sync (audioscrobbler);
        } else if (elapsed_delta > 20) {
                ARIO_LOG_DBG ("Skipping detected; not submitting current song");
                /* not sure about this - what if I skip to somewhere towards
                 * the end, but then go back and listen to the whole song?
                 */
                audioscrobbler_entry_free (audioscrobbler->priv->currently_playing);
                audioscrobbler->priv->currently_playing = NULL;

        }
}

/* updates the queue and submits entries as required */
static gboolean
ario_audioscrobbler_timeout_cb (ArioAudioscrobbler *audioscrobbler)
{
        maybe_add_current_song_to_queue (audioscrobbler);

        /* do handshake if we need to */
        ario_audioscrobbler_do_handshake (audioscrobbler);

        /* if there's something in the queue, submit it if we can, save it otherwise */
        if (!g_queue_is_empty(audioscrobbler->priv->queue)) {
                if (audioscrobbler->priv->handshake)
                        ario_audioscrobbler_submit_queue (audioscrobbler);
                else
                        ario_audioscrobbler_save_queue (audioscrobbler);
        } else {
                ARIO_LOG_DBG ("the queue is empty"); 
        }
        return TRUE;
}

/* Audioscrobbler functions: */
static gchar *
mkmd5 (char *string)
{
        md5_state_t md5state;
        guchar md5pword[16];
        gchar md5_response[33];

        int j = 0;

        memset (md5_response, 0, sizeof (md5_response));

        md5_init (&md5state);
        md5_append (&md5state, (unsigned char*)string, strlen (string));
        md5_finish (&md5state, md5pword);

        for (j = 0; j < 16; j++) {
                char a[3];
                sprintf (a, "%02x", md5pword[j]);
                md5_response[2*j] = a[0];
                md5_response[2*j+1] = a[1];
        }

        return (g_strdup (md5_response));
}

static void
ario_audioscrobbler_parse_response (ArioAudioscrobbler *audioscrobbler, SoupMessage *msg)
{
        gboolean successful;
        ARIO_LOG_DBG ("Parsing response, status=%d", msg->status_code);

        successful = FALSE;
#if defined(HAVE_LIBSOUP_2_4)
        if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code) && msg->response_body->length != 0)
                successful = TRUE;
#else
        if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code) && (msg->response).body != NULL)
                successful = TRUE;
#endif
        if (successful) {
                gchar **breaks;
                int i;
#if defined(HAVE_LIBSOUP_2_2)
                gchar *body;

                body = g_malloc0 ((msg->response).length + 1);
                memcpy (body, (msg->response).body, (msg->response).length);

                g_strstrip (body);
                breaks = g_strsplit (body, "\n", 4);
#else
                breaks = g_strsplit (msg->response_body->data, "\n", 4);
#endif

                g_free (audioscrobbler->priv->status_msg);
                audioscrobbler->priv->status = STATUS_OK;
                audioscrobbler->priv->status_msg = NULL;
                for (i = 0; breaks[i] != NULL; i++) {
                        ARIO_LOG_DBG ("RESPONSE: %s", breaks[i]);
                        if (g_str_has_prefix (breaks[i], "UPTODATE")) {
                                ARIO_LOG_DBG ("UPTODATE");

                                if (breaks[i+1] != NULL) {
                                        g_free (audioscrobbler->priv->md5_challenge);
                                        audioscrobbler->priv->md5_challenge = g_strdup (breaks[i+1]);
                                        ARIO_LOG_DBG ("MD5 challenge: \"%s\"", audioscrobbler->priv->md5_challenge);

                                        if (breaks[i+2] != NULL) {
                                                g_free (audioscrobbler->priv->submit_url);
                                                audioscrobbler->priv->submit_url = g_strdup (breaks[i+2]);
                                                ARIO_LOG_DBG ("Submit URL: \"%s\"", audioscrobbler->priv->submit_url);
                                                i++;
                                        }
                                        i++;
                                }

                        } else if (g_str_has_prefix (breaks[i], "UPDATE")) {
                                ARIO_LOG_DBG ("UPDATE");
                                audioscrobbler->priv->status = CLIENT_UPDATE_REQUIRED;

                                if (breaks[i+1] != NULL) {
                                        g_free (audioscrobbler->priv->md5_challenge);
                                        audioscrobbler->priv->md5_challenge = g_strdup (breaks[i+1]);
                                        ARIO_LOG_DBG ("MD5 challenge: \"%s\"", audioscrobbler->priv->md5_challenge);

                                        if (breaks[i+2] != NULL) {
                                                g_free (audioscrobbler->priv->submit_url);
                                                audioscrobbler->priv->submit_url = g_strdup (breaks[i+2]);
                                                ARIO_LOG_DBG ("Submit URL: \"%s\"", audioscrobbler->priv->submit_url);
                                                i++;
                                        }
                                        i++;
                                }

                        } else if (g_str_has_prefix (breaks[i], "FAILED")) {
                                audioscrobbler->priv->status = HANDSHAKE_FAILED;

                                if (strlen (breaks[i]) > 7) {
                                        ARIO_LOG_DBG ("FAILED: \"%s\"", breaks[i] + 7);
                                        audioscrobbler->priv->status_msg = g_strdup (breaks[i] + 7);
                                } else {
                                        ARIO_LOG_DBG ("FAILED");
                                }


                        } else if (g_str_has_prefix (breaks[i], "BADUSER")) {
                                ARIO_LOG_DBG ("BADUSER");
                                audioscrobbler->priv->status = BAD_USERNAME;
                        } else if (g_str_has_prefix (breaks[i], "BADAUTH")) {
                                ARIO_LOG_DBG ("BADAUTH");
                                audioscrobbler->priv->status = BAD_PASSWORD;
                        } else if (g_str_has_prefix (breaks[i], "OK")) {
                                ARIO_LOG_DBG ("OK");
                        } else if (g_str_has_prefix (breaks[i], "INTERVAL ")) {
                                audioscrobbler->priv->submit_interval = g_ascii_strtod(breaks[i] + 9, NULL);
                                ARIO_LOG_DBG ("INTERVAL: %s", breaks[i] + 9);
                        }
                }

                /* respect the last submit interval we were given */
                if (audioscrobbler->priv->submit_interval > 0)
                        audioscrobbler->priv->submit_next = time(NULL) + audioscrobbler->priv->submit_interval;

                g_strfreev (breaks);
#if defined(HAVE_LIBSOUP_2_2)
                g_free (body);
#endif
        } else {
                audioscrobbler->priv->status = REQUEST_FAILED;
                audioscrobbler->priv->status_msg = g_strdup (msg->reason_phrase);
        }
}

static gboolean
idle_unref_cb (GObject *object)
{
        g_object_unref (object);
        return FALSE;
}

/*
 * NOTE: the caller *must* unref the audioscrobbler object in an idle
 * handler created in the callback.
 */
static void
ario_audioscrobbler_perform (ArioAudioscrobbler *audioscrobbler,
                             char *url,
                             char *post_data,
                             SoupSessionCallback response_handler)
{
        SoupMessage *msg;

        msg = soup_message_new (post_data == NULL ? "GET" : "POST", url);
        soup_message_headers_append (msg->request_headers, "User-Agent", USER_AGENT);

        if (post_data != NULL) {
                ARIO_LOG_DBG ("Submitting to Audioscrobbler: %s", post_data);
                soup_message_set_request (msg,
                                          "application/x-www-form-urlencoded",
                                          SOUP_MEMORY_TAKE,
                                          post_data,
                                          strlen (post_data));
        }

        /* create soup session, if we haven't got one yet */
        if (!audioscrobbler->priv->soup_session) {
                SoupURI *uri;

                uri = ario_audioscrobbler_get_libsoup_uri ();
                audioscrobbler->priv->soup_session = soup_session_async_new_with_options (
                                                                                          "proxy-uri", uri,
                                                                                          NULL);
                if (uri)
                        soup_uri_free (uri);
        }

        soup_session_queue_message (audioscrobbler->priv->soup_session,
                                    msg,
                                    response_handler,
                                    g_object_ref (audioscrobbler));
}

static gboolean
ario_audioscrobbler_should_handshake (ArioAudioscrobbler *audioscrobbler)
{
        /* Perform handshake if necessary. Only perform handshake if
         *   - we have no current handshake; AND
         *   - we have waited the appropriate amount of time between
         *     handshakes; AND
         *   - we have a non-empty username
         */
        if (audioscrobbler->priv->handshake) {
                return FALSE;
        }

        if (time (NULL) < audioscrobbler->priv->handshake_next) {
                ARIO_LOG_DBG ("Too soon; time=%lu, handshake_next=%lu",
                              time (NULL),
                              audioscrobbler->priv->handshake_next);
                return FALSE;
        }

        if ((audioscrobbler->priv->username == NULL) ||
            (strcmp (audioscrobbler->priv->username, "") == 0)) {
                ARIO_LOG_DBG ("No username set");
                return FALSE;
        }

        return TRUE;
}

static void
ario_audioscrobbler_do_handshake (ArioAudioscrobbler *audioscrobbler)
{
        gchar *username;
        gchar *url;

        if (!ario_audioscrobbler_should_handshake (audioscrobbler)) {
                return;
        }

        username = soup_uri_encode (audioscrobbler->priv->username, EXTRA_URI_ENCODE_CHARS);
        url = g_strdup_printf ("%s?hs=true&p=%s&c=%s&v=%s&u=%s",
                               SCROBBLER_URL,
                               SCROBBLER_VERSION,
                               CLIENT_ID,
                               CLIENT_VERSION,
                               username);
        g_free (username);

        /* Make sure we wait at least 30 minutes between handshakes. */
        audioscrobbler->priv->handshake_next = time (NULL) + 1800;

        ARIO_LOG_DBG ("Performing handshake with Audioscrobbler server: %s", url);

        audioscrobbler->priv->status = HANDSHAKING;
        ario_audioscrobbler_preferences_sync (audioscrobbler);

        ario_audioscrobbler_perform (audioscrobbler,
                                     url,
                                     NULL,
                                     ario_audioscrobbler_do_handshake_cb);

        g_free (url);
}


#if defined(HAVE_LIBSOUP_2_4)
static void
ario_audioscrobbler_do_handshake_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
#else
static void
ario_audioscrobbler_do_handshake_cb (SoupMessage *msg, gpointer user_data)
#endif
{
        ArioAudioscrobbler *audioscrobbler = ARIO_AUDIOSCROBBLER(user_data);

        ARIO_LOG_DBG ("Handshake response");
        ario_audioscrobbler_parse_response (audioscrobbler, msg);
        ario_audioscrobbler_preferences_sync (audioscrobbler);

        switch (audioscrobbler->priv->status) {
        case STATUS_OK:
        case CLIENT_UPDATE_REQUIRED:
                audioscrobbler->priv->handshake = TRUE;
                audioscrobbler->priv->failures = 0;
                break;
        default:
                ARIO_LOG_DBG ("Handshake failed");
                ++audioscrobbler->priv->failures;
                break;
        }

        g_idle_add ((GSourceFunc) idle_unref_cb, audioscrobbler);
}


static gchar *
ario_audioscrobbler_build_authentication_data (ArioAudioscrobbler *audioscrobbler)
{
        gchar *md5_password;
        gchar *md5_temp;
        gchar *md5_response;
        gchar *username;
        gchar *post_data;
        time_t now;

        /* Conditions:
         *   - Must have username and password
         *   - Must have md5_challenge
         *   - Queue must not be empty
         */
        if ((audioscrobbler->priv->username == NULL) 
            || (*audioscrobbler->priv->username == '\0')) {
                ARIO_LOG_DBG ("No username set");
                return NULL;
        }

        if ((audioscrobbler->priv->password == NULL) 
            || (*audioscrobbler->priv->password == '\0')) {
                ARIO_LOG_DBG ("No password set");
                return NULL;
        }

        if (*audioscrobbler->priv->md5_challenge == '\0') {
                ARIO_LOG_DBG ("No md5 challenge");
                return NULL;
        }

        time(&now);
        if (now < audioscrobbler->priv->submit_next) {
                ARIO_LOG_DBG ("Too soon (next submission in %ld seconds)",
                              audioscrobbler->priv->submit_next - now);
                return NULL;
        }

        if (g_queue_is_empty (audioscrobbler->priv->queue)) {
                ARIO_LOG_DBG ("No queued songs to submit");
                return NULL;
        }

        md5_password = mkmd5 (audioscrobbler->priv->password);
        md5_temp = g_strconcat (md5_password,
                                audioscrobbler->priv->md5_challenge,
                                NULL);
        md5_response = mkmd5 (md5_temp);

        username = soup_uri_encode (audioscrobbler->priv->username, 
                                    EXTRA_URI_ENCODE_CHARS);
        post_data = g_strdup_printf ("u=%s&s=%s&", username, md5_response);

        g_free (md5_password);
        g_free (md5_temp);
        g_free (md5_response);
        g_free (username);

        return post_data;
}

static gchar *
ario_audioscrobbler_build_post_data (ArioAudioscrobbler *audioscrobbler,
                                     const gchar *authentication_data)
{
        g_return_val_if_fail (!g_queue_is_empty (audioscrobbler->priv->queue),
                              NULL);

        gchar *post_data = g_strdup (authentication_data);
        int i = 0;
        do {
                AudioscrobblerEntry *entry;
                AudioscrobblerEncodedEntry *encoded;
                gchar *new;
                /* remove first queue entry */
                entry = g_queue_pop_head (audioscrobbler->priv->queue);
                encoded = audioscrobbler_entry_encode (entry);
                new = g_strdup_printf ("%sa[%d]=%s&t[%d]=%s&b[%d]=%s&m[%d]=&l[%d]=%d&i[%d]=%s&",
                                       post_data,
                                       i, encoded->artist,
                                       i, encoded->title,
                                       i, encoded->album,
                                       i,
                                       i, encoded->length,
                                       i, encoded->timestamp);
                audioscrobbler_encoded_entry_free (encoded);
                g_free (post_data);
                post_data = new;

                /* add to submission list */
                g_queue_push_tail (audioscrobbler->priv->submission, 
                                   entry);
                i++;
        } while ((!g_queue_is_empty(audioscrobbler->priv->queue)) && (i < MAX_SUBMIT_SIZE));

        return post_data;
}

static void
ario_audioscrobbler_submit_queue (ArioAudioscrobbler *audioscrobbler)
{
        gchar *auth_data;

        auth_data = ario_audioscrobbler_build_authentication_data (audioscrobbler);
        if (auth_data != NULL) {
                gchar *post_data;

                post_data = ario_audioscrobbler_build_post_data (audioscrobbler,
                                                                 auth_data);
                g_free (auth_data);
                ARIO_LOG_DBG ("Submitting queue to Audioscrobbler");
                ario_audioscrobbler_print_queue (audioscrobbler, TRUE);

                ario_audioscrobbler_perform (audioscrobbler,
                                             audioscrobbler->priv->submit_url,
                                             post_data,
                                             ario_audioscrobbler_submit_queue_cb);
                /* libsoup will free post_data when the request is finished */
        }
}

static void
ario_g_queue_concat (GQueue *q1, GQueue *q2)
{
        GList *elem;

        while (!g_queue_is_empty (q2)) {
                elem = g_queue_pop_head_link (q2);
                g_queue_push_tail_link (q1, elem);
        }
}

/* Legal conversion specifiers, as specified in the C standard. */
#define C_STANDARD_STRFTIME_CHARACTERS "aAbBcdHIjmMpSUwWxXyYZ"
#define C_STANDARD_NUMERIC_STRFTIME_CHARACTERS "dHIjmMSUwWyY"
#define SUS_EXTENDED_STRFTIME_MODIFIERS "EO"

/**
 * eel_strdup_strftime:
 *
 * Cover for standard date-and-time-formatting routine strftime that returns
 * a newly-allocated string of the correct size. The caller is responsible
 * for g_free-ing the returned string.
 *
 * Besides the buffer management, there are two differences between this
 * and the library strftime:
 *
 *   1) The modifiers "-" and "_" between a "%" and a numeric directive
 *      are defined as for the GNU version of strftime. "-" means "do not
 *      pad the field" and "_" means "pad with spaces instead of zeroes".
 *   2) Non-ANSI extensions to strftime are flagged at runtime with a
 *      warning, so it's easy to notice use of the extensions without
 *      testing with multiple versions of the library.
 *
 * @format: format string to pass to strftime. See strftime documentation
 * for details.
 * @time_pieces: date/time, in struct format.
 *
 * Return value: Newly allocated string containing the formatted time.
 **/
static char *
eel_strdup_strftime (const char *format, struct tm *time_pieces)
{
        GString *string;
        const char *remainder, *percent;
        char code[4], buffer[512];
        char *piece, *result, *converted;
        size_t string_length;
        gboolean strip_leading_zeros, turn_leading_zeros_to_spaces;
        char modifier;
        int i;

        /* Format could be translated, and contain UTF-8 chars,
         * so convert to locale encoding which strftime uses */
        converted = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
        g_return_val_if_fail (converted != NULL, NULL);

        string = g_string_new ("");
        remainder = converted;

        /* Walk from % character to % character. */
        for (;;) {
                percent = strchr (remainder, '%');
                if (percent == NULL) {
                        g_string_append (string, remainder);
                        break;
                }
                g_string_append_len (string, remainder,
                                     percent - remainder);

                /* Handle the "%" character. */
                remainder = percent + 1;
                switch (*remainder) {
                case '-':
                        strip_leading_zeros = TRUE;
                        turn_leading_zeros_to_spaces = FALSE;
                        remainder++;
                        break;
                case '_':
                        strip_leading_zeros = FALSE;
                        turn_leading_zeros_to_spaces = TRUE;
                        remainder++;
                        break;
                case '%':
                        g_string_append_c (string, '%');
                        remainder++;
                        continue;
                case '\0':
                        g_warning ("Trailing %% passed to eel_strdup_strftime");
                        g_string_append_c (string, '%');
                        continue;
                default:
                        strip_leading_zeros = FALSE;
                        turn_leading_zeros_to_spaces = FALSE;
                        break;
                }

                modifier = 0;
                if (strchr (SUS_EXTENDED_STRFTIME_MODIFIERS, *remainder) != NULL) {
                        modifier = *remainder;
                        remainder++;

                        if (*remainder == 0) {
                                g_warning ("Unfinished %%%c modifier passed to eel_strdup_strftime", modifier);
                                break;
                        }
                }

                if (strchr (C_STANDARD_STRFTIME_CHARACTERS, *remainder) == NULL) {
                        g_warning ("eel_strdup_strftime does not support "
                                   "non-standard escape code %%%c",
                                   *remainder);
                }

                /* Convert code to strftime format. We have a fixed
                 * limit here that each code can expand to a maximum
                 * of 512 bytes, which is probably OK. There's no
                 * limit on the total size of the result string.
                 */
                i = 0;
                code[i++] = '%';
                if (modifier != 0) {
#ifdef HAVE_STRFTIME_EXTENSION
                        code[i++] = modifier;
#endif
                }
                code[i++] = *remainder;
                code[i++] = '\0';
                string_length = strftime (buffer, sizeof (buffer),
                                          code, time_pieces);
                if (string_length == 0) {
                        /* We could put a warning here, but there's no
                         * way to tell a successful conversion to
                         * empty string from a failure.
                         */
                        buffer[0] = '\0';
                }

                /* Strip leading zeros if requested. */
                piece = buffer;
                if (strip_leading_zeros || turn_leading_zeros_to_spaces) {
                        if (strchr (C_STANDARD_NUMERIC_STRFTIME_CHARACTERS, *remainder) == NULL) {
                                g_warning ("eel_strdup_strftime does not support "
                                           "modifier for non-numeric escape code %%%c%c",
                                           remainder[-1],
                                           *remainder);
                        }
                        if (*piece == '0') {
                                do {
                                        piece++;
                                } while (*piece == '0');
                                if (!g_ascii_isdigit (*piece)) {
                                        piece--;
                                }
                        }
                        if (turn_leading_zeros_to_spaces) {
                                memset (buffer, ' ', piece - buffer);
                                piece = buffer;
                        }
                }
                remainder++;

                /* Add this piece. */
                g_string_append (string, piece);
        }

        /* Convert the string back into utf-8. */
        result = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);

        g_string_free (string, TRUE);
        g_free (converted);

        return result;
}

/* Based on evolution/mail/message-list.c:filter_date() */
static char *
ario_utf_friendly_time (time_t date)
{
        time_t nowdate;
        time_t yesdate;
        struct tm then, now, yesterday;
        const char *format = NULL;
        char *str = NULL;
        gboolean done = FALSE;

        nowdate = time (NULL);

        if (date == 0)
                return NULL;

        localtime_r (&date, &then);
        localtime_r (&nowdate, &now);

        if (then.tm_mday == now.tm_mday &&
            then.tm_mon == now.tm_mon &&
            then.tm_year == now.tm_year) {
                /* Translators: "friendly time" string for the current day, strftime format. like "Today 12:34 am" */
                format = _("Today %I:%M %p");
                done = TRUE;
        }

        if (! done) {
                yesdate = nowdate - 60 * 60 * 24;
                localtime_r (&yesdate, &yesterday);
                if (then.tm_mday == yesterday.tm_mday &&
                    then.tm_mon == yesterday.tm_mon &&
                    then.tm_year == yesterday.tm_year) {
                        /* Translators: "friendly time" string for the previous day,
                         * strftime format. e.g. "Yesterday 12:34 am"
                         */
                        format = _("Yesterday %I:%M %p");
                        done = TRUE;
                }
        }

        if (! done) {
                int i;
                for (i = 2; i < 7; i++) {
                        yesdate = nowdate - 60 * 60 * 24 * i;
                        localtime_r (&yesdate, &yesterday);
                        if (then.tm_mday == yesterday.tm_mday &&
                            then.tm_mon == yesterday.tm_mon &&
                            then.tm_year == yesterday.tm_year) {
                                /* Translators: "friendly time" string for a day in the current week,
                                 * strftime format. e.g. "Wed 12:34 am"
                                 */
                                format = _("%a %I:%M %p");
                                done = TRUE;
                                break;
                        }
                }
        }

        if (! done) {
                if (then.tm_year == now.tm_year) {
                        /* Translators: "friendly time" string for a day in the current year,
                         * strftime format. e.g. "Feb 12 12:34 am"
                         */
                        format = _("%b %d %I:%M %p");
                } else {
                        /* Translators: "friendly time" string for a day in a different year,
                         * strftime format. e.g. "Feb 12 1997"
                         */
                        format = _("%b %d %Y");
                }
        }

        if (format != NULL) {
                str = eel_strdup_strftime (format, &then);
        }

        if (str == NULL) {
                /* impossible time or broken locale settings */
                str = g_strdup (_("Unknown"));
        }

        return str;
}

#if defined(HAVE_LIBSOUP_2_4)
static void
ario_audioscrobbler_submit_queue_cb (SoupSession *session, SoupMessage *msg, gpointer user_data)
#else
static void
ario_audioscrobbler_submit_queue_cb (SoupMessage *msg, gpointer user_data)
#endif
{
        ArioAudioscrobbler *audioscrobbler = ARIO_AUDIOSCROBBLER (user_data);

        ARIO_LOG_DBG ("Submission response");
        ario_audioscrobbler_parse_response (audioscrobbler, msg);

        if (audioscrobbler->priv->status == STATUS_OK) {
                ARIO_LOG_DBG ("Queue submitted successfully");
                ario_audioscrobbler_free_queue_entries (audioscrobbler, &audioscrobbler->priv->submission);
                audioscrobbler->priv->submission = g_queue_new ();
                ario_audioscrobbler_save_queue (audioscrobbler);

                audioscrobbler->priv->submit_count += audioscrobbler->priv->queue_count;
                audioscrobbler->priv->queue_count = 0;

                g_free (audioscrobbler->priv->submit_time);
                audioscrobbler->priv->submit_time = ario_utf_friendly_time (time (NULL));
        } else {
                ++audioscrobbler->priv->failures;

                /* add failed submission entries back to queue */
                ario_g_queue_concat (audioscrobbler->priv->submission, 
                                     audioscrobbler->priv->queue);
                g_assert (g_queue_is_empty (audioscrobbler->priv->queue));
                g_queue_free (audioscrobbler->priv->queue);
                audioscrobbler->priv->queue = audioscrobbler->priv->submission;
                audioscrobbler->priv->submission = g_queue_new ();;
                ario_audioscrobbler_save_queue (audioscrobbler);

                ario_audioscrobbler_print_queue (audioscrobbler, FALSE);

                if (audioscrobbler->priv->failures >= 3) {
                        ARIO_LOG_DBG ("Queue submission has failed %d times; caching tracks locally",
                                      audioscrobbler->priv->failures);
                        g_free (audioscrobbler->priv->status_msg);

                        audioscrobbler->priv->handshake = FALSE;
                        audioscrobbler->priv->status = GIVEN_UP;
                        audioscrobbler->priv->status_msg = NULL;
                } else {
                        ARIO_LOG_DBG ("Queue submission failed %d times", audioscrobbler->priv->failures);
                }
        }

        ario_audioscrobbler_preferences_sync (audioscrobbler);
        g_idle_add ((GSourceFunc) idle_unref_cb, audioscrobbler);
}

/* Configuration functions: */
static void
ario_audioscrobbler_import_settings (ArioAudioscrobbler *audioscrobbler)
{
        /* import conf settings. */
        g_free (audioscrobbler->priv->username);
        g_free (audioscrobbler->priv->password);
        audioscrobbler->priv->username = ario_conf_get_string (CONF_AUDIOSCROBBLER_USERNAME, NULL);
        audioscrobbler->priv->password = ario_conf_get_string (CONF_AUDIOSCROBBLER_PASSWORD, NULL);

        ario_audioscrobbler_add_timeout (audioscrobbler);
        audioscrobbler->priv->status = HANDSHAKING;

        audioscrobbler->priv->submit_time = g_strdup (_("Never"));
}

static void
ario_audioscrobbler_preferences_sync (ArioAudioscrobbler *audioscrobbler)
{
        const char *status;
        char *free_this = NULL;
        char *v;

        if (audioscrobbler->priv->preferences == NULL)
                return;

        ARIO_LOG_DBG ("Syncing data with preferences window");
        v = audioscrobbler->priv->username;
        gtk_entry_set_text (GTK_ENTRY (audioscrobbler->priv->username_entry),
                            v ? v : "");
        v = audioscrobbler->priv->password;
        gtk_entry_set_text (GTK_ENTRY (audioscrobbler->priv->password_entry),
                            v ? v : "");

        switch (audioscrobbler->priv->status) {
        case STATUS_OK:
                status = _("OK");
                break;
        case HANDSHAKING:
                status = _("Logging in");
                break;
        case REQUEST_FAILED:
                status = _("Request failed");
                break;
        case BAD_USERNAME:
                status = _("Incorrect username");
                break;
        case BAD_PASSWORD:
                status = _("Incorrect password");
                break;
        case HANDSHAKE_FAILED:
                status = _("Handshake failed");
                break;
        case CLIENT_UPDATE_REQUIRED:
                status = _("Client update required");
                break;
        case SUBMIT_FAILED:
                status = _("Track submission failed");
                break;
        case QUEUE_TOO_LONG:
                status = _("Queue is too long");
                break;
        case GIVEN_UP:
                status = _("Track submission failed too many times");
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        if (audioscrobbler->priv->status_msg && audioscrobbler->priv->status_msg[0] != '\0') {
                free_this = g_strdup_printf ("%s: %s", status, audioscrobbler->priv->status_msg);
                status = free_this;
        }

        gtk_label_set_text (GTK_LABEL (audioscrobbler->priv->status_label),
                            status);
        g_free (free_this);

        free_this = g_strdup_printf ("%u", audioscrobbler->priv->submit_count);
        gtk_label_set_text (GTK_LABEL (audioscrobbler->priv->submit_count_label), free_this);
        g_free (free_this);

        free_this = g_strdup_printf ("%u", audioscrobbler->priv->queue_count);
        gtk_label_set_text (GTK_LABEL (audioscrobbler->priv->queue_count_label), free_this);
        g_free (free_this);

        gtk_label_set_text (GTK_LABEL (audioscrobbler->priv->submit_time_label),
                            audioscrobbler->priv->submit_time);
}

static void
ario_audioscrobbler_preferences_response_cb (GtkWidget *dialog, gint response, ArioAudioscrobbler *audioscrobbler)
{
        gtk_widget_destroy (audioscrobbler->priv->preferences);
        audioscrobbler->priv->preferences = NULL;
}


static void
ario_audioscrobbler_preferences_close_cb (GtkWidget *dialog, ArioAudioscrobbler *audioscrobbler)
{
        gtk_widget_destroy (audioscrobbler->priv->preferences);
        audioscrobbler->priv->preferences = NULL;
}

GtkWidget *
ario_audioscrobbler_get_config_widget (ArioAudioscrobbler *audioscrobbler,
                                       ArioPlugin *plugin)
{
        GladeXML *xml;
        GtkWidget *config_widget;
        if (audioscrobbler->priv->preferences == NULL) {
                audioscrobbler->priv->preferences = gtk_dialog_new_with_buttons (_("Audioscrobbler preferences"),
                                                                                 NULL,
                                                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                                 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                                                                 NULL);
                g_signal_connect (G_OBJECT (audioscrobbler->priv->preferences),
                                  "response",
                                  G_CALLBACK (ario_audioscrobbler_preferences_response_cb),
                                  audioscrobbler);
                g_signal_connect (G_OBJECT (audioscrobbler->priv->preferences),
                                  "close",
                                  G_CALLBACK (ario_audioscrobbler_preferences_close_cb),
                                  audioscrobbler);

                xml = rb_glade_xml_new (PLUGINDIR "audioscrobbler-prefs.glade", "audioscrobbler_vbox", audioscrobbler);

                config_widget = glade_xml_get_widget (xml, "audioscrobbler_vbox");
                audioscrobbler->priv->username_entry = glade_xml_get_widget (xml, "username_entry");
                audioscrobbler->priv->username_label = glade_xml_get_widget (xml, "username_label");
                audioscrobbler->priv->password_entry = glade_xml_get_widget (xml, "password_entry");
                audioscrobbler->priv->password_label = glade_xml_get_widget (xml, "password_label");
                audioscrobbler->priv->status_label = glade_xml_get_widget (xml, "status_label");
                audioscrobbler->priv->queue_count_label = glade_xml_get_widget (xml, "queue_count_label");
                audioscrobbler->priv->submit_count_label = glade_xml_get_widget (xml, "submit_count_label");
                audioscrobbler->priv->submit_time_label = glade_xml_get_widget (xml, "submit_time_label");

                rb_glade_boldify_label (xml, "audioscrobbler_label");

                g_object_unref (G_OBJECT (xml));

                gtk_container_add (GTK_CONTAINER (GTK_DIALOG (audioscrobbler->priv->preferences)->vbox), config_widget);
        }

        ario_audioscrobbler_preferences_sync (audioscrobbler);

        gtk_widget_show_all (audioscrobbler->priv->preferences);

        return audioscrobbler->priv->preferences;
}

static void
ario_audioscrobbler_conf_username_changed_cb (gpointer do_not_use1,
                                              guint cnxn_id,
                                              gpointer do_not_use2,
                                              ArioAudioscrobbler *audioscrobbler)
{

        const gchar *username;

        g_free (audioscrobbler->priv->username);
        audioscrobbler->priv->username = NULL;

        username = ario_conf_get_string (CONF_AUDIOSCROBBLER_USERNAME, NULL);
        if (username != NULL) {
                audioscrobbler->priv->username = g_strdup (username);
        }

        if (audioscrobbler->priv->username_entry) {
                char *v = audioscrobbler->priv->username;
                gtk_entry_set_text (GTK_ENTRY (audioscrobbler->priv->username_entry),
                                    v ? v : "");
        }

        audioscrobbler->priv->handshake = FALSE;
}

static void
ario_audioscrobbler_conf_password_changed_cb (gpointer do_not_use1,
                                              guint cnxn_id,
                                              gpointer do_not_use2,
                                              ArioAudioscrobbler *audioscrobbler)
{
        const gchar *password;

        g_free (audioscrobbler->priv->password);
        audioscrobbler->priv->password = NULL;

        password = ario_conf_get_string (CONF_AUDIOSCROBBLER_PASSWORD, NULL);
        if (password != NULL) {
                audioscrobbler->priv->password = g_strdup (password);
        }

        if (audioscrobbler->priv->password_entry) {
                char *v = audioscrobbler->priv->password;
                gtk_entry_set_text (GTK_ENTRY (audioscrobbler->priv->password_entry),
                                    v ? v : "");
        }
}

static void
ario_audioscrobbler_song_changed_cb (ArioMpd *mpd,
                                     ArioAudioscrobbler *audioscrobbler)
{
        ArioMpdSong *song;

        song = ario_mpd_get_current_song (mpd);

        if (audioscrobbler->priv->currently_playing != NULL) {
                audioscrobbler_entry_free (audioscrobbler->priv->currently_playing);
                audioscrobbler->priv->currently_playing = NULL;
        }

        if (song == NULL) {
                ARIO_LOG_DBG ("called with no playing song");
                return;
        }

        audioscrobbler->priv->current_elapsed = ario_mpd_get_current_elapsed (mpd);

        if (ario_audioscrobbler_is_queueable (song) && (audioscrobbler->priv->current_elapsed < 15)) {
                AudioscrobblerEntry *as_entry;

                /* even if it's the same song, it's being played again from
                 * the start so we can queue it again.
                 */
                as_entry = ario_audioscrobbler_create_entry (song);
                audioscrobbler->priv->currently_playing = as_entry;
        }
}


void
ario_audioscrobbler_username_entry_changed_cb (GtkEntry *entry,
                                               ArioAudioscrobbler *audioscrobbler)
{
        ario_conf_set_string (CONF_AUDIOSCROBBLER_USERNAME,
                              gtk_entry_get_text (entry));
}

void
ario_audioscrobbler_username_entry_activate_cb (GtkEntry *entry,
                                                ArioAudioscrobbler *audioscrobbler)
{
        gtk_widget_grab_focus (audioscrobbler->priv->password_entry);
}

void
ario_audioscrobbler_password_entry_changed_cb (GtkEntry *entry,
                                               ArioAudioscrobbler *audioscrobbler)
{
        ario_conf_set_string (CONF_AUDIOSCROBBLER_PASSWORD,
                              gtk_entry_get_text (entry));
}

void
ario_audioscrobbler_password_entry_activate_cb (GtkEntry *entry,
                                                ArioAudioscrobbler *audioscrobbler)
{
        /* ? */
}

/* AudioscrobblerEntry functions: */
static void
audioscrobbler_entry_init (AudioscrobblerEntry *entry)
{
        entry->artist = g_strdup ("");
        entry->album = g_strdup ("");
        entry->title = g_strdup ("");
        entry->length = 0;
        entry->play_time = 0;
}

static void
audioscrobbler_entry_free (AudioscrobblerEntry *entry)
{
        g_free (entry->artist);
        g_free (entry->album);
        g_free (entry->title);

        g_free (entry);
}

static AudioscrobblerEncodedEntry *
audioscrobbler_entry_encode (AudioscrobblerEntry *entry)
{

        AudioscrobblerEncodedEntry *encoded;

        encoded = g_new0 (AudioscrobblerEncodedEntry, 1);

        encoded->artist = soup_uri_encode (entry->artist, 
                                           EXTRA_URI_ENCODE_CHARS);
        encoded->title = soup_uri_encode (entry->title,
                                          EXTRA_URI_ENCODE_CHARS);
        encoded->album = soup_uri_encode (entry->album, 
                                          EXTRA_URI_ENCODE_CHARS);

        encoded->timestamp = g_new0 (gchar, 30);
        strftime (encoded->timestamp, 30, SCROBBLER_DATE_FORMAT, 
                  gmtime (&entry->play_time));

        encoded->length = entry->length;

        return encoded;
}

static 
void audioscrobbler_encoded_entry_free (AudioscrobblerEncodedEntry *entry)
{
        g_free (entry->artist);
        g_free (entry->album);
        g_free (entry->title);
        g_free (entry->timestamp);

        g_free (entry);
}


/* Queue functions: */

static AudioscrobblerEntry*
ario_audioscrobbler_load_entry_from_string (const char *string)
{
        AudioscrobblerEntry *entry;
        int i = 0;
        char **breaks;

        entry = g_new0 (AudioscrobblerEntry, 1);
        audioscrobbler_entry_init (entry);

        breaks = g_strsplit (string, "&", 6);

        for (i = 0; breaks[i] != NULL; i++) {
                char **breaks2 = g_strsplit (breaks[i], "=", 2);

                if (breaks2[0] != NULL && breaks2[1] != NULL) {
                        if (g_str_has_prefix (breaks2[0], "a")) {
                                g_free (entry->artist);
                                entry->artist = g_strdup (breaks2[1]);
                                soup_uri_decode (entry->artist);
                        }
                        if (g_str_has_prefix (breaks2[0], "t")) {
                                g_free (entry->title);
                                entry->title = g_strdup (breaks2[1]);
                                soup_uri_decode (entry->title);
                        }
                        if (g_str_has_prefix (breaks2[0], "b")) {
                                g_free (entry->album);
                                entry->album = g_strdup (breaks2[1]);
                                soup_uri_decode (entry->album);
                        }

                        if (g_str_has_prefix (breaks2[0], "l")) {
                                entry->length = atoi (breaks2[1]);
                        }
                        if (g_str_has_prefix (breaks2[0], "i")) {
                                struct tm tm;
                                strptime (breaks2[1], SCROBBLER_DATE_FORMAT, 
                                          &tm);
                                entry->play_time = mktime (&tm);
                        }
                }

                g_strfreev (breaks2);
        }

        g_strfreev (breaks);

        if (strcmp (entry->artist, "") == 0 || strcmp (entry->title, "") == 0) {
                audioscrobbler_entry_free (entry);
                entry = NULL;
        }

        return entry;
}

static gboolean
ario_audioscrobbler_load_queue (ArioAudioscrobbler *audioscrobbler)
{
        char *pathname;
        gboolean result;
        char *data;
        gsize size;

        pathname = g_build_filename (ario_util_config_dir (), "audioscrobbler.queue", NULL);
        ARIO_LOG_DBG ("Loading Audioscrobbler queue from \"%s\"", pathname);

        result = g_file_get_contents (pathname, &data, &size, NULL);
        g_free (pathname);

        /* do stuff */
        if (result) {
                char *start = data, *end;

                /* scan along the file's data, turning each line into a string */
                while (start < (data + size)) {
                        AudioscrobblerEntry *entry;

                        /* find the end of the line, to terminate the string */
                        end = g_utf8_strchr (start, -1, '\n');
                        if (end == NULL)
                                break;
                        *end = 0;

                        entry = ario_audioscrobbler_load_entry_from_string (start);
                        if (entry) {
                                g_queue_push_tail (audioscrobbler->priv->queue,
                                                   entry);
                                audioscrobbler->priv->queue_count++;
                        }

                        start = end + 1;
                }
        } else {
                ARIO_LOG_DBG ("Unable to load Audioscrobbler queue from disk");
        }

        g_free (data);
        return result;
}

static char *
ario_audioscrobbler_save_entry_to_string (AudioscrobblerEntry *entry)
{
        char *result;
        AudioscrobblerEncodedEntry *encoded;

        encoded = audioscrobbler_entry_encode (entry);
        result = g_strdup_printf ("a=%s&t=%s&b=%s&m=&l=%d&i=%s\n",
                                  encoded->artist, 
                                  encoded->title,
                                  encoded->album,
                                  encoded->length,
                                  encoded->timestamp);
        audioscrobbler_encoded_entry_free (encoded);
        return result;
}

static gboolean
ario_audioscrobbler_save_queue (ArioAudioscrobbler *audioscrobbler)
{
        char *pathname;
        gboolean ret;
        GString *string = g_string_new (NULL);
        GList *list;

        if (!audioscrobbler->priv->queue_changed) {
                return TRUE;
        }

        pathname = g_build_filename (ario_util_config_dir (), "audioscrobbler.queue", NULL);
        ARIO_LOG_DBG ("Saving Audioscrobbler queue to \"%s\"", pathname);

        for (list = audioscrobbler->priv->queue->head;
             list != NULL;
             list = g_list_next (list)) {
                AudioscrobblerEntry *entry;
                char *str;
                entry = (AudioscrobblerEntry *) list->data;
                str = ario_audioscrobbler_save_entry_to_string (entry);
                string = g_string_append (string, str);
        }

        ret = g_file_set_contents (pathname,
                                   string->str, -1,
                                   NULL);
        g_string_free (string, TRUE);

        g_free (pathname);

        audioscrobbler->priv->queue_changed = FALSE;

        return ret;
}

static void
ario_audioscrobbler_print_queue (ArioAudioscrobbler *audioscrobbler, gboolean submission)
{
        GList *l;
        AudioscrobblerEntry *entry;

        if (submission) {
                l = audioscrobbler->priv->submission->head;
                ARIO_LOG_DBG ("Audioscrobbler submission (%d entries): ", 
                              g_queue_get_length (audioscrobbler->priv->submission));

        } else {
                l = audioscrobbler->priv->queue->head;
                ARIO_LOG_DBG ("Audioscrobbler queue (%d entries): ", 
                              g_queue_get_length (audioscrobbler->priv->queue));
        }

        for (; l != NULL; l = g_list_next (l)) {
                char timestamp[30];
                entry = (AudioscrobblerEntry *) l->data;

                ARIO_LOG_DBG ("      artist: %s", entry->artist);
                ARIO_LOG_DBG ("      album: %s", entry->album);
                ARIO_LOG_DBG ("      title: %s", entry->title);
                ARIO_LOG_DBG ("     length: %d", entry->length);
                strftime (timestamp, 30, SCROBBLER_DATE_FORMAT, 
                          gmtime (&entry->play_time));
                ARIO_LOG_DBG ("  timestamp: %s", timestamp);
        }
}

static void
ario_audioscrobbler_free_queue_entries (ArioAudioscrobbler *audioscrobbler, GQueue **queue)
{
        g_queue_foreach (*queue, (GFunc) audioscrobbler_entry_free, NULL);
        g_queue_free (*queue);
        *queue = NULL;

        audioscrobbler->priv->queue_changed = TRUE;
}

