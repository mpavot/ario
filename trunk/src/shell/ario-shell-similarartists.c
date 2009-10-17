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

#include "shell/ario-shell-similarartists.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "lib/gtk-builder-helpers.h"
#include "servers/ario-server.h"
#include "widgets/ario-playlist.h"

static gboolean ario_shell_similarartists_window_delete_cb (GtkWidget *window,
                                                            GdkEventAny *event,
                                                            ArioShellSimilarartists *shell_similarartists);
G_MODULE_EXPORT void ario_shell_similarartists_close_cb (GtkButton *button,
                                                         ArioShellSimilarartists *shell_similarartists);
G_MODULE_EXPORT void ario_shell_similarartists_lastfm_cb (GtkButton *button,
                                                          ArioShellSimilarartists *shell_similarartists);
G_MODULE_EXPORT void ario_shell_similarartists_add_cb (GtkButton *button,
                                                       ArioShellSimilarartists *shell_similarartists);
G_MODULE_EXPORT void ario_shell_similarartists_addall_cb (GtkButton *button,
                                                          ArioShellSimilarartists *shell_similarartists);

#define LASTFM_URI "http://ws.audioscrobbler.com/1.0/artist/%s/similar.xml"
#define MAX_ARTISTS 10
#define IMAGE_SIZE 120

/* Private attributes */
struct ArioShellSimilarartistsPrivate
{
        GtkTreeSelection *selection;
        GtkListStore *liststore;
        GThread *thread;

        gboolean closed;
        const gchar* artist;
};

/* Tree model columns */
enum
{
        IMAGE_COLUMN,
        ARTIST_COLUMN,
        SONGS_COLUMN,
        IMAGEURL_COLUMN,
        URL_COLUMN,
        N_COLUMN
};

#define ARIO_SHELL_SIMILARARTISTS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_SIMILARARTISTS, ArioShellSimilarartistsPrivate))
G_DEFINE_TYPE (ArioShellSimilarartists, ario_shell_similarartists, GTK_TYPE_WINDOW)

static void
ario_shell_similarartists_class_init (ArioShellSimilarartistsClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioShellSimilarartistsPrivate));
}

static void
ario_shell_similarartists_init (ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        shell_similarartists->priv = ARIO_SHELL_SIMILARARTISTS_GET_PRIVATE (shell_similarartists);

        /* Connect signal for window deletion */
        g_signal_connect (shell_similarartists,
                          "delete_event",
                          G_CALLBACK (ario_shell_similarartists_window_delete_cb),
                          shell_similarartists);

        /* Set window properties */
        gtk_window_set_title (GTK_WINDOW (shell_similarartists), "Ario");
        gtk_window_set_resizable (GTK_WINDOW (shell_similarartists), TRUE);
        gtk_container_set_border_width (GTK_CONTAINER (shell_similarartists), 5);
}

static GSList *
ario_shell_similarartists_parse_xml_file (char *xmldata,
                                          int size)
{
        ARIO_LOG_FUNCTION_START;
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        GSList *similar_artists = NULL;
        ArioSimilarArtist *similar_artist;

        /* Parse XML file */
        doc = xmlParseMemory (xmldata, size);
        if (doc == NULL ) {
                return NULL;
        }

        /* Get root of document */
        cur = xmlDocGetRootElement(doc);
        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* Check that the root node name is "similarartists" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "similarartists")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                /* For each artist node */
                if (!xmlStrcmp (cur->name, (const xmlChar *) "artist")) {
                        /* Create an ArioSimilarArtist */
                        similar_artist = (ArioSimilarArtist *) g_malloc0 (sizeof (ArioSimilarArtist));
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if ((!xmlStrcmp (cur2->name, (const xmlChar *) "name"))) {
                                        /* Fill name */
                                        similar_artist->name = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                } else if ((!xmlStrcmp (cur2->name, (const xmlChar *) "image"))) {
                                        /* Fill image */
                                        similar_artist->image = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                } else if ((!xmlStrcmp (cur2->name, (const xmlChar *) "url"))) {
                                        /* Fill URL */
                                        similar_artist->url = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                }
                        }
                        /* Append ArioSimilarArtist to the list */
                        similar_artists = g_slist_append (similar_artists, similar_artist);
                }
        }

        xmlFreeDoc (doc);

        return similar_artists;
}

