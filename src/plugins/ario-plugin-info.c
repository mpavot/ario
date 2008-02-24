/*
 * heavily based on code from Gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi
 * Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <glib/gkeyfile.h>

#include "plugins/ario-plugin-info.h"
#include "plugins/ario-plugin-info-priv.h"
#include "ario-debug.h"
#include "plugins/ario-plugin.h"

void
_ario_plugin_info_ref (ArioPluginInfo *info)
{
        g_atomic_int_inc (&info->refcount);
}

static ArioPluginInfo *
ario_plugin_info_copy (ArioPluginInfo *info)
{
        _ario_plugin_info_ref (info);
        return info;
}

void
_ario_plugin_info_unref (ArioPluginInfo *info)
{
        if (!g_atomic_int_dec_and_test (&info->refcount))
                return;

        if (info->plugin != NULL) {
                ARIO_LOG_DBG ("Unref plugin %s", info->name);

                g_object_unref (info->plugin);

                /* info->module must not be unref since it is not possible to finalize 
                 * a type module */
        }

        g_free (info->file);
        g_free (info->module_name);
        g_strfreev (info->dependencies);
        g_free (info->name);
        g_free (info->desc);
        g_free (info->icon_name);
        g_free (info->website);
        g_free (info->copyright);
        g_strfreev (info->authors);

        g_free (info);
}

/**
 * ario_plugin_info_get_type:
 *
 * Retrieves the #GType object which is associated with the #ArioPluginInfo
 * class.
 *
 * Return value: the GType associated with #ArioPluginInfo.
 **/
GType
ario_plugin_info_get_type (void)
{
        static GType the_type = 0;

        if (G_UNLIKELY (!the_type))
                the_type = g_boxed_type_register_static (
                                                         "ArioPluginInfo",
                                                         (GBoxedCopyFunc) ario_plugin_info_copy,
                                                         (GBoxedFreeFunc) _ario_plugin_info_unref);

        return the_type;
} 

/**
 * ario_plugin_info_new:
 * @filename: the filename where to read the plugin information
 *
 * Creates a new #ArioPluginInfo from a file on the disk.
 *
 * Return value: a newly created #ArioPluginInfo.
 */
