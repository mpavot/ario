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
#include <gdk/gdk.h>
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
#endif

static ArioShell *shell = NULL;
static gboolean minimized = FALSE;

static void
activate (GtkApplication *app)
{
        ario_shell_present (shell);
}

int
main (int argc, char *argv[])
{
        ARIO_LOG_FUNCTION_START;
        GError *error = NULL;
        gint status = 0;
#ifdef WIN32
        ShowWindow (GetConsoleWindow(), SW_HIDE);
#endif
        /* Parse options */
        GOptionContext *context;
        gchar *profile = NULL;
        const GOptionEntry options []  = {
                { "minimized", 'm', 0, G_OPTION_ARG_NONE, &minimized, N_("Start minimized window"), NULL },
                { "profile", 'p', 0, G_OPTION_ARG_STRING, &profile, N_("Start with specific profile"), NULL },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        context = g_option_context_new (NULL);
        g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
        g_option_context_add_group (context, gtk_get_option_group (TRUE));

        if (!g_option_context_parse (context, &argc, &argv, &error)) {
                g_print ("option parsing failed: %s\n", error->message);
                exit (1);
        }
        g_option_context_free (context);

        /* Initialisation of configurations engine */
        ario_conf_init ();

        /* Check in an instance of Ario is already running */
#ifdef WIN32
        CreateMutex (NULL, FALSE, "ArioMain");
        if (ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT)
            && (GetLastError() == ERROR_ALREADY_EXISTS)) {
                ARIO_LOG_INFO ("Ario is already running\n");
                return 0;
        }
#endif

        /* Initialisation of translation */
#ifdef ENABLE_NLS
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
#endif

        /* Register Ario icons */
        ario_util_init_icons ();

        /* Initialisation of Curl */
        curl_global_init (CURL_GLOBAL_WIN32);

#ifndef WIN32
        /* Set a specific profile */
        if (profile)
                ario_profiles_set_current_by_name (profile);
#endif
        /* Creates Ario main window */
        GtkApplication *app;
        app = gtk_application_new ("org.Ario", G_APPLICATION_FLAGS_NONE);
        g_application_register (G_APPLICATION (app), NULL, NULL);
        if (g_application_get_is_remote (G_APPLICATION (app))) {
                g_application_activate (G_APPLICATION (app));
                g_object_unref (app);
                return 0;
        }
        shell = ario_shell_new (app);
        gtk_window_set_application (GTK_WINDOW (shell), app);
        ario_shell_construct (shell, minimized);
        g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

        /* Initialisation of plugins engine */
        ario_plugins_engine_init (shell);

        /* Starts GTK main loop */
        status = g_application_run (G_APPLICATION (app), argc, argv);

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

        return status;
}