void
ario_shell_similarartists_free_similarartist (ArioSimilarArtist *similar_artist)
{
        if (similar_artist) {
                g_free (similar_artist->name);
                g_free (similar_artist->url);
                g_free (similar_artist->image);
                g_free (similar_artist);
        }
}

static gboolean
ario_shell_similarartists_get_images_foreach (GtkTreeModel *model,
                                              GtkTreePath *path,
                                              GtkTreeIter *iter,
                                              ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        int size;
        char *data;
        GdkPixbufLoader *loader;
        GdkPixbuf *pixbuf, *tmp_pixbuf;
        int width, height;
        gchar *image_url;

        /* Get image URL of current row */
        gtk_tree_model_get (model,
                            iter,
                            IMAGEURL_COLUMN, &image_url,
                            -1);

        /* Download image */
        ario_util_download_file (image_url,
                                 NULL, 0, NULL,
                                 &size,
                                 &data);
        g_free (image_url);

        if (size == 0 || !data)
                return FALSE;

        /* Create pixbuf from image data */
        loader = gdk_pixbuf_loader_new ();
        gdk_pixbuf_loader_write (loader,
                                 (const guchar *) data,
                                 size,
                                 NULL);
        gdk_pixbuf_loader_close (loader, NULL);
        g_free (data);

        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        if (!pixbuf)
                return FALSE;

        /* Resize image to IMAGE_SIZE, keeping proportions */
        width = gdk_pixbuf_get_width (pixbuf);
        height = gdk_pixbuf_get_height (pixbuf);
        if (width > height) {
                tmp_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                      IMAGE_SIZE,
                                                      height * IMAGE_SIZE / width,
                                                      GDK_INTERP_BILINEAR);
        } else {
                tmp_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                      width * IMAGE_SIZE / height,
                                                      IMAGE_SIZE,
                                                      GDK_INTERP_BILINEAR);
        }
        g_object_unref (G_OBJECT (pixbuf));
        pixbuf = tmp_pixbuf;

        /* Set pixbuf in current row */
        gtk_list_store_set (shell_similarartists->priv->liststore, iter,
                            IMAGE_COLUMN, pixbuf,
                            -1);
        g_object_unref (G_OBJECT (pixbuf));

        return FALSE;
}

static gpointer
ario_shell_similarartists_get_images (ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;

        /* Get images of each row */
        gtk_tree_model_foreach (GTK_TREE_MODEL (shell_similarartists->priv->liststore),
                                (GtkTreeModelForeachFunc) ario_shell_similarartists_get_images_foreach,
                                shell_similarartists);

        return NULL;
}

GSList *
ario_shell_similarartists_get_similar_artists (const gchar *artist)
{
        ARIO_LOG_FUNCTION_START;
        char *keyword;
        char *xml_uri;
        int xml_size;
        char *xml_data;
        GSList *similar_artists;

        /* Format artist */
        keyword = ario_util_format_keyword (artist);

        /* Get last.fm uri */
        xml_uri = g_strdup_printf (LASTFM_URI, keyword);
        g_free (keyword);

        /* Download XML file */
        ario_util_download_file (xml_uri,
                                 NULL, 0, NULL,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);
        if (xml_size == 0) {
                return NULL;
        }

        /* Parse XML file */
        similar_artists = ario_shell_similarartists_parse_xml_file (xml_data,
                                                                    xml_size);
        g_free (xml_data);

        return similar_artists;
}

