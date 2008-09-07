/*
 *  Based on Rhythmbox MMKEYS plugin:
 *  Copyright (C) 2002, 2003 Jorn Baayen <jorn@nl.linux.org>
 *  Copyright (C) 2002,2003 Colin Walters <walters@debian.org>
 *  Copyright (C) 2007  James Livingston  <doclivingston@gmail.com>
 *  Copyright (C) 2007  Jonathan Matthew  <jonathan@kaolin.wh9.net>
 *
 *  Adapted to Ario:
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */
#include <dbus/dbus-glib.h>
#include "ario-mmkeys-plugin.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-debug.h>
#include <ario-shell.h>
#include <ario-mpd.h>

#define ARIO_MMKEYS_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_MMKEYS_PLUGIN, ArioMmkeysPluginPrivate))

struct _ArioMmkeysPluginPrivate
{
        DBusGProxy *proxy;
        ArioMpd *mpd;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioMmkeysPlugin, ario_mmkeys_plugin)

static void
ario_mmkeys_plugin_init (ArioMmkeysPlugin *plugin)
{
        plugin->priv = ARIO_MMKEYS_PLUGIN_GET_PRIVATE (plugin);
}

static void
ario_mmkeys_plugin_finalize (GObject *object)
{
        G_OBJECT_CLASS (ario_mmkeys_plugin_parent_class)->finalize (object);
}

static void
media_player_key_pressed (DBusGProxy *proxy,
                          const gchar *application,
                          const gchar *key,
                          ArioMmkeysPlugin *plugin)
{
        ARIO_LOG_DBG ("got media key '%s' for application '%s'",
                      key, application);

        if (strcmp (application, "Ario"))
                return;

        if (strcmp (key, "Play") == 0 ||
            strcmp (key, "Pause") == 0) {
                if (ario_mpd_is_paused (plugin->priv->mpd))
                        ario_mpd_do_play (plugin->priv->mpd);
                else
                        ario_mpd_do_pause (plugin->priv->mpd);
        } else if (strcmp (key, "Stop") == 0) {
                ario_mpd_do_stop (plugin->priv->mpd);
        } else if (strcmp (key, "Previous") == 0) {
                ario_mpd_do_prev (plugin->priv->mpd);
        } else if (strcmp (key, "Next") == 0) {
                ario_mpd_do_next (plugin->priv->mpd);
        }
}

static void
impl_activate (ArioPlugin *pl,
               ArioShell *shell)
{
        DBusGConnection *bus;
        ArioMmkeysPlugin *plugin = ARIO_MMKEYS_PLUGIN (pl);

        ARIO_LOG_DBG ("activating media player keys plugin");

        g_object_get (shell,
                      "mpd", &plugin->priv->mpd,
                      NULL);
        g_object_unref (plugin->priv->mpd);

        bus = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
        if (bus) {
                GError *error = NULL;

                plugin->priv->proxy = dbus_g_proxy_new_for_name (bus,
                                                                 "org.gnome.SettingsDaemon",
                                                                 "/org/gnome/SettingsDaemon",
                                                                 "org.gnome.SettingsDaemon");
                if (plugin->priv->proxy) {
                        dbus_g_proxy_call (plugin->priv->proxy,
                                           "GrabMediaPlayerKeys", &error,
                                           G_TYPE_STRING, "Ario",
                                           G_TYPE_UINT, 0,
                                           G_TYPE_INVALID,
                                           G_TYPE_INVALID);
                        if (!error) {
                                ARIO_LOG_DBG ("created dbus proxy for org.gnome.SettingsDaemon; grabbing keys");
                                dbus_g_object_register_marshaller (ario_marshal_VOID__STRING_STRING,
                                                                   G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

                                dbus_g_proxy_add_signal (plugin->priv->proxy,
                                                         "MediaPlayerKeyPressed",
                                                         G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INVALID);

                                dbus_g_proxy_connect_signal (plugin->priv->proxy,
                                                             "MediaPlayerKeyPressed",
                                                             G_CALLBACK (media_player_key_pressed),
                                                             plugin, NULL);

                        } else if (error->domain == DBUS_GERROR &&
                                   (error->code != DBUS_GERROR_NAME_HAS_NO_OWNER ||
                                    error->code != DBUS_GERROR_SERVICE_UNKNOWN)) {
                                /* settings daemon dbus service doesn't exist.
                                 * just silently fail.
                                 */
                                g_warning ("org.gnome.SettingsDaemon dbus service not found");
                                g_error_free (error);
                        } else {
                                g_warning ("Unable to grab media player keys: %s", error->message);
                                g_error_free (error);
                        }
                }
        } else {
                g_warning ("couldn't get dbus session bus");
        }
}

static void
impl_deactivate (ArioPlugin *pl,
                 ArioShell *shell)
{
        ArioMmkeysPlugin *plugin = ARIO_MMKEYS_PLUGIN (pl);

        if (plugin->priv->proxy != NULL) {
                GError *error = NULL;

                dbus_g_proxy_call (plugin->priv->proxy,
                                   "ReleaseMediaPlayerKeys", &error,
                                   G_TYPE_STRING, "Rhythmbox",
                                   G_TYPE_INVALID, G_TYPE_INVALID);
                if (error != NULL) {
                        g_warning ("Could not release media player keys: %s", error->message);
                        g_error_free (error);
                }

                g_object_unref (plugin->priv->proxy);
                plugin->priv->proxy = NULL;
        }
}

static void
ario_mmkeys_plugin_class_init (ArioMmkeysPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        object_class->finalize = ario_mmkeys_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (object_class, sizeof (ArioMmkeysPluginPrivate));
}
