/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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
#include <string.h>

#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "shell/ario-shell-coverselect.h"
#include "ario-cover.h"
#include "lib/rb-glade-helpers.h"
#include "ario-debug.h"
#include "ario-cover-handler.h"

#define CURRENT_COVER_SIZE 130

static void ario_shell_coverselect_class_init (ArioShellCoverselectClass *klass);
static void ario_shell_coverselect_init (ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_finalize (GObject *object);
static GObject * ario_shell_coverselect_constructor (GType type, guint n_construct_properties,
                                                     GObjectConstructParam *construct_properties);
static gboolean ario_shell_coverselect_window_delete_cb (GtkWidget *window,
                                                         GdkEventAny *event,
                                                         ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_response_cb (GtkDialog *dialog,
                                                int response_id,
                                                ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_option_small_cb (GtkWidget *widget,
                                                    ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_option_medium_cb (GtkWidget *widget,
                                                     ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_option_large_cb (GtkWidget *widget,
                                                    ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_local_open_button_cb (GtkWidget *widget,
                                                         ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_get_amazon_covers_cb (GtkWidget *widget,
                                                         ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_show_covers (ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_save_cover (ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_set_current_cover (ArioShellCoverselect *ario_shell_coverselect);

enum
{
        PROP_0,
};

enum 
{
        BMP_COLUMN,
        N_COLUMN
};

enum 
{
        AMAZON_PAGE,
        LOCAL_PAGE
};

struct ArioShellCoverselectPrivate
{
        GtkWidget *amazon_artist_entry;
        GtkWidget *amazon_album_entry;

        GtkWidget *notebook;

        GtkWidget *artist_label;
        GtkWidget *album_label;

        GtkWidget *get_amazon_covers_button;
        GtkWidget *current_cover;

        GtkWidget *listview;
        GtkListStore *liststore;

        GtkWidget *option_small;
        GtkWidget *option_medium;
        GtkWidget *option_large;

        GtkWidget *local_file_entry;
        GtkWidget *local_open_button;

        const gchar *file_artist;
        const gchar *file_album;

        int coversize;

        GArray *file_size;
        GSList *ario_cover_uris;
        GSList *file_contents;
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_coverselect_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_coverselect_type = 0;

        if (ario_shell_coverselect_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellCoverselectClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_coverselect_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellCoverselect),
                        0,
                        (GInstanceInitFunc) ario_shell_coverselect_init
                };

                ario_shell_coverselect_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                                      "ArioShellCoverselect",
                                                                      &our_info, 0);
        }

        return ario_shell_coverselect_type;
}

static void
ario_shell_coverselect_class_init (ArioShellCoverselectClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_coverselect_finalize;
        object_class->constructor = ario_shell_coverselect_constructor;
}

static void
ario_shell_coverselect_init (ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_coverselect->priv = g_new0 (ArioShellCoverselectPrivate, 1);
        ario_shell_coverselect->priv->liststore = gtk_list_store_new (1, GDK_TYPE_PIXBUF);
        ario_shell_coverselect->priv->coversize = AMAZON_MEDIUM_COVER;
        ario_shell_coverselect->priv->file_contents = NULL;
        ario_shell_coverselect->priv->ario_cover_uris = NULL;
}

static void
ario_shell_coverselect_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverselect *ario_shell_coverselect;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_COVERSELECT (object));

        ario_shell_coverselect = ARIO_SHELL_COVERSELECT (object);

        g_return_if_fail (ario_shell_coverselect->priv != NULL);

        if (ario_shell_coverselect->priv->file_size)
                g_array_free (ario_shell_coverselect->priv->file_size, TRUE);
        g_slist_foreach (ario_shell_coverselect->priv->file_contents, (GFunc) g_free, NULL);
        g_slist_free (ario_shell_coverselect->priv->file_contents);
        g_slist_foreach (ario_shell_coverselect->priv->ario_cover_uris, (GFunc) g_free, NULL);
        g_slist_free (ario_shell_coverselect->priv->ario_cover_uris);
        g_free (ario_shell_coverselect->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject *
ario_shell_coverselect_constructor (GType type, guint n_construct_properties,
                                    GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverselect *ario_shell_coverselect;
        ArioShellCoverselectClass *klass;
        GObjectClass *parent_class;
        GladeXML *xml = NULL;
        GtkWidget *vbox;

        GtkTreeViewColumn *column;
        GtkCellRenderer *cell_renderer;

        klass = ARIO_SHELL_COVERSELECT_CLASS (g_type_class_peek (TYPE_ARIO_SHELL_COVERSELECT));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        ario_shell_coverselect = ARIO_SHELL_COVERSELECT (parent_class->constructor (type, n_construct_properties,
                                                                                    construct_properties));

        xml = rb_glade_xml_new (GLADE_PATH "cover-select.glade", "vbox", NULL);
        vbox = glade_xml_get_widget (xml, "vbox");
        ario_shell_coverselect->priv->artist_label =
                glade_xml_get_widget (xml, "artist_label");
        ario_shell_coverselect->priv->album_label =
                glade_xml_get_widget (xml, "album_label");
        ario_shell_coverselect->priv->notebook =
                glade_xml_get_widget (xml, "notebook");
        ario_shell_coverselect->priv->amazon_artist_entry =
                glade_xml_get_widget (xml, "amazon_artist_entry");
        ario_shell_coverselect->priv->amazon_album_entry =
                glade_xml_get_widget (xml, "amazon_album_entry");
        ario_shell_coverselect->priv->get_amazon_covers_button =
                glade_xml_get_widget (xml, "search_button");
        ario_shell_coverselect->priv->current_cover =
                glade_xml_get_widget (xml, "current_cover");
        ario_shell_coverselect->priv->listview =
                glade_xml_get_widget (xml, "listview");
        ario_shell_coverselect->priv->option_small =
                glade_xml_get_widget (xml, "option_small");
        ario_shell_coverselect->priv->option_medium =
                glade_xml_get_widget (xml, "option_medium");
        ario_shell_coverselect->priv->option_large =
                glade_xml_get_widget (xml, "option_large");
        ario_shell_coverselect->priv->local_file_entry =
                glade_xml_get_widget (xml, "local_file_entry");
        ario_shell_coverselect->priv->local_open_button =
                glade_xml_get_widget (xml, "local_open_button");
                
        rb_glade_boldify_label (xml, "cover_frame_label");
        rb_glade_boldify_label (xml, "static_artist_label");
        rb_glade_boldify_label (xml, "static_album_label");

        g_object_unref (G_OBJECT (xml));

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (ario_shell_coverselect)->vbox), 
                           vbox);

        gtk_window_set_title (GTK_WINDOW (ario_shell_coverselect), _("Cover Download"));
        gtk_window_set_default_size (GTK_WINDOW (ario_shell_coverselect), 520, 620);
        gtk_dialog_add_button (GTK_DIALOG (ario_shell_coverselect),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (ario_shell_coverselect),
                               GTK_STOCK_OK,
                               GTK_RESPONSE_OK);
        gtk_dialog_set_default_response (GTK_DIALOG (ario_shell_coverselect),
                                         GTK_RESPONSE_OK);

        cell_renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Covers"), 
                                                           cell_renderer, 
                                                           "pixbuf", 
                                                           BMP_COLUMN, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (ario_shell_coverselect->priv->listview), 
                                     column);

        gtk_tree_view_set_model (GTK_TREE_VIEW (ario_shell_coverselect->priv->listview),
                                 GTK_TREE_MODEL (ario_shell_coverselect->priv->liststore));

        switch (ario_shell_coverselect->priv->coversize) {
        case AMAZON_SMALL_COVER:
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ario_shell_coverselect->priv->option_small), TRUE);
                break;
        case AMAZON_MEDIUM_COVER:
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ario_shell_coverselect->priv->option_medium), TRUE);
                break;
        case AMAZON_LARGE_COVER:
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ario_shell_coverselect->priv->option_large), TRUE);
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        g_signal_connect_object  (G_OBJECT (ario_shell_coverselect),
                                  "delete_event",
                                  G_CALLBACK (ario_shell_coverselect_window_delete_cb),
                                  ario_shell_coverselect, 0);
        g_signal_connect_object (G_OBJECT (ario_shell_coverselect),
                                 "response",
                                 G_CALLBACK (ario_shell_coverselect_response_cb),
                                 ario_shell_coverselect, 0);                                         
        g_signal_connect (G_OBJECT (ario_shell_coverselect->priv->get_amazon_covers_button),
                          "clicked", G_CALLBACK (ario_shell_coverselect_get_amazon_covers_cb),
                          ario_shell_coverselect);
        g_signal_connect (G_OBJECT (ario_shell_coverselect->priv->option_small), 
                          "clicked", G_CALLBACK (ario_shell_coverselect_option_small_cb),
                          ario_shell_coverselect);
        g_signal_connect (G_OBJECT (ario_shell_coverselect->priv->option_medium), 
                          "clicked", G_CALLBACK (ario_shell_coverselect_option_medium_cb),
                          ario_shell_coverselect);
        g_signal_connect (G_OBJECT (ario_shell_coverselect->priv->option_large), 
                          "clicked", G_CALLBACK (ario_shell_coverselect_option_large_cb),
                          ario_shell_coverselect);
        g_signal_connect (G_OBJECT (ario_shell_coverselect->priv->local_open_button), 
                          "clicked", G_CALLBACK (ario_shell_coverselect_local_open_button_cb),
                          ario_shell_coverselect);

        return G_OBJECT (ario_shell_coverselect);
}

