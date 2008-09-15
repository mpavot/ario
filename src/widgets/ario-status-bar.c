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


#include "widgets/ario-status-bar.h"
#include "ario-util.h"
#include <glib/gi18n.h>
#include "ario-debug.h"

static void ario_status_bar_set_property (GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec);
static void ario_status_bar_get_property (GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec);
static void ario_status_bar_playlist_changed_cb (ArioMpd *mpd,
                                                 ArioStatusBar *status_bar);

struct ArioStatusBarPrivate
{
        guint ario_playlist_context_id;

        ArioMpd *mpd;
};

enum
{
        PROP_NONE,
        PROP_MPD
};

#define ARIO_STATUS_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_STATUS_BAR, ArioStatusBarPrivate))
G_DEFINE_TYPE (ArioStatusBar, ario_status_bar, GTK_TYPE_STATUSBAR)

static void
ario_status_bar_class_init (ArioStatusBarClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = ario_status_bar_set_property;
        object_class->get_property = ario_status_bar_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));

        g_type_class_add_private (klass, sizeof (ArioStatusBarPrivate));
}

static void
ario_status_bar_init (ArioStatusBar *status_bar)
{
        ARIO_LOG_FUNCTION_START
        status_bar->priv = ARIO_STATUS_BAR_GET_PRIVATE (status_bar);
        status_bar->priv->ario_playlist_context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (status_bar), "PlaylistMsg");
}

static void
ario_status_bar_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusBar *status_bar = ARIO_STATUS_BAR (object);

        switch (prop_id)
        {
        case PROP_MPD:
                status_bar->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (status_bar->priv->mpd,
                                         "playlist_changed",
                                         G_CALLBACK (ario_status_bar_playlist_changed_cb),
                                         status_bar, 0);
                g_signal_connect_object (status_bar->priv->mpd,
                                         "updatingdb_changed",
                                         G_CALLBACK (ario_status_bar_playlist_changed_cb),
                                         status_bar, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_status_bar_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusBar *status_bar = ARIO_STATUS_BAR (object);

        switch (prop_id)
        {
        case PROP_MPD:
                g_value_set_object (value, status_bar->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_status_bar_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusBar *status_bar;

        status_bar = g_object_new (TYPE_ARIO_STATUS_BAR,
                                   "mpd", mpd,
                                   NULL);

        g_return_val_if_fail (status_bar->priv != NULL, NULL);

        return GTK_WIDGET (status_bar);
}

static void
ario_status_bar_playlist_changed_cb (ArioMpd *mpd,
                                     ArioStatusBar *status_bar)
{
        ARIO_LOG_FUNCTION_START
        gchar *msg, *tmp;
        gchar *formated_total_time;
        int ario_playlist_length;
        int ario_playlist_total_time;

        ario_playlist_length = ario_mpd_get_current_playlist_length (mpd);

        ario_playlist_total_time = ario_mpd_get_current_playlist_total_time (mpd);
        formated_total_time = ario_util_format_total_time (ario_playlist_total_time);

        msg = g_strdup_printf ("%d %s - %s", ario_playlist_length, _("Songs"), formated_total_time);
        g_free (formated_total_time);

        if (ario_mpd_get_updating (status_bar->priv->mpd)) {
                tmp = g_strdup_printf ("%s - %s", msg, _("Updating..."));
                g_free (msg);
                msg = tmp;
        }                

        gtk_statusbar_pop (GTK_STATUSBAR(status_bar), status_bar->priv->ario_playlist_context_id);
        gtk_statusbar_push (GTK_STATUSBAR (status_bar), status_bar->priv->ario_playlist_context_id, msg);

        g_free (msg);
}
