/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
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

#include <config.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "shell/ario-shell.h"
#include "plugins/ario-plugins-engine.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "ario-profiles.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unique/unique.h>
#endif

#ifndef WIN32
static UniqueResponse
ario_main_on_message_received (G_GNUC_UNUSED UniqueApp *app,
                               G_GNUC_UNUSED UniqueCommand command,
                               G_GNUC_UNUSED UniqueMessageData *message,
                               G_GNUC_UNUSED guint time_,
                               ArioShell *shell)
{
        ario_shell_present (shell);
        return UNIQUE_RESPONSE_OK;
}
#endif

int
main (int argc, char *argv[])
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell;

        /* Parse options */
        GOptionContext *context;
        gchar *profile = NULL;
        gboolean minimized = FALSE;
        const GOptionEntry options []  = {
                { "minimized", 'm', 0, G_OPTION_ARG_NONE, &minimized, N_("Start minimized window"), NULL },
                { "profile", 'p', 0, G_OPTION_ARG_STRING, &profile, N_("Start with specific profile"), NULL },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        context = g_option_context_new (NULL);
        g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
        g_option_context_add_group (context, gtk_get_option_group (TRUE));

        g_option_context_parse (context, &argc, &argv, NULL);
        g_option_context_free (context);

        /* Initialisation of configurations engine */
        ario_conf_init ();

        /* Check in an instance of Ario is already running */
#ifdef WIN32
        HANDLE hMutex;
        hMutex = CreateMutex (NULL, FALSE, "ArioMain");
        if (ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT)
            && (GetLastError() == ERROR_ALREADY_EXISTS)) {
                ARIO_LOG_INFO ("Ario is already running\n");
                return 0;
        }
#else
        UniqueApp *app;
        app = unique_app_new ("org.Ario", NULL);
        if (unique_app_is_running (app)) {
                if (ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT)) {
                        unique_app_send_message (app, UNIQUE_ACTIVATE, NULL);
                        g_object_unref (app);
                        return 0;
                }
        }
#endif

        /* Initialisation of translation */
#ifdef ENABLE_NLS
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
#endif

        /* Initialisation of threads */
        if (!g_thread_supported ()) g_thread_init (NULL);

        /* Initialisation of GTK */
        gtk_set_locale ();
        gtk_init (&argc, &argv);

        /* Register Ario icons */
        ario_util_init_stock_icons ();

        /* Initialisation of Curl */
        curl_global_init (CURL_GLOBAL_WIN32);

        /* Set a specific profile */
        if (profile)
                ario_profiles_set_current_by_name (profile);

        /* Creates Ario main window */
        shell = ario_shell_new ();
        ario_shell_construct (shell, minimized);

#ifndef WIN32
        unique_app_watch_window (app, GTK_WINDOW (shell));
        g_signal_connect (app, "message-received", G_CALLBACK (ario_main_on_message_received), shell);
#endif
        /* Initialisation of plugins engine */
        ario_plugins_engine_init (shell);

        /* Starts GTK main loop */
        gtk_main ();

        /* Shutdown main window */
        ario_shell_shutdown (shell);
        gtk_widget_destroy (GTK_WIDGET (shell));

        /* Shutdown plugins engine */
        ario_plugins_engine_shutdown ();

        /* Shutdown configurations engine */
        ario_conf_shutdown ();

#ifndef WIN32
        g_object_unref (app);
#endif

        /* Clean libxml stuff */
        xmlCleanupParser ();

        return 0;
}

