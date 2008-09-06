/*
 * Copyright (C) 2002-2005 - Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __ARIO_INFORMATION_PLUGIN_H__
#define __ARIO_INFORMATION_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <ario-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_INFORMATION_PLUGIN                 (ario_information_plugin_get_type ())
#define ARIO_INFORMATION_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_INFORMATION_PLUGIN, ArioInformationPlugin))
#define ARIO_INFORMATION_PLUGIN_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_INFORMATION_PLUGIN, ArioInformationPluginClass))
#define ARIO_IS_INFORMATION_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_INFORMATION_PLUGIN))
#define ARIO_IS_INFORMATION_PLUGIN_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_INFORMATION_PLUGIN))
#define ARIO_INFORMATION_PLUGIN_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_INFORMATION_PLUGIN, ArioInformationPluginClass))

/* Private structure type */
typedef struct _ArioInformationPluginPrivate        ArioInformationPluginPrivate;

/*
 * Main object structure
 */
typedef struct _ArioInformationPlugin                ArioInformationPlugin;

struct _ArioInformationPlugin
{
        ArioPlugin parent_instance;

        /*< private >*/
        ArioInformationPluginPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _ArioInformationPluginClass        ArioInformationPluginClass;

struct _ArioInformationPluginClass
{
        ArioPluginClass parent_class;
};

/*
 * Public methods
 */
GType        ario_information_plugin_get_type                (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_ario_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __ARIO_INFORMATION_PLUGIN_H__ */

