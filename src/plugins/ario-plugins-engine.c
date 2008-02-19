/*
 * heavily based on code from Gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gconf/gconf-client.h>

#include "plugins/ario-plugins-engine.h"
#include "plugins/ario-plugin-info-priv.h"
#include "plugins/ario-plugin.h"
#include "ario-debug.h"
#include "ario-util.h"

#include "ario-module.h"
#ifdef ENABLE_PYTHON
#include "ario-python-module.h"
#endif

#define ARIO_PLUGINS_ENGINE_BASE_KEY "/apps/ario/plugins"
#define ARIO_PLUGINS_ENGINE_KEY ARIO_PLUGINS_ENGINE_BASE_KEY "/active-plugins"

#define PLUGIN_EXT ".ario-plugin"

/* Signals */
enum
{
        ACTIVATE_PLUGIN,
        DEACTIVATE_PLUGIN,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE(ArioPluginsEngine, ario_plugins_engine, G_TYPE_OBJECT)

struct _ArioPluginsEnginePrivate
{
        GList *plugin_list;
        GConfClient *gconf_client;
};

ArioPluginsEngine *default_engine = NULL;

static void ario_plugins_engine_active_plugins_changed (GConfClient* client,
                                                        guint cnxn_id, 
                                                        GConfEntry* entry, 
                                                        gpointer user_data);
static void ario_plugins_engine_activate_plugin_real (ArioPluginsEngine* engine,
                                                      ArioPluginInfo* info);
static void ario_plugins_engine_deactivate_plugin_real (ArioPluginsEngine* engine,
                                                        ArioPluginInfo* info);

static gint
compare_plugin_info (ArioPluginInfo *info1,
                     ArioPluginInfo *info2)
{
        return strcmp (info1->module_name, info2->module_name);
}

static ArioShell *static_shell;

static void
ario_plugins_engine_load_dir (ArioPluginsEngine *engine,
                              const gchar        *dir,
                              GSList             *active_plugins)
{
        GError *error = NULL;
        GDir *d;
        const gchar *dirent;

        g_return_if_fail (engine->priv->gconf_client != NULL);
        g_return_if_fail (dir != NULL);

        ARIO_LOG_DBG ("DIR: %s", dir);

        d = g_dir_open (dir, 0, &error);
        if (!d) {
                g_warning (error->message);
                g_error_free (error);
                return;
        }

        while ((dirent = g_dir_read_name (d))) {
                if (g_str_has_suffix (dirent, PLUGIN_EXT)) {
                        gchar *plugin_file;
                        ArioPluginInfo *info;
                        
                        plugin_file = g_build_filename (dir, dirent, NULL);
                        info = _ario_plugin_info_new (plugin_file);
                        g_free (plugin_file);

                        if (info == NULL)
                                continue;

                        /* If a plugin with this name has already been loaded
                         * drop this one (user plugins override system plugins) */
                        if (g_list_find_custom (engine->priv->plugin_list,
                                                info,
                                                (GCompareFunc)compare_plugin_info) != NULL) {
                                g_warning ("Two or more plugins named '%s'. "
                                           "Only the first will be considered.\n",
                                           info->module_name);

                                _ario_plugin_info_unref (info);

                                continue;
                        }

                        /* Actually, the plugin will be activated when reactivate_all
                         * will be called for the first time. */
                        info->active = g_slist_find_custom (active_plugins,
                                                            info->module_name,
                                                            (GCompareFunc)strcmp) != NULL;

                        engine->priv->plugin_list = g_list_prepend (engine->priv->plugin_list, info);

                        ARIO_LOG_DBG ("Plugin %s loaded", info->name);
                }
        }

        g_dir_close (d);
}

static void
ario_plugins_engine_load_all (ArioPluginsEngine *engine)
{
        GSList *active_plugins;
        const gchar *home;
        const gchar *pdirs_env;
        gchar **pdirs;
        int i;

        active_plugins = gconf_client_get_list (engine->priv->gconf_client,
                                                ARIO_PLUGINS_ENGINE_KEY,
                                                GCONF_VALUE_STRING,
                                                NULL);

        /* load user's plugins */
        home = g_get_home_dir ();
        if (home == NULL) {
                g_warning ("Could not get HOME directory\n");
        } else {
                gchar *pdir;

                pdir = g_build_filename (ario_util_config_dir (), "plugins", NULL);

                if (g_file_test (pdir, G_FILE_TEST_IS_DIR))
                        ario_plugins_engine_load_dir (engine, pdir, active_plugins);
                g_free (pdir);
        }

        //pdirs_env = g_getenv ("ARIO_PLUGINS_PATH");
        /* What if no env var is set? We use the default location(s)! */
        //if (pdirs_env == NULL)
                pdirs_env = ARIO_PLUGIN_DIR;

        ARIO_LOG_DBG ("ARIO_PLUGINS_PATH=%s", pdirs_env);
        pdirs = g_strsplit (pdirs_env, G_SEARCHPATH_SEPARATOR_S, 0);

        for (i = 0; pdirs[i] != NULL; i++)
                ario_plugins_engine_load_dir (engine, pdirs[i], active_plugins);

        g_strfreev (pdirs);
}

static void
ario_plugins_engine_init (ArioPluginsEngine *engine)
{
        if (!g_module_supported ()) {
                g_warning ("ario is not able to initialize the plugins engine.");
                return;
        }

        engine->priv = G_TYPE_INSTANCE_GET_PRIVATE (engine,
                                                    ARIO_TYPE_PLUGINS_ENGINE,
                                                    ArioPluginsEnginePrivate);

        engine->priv->gconf_client = gconf_client_get_default ();
        g_return_if_fail (engine->priv->gconf_client != NULL);

        gconf_client_add_dir (engine->priv->gconf_client,
                              ARIO_PLUGINS_ENGINE_BASE_KEY,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);

        gconf_client_notify_add (engine->priv->gconf_client,
                                 ARIO_PLUGINS_ENGINE_KEY,
                                 ario_plugins_engine_active_plugins_changed,
                                 engine, NULL, NULL);

        ario_plugins_engine_load_all (engine);
}

void
ario_plugins_engine_garbage_collect (ArioPluginsEngine *engine)
{
#ifdef ENABLE_PYTHON
        ario_python_garbage_collect ();
#endif
}

static void
ario_plugins_engine_finalize (GObject *object)
{
        ArioPluginsEngine *engine = ARIO_PLUGINS_ENGINE (object);

#ifdef ENABLE_PYTHON
        /* Note: that this may cause finalization of objects (typically
         * the ArioShell) by running the garbage collector. Since some
         * of the plugin may have installed callbacks upon object
         * finalization (typically they need to free the WindowData)
         * it must run before we get rid of the plugins.
         */
        ario_python_shutdown ();
#endif

        g_return_if_fail (engine->priv->gconf_client != NULL);

        g_list_foreach (engine->priv->plugin_list,
                        (GFunc) _ario_plugin_info_unref, NULL);
        g_list_free (engine->priv->plugin_list);

        g_object_unref (engine->priv->gconf_client);
}

static void
ario_plugins_engine_class_init (ArioPluginsEngineClass *klass)
{
        GType the_type = G_TYPE_FROM_CLASS (klass);
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_plugins_engine_finalize;
        klass->activate_plugin = ario_plugins_engine_activate_plugin_real;
        klass->deactivate_plugin = ario_plugins_engine_deactivate_plugin_real;

        signals[ACTIVATE_PLUGIN] =
                g_signal_new ("activate-plugin",
                              the_type,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioPluginsEngineClass, activate_plugin),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__BOXED,
                              G_TYPE_NONE,
                              1,
                              ARIO_TYPE_PLUGIN_INFO | G_SIGNAL_TYPE_STATIC_SCOPE);

