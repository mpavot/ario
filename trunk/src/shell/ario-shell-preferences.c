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

#include "shell/ario-shell-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-browser-preferences.h"
#include "preferences/ario-connection-preferences.h"
#include "preferences/ario-cover-preferences.h"
#include "preferences/ario-lyrics-preferences.h"
#include "preferences/ario-interface-preferences.h"
#include "preferences/ario-proxy-preferences.h"
#include "preferences/ario-server-preferences.h"
#include "preferences/ario-trayicon-preferences.h"
#include "preferences/ario-stats-preferences.h"
#include "ario-debug.h"

static gboolean ario_shell_preferences_window_delete_cb (GtkWidget *window,
                                                         GdkEventAny *event,
                                                         ArioShellPreferences *shell_preferences);
static void ario_shell_preferences_response_cb (GtkDialog *dialog,
                                                int response_id,
                                                ArioShellPreferences *shell_preferences);
struct ArioShellPreferencesPrivate
{
        GtkWidget *notebook;
};

#define ARIO_SHELL_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_PREFERENCES, ArioShellPreferencesPrivate))
G_DEFINE_TYPE (ArioShellPreferences, ario_shell_preferences, GTK_TYPE_DIALOG)

static void
ario_shell_preferences_class_init (ArioShellPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioShellPreferencesPrivate));
}

static void
ario_shell_preferences_init (ArioShellPreferences *shell_preferences)
{
        ARIO_LOG_FUNCTION_START
        shell_preferences->priv = ARIO_SHELL_PREFERENCES_GET_PRIVATE (shell_preferences);

        g_signal_connect (shell_preferences,
                          "delete_event",
                          G_CALLBACK (ario_shell_preferences_window_delete_cb),
                          shell_preferences);
        g_signal_connect (shell_preferences,
                          "response",
                          G_CALLBACK (ario_shell_preferences_response_cb),
                          shell_preferences);

        gtk_dialog_add_button (GTK_DIALOG (shell_preferences),
                               GTK_STOCK_CLOSE,
                               GTK_RESPONSE_CLOSE);

        gtk_dialog_set_default_response (GTK_DIALOG (shell_preferences),
                                         GTK_RESPONSE_CLOSE);

        gtk_window_set_title (GTK_WINDOW (shell_preferences), _("Ario Preferences"));
        gtk_window_set_resizable (GTK_WINDOW (shell_preferences), FALSE);

        shell_preferences->priv->notebook = GTK_WIDGET (gtk_notebook_new ());
        gtk_container_set_border_width (GTK_CONTAINER (shell_preferences->priv->notebook), 5);

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell_preferences)->vbox),
                           shell_preferences->priv->notebook);

        gtk_container_set_border_width (GTK_CONTAINER (shell_preferences), 5);
        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (shell_preferences)->vbox), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (shell_preferences), FALSE);
}

GtkWidget *
ario_shell_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioShellPreferences *shell_preferences;
        GtkWidget *widget;

        shell_preferences = g_object_new (TYPE_ARIO_SHELL_PREFERENCES, NULL);

        g_return_val_if_fail (shell_preferences->priv != NULL, NULL);

        widget = ario_connection_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Connection")));

        widget = ario_server_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Server")));

        widget = ario_browser_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Library")));

        widget = ario_cover_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Covers")));

        widget = ario_lyrics_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Lyrics")));

        widget = ario_interface_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Interface")));

        widget = ario_trayicon_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Tray icon")));

        widget = ario_proxy_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Proxy")));

        widget = ario_stats_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Statistics")));

        return GTK_WIDGET (shell_preferences);
}

static gboolean
ario_shell_preferences_window_delete_cb (GtkWidget *window,
                                         GdkEventAny *event,
                                         ArioShellPreferences *shell_preferences)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (shell_preferences));
        gtk_widget_destroy (GTK_WIDGET (shell_preferences));

        return TRUE;
}

static void
ario_shell_preferences_response_cb (GtkDialog *dialog,
                                    int response_id,
                                    ArioShellPreferences *shell_preferences)
{
        ARIO_LOG_FUNCTION_START
        if (response_id == GTK_RESPONSE_CLOSE) {
                gtk_widget_hide (GTK_WIDGET (shell_preferences));
                gtk_widget_destroy (GTK_WIDGET (shell_preferences));
        }
}

