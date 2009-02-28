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

#include "shell/ario-shell-coverselect.h"
#include <gtk/gtk.h>
#include <string.h>

#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "covers/ario-cover.h"
#include "covers/ario-cover-manager.h"
#include "covers/ario-cover-handler.h"
#include "lib/gtk-builder-helpers.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "ario-profiles.h"
#include "preferences/ario-preferences.h"

#define CURRENT_COVER_SIZE 130
#define MAX_COVER_SIZE 300

static void ario_shell_coverselect_finalize (GObject *object);
static GObject * ario_shell_coverselect_constructor (GType type, guint n_construct_properties,
                                                     GObjectConstructParam *construct_properties);
static gboolean ario_shell_coverselect_window_delete_cb (GtkWidget *window,
                                                         GdkEventAny *event,
                                                         ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_response_cb (GtkDialog *dialog,
                                                int response_id,
                                                ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_local_open_button_cb (GtkWidget *widget,
                                                         ArioShellCoverselect *ario_shell_coverselect);
static void ario_shell_coverselect_get_covers_cb (GtkWidget *widget,
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
        GLOBAL_PAGE,
        LOCAL_PAGE
};

struct ArioShellCoverselectPrivate
{
        GtkWidget *artist_entry;
        GtkWidget *album_entry;

        GtkWidget *notebook;

        GtkWidget *artist_label;
        GtkWidget *album_label;

        GtkWidget *get_covers_button;
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
        gchar *path;

        GArray *file_size;
        GSList *file_contents;
};

#define ARIO_SHELL_COVERSELECT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_COVERSELECT, ArioShellCoverselectPrivate))
G_DEFINE_TYPE (ArioShellCoverselect, ario_shell_coverselect, GTK_TYPE_DIALOG)

static void
ario_shell_coverselect_class_init (ArioShellCoverselectClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_shell_coverselect_finalize;
        object_class->constructor = ario_shell_coverselect_constructor;
        g_type_class_add_private (klass, sizeof (ArioShellCoverselectPrivate));
}

static void
ario_shell_coverselect_init (ArioShellCoverselect *shell_coverselect)
{
        ARIO_LOG_FUNCTION_START;
        shell_coverselect->priv = ARIO_SHELL_COVERSELECT_GET_PRIVATE (shell_coverselect);
        shell_coverselect->priv->liststore = gtk_list_store_new (1, GDK_TYPE_PIXBUF);
        shell_coverselect->priv->file_contents = NULL;
}

static void
ario_shell_coverselect_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellCoverselect *ario_shell_coverselect;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_COVERSELECT (object));

        ario_shell_coverselect = ARIO_SHELL_COVERSELECT (object);

        g_return_if_fail (ario_shell_coverselect->priv != NULL);

        if (ario_shell_coverselect->priv->file_size)
                g_array_free (ario_shell_coverselect->priv->file_size, TRUE);
        g_slist_foreach (ario_shell_coverselect->priv->file_contents, (GFunc) g_free, NULL);
        g_slist_free (ario_shell_coverselect->priv->file_contents);
        g_free (ario_shell_coverselect->priv->path);

        G_OBJECT_CLASS (ario_shell_coverselect_parent_class)->finalize (object);
}

static void
ario_shell_coverselect_drag_leave_cb (GtkWidget *widget,
                                      GdkDragContext *context,
                                      gint x,
                                      gint y,
                                      GtkSelectionData *data,
                                      guint info,
                                      guint time,
                                      ArioShellCoverselect *shell_coverselect)
{
        ARIO_LOG_FUNCTION_START;
        gchar *url;
        gchar *contents;
        gsize length;

        if (info == 1) {
                printf ("image  DND : TODO\n");
        } else if (info == 2) {
                data->data[data->length - 2] = 0;
                url = g_strdup ((gchar *) data->data + 7);
                if (ario_util_uri_exists (url)) {
                        if (ario_file_get_contents (url,
                                                    &contents,
                                                    &length,
                                                    NULL)) {
                                ario_cover_save_cover (shell_coverselect->priv->file_artist,
                                                       shell_coverselect->priv->file_album,
                                                       contents, length,
                                                       OVERWRITE_MODE_REPLACE);
                                g_free (contents);
                                ario_cover_handler_force_reload ();
                                ario_shell_coverselect_set_current_cover (shell_coverselect);
                        }
                }
                g_free (url);
        }

        /* finish the drag */
        gtk_drag_finish (context, TRUE, FALSE, time);
}