        signals[DEACTIVATE_PLUGIN] =
                g_signal_new ("deactivate-plugin",
                              the_type,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioPluginsEngineClass, deactivate_plugin),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__BOXED,
                              G_TYPE_NONE,
                              1,
                              ARIO_TYPE_PLUGIN_INFO | G_SIGNAL_TYPE_STATIC_SCOPE);

        g_type_class_add_private (klass, sizeof (ArioPluginsEnginePrivate));
}

void
ario_plugins_engine_set_shell (ArioShell *shell)
{
        static_shell = shell;
}

ArioPluginsEngine *
ario_plugins_engine_get_default (void)
{
        if (default_engine != NULL)
                return default_engine;

        default_engine = ARIO_PLUGINS_ENGINE (g_object_new (ARIO_TYPE_PLUGINS_ENGINE, NULL));
        g_object_add_weak_pointer (G_OBJECT (default_engine),
                                   (gpointer) &default_engine);
        return default_engine;
}

const GList *
ario_plugins_engine_get_plugin_list (ArioPluginsEngine *engine)
{
        return engine->priv->plugin_list;
}

static gboolean
load_plugin_module (ArioPluginInfo *info)
{
        gchar *path;
        gchar *dirname;

        g_return_val_if_fail (info != NULL, FALSE);
        g_return_val_if_fail (info->file != NULL, FALSE);
        g_return_val_if_fail (info->module_name != NULL, FALSE);
        g_return_val_if_fail (info->plugin == NULL, FALSE);
        g_return_val_if_fail (info->available, FALSE);
        
        switch (info->loader)
        {
                case ARIO_PLUGIN_LOADER_C:
                        dirname = g_path_get_dirname (info->file);        
                        g_return_val_if_fail (dirname != NULL, FALSE);

                        path = g_module_build_path (dirname, info->module_name);
                        g_free (dirname);
                        g_return_val_if_fail (path != NULL, FALSE);
        
                        info->module = G_TYPE_MODULE (ario_module_new (path));
                        g_free (path);
                        
                        break;

#ifdef ENABLE_PYTHON
                case ARIO_PLUGIN_LOADER_PY:
                {
                        gchar *dir;
                        
                        if (!ario_python_init ()) {
                                /* Mark plugin as unavailable and fails */
                                info->available = FALSE;
                                
                                g_warning ("Cannot load Python plugin '%s' since ario "
                                           "was not able to initialize the Python interpreter.",
                                           info->name);

                                return FALSE;
                        }

                        dir = g_path_get_dirname (info->file);
                        
                        g_return_val_if_fail ((info->module_name != NULL) &&
                                              (info->module_name[0] != '\0'),
                                              FALSE);

                        info->module = G_TYPE_MODULE (
                                        ario_python_module_new (dir, info->module_name));
                                        
                        g_free (dir);
                        break;
                }
#endif
                default:
                        g_return_val_if_reached (FALSE);
        }

        if (!g_type_module_use (info->module)) {
                switch (info->loader)
                {
                        case ARIO_PLUGIN_LOADER_C:
                                g_warning ("Cannot load plugin '%s' since file '%s' cannot be read.",
                                           info->name,                                   
                                           ario_module_get_path (ARIO_MODULE (info->module)));
                                break;

                        case ARIO_PLUGIN_LOADER_PY:
                                g_warning ("Cannot load Python plugin '%s' since file '%s' cannot be read.",
                                           info->name,                                   
                                           info->module_name);
                                break;

                        default:
                                g_return_val_if_reached (FALSE);                                
                }
                           
                g_object_unref (G_OBJECT (info->module));
                info->module = NULL;

                /* Mark plugin as unavailable and fails */
                info->available = FALSE;
                
                return FALSE;
        }
        
        switch (info->loader)
        {
                case ARIO_PLUGIN_LOADER_C:
                        info->plugin = 
                                ARIO_PLUGIN (ario_module_new_object (ARIO_MODULE (info->module)));
                        break;

#ifdef ENABLE_PYTHON
                case ARIO_PLUGIN_LOADER_PY:
                        info->plugin = 
                                ARIO_PLUGIN (ario_python_module_new_object (ARIO_PYTHON_MODULE (info->module)));
                        break;
#endif

                default:
                        g_return_val_if_reached (FALSE);
        }
        
        g_type_module_unuse (info->module);

        ARIO_LOG_DBG ("End");
        
        return TRUE;
}

