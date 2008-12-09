/*
 * heavily based on code from Gedit
 *
 * Copyright (C) 2005 Raphael Slinckx
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

#ifndef ARIO_PYTHON_OBJECT_H
#define ARIO_PYTHON_OBJECT_H

#include <Python.h>
#include <glib-object.h>
#include "ario-plugin.h"

G_BEGIN_DECLS

typedef struct _ArioPythonObject ArioPythonObject;
typedef struct _ArioPythonObjectClass ArioPythonObjectClass;

struct _ArioPythonObject
{
        ArioPlugin parent_slot;
        PyObject *instance;
};

struct _ArioPythonObjectClass
{
        ArioPluginClass parent_slot;
        PyObject *type;
};

GType ario_python_object_get_type (GTypeModule *module, PyObject *type);

G_END_DECLS

#endif