ArioPluginInfo *
_ario_plugin_info_new (const gchar *file)
{
        ArioPluginInfo *info;
        GKeyFile *plugin_file = NULL;
        gchar *str;

        g_return_val_if_fail (file != NULL, NULL);

        ARIO_LOG_DBG ("Loading plugin: %s", file);

        info = g_new0 (ArioPluginInfo, 1);
        info->refcount = 1;
        info->file = g_strdup (file);

        plugin_file = g_key_file_new ();
        if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL)) {
                g_warning ("Bad plugin file: %s", file);
                goto error;
        }

        if (!g_key_file_has_key (plugin_file,
                                 "Ario Plugin",
                                 "IAge",
                                 NULL)) {
                ARIO_LOG_DBG ("IAge key does not exist in file: %s", file);
                goto error;
        }

        /* Check IAge=1 */
        if (g_key_file_get_integer (plugin_file,
                                    "Ario Plugin",
                                    "IAge",
                                    NULL) != 1) {
                ARIO_LOG_DBG ("Wrong IAge in file: %s", file);
                goto error;
        }

        /* Get module name */
        str = g_key_file_get_string (plugin_file,
                                     "Ario Plugin",
                                     "Module",
                                     NULL);

        if ((str != NULL) && (*str != '\0')) {
                info->module_name = str;
        } else {
                g_warning ("Could not find 'Module' in %s", file);
                goto error;
        }

        /* Get the dependency list */
        info->dependencies = g_key_file_get_string_list (plugin_file,
                                                         "Ario Plugin",
                                                         "Depends",
                                                         NULL,
                                                         NULL);
        if (info->dependencies == NULL) {
                ARIO_LOG_DBG ("Could not find 'Depends' in %s", file);
                info->dependencies = g_new0 (gchar *, 1);
        }

        /* Get the loader for this plugin */
        str = g_key_file_get_string (plugin_file,
                                     "Ario Plugin",
                                     "Loader",
                                     NULL);
        if (str && strcmp(str, "python") == 0) {
                info->loader = ARIO_PLUGIN_LOADER_PY;
#ifndef ENABLE_PYTHON
                g_warning ("Cannot load Python plugin '%s' since ario was not "
                           "compiled with Python support.", file);
                goto error;
#endif
        } else {
                info->loader = ARIO_PLUGIN_LOADER_C;
        }
        g_free (str);

        /* Get Name */
        str = g_key_file_get_locale_string (plugin_file,
                                            "Ario Plugin",
                                            "Name",
                                            NULL, NULL);
        if (str) {
                info->name = str;
        } else {
                g_warning ("Could not find 'Name' in %s", file);
                goto error;
        }

        /* Get Description */
        str = g_key_file_get_locale_string (plugin_file,
                                            "Ario Plugin",
                                            "Description",
                                            NULL, NULL);
        if (str) {
                info->desc = str;
        } else {
                ARIO_LOG_DBG ("Could not find 'Description' in %s", file);
        }

        /* Get Icon */
        str = g_key_file_get_locale_string (plugin_file,
                                            "Ario Plugin",
                                            "Icon",
                                            NULL, NULL);
        if (str) {
                info->icon_name = str;
        } else {
                ARIO_LOG_DBG ("Could not find 'Icon' in %s, using 'ario-plugin'", file);
        }

        /* Get Authors */
        info->authors = g_key_file_get_string_list (plugin_file,
                                                    "Ario Plugin",
                                                    "Authors",
                                                    NULL,
                                                    NULL);
        if (info->authors == NULL)
                ARIO_LOG_DBG ("Could not find 'Authors' in %s", file);


        /* Get Copyright */
        str = g_key_file_get_string (plugin_file,
                                     "Ario Plugin",
                                     "Copyright",
                                     NULL);
        if (str) {
                info->copyright = str;
        } else {
                ARIO_LOG_DBG ("Could not find 'Copyright' in %s", file);
        }

        /* Get Website */
        str = g_key_file_get_string (plugin_file,
                                     "Ario Plugin",
                                     "Website",
                                     NULL);
        if (str) {
                info->website = str;
        } else {
                ARIO_LOG_DBG ("Could not find 'Website' in %s", file);
        }

        g_key_file_free (plugin_file);

        /* If we know nothing about the availability of the plugin,
           set it as available */
        info->available = TRUE;

        return info;

error:
        g_free (info->file);
        g_free (info->module_name);
        g_free (info->name);
        g_free (info);
        g_key_file_free (plugin_file);

        return NULL;
}

gboolean
ario_plugin_info_is_active (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, FALSE);

        return info->available && info->active;
}

gboolean
ario_plugin_info_is_available (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, FALSE);

        return info->available != FALSE;
}

gboolean
ario_plugin_info_is_configurable (ArioPluginInfo *info)
{
        ARIO_LOG_DBG ("Is '%s' configurable?", info->name);

        g_return_val_if_fail (info != NULL, FALSE);

        if (info->plugin == NULL || !info->active || !info->available)
                return FALSE;

        return ario_plugin_is_configurable (info->plugin);
}

const gchar *
ario_plugin_info_get_name (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, NULL);

        return info->name;
}

const gchar *
ario_plugin_info_get_description (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, NULL);

        return info->desc;
}

const gchar *
ario_plugin_info_get_icon_name (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, NULL);

        /* use the ario icon as a default if the plugin does not
           have its own */

        if (info->icon_name != NULL && 
            gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
                                     info->icon_name))
                return info->icon_name;
        else
                return "ario";
}

const gchar **
ario_plugin_info_get_authors (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, (const gchar **)NULL);

        return (const gchar **) info->authors;
}

const gchar *
ario_plugin_info_get_website (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, NULL);

        return info->website;
}

const gchar *
ario_plugin_info_get_copyright (ArioPluginInfo *info)
{
        g_return_val_if_fail (info != NULL, NULL);

        return info->copyright;
}
