/*
 * Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_LIBNOTIFY_PLUGIN_H__
#define __ARIO_LIBNOTIFY_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <ario-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_LIBNOTIFY_PLUGIN                 (ario_libnotify_plugin_get_type ())
#define ARIO_LIBNOTIFY_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_LIBNOTIFY_PLUGIN, ArioLibnotifyPlugin))
#define ARIO_LIBNOTIFY_PLUGIN_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_LIBNOTIFY_PLUGIN, ArioLibnotifyPluginClass))
#define ARIO_IS_LIBNOTIFY_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_LIBNOTIFY_PLUGIN))
#define ARIO_IS_LIBNOTIFY_PLUGIN_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_LIBNOTIFY_PLUGIN))
#define ARIO_LIBNOTIFY_PLUGIN_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_LIBNOTIFY_PLUGIN, ArioLibnotifyPluginClass))

/* Private structure type */
typedef struct _ArioLibnotifyPluginPrivate        ArioLibnotifyPluginPrivate;

/*
 * Main object structure
 */
typedef struct _ArioLibnotifyPlugin                ArioLibnotifyPlugin;

struct _ArioLibnotifyPlugin
{
        ArioPlugin parent_instance;

        /*< private >*/
        ArioLibnotifyPluginPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _ArioLibnotifyPluginClass        ArioLibnotifyPluginClass;

struct _ArioLibnotifyPluginClass
{
        ArioPluginClass parent_class;
};

/*
 * Public methods
 */
GType        ario_libnotify_plugin_get_type                (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_ario_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __ARIO_LIBNOTIFY_PLUGIN_H__ */

