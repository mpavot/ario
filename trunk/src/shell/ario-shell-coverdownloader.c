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
#include "ario-cover.h"
#include "shell/ario-shell-coverdownloader.h"
#include "lib/rb-glade-helpers.h"
#include "ario-debug.h"
#include "ario-cover-handler.h"

static void ario_shell_coverdownloader_class_init (ArioShellCoverdownloaderClass *klass);
static void ario_shell_coverdownloader_init (ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_finalize (GObject *object);
static void ario_shell_coverdownloader_set_property (GObject *object,
                                                     guint prop_id,
                                                     const GValue *value,
                                                     GParamSpec *pspec);
static void ario_shell_coverdownloader_get_property (GObject *object,
                                                     guint prop_id,
                                                     GValue *value,
                                                     GParamSpec *pspec);
static GObject * ario_shell_coverdownloader_constructor (GType type, guint n_construct_properties,
                                                         GObjectConstructParam *construct_properties);
static gboolean ario_shell_coverdownloader_window_delete_cb (GtkWidget *window,
                                                             GdkEventAny *event,
                                                             ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_find_amazon_image (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                          const char *artist,
                                                          const char *album);
static void ario_shell_coverdownloader_close_cb (GtkButton *button,
                                                 ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_cancel_cb (GtkButton *button,
                                                  ArioShellCoverdownloader *ario_shell_coverdownloader);
static void ario_shell_coverdownloader_get_cover_from_album (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                             ArioMpdAlbum *ario_mpd_album,
                                                             ArioShellCoverdownloaderOperation operation);
enum
{
        PROP_0,
        PROP_MPD
};

struct ArioShellCoverdownloaderPrivate
{
        int nb_covers;
        int nb_covers_already_exist;
        int nb_covers_found;
        int nb_covers_not_found;

        gboolean cancelled;
        gboolean closed;

        GSList *entries;

        GtkWidget *progress_artist_label;
        GtkWidget *progress_album_label;
        GtkWidget *progress_operation_label;
        GtkWidget *progress_hbox;
        GtkWidget *progress_const_artist_label;
        GtkWidget *progressbar;
        GtkWidget *cancel_button;
        GtkWidget *close_button;

        ArioMpd *mpd;
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_coverdownloader_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_coverdownloader_type = 0;
        if (ario_shell_coverdownloader_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellCoverdownloaderClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_coverdownloader_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellCoverdownloader),
                        0,
                        (GInstanceInitFunc) ario_shell_coverdownloader_init
                };

                ario_shell_coverdownloader_type = g_type_register_static (GTK_TYPE_WINDOW,
                                                                          "ArioShellCoverdownloader",
                                                                          &our_info, 0);
        }
        return ario_shell_coverdownloader_type;
}

static void
ario_shell_coverdownloader_class_init (ArioShellCoverdownloaderClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_coverdownloader_finalize;
        object_class->constructor = ario_shell_coverdownloader_constructor;

        object_class->set_property = ario_shell_coverdownloader_set_property;
        object_class->get_property = ario_shell_coverdownloader_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ario_shell_coverdownloader_init (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_coverdownloader->priv = g_new0 (ArioShellCoverdownloaderPrivate, 1);
        ario_shell_coverdownloader->priv->cancelled = FALSE;
        ario_shell_coverdownloader->priv->closed = FALSE;
}

static void
ario_shell_coverdownloader_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverdownloader *ario_shell_coverdownloader;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_COVERDOWNLOADER (object));

        ario_shell_coverdownloader = ARIO_SHELL_COVERDOWNLOADER (object);

        g_return_if_fail (ario_shell_coverdownloader->priv != NULL);
        g_free (ario_shell_coverdownloader->priv);
        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_shell_coverdownloader_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverdownloader *ario_shell_coverdownloader = ARIO_SHELL_COVERDOWNLOADER (object);

        switch (prop_id)
        {
        case PROP_MPD:
                ario_shell_coverdownloader->priv->mpd = g_value_get_object (value);
                break;        
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_shell_coverdownloader_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverdownloader *ario_shell_coverdownloader = ARIO_SHELL_COVERDOWNLOADER (object);

        switch (prop_id)
        {
        case PROP_MPD:
                g_value_set_object (value, ario_shell_coverdownloader->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
ario_shell_coverdownloader_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverdownloader *ario_shell_coverdownloader;

        ario_shell_coverdownloader = g_object_new (TYPE_ARIO_SHELL_COVERDOWNLOADER,
                                                   "mpd", mpd,
                                                   NULL);

        g_return_val_if_fail (ario_shell_coverdownloader->priv != NULL, NULL);

        return GTK_WIDGET (ario_shell_coverdownloader);
}

static GObject *
ario_shell_coverdownloader_constructor (GType type, guint n_construct_properties,
                                        GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START
        ArioShellCoverdownloader *ario_shell_coverdownloader;
        ArioShellCoverdownloaderClass *klass;
        GObjectClass *parent_class;
        GladeXML *xml = NULL;
        GtkWidget *vbox;

        klass = ARIO_SHELL_COVERDOWNLOADER_CLASS (g_type_class_peek (TYPE_ARIO_SHELL_COVERDOWNLOADER));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        ario_shell_coverdownloader = ARIO_SHELL_COVERDOWNLOADER (parent_class->constructor (type, n_construct_properties,
                                                                                            construct_properties));

        xml = rb_glade_xml_new (GLADE_PATH "cover-progress.glade", "vbox", NULL);
        vbox =
                glade_xml_get_widget (xml, "vbox");
        ario_shell_coverdownloader->priv->progress_artist_label =
                glade_xml_get_widget (xml, "artist_label");
        ario_shell_coverdownloader->priv->progress_album_label =
                glade_xml_get_widget (xml, "album_label");
        ario_shell_coverdownloader->priv->progress_operation_label =
                glade_xml_get_widget (xml, "operation_label");
        ario_shell_coverdownloader->priv->progressbar =
                glade_xml_get_widget (xml, "progressbar");
        ario_shell_coverdownloader->priv->progress_hbox =
                glade_xml_get_widget (xml, "hbox2");
        ario_shell_coverdownloader->priv->progress_const_artist_label =
                glade_xml_get_widget (xml, "const_artist_label");
        ario_shell_coverdownloader->priv->cancel_button =
                glade_xml_get_widget (xml, "cancel_button");
        ario_shell_coverdownloader->priv->close_button =
                glade_xml_get_widget (xml, "close_button");
                
        rb_glade_boldify_label (xml, "operation_label");

        g_object_unref (G_OBJECT (xml));

        gtk_window_set_title (GTK_WINDOW (ario_shell_coverdownloader), _("Music Player Cover Download"));
        gtk_container_add (GTK_CONTAINER (ario_shell_coverdownloader), 
                           vbox);
        gtk_window_set_position (GTK_WINDOW (ario_shell_coverdownloader),
                                 GTK_WIN_POS_CENTER);

        g_signal_connect_object (G_OBJECT (ario_shell_coverdownloader->priv->cancel_button),
                                 "clicked",
                                 G_CALLBACK (ario_shell_coverdownloader_cancel_cb),
                                 ario_shell_coverdownloader, 0);

        g_signal_connect_object (G_OBJECT (ario_shell_coverdownloader->priv->close_button),
                                 "clicked",
                                 G_CALLBACK (ario_shell_coverdownloader_close_cb),
                                 ario_shell_coverdownloader, 0);

        g_signal_connect_object (G_OBJECT (ario_shell_coverdownloader),
                                 "delete_event",
                                 G_CALLBACK (ario_shell_coverdownloader_window_delete_cb),
                                 ario_shell_coverdownloader, 0);

        return G_OBJECT (ario_shell_coverdownloader);
}

static void
ario_shell_coverdownloader_close_cb (GtkButton *button,
                                     ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
        /* Close button pressed : we close and destroy the window */
        ario_shell_coverdownloader->priv->closed = TRUE;
}

static void
ario_shell_coverdownloader_cancel_cb (GtkButton *button,
                                      ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
        /* Cancel button pressed : we wait until the end of the current download and we stop the search */
        ario_shell_coverdownloader->priv->cancelled = TRUE;
}

static gboolean
ario_shell_coverdownloader_window_delete_cb (GtkWidget *window,
                                             GdkEventAny *event,
                                             ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
        if (!ario_shell_coverdownloader->priv->cancelled)
                /* window destroyed for the first time : we wait until the end of the current download and we stop the search */
                ario_shell_coverdownloader->priv->cancelled = TRUE;
        else
                /* window destroyed for the second time : we close and destroy the window */
                ario_shell_coverdownloader->priv->closed = TRUE;

        return TRUE;
}

static void 
ario_shell_coverdownloader_progress_start (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
        gtk_window_resize (GTK_WINDOW (ario_shell_coverdownloader), 350, 150);

        /*gtk_window_set_policy (GTK_WINDOW (ario_shell_coverdownloader), FALSE, TRUE, FALSE);*/
        gtk_window_set_resizable (GTK_WINDOW (ario_shell_coverdownloader),
                                  FALSE);

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ario_shell_coverdownloader->priv->progressbar), 0);
        gtk_widget_show_all (GTK_WIDGET (ario_shell_coverdownloader));

        /* We only want the cancel button at beginning, not the close button */
        gtk_widget_hide (ario_shell_coverdownloader->priv->close_button);

        /* We refresh the window */
        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void
ario_shell_coverdownloader_progress_update (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                            const gchar *artist,
                                            const gchar *album)
{
        ARIO_LOG_FUNCTION_START
        /* We have already searched for nb_covers_done covers */
        gdouble nb_covers_done = (ario_shell_coverdownloader->priv->nb_covers_found 
                                  + ario_shell_coverdownloader->priv->nb_covers_not_found 
                                  + ario_shell_coverdownloader->priv->nb_covers_already_exist);

        /* We update the progress bar */
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ario_shell_coverdownloader->priv->progressbar), 
                                       nb_covers_done / ario_shell_coverdownloader->priv->nb_covers);

        /* We update the artist and the album label */
        gtk_label_set_text (GTK_LABEL (ario_shell_coverdownloader->priv->progress_artist_label), artist);
        gtk_label_set_text (GTK_LABEL (ario_shell_coverdownloader->priv->progress_album_label), album);

        /* We refresh the window */
        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void
ario_shell_coverdownloader_progress_end (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
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

        /* We wait until the user click on the close button */
        while (!ario_shell_coverdownloader->priv->closed)
                gtk_main_iteration ();
}

static void
ario_shell_coverdownloader_nocover (ArioShellCoverdownloader *ario_shell_coverdownloader)
{
        ARIO_LOG_FUNCTION_START
        /* This information will be displayed at the end of the search */
        ++ario_shell_coverdownloader->priv->nb_covers_not_found;
}

void
ario_shell_coverdownloader_get_covers (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                       ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_coverdownloader_get_covers_from_artists (ario_shell_coverdownloader,
                                                            ario_mpd_get_artists(ario_shell_coverdownloader->priv->mpd),
                                                            operation);
}

void
ario_shell_coverdownloader_get_covers_from_artists (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                    GSList *artists,
                                                    ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START
        GSList *albums = NULL, *temp_albums = NULL, *temp;

        if (!artists)
                return;

        for (temp = artists; temp; temp = temp->next) {
                /* We make a query for each selected artist */
                temp_albums = ario_mpd_get_albums (ario_shell_coverdownloader->priv->mpd, (gchar *) temp->data);

                /* We add the result of the query to a list */
                albums = g_slist_concat (albums, temp_albums);
        }

        /* We search for all the covers of the entry in the list */
        ario_shell_coverdownloader_get_covers_from_albums (ario_shell_coverdownloader, albums, operation);

        /* We free the list */
        g_slist_foreach (albums, (GFunc) ario_mpd_free_album, NULL);
        g_slist_free (albums);
}

void
ario_shell_coverdownloader_get_covers_from_albums (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                   GSList *albums,
                                                   ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START
        GSList *temp;

        if (!albums)
                return;

        /* We show the window with the progress bar */
        if (operation == GET_AMAZON_COVERS)
                ario_shell_coverdownloader_progress_start (ario_shell_coverdownloader);

        ario_shell_coverdownloader->priv->nb_covers = g_slist_length (albums);

        /* While there are still covers to search */
        for (temp = albums; temp; temp = g_slist_next (temp)) {
                /* The user has pressed the "cancel button" or has closed the window : we stop the search */
                if (ario_shell_coverdownloader->priv->cancelled)
                        break;

                /* We search for a new cover */
                ario_shell_coverdownloader_get_cover_from_album (ario_shell_coverdownloader,
                                                                 temp->data,
                                                                 operation);
        }

        /* We change the window to show a close button and infos about the search */
        if (operation == GET_AMAZON_COVERS)
                ario_shell_coverdownloader_progress_end (ario_shell_coverdownloader);

        ario_cover_handler_force_reload();
}

static void
ario_shell_coverdownloader_get_cover_from_album (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                 ArioMpdAlbum *ario_mpd_album,
                                                 ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START
        const gchar *artist;
        const gchar *album;

        if (!ario_mpd_album)
                return;

        artist = ario_mpd_album->artist;
        album = ario_mpd_album->album;

        if (!album || !artist)
                return;

        switch (operation) {
        case GET_AMAZON_COVERS:
                /* We update the progress bar */
                ario_shell_coverdownloader_progress_update (ario_shell_coverdownloader, artist, album);

                if (ario_cover_cover_exists (artist, album))
                        /* The cover already exists, we do nothing */
                        ++ario_shell_coverdownloader->priv->nb_covers_already_exist;
                else
                        /* We search for the cover on amazon */
                        ario_shell_coverdownloader_find_amazon_image (ario_shell_coverdownloader, artist, album);
                break;

        case REMOVE_COVERS: 
                /* We remove the cover from the ~/.gnome2/ario/covers/ directory */
                ario_cover_remove_cover (artist, album);
                break;

        default:
                break;
        }
}

static void 
ario_shell_coverdownloader_find_amazon_image (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                              const char *artist,
                                              const char *album)
{
        ARIO_LOG_FUNCTION_START
        GArray *size;
        GSList *data = NULL, *ario_cover_uris = NULL;
        gboolean ret;

        size = g_array_new (TRUE, TRUE, sizeof (int));

        /* If a cover is found on amazon, it is loaded in data(0) */
        ret = ario_cover_load_amazon_covers (artist,
                                             album,
                                             &ario_cover_uris,
                                             &size,
                                             &data,
                                             GET_FIRST_COVER,
                                             AMAZON_MEDIUM_COVER);

        /* If the cover is not too big and not too small (blank amazon image), we save it */
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
                ario_shell_coverdownloader_nocover (ario_shell_coverdownloader);
        }

        g_array_free (size, TRUE);
        g_slist_foreach (data, (GFunc) g_free, NULL);
        g_slist_free (data);
        g_slist_foreach (ario_cover_uris, (GFunc) g_free, NULL);
        g_slist_free (ario_cover_uris);
}
