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

#ifndef __ARIO_PLUGIN_MANAGER_H__
#define __ARIO_PLUGIN_MANAGER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_PLUGIN_MANAGER              (ario_plugin_manager_get_type())
#define ARIO_PLUGIN_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_PLUGIN_MANAGER, ArioPluginManager))
#define ARIO_PLUGIN_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_PLUGIN_MANAGER, ArioPluginManagerClass))
#define ARIO_IS_PLUGIN_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_PLUGIN_MANAGER))
#define ARIO_IS_PLUGIN_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_PLUGIN_MANAGER))
#define ARIO_PLUGIN_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_PLUGIN_MANAGER, ArioPluginManagerClass))

/* Private structure type */
typedef struct _ArioPluginManagerPrivate ArioPluginManagerPrivate;

/*
 * Main object structure
 */
typedef struct _ArioPluginManager ArioPluginManager;

struct _ArioPluginManager 
{
        GtkVBox vbox;

        /*< private > */
        ArioPluginManagerPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _ArioPluginManagerClass ArioPluginManagerClass;

struct _ArioPluginManagerClass 
{
        GtkVBoxClass parent_class;
};

/*
 * Public methods
 */
GType           ario_plugin_manager_get_type    (void) G_GNUC_CONST;

GtkWidget*      ario_plugin_manager_new         (void);

G_END_DECLS

#endif  /* __ARIO_PLUGIN_MANAGER_H__  */