static GObject *
ario_shell_coverselect_constructor (GType type, guint n_construct_properties,
                                    GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellCoverselect *ario_shell_coverselect;
        ArioShellCoverselectClass *klass;
        GObjectClass *parent_class;
        GtkBuilder *builder;
        GtkWidget *vbox;
        GtkTargetList *targets;
        GtkTargetEntry *target_entry;
        gint n_elem;

        GtkTreeViewColumn *column;
        GtkCellRenderer *cell_renderer;

        klass = ARIO_SHELL_COVERSELECT_CLASS (g_type_class_peek (TYPE_ARIO_SHELL_COVERSELECT));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        ario_shell_coverselect = ARIO_SHELL_COVERSELECT (parent_class->constructor (type, n_construct_properties,
                                                                                    construct_properties));

        builder = gtk_builder_helpers_new (UI_PATH "cover-select.ui",
                                           NULL);
        vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));
        ario_shell_coverselect->priv->artist_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "artist_label"));
        ario_shell_coverselect->priv->album_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "album_label"));
        ario_shell_coverselect->priv->notebook =
                GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
        ario_shell_coverselect->priv->artist_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "artist_entry"));
        ario_shell_coverselect->priv->album_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "album_entry"));
        ario_shell_coverselect->priv->get_covers_button =
                GTK_WIDGET (gtk_builder_get_object (builder, "search_button"));
        ario_shell_coverselect->priv->current_cover =
                GTK_WIDGET (gtk_builder_get_object (builder, "current_cover"));
        ario_shell_coverselect->priv->listview =
                GTK_WIDGET (gtk_builder_get_object (builder, "listview"));
        ario_shell_coverselect->priv->option_small =
                GTK_WIDGET (gtk_builder_get_object (builder, "option_small"));
        ario_shell_coverselect->priv->option_medium =
                GTK_WIDGET (gtk_builder_get_object (builder, "option_medium"));
        ario_shell_coverselect->priv->option_large =
                GTK_WIDGET (gtk_builder_get_object (builder, "option_large"));
        ario_shell_coverselect->priv->local_file_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "local_file_entry"));
        ario_shell_coverselect->priv->local_open_button =
                GTK_WIDGET (gtk_builder_get_object (builder, "local_open_button"));

        gtk_builder_helpers_boldify_label (builder, "static_artist_label");
        gtk_builder_helpers_boldify_label (builder, "static_album_label");

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

        g_signal_connect (ario_shell_coverselect,
                          "delete_event",
                          G_CALLBACK (ario_shell_coverselect_window_delete_cb),
                          ario_shell_coverselect);
        g_signal_connect (ario_shell_coverselect,
                          "response",
                          G_CALLBACK (ario_shell_coverselect_response_cb),
                          ario_shell_coverselect);
        g_signal_connect (ario_shell_coverselect->priv->get_covers_button,
                          "clicked",
                          G_CALLBACK (ario_shell_coverselect_get_covers_cb),
                          ario_shell_coverselect);
        g_signal_connect (ario_shell_coverselect->priv->local_open_button, 
                          "clicked",
                          G_CALLBACK (ario_shell_coverselect_local_open_button_cb),
                          ario_shell_coverselect);

        targets = gtk_target_list_new (NULL, 0);
        gtk_target_list_add_image_targets (targets, 1, TRUE);
        gtk_target_list_add_uri_targets (targets, 2);
        target_entry = gtk_target_table_new_from_list (targets, &n_elem);
        gtk_target_list_unref (targets);

        gtk_drag_dest_set (ario_shell_coverselect->priv->current_cover,
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP,
                           target_entry, n_elem,
                           GDK_ACTION_COPY);
        gtk_target_table_free (target_entry, n_elem);

        g_signal_connect (ario_shell_coverselect->priv->current_cover,
                          "drag_data_received",
                          G_CALLBACK (ario_shell_coverselect_drag_leave_cb),
                          ario_shell_coverselect);

        g_object_unref (builder);

        return G_OBJECT (ario_shell_coverselect);
}

GtkWidget *
ario_shell_coverselect_new (ArioServerAlbum *server_album)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellCoverselect *ario_shell_coverselect;

        ario_shell_coverselect = g_object_new (TYPE_ARIO_SHELL_COVERSELECT,
                                               NULL);

        ario_shell_coverselect->priv->file_artist = server_album->artist;
        ario_shell_coverselect->priv->file_album = server_album->album;
        ario_shell_coverselect->priv->path = g_path_get_dirname (server_album->path);

        ario_shell_coverselect_set_current_cover (ario_shell_coverselect);

        gtk_entry_set_text (GTK_ENTRY (ario_shell_coverselect->priv->artist_entry), 
                            ario_shell_coverselect->priv->file_artist);
        gtk_entry_set_text (GTK_ENTRY (ario_shell_coverselect->priv->album_entry), 
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
        ARIO_LOG_FUNCTION_START;
        gtk_widget_hide (GTK_WIDGET (ario_shell_coverselect));
        return FALSE;
}