GtkWidget *
ario_shell_coverselect_new (const char *artist,
                            const char *album)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverselect *ario_shell_coverselect;

        ario_shell_coverselect = g_object_new (TYPE_ARIO_SHELL_COVERSELECT,
                                               NULL);

        ario_shell_coverselect->priv->file_artist = artist;        
        ario_shell_coverselect->priv->file_album = album;

        ario_shell_coverselect_set_current_cover (ario_shell_coverselect);

        gtk_entry_set_text (GTK_ENTRY (ario_shell_coverselect->priv->amazon_artist_entry), 
                            ario_shell_coverselect->priv->file_artist);
        gtk_entry_set_text (GTK_ENTRY (ario_shell_coverselect->priv->amazon_album_entry), 
                            ario_shell_coverselect->priv->file_album);

        gtk_label_set_label (GTK_LABEL (ario_shell_coverselect->priv->artist_label), 
                             ario_shell_coverselect->priv->file_artist);
        gtk_label_set_label (GTK_LABEL (ario_shell_coverselect->priv->album_label), 
                             ario_shell_coverselect->priv->file_album);

        g_return_val_if_fail (ario_shell_coverselect->priv != NULL, NULL);

        return GTK_WIDGET (ario_shell_coverselect);
}

static gboolean
ario_shell_coverselect_window_delete_cb (GtkWidget *window,
                                         GdkEventAny *event,
                                         ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (ario_shell_coverselect));
        return FALSE;
}

