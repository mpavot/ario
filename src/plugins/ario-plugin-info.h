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

#ifndef __ARIO_PLUGIN_INFO_H__
#define __ARIO_PLUGIN_INFO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ARIO_TYPE_PLUGIN_INFO (ario_plugin_info_get_type ())
#define ARIO_PLUGIN_INFO(obj) ((ArioPluginInfo *) (obj))

typedef struct _ArioPluginInfo ArioPluginInfo;

GType           ario_plugin_info_get_type               (void) G_GNUC_CONST;

gboolean        ario_plugin_info_is_active              (ArioPluginInfo *info);
gboolean        ario_plugin_info_is_available           (ArioPluginInfo *info);
gboolean        ario_plugin_info_is_configurable        (ArioPluginInfo *info);

const gchar*    ario_plugin_info_get_name               (ArioPluginInfo *info);
const gchar*    ario_plugin_info_get_description        (ArioPluginInfo *info);
const gchar*    ario_plugin_info_get_icon_name          (ArioPluginInfo *info);
const gchar**   ario_plugin_info_get_authors            (ArioPluginInfo *info);
const gchar*    ario_plugin_info_get_website            (ArioPluginInfo *info);
const gchar*    ario_plugin_info_get_copyright          (ArioPluginInfo *info);

G_END_DECLS

#endif /* __ARIO_PLUGIN_INFO_H__ */

