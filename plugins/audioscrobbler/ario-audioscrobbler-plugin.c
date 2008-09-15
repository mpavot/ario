/*
 *  Based on Rhythmbox audioscrobbler plugin:
 * Copyright (C) 2006 James Livingston <jrl@ids.org.au>
 *
 *  Adapted to Ario:
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include "ario-audioscrobbler.h"
#include "plugins/ario-plugin.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "shell/ario-shell.h"


#define ARIO_TYPE_AUDIOSCROBBLER_PLUGIN                 (ario_audioscrobbler_plugin_get_type ())
#define ARIO_AUDIOSCROBBLER_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_AUDIOSCROBBLER_PLUGIN, ArioAudioscrobblerPlugin))
#define ARIO_AUDIOSCROBBLER_PLUGIN_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_AUDIOSCROBBLER_PLUGIN, ArioAudioscrobblerPluginClass))
#define ARIO_IS_AUDIOSCROBBLER_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_AUDIOSCROBBLER_PLUGIN))
#define ARIO_IS_AUDIOSCROBBLER_PLUGIN_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_AUDIOSCROBBLER_PLUGIN))
#define ARIO_AUDIOSCROBBLER_PLUGIN_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_AUDIOSCROBBLER_PLUGIN, ArioAudioscrobblerPluginClass))

typedef struct
{
        ArioPlugin parent;
        ArioAudioscrobbler *audioscrobbler;

        ArioMpd *mpd;
} ArioAudioscrobblerPlugin;

typedef struct
{
        ArioPluginClass parent_class;
} ArioAudioscrobblerPluginClass;


G_MODULE_EXPORT GType register_ario_plugin (GTypeModule *module);
GType ario_audioscrobbler_plugin_get_type (void) G_GNUC_CONST;



static void ario_audioscrobbler_plugin_init (ArioAudioscrobblerPlugin *plugin);
static void impl_activate (ArioPlugin *plugin, ArioShell *shell);
static void impl_deactivate (ArioPlugin *plugin, ArioShell *shell);
static GtkWidget* impl_create_configure_dialog (ArioPlugin *plugin);

ARIO_PLUGIN_REGISTER_TYPE(ArioAudioscrobblerPlugin, ario_audioscrobbler_plugin)

static void
ario_audioscrobbler_plugin_class_init (ArioAudioscrobblerPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
        plugin_class->create_configure_dialog = impl_create_configure_dialog;
}

static void
ario_audioscrobbler_plugin_init (ArioAudioscrobblerPlugin *plugin)
{
        ARIO_LOG_DBG ("ArioAudioscrobblerPlugin initialising");
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        ArioAudioscrobblerPlugin *asplugin = ARIO_AUDIOSCROBBLER_PLUGIN (plugin);

        g_object_get (shell, "mpd", &asplugin->mpd, NULL);
        asplugin->audioscrobbler = ario_audioscrobbler_new (asplugin->mpd);

        ario_mpd_use_count_inc (asplugin->mpd);
        g_object_unref (asplugin->mpd);
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        ArioAudioscrobblerPlugin *asplugin = ARIO_AUDIOSCROBBLER_PLUGIN (plugin);

        g_object_unref (asplugin->audioscrobbler);

        if (asplugin->mpd) {
                ario_mpd_use_count_dec (asplugin->mpd);
                asplugin->mpd = NULL;
        }
}

static GtkWidget*
impl_create_configure_dialog (ArioPlugin *plugin)
{
        ArioAudioscrobblerPlugin *asplugin = ARIO_AUDIOSCROBBLER_PLUGIN (plugin);
        if (asplugin->audioscrobbler == NULL)
                return NULL;

        return ario_audioscrobbler_get_config_widget (asplugin->audioscrobbler, plugin);
}

