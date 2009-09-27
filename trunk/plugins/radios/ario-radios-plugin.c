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

#include "ario-radios-plugin.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-debug.h>
#include <ario-shell.h>
#include <ario-source-manager.h>
#include "ario-radio.h"

#define ARIO_RADIOS_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_RADIOS_PLUGIN, ArioRadiosPluginPrivate))

struct _ArioRadiosPluginPrivate
{
        guint ui_merge_id;
        GtkWidget *source;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioRadiosPlugin, ario_radios_plugin)

static void
ario_radios_plugin_init (ArioRadiosPlugin *plugin)
{
        plugin->priv = ARIO_RADIOS_PLUGIN_GET_PRIVATE (plugin);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        GtkUIManager *uimanager;
        GtkActionGroup *actiongroup;
        ArioRadiosPlugin *pi = ARIO_RADIOS_PLUGIN (plugin);
        gchar *file;

        g_object_get (shell,
                      "ui-manager", &uimanager,
                      "action-group", &actiongroup,
                      NULL);

        pi->priv->source = ario_radio_new (uimanager,
                                           actiongroup);
        g_return_if_fail (IS_ARIO_RADIO (pi->priv->source));

        file = ario_plugin_find_file ("radios-ui.xml");
        if (file) {
                pi->priv->ui_merge_id = gtk_ui_manager_add_ui_from_file (uimanager,
                                                                         file, NULL);
                g_free (file);
        }

        g_object_unref (uimanager);
        g_object_unref (actiongroup);

        ario_source_manager_append (ARIO_SOURCE (pi->priv->source));
        ario_source_manager_reorder ();
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        GtkUIManager *uimanager;

        ArioRadiosPlugin *pi = ARIO_RADIOS_PLUGIN (plugin);

        g_object_get (shell, "ui-manager", &uimanager, NULL);
        gtk_ui_manager_remove_ui (uimanager, pi->priv->ui_merge_id);
        g_object_unref (uimanager);

        ario_source_manager_remove (ARIO_SOURCE (pi->priv->source));
}

static void
ario_radios_plugin_class_init (ArioRadiosPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (object_class, sizeof (ArioRadiosPluginPrivate));
}
