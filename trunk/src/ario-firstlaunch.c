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

#include <gtk/gtk.h>
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include "eel-gconf-extensions.h"
#include "ario-firstlaunch.h"
#include "ario-preferences.h"
#include "ario-debug.h"

static void ario_firstlaunch_class_init (ArioFirstlaunchClass *klass);
static void ario_firstlaunch_init (ArioFirstlaunch *firstlaunch);
static void ario_firstlaunch_finalize (GObject *object);

struct ArioFirstlaunchPrivate
{
        GtkWidget *host_entry;
        GtkWidget *port_entry;
        GtkWidget *final_label;

        gboolean applied;
};

static GObjectClass *parent_class;

GType
ario_firstlaunch_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;
                                                                              
        if (type == 0)
        { 
                static GTypeInfo info =
                {
                        sizeof (ArioFirstlaunchClass),
                        NULL, 
                        NULL,
                        (GClassInitFunc) ario_firstlaunch_class_init, 
                        NULL,
                        NULL, 
                        sizeof (ArioFirstlaunch),
                        0,
                        (GInstanceInitFunc) ario_firstlaunch_init
                };

                type = g_type_register_static (GTK_TYPE_ASSISTANT,
                                               "ArioFirstlaunch",
                                               &info, 0);
        }

        return type;
}

static void
ario_firstlaunch_class_init (ArioFirstlaunchClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_firstlaunch_finalize;
}

static void
ario_firstlaunch_cancel_cb (GtkWidget *widget,
                                  ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        if (!firstlaunch->priv->applied)
                gtk_main_quit ();
}

static void
ario_firstlaunch_apply_cb (GtkWidget *widget,
                           ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        eel_gconf_set_string (CONF_HOST,
                              gtk_entry_get_text (GTK_ENTRY (firstlaunch->priv->host_entry)));

        eel_gconf_set_integer (CONF_PORT,
                               atoi (gtk_entry_get_text (GTK_ENTRY (firstlaunch->priv->port_entry))));

        firstlaunch->priv->applied = TRUE;
        eel_gconf_set_boolean (CONF_FIRST_TIME, TRUE);
        gtk_widget_destroy (GTK_WIDGET (firstlaunch));
}

static void
ario_firstlaunch_page2_prepare_cb (GtkAssistant *assistant,
                                   GtkWidget    *page,
			           ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START

        gchar *text;
        text = g_strdup_printf (_("The following configuration will be used: \n\nHost: <b>%s</b>\nPort: <b>%s</b>"),
                                gtk_entry_get_text (GTK_ENTRY (firstlaunch->priv->host_entry)),
                                gtk_entry_get_text (GTK_ENTRY (firstlaunch->priv->port_entry)));
        gtk_label_set_markup (GTK_LABEL (firstlaunch->priv->final_label), text);
        g_free (text);
}

static void
ario_firstlaunch_init (ArioFirstlaunch *firstlaunch) 
{
        ARIO_LOG_FUNCTION_START
        GdkPixbuf *pixbuf;
        GtkWidget *label, *vbox, *table;

        firstlaunch->priv = g_new0 (ArioFirstlaunchPrivate, 1);
        firstlaunch->priv->applied = FALSE;
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
        vbox = gtk_vbox_new (FALSE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

        label = gtk_label_new (_("You need to specify an MPD server to connect to:"));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        table = gtk_table_new (2, 2, FALSE);
        gtk_container_set_border_width (GTK_CONTAINER (table), 12);
        label = gtk_label_new (_("Host: "));
        firstlaunch->priv->host_entry = gtk_entry_new ();
        gtk_entry_set_text (GTK_ENTRY (firstlaunch->priv->host_entry), "localhost");
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   label,
                                   0, 1,
                                   0, 1);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   firstlaunch->priv->host_entry,
                                   1, 2,
                                   0, 1);
        label = gtk_label_new (_("Port: "));
        firstlaunch->priv->port_entry = gtk_entry_new ();
        gtk_entry_set_text (GTK_ENTRY (firstlaunch->priv->port_entry), "6600");
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   label,
                                   0, 1,
                                   1, 2);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   firstlaunch->priv->port_entry,
                                   1, 2,
                                   1, 2);

        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        gtk_assistant_set_page_title (GTK_ASSISTANT (firstlaunch), vbox, _("Configuration"));
        gtk_assistant_set_page_type (GTK_ASSISTANT (firstlaunch), vbox, GTK_ASSISTANT_PAGE_CONTENT);
        gtk_assistant_set_page_header_image (GTK_ASSISTANT (firstlaunch), vbox, pixbuf);
        gtk_assistant_set_page_complete (GTK_ASSISTANT (firstlaunch), vbox, TRUE);
	g_signal_connect_object (G_OBJECT (firstlaunch), "prepare", G_CALLBACK (ario_firstlaunch_page2_prepare_cb), firstlaunch, 0);

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

        g_signal_connect (G_OBJECT (firstlaunch), "cancel",
		          G_CALLBACK (ario_firstlaunch_cancel_cb), firstlaunch);
        g_signal_connect (G_OBJECT (firstlaunch), "close",
		          G_CALLBACK (ario_firstlaunch_apply_cb),firstlaunch);

        gtk_window_set_position (GTK_WINDOW (firstlaunch), GTK_WIN_POS_CENTER);
}

static void
ario_firstlaunch_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioFirstlaunch *firstlaunch = ARIO_FIRSTLAUNCH (object);
        
        g_free (firstlaunch->priv);

        parent_class->finalize (G_OBJECT (firstlaunch));
}

ArioFirstlaunch *
ario_firstlaunch_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioFirstlaunch *s;

        s = g_object_new (TYPE_ARIO_FIRSTLAUNCH, NULL);

        return s;
}


