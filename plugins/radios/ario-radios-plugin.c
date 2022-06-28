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


struct _ArioRadiosPluginPrivate
{
        GtkWidget *source;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioRadiosPlugin, ario_radios_plugin, G_ADD_PRIVATE_DYNAMIC (ArioRadiosPlugin))

static void
ario_radios_plugin_init (ArioRadiosPlugin *plugin)
{
        plugin->priv = ario_radios_plugin_get_instance_private (plugin);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        ArioRadiosPlugin *pi = ARIO_RADIOS_PLUGIN (plugin);

        pi->priv->source = ario_radio_new ();
        g_return_if_fail (IS_ARIO_RADIO (pi->priv->source));

        ario_source_manager_append (ARIO_SOURCE (pi->priv->source));
        ario_source_manager_reorder ();
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        ArioRadiosPlugin *pi = ARIO_RADIOS_PLUGIN (plugin);

        ario_source_manager_remove (ARIO_SOURCE (pi->priv->source));
}

static void
ario_radios_plugin_class_init (ArioRadiosPluginClass *klass)
{
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
}
