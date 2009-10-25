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

#include "covers/ario-cover-handler.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "covers/ario-cover-manager.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "servers/ario-server.h"
#include "ario-cover.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"

static void ario_cover_handler_finalize (GObject *object);
static void ario_cover_handler_load_pixbuf (ArioCoverHandler *cover_handler,
                                            gboolean should_get);
static void ario_cover_handler_album_changed_cb (ArioServer *server,
                                                 ArioCoverHandler *cover_handler);
static void ario_cover_handler_state_changed_cb (ArioServer *server,
                                                 ArioCoverHandler *cover_handler);

enum
{
        COVER_CHANGED,
        LAST_SIGNAL
};
static guint ario_cover_handler_signals[LAST_SIGNAL] = { 0 };

struct ArioCoverHandlerPrivate
{
        GThread *thread;
        GAsyncQueue *queue;

        gchar *cover_path;

        GdkPixbuf *pixbuf;
        GdkPixbuf *large_pixbuf;
};

typedef struct ArioCoverHandlerData
{
        gchar *artist;
        gchar *album;
        gchar *path;
} ArioCoverHandlerData;

#define ARIO_COVER_HANDLER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_COVER_HANDLER, ArioCoverHandlerPrivate))
G_DEFINE_TYPE (ArioCoverHandler, ario_cover_handler, G_TYPE_OBJECT)

static ArioCoverHandler *instance = NULL;

static void
ario_cover_handler_class_init (ArioCoverHandlerClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_cover_handler_finalize;

        ario_cover_handler_signals[COVER_CHANGED] =
                g_signal_new ("cover_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioCoverHandlerClass, cover_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        g_type_class_add_private (klass, sizeof (ArioCoverHandlerPrivate));
}

static void
ario_cover_handler_init (ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START;
        cover_handler->priv = ARIO_COVER_HANDLER_GET_PRIVATE (cover_handler);
        cover_handler->priv->thread = NULL;
        cover_handler->priv->queue = g_async_queue_new ();
}

ArioCoverHandler *
ario_cover_handler_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverHandler *cover_handler;
        ArioServer *server = ario_server_get_instance ();

        cover_handler = g_object_new (TYPE_ARIO_COVER_HANDLER,
                                      NULL);

        g_return_val_if_fail (cover_handler->priv != NULL, NULL);

        instance = cover_handler;

        g_signal_connect_object (server,
                                 "album_changed",
                                 G_CALLBACK (ario_cover_handler_album_changed_cb),
                                 cover_handler, 0);
        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_cover_handler_state_changed_cb),
                                 cover_handler, 0);

        return cover_handler;
}

static void
ario_cover_handler_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverHandler *cover_handler;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_COVER_HANDLER (object));

        cover_handler = ARIO_COVER_HANDLER (object);

        g_return_if_fail (cover_handler->priv != NULL);

        if (cover_handler->priv->thread)
                g_thread_join (cover_handler->priv->thread);
        g_async_queue_unref (cover_handler->priv->queue);

        if (cover_handler->priv->pixbuf)
                g_object_unref(cover_handler->priv->pixbuf);

        if (cover_handler->priv->large_pixbuf)
                g_object_unref(cover_handler->priv->large_pixbuf);

        g_free (cover_handler->priv->cover_path);

        G_OBJECT_CLASS (ario_cover_handler_parent_class)->finalize (object);
}

static gboolean
ario_cover_handler_cover_changed (ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START;

        g_signal_emit (G_OBJECT (cover_handler), ario_cover_handler_signals[COVER_CHANGED], 0);

        return FALSE;
}

static void
ario_cover_handler_free_data (ArioCoverHandlerData *data)
{
        ARIO_LOG_FUNCTION_START;
        if (data) {
                g_free (data->artist);
                g_free (data->album);
                g_free (data->path);
                g_free (data);
        }
}

static gpointer
ario_cover_handler_get_covers (ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START;
        GArray *size;
        GSList *covers;
        gboolean ret;
        ArioCoverHandlerData *data;

        while ((data = (ArioCoverHandlerData *) g_async_queue_try_pop (cover_handler->priv->queue))) {
                if (ario_cover_cover_exists (data->artist, data->album)) {
                        ario_cover_handler_free_data (data);
                        continue;
                }

                covers = NULL;
                size = g_array_new (TRUE, TRUE, sizeof (int));

                /* If a cover is found, it is loaded in covers(0) */
                ret = ario_cover_manager_get_covers (ario_cover_manager_get_instance (),
                                                     data->artist,
                                                     data->album,
                                                     data->path,
                                                     &size,
                                                     &covers,
                                                     GET_FIRST_COVER);

                /* If the cover is not too big and not too small (blank image), we save it */
                if (ret && ario_cover_size_is_valid (g_array_index (size, int, 0))) {
                        ret = ario_cover_save_cover (data->artist,
                                                     data->album,
                                                     g_slist_nth_data (covers, 0),
                                                     g_array_index (size, int, 0),
                                                     OVERWRITE_MODE_SKIP);

                        if (ret) {
                                ario_cover_handler_load_pixbuf (cover_handler, FALSE);
                                g_idle_add ((GSourceFunc) ario_cover_handler_cover_changed, cover_handler);
                        }
                }
                ario_cover_handler_free_data (data);

                g_array_free (size, TRUE);
                g_slist_foreach (covers, (GFunc) g_free, NULL);
                g_slist_free (covers);
        }
        cover_handler->priv->thread = NULL;

        return NULL;
}

