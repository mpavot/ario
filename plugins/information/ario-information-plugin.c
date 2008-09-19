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

#include "ario-information-plugin.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-debug.h>
#include <ario-shell.h>
#include <ario-source-manager.h>
#include "ario-information.h"

#define ARIO_INFORMATION_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_INFORMATION_PLUGIN, ArioInformationPluginPrivate))

struct _ArioInformationPluginPrivate
{
        guint ui_merge_id;
        GtkWidget *source;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioInformationPlugin, ario_information_plugin)

static void
ario_information_plugin_init (ArioInformationPlugin *plugin)
{
        plugin->priv = ARIO_INFORMATION_PLUGIN_GET_PRIVATE (plugin);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        GtkUIManager *uimanager;
        ArioInformationPlugin *pi = ARIO_INFORMATION_PLUGIN (plugin);
        g_object_get (shell,
                      "ui-manager", &uimanager,
                      NULL);

        pi->priv->source = ario_information_new (uimanager);
        g_return_if_fail (IS_ARIO_INFORMATION (pi->priv->source));

        g_object_unref (uimanager);

        ario_sourcemanager_append (ARIO_SOURCE (pi->priv->source));
        ario_sourcemanager_reorder ();
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        ArioInformationPlugin *pi = ARIO_INFORMATION_PLUGIN (plugin);
        ario_sourcemanager_remove (ARIO_SOURCE (pi->priv->source));
}

static void
ario_information_plugin_class_init (ArioInformationPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (object_class, sizeof (ArioInformationPluginPrivate));
}
