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
#include <glade/glade.h>
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "ario-profiles.h"
#include "widgets/ario-connection-widget.h"
#include "ario-debug.h"

struct ArioFirstlaunchPrivate
{
        GtkWidget *final_label;
};

#define ARIO_FIRSTLAUNCH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_FIRSTLAUNCH, ArioFirstlaunchPrivate))
G_DEFINE_TYPE (ArioFirstlaunch, ario_firstlaunch, GTK_TYPE_ASSISTANT)

static void
ario_firstlaunch_class_init (ArioFirstlaunchClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioFirstlaunchPrivate));
}

static void
ario_firstlaunch_cancel_cb (GtkWidget *widget,
                            ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        gtk_main_quit ();
}

static void
ario_firstlaunch_apply_cb (GtkWidget *widget,
                           ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_boolean (PREF_FIRST_TIME, TRUE);
        gtk_widget_destroy (GTK_WIDGET (firstlaunch));
}

static void
ario_firstlaunch_page_prepare_cb (GtkAssistant *assistant,
                                  GtkWidget    *page,
                                  ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        gchar *text;
        ArioProfile *profile;

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
        ARIO_LOG_FUNCTION_START
        GdkPixbuf *pixbuf;
        GtkWidget *label, *vbox, *connection_vbox;
        GladeXML *xml;

        firstlaunch->priv = ARIO_FIRSTLAUNCH_GET_PRIVATE (firstlaunch);
        pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "ario.png", NULL);

        /* Page 1 */
        vbox = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

        label = gtk_label_new (_("It is the first time you launch Ario.\nThis assistant will help you to configure it."));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Welcome to Ario"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_INTRO);
        gtk_assistant_set_page_header_image (GTK_ASSISTANT (firstlaunch), vbox, pixbuf);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);

        /* Page 2 */
        xml = rb_glade_xml_new (GLADE_PATH "connection-assistant.glade",
                                "vbox",
                                firstlaunch);

        vbox = glade_xml_get_widget (xml, "vbox");
        connection_vbox = glade_xml_get_widget (xml, "connection_vbox");

        gtk_box_pack_start (GTK_BOX (connection_vbox),
                            ario_connection_widget_new (),
                            TRUE, TRUE, 0);

        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        g_object_unref (xml);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Configuration"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_CONTENT);
        gtk_assistant_set_page_header_image (GTK_ASSISTANT (firstlaunch), vbox, pixbuf);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);

        /* Page 3 */
        firstlaunch->priv->final_label = gtk_label_new (NULL);
        gtk_label_set_line_wrap (GTK_LABEL (firstlaunch->priv->final_label), TRUE);
        vbox = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
        gtk_box_pack_start (GTK_BOX (vbox), firstlaunch->priv->final_label, FALSE, FALSE, 0);
        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Confirmation"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_CONFIRM);
        gtk_assistant_set_page_header_image (GTK_ASSISTANT (firstlaunch), vbox, pixbuf);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);

        g_object_unref (pixbuf);

        g_signal_connect (firstlaunch,
                          "cancel",
                          G_CALLBACK (ario_firstlaunch_cancel_cb),
                          firstlaunch);
        g_signal_connect (firstlaunch,
                          "close",
                          G_CALLBACK (ario_firstlaunch_apply_cb),
                          firstlaunch);

        gtk_window_set_position (GTK_WINDOW (firstlaunch), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size (GTK_WINDOW (firstlaunch), 400, 450);

        g_signal_connect (firstlaunch,
                          "prepare",
                          G_CALLBACK (ario_firstlaunch_page_prepare_cb),
                          firstlaunch);
}

ArioFirstlaunch *
ario_firstlaunch_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioFirstlaunch *firstlaunch;

        firstlaunch = g_object_new (TYPE_ARIO_FIRSTLAUNCH, NULL);

        return firstlaunch;
}


