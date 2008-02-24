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

#ifndef ARIO_MODULE_H
#define ARIO_MODULE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define ARIO_TYPE_MODULE                (ario_module_get_type ())
#define ARIO_MODULE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARIO_TYPE_MODULE, ArioModule))
#define ARIO_MODULE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), ARIO_TYPE_MODULE, ArioModuleClass))
#define ARIO_IS_MODULE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARIO_TYPE_MODULE))
#define ARIO_IS_MODULE_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((obj), ARIO_TYPE_MODULE))
#define ARIO_MODULE_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_MODULE, ArioModuleClass))

typedef struct _ArioModule        ArioModule;

GType                   ario_module_get_type            (void) G_GNUC_CONST;

ArioModule*             ario_module_new                 (const gchar *path);

const gchar*            ario_module_get_path            (ArioModule *module);

GObject*                ario_module_new_object          (ArioModule *module);

G_END_DECLS

#endif
