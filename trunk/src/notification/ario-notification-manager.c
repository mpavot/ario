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

#include "notification/ario-notification-manager.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "servers/ario-server.h"
#include "notification/ario-notifier-tooltip.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_notification_manager_song_changed_cb (ArioServer *server,
                                                       ArioNotificationManager *notification_manager);

struct ArioNotificationManagerPrivate
{
        GSList *notifiers;
};

#define ARIO_NOTIFICATION_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_NOTIFICATION_MANAGER, ArioNotificationManagerPrivate))
G_DEFINE_TYPE (ArioNotificationManager, ario_notification_manager, G_TYPE_OBJECT)

static void
ario_notification_manager_class_init (ArioNotificationManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioNotificationManagerPrivate));
}

static void
ario_notification_manager_init (ArioNotificationManager *notification_manager)
{
        ARIO_LOG_FUNCTION_START

        notification_manager->priv = ARIO_NOTIFICATION_MANAGER_GET_PRIVATE (notification_manager);
}

ArioNotificationManager *
ario_notification_manager_get_instance (void)
{
        ARIO_LOG_FUNCTION_START
        static ArioNotificationManager *notification_manager = NULL;
        ArioServer *server;

        if (!notification_manager) {
                ArioNotifier *notifier;

                notification_manager = g_object_new (TYPE_ARIO_NOTIFICATION_MANAGER,
                                                     NULL);
                g_return_val_if_fail (notification_manager->priv != NULL, NULL);

                notifier = ario_notifier_tooltip_new ();
                ario_notification_manager_add_notifier (notification_manager,
                                                        ARIO_NOTIFIER (notifier));

                server = ario_server_get_instance ();
                g_signal_connect_object (server,
                                         "song_changed",
                                         G_CALLBACK (ario_notification_manager_song_changed_cb),
                                         notification_manager, G_CONNECT_AFTER);

                g_signal_connect_object (server,
                                         "state_changed",
                                         G_CALLBACK (ario_notification_manager_song_changed_cb),
                                         notification_manager, G_CONNECT_AFTER);
        }

        return notification_manager;
}

static gint
ario_notification_manager_compare_notifiers (ArioNotifier *notifier,
                                             const gchar *id)
{
        return strcmp (ario_notifier_get_id (notifier), id);
}

GSList*
ario_notification_manager_get_notifiers (ArioNotificationManager *notification_manager)
{
        ARIO_LOG_FUNCTION_START
        return notification_manager->priv->notifiers;
}

ArioNotifier*
ario_notification_manager_get_notifier_from_id (ArioNotificationManager *notification_manager,
                                                const gchar *id)
{
        ARIO_LOG_FUNCTION_START
        GSList *found;

        found = g_slist_find_custom (notification_manager->priv->notifiers,
                                     id,
                                     (GCompareFunc) ario_notification_manager_compare_notifiers);

        if (!found)
                return NULL;

        return ARIO_NOTIFIER (found->data);
}

void
ario_notification_manager_add_notifier (ArioNotificationManager *notification_manager,
                                        ArioNotifier *notifier)
{
        ARIO_LOG_FUNCTION_START
        notification_manager->priv->notifiers = g_slist_append (notification_manager->priv->notifiers, notifier);
}

void
ario_notification_manager_remove_notifier (ArioNotificationManager *notification_manager,
                                           ArioNotifier *notifier)
{
        ARIO_LOG_FUNCTION_START
        notification_manager->priv->notifiers = g_slist_remove (notification_manager->priv->notifiers, notifier);
}

static void
ario_notification_manager_song_changed_cb (ArioServer *server,
                                           ArioNotificationManager *notification_manager)
{
        ARIO_LOG_FUNCTION_START
        gchar *id;
        ArioNotifier *notifier;

        if (ario_conf_get_boolean (PREF_HAVE_NOTIFICATION, PREF_HAVE_NOTIFICATION_DEFAULT)) {
                id = ario_conf_get_string (PREF_NOTIFIER, PREF_NOTIFIER_DEFAULT);
                notifier = ario_notification_manager_get_notifier_from_id (notification_manager, id);

                if (notifier)
                        ario_notifier_notify (notifier);
                g_free (id);
        }
}
