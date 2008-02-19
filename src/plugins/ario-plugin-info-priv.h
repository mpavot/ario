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

#ifndef __ARIO_PLUGIN_INFO_PRIV_H__
#define __ARIO_PLUGIN_INFO_PRIV_H__

#include "plugins/ario-plugin-info.h"
#include "plugins/ario-plugin.h"

typedef enum
{
        ARIO_PLUGIN_LOADER_C,
        ARIO_PLUGIN_LOADER_PY,
} ArioPluginLoader;

struct _ArioPluginInfo
{
        gint               refcount;

        gchar             *file;

        gchar             *module_name;
        ArioPluginLoader  loader;
        GTypeModule       *module;
        gchar            **dependencies;

        gchar             *name;
        gchar             *desc;
        gchar             *icon_name;
        gchar            **authors;
        gchar             *copyright;
        gchar             *website;

        ArioPlugin        *plugin;

        gboolean           active;

        /* A plugin is unavailable if it is not possible to activate it
           due to an error loading the plugin module (e.g. for Python plugins
           when the interpreter has not been correctly initializated) */
        gboolean           available;
};

ArioPluginInfo*         _ario_plugin_info_new           (const gchar *file);
void                    _ario_plugin_info_ref           (ArioPluginInfo *info);
void                    _ario_plugin_info_unref         (ArioPluginInfo *info);


#endif /* __ARIO_PLUGIN_INFO_PRIV_H__ */
