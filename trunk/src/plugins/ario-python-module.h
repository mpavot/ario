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

#ifndef ARIO_PYTHON_MODULE_H
#define ARIO_PYTHON_MODULE_H

#include <Python.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define ARIO_TYPE_PYTHON_MODULE		(ario_python_module_get_type ())
#define ARIO_PYTHON_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ARIO_TYPE_PYTHON_MODULE, ArioPythonModule))
#define ARIO_PYTHON_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ARIO_TYPE_PYTHON_MODULE, ArioPythonModuleClass))
#define ARIO_IS_PYTHON_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARIO_TYPE_PYTHON_MODULE))
#define ARIO_IS_PYTHON_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), ARIO_TYPE_PYTHON_MODULE))
#define ARIO_PYTHON_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_PYTHON_MODULE, ArioPythonModuleClass))

typedef struct _ArioPythonModule		ArioPythonModule;
typedef struct _ArioPythonModuleClass 		ArioPythonModuleClass;
typedef struct _ArioPythonModulePrivate	ArioPythonModulePrivate;

struct _ArioPythonModuleClass
{
	GTypeModuleClass parent_class;
};

struct _ArioPythonModule
{
	GTypeModule parent_instance;
};

GType			 ario_python_module_get_type		(void);

ArioPythonModule	*ario_python_module_new		(const gchar* path,
								 const gchar *module);

GObject			*ario_python_module_new_object		(ArioPythonModule *module);


/* --- Python utils --- */

/* Must be called before loading python modules */
gboolean		ario_python_init			(void);

void			ario_python_shutdown			(void);

void			ario_python_garbage_collect		(void);

G_END_DECLS

#endif
