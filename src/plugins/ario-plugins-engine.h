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

#ifndef __ARIO_PLUGINS_ENGINE_H__
#define __ARIO_PLUGINS_ENGINE_H__

#include <glib.h>
#include "shell/ario-shell.h"
#include "plugins/ario-plugin-info.h"
#include "plugins/ario-plugin.h"

void                    ario_plugins_engine_init                (ArioShell *shell);

void                    ario_plugins_engine_shutdown            (void);

const GList             *ario_plugins_engine_get_plugin_list    (void);

gboolean                ario_plugins_engine_activate_plugin     (ArioPluginInfo    *info);
gboolean                ario_plugins_engine_deactivate_plugin   (ArioPluginInfo    *info);

void                    ario_plugins_engine_configure_plugin    (ArioPluginInfo    *info,
                                                                 GtkWindow          *parent);

#endif  /* __ARIO_PLUGINS_ENGINE_H__ */
