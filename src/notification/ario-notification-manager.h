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

#ifndef __ARIO_NOTIFICATION_MANAGER_H
#define __ARIO_NOTIFICATION_MANAGER_H

#include <glib-object.h>
#include "notification/ario-notifier.h"

G_BEGIN_DECLS

#define TYPE_ARIO_NOTIFICATION_MANAGER         (ario_notification_manager_get_type ())
#define ARIO_NOTIFICATION_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_NOTIFICATION_MANAGER, ArioNotificationManager))
#define ARIO_NOTIFICATION_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_NOTIFICATION_MANAGER, ArioNotificationManagerClass))
#define IS_ARIO_NOTIFICATION_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_NOTIFICATION_MANAGER))
#define IS_ARIO_NOTIFICATION_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_NOTIFICATION_MANAGER))
#define ARIO_NOTIFICATION_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_NOTIFICATION_MANAGER, ArioNotificationManagerClass))

typedef struct ArioNotificationManagerPrivate ArioNotificationManagerPrivate;

typedef struct
{
        GObject parent;

        ArioNotificationManagerPrivate *priv;
} ArioNotificationManager;

typedef struct
{
        GObjectClass parent;
} ArioNotificationManagerClass;

GType                           ario_notification_manager_get_type              (void) G_GNUC_CONST;

ArioNotificationManager*        ario_notification_manager_get_instance          (void);

void                            ario_notification_manager_add_notifier          (ArioNotificationManager *notification_manager,
                                                                                 ArioNotifier *notifier);
void                            ario_notification_manager_remove_notifier       (ArioNotificationManager *notification_manager,
                                                                                 ArioNotifier *notifier);
GSList*                         ario_notification_manager_get_notifiers         (ArioNotificationManager *notification_manager);
ArioNotifier*                   ario_notification_manager_get_notifier_from_id  (ArioNotificationManager *notification_manager,
                                                                                 const gchar *id);
G_END_DECLS

#endif /* __ARIO_NOTIFICATION_MANAGER_H */
