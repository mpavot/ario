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

#include "shell/ario-shell-coverdownloader.h"
#include <gtk/gtk.h>
#include <string.h>

#include <glib/gi18n.h>
#include "covers/ario-cover.h"
#include "covers/ario-cover-manager.h"
#include "covers/ario-cover-handler.h"
#include "lib/gtk-builder-helpers.h"
#include "ario-debug.h"
#include "servers/ario-server.h"

static void ario_shell_coverdownloader_finalize (GObject *object);
static GObject * ario_shell_coverdownloader_constructor (GType type, guint n_construct_properties,
                                                         GObjectConstructParam *construct_properties);
static gboolean ario_shell_coverdownloader_window_delete_cb (GtkWidget *window,
                                                             GdkEventAny *event,
                                                             ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_get_cover (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                  const char *artist,
                                                  const char *album,
                                                  const char *path);
static void ario_shell_coverdownloader_close_cb (GtkButton *button,
                                                 ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_cancel_cb (GtkButton *button,
                                                  ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_get_cover_from_album (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                             const ArioServerAlbum *server_album,
                                                             const ArioShellCoverdownloaderOperation operation);

struct ArioShellCoverdownloaderPrivate
{
        int nb_covers;
        int nb_covers_already_exist;
        int nb_covers_found;
        int nb_covers_not_found;

        gboolean cancelled;

        GtkWidget *progress_artist_label;
        GtkWidget *progress_album_label;
        GtkWidget *progress_hbox;
        GtkWidget *progress_const_artist_label;
        GtkWidget *progressbar;
        GtkWidget *cancel_button;
        GtkWidget *close_button;

        GSList *albums;
        ArioShellCoverdownloaderOperation operation;

        GThread *thread;
};

static gboolean is_instantiated = FALSE;

#define ARIO_SHELL_COVERDOWNLOADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_COVERDOWNLOADER, ArioShellCoverdownloaderPrivate))
G_DEFINE_TYPE (ArioShellCoverdownloader, ario_shell_coverdownloader, GTK_TYPE_WINDOW)

static void
ario_shell_coverdownloader_class_init (ArioShellCoverdownloaderClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_shell_coverdownloader_finalize;
        object_class->constructor = ario_shell_coverdownloader_constructor;

        g_type_class_add_private (klass, sizeof (ArioShellCoverdownloaderPrivate));
}

static void
ario_shell_coverdownloader_init (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        ario_shell_coverdownloader->priv = ARIO_SHELL_COVERDOWNLOADER_GET_PRIVATE (ario_shell_coverdownloader);
        ario_shell_coverdownloader->priv->cancelled = FALSE;
}

static void
ario_shell_coverdownloader_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellCoverdownloader *ario_shell_coverdownloader;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_COVERDOWNLOADER (object));

        ario_shell_coverdownloader = ARIO_SHELL_COVERDOWNLOADER (object);

        g_return_if_fail (ario_shell_coverdownloader->priv != NULL);

        if (ario_shell_coverdownloader->priv->thread)
                g_thread_join (ario_shell_coverdownloader->priv->thread);

        /* We free the list */
        g_slist_foreach (ario_shell_coverdownloader->priv->albums, (GFunc) ario_server_free_album, NULL);
        g_slist_free (ario_shell_coverdownloader->priv->albums);

        is_instantiated = FALSE;

        G_OBJECT_CLASS (ario_shell_coverdownloader_parent_class)->finalize (object);
}

GtkWidget *
ario_shell_coverdownloader_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellCoverdownloader *ario_shell_coverdownloader;

        if (is_instantiated)
                return NULL;
        else
                is_instantiated = TRUE;

        ario_shell_coverdownloader = g_object_new (TYPE_ARIO_SHELL_COVERDOWNLOADER,
                                                   NULL);

        g_return_val_if_fail (ario_shell_coverdownloader->priv != NULL, NULL);

        return GTK_WIDGET (ario_shell_coverdownloader);
}

