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
#include <glade/glade.h>
#include "lib/rb-glade-helpers.h"
#include "lib/eel-gconf-extensions.h"
#include "widgets/ario-firstlaunch.h"
#include "preferences/ario-preferences.h"
#include "ario-avahi.h"
#include "ario-profiles.h"
#include "ario-debug.h"

static void ario_firstlaunch_class_init (ArioFirstlaunchClass *klass);
static void ario_firstlaunch_init (ArioFirstlaunch *firstlaunch);
static void ario_firstlaunch_finalize (GObject *object);

struct ArioFirstlaunchPrivate
{
        GtkWidget *host_entry;
        GtkWidget *port_entry;
        GtkWidget *final_label;

        GtkWidget *automatic_radiobutton;
        GtkWidget *manual_radiobutton;

        GtkWidget *treeview;
        GtkListStore *hosts_model;
        GtkTreeSelection *hosts_selection;

        ArioAvahi *avahi;

        gboolean applied;
};

enum
{
        NAME_COLUMN,
        HOST_COLUMN,
        PORT_COLUMN,
        N_COLUMN
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
ario_firstlaunch_get_host_port (ArioFirstlaunch *firstlaunch,
                                gchar **host,
                                int *port)
{
        ARIO_LOG_FUNCTION_START
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (firstlaunch->priv->automatic_radiobutton))) {
                GtkTreeIter iter;
                gchar *tmp;
                GtkTreeModel *model = GTK_TREE_MODEL (firstlaunch->priv->hosts_model);
                if (gtk_tree_selection_get_selected (firstlaunch->priv->hosts_selection,
                                                     &model,
                                                     &iter)) {
                        gtk_tree_model_get (model, &iter,
                                            HOST_COLUMN, host,
                                            PORT_COLUMN, &tmp, -1);
                        *port = atoi (tmp);
                        g_free (tmp);
                } else {
                        *host = g_strdup ("localhost");
                        *port = 6600;
                }
        } else {
                *host = g_strdup (gtk_entry_get_text (GTK_ENTRY (firstlaunch->priv->host_entry)));
                *port = atoi (gtk_entry_get_text (GTK_ENTRY (firstlaunch->priv->port_entry)));
        }
}

static void
ario_firstlaunch_apply_cb (GtkWidget *widget,
                           ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        ArioProfile *profile;
        GSList *profiles = NULL;
        char *host;
        int port;

        ario_firstlaunch_get_host_port (firstlaunch,
                                        &host,
                                        &port);

        eel_gconf_set_string (CONF_HOST, host);
        eel_gconf_set_integer (CONF_PORT, port);

        profile = (ArioProfile *) g_malloc (sizeof (ArioProfile));
        profile->name = g_strdup (_("Default"));
        profile->host = g_strdup (host);
        profile->port = port;
        profile->password = NULL;
        g_free (host);

        profiles = g_slist_append (profiles, profile);
        ario_profiles_save (profiles);

        firstlaunch->priv->applied = TRUE;
        eel_gconf_set_boolean (CONF_FIRST_TIME, TRUE);
        gtk_widget_destroy (GTK_WIDGET (firstlaunch));
}

static void
ario_firstlaunch_mode_sync (ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START

        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (firstlaunch->priv->automatic_radiobutton))) {
                gtk_widget_set_sensitive (firstlaunch->priv->treeview, TRUE);
                gtk_widget_set_sensitive (firstlaunch->priv->host_entry, FALSE);
                gtk_widget_set_sensitive (firstlaunch->priv->port_entry, FALSE);
        } else {
                gtk_widget_set_sensitive (firstlaunch->priv->treeview, FALSE);
                gtk_widget_set_sensitive (firstlaunch->priv->host_entry, TRUE);
                gtk_widget_set_sensitive (firstlaunch->priv->port_entry, TRUE);
        }
}

static void
ario_firstlaunch_page_prepare_cb (GtkAssistant *assistant,
                                  GtkWidget    *page,
                                  ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        gchar *host;
        int port;
        gchar *text;

        ario_firstlaunch_get_host_port (firstlaunch,
                                        &host,
                                        &port);

        text = g_strdup_printf (_("The following configuration will be used: \n\nHost: <b>%s</b>\nPort: <b>%d</b>"),
                                host,
                                port);
        g_free (host);
        gtk_label_set_markup (GTK_LABEL (firstlaunch->priv->final_label), text);
        g_free (text);

        ario_firstlaunch_mode_sync (firstlaunch);
}

