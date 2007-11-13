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
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <curl/curl.h>
#include "lib/eel-gconf-extensions.h"
#include "shell/ario-shell.h"
#include "ario-util.h"
#include "ario-debug.h"

int
main (int argc, char *argv[])
{
        ARIO_LOG_FUNCTION_START

        ArioShell *shell;

#ifdef ENABLE_NLS
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
#endif

        gtk_set_locale ();
        gtk_init (&argc, &argv);

        ario_util_init_stock_icons ();
        curl_global_init(0);
        if (!g_thread_supported ()) g_thread_init (NULL);

        eel_gconf_monitor_add ("/apps/ario");
        shell = ario_shell_new ();
        ario_shell_construct (shell);

        gtk_main ();

        ario_shell_shutdown (shell);
        eel_gconf_monitor_remove ("/apps/ario");


        g_object_unref (G_OBJECT (shell));

        return 0;
}

