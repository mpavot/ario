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
#include "shell/ario-shell-lyricsselect.h"
#include "lib/rb-glade-helpers.h"
#include "ario-debug.h"

#define CURRENT_LYRICS_SIZE 130

static void ario_shell_lyricsselect_class_init (ArioShellLyricsselectClass *klass);
static void ario_shell_lyricsselect_init (ArioShellLyricsselect *ario_shell_lyricsselect);
static void ario_shell_lyricsselect_finalize (GObject *object);
static GObject * ario_shell_lyricsselect_constructor (GType type, guint n_construct_properties,
                                                      GObjectConstructParam *construct_properties);
static gboolean ario_shell_lyricsselect_window_delete_cb (GtkWidget *window,
                                                          GdkEventAny *event,
                                                          ArioShellLyricsselect *ario_shell_lyricsselect);
static void ario_shell_lyricsselect_search_cb (GtkWidget *widget,
                                               ArioShellLyricsselect *ario_shell_lyricsselect);
static void ario_shell_lyricsselect_show_lyrics (ArioShellLyricsselect *ario_shell_lyricsselect);

enum
{
        PROP_0,
};

enum 
{
        ARTIST_COLUMN,
        TITLE_COLUMN,
        HID_COLUMN,
        N_COLUMN
};

struct ArioShellLyricsselectPrivate
{
        GtkWidget *artist_entry;
        GtkWidget *title_entry;

        GtkWidget *artist_label;
        GtkWidget *title_label;

        GtkWidget *search_button;

        GtkWidget *treeview;
        GtkListStore *liststore;

        const gchar *file_artist;
        const gchar *file_title;

        GSList *lyrics;
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_lyricsselect_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_lyricsselect_type = 0;

        if (ario_shell_lyricsselect_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellLyricsselectClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_lyricsselect_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellLyricsselect),
                        0,
                        (GInstanceInitFunc) ario_shell_lyricsselect_init
                };

                ario_shell_lyricsselect_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                                       "ArioShellLyricsselect",
                                                                       &our_info, 0);
        }

        return ario_shell_lyricsselect_type;
}

static void
ario_shell_lyricsselect_class_init (ArioShellLyricsselectClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_lyricsselect_finalize;
        object_class->constructor = ario_shell_lyricsselect_constructor;
}