static void
ario_shell_coverselect_response_cb (GtkDialog *dialog,
                                    int response_id,
                                    ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        if (response_id == GTK_RESPONSE_OK) {
                ario_shell_coverselect_save_cover (ario_shell_coverselect);
                gtk_widget_hide (GTK_WIDGET (ario_shell_coverselect));
        }

        if (response_id == GTK_RESPONSE_CANCEL)
                gtk_widget_hide (GTK_WIDGET (ario_shell_coverselect));
}

static void
ario_shell_coverselect_option_small_cb (GtkWidget *widget,
                                        ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_coverselect->priv->coversize = AMAZON_SMALL_COVER;
}

static void
ario_shell_coverselect_option_medium_cb (GtkWidget *widget,
                                         ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_coverselect->priv->coversize = AMAZON_MEDIUM_COVER;
}

static void
ario_shell_coverselect_option_large_cb (GtkWidget *widget,
                                        ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_coverselect->priv->coversize = AMAZON_LARGE_COVER;
}

static void
ario_shell_coverselect_local_open_button_cb (GtkWidget *widget,
                                             ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *fileselection;

        fileselection = gtk_file_selection_new (_("Select a file"));
        gtk_window_set_modal (GTK_WINDOW (fileselection), TRUE);

        gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileselection),
                                         g_get_home_dir ());

        if (gtk_dialog_run (GTK_DIALOG (fileselection)) == GTK_RESPONSE_OK)
                gtk_entry_set_text (GTK_ENTRY (ario_shell_coverselect->priv->local_file_entry), 
                                    gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselection)));

        gtk_widget_destroy (fileselection);
}

