/*
 *  Copyright (C) 2007 Marc Pavot <marc.pavot@gmail.com>
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

#include "widgets/ario-firstlaunch.h"
#include <gtk/gtk.h>
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include "lib/gtk-builder-helpers.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "ario-profiles.h"
#include "widgets/ario-connection-widget.h"
#include "ario-debug.h"

struct ArioFirstlaunchPrivate
{
        GtkApplication * app;
        GtkWidget *final_label;
};

G_DEFINE_TYPE_WITH_CODE (ArioFirstlaunch, ario_firstlaunch, GTK_TYPE_ASSISTANT, G_ADD_PRIVATE(ArioFirstlaunch))

static void
ario_firstlaunch_class_init (ArioFirstlaunchClass *klass)
{
        ARIO_LOG_FUNCTION_START;
}

static void
ario_firstlaunch_cancel_cb (GtkWidget *widget,
                            ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START;
        /* Exit Ario if user click on cancel */
        g_application_quit (G_APPLICATION (firstlaunch->priv->app));
}

static void
ario_firstlaunch_apply_cb (GtkWidget *widget,
                           ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START;
        /* Remember that first launch assistant has already been launched */
        ario_conf_set_boolean (PREF_FIRST_TIME, TRUE);
        gtk_widget_destroy (GTK_WIDGET (firstlaunch));
}

static void
ario_firstlaunch_page_prepare_cb (GtkAssistant *assistant,
                                  GtkWidget    *page,
                                  ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START;
        gchar *text;
        ArioProfile *profile;

        /* Create final page with current profile information */
        profile = ario_profiles_get_current (ario_profiles_get ());

        text = g_strdup_printf ("%s \n\n%s <b>%s</b>\n%s <b>%d</b>",
                                _("The following configuration will be used:"),
                                _("Host :"),
                                profile->host,
                                _("Port :"),
                                profile->port);
        gtk_label_set_markup (GTK_LABEL (firstlaunch->priv->final_label), text);
        g_free (text);
}

static void
ario_firstlaunch_init (ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *label, *vbox, *connection_vbox;
        GtkBuilder *builder;

        firstlaunch->priv = ario_firstlaunch_get_instance_private (firstlaunch);

        /* Page 1 - Presentation of first launch assistant */
        vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

        label = gtk_label_new (_("It is the first time you launch Ario.\nThis assistant will help you to configure it."));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Welcome to Ario"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_INTRO);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);

        /* Page 2 - Connection configuration */
        builder = gtk_builder_helpers_new (UI_PATH "connection-assistant.ui",
                                           firstlaunch);

        vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));
        connection_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "connection_vbox"));

        gtk_box_pack_start (GTK_BOX (connection_vbox),
                            ario_connection_widget_new (),
                            TRUE, TRUE, 0);

        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        g_object_unref (builder);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Configuration"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_CONTENT);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);

        /* Page 3 - Confirmation */
        firstlaunch->priv->final_label = gtk_label_new (NULL);
        gtk_label_set_line_wrap (GTK_LABEL (firstlaunch->priv->final_label), TRUE);
        vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
        gtk_box_pack_start (GTK_BOX (vbox), firstlaunch->priv->final_label, FALSE, FALSE, 0);
        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Confirmation"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_CONFIRM);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);

        /* Connect signals for actions on assistant */
        g_signal_connect (firstlaunch,
                          "cancel",
                          G_CALLBACK (ario_firstlaunch_cancel_cb),
                          firstlaunch);
        g_signal_connect (firstlaunch,
                          "close",
                          G_CALLBACK (ario_firstlaunch_apply_cb),
                          firstlaunch);

        /* Window properties */
        gtk_window_set_position (GTK_WINDOW (firstlaunch), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size (GTK_WINDOW (firstlaunch), 400, 450);

        g_signal_connect (firstlaunch,
                          "prepare",
                          G_CALLBACK (ario_firstlaunch_page_prepare_cb),
                          firstlaunch);
}

ArioFirstlaunch *
ario_firstlaunch_new (GtkApplication * app)
{
        ARIO_LOG_FUNCTION_START;
        ArioFirstlaunch *firstlaunch;

        firstlaunch = g_object_new (TYPE_ARIO_FIRSTLAUNCH, NULL);
        firstlaunch->priv->app = app;

        return firstlaunch;
}