static void
ario_shell_lyricsselect_init (ArioShellLyricsselect *ario_shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_lyricsselect->priv = g_new0 (ArioShellLyricsselectPrivate, 1);
        ario_shell_lyricsselect->priv->liststore = gtk_list_store_new (N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        ario_shell_lyricsselect->priv->lyrics = NULL;
}

static void
ario_shell_lyricsselect_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyricsselect *ario_shell_lyricsselect;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_LYRICSSELECT (object));

        ario_shell_lyricsselect = ARIO_SHELL_LYRICSSELECT (object);

        g_return_if_fail (ario_shell_lyricsselect->priv != NULL);

        g_slist_foreach (ario_shell_lyricsselect->priv->lyrics, (GFunc) ario_lyrics_candidates_free, NULL);
        g_slist_free (ario_shell_lyricsselect->priv->lyrics);

        g_free (ario_shell_lyricsselect->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject *
ario_shell_lyricsselect_constructor (GType type, guint n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyricsselect *ario_shell_lyricsselect;
        ArioShellLyricsselectClass *klass;
        GObjectClass *parent_class;
        GladeXML *xml = NULL;
        GtkWidget *vbox;

        GtkTreeViewColumn *column;
        GtkCellRenderer *cell_renderer;

        klass = ARIO_SHELL_LYRICSSELECT_CLASS (g_type_class_peek (TYPE_ARIO_SHELL_LYRICSSELECT));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        ario_shell_lyricsselect = ARIO_SHELL_LYRICSSELECT (parent_class->constructor (type, n_construct_properties,
                                                                                      construct_properties));

        xml = rb_glade_xml_new (GLADE_PATH "lyrics-select.glade", "vbox", NULL);
        vbox = glade_xml_get_widget (xml, "vbox");
        ario_shell_lyricsselect->priv->artist_label =
                glade_xml_get_widget (xml, "artist_label");
        ario_shell_lyricsselect->priv->title_label =
                glade_xml_get_widget (xml, "title_label");
        ario_shell_lyricsselect->priv->artist_entry =
                glade_xml_get_widget (xml, "artist_entry");
        ario_shell_lyricsselect->priv->title_entry =
                glade_xml_get_widget (xml, "title_entry");
        ario_shell_lyricsselect->priv->search_button =
                glade_xml_get_widget (xml, "search_button");
        ario_shell_lyricsselect->priv->treeview =
                glade_xml_get_widget (xml, "treeview");
                
        rb_glade_boldify_label (xml, "static_artist_label");
        rb_glade_boldify_label (xml, "static_title_label");

        g_object_unref (G_OBJECT (xml));

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (ario_shell_lyricsselect)->vbox), 
                           vbox);

        gtk_window_set_title (GTK_WINDOW (ario_shell_lyricsselect), _("Lyrics Download"));
        gtk_window_set_default_size (GTK_WINDOW (ario_shell_lyricsselect), 520, 350);
        gtk_dialog_add_button (GTK_DIALOG (ario_shell_lyricsselect),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (ario_shell_lyricsselect),
                               GTK_STOCK_OK,
                               GTK_RESPONSE_OK);
        gtk_dialog_set_default_response (GTK_DIALOG (ario_shell_lyricsselect),
                                         GTK_RESPONSE_OK);

        cell_renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           cell_renderer, 
                                                           "text", 
                                                           ARTIST_COLUMN, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (ario_shell_lyricsselect->priv->treeview), 
                                     column);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 125);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, ARTIST_COLUMN);
        cell_renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           cell_renderer, 
                                                           "text", 
                                                           TITLE_COLUMN, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (ario_shell_lyricsselect->priv->treeview), 
                                     column);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 125);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
        gtk_tree_view_column_set_sort_column_id (column, TITLE_COLUMN);
        gtk_tree_view_set_model (GTK_TREE_VIEW (ario_shell_lyricsselect->priv->treeview),
                                 GTK_TREE_MODEL (ario_shell_lyricsselect->priv->liststore));

        g_signal_connect_object  (G_OBJECT (ario_shell_lyricsselect),
                                  "delete_event",
                                  G_CALLBACK (ario_shell_lyricsselect_window_delete_cb),
                                  ario_shell_lyricsselect, 0);                  
        g_signal_connect (G_OBJECT (ario_shell_lyricsselect->priv->search_button),
                          "clicked", G_CALLBACK (ario_shell_lyricsselect_search_cb),
                          ario_shell_lyricsselect);

        return G_OBJECT (ario_shell_lyricsselect);
}

GtkWidget *
ario_shell_lyricsselect_new (const char *artist,
                             const char *title)
{
        ARIO_LOG_FUNCTION_START
        ArioShellLyricsselect *ario_shell_lyricsselect;

        ario_shell_lyricsselect = g_object_new (TYPE_ARIO_SHELL_LYRICSSELECT,
                                                NULL);

        ario_shell_lyricsselect->priv->file_artist = artist;
        ario_shell_lyricsselect->priv->file_title = title;

        gtk_entry_set_text (GTK_ENTRY (ario_shell_lyricsselect->priv->artist_entry), 
                            ario_shell_lyricsselect->priv->file_artist);
        gtk_entry_set_text (GTK_ENTRY (ario_shell_lyricsselect->priv->title_entry), 
                            ario_shell_lyricsselect->priv->file_title);

        gtk_label_set_label (GTK_LABEL (ario_shell_lyricsselect->priv->artist_label), 
                             ario_shell_lyricsselect->priv->file_artist);
        gtk_label_set_label (GTK_LABEL (ario_shell_lyricsselect->priv->title_label), 
                             ario_shell_lyricsselect->priv->file_title);

        g_return_val_if_fail (ario_shell_lyricsselect->priv != NULL, NULL);

        return GTK_WIDGET (ario_shell_lyricsselect);
}

static gboolean
ario_shell_lyricsselect_window_delete_cb (GtkWidget *window,
                                          GdkEventAny *event,
                                          ArioShellLyricsselect *ario_shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (ario_shell_lyricsselect));
        return FALSE;
}