static GObject *
ario_shell_coverdownloader_constructor (GType type, guint n_construct_properties,
                                        GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellCoverdownloader *ario_shell_coverdownloader;
        ArioShellCoverdownloaderClass *klass;
        GObjectClass *parent_class;
        GtkBuilder *builder;
        GtkWidget *vbox;

        klass = ARIO_SHELL_COVERDOWNLOADER_CLASS (g_type_class_peek (TYPE_ARIO_SHELL_COVERDOWNLOADER));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        ario_shell_coverdownloader = ARIO_SHELL_COVERDOWNLOADER (parent_class->constructor (type, n_construct_properties,
                                                                                            construct_properties));

        builder = gtk_builder_helpers_new (UI_PATH "cover-progress.ui",
                                           NULL);
        vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));
        ario_shell_coverdownloader->priv->progress_artist_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "artist_label"));
        ario_shell_coverdownloader->priv->progress_album_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "album_label"));
        ario_shell_coverdownloader->priv->progressbar =
                GTK_WIDGET (gtk_builder_get_object (builder, "progressbar"));
        ario_shell_coverdownloader->priv->progress_hbox =
                GTK_WIDGET (gtk_builder_get_object (builder, "hbox2"));
        ario_shell_coverdownloader->priv->progress_const_artist_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "const_artist_label"));
        ario_shell_coverdownloader->priv->cancel_button =
                GTK_WIDGET (gtk_builder_get_object (builder, "cancel_button"));
        ario_shell_coverdownloader->priv->close_button =
                GTK_WIDGET (gtk_builder_get_object (builder, "close_button"));

        gtk_builder_helpers_boldify_label (builder, "operation_label");

        gtk_window_set_title (GTK_WINDOW (ario_shell_coverdownloader), _("Music Player Cover Download"));
        gtk_container_add (GTK_CONTAINER (ario_shell_coverdownloader), 
                           vbox);
        gtk_window_set_position (GTK_WINDOW (ario_shell_coverdownloader),
                                 GTK_WIN_POS_CENTER);

        g_signal_connect (ario_shell_coverdownloader->priv->cancel_button,
                          "clicked",
                          G_CALLBACK (ario_shell_coverdownloader_cancel_cb),
                          ario_shell_coverdownloader);

        g_signal_connect (ario_shell_coverdownloader->priv->close_button,
                          "clicked",
                          G_CALLBACK (ario_shell_coverdownloader_close_cb),
                          ario_shell_coverdownloader);

        g_signal_connect (ario_shell_coverdownloader,
                          "delete_event",
                          G_CALLBACK (ario_shell_coverdownloader_window_delete_cb),
                          ario_shell_coverdownloader);

        g_object_unref (builder);

        return G_OBJECT (ario_shell_coverdownloader);
}

static void
ario_shell_coverdownloader_close_cb (GtkButton *button,
                                     ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        /* Close button pressed : we close and destroy the window */
        ario_shell_coverdownloader->priv->cancelled = TRUE;
        gtk_widget_hide (GTK_WIDGET (ario_shell_coverdownloader));
        gtk_widget_destroy (GTK_WIDGET (ario_shell_coverdownloader));
}

static void
ario_shell_coverdownloader_cancel_cb (GtkButton *button,
                                      ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        /* Cancel button pressed : we wait until the end of the current download and we stop the search */
        ario_shell_coverdownloader->priv->cancelled = TRUE;
}

static gboolean
ario_shell_coverdownloader_window_delete_cb (GtkWidget *window,
                                             GdkEventAny *event,
                                             ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        if (!ario_shell_coverdownloader->priv->cancelled) {
                /* window destroyed for the first time : we wait until the end of the current download and we stop the search */
                ario_shell_coverdownloader->priv->cancelled = TRUE;
        } else {
                /* window destroyed for the second time : we close and destroy the window */
                gtk_widget_hide (GTK_WIDGET (ario_shell_coverdownloader));
                gtk_widget_destroy (GTK_WIDGET (ario_shell_coverdownloader));
        }

        return TRUE;
}

static gboolean
ario_shell_coverdownloader_refresh (gpointer data)
{
        while (gtk_events_pending ())
                gtk_main_iteration ();

        return FALSE;
}

static gboolean 
ario_shell_coverdownloader_progress_start (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        gtk_window_resize (GTK_WINDOW (ario_shell_coverdownloader), 350, 150);

        /*gtk_window_set_policy (GTK_WINDOW (ario_shell_coverdownloader), FALSE, TRUE, FALSE);*/
        gtk_window_set_resizable (GTK_WINDOW (ario_shell_coverdownloader),
                                  FALSE);

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ario_shell_coverdownloader->priv->progressbar), 0);
        gtk_widget_show_all (GTK_WIDGET (ario_shell_coverdownloader));

        /* We only want the cancel button at beginning, not the close button */
        gtk_widget_hide (ario_shell_coverdownloader->priv->close_button);

        /* We refresh the window */
        ario_shell_coverdownloader_refresh (NULL);

        return FALSE;
}

