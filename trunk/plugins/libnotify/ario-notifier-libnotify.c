/*
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "ario-notifier-libnotify.h"
#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "servers/ario-server.h"
#include "ario-util.h"
#include "widgets/ario-tray-icon.h"
#include <libnotify/notify.h>
#include "covers/ario-cover-handler.h"
#include "preferences/ario-preferences.h"
#include "lib/ario-conf.h"
#include "ario-debug.h"

struct ArioNotifierLibnotifyPrivate
{
        NotifyNotification *notification;
};

#define ARIO_NOTIFIER_LIBNOTIFY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_NOTIFIER_LIBNOTIFY, ArioNotifierLibnotifyPrivate))
G_DEFINE_TYPE (ArioNotifierLibnotify, ario_notifier_libnotify, ARIO_TYPE_NOTIFIER)

static gchar *
ario_notifier_libnotify_get_id (ArioNotifier *notifier)
{
        return "libnotify";
}

static gchar *
ario_notifier_libnotify_get_name (ArioNotifier *notifier)
{
        return "Libnotify";
}

static void
ario_notifier_libnotify_set_string_property (ArioNotifierLibnotify *notifier_libnotify,
                                             const gchar *prop,
                                             const gchar *str)
{
        ARIO_LOG_FUNCTION_START;
        g_object_set (G_OBJECT (notifier_libnotify->priv->notification), prop, str, NULL);
}

static void
ario_notifier_libnotify_notify (ArioNotifier *notifier)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierLibnotify *notifier_libnotify = ARIO_NOTIFIER_LIBNOTIFY (notifier);
        gchar *title;
        gchar *artist;
        gchar *album;
        gchar *secondary;

        switch (ario_server_get_current_state ()) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Title */
                title = ario_util_format_title (ario_server_get_current_song ());
                ario_notifier_libnotify_set_string_property (notifier_libnotify, "summary", title);

                /* Artist - Album */
                artist = ario_server_get_current_artist ();
                album = ario_server_get_current_album ();

                if (!artist)
                        artist = ARIO_SERVER_UNKNOWN;
                if (!album)
                        album = ARIO_SERVER_UNKNOWN;

                secondary = TRAY_ICON_FROM_MARKUP (album, artist);
                ario_notifier_libnotify_set_string_property (notifier_libnotify, "body", secondary);
                g_free(secondary);
                break;
        default:
                /* Title */
                ario_notifier_libnotify_set_string_property (notifier_libnotify, "summary", TRAY_ICON_DEFAULT_TOOLTIP);

                /* Artist - Album */
                ario_notifier_libnotify_set_string_property (notifier_libnotify, "body", NULL);
                break;
        }

        notify_notification_set_timeout (notifier_libnotify->priv->notification,
                                         ario_conf_get_integer (PREF_NOTIFICATION_TIME, PREF_NOTIFICATION_TIME_DEFAULT) * 1000);
        notify_notification_show (notifier_libnotify->priv->notification, NULL);
}

static void
ario_notifier_libnotify_class_init (ArioNotifierLibnotifyClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierClass *notifier_class = ARIO_NOTIFIER_CLASS (klass);

        notifier_class->get_id = ario_notifier_libnotify_get_id;
        notifier_class->get_name = ario_notifier_libnotify_get_name;
        notifier_class->notify = ario_notifier_libnotify_notify;

        g_type_class_add_private (klass, sizeof (ArioNotifierLibnotifyPrivate));
}

static void
ario_notifier_libnotify_init (ArioNotifierLibnotify *notifier_libnotify)
{
        ARIO_LOG_FUNCTION_START;
        notifier_libnotify->priv = ARIO_NOTIFIER_LIBNOTIFY_GET_PRIVATE (notifier_libnotify);

        notifier_libnotify->priv->notification = notify_notification_new ("Ario",  NULL, NULL, NULL);
        notify_notification_attach_to_status_icon (notifier_libnotify->priv->notification,
                                                   GTK_STATUS_ICON (ario_tray_icon_get_instance ()));
}

static void
ario_notifier_libnotify_cover_changed_cb (ArioCoverHandler *cover_handler,
                                          ArioNotifierLibnotify *notifier_libnotify)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *id;

        id = ario_conf_get_string (PREF_NOTIFIER, PREF_NOTIFIER_DEFAULT);
        if (id &&
           !g_utf8_collate (id, ario_notifier_libnotify_get_id (ARIO_NOTIFIER (notifier_libnotify)))) {
                ario_notifier_libnotify_set_string_property (notifier_libnotify, "icon-name", ario_cover_handler_get_cover_path ());
                notify_notification_show (notifier_libnotify->priv->notification, NULL);
        }
}

ArioNotifier*
ario_notifier_libnotify_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierLibnotify *libnotify;

        if (!notify_is_initted () && !notify_init ("ario"))
                return NULL;

        libnotify = g_object_new (TYPE_ARIO_NOTIFIER_LIBNOTIFY, NULL);

        g_signal_connect_object (ario_cover_handler_get_instance (),
                                 "cover_changed",
                                 G_CALLBACK (ario_notifier_libnotify_cover_changed_cb),
                                 libnotify, 0);

        return ARIO_NOTIFIER (libnotify);
}

