/*
   Heavily based on eel-gconf-extension
   Copyright (C) 2000, 2001 Eazel, Inc.

   Modified by:
   Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef ARIO_CONF_H
#define ARIO_CONF_H

#include <config.h>

#ifdef ENABLE_GCONF
#include <gconf/gconf-client.h>
typedef GConfClientNotifyFunc ArioNotifyFunc;
#else
typedef void    (*ArioNotifyFunc)               (gpointer do_not_use1,
                                                 guint notification_id,
                                                 gpointer do_not_use2,
                                                 gpointer user_data);
#endif

void            ario_conf_set_boolean           (const char             *key,
                                                 gboolean                boolean_value);
gboolean        ario_conf_get_boolean           (const char             *key,
                                                 const gboolean          default_value);
int             ario_conf_get_integer           (const char             *key,
                                                 const int               default_value);
void            ario_conf_set_integer           (const char             *key,
                                                 int                     int_value);
gfloat          ario_conf_get_float             (const char             *key,
                                                 const gfloat            default_value);
void            ario_conf_set_float             (const char             *key,
                                                 gfloat                 float_value);
char *          ario_conf_get_string            (const char             *key,
                                                 const char             *default_value);
void            ario_conf_set_string            (const char             *key,
                                                 const char             *string_value);
GSList *        ario_conf_get_string_slist      (const char             *key);
void            ario_conf_set_string_slist      (const char             *key,
                                                 const GSList           *string_slist_value);
void            ario_conf_init                  (void);
void            ario_conf_shutdown              (void);
guint           ario_conf_notification_add      (const char *key,
                                                 ArioNotifyFunc notification_callback,
                                                 gpointer callback_data);
void            ario_conf_notification_remove   (guint notification_id);

#endif /* ARIO_CONF_H */
