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

        void            (*update_ui)                    (ArioPlugin *plugin,
                                                         ArioShell *shell);

        GtkWidget       *(*create_configure_dialog)     (ArioPlugin *plugin);

        /* Plugins should not override this, it's handled automatically by
           the ArioPluginClass */
        gboolean        (*is_configurable)              (ArioPlugin *plugin);

        /* Padding for future expansion */
        void            (*_ario_reserved1)              (void);
        void            (*_ario_reserved2)              (void);
        void            (*_ario_reserved3)              (void);
        void            (*_ario_reserved4)              (void);
};

/*
 * Public methods
 */
GType           ario_plugin_get_type                    (void) G_GNUC_CONST;

void            ario_plugin_activate                    (ArioPlugin *plugin,
                                                         ArioShell *shell);
void            ario_plugin_deactivate                  (ArioPlugin *plugin,
                                                         ArioShell *shell);
                                 
void            ario_plugin_update_ui                   (ArioPlugin *plugin,
                                                         ArioShell *shell);

gboolean        ario_plugin_is_configurable             (ArioPlugin *plugin);

GtkWidget*      ario_plugin_create_configure_dialog     (ArioPlugin *plugin);

/*
 * Utility macro used to register plugins
 *
 * use: ARIO_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, CODE)
 */
#define ARIO_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, CODE)      \
                                                                                \
static GType plugin_name##_type = 0;                                            \
                                                                                \
GType                                                                           \
plugin_name##_get_type (void)                                                   \
{                                                                               \
        return plugin_name##_type;                                              \
}                                                                               \
                                                                                \
static void     plugin_name##_init              (PluginName        *self);      \
static void     plugin_name##_class_init        (PluginName##Class *klass);     \
static gpointer plugin_name##_parent_class = NULL;                              \
static void     plugin_name##_class_intern_init (gpointer klass)                \
{                                                                               \
        plugin_name##_parent_class = g_type_class_peek_parent (klass);          \
        plugin_name##_class_init ((PluginName##Class *) klass);                 \
}                                                                               \
                                                                                \
G_MODULE_EXPORT GType                                                           \
register_ario_plugin (GTypeModule *module)                                      \
{                                                                               \
        static const GTypeInfo our_info =                                       \
        {                                                                       \
                sizeof (PluginName##Class),                                     \
                NULL, /* base_init */                                           \
                NULL, /* base_finalize */                                       \
                (GClassInitFunc) plugin_name##_class_intern_init,               \
                NULL,                                                           \
                NULL, /* class_data */                                          \
                sizeof (PluginName),                                            \
                0, /* n_preallocs */                                            \
                (GInstanceInitFunc) plugin_name##_init                          \
        };                                                                      \
                                                                                \
        ARIO_LOG_DBG ("Registering " #PluginName);                              \
                                                                                \
        /* Initialise the i18n stuff */                                         \
        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);                           \
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");                     \
                                                                                \
        plugin_name##_type = g_type_module_register_type (module,               \
                                            ARIO_TYPE_PLUGIN,                   \
                                            #PluginName,                        \
                                            &our_info,                          \
                                            0);                                 \
                                                                                \
        CODE                                                                    \
                                                                                \
        return plugin_name##_type;                                              \
}

/*
 * Utility macro used to register plugins
 *
 * use: ARIO_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)
 */
#define ARIO_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)                        \
        ARIO_PLUGIN_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, ;)

/*
 * Utility macro used to register gobject types in plugins with additional code
 *
 * use: ARIO_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)
 */
#define ARIO_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)   \
                                                                                        \
static GType g_define_type_id = 0;                                                      \
                                                                                        \
GType                                                                                   \
object_name##_get_type (void)                                                           \
{                                                                                       \
        return g_define_type_id;                                                        \
}                                                                                       \
                                                                                        \
static void     object_name##_init              (ObjectName        *self);              \
static void     object_name##_class_init        (ObjectName##Class *klass);             \
static gpointer object_name##_parent_class = NULL;                                      \
static void     object_name##_class_intern_init (gpointer klass)                        \
{                                                                                       \
        object_name##_parent_class = g_type_class_peek_parent (klass);                  \
        object_name##_class_init ((ObjectName##Class *) klass);                         \
}                                                                                       \
                                                                                        \
GType                                                                                   \
object_name##_register_type (GTypeModule *module)                                       \
{                                                                                       \
        static const GTypeInfo our_info =                                               \
        {                                                                               \
                sizeof (ObjectName##Class),                                             \
                NULL, /* base_init */                                                   \
                NULL, /* base_finalize */                                               \
                (GClassInitFunc) object_name##_class_intern_init,                       \
                NULL,                                                                   \
                NULL, /* class_data */                                                  \
                sizeof (ObjectName),                                                    \
                0, /* n_preallocs */                                                    \
                (GInstanceInitFunc) object_name##_init                                  \
        };                                                                              \
                                                                                        \
        ARIO_LOG_DBG ("Registering " #ObjectName);                                      \
                                                                                        \
        g_define_type_id = g_type_module_register_type (module,                         \
                                                           PARENT_TYPE,                 \
                                                        #ObjectName,                    \
                                                        &our_info,                      \
                                                        0);                             \
                                                                                        \
        CODE                                                                            \
                                                                                        \
        return g_define_type_id;                                                        \
}

/*
 * Utility macro used to register gobject types in plugins
 *
 * use: ARIO_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)
 */
#define ARIO_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)                \
        ARIO_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, ;)

G_END_DECLS

#endif  /* __ARIO_PLUGIN_H__ */


