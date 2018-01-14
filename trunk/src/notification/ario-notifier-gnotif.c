/*
 *  Copyright (C) 2018 Marc Pavot <marc.pavot@gmail.com>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "notification/ario-notifier-gnotif.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "lib/ario-conf.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "covers/ario-cover-handler.h"
#include "preferences/ario-preferences.h"

struct ArioNotifierGnotifPrivate
{
        gint dummy;
};

#define ARIO_NOTIFIER_GNOTIF_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_NOTIFIER_GNOTIF, ArioNotifierGnotifPrivate))
G_DEFINE_TYPE (ArioNotifierGnotif, ario_notifier_gnotif, ARIO_TYPE_NOTIFIER)

static gchar *
ario_notifier_gnotif_get_id (ArioNotifier *notifier)
{
        return "gnotif";
}

static gchar *
ario_notifier_gnotif_get_name (ArioNotifier *notifier)
{
        return "GNotification";
}

static void
ario_notifier_gnotif_notify (ArioNotifier *notifier_parent)
{
        GNotification *notification;
        gchar *title = NULL;
        gchar *body = NULL;
        gchar *artist;
        gchar *album;
        GFile *file;
        GIcon *icon;
        gchar *cover_path;

        switch (ario_server_get_current_state ()) {
        case ARIO_STATE_PLAY:
        case ARIO_STATE_PAUSE:
                /* Title */
                title = ario_util_format_title (ario_server_get_current_song ());

                /* Artist - Album */
                artist = ario_server_get_current_artist ();
                album = ario_server_get_current_album ();

                if (!artist)
                        artist = ARIO_SERVER_UNKNOWN;
                if (!album)
                        album = ARIO_SERVER_UNKNOWN;

                body = g_markup_printf_escaped (_("<i>from</i> %s <i>by</i> %s"), album, artist);
                break;
        default:
                break;
        }

        if (!title)
                return;

        notification = g_notification_new (title);
        if (body) {
                g_notification_set_body (notification, body);
                g_free(body);
        }
        cover_path = ario_cover_handler_get_cover_path ();
        if (cover_path) {
                file = g_file_new_for_path (cover_path);
                if (file) {
                        icon = g_file_icon_new (file);
                        if (icon) {
                                g_notification_set_icon (notification, icon);
                                g_object_unref (icon);
                        }
                        g_object_unref (file);
                }
        }

        g_application_send_notification (g_application_get_default (), "ario-song", notification);
        g_object_unref (notification);
}

static void
ario_notifier_gnotif_class_init (ArioNotifierGnotifClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierClass *notifier_class = ARIO_NOTIFIER_CLASS (klass);

        notifier_class->get_id = ario_notifier_gnotif_get_id;
        notifier_class->get_name = ario_notifier_gnotif_get_name;
        notifier_class->notify = ario_notifier_gnotif_notify;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioNotifierGnotifPrivate));
}

static void
ario_notifier_gnotif_init (ArioNotifierGnotif *notifier_gnotif)
{
        ARIO_LOG_FUNCTION_START;

        notifier_gnotif->priv = ARIO_NOTIFIER_GNOTIF_GET_PRIVATE (notifier_gnotif);
}

ArioNotifier*
ario_notifier_gnotif_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierGnotif *local;

        local = g_object_new (TYPE_ARIO_NOTIFIER_GNOTIF, NULL);

        return ARIO_NOTIFIER (local);
}

