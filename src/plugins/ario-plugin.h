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

#ifndef __ARIO_PLUGIN_H__
#define __ARIO_PLUGIN_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <shell/ario-shell.h>
#include <ario-debug.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_PLUGIN              (ario_plugin_get_type())
#define ARIO_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_PLUGIN, ArioPlugin))
#define ARIO_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_PLUGIN, ArioPluginClass))
#define ARIO_IS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_PLUGIN))
#define ARIO_IS_PLUGIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_PLUGIN))
#define ARIO_PLUGIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_PLUGIN, ArioPluginClass))

/*
 * Main object structure
 */
typedef struct _ArioPlugin ArioPlugin;

struct _ArioPlugin
{
        GObject parent;
};

/*
 * Class definition
 */
typedef struct _ArioPluginClass ArioPluginClass;

struct _ArioPluginClass
{
        GObjectClass parent_class;

        /* Virtual public methods */

        void            (*activate)                     (ArioPlugin *plugin,
                                                         ArioShell *shell);
        void            (*deactivate)                   (ArioPlugin *plugin,
                                                         ArioShell *shell);

        GtkWidget       *(*create_configure_dialog)     (ArioPlugin *plugin);

        /* Plugins should not override this, it's handled automatically by
           the ArioPluginClass */
        gboolean        (*is_configurable)              (ArioPlugin *plugin);

        /* Padding for future expansion */
        void            (*_ario_reserved1)              (void);
        void            (*_ario_reserved2)              (void);
        void            (*_ario_reserved3)              (void);
};

/*
 * Public methods
 */
G_MODULE_EXPORT
GType           ario_plugin_get_type                    (void) G_GNUC_CONST;

void            ario_plugin_activate                    (ArioPlugin *plugin,
                                                         ArioShell *shell);
void            ario_plugin_deactivate                  (ArioPlugin *plugin,
                                                         ArioShell *shell);

gboolean        ario_plugin_is_configurable             (ArioPlugin *plugin);

GtkWidget*      ario_plugin_create_configure_dialog     (ArioPlugin *plugin);
G_MODULE_EXPORT
GSList *        ario_plugin_get_plugin_paths            (void);
G_MODULE_EXPORT
GSList *        ario_plugin_get_plugin_data_paths       (void);
G_MODULE_EXPORT
char *          ario_plugin_find_file                   (const char *file);

/*
 * Utility macro used to register plugins
 *
 * use: ARIO_PLUGIN_REGISTER_TYPE(PluginName, plugin_name, CODE)
 */
#define ARIO_PLUGIN_REGISTER_TYPE(PluginName, plugin_name, CODE)                \
G_DEFINE_DYNAMIC_TYPE_EXTENDED (PluginName,                                     \
                                plugin_name,                                    \
                                ARIO_TYPE_PLUGIN,                               \
                                0,                                              \
                                CODE)                                           \
                                                                                \
GType register_ario_plugin (GTypeModule *module)                                \
{                                                                               \
        plugin_name##_register_type (module);                                   \
        return plugin_name##_type_id;                                           \
}                                                                               \
                                                                                \
static void                                                                     \
plugin_name##_class_finalize (PluginName##Class *klass)                         \
{                                                                               \
}

G_END_DECLS

#endif  /* __ARIO_PLUGIN_H__ */


