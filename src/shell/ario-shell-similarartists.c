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

#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <time.h>
#include <glib/gi18n.h>
#include "shell/ario-shell-similarartists.h"
#include "lib/rb-glade-helpers.h"
#include "ario-debug.h"
#include "ario-util.h"

static void ario_shell_similarartists_class_init (ArioShellSimilarartistsClass *klass);
static void ario_shell_similarartists_init (ArioShellSimilarartists *shell_similarartists);
static void ario_shell_similarartists_finalize (GObject *object);
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

struct ArioShellSimilarartistsPrivate
{      
        GtkListStore *liststore;
        ArioPlaylist *playlist;
        GThread *thread;

        GtkTreeSelection *selection;

        gboolean closed;

        const gchar* artist;
};

typedef struct
{
        guchar *name;
        guchar *image;
        guchar *url;
} ArioSimilarArtist;

enum
{
        IMAGE_COLUMN,
        ARTIST_COLUMN,
        SONGS_COLUMN,
        IMAGEURL_COLUMN,
        URL_COLUMN,
        N_COLUMN
};

static GObjectClass *parent_class = NULL;

GType
ario_shell_similarartists_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_shell_similarartists_type = 0;

        if (ario_shell_similarartists_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioShellSimilarartistsClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_shell_similarartists_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioShellSimilarartists),
                        0,
                        (GInstanceInitFunc) ario_shell_similarartists_init
                };

                ario_shell_similarartists_type = g_type_register_static (GTK_TYPE_WINDOW,
                                                                 "ArioShellSimilarartists",
                                                                 &our_info, 0);
        }

        return ario_shell_similarartists_type;
}

static void
ario_shell_similarartists_class_init (ArioShellSimilarartistsClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_shell_similarartists_finalize;
}

static void
ario_shell_similarartists_init (ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        shell_similarartists->priv = g_new0 (ArioShellSimilarartistsPrivate, 1);

        g_signal_connect_object (G_OBJECT (shell_similarartists),
                                 "delete_event",
                                 G_CALLBACK (ario_shell_similarartists_window_delete_cb),
                                 shell_similarartists, 0);

        gtk_window_set_title (GTK_WINDOW (shell_similarartists), "Ario");
        gtk_window_set_resizable (GTK_WINDOW (shell_similarartists), TRUE);

        gtk_container_set_border_width (GTK_CONTAINER (shell_similarartists), 5);
}

static GSList *
ario_shell_similarartists_parse_xml_file (char *xmldata,
                                          int size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        GSList *similar_artists = NULL;
        ArioSimilarArtist *similar_artist;

        doc = xmlParseMemory (xmldata, size);

        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);

        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* We check that the root node name is "similarartists" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "similarartists")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "artist")) {
                        similar_artist = (ArioSimilarArtist *) g_malloc0 (sizeof (ArioSimilarArtist));
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if ((!xmlStrcmp (cur2->name, (const xmlChar *) "name"))) {
                                        similar_artist->name = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                } else if ((!xmlStrcmp (cur2->name, (const xmlChar *) "image"))) {
                                        similar_artist->image = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                } else if ((!xmlStrcmp (cur2->name, (const xmlChar *) "url"))) {
                                        similar_artist->url = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                }
                        }
                        similar_artists = g_slist_append (similar_artists, similar_artist);
                }
        }

        xmlFreeDoc (doc);

        return similar_artists;
}
static void
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
ario_shell_similarartists_refresh (gpointer data)
{
        while (gtk_events_pending ())
                gtk_main_iteration ();

        return FALSE;
}

static gboolean
ario_shell_similarartists_get_images_foreach (GtkTreeModel *model,
                                              GtkTreePath *path,
                                              GtkTreeIter *iter,
                                              ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        int size;
        char *data;
        GdkPixbufLoader *loader;
        GdkPixbuf *pixbuf, *tmp_pixbuf;
        int width, height;
        gchar *image_url;

        gtk_tree_model_get (model,
                            iter,
                            IMAGEURL_COLUMN, &image_url,
                            -1);

        ario_util_download_file (image_url,
                                 NULL, 0, NULL,
                                 &size,
                                 &data);
        g_free (image_url);

        if (size == 0 || !data)
                return FALSE;
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

        gtk_list_store_set (shell_similarartists->priv->liststore, iter,
                            IMAGE_COLUMN, pixbuf,
                            -1);
        g_object_unref (G_OBJECT (pixbuf));

        g_idle_add (ario_shell_similarartists_refresh, NULL);

        return FALSE;
}