static void
ario_shell_coverselect_set_sensitive (ArioShellCoverselect *ario_shell_coverselect,
                                      gboolean sensitive)
{
        ARIO_LOG_FUNCTION_START
        gtk_dialog_set_response_sensitive (GTK_DIALOG (ario_shell_coverselect),
                                           GTK_RESPONSE_CLOSE,
                                           sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->amazon_artist_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->amazon_album_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->get_amazon_covers_button), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->listview), sensitive);

        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void 
ario_shell_coverselect_get_amazon_covers_cb (GtkWidget *widget,
                                             ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        gchar *artist;
        gchar *album;
        gboolean ret;

        ario_shell_coverselect_set_sensitive (ario_shell_coverselect, FALSE);

        artist = gtk_editable_get_chars (GTK_EDITABLE (ario_shell_coverselect->priv->amazon_artist_entry), 0, -1);
        album = gtk_editable_get_chars (GTK_EDITABLE (ario_shell_coverselect->priv->amazon_album_entry), 0, -1);

        if (ario_shell_coverselect->priv->file_size)
                g_array_free (ario_shell_coverselect->priv->file_size, TRUE);
        g_slist_foreach (ario_shell_coverselect->priv->file_contents, (GFunc) g_free, NULL);
        g_slist_free (ario_shell_coverselect->priv->file_contents);
        ario_shell_coverselect->priv->file_contents = NULL;
        g_slist_foreach (ario_shell_coverselect->priv->ario_cover_uris, (GFunc) g_free, NULL);
        g_slist_free (ario_shell_coverselect->priv->ario_cover_uris);
        ario_shell_coverselect->priv->ario_cover_uris = NULL;

        ario_shell_coverselect->priv->file_size = g_array_new (TRUE, TRUE, sizeof (int));

        ret = ario_cover_load_amazon_covers (artist,
                                             album,
                                             &ario_shell_coverselect->priv->ario_cover_uris,
                                             &ario_shell_coverselect->priv->file_size,
                                             &ario_shell_coverselect->priv->file_contents,
                                             GET_ALL_COVERS,
                                             ario_shell_coverselect->priv->coversize);
        g_free (artist);
        g_free (album);

        ario_shell_coverselect_show_covers (ario_shell_coverselect);

        ario_shell_coverselect_set_sensitive (ario_shell_coverselect, TRUE);
}

