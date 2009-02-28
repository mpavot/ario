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

#ifdef WIN32
#include <windows.h>
#else
#include "lib/bacon-message-connection.h"
#endif

static ArioShell *shell;
static gboolean minimized = FALSE;

#ifndef WIN32
static void
ario_main_on_message_received (const char *message,
                               gpointer data)
{
        ario_shell_present (shell);
}
#endif

int
main (int argc, char *argv[])
{
        ARIO_LOG_FUNCTION_START;

        GOptionContext *context;
        static const GOptionEntry options []  = {
                { "minimized",           'm', 0, G_OPTION_ARG_NONE,         &minimized,           N_("Start minimized window"), NULL },
                { NULL }
        };

        context = g_option_context_new (NULL);
        g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
        g_option_context_add_group (context, gtk_get_option_group (TRUE));

        g_option_context_parse (context, &argc, &argv, NULL);
        g_option_context_free (context);

        ario_conf_init ();

#ifdef WIN32
        HANDLE hMutex;
        hMutex = CreateMutex (NULL, FALSE, "ArioMain");
        if (ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT)
            && (GetLastError() == ERROR_ALREADY_EXISTS)) {
                ARIO_LOG_INFO ("Ario is already running\n");
                return 0;
        }
#else
        BaconMessageConnection *bacon_connection = bacon_message_connection_new ("ario");
        if (bacon_connection) {
                if (ario_conf_get_boolean (PREF_ONE_INSTANCE, PREF_ONE_INSTANCE_DEFAULT)
                    && !bacon_message_connection_get_is_server (bacon_connection)) {
                        ARIO_LOG_INFO ("Ario is already running\n");
                        bacon_message_connection_send (bacon_connection, "PRESENT");
                        return 0;
                }
                bacon_message_connection_set_callback (bacon_connection,
                                                       ario_main_on_message_received,
                                                       NULL);
        }
#endif

#ifdef ENABLE_NLS
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
#endif

        if (!g_thread_supported ()) g_thread_init (NULL);

        gtk_set_locale ();
        gtk_init (&argc, &argv);

        ario_util_init_stock_icons ();
        curl_global_init (CURL_GLOBAL_WIN32);

        shell = ario_shell_new ();
        ario_shell_construct (shell, minimized);
        ario_plugins_engine_init (shell);

        gtk_main ();

        ario_shell_shutdown (shell);

        g_object_unref (G_OBJECT (shell));

        ario_plugins_engine_shutdown ();
        ario_conf_shutdown ();

        xmlCleanupParser ();

        return 0;
}

