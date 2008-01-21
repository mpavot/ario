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
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "shell/ario-shell-preferences.h"
#include "preferences/ario-connection-preferences.h"
#include "preferences/ario-cover-preferences.h"
#include "preferences/ario-interface-preferences.h"
#include "preferences/ario-server-preferences.h"
#include "ario-debug.h"

static void ario_shell_preferences_class_init (ArioShellPreferencesClass *klass);
static void ario_shell_preferences_init (ArioShellPreferences *shell_preferences);
static void ario_shell_preferences_finalize (GObject *object);
static void ario_shell_preferences_set_property (GObject *object,
                                                 guint prop_id,
                                                 const GValue *value,
                                                 GParamSpec *pspec);
static void ario_shell_preferences_get_property (GObject *object,
                                                 guint prop_id,
                                                 GValue *value,
                                                 GParamSpec *pspec);
static gboolean ario_shell_preferences_window_delete_cb (GtkWidget *window,
                                                         GdkEventAny *event,
                                                         ArioShellPreferences *shell_preferences);
static void ario_shell_preferences_response_cb (GtkDialog *dialog,
                                                int response_id,
                                                ArioShellPreferences *shell_preferences);
enum
{
        PROP_0,
        PROP_MPD
};

struct ArioShellPreferencesPrivate
{
        GtkWidget *notebook;

        ArioMpd *mpd;
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_preferences_type = 0;

        if (ario_shell_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellPreferences),
                        0,
                        (GInstanceInitFunc) ario_shell_preferences_init
                };

                ario_shell_preferences_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                                      "ArioShellPreferences",
                                                                      &our_info, 0);
        }

        return ario_shell_preferences_type;
}

static void
ario_shell_preferences_class_init (ArioShellPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_preferences_finalize;
        object_class->set_property = ario_shell_preferences_set_property;
        object_class->get_property = ario_shell_preferences_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_shell_preferences_init (ArioShellPreferences *shell_preferences)
{
        ARIO_LOG_FUNCTION_START
        shell_preferences->priv = g_new0 (ArioShellPreferencesPrivate, 1);

        g_signal_connect_object (G_OBJECT (shell_preferences),
                                 "delete_event",
                                 G_CALLBACK (ario_shell_preferences_window_delete_cb),
                                 shell_preferences, 0);
        g_signal_connect_object (G_OBJECT (shell_preferences),
                                 "response",
                                 G_CALLBACK (ario_shell_preferences_response_cb),
                                 shell_preferences, 0);

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
ario_shell_preferences_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioShellPreferences *shell_preferences;
        GtkWidget *widget;

        shell_preferences = g_object_new (TYPE_ARIO_SHELL_PREFERENCES,
                                          "mpd", mpd,
                                          NULL);

        g_return_val_if_fail (shell_preferences->priv != NULL, NULL);

        widget = ario_connection_preferences_new (mpd);
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Connection")));

        widget = ario_server_preferences_new (mpd);
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Server")));

        widget = ario_cover_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Covers")));

        widget = ario_interface_preferences_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_preferences->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Interface")));

        return GTK_WIDGET (shell_preferences);
}

static void
ario_shell_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellPreferences *shell_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_PREFERENCES (object));

        shell_preferences = ARIO_SHELL_PREFERENCES (object);

        g_return_if_fail (shell_preferences->priv != NULL);

        g_free (shell_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_shell_preferences_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellPreferences *shell_preferences = ARIO_SHELL_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                shell_preferences->priv->mpd = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_shell_preferences_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellPreferences *shell_preferences = ARIO_SHELL_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, shell_preferences->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
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

