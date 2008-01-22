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

#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "ario-cover-handler.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "ario-cover.h"

static void ario_cover_handler_class_init (ArioCoverHandlerClass *klass);
static void ario_cover_handler_init (ArioCoverHandler *cover_handler);
static void ario_cover_handler_finalize (GObject *object);
static void ario_cover_handler_set_property (GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec);
static void ario_cover_handler_get_property (GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec);
static void ario_cover_handler_load_pixbuf (ArioCoverHandler *cover_handler);
static void ario_cover_handler_album_changed_cb (ArioMpd *mpd,
                                                 ArioCoverHandler *cover_handler);
static void ario_cover_handler_state_changed_cb (ArioMpd *mpd,
                                                 ArioCoverHandler *cover_handler);

enum
{
        PROP_0,
        PROP_MPD
};

enum
{
        COVER_CHANGED,
        LAST_SIGNAL
};

static guint ario_cover_handler_signals[LAST_SIGNAL] = { 0 };

struct ArioCoverHandlerPrivate
{
        ArioMpd *mpd;

        GdkPixbuf *pixbuf;
};

static GObjectClass *parent_class = NULL;

static ArioCoverHandler *instance = NULL;

GType
ario_cover_handler_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_cover_handler_type = 0;

        if (ario_cover_handler_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioCoverHandlerClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_cover_handler_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioCoverHandler),
                        0,
                        (GInstanceInitFunc) ario_cover_handler_init
                };

                ario_cover_handler_type = g_type_register_static (G_TYPE_OBJECT,
                                                                  "ArioCoverHandler",
                                                                  &our_info, 0);
        }

        return ario_cover_handler_type;
}

static void
ario_cover_handler_class_init (ArioCoverHandlerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_cover_handler_finalize;
        object_class->set_property = ario_cover_handler_set_property;
        object_class->get_property = ario_cover_handler_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
        ario_cover_handler_signals[COVER_CHANGED] =
                g_signal_new ("cover_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioCoverHandlerClass, cover_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
ario_cover_handler_init (ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START
        cover_handler->priv = g_new0 (ArioCoverHandlerPrivate, 1);
        cover_handler->priv->pixbuf = NULL;
}

ArioCoverHandler *
ario_cover_handler_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverHandler *cover_handler;

        cover_handler = g_object_new (TYPE_ARIO_COVER_HANDLER,
                                      "mpd", mpd,
                                      NULL);

        g_return_val_if_fail (cover_handler->priv != NULL, NULL);

        instance = cover_handler;

        return cover_handler;
}

static void
ario_cover_handler_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverHandler *cover_handler;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_COVER_HANDLER (object));

        cover_handler = ARIO_COVER_HANDLER (object);

        g_return_if_fail (cover_handler->priv != NULL);

        if (cover_handler->priv->pixbuf) 
                g_object_unref(cover_handler->priv->pixbuf);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_cover_handler_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverHandler *cover_handler = ARIO_COVER_HANDLER (object);

        switch (prop_id) {
        case PROP_MPD:
                cover_handler->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (cover_handler->priv->mpd),
                                         "album_changed", G_CALLBACK (ario_cover_handler_album_changed_cb),
                                         cover_handler, 0);
                g_signal_connect_object (G_OBJECT (cover_handler->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_cover_handler_state_changed_cb),
                                         cover_handler, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_cover_handler_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverHandler *cover_handler = ARIO_COVER_HANDLER (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, cover_handler->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_cover_handler_load_pixbuf (ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START
        gchar *cover_path;

        if (cover_handler->priv->pixbuf) {
                g_object_unref(cover_handler->priv->pixbuf);
                cover_handler->priv->pixbuf = NULL;
        }

        cover_path = ario_cover_make_ario_cover_path (ario_mpd_get_current_artist (cover_handler->priv->mpd),
                                                      ario_mpd_get_current_album (cover_handler->priv->mpd), SMALL_COVER);
        cover_handler->priv->pixbuf = gdk_pixbuf_new_from_file_at_size (cover_path, COVER_SIZE, COVER_SIZE, NULL);
        g_free (cover_path);
}

static void
ario_cover_handler_album_changed_cb (ArioMpd *mpd,
                                     ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START
        ario_cover_handler_load_pixbuf(cover_handler);
        g_signal_emit (G_OBJECT (cover_handler), ario_cover_handler_signals[COVER_CHANGED], 0);
}

static void
ario_cover_handler_state_changed_cb (ArioMpd *mpd,
                                     ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START
        ario_cover_handler_load_pixbuf(cover_handler);
        g_signal_emit (G_OBJECT (cover_handler), ario_cover_handler_signals[COVER_CHANGED], 0);
}

void
ario_cover_handler_force_reload (void)
{
        ARIO_LOG_FUNCTION_START
        ario_cover_handler_load_pixbuf (instance);
        g_signal_emit (G_OBJECT (instance), ario_cover_handler_signals[COVER_CHANGED], 0);
}

GdkPixbuf *
ario_cover_handler_get_cover (ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START
        return cover_handler->priv->pixbuf;
}
