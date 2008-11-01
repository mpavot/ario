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

#include "plugins/ario-plugins-engine.h"
#include "plugins/ario-plugin-info-priv.h"
#include "plugins/ario-plugin.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "lib/ario-conf.h"

#include "ario-module.h"
#ifdef ENABLE_PYTHON
#include "ario-python-module.h"
#endif

#define PLUGIN_EXT ".ario-plugin"

static void ario_plugins_engine_active_plugins_changed (void);
static void ario_plugins_engine_activate_plugin_real (ArioPluginInfo* info);
static void ario_plugins_engine_deactivate_plugin_real (ArioPluginInfo* info);

static gint
compare_plugin_info (ArioPluginInfo *info1,
                     ArioPluginInfo *info2)
{
        return strcmp (info1->module_name, info2->module_name);
}

static GList *plugin_list;
static ArioShell *static_shell;

static void
ario_plugins_engine_load_dir (const gchar        *dir,
                              GSList             *active_plugins)
{
        GError *error = NULL;
        GDir *d;
        const gchar *dirent;

        g_return_if_fail (dir != NULL);

        ARIO_LOG_DBG ("DIR: %s", dir);

        d = g_dir_open (dir, 0, &error);
        if (!d) {
                g_warning ("%s", error->message);
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
                        if (g_list_find_custom (plugin_list,
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

                        plugin_list = g_list_prepend (plugin_list, info);

                        ARIO_LOG_DBG ("Plugin %s loaded", info->name);
                }
        }

        g_dir_close (d);
}

static void
ario_plugins_engine_load_all (void)
{
        GSList *active_plugins;
        GSList *paths;
        GSList *l;

        active_plugins = ario_conf_get_string_slist (PREF_PLUGINS_LIST, PREF_PLUGINS_LIST_DEFAULT);

        paths = ario_plugin_get_plugin_paths ();

        for (l = paths; l != NULL; l = l->next) {
                if (g_file_test (l->data, G_FILE_TEST_IS_DIR))
                        ario_plugins_engine_load_dir (l->data, active_plugins);
        }

        g_slist_foreach (paths, (GFunc) g_free, NULL);
        g_slist_free (paths);

        g_slist_foreach (active_plugins, (GFunc) g_free, NULL);
        g_slist_free (active_plugins);
}

static void
ario_plugins_engine_load_icons_dir (const gchar        *dir)
{
        GError *error = NULL;
        GDir *d;
        const gchar *dirent;

        g_return_if_fail (dir != NULL);

        ARIO_LOG_DBG ("ICONS DIR: %s", dir);

        d = g_dir_open (dir, 0, &error);
        if (!d) {
                g_warning ("%s", error->message);
                g_error_free (error);
                return;
        }

        while ((dirent = g_dir_read_name (d))) {
                if (g_str_has_suffix (dirent, "png") || g_str_has_suffix (dirent, "jpg")) {
                        gchar *icon_file;

                        icon_file = g_build_filename (dir, dirent, NULL);
                        ario_util_add_stock_icons (dirent, icon_file);
                        g_free (icon_file);
                }
        }

        g_dir_close (d);
}

static void
ario_plugins_engine_load_icons_all (void)
{
        GSList *paths;
        GSList *l;
        gchar *tmp;

        paths = ario_plugin_get_plugin_paths ();

        for (l = paths; l != NULL; l = l->next) {
                tmp = g_build_filename (l->data, "icons", NULL);
                if (g_file_test (tmp, G_FILE_TEST_IS_DIR))
                        ario_plugins_engine_load_icons_dir (tmp);
                g_free (tmp);
        }

        g_slist_foreach (paths, (GFunc) g_free, NULL);
        g_slist_free (paths);
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

                        info->module = G_TYPE_MODULE (ario_python_module_new (dir, info->module_name));

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
reactivate_all ()
{
        GList *pl;

        for (pl = plugin_list; pl; pl = pl->next) {
                gboolean res = TRUE;

                ArioPluginInfo *info = (ArioPluginInfo*)pl->data;

                /* If plugin is not available, don't try to activate/load it */
                if (info->available && info->active) {                
                        if (info->plugin == NULL)
                                res = load_plugin_module (info);

                        if (res)
                                ario_plugin_activate (info->plugin,
                                                      static_shell);                        
                }
        }

        ARIO_LOG_DBG ("End");
}

void
ario_plugins_engine_init (ArioShell *shell)
{
        static_shell = shell;

        ario_plugins_engine_load_all ();

        ario_plugins_engine_load_icons_all ();

        reactivate_all (shell);
}

void
ario_plugins_engine_shutdown ()
{
#ifdef ENABLE_PYTHON
        /* Note: that this may cause finalization of objects (typically
         * the ArioShell) by running the garbage collector. Since some
         * of the plugin may have installed callbacks upon object
         * finalization (typically they need to free the WindowData)
         * it must run before we get rid of the plugins.
         */
        ario_python_shutdown ();
#endif

        g_list_foreach (plugin_list,
                        (GFunc) _ario_plugin_info_unref, NULL);
        g_list_free (plugin_list);
}

const GList *
ario_plugins_engine_get_plugin_list ()
{
        return plugin_list;
}

static void
save_active_plugin_list ()
{
        GSList *active_plugins = NULL;
        GList *l;

        for (l = plugin_list; l != NULL; l = l->next) {
                const ArioPluginInfo *info = (const ArioPluginInfo *) l->data;
                if (info->active) {
                        active_plugins = g_slist_prepend (active_plugins,
                                                          info->module_name);
                }
        }

        ario_conf_set_string_slist (PREF_PLUGINS_LIST, active_plugins);
        g_slist_free (active_plugins);

        ario_plugins_engine_active_plugins_changed ();
}

static void
ario_plugins_engine_activate_plugin_real (ArioPluginInfo *info)
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
ario_plugins_engine_activate_plugin (ArioPluginInfo    *info)
{
        g_return_val_if_fail (info != NULL, FALSE);

        if (!info->available)
                return FALSE;

        if (info->active)
                return TRUE;

        ario_plugins_engine_activate_plugin_real (info);
        if (info->active)
                save_active_plugin_list ();

        return info->active;
}

static void
ario_plugins_engine_deactivate_plugin_real (ArioPluginInfo *info)
{
        if (!info->active || !info->available)
                return;

        ario_plugin_deactivate (info->plugin, static_shell);

        info->active = FALSE;
}

gboolean
ario_plugins_engine_deactivate_plugin (ArioPluginInfo    *info)
{
        g_return_val_if_fail (info != NULL, FALSE);

        if (!info->active || !info->available)
                return TRUE;

        ario_plugins_engine_deactivate_plugin_real (info);
        if (!info->active)
                save_active_plugin_list ();

        return !info->active;
}

void          
ario_plugins_engine_configure_plugin (ArioPluginInfo    *info,
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
ario_plugins_engine_active_plugins_changed (void)
{
        GList *pl;
        gboolean to_activate;
        GSList *active_plugins;

        active_plugins = ario_conf_get_string_slist (PREF_PLUGINS_LIST, PREF_PLUGINS_LIST_DEFAULT);

        for (pl = plugin_list; pl; pl = pl->next) {
                ArioPluginInfo *info = (ArioPluginInfo*)pl->data;

                if (!info->available)
                        continue;

                to_activate = (g_slist_find_custom (active_plugins,
                                                    info->module_name,
                                                    (GCompareFunc)strcmp) != NULL);

                if (!info->active && to_activate)
                        ario_plugins_engine_activate_plugin_real (info);
                else if (info->active && !to_activate)
                        ario_plugins_engine_deactivate_plugin_real (info);
        }

        g_slist_foreach (active_plugins, (GFunc) g_free, NULL);
        g_slist_free (active_plugins);
}

#ifdef ENABLE_PYTHON
void
ario_plugins_engine_garbage_collect (void)
{
        ario_python_garbage_collect ();
}
#endif