static gpointer
ario_shell_similarartists_get_images (ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START

        gtk_tree_model_foreach (GTK_TREE_MODEL (shell_similarartists->priv->liststore),
                                (GtkTreeModelForeachFunc) ario_shell_similarartists_get_images_foreach,
                                shell_similarartists);

        return NULL;
}

static GSList *
ario_shell_similarartists_get_similar_artists (const gchar *artist)
{
        ARIO_LOG_FUNCTION_START
        char *keyword;
        char *xml_uri;
        int xml_size;
        char *xml_data;
        GSList *similar_artists;

        keyword = ario_util_format_keyword (artist);
        xml_uri = g_strdup_printf (LASTFM_URI, keyword);
        g_free (keyword);

        ario_util_download_file (xml_uri,
                                 NULL, 0, NULL,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);
        if (xml_size == 0) {
                return NULL;
        }

        similar_artists = ario_shell_similarartists_parse_xml_file (xml_data,
                                                                    xml_size);
        g_free (xml_data);

        return similar_artists;
}

static void
ario_shell_similarartists_get_artists (ArioShellSimilarartists *shell_similarartists,
                                       ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GSList *similar_artists, *tmp;
        ArioSimilarArtist *similar_artist;
        GtkTreeIter iter;
        int i = 0;
        gchar *songs_txt;
        GSList *songs = NULL;
        ArioMpdAtomicCriteria atomic_criteria;
        ArioMpdCriteria *criteria = NULL;

        similar_artists = ario_shell_similarartists_get_similar_artists (shell_similarartists->priv->artist);

        atomic_criteria.tag = MPD_TAG_ITEM_ARTIST;
        criteria = g_slist_append (criteria, &atomic_criteria);

        for (tmp = similar_artists; tmp; tmp = g_slist_next (tmp)) {
                if (++i > MAX_ARTISTS || shell_similarartists->priv->closed)
                        break;
                similar_artist = tmp->data;
                atomic_criteria.value = (gchar *) similar_artist->name;

                songs = ario_mpd_get_songs (mpd, criteria, TRUE);
                if (songs)
                        songs_txt = g_strdup_printf (_("%d songs"), g_slist_length (songs));
                else
                        songs_txt = g_strdup ("");
                g_slist_foreach (songs, (GFunc) ario_mpd_free_song, NULL);
                g_slist_free (songs);

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

        shell_similarartists->priv->thread = g_thread_create ((GThreadFunc) ario_shell_similarartists_get_images,
                                                               shell_similarartists,
                                                               TRUE,
                                                               NULL);
        g_slist_free (criteria);
}

GtkWidget *
ario_shell_similarartists_new (ArioMpd *mpd,
                               ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSimilarartists *shell_similarartists;
        GladeXML *xml;
        GtkWidget *treeview;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        gchar *artist;

        artist = ario_mpd_get_current_artist (mpd);

        if (!artist)
                return NULL;

        shell_similarartists = g_object_new (TYPE_ARIO_SHELL_SIMILARARTISTS, NULL);

        g_return_val_if_fail (shell_similarartists->priv != NULL, NULL);
        shell_similarartists->priv->closed = FALSE;
        shell_similarartists->priv->playlist = playlist;

        xml = rb_glade_xml_new (GLADE_PATH "similar-artists.glade",
                                "vbox",
                                shell_similarartists);
        treeview = glade_xml_get_widget (xml, "treeview");

        shell_similarartists->priv->liststore = gtk_list_store_new (N_COLUMN,
                                                                    GDK_TYPE_PIXBUF,
                                                                    G_TYPE_STRING,
                                                                    G_TYPE_STRING,
                                                                    G_TYPE_STRING,
                                                                    G_TYPE_STRING);

        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Image"), 
                                                           renderer,
                                                           "pixbuf", 
                                                           IMAGE_COLUMN, NULL);

        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, IMAGE_SIZE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);


        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           renderer,
                                                           "text", ARTIST_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Songs"),
                                                           renderer,
                                                           "text", SONGS_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (shell_similarartists->priv->liststore));
        shell_similarartists->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (shell_similarartists->priv->selection,
                                     GTK_SELECTION_BROWSE);

        gtk_window_set_resizable (GTK_WINDOW (shell_similarartists), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell_similarartists), 350, 500);
        gtk_window_set_position (GTK_WINDOW (shell_similarartists), GTK_WIN_POS_CENTER);

        gtk_container_add (GTK_CONTAINER (shell_similarartists), glade_xml_get_widget (xml, "vbox"));

        gtk_widget_show_all (GTK_WIDGET (shell_similarartists));

        while (gtk_events_pending ())
                gtk_main_iteration ();

        shell_similarartists->priv->artist = artist;
        ario_shell_similarartists_get_artists (shell_similarartists, mpd);
        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (shell_similarartists);
}