static void
ario_shell_lyricsselect_set_sensitive (ArioShellLyricsselect *ario_shell_lyricsselect,
                                       gboolean sensitive)
{
        ARIO_LOG_FUNCTION_START
        gtk_dialog_set_response_sensitive (GTK_DIALOG (ario_shell_lyricsselect),
                                           GTK_RESPONSE_CLOSE,
                                           sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_lyricsselect->priv->artist_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_lyricsselect->priv->title_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_lyricsselect->priv->search_button), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_lyricsselect->priv->treeview), sensitive);

        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void 
ario_shell_lyricsselect_search_cb (GtkWidget *widget,
                                   ArioShellLyricsselect *ario_shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START
        gchar *artist;
        gchar *title;

        ario_shell_lyricsselect_set_sensitive (ario_shell_lyricsselect, FALSE);

        artist = gtk_editable_get_chars (GTK_EDITABLE (ario_shell_lyricsselect->priv->artist_entry), 0, -1);
        title = gtk_editable_get_chars (GTK_EDITABLE (ario_shell_lyricsselect->priv->title_entry), 0, -1);

        g_slist_foreach (ario_shell_lyricsselect->priv->lyrics, (GFunc) ario_lyrics_candidates_free, NULL);
        g_slist_free (ario_shell_lyricsselect->priv->lyrics);
        ario_shell_lyricsselect->priv->lyrics = NULL;

        ario_shell_lyricsselect->priv->lyrics = ario_lyrics_get_lyrics_candidates (artist, title);
        g_free (artist);
        g_free (title);

        ario_shell_lyricsselect_show_lyrics (ario_shell_lyricsselect);

        ario_shell_lyricsselect_set_sensitive (ario_shell_lyricsselect, TRUE);
}

static void 
ario_shell_lyricsselect_show_lyrics (ArioShellLyricsselect *ario_shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        GSList *tmp;
        GtkTreePath *tree_path;
        ArioLyricsCandidate *candidate;

        gtk_list_store_clear (ario_shell_lyricsselect->priv->liststore);

        for (tmp = ario_shell_lyricsselect->priv->lyrics; tmp; tmp = g_slist_next (tmp)) {
                candidate = (ArioLyricsCandidate *) tmp->data;
                gtk_list_store_append(ario_shell_lyricsselect->priv->liststore, 
                                      &iter);
                gtk_list_store_set (ario_shell_lyricsselect->priv->liststore, 
                                    &iter, 
                                    ARTIST_COLUMN, 
                                    candidate->artist,
                                    TITLE_COLUMN, 
                                    candidate->title,
                                    HID_COLUMN, 
                                    candidate->hid, -1);
        }

        tree_path = gtk_tree_path_new_from_indices (0, -1);
        gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (ario_shell_lyricsselect->priv->treeview)), tree_path);
        gtk_tree_path_free(tree_path);
}

ArioLyricsCandidate *
ario_shell_lyricsselect_get_lyrics_candidate (ArioShellLyricsselect *ario_shell_lyricsselect)
{
        ARIO_LOG_FUNCTION_START
        GList* paths;
        GtkTreeIter iter;
        ArioLyricsCandidate *candidate;
        GtkTreeModel *tree_model = GTK_TREE_MODEL (ario_shell_lyricsselect->priv->liststore);
        GtkTreePath *path;
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ario_shell_lyricsselect->priv->treeview));

        paths = gtk_tree_selection_get_selected_rows (selection,
                                                      &tree_model);
        if (!paths)
                return NULL;

        path = g_list_first(paths)->data;
        if (!path)
                return NULL;

        gtk_tree_model_get_iter (tree_model,
                                 &iter,
                                 path);
        candidate = (ArioLyricsCandidate *) g_malloc (sizeof (ArioLyricsCandidate));;
        gtk_tree_model_get (tree_model, &iter, ARTIST_COLUMN, &candidate->artist, -1);
        gtk_tree_model_get (tree_model, &iter, TITLE_COLUMN, &candidate->title, -1);
        gtk_tree_model_get (tree_model, &iter, HID_COLUMN, &candidate->hid, -1);

        return candidate;
}