typedef struct
{
        ArioShellCoverdownloader *ario_shell_coverdownloader;
        gchar *artist;
        gchar *album;
} ArioShellCoverdownloaderIdleData;

static gboolean
ario_shell_coverdownloader_progress_update (ArioShellCoverdownloaderIdleData *data)
{
        ARIO_LOG_FUNCTION_START;
        /* We have already searched for nb_covers_done covers */
        gdouble nb_covers_done = (data->ario_shell_coverdownloader->priv->nb_covers_found 
                                  + data->ario_shell_coverdownloader->priv->nb_covers_not_found 
                                  + data->ario_shell_coverdownloader->priv->nb_covers_already_exist);

        /* We update the progress bar */
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->ario_shell_coverdownloader->priv->progressbar), 
                                       nb_covers_done / data->ario_shell_coverdownloader->priv->nb_covers);

        /* We update the artist and the album label */
        gtk_label_set_text (GTK_LABEL (data->ario_shell_coverdownloader->priv->progress_artist_label), data->artist);
        gtk_label_set_text (GTK_LABEL (data->ario_shell_coverdownloader->priv->progress_album_label), data->album);

        /* We refresh the window */
        ario_shell_coverdownloader_refresh (NULL);

        g_free (data->artist);
        g_free (data->album);
        g_free (data);

        return FALSE;
}

static gboolean
ario_shell_coverdownloader_progress_end (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        char *label_text;

        /* We only want the close button at the end, not the cancel button */
        gtk_widget_hide (ario_shell_coverdownloader->priv->cancel_button);
        gtk_widget_show (ario_shell_coverdownloader->priv->close_button);

        gtk_label_set_text (GTK_LABEL (ario_shell_coverdownloader->priv->progress_artist_label), "");
        gtk_label_set_text (GTK_LABEL (ario_shell_coverdownloader->priv->progress_album_label), "");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ario_shell_coverdownloader->priv->progressbar), 1);

        gtk_label_set_text (GTK_LABEL (ario_shell_coverdownloader->priv->progress_const_artist_label),
                            _("Download Finished!"));

        /* We show the numbers of covers found and not found */
        label_text = g_strdup_printf (_("%i covers found\n%i covers not found\n%i covers already exist"), 
                                      ario_shell_coverdownloader->priv->nb_covers_found, 
                                      ario_shell_coverdownloader->priv->nb_covers_not_found,
                                      ario_shell_coverdownloader->priv->nb_covers_already_exist);

        gtk_label_set_text (GTK_LABEL (ario_shell_coverdownloader->priv->progress_const_artist_label),
                            label_text);
        g_free (label_text);

        gtk_widget_destroy (ario_shell_coverdownloader->priv->progress_hbox);
        gtk_widget_destroy (ario_shell_coverdownloader->priv->progress_artist_label);

        return FALSE;
}

void
ario_shell_coverdownloader_get_covers (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                       const ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        GSList *albums = ario_server_get_albums (NULL);

        ario_shell_coverdownloader_get_covers_from_albums (ario_shell_coverdownloader,
                                                           albums,
                                                           operation);

        g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
        g_slist_free (albums);
}

static gboolean
ario_shell_coverdownloader_force_reload (gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ario_cover_handler_force_reload();

        return FALSE;
}