static void
save_active_plugin_list (ArioPluginsEngine *engine)
{
        GSList *active_plugins = NULL;
        GList *l;
        gboolean res;

        for (l = engine->priv->plugin_list; l != NULL; l = l->next) {
                const ArioPluginInfo *info = (const ArioPluginInfo *) l->data;
                if (info->active) {
                        active_plugins = g_slist_prepend (active_plugins,
                                                          info->module_name);
                }
        }

        res = gconf_client_set_list (engine->priv->gconf_client,
                                     ARIO_PLUGINS_ENGINE_KEY,
                                     GCONF_VALUE_STRING,
                                     active_plugins,
                                     NULL);

        if (!res)
                g_warning ("Error saving the list of active plugins.");

        g_slist_free (active_plugins);
}

static void
ario_plugins_engine_activate_plugin_real (ArioPluginsEngine *engine,
                                           ArioPluginInfo *info)
{
        gboolean res = TRUE;

        if (info->active || !info->available)
                return;

        if (info->plugin == NULL)
                res = load_plugin_module (info);

        if (res) {
                ario_plugin_activate (info->plugin, static_shell);
                info->active = TRUE;
        } else {
                g_warning ("Error activating plugin '%s'", info->name);
        }
}

gboolean
ario_plugins_engine_activate_plugin (ArioPluginsEngine *engine,
                                      ArioPluginInfo    *info)
{
        g_return_val_if_fail (info != NULL, FALSE);

        if (!info->available)
                return FALSE;
                
        if (info->active)
                return TRUE;

        g_signal_emit (engine, signals[ACTIVATE_PLUGIN], 0, info);
        if (info->active)
                save_active_plugin_list (engine);

        return info->active;
}

