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

#include "config.h"

#include "plugins/ario-module.h"
#include "ario-debug.h"

#include <gmodule.h>

typedef struct _ArioModuleClass ArioModuleClass;

struct _ArioModuleClass
{
        GTypeModuleClass parent_class;
};

struct _ArioModule
{
        GTypeModule parent_instance;

        GModule *library;

        gchar *path;
        GType type;
};

typedef GType (*ArioModuleRegisterFunc) (GTypeModule *);

G_DEFINE_TYPE (ArioModule, ario_module, G_TYPE_TYPE_MODULE)

static gboolean
ario_module_load (GTypeModule *gmodule)
{
        ArioModule *module = ARIO_MODULE (gmodule);
        ArioModuleRegisterFunc register_func;

        ARIO_LOG_DBG ("Loading %s", module->path);

        module->library = g_module_open (module->path, 0);

        if (module->library == NULL) {
                g_warning ("%s", g_module_error());

                return FALSE;
        }

        /* extract symbols from the lib */
        if (!g_module_symbol (module->library, "register_ario_plugin",
                              (void *) &register_func)) {
                g_warning ("%s", g_module_error());
                g_module_close (module->library);

                return FALSE;
        }

        /* symbol can still be NULL even though g_module_symbol
         * returned TRUE */
        if (register_func == NULL) {
                g_warning ("Symbol 'register_ario_plugin' should not be NULL");
                g_module_close (module->library);

                return FALSE;
        }

        module->type = register_func (gmodule);

        if (module->type == 0) {
                g_warning ("Invalid ario plugin contained by module %s", module->path);
                return FALSE;
        }

        return TRUE;
}

static void
ario_module_unload (GTypeModule *gmodule)
{
        ArioModule *module = ARIO_MODULE (gmodule);

        ARIO_LOG_DBG ("Unloading %s", module->path);

        g_module_close (module->library);

        module->library = NULL;
        module->type = 0;
}

const gchar *
ario_module_get_path (ArioModule *module)
{
        g_return_val_if_fail (ARIO_IS_MODULE (module), NULL);

        return module->path;
}

GObject *
ario_module_new_object (ArioModule *module)
{
        ARIO_LOG_DBG ("Creating object of type %s", g_type_name (module->type));

        if (module->type == 0) {
                return NULL;
        }

        return g_object_new (module->type, NULL);
}

static void
ario_module_init (ArioModule *module)
{
        ARIO_LOG_DBG ("ArioModule %p initialising", module);
}

static void
ario_module_finalize (GObject *object)
{
        ArioModule *module = ARIO_MODULE (object);

        ARIO_LOG_DBG ("ArioModule %p finalising", module);

        g_free (module->path);

        G_OBJECT_CLASS (ario_module_parent_class)->finalize (object);
}

static void
ario_module_class_init (ArioModuleClass *class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (class);
        GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

        object_class->finalize = ario_module_finalize;

        module_class->load = ario_module_load;
        module_class->unload = ario_module_unload;
}

ArioModule *
ario_module_new (const gchar *path)
{
        ArioModule *result;

        if (path == NULL || path[0] == '\0') {
                return NULL;
        }

        result = g_object_new (ARIO_TYPE_MODULE, NULL);

        g_type_module_set_name (G_TYPE_MODULE (result), path);
        result->path = g_strdup (path);

        return result;
}
