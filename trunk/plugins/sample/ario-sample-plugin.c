/*
 * Copyright (C) 2002-2005 - Paolo Maggi
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include "ario-sample-plugin.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-debug.h>
#include <ario-shell.h>

#define ARIO_SAMPLE_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_SAMPLE_PLUGIN, ArioSamplePluginPrivate))

struct _ArioSamplePluginPrivate
{
        gpointer dummy;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioSamplePlugin, ario_sample_plugin)

static void
ario_sample_plugin_init (ArioSamplePlugin *plugin)
{
        plugin->priv = ARIO_SAMPLE_PLUGIN_GET_PRIVATE (plugin);

        printf ("ArioSamplePlugin initialising\n");
}

static void
ario_sample_plugin_finalize (GObject *object)
{
/*
        ArioSamplePlugin *plugin = ARIO_SAMPLE_PLUGIN (object);
*/
        printf ("ArioSamplePlugin finalising\n");

        G_OBJECT_CLASS (ario_sample_plugin_parent_class)->finalize (object);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        printf ("ArioSamplePlugin activate\n");
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        printf ("ArioSamplePlugin deactivate\n");
}

static void
ario_sample_plugin_class_init (ArioSamplePluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        object_class->finalize = ario_sample_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
        
        g_type_class_add_private (object_class, sizeof (ArioSamplePluginPrivate));
}
