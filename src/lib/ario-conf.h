/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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


#ifndef ARIO_CONF_H
#define ARIO_CONF_H

#include <glib.h>
#include <gmodule.h>

typedef void    (*ArioNotifyFunc)               (guint notification_id,
                                                 gpointer user_data);

G_MODULE_EXPORT
void            ario_conf_set_boolean           (const char             *key,
                                                 gboolean                boolean_value);
G_MODULE_EXPORT
gboolean        ario_conf_get_boolean           (const char             *key,
                                                 const gboolean          default_value);
G_MODULE_EXPORT
int             ario_conf_get_integer           (const char             *key,
                                                 const int               default_value);
G_MODULE_EXPORT
void            ario_conf_set_integer           (const char             *key,
                                                 int                     int_value);
G_MODULE_EXPORT
gfloat          ario_conf_get_float             (const char             *key,
                                                 const gfloat            default_value);
G_MODULE_EXPORT
void            ario_conf_set_float             (const char             *key,
                                                 gfloat                 float_value);
G_MODULE_EXPORT
const char *    ario_conf_get_string            (const char             *key,
                                                 const char             *default_value);
G_MODULE_EXPORT
void            ario_conf_set_string            (const char             *key,
                                                 const char             *string_value);
G_MODULE_EXPORT
GSList *        ario_conf_get_string_slist      (const char             *key,
                                                 const char             *string_value);
G_MODULE_EXPORT
void            ario_conf_set_string_slist      (const char             *key,
                                                 const GSList           *string_slist_value);
G_MODULE_EXPORT
void            ario_conf_init                  (void);
G_MODULE_EXPORT
void            ario_conf_shutdown              (void);
G_MODULE_EXPORT
guint           ario_conf_notification_add      (const char *key,
                                                 ArioNotifyFunc notification_callback,
                                                 gpointer callback_data);
G_MODULE_EXPORT
void            ario_conf_notification_remove   (guint notification_id);

#endif /* ARIO_CONF_H */