static gpointer
ario_shell_coverdownloader_get_covers_from_albums_thread (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp;

        if (!ario_shell_coverdownloader->priv->albums)
                return NULL;

        /* We show the window with the progress bar */
        if (ario_shell_coverdownloader->priv->operation == GET_COVERS)
                g_idle_add ((GSourceFunc) ario_shell_coverdownloader_progress_start, ario_shell_coverdownloader);

        ario_shell_coverdownloader->priv->nb_covers = g_slist_length (ario_shell_coverdownloader->priv->albums);

        /* While there are still covers to search */
        for (tmp = ario_shell_coverdownloader->priv->albums; tmp; tmp = g_slist_next (tmp)) {
                /* The user has pressed the "cancel button" or has closed the window : we stop the search */
                if (ario_shell_coverdownloader->priv->cancelled)
                        break;

                /* We search for a new cover */
                ario_shell_coverdownloader_get_cover_from_album (ario_shell_coverdownloader,
                                                                 tmp->data,
                                                                 ario_shell_coverdownloader->priv->operation);
        }
        /* We change the window to show a close button and infos about the search */
        if (ario_shell_coverdownloader->priv->operation == GET_COVERS)
                g_idle_add ((GSourceFunc) ario_shell_coverdownloader_progress_end, ario_shell_coverdownloader);
        else
                g_idle_add ((GSourceFunc) gtk_widget_destroy, ario_shell_coverdownloader);

        g_idle_add ((GSourceFunc) ario_shell_coverdownloader_force_reload, NULL);

        return NULL;
}

void
ario_shell_coverdownloader_get_covers_from_albums (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                   const GSList *albums,
                                                   const ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        const GSList *tmp;

        if (!albums)
                return;

        ario_shell_coverdownloader->priv->albums = NULL;
        for (tmp = albums; tmp; tmp = g_slist_next (tmp)) {
                ario_shell_coverdownloader->priv->albums = g_slist_append (ario_shell_coverdownloader->priv->albums, ario_server_copy_album (tmp->data));
        }

        ario_shell_coverdownloader->priv->operation = operation;

        ario_shell_coverdownloader->priv->thread = g_thread_create ((GThreadFunc) ario_shell_coverdownloader_get_covers_from_albums_thread,
                                                                    ario_shell_coverdownloader,
                                                                    TRUE,
                                                                    NULL);
}

static void
ario_shell_coverdownloader_get_cover_from_album (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                 const ArioServerAlbum *server_album,
                                                 const ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        const gchar *artist;
        const gchar *album;
        const gchar *path;
        ArioShellCoverdownloaderIdleData *data;

        if (!server_album)
                return;

        artist = server_album->artist;
        album = server_album->album;
        path = server_album->path;

        if (!album || !artist)
                return;

        switch (operation) {
        case GET_COVERS:
                /* We update the progress bar */
                data = (ArioShellCoverdownloaderIdleData *) g_malloc0 (sizeof (ArioShellCoverdownloaderIdleData));

                data->ario_shell_coverdownloader = ario_shell_coverdownloader;
                data->artist = g_strdup (artist);
                data->album = g_strdup (album);
                g_idle_add ((GSourceFunc) ario_shell_coverdownloader_progress_update, data);

                if (ario_cover_cover_exists (artist, album))
                        /* The cover already exists, we do nothing */
                        ++ario_shell_coverdownloader->priv->nb_covers_already_exist;
                else
                        /* We search for the cover */
                        ario_shell_coverdownloader_get_cover (ario_shell_coverdownloader, artist, album, path);
                break;

        case REMOVE_COVERS:
                /* We remove the cover from the ~/.config/ario/covers/ directory */
                ario_cover_remove_cover (artist, album);
                break;

        default:
                break;
        }
}

static void
ario_shell_coverdownloader_get_cover (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                      const char *artist,
                                      const char *album,
                                      const char *path)
{
        ARIO_LOG_FUNCTION_START;
        GArray *size;
        GSList *data = NULL;
        gboolean ret;

        size = g_array_new (TRUE, TRUE, sizeof (int));

        /* If a cover is found, it is loaded in data(0) */
        ret = ario_cover_manager_get_covers (ario_cover_manager_get_instance (),
                                             artist,
                                             album,
                                             path,
                                             &size,
                                             &data,
                                             GET_FIRST_COVER);

        /* If the cover is not too big and not too small (blank image), we save it */
        if (ret && ario_cover_size_is_valid (g_array_index (size, int, 0))) {
                ret = ario_cover_save_cover (artist,
                                             album,
                                             g_slist_nth_data (data, 0),
                                             g_array_index (size, int, 0),
                                             OVERWRITE_MODE_SKIP);

                if (ret)
                        ++ario_shell_coverdownloader->priv->nb_covers_found;
                else
                        ++ario_shell_coverdownloader->priv->nb_covers_not_found;
        } else {
                ++ario_shell_coverdownloader->priv->nb_covers_not_found;
        }

        g_array_free (size, TRUE);
        g_slist_foreach (data, (GFunc) g_free, NULL);
        g_slist_free (data);
}