static void
ario_shell_similarartists_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSimilarartists *shell_similarartists;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_SIMILARARTISTS (object));

        shell_similarartists = ARIO_SHELL_SIMILARARTISTS (object);

        g_return_if_fail (shell_similarartists->priv != NULL);
        g_free (shell_similarartists->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ario_shell_similarartists_window_delete_cb (GtkWidget *window,
                                    GdkEventAny *event,
                                    ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        shell_similarartists->priv->closed = TRUE;

        g_thread_join (shell_similarartists->priv->thread);

        gtk_widget_hide (GTK_WIDGET (shell_similarartists));
        gtk_widget_destroy (GTK_WIDGET (shell_similarartists));

        return TRUE;
}

void
ario_shell_similarartists_close_cb (GtkButton *button,
                            ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        shell_similarartists->priv->closed = TRUE;

        g_thread_join (shell_similarartists->priv->thread);

        gtk_widget_hide (GTK_WIDGET (shell_similarartists));
        gtk_widget_destroy (GTK_WIDGET (shell_similarartists));
}

void
ario_shell_similarartists_lastfm_cb (GtkButton *button,
                                     ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        gchar *url;

        if (gtk_tree_selection_get_selected (shell_similarartists->priv->selection,
                                             &treemodel,
                                             &iter)) {
                gtk_tree_model_get (treemodel, &iter,
                                    URL_COLUMN, &url, -1);
                ario_util_load_uri (url);
                g_free (url);
        }
}

void
ario_shell_similarartists_add_cb (GtkButton *button,
                                  ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeModel *treemodel;
        GtkTreeIter iter;
        GSList *artists = NULL;
        gchar *artist;

        if (gtk_tree_selection_get_selected (shell_similarartists->priv->selection,
                                             &treemodel,
                                             &iter)) {
                gtk_tree_model_get (treemodel, &iter,
                                    ARTIST_COLUMN, &artist, -1);

                artists = g_slist_append (artists, artist);

                ario_playlist_append_artists (shell_similarartists->priv->playlist, artists, FALSE);

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
        ARIO_LOG_FUNCTION_START
        gchar *artist;

        gtk_tree_model_get (model,
                            iter,
                            ARTIST_COLUMN, &artist,
                            -1);

        *artists = g_slist_append (*artists, artist);

        return FALSE;
}

void
ario_shell_similarartists_addall_cb (GtkButton *button,
                                     ArioShellSimilarartists *shell_similarartists)
{
        ARIO_LOG_FUNCTION_START
        GSList *artists = NULL;

        gtk_tree_model_foreach (GTK_TREE_MODEL (shell_similarartists->priv->liststore),
                                (GtkTreeModelForeachFunc) ario_shell_similarartists_addall_foreach,
                                &artists);

        ario_playlist_append_artists (shell_similarartists->priv->playlist, artists, FALSE);

        g_slist_foreach (artists, (GFunc) g_free, NULL);
        g_slist_free (artists);
}

void
ario_shell_similarartists_add_similar_to_playlist (ArioMpd *mpd,
                                                   ArioPlaylist *playlist,
                                                   const gchar *artist)
{
        ARIO_LOG_FUNCTION_START
        ArioSimilarArtist *similar_artist;
        GSList *artists = NULL, *similar_artists, *tmp;

        similar_artists = ario_shell_similarartists_get_similar_artists (artist);
        for (tmp = similar_artists; tmp; tmp = g_slist_next (tmp)) {
                similar_artist = tmp->data;

                artists = g_slist_append (artists, similar_artist->name);
        }

        ario_playlist_append_artists (playlist, artists, FALSE);

        g_slist_foreach (similar_artists, (GFunc) ario_shell_similarartists_free_similarartist, NULL);
        g_slist_free (similar_artists);
        g_slist_free (artists);
}