static void
ario_shell_coverselect_response_cb (GtkDialog *dialog,
                                    int response_id,
                                    ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START;
        if (response_id == GTK_RESPONSE_OK) {
                ario_shell_coverselect_save_cover (ario_shell_coverselect);
                gtk_widget_hide (GTK_WIDGET (ario_shell_coverselect));
        }

        if (response_id == GTK_RESPONSE_CANCEL)
                gtk_widget_hide (GTK_WIDGET (ario_shell_coverselect));
}

static void
ario_shell_coverselect_local_open_button_cb (GtkWidget *widget,
                                             ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *dialog;
        gchar *musicdir;
        gchar *path;

        dialog = gtk_file_chooser_dialog_new (NULL,
                                              NULL,
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                              NULL);
        musicdir = ario_profiles_get_current (ario_profiles_get ())->musicdir;
        if (musicdir) {
                path = g_build_filename (musicdir, ario_shell_coverselect->priv->path, NULL);

                if (ario_util_uri_exists (path))
                        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), path);

                g_free (path);
        }

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                char *filename;

                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
                if (filename) {
                        gtk_entry_set_text (GTK_ENTRY (ario_shell_coverselect->priv->local_file_entry), 
                                            filename);
                        g_free (filename);
                }
        }

        gtk_widget_destroy (dialog);
}

static void
ario_shell_coverselect_set_sensitive (ArioShellCoverselect *ario_shell_coverselect,
                                      gboolean sensitive)
{
        ARIO_LOG_FUNCTION_START;
        gtk_dialog_set_response_sensitive (GTK_DIALOG (ario_shell_coverselect),
                                           GTK_RESPONSE_CLOSE,
                                           sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->artist_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->album_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->get_covers_button), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (ario_shell_coverselect->priv->listview), sensitive);

        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void 
ario_shell_coverselect_get_covers_cb (GtkWidget *widget,
                                      ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START;
        gchar *artist;
        gchar *album;
        gboolean ret;

        ario_shell_coverselect_set_sensitive (ario_shell_coverselect, FALSE);

        artist = gtk_editable_get_chars (GTK_EDITABLE (ario_shell_coverselect->priv->artist_entry), 0, -1);
        album = gtk_editable_get_chars (GTK_EDITABLE (ario_shell_coverselect->priv->album_entry), 0, -1);

        if (ario_shell_coverselect->priv->file_size)
                g_array_free (ario_shell_coverselect->priv->file_size, TRUE);
        g_slist_foreach (ario_shell_coverselect->priv->file_contents, (GFunc) g_free, NULL);
        g_slist_free (ario_shell_coverselect->priv->file_contents);
        ario_shell_coverselect->priv->file_contents = NULL;

        ario_shell_coverselect->priv->file_size = g_array_new (TRUE, TRUE, sizeof (int));

        ret = ario_cover_manager_get_covers (ario_cover_manager_get_instance (),
                                             artist,
                                             album,
                                             ario_shell_coverselect->priv->path,
                                             &ario_shell_coverselect->priv->file_size,
                                             &ario_shell_coverselect->priv->file_contents,
                                             GET_ALL_COVERS);
        g_free (artist);
        g_free (album);

        ario_shell_coverselect_show_covers (ario_shell_coverselect);

        ario_shell_coverselect_set_sensitive (ario_shell_coverselect, TRUE);
}

static void 
ario_shell_coverselect_show_covers (ArioShellCoverselect *ario_shell_coverselect)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        int i = 0;
        GSList *temp;
        GdkPixbuf *pixbuf, *tmp_pixbuf;
        GtkTreePath *tree_path;
        GdkPixbufLoader *loader;
        int height, width;

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

                        height = gdk_pixbuf_get_height (pixbuf);
                        width = gdk_pixbuf_get_width (pixbuf);
                        if (height > MAX_COVER_SIZE || width > MAX_COVER_SIZE) {
                                tmp_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                                      MAX_COVER_SIZE,
                                                                      height * MAX_COVER_SIZE / width,
                                                                      GDK_INTERP_BILINEAR);
                                g_object_unref (G_OBJECT (pixbuf));
                                pixbuf = tmp_pixbuf;
                        }

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
        ARIO_LOG_FUNCTION_START;
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
        case GLOBAL_PAGE:
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


                ret = ario_file_get_contents (local_file,
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
        ARIO_LOG_FUNCTION_START;
        GdkPixbuf *pixbuf;
        gchar *ario_cover_path;

        if (ario_cover_cover_exists (ario_shell_coverselect->priv->file_artist, ario_shell_coverselect->priv->file_album)) {
                ario_cover_path = ario_cover_make_cover_path (ario_shell_coverselect->priv->file_artist,
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
