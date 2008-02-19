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

G_BEGIN_DECLS

#define ARIO_TYPE_PLUGINS_ENGINE              (ario_plugins_engine_get_type ())
#define ARIO_PLUGINS_ENGINE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_PLUGINS_ENGINE, ArioPluginsEngine))
#define ARIO_PLUGINS_ENGINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_PLUGINS_ENGINE, ArioPluginsEngineClass))
#define ARIO_IS_PLUGINS_ENGINE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_PLUGINS_ENGINE))
#define ARIO_IS_PLUGINS_ENGINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_PLUGINS_ENGINE))
#define ARIO_PLUGINS_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_PLUGINS_ENGINE, ArioPluginsEngineClass))

typedef struct _ArioPluginsEngine               ArioPluginsEngine;
typedef struct _ArioPluginsEnginePrivate        ArioPluginsEnginePrivate;

struct _ArioPluginsEngine
{
        GObject parent;
        ArioPluginsEnginePrivate *priv;
};

typedef struct _ArioPluginsEngineClass          ArioPluginsEngineClass;

struct _ArioPluginsEngineClass
{
        GObjectClass parent_class;

        void         (* activate_plugin)        (ArioPluginsEngine *engine,
                                                 ArioPluginInfo    *info);

        void         (* deactivate_plugin)      (ArioPluginsEngine *engine,
                                                 ArioPluginInfo    *info);
};

GType                   ario_plugins_engine_get_type            (void) G_GNUC_CONST;

void                    ario_plugins_engine_set_shell           (ArioShell *shell);

ArioPluginsEngine       *ario_plugins_engine_get_default        (void);

void                    ario_plugins_engine_garbage_collect     (ArioPluginsEngine *engine);

const GList             *ario_plugins_engine_get_plugin_list    (ArioPluginsEngine *engine);

gboolean                ario_plugins_engine_activate_plugin     (ArioPluginsEngine *engine,
                                                                 ArioPluginInfo    *info);
gboolean                ario_plugins_engine_deactivate_plugin   (ArioPluginsEngine *engine,
                                                                 ArioPluginInfo    *info);

void                    ario_plugins_engine_configure_plugin    (ArioPluginsEngine *engine,
                                                                 ArioPluginInfo    *info,
                                                                 GtkWindow          *parent);

/* 
 * new_window == TRUE if this function is called because a new top window
 * has been created
 */
void                    ario_plugins_engine_update_plugins_ui   (ArioPluginsEngine *engine,
                                                                 ArioShell        *shell, 
                                                                 gboolean          new_shell);

G_END_DECLS

#endif  /* __ARIO_PLUGINS_ENGINE_H__ */
