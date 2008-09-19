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
#include <glib/gi18n.h>
#include "ario-mpd.h"
#include "ario-util.h"
#include "ario-debug.h"

static void ario_status_bar_playlist_changed_cb (ArioMpd *mpd,
                                                 ArioStatusBar *status_bar);

struct ArioStatusBarPrivate
{
        guint ario_playlist_context_id;
};

#define ARIO_STATUS_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_STATUS_BAR, ArioStatusBarPrivate))
G_DEFINE_TYPE (ArioStatusBar, ario_status_bar, GTK_TYPE_STATUSBAR)

static void
ario_status_bar_class_init (ArioStatusBarClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioStatusBarPrivate));
}

static void
ario_status_bar_init (ArioStatusBar *status_bar)
{
        ARIO_LOG_FUNCTION_START
        status_bar->priv = ARIO_STATUS_BAR_GET_PRIVATE (status_bar);
        status_bar->priv->ario_playlist_context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (status_bar), "PlaylistMsg");
}

GtkWidget *
ario_status_bar_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusBar *status_bar;
        ArioMpd *mpd = ario_mpd_get_instance ();

        status_bar = g_object_new (TYPE_ARIO_STATUS_BAR,
                                   NULL);

        g_return_val_if_fail (status_bar->priv != NULL, NULL);

        g_signal_connect_object (mpd,
                                 "playlist_changed",
                                 G_CALLBACK (ario_status_bar_playlist_changed_cb),
                                 status_bar, 0);
        g_signal_connect_object (mpd,
                                 "updatingdb_changed",
                                 G_CALLBACK (ario_status_bar_playlist_changed_cb),
                                 status_bar, 0);
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

        ario_playlist_length = ario_mpd_get_current_playlist_length ();

        ario_playlist_total_time = ario_mpd_get_current_playlist_total_time ();
        formated_total_time = ario_util_format_total_time (ario_playlist_total_time);

        msg = g_strdup_printf ("%d %s - %s", ario_playlist_length, _("Songs"), formated_total_time);
        g_free (formated_total_time);

        if (ario_mpd_get_updating ()) {
                tmp = g_strdup_printf ("%s - %s", msg, _("Updating..."));
                g_free (msg);
                msg = tmp;
        }                

        gtk_statusbar_pop (GTK_STATUSBAR(status_bar), status_bar->priv->ario_playlist_context_id);
        gtk_statusbar_push (GTK_STATUSBAR (status_bar), status_bar->priv->ario_playlist_context_id, msg);

        g_free (msg);
}
