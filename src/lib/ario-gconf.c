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

#include <config.h>
#include <stdlib.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf.h>
#include <glib/gerror.h>

#include "lib/ario-conf.h"

#define ARIO_GCONF_UNDEFINED_CONNECTION 0

static GConfClient *ario_conf_client_get_global (void);
static gboolean ario_conf_handle_error (GError **error);
static gboolean ario_conf_monitor_add (const char *directory);
static gboolean ario_conf_monitor_remove (const char *directory);

static GConfClient *global_gconf_client = NULL;

static void
global_client_free (void)
{
        if (global_gconf_client == NULL) {
                return;
        }

        g_object_unref (G_OBJECT (global_gconf_client));
        global_gconf_client = NULL;
}

/* Public */
static GConfClient *
ario_conf_client_get_global (void)
{
        if (global_gconf_client == NULL) {
                global_gconf_client = gconf_client_get_default ();
                g_atexit (global_client_free);
        }
        
        return global_gconf_client;
}

static gboolean
ario_conf_handle_error (GError **error)
{
        g_return_val_if_fail (error != NULL, FALSE);

        if (*error != NULL) {
                g_warning ((*error)->message);
                g_error_free (*error);
                *error = NULL;

                return TRUE;
        }

        return FALSE;
}

static gboolean
ario_conf_is_set (const char *key)
{
        gboolean result = TRUE;
        GConfClient *client;
        GError *error = NULL;
        GConfValue *value;

        g_return_val_if_fail (key != NULL, FALSE);
        
        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, FALSE);
        
        value = gconf_client_get (client, key, &error);
        
        if (ario_conf_handle_error (&error) || !value) {
                result = FALSE;
        }

        if (value) {
                gconf_value_free (value);
        }
        
        return result;
}

void
ario_conf_set_boolean (const char *key,
                            gboolean boolean_value)
{
        GConfClient *client;
        GError *error = NULL;
        
        g_return_if_fail (key != NULL);

        client = ario_conf_client_get_global ();
        g_return_if_fail (client != NULL);
        
        gconf_client_set_bool (client, key, boolean_value, &error);
        ario_conf_handle_error (&error);
}

gboolean
ario_conf_get_boolean (const char *key,
                       const gboolean default_value)
{
        gboolean result;
        GConfClient *client;
        GError *error = NULL;
        
        g_return_val_if_fail (key != NULL, FALSE);
        
        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, FALSE);
        
        result = gconf_client_get_bool (client, key, &error);
        
        if (ario_conf_handle_error (&error) || !ario_conf_is_set (key)) {
                result = default_value;
        }
        
        return result;
}

void
ario_conf_set_integer (const char *key,
                            int int_value)
{
        GConfClient *client;
        GError *error = NULL;

        g_return_if_fail (key != NULL);

        client = ario_conf_client_get_global ();
        g_return_if_fail (client != NULL);

        gconf_client_set_int (client, key, int_value, &error);
        ario_conf_handle_error (&error);
}

int
ario_conf_get_integer (const char *key,
                       const int default_value)
{
        int result;
        GConfClient *client;
        GError *error = NULL;
        
        g_return_val_if_fail (key != NULL, 0);
        
        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, 0);
        
        result = gconf_client_get_int (client, key, &error);

        if (ario_conf_handle_error (&error) || !ario_conf_is_set (key)) {
                result = default_value;
        }

        return result;
}

void
ario_conf_set_float (const char *key,
                            gfloat float_value)
{
        GConfClient *client;
        GError *error = NULL;

        g_return_if_fail (key != NULL);

        client = ario_conf_client_get_global ();
        g_return_if_fail (client != NULL);

        gconf_client_set_float (client, key, float_value, &error);
        ario_conf_handle_error (&error);
}

gfloat
ario_conf_get_float (const char *key,
                     const gfloat default_value)
{
        gfloat result;
        GConfClient *client;
        GError *error = NULL;
        
        g_return_val_if_fail (key != NULL, 0);
        
        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, 0);
        
        result = gconf_client_get_float (client, key, &error);

        if (ario_conf_handle_error (&error) || !ario_conf_is_set (key)) {
                result = default_value;
        }

        return result;
}

