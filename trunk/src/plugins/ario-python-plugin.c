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

#include <config.h>

#include "ario-python-plugin.h"
#include "ario-plugin.h"
#include "ario-debug.h"

#include <pygobject.h>
#include <string.h>

static GObjectClass *parent_class;

static PyObject *
call_python_method (ArioPythonObject *object,
                    ArioShell        *shell,
                    gchar             *method)
{
        PyObject *py_ret = NULL;

        g_return_val_if_fail (PyObject_HasAttrString (object->instance, method), NULL);

        if (shell == NULL)
        {
                py_ret = PyObject_CallMethod (object->instance,
                                              method,
                                              NULL);
        }
        else
        {
                py_ret = PyObject_CallMethod (object->instance,
                                              method,
                                              "(N)",
                                              pygobject_new (G_OBJECT (shell)));
        }
        
        if (!py_ret)
                PyErr_Print ();

        return py_ret;
}

static gboolean
check_py_object_is_gtk_widget (PyObject *py_obj)
{
        static PyTypeObject *_PyGtkWidget_Type = NULL;

        if (_PyGtkWidget_Type == NULL)
        {
                PyObject *module;

                    if ((module = PyImport_ImportModule ("gtk")))
                    {
                        PyObject *moddict = PyModule_GetDict (module);
                        _PyGtkWidget_Type = (PyTypeObject *) PyDict_GetItemString (moddict, "Widget");
                    }

                if (_PyGtkWidget_Type == NULL)
                {
                        PyErr_SetString(PyExc_TypeError, "could not find Python gtk widget type");
                        PyErr_Print();

                        return FALSE;
                }
        }

        return PyObject_TypeCheck (py_obj, _PyGtkWidget_Type) ? TRUE : FALSE;
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        PyGILState_STATE state = pyg_gil_state_ensure ();
        ArioPythonObject *object = (ArioPythonObject *)plugin;
        
        if (PyObject_HasAttrString (object->instance, "deactivate"))
        {                
                PyObject *py_ret = call_python_method (object, shell, "deactivate");
                
                if (py_ret)
                {
                        Py_XDECREF (py_ret);
                }
        }
        else
                ARIO_PLUGIN_CLASS (parent_class)->deactivate (plugin, shell);

        pyg_gil_state_release (state);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        PyGILState_STATE state = pyg_gil_state_ensure ();
        ArioPythonObject *object = (ArioPythonObject *)plugin;
        
        if (PyObject_HasAttrString (object->instance, "activate"))
        {
                PyObject *py_ret = call_python_method (object, shell, "activate");

                if (py_ret)
                {
                        Py_XDECREF (py_ret);
                }
        }
        else
                ARIO_PLUGIN_CLASS (parent_class)->activate (plugin, shell);
        
        pyg_gil_state_release (state);
}

static GtkWidget *
impl_create_configure_dialog (ArioPlugin *plugin)
{
        PyGILState_STATE state = pyg_gil_state_ensure ();
        ArioPythonObject *object = (ArioPythonObject *)plugin;
        GtkWidget *ret = NULL;
        
        if (PyObject_HasAttrString (object->instance, "create_configure_dialog"))
        {
                PyObject *py_ret = call_python_method (object, NULL, "create_configure_dialog");
        
                if (py_ret)
                {
                        if (check_py_object_is_gtk_widget (py_ret))
                        {
                                ret = GTK_WIDGET (pygobject_get (py_ret));
                                g_object_ref (ret);
                        }
                        else
                        {
                                PyErr_SetString(PyExc_TypeError, "return value for create_configure_dialog is not a GtkWidget");
                                PyErr_Print();
                        }
                        
                        Py_DECREF (py_ret);
                }
        }
        else
                ret = ARIO_PLUGIN_CLASS (parent_class)->create_configure_dialog (plugin);
 
        pyg_gil_state_release (state);
        return ret;
}

static gboolean
impl_is_configurable (ArioPlugin *plugin)
{
        PyGILState_STATE state = pyg_gil_state_ensure ();
        ArioPythonObject *object = (ArioPythonObject *) plugin;
        PyObject *dict = object->instance->ob_type->tp_dict;        
        gboolean result;
        
        if (dict == NULL)
                result = FALSE;
        else if (!PyDict_Check(dict))
                result = FALSE;
        else 
                result = PyDict_GetItemString(dict, "create_configure_dialog") != NULL;

        pyg_gil_state_release (state);
        
        return result;
}
                                                
static void 
ario_python_object_init (ArioPythonObject *object)
{
        ArioPythonObjectClass *class;

        ARIO_LOG_DBG ("Creating Python plugin instance");

        class = (ArioPythonObjectClass*) (((GTypeInstance*) object)->g_class);

        object->instance = PyObject_CallObject (class->type, NULL);
        if (object->instance == NULL)
                PyErr_Print();
}

static void
ario_python_object_finalize (GObject *object)
{
        ARIO_LOG_DBG ("Finalizing Python plugin instance");

        Py_DECREF (((ArioPythonObject *) object)->instance);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_python_object_class_init (ArioPythonObjectClass *klass,
                                gpointer                class_data)
{
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        klass->type = (PyObject*) class_data;

        G_OBJECT_CLASS (klass)->finalize = ario_python_object_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
        plugin_class->create_configure_dialog = impl_create_configure_dialog;
        plugin_class->is_configurable = impl_is_configurable;
}

GType
ario_python_object_get_type (GTypeModule *module, 
                              PyObject    *type)
{
        GType gtype;
        gchar *type_name;

        GTypeInfo info = {
                sizeof (ArioPythonObjectClass),
                NULL,           /* base_init */
                NULL,           /* base_finalize */
                (GClassInitFunc) ario_python_object_class_init,
                NULL,           /* class_finalize */
                type,           /* class_data */
                sizeof (ArioPythonObject),
                0,              /* n_preallocs */
                (GInstanceInitFunc) ario_python_object_init,
        };

        Py_INCREF (type);

        type_name = g_strdup_printf ("%s+ArioPythonPlugin",
                                     PyString_AsString (PyObject_GetAttrString (type, "__name__")));

        ARIO_LOG_DBG ("Registering Python plugin instance: %s", type_name);
        gtype = g_type_module_register_type (module, 
                                             ARIO_TYPE_PLUGIN,
                                             type_name,
                                             &info, 0);
        g_free (type_name);

        return gtype;
}