static void
ario_shell_similarartists_get_artists (ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        GSList *similar_artists, *tmp;
        ArioSimilarArtist *similar_artist;
        GtkTreeIter iter;
        int i = 0;
        gchar *songs_txt;
        GSList *songs = NULL;
        ArioServerAtomicCriteria atomic_criteria;
        ArioServerCriteria *criteria = NULL;

        /* Get list of similar artists */
        similar_artists = ario_shell_similarartists_get_similar_artists (shell_similarartists->priv->artist);

        atomic_criteria.tag = MPD_TAG_ITEM_ARTIST;
        criteria = g_slist_append (criteria, &atomic_criteria);

        /* For each similar artist */
        for (tmp = similar_artists; tmp; tmp = g_slist_next (tmp)) {
                /* Stop here if needed */
                if (++i > MAX_ARTISTS || shell_similarartists->priv->closed)
                        break;

                similar_artist = tmp->data;
                atomic_criteria.value = (gchar *) similar_artist->name;

                /* Get all songs of artist */
                songs = ario_server_get_songs (criteria, TRUE);

                /* Format number of songs for display */
                if (songs)
                        songs_txt = g_strdup_printf (_("%d songs"), g_slist_length (songs));
                else
                        songs_txt = g_strdup ("");
                g_slist_foreach (songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (songs);

                /* Append row */
                gtk_list_store_append (shell_similarartists->priv->liststore, &iter);
                gtk_list_store_set (shell_similarartists->priv->liststore, &iter,
                                    ARTIST_COLUMN, similar_artist->name,
                                    SONGS_COLUMN, songs_txt,
                                    IMAGEURL_COLUMN, similar_artist->image,
                                    URL_COLUMN, similar_artist->url,
                                    -1);
                g_free (songs_txt);
        }

        g_slist_foreach (similar_artists, (GFunc) ario_shell_similarartists_free_similarartist, NULL);
        g_slist_free (similar_artists);

        /* Launch a thread to download artist images */
        shell_similarartists->priv->thread = g_thread_create ((GThreadFunc) ario_shell_similarartists_get_images,
                                                              shell_similarartists,
                                                              TRUE,
                                                              NULL);
        g_slist_free (criteria);
}

GtkWidget *
ario_shell_similarartists_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioShellSimilarartists *shell_similarartists;
        GtkBuilder *builder;
        GtkWidget *treeview;
        gchar *artist;

        artist = ario_server_get_current_artist ();

        if (!artist)
                return NULL;

        shell_similarartists = g_object_new (TYPE_ARIO_SHELL_SIMILARARTISTS, NULL);

        g_return_val_if_fail (shell_similarartists->priv != NULL, NULL);
        shell_similarartists->priv->closed = FALSE;

        /* Build UI using GtkBuilder */
        builder = gtk_builder_helpers_new (UI_PATH "similar-artists.ui",
                                           shell_similarartists);

        /* Get pointers to various widgets */
        treeview = GTK_WIDGET (gtk_builder_get_object (builder, "treeview"));
        shell_similarartists->priv->liststore =
                GTK_LIST_STORE (gtk_builder_get_object (builder, "liststore"));

        /* Get tree selection */
        shell_similarartists->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (shell_similarartists->priv->selection,
                                     GTK_SELECTION_BROWSE);

        /* Set window properties */
        gtk_window_set_resizable (GTK_WINDOW (shell_similarartists), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell_similarartists), 350, 500);
        gtk_window_set_position (GTK_WINDOW (shell_similarartists), GTK_WIN_POS_CENTER);
        gtk_container_add (GTK_CONTAINER (shell_similarartists), GTK_WIDGET (gtk_builder_get_object (builder, "vbox")));

        gtk_widget_show_all (GTK_WIDGET (shell_similarartists));

        /* Refresh UI */
        while (gtk_events_pending ())
                gtk_main_iteration ();

        /* Get similar artists and fill tree */
        shell_similarartists->priv->artist = artist;
        ario_shell_similarartists_get_artists (shell_similarartists);
        g_object_unref (builder);

        return GTK_WIDGET (shell_similarartists);
}

