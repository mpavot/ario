/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __ARIO_MMKEYS_PLUGIN_H__
#define __ARIO_MMKEYS_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <ario-plugin.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_MMKEYS_PLUGIN                 (ario_mmkeys_plugin_get_type ())
#define ARIO_MMKEYS_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_MMKEYS_PLUGIN, ArioMmkeysPlugin))
#define ARIO_MMKEYS_PLUGIN_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_MMKEYS_PLUGIN, ArioMmkeysPluginClass))
#define ARIO_IS_MMKEYS_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_MMKEYS_PLUGIN))
#define ARIO_IS_MMKEYS_PLUGIN_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_MMKEYS_PLUGIN))
#define ARIO_MMKEYS_PLUGIN_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_MMKEYS_PLUGIN, ArioMmkeysPluginClass))

/* Private structure type */
typedef struct _ArioMmkeysPluginPrivate        ArioMmkeysPluginPrivate;

/*
 * Main object structure
 */
typedef struct _ArioMmkeysPlugin                ArioMmkeysPlugin;

struct _ArioMmkeysPlugin
{
        ArioPlugin parent_instance;

        /*< private >*/
        ArioMmkeysPluginPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _ArioMmkeysPluginClass        ArioMmkeysPluginClass;

struct _ArioMmkeysPluginClass
{
        ArioPluginClass parent_class;
};

/*
 * Public methods
 */
GType        ario_mmkeys_plugin_get_type                (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_ario_plugin (GTypeModule *module);

#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer

extern void ario_marshal_VOID__STRING_STRING (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);
void
ario_marshal_VOID__STRING_STRING (GClosure     *closure,
                                  GValue       *return_value,
                                  guint         n_param_values,
                                  const GValue *param_values,
                                  gpointer      invocation_hint,
                                  gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     arg_2,
                                                    gpointer     data2);
  register GMarshalFunc_VOID__STRING_STRING callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            data2);
}

G_END_DECLS

#endif /* __ARIO_MMKEYS_PLUGIN_H__ */