void
ario_conf_set_string (const char *key,
                           const char *string_value)
{
        GConfClient *client;
        GError *error = NULL;

        g_return_if_fail (key != NULL);
        g_return_if_fail (string_value != NULL);

        client = ario_conf_client_get_global ();
        g_return_if_fail (client != NULL);
        
        gconf_client_set_string (client, key, string_value, &error);
        ario_conf_handle_error (&error);
}

char *
ario_conf_get_string (const char *key,
                      const char *default_value)
{
        char *result;
        GConfClient *client;
        GError *error = NULL;
        
        g_return_val_if_fail (key != NULL, NULL);
        
        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, NULL);
        
        result = gconf_client_get_string (client, key, &error);
        
        if (ario_conf_handle_error (&error) || !ario_conf_is_set (key)) {
                if (default_value) {
                        result = g_strdup (default_value);
                } else {
                        result = g_strdup ("");
                }
        }
        
        return result;
}

void
ario_conf_set_string_slist (const char *key,
                                const GSList *slist)
{
        GConfClient *client;
        GError *error;

        g_return_if_fail (key != NULL);

        client = ario_conf_client_get_global ();
        g_return_if_fail (client != NULL);

        error = NULL;
        gconf_client_set_list (client, key, GCONF_VALUE_STRING,
                               /* Need cast cause of GConf api bug */
                               (GSList *) slist,
                               &error);
        ario_conf_handle_error (&error);
}

GSList *
ario_conf_get_string_slist (const char *key)
{
        GSList *slist;
        GConfClient *client;
        GError *error;
        
        g_return_val_if_fail (key != NULL, NULL);
        
        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, NULL);
        
        error = NULL;
        slist = gconf_client_get_list (client, key, GCONF_VALUE_STRING, &error);
        if (ario_conf_handle_error (&error)) {
                slist = NULL;
        }

        return slist;
}

static gboolean
ario_conf_monitor_add (const char *directory)
{
        GError *error = NULL;
        GConfClient *client;

        g_return_val_if_fail (directory != NULL, FALSE);

        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, FALSE);

        gconf_client_add_dir (client,
                              directory,
                              GCONF_CLIENT_PRELOAD_NONE,
                              &error);
        
        if (ario_conf_handle_error (&error)) {
                return FALSE;
        }

        return TRUE;
}

static gboolean
ario_conf_monitor_remove (const char *directory)
{
        GError *error = NULL;
        GConfClient *client;

        if (directory == NULL) {
                return FALSE;
        }

        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, FALSE);
        
        gconf_client_remove_dir (client,
                                 directory,
                                 &error);

        if (ario_conf_handle_error (&error)) {
                return FALSE;
        }

        return TRUE;
}

guint
ario_conf_notification_add (const char *key,
                            GConfClientNotifyFunc notification_callback,
                            gpointer callback_data)
{
        guint notification_id;
        GConfClient *client;
        GError *error = NULL;

        g_return_val_if_fail (key != NULL, ARIO_GCONF_UNDEFINED_CONNECTION);
        g_return_val_if_fail (notification_callback != NULL, ARIO_GCONF_UNDEFINED_CONNECTION);

        client = ario_conf_client_get_global ();
        g_return_val_if_fail (client != NULL, ARIO_GCONF_UNDEFINED_CONNECTION);

        notification_id = gconf_client_notify_add (client,
                                                   key,
                                                   notification_callback,
                                                   callback_data,
                                                   NULL,
                                                   &error);

        if (ario_conf_handle_error (&error)) {
                if (notification_id != ARIO_GCONF_UNDEFINED_CONNECTION) {
                        gconf_client_notify_remove (client, notification_id);
                        notification_id = ARIO_GCONF_UNDEFINED_CONNECTION;
                }
        }

        return notification_id;
}

void
ario_conf_notification_remove (guint notification_id)
{
        GConfClient *client;

        if (notification_id == ARIO_GCONF_UNDEFINED_CONNECTION) {
                return;
        }

        client = ario_conf_client_get_global ();
        g_return_if_fail (client != NULL);

        gconf_client_notify_remove (client, notification_id);
}

void
ario_conf_init (void)
{
        ario_conf_monitor_add ("/apps/ario");
}

void
ario_conf_shutdown (void)
{
        ario_conf_monitor_remove ("/apps/ario");
}