static void
ario_firstlaunch_hosts_changed_cb (ArioAvahi *avahi,
                                   ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GSList *hosts = ario_avahi_get_hosts (avahi);
        gtk_list_store_clear (firstlaunch->priv->hosts_model);

        while (hosts) {
                ArioHost *host = hosts->data;
                char *tmp;
                gtk_list_store_append (firstlaunch->priv->hosts_model, &iter);
                tmp = g_strdup_printf ("%d", host->port);
                gtk_list_store_set (firstlaunch->priv->hosts_model, &iter,
                                    NAME_COLUMN, host->name,
                                    HOST_COLUMN, host->host,
                                    PORT_COLUMN, tmp,
                                    -1);
                g_free (tmp);
                hosts = g_slist_next (hosts);
        }

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (firstlaunch->priv->hosts_model), &iter))
                gtk_tree_selection_select_iter (firstlaunch->priv->hosts_selection, &iter);
}

static void
ario_firstlaunch_radiobutton_toogled_cb (GtkToggleAction *toggleaction,
                                         ArioFirstlaunch *firstlaunch)
{
        ARIO_LOG_FUNCTION_START
        ario_firstlaunch_mode_sync (firstlaunch);
}

static void
ario_firstlaunch_init (ArioFirstlaunch *firstlaunch) 
{
        ARIO_LOG_FUNCTION_START
        GdkPixbuf *pixbuf;
        GtkWidget *label, *vbox;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GladeXML *xml;

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
        xml = rb_glade_xml_new (GLADE_PATH "connection-assistant.glade",
                                "vbox",
                                firstlaunch);

        vbox = glade_xml_get_widget (xml, "vbox");
        firstlaunch->priv->host_entry = 
                glade_xml_get_widget (xml, "host_entry");
        firstlaunch->priv->port_entry = 
                glade_xml_get_widget (xml, "port_entry");
        firstlaunch->priv->automatic_radiobutton = 
                glade_xml_get_widget (xml, "automatic_radiobutton");
        firstlaunch->priv->manual_radiobutton = 
                glade_xml_get_widget (xml, "manual_radiobutton");
        firstlaunch->priv->treeview = glade_xml_get_widget (xml, "treeview");

        g_signal_connect (G_OBJECT (firstlaunch->priv->automatic_radiobutton), "toggled",
                          G_CALLBACK (ario_firstlaunch_radiobutton_toogled_cb), firstlaunch);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", NAME_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 150);
        gtk_tree_view_append_column (GTK_TREE_VIEW (firstlaunch->priv->treeview), column);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Host"),
                                                           renderer,
                                                           "text", HOST_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 150);
        gtk_tree_view_append_column (GTK_TREE_VIEW (firstlaunch->priv->treeview), column);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Port"),
                                                           renderer,
                                                           "text", PORT_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 50);
        gtk_tree_view_append_column (GTK_TREE_VIEW (firstlaunch->priv->treeview), column);
        firstlaunch->priv->hosts_model = gtk_list_store_new (N_COLUMN,
                                                             G_TYPE_STRING,
                                                             G_TYPE_STRING,
                                                             G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (firstlaunch->priv->treeview),
                                 GTK_TREE_MODEL (firstlaunch->priv->hosts_model));
        firstlaunch->priv->hosts_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (firstlaunch->priv->treeview));
        gtk_tree_selection_set_mode (firstlaunch->priv->hosts_selection,
                                     GTK_SELECTION_BROWSE);
        gtk_assistant_append_page (GTK_ASSISTANT (firstlaunch), vbox);
        g_object_unref (G_OBJECT (xml));
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

        firstlaunch->priv->avahi = ario_avahi_new ();
        g_signal_connect_object (G_OBJECT (firstlaunch->priv->avahi),
                                 "hosts_changed", G_CALLBACK (ario_firstlaunch_hosts_changed_cb),
                                 firstlaunch, 0);

        g_signal_connect (G_OBJECT (firstlaunch), "cancel",
                          G_CALLBACK (ario_firstlaunch_cancel_cb), firstlaunch);
        g_signal_connect (G_OBJECT (firstlaunch), "close",
                          G_CALLBACK (ario_firstlaunch_apply_cb),firstlaunch);

        gtk_window_set_position (GTK_WINDOW (firstlaunch), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size (GTK_WINDOW (firstlaunch), 400, 450);

        g_signal_connect_object (G_OBJECT (firstlaunch), "prepare", G_CALLBACK (ario_firstlaunch_page_prepare_cb), firstlaunch, 0);
}

static void
ario_firstlaunch_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioFirstlaunch *firstlaunch = ARIO_FIRSTLAUNCH (object);

        g_object_unref (firstlaunch->priv->avahi);
        g_free (firstlaunch->priv);

        parent_class->finalize (G_OBJECT (firstlaunch));
}

ArioFirstlaunch *
ario_firstlaunch_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioFirstlaunch *firstlaunch;

        firstlaunch = g_object_new (TYPE_ARIO_FIRSTLAUNCH, NULL);

        return firstlaunch;
}