static void
ario_plugins_engine_deactivate_plugin_real (ArioPluginsEngine *engine,
                                             ArioPluginInfo *info)
{
        if (!info->active || !info->available)
                return;

        ario_plugin_deactivate (info->plugin, static_shell);

        info->active = FALSE;
}

gboolean
ario_plugins_engine_deactivate_plugin (ArioPluginsEngine *engine,
                                        ArioPluginInfo    *info)
{
        g_return_val_if_fail (info != NULL, FALSE);

        if (!info->active || !info->available)
                return TRUE;

        g_signal_emit (engine, signals[DEACTIVATE_PLUGIN], 0, info);
        if (!info->active)
                save_active_plugin_list (engine);

        return !info->active;
}

static void
reactivate_all (ArioPluginsEngine *engine,
                ArioShell        *shell)
{
        GList *pl;

        for (pl = engine->priv->plugin_list; pl; pl = pl->next) {
                gboolean res = TRUE;
                
                ArioPluginInfo *info = (ArioPluginInfo*)pl->data;

                /* If plugin is not available, don't try to activate/load it */
                if (info->available && info->active) {                
                        if (info->plugin == NULL)
                                res = load_plugin_module (info);
                                
                        if (res)
                                ario_plugin_activate (info->plugin,
                                                       shell);                        
                }
        }
        
        ARIO_LOG_DBG ("End");
}

void
ario_plugins_engine_update_plugins_ui (ArioPluginsEngine *engine,
                                        ArioShell        *shell,
                                        gboolean            new_shell)
{
        GList *pl;

        g_return_if_fail (IS_ARIO_SHELL (shell));

        if (new_shell)
                reactivate_all (engine, shell);

        /* updated ui of all the plugins that implement update_ui */
        for (pl = engine->priv->plugin_list; pl; pl = pl->next) {
                ArioPluginInfo *info = (ArioPluginInfo*)pl->data;

                if (!info->available || !info->active)
                        continue;
                        
                       ARIO_LOG_DBG ("Updating UI of %s", info->name);
                
                ario_plugin_update_ui (info->plugin, shell);
        }
}

void          
ario_plugins_engine_configure_plugin (ArioPluginsEngine *engine,
                                       ArioPluginInfo    *info,
                                       GtkWindow          *parent)
{
        GtkWidget *conf_dlg;
        
        GtkWindowGroup *wg;
        
        g_return_if_fail (info != NULL);

        conf_dlg = ario_plugin_create_configure_dialog (info->plugin);
        g_return_if_fail (conf_dlg != NULL);
        gtk_window_set_transient_for (GTK_WINDOW (conf_dlg),
                                      parent);

        wg = parent->group;                      
        if (wg == NULL) {
                wg = gtk_window_group_new ();
                gtk_window_group_add_window (wg, parent);
        }
                        
        gtk_window_group_add_window (wg,
                                     GTK_WINDOW (conf_dlg));
                
        gtk_window_set_modal (GTK_WINDOW (conf_dlg), TRUE);                     
        gtk_widget_show (conf_dlg);
}

static void 
ario_plugins_engine_active_plugins_changed (GConfClient *client,
                                             guint cnxn_id,
                                             GConfEntry *entry,
                                             gpointer user_data)
{
        ArioPluginsEngine *engine;
        GList *pl;
        gboolean to_activate;
        GSList *active_plugins;

        g_return_if_fail (entry->key != NULL);
        g_return_if_fail (entry->value != NULL);

        engine = ARIO_PLUGINS_ENGINE (user_data);
        
        if (!((entry->value->type == GCONF_VALUE_LIST) && 
              (gconf_value_get_list_type (entry->value) == GCONF_VALUE_STRING))) {
                g_warning ("The gconf key '%s' may be corrupted.", ARIO_PLUGINS_ENGINE_KEY);
                return;
        }
        
        active_plugins = gconf_client_get_list (engine->priv->gconf_client,
                                                ARIO_PLUGINS_ENGINE_KEY,
                                                GCONF_VALUE_STRING,
                                                NULL);

        for (pl = engine->priv->plugin_list; pl; pl = pl->next) {
                ArioPluginInfo *info = (ArioPluginInfo*)pl->data;

                if (!info->available)
                        continue;

                to_activate = (g_slist_find_custom (active_plugins,
                                                    info->module_name,
                                                    (GCompareFunc)strcmp) != NULL);

                if (!info->active && to_activate)
                        g_signal_emit (engine, signals[ACTIVATE_PLUGIN], 0, info);
                else if (info->active && !to_activate)
                        g_signal_emit (engine, signals[DEACTIVATE_PLUGIN], 0, info);
        }

        g_slist_foreach (active_plugins, (GFunc) g_free, NULL);
        g_slist_free (active_plugins);
}


