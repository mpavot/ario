/*
 * Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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


#include "ario-filesystem-plugin.h"
#include <config.h>
#include <string.h> /* For strlen */

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-debug.h>
#include <ario-shell.h>
#include <ario-source-manager.h>
#include "ario-filesystem.h"


struct _ArioFilesystemPluginPrivate
{
        gchar *menu_file;
        guint ui_merge_id;
        GtkWidget *source;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioFilesystemPlugin, ario_filesystem_plugin, G_ADD_PRIVATE_DYNAMIC (ArioFilesystemPlugin))

static void
ario_filesystem_plugin_init (ArioFilesystemPlugin *plugin)
{
        plugin->priv = ario_filesystem_plugin_get_instance_private (plugin);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        ArioFilesystemPlugin *pi = ARIO_FILESYSTEM_PLUGIN (plugin);

        pi->priv->source = ario_filesystem_new ();
        g_return_if_fail (IS_ARIO_FILESYSTEM (pi->priv->source));

        ario_source_manager_append (ARIO_SOURCE (pi->priv->source));
        ario_source_manager_reorder ();
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        ArioFilesystemPlugin *pi = ARIO_FILESYSTEM_PLUGIN (plugin);
        ario_source_manager_remove (ARIO_SOURCE (pi->priv->source));
}

static void
ario_filesystem_plugin_class_init (ArioFilesystemPluginClass *klass)
{
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
}
