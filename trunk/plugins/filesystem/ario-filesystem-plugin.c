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

#define ARIO_FILESYSTEM_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_FILESYSTEM_PLUGIN, ArioFilesystemPluginPrivate))

struct _ArioFilesystemPluginPrivate
{
        guint ui_merge_id;
        GtkWidget *source;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioFilesystemPlugin, ario_filesystem_plugin)

static void
ario_filesystem_plugin_init (ArioFilesystemPlugin *plugin)
{
        plugin->priv = ARIO_FILESYSTEM_PLUGIN_GET_PRIVATE (plugin);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        GtkUIManager *uimanager;
        GtkActionGroup *actiongroup;
        ArioFilesystemPlugin *pi = ARIO_FILESYSTEM_PLUGIN (plugin);
        gchar *file;

        g_object_get (shell,
                      "ui-manager", &uimanager,
                      "action-group", &actiongroup,
                      NULL);

        pi->priv->source = ario_filesystem_new (uimanager,
                                                actiongroup);
        g_return_if_fail (IS_ARIO_FILESYSTEM (pi->priv->source));

        file = ario_plugin_find_file ("filesystem-ui.xml");
        if (file) {
                pi->priv->ui_merge_id = gtk_ui_manager_add_ui_from_file (uimanager,
                                                                         file, NULL);
                g_free (file);
        }

        g_object_unref (uimanager);
        g_object_unref (actiongroup);

        ario_sourcemanager_append (ARIO_SOURCE (pi->priv->source));
        ario_sourcemanager_reorder ();
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        GtkUIManager *uimanager;

        ArioFilesystemPlugin *pi = ARIO_FILESYSTEM_PLUGIN (plugin);

        g_object_get (shell, "ui-manager", &uimanager, NULL);
        gtk_ui_manager_remove_ui (uimanager, pi->priv->ui_merge_id);
        g_object_unref (uimanager);

        ario_sourcemanager_remove (ARIO_SOURCE (pi->priv->source));
}

static void
ario_filesystem_plugin_class_init (ArioFilesystemPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (object_class, sizeof (ArioFilesystemPluginPrivate));
}