static void 
ario_shell_coverselect_show_covers (ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        int i = 0;
        GSList *temp;
        GdkPixbuf *pixbuf;
        GtkTreePath *tree_path;
        GdkPixbufLoader *loader;

        gtk_list_store_clear (ario_shell_coverselect->priv->liststore);

        if (!ario_shell_coverselect->priv->file_contents)
                return;

        temp = ario_shell_coverselect->priv->file_contents;
        while (g_array_index (ario_shell_coverselect->priv->file_size, int, i) != 0) {
                loader = gdk_pixbuf_loader_new ();
                if (gdk_pixbuf_loader_write (loader, 
                                             temp->data,
                                             g_array_index (ario_shell_coverselect->priv->file_size, int, i),
                                             NULL)) {
                        gdk_pixbuf_loader_close (loader, NULL);
                        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

                        gtk_list_store_append(ario_shell_coverselect->priv->liststore, 
                                              &iter);
                        gtk_list_store_set (ario_shell_coverselect->priv->liststore, 
                                            &iter, 
                                            BMP_COLUMN, 
                                            pixbuf, -1);

                        g_object_unref (G_OBJECT (pixbuf));
                }
                temp = g_slist_next (temp);
                ++i;
        }

        tree_path = gtk_tree_path_new_from_indices (0, -1);
        gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (ario_shell_coverselect->priv->listview)), tree_path);
        gtk_tree_path_free(tree_path);
}

static void 
ario_shell_coverselect_save_cover (ArioShellCoverselect *ario_shell_coverselect)
{        
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;

        GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreePath *tree_path;
        gint *indice;
        gchar *data;
        const gchar *local_file;
        gsize size;
        gboolean ret;

        switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (ario_shell_coverselect->priv->notebook))) {
        case AMAZON_PAGE:
                selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ario_shell_coverselect->priv->listview));

                if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
                        return;
                tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (ario_shell_coverselect->priv->liststore), 
                                                     &iter);

                indice = gtk_tree_path_get_indices (tree_path);

                ret = ario_cover_save_cover (ario_shell_coverselect->priv->file_artist,
                                             ario_shell_coverselect->priv->file_album,
                                             g_slist_nth_data (ario_shell_coverselect->priv->file_contents, indice[0]),
                                             g_array_index (ario_shell_coverselect->priv->file_size, int, indice[0]),
                                             OVERWRITE_MODE_ASK);
                gtk_tree_path_free (tree_path);
                break;
        case LOCAL_PAGE:
                local_file = gtk_entry_get_text (GTK_ENTRY (ario_shell_coverselect->priv->local_file_entry));
                if (!local_file)
                        return;

                if (!strcmp (local_file, ""))
                        return;


                ret = g_file_get_contents (local_file,
                                           &data,
                                           &size,
                                           NULL);
                if (!ret) {
                        dialog = gtk_message_dialog_new(NULL,
                                                        GTK_DIALOG_MODAL,
                                                        GTK_MESSAGE_ERROR,
                                                        GTK_BUTTONS_OK,
                                                        _("Error reading file"));
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                        return;
                }

                ret = ario_cover_save_cover (ario_shell_coverselect->priv->file_artist,
                                             ario_shell_coverselect->priv->file_album,
                                             data,
                                             size,
                                             OVERWRITE_MODE_ASK);
                g_free (data);
                break;
        default:
                return;
                break;
        }

        if (!ret) {
                dialog = gtk_message_dialog_new(NULL,
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Error saving file"));
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
        }
        ario_cover_handler_force_reload();
}

static void
ario_shell_coverselect_set_current_cover (ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START
        GdkPixbuf *pixbuf;
        gchar *ario_cover_path;

        if (ario_cover_cover_exists (ario_shell_coverselect->priv->file_artist, ario_shell_coverselect->priv->file_album)) {
                ario_cover_path = ario_cover_make_ario_cover_path (ario_shell_coverselect->priv->file_artist,
                                                                   ario_shell_coverselect->priv->file_album,
                                                                   NORMAL_COVER);
                gtk_widget_show_all (ario_shell_coverselect->priv->current_cover);
                pixbuf = gdk_pixbuf_new_from_file_at_size (ario_cover_path, CURRENT_COVER_SIZE, CURRENT_COVER_SIZE, NULL);
                g_free (ario_cover_path);
                gtk_image_set_from_pixbuf (GTK_IMAGE (ario_shell_coverselect->priv->current_cover),
                                           pixbuf);
                g_object_unref (pixbuf);
        } else {
                gtk_widget_hide_all (ario_shell_coverselect->priv->current_cover);
        }
}