static gboolean
ario_shell_similarartists_window_delete_cb (GtkWidget *window,
                                            GdkEventAny *event,
                                            ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        shell_similarartists->priv->closed = TRUE;

        /* Wait for end of images download thread */
        g_thread_join (shell_similarartists->priv->thread);

        /* Destroy window */
        gtk_widget_hide (GTK_WIDGET (shell_similarartists));
        gtk_widget_destroy (GTK_WIDGET (shell_similarartists));

        return TRUE;
}

void
ario_shell_similarartists_close_cb (GtkButton *button,
                                    ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        shell_similarartists->priv->closed = TRUE;

        /* Wait for end of images download thread */
        g_thread_join (shell_similarartists->priv->thread);

        /* Destroy window */
        gtk_widget_hide (GTK_WIDGET (shell_similarartists));
        gtk_widget_destroy (GTK_WIDGET (shell_similarartists));
}

void
ario_shell_similarartists_lastfm_cb (GtkButton *button,
                                     ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        gchar *url;

        /* Get selected row */
        if (gtk_tree_selection_get_selected (shell_similarartists->priv->selection,
                                             &treemodel,
                                             &iter)) {
                /* Get URL of selected row */
                gtk_tree_model_get (treemodel, &iter,
                                    URL_COLUMN, &url, -1);

                /* Open last.fm URL in web browser */
                ario_util_load_uri (url);
                g_free (url);
        }
}

void
ario_shell_similarartists_add_cb (GtkButton *button,
                                  ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        GSList *artists = NULL;
        gchar *artist;

        /* Get selected row */
        if (gtk_tree_selection_get_selected (shell_similarartists->priv->selection,
                                             &treemodel,
                                             &iter)) {
                /* Get artist of selected row */
                gtk_tree_model_get (treemodel, &iter,
                                    ARTIST_COLUMN, &artist, -1);

                /* Append artist songs to playlist */
                artists = g_slist_append (artists, artist);
                ario_server_playlist_append_artists (artists, PLAYLIST_ADD, -1);

                g_slist_foreach (artists, (GFunc) g_free, NULL);
                g_slist_free (artists);
        }
}

static gboolean
ario_shell_similarartists_addall_foreach (GtkTreeModel *model,
                                          GtkTreePath *path,
                                          GtkTreeIter *iter,
                                          GSList **artists)
{
        ARIO_LOG_FUNCTION_START;
        gchar *artist;

        /* Get artist of row */
        gtk_tree_model_get (model,
                            iter,
                            ARTIST_COLUMN, &artist,
                            -1);

        /* Append artist to the list */
        *artists = g_slist_append (*artists, artist);

        return FALSE;
}

void
ario_shell_similarartists_addall_cb (GtkButton *button,
                                     ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START;
        GSList *artists = NULL;

        /* Get list of all artists */
        gtk_tree_model_foreach (GTK_TREE_MODEL (shell_similarartists->priv->liststore),
                                (GtkTreeModelForeachFunc) ario_shell_similarartists_addall_foreach,
                                &artists);

        /* Appends songs of all artists to playlist */
        ario_server_playlist_append_artists (artists, PLAYLIST_ADD, -1);

        g_slist_foreach (artists, (GFunc) g_free, NULL);
        g_slist_free (artists);
}

void
ario_shell_similarartists_add_similar_to_playlist (const gchar *artist,
                                                   const int nb_entries)
{
        ARIO_LOG_FUNCTION_START;
        ArioSimilarArtist *similar_artist;
        GSList *artists = NULL, *similar_artists, *tmp;

        /* Get list of similar artists */
        similar_artists = ario_shell_similarartists_get_similar_artists (artist);

        /* For each similar artist */
        for (tmp = similar_artists; tmp; tmp = g_slist_next (tmp)) {
                /* Build a list of artists names */
                similar_artist = tmp->data;
                artists = g_slist_append (artists, similar_artist->name);
        }

        /* Append songs of artists to playlist */
        ario_server_playlist_append_artists (artists, PLAYLIST_ADD, nb_entries);

        g_slist_foreach (similar_artists, (GFunc) ario_shell_similarartists_free_similarartist, NULL);
        g_slist_free (similar_artists);
        g_slist_free (artists);
}