static void
ario_cover_handler_load_pixbuf (ArioCoverHandler *cover_handler,
                                gboolean should_get)
{
        ARIO_LOG_FUNCTION_START;
        gchar *large_cover_path;
        ArioCoverHandlerData *data;
        gchar *artist = ario_server_get_current_artist ();
        gchar *album = ario_server_get_current_album ();

        if (!artist)
                artist = ARIO_SERVER_UNKNOWN;
        if (!album)
                album = ARIO_SERVER_UNKNOWN;

        if (cover_handler->priv->pixbuf) {
                g_object_unref(cover_handler->priv->pixbuf);
                cover_handler->priv->pixbuf = NULL;
        }

        if (cover_handler->priv->large_pixbuf) {
                g_object_unref(cover_handler->priv->large_pixbuf);
                cover_handler->priv->large_pixbuf = NULL;
        }

        if (cover_handler->priv->cover_path) {
                g_free (cover_handler->priv->cover_path);
                cover_handler->priv->cover_path = NULL;
        }

        switch (ario_server_get_current_state ()) {
        case ARIO_STATE_PLAY:
        case ARIO_STATE_PAUSE:
                cover_handler->priv->cover_path = ario_cover_make_cover_path (artist, album, SMALL_COVER);
                large_cover_path = ario_cover_make_cover_path (artist, album, NORMAL_COVER);
                if (cover_handler->priv->cover_path && large_cover_path) {
                        cover_handler->priv->pixbuf = gdk_pixbuf_new_from_file_at_size (cover_handler->priv->cover_path, COVER_SIZE, COVER_SIZE, NULL);
                        cover_handler->priv->large_pixbuf = gdk_pixbuf_new_from_file_at_size (large_cover_path, 2*COVER_SIZE, 2*COVER_SIZE, NULL);
                        if (!cover_handler->priv->cover_path) {
                                g_free (cover_handler->priv->cover_path);
                                cover_handler->priv->cover_path = NULL;
                        }
                        g_free (large_cover_path);
                        if (!cover_handler->priv->pixbuf
                            && should_get
                            && ario_conf_get_boolean (PREF_AUTOMATIC_GET_COVER, PREF_AUTOMATIC_GET_COVER_DEFAULT)) {
                                data = (ArioCoverHandlerData *) g_malloc0 (sizeof (ArioCoverHandlerData));
                                data->artist = g_strdup (artist);
                                data->album = g_strdup (album);
                                if (ario_server_get_current_song_path ())
                                {
                                        data->path = g_path_get_dirname (ario_server_get_current_song_path ());
                                        g_async_queue_push (cover_handler->priv->queue, data);

                                        if (!cover_handler->priv->thread) {
                                                cover_handler->priv->thread = g_thread_create ((GThreadFunc) ario_cover_handler_get_covers,
                                                                                               cover_handler,
                                                                                               TRUE,
                                                                                               NULL);
                                        }
                                }
                        }
                }
                break;
        default:
                break;
        }
}

static void
ario_cover_handler_album_changed_cb (ArioServer *server,
                                     ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START;
        ario_cover_handler_load_pixbuf(cover_handler, TRUE);
        g_signal_emit (G_OBJECT (cover_handler), ario_cover_handler_signals[COVER_CHANGED], 0);
}

static void
ario_cover_handler_state_changed_cb (ArioServer *server,
                                     ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START;
        ario_cover_handler_load_pixbuf(cover_handler, TRUE);
        g_signal_emit (G_OBJECT (cover_handler), ario_cover_handler_signals[COVER_CHANGED], 0);
}

void
ario_cover_handler_force_reload (void)
{
        ARIO_LOG_FUNCTION_START;
        ario_cover_handler_load_pixbuf (instance, TRUE);
        g_signal_emit (G_OBJECT (instance), ario_cover_handler_signals[COVER_CHANGED], 0);
}

ArioCoverHandler *
ario_cover_handler_get_instance (void)
{
        ARIO_LOG_FUNCTION_START;
        return instance;
}

GdkPixbuf *
ario_cover_handler_get_cover (void)
{
        ARIO_LOG_FUNCTION_START;
        return instance->priv->pixbuf;
}

GdkPixbuf *
ario_cover_handler_get_large_cover (void)
{
        ARIO_LOG_FUNCTION_START;
        return instance->priv->large_pixbuf;
}

gchar *
ario_cover_handler_get_cover_path (void)
{
        ARIO_LOG_FUNCTION_START;
        return instance->priv->cover_path;
}
