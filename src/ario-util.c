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

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "ario-util.h"
#include "ario-debug.h"
#include "covers/ario-cover.h"
#include "preferences/ario-preferences.h"
#ifdef WIN32
#include <windows.h>
#else
#include <gcrypt.h>
#endif

static char *config_dir = NULL;

char *
ario_util_format_time (const int time)
{
        ARIO_LOG_FUNCTION_START
        int sec, min, hours;

        if (time < 0)
                return g_strdup_printf (_("n/a"));

        hours = (int)(time / 3600);
        min = (int)((time % 3600) / 60) ;
        sec = (time % 60);

        if (hours > 0)
                return g_strdup_printf ("%d:%02i:%02i", hours, min, sec);
        else
                return g_strdup_printf ("%02i:%02i", min, sec);
}

char *
ario_util_format_total_time (const int time)
{
        ARIO_LOG_FUNCTION_START
        gchar *res;
        gchar *temp1, *temp2;
        int temp_time;
        int sec, min, hours, days;

        if (time < 0)
                return g_strdup_printf (_("n/a"));

        days = (int)(time / 86400);
        temp_time = (time % 86400);

        hours = (int)(temp_time / 3600);
        temp_time = (temp_time % 3600);

        min = (int)(temp_time / 60);
        sec = (temp_time % 60);

        res = g_strdup_printf ("%d %s", sec, _("seconds"));

        if (min != 0) {
                temp1 = g_strdup_printf ("%d %s, ", min, _("minutes"));
                temp2 = g_strconcat (temp1, res, NULL);
                g_free (temp1);
                g_free (res);
                res = temp2;
        }

        if (hours != 0) {
                temp1 = g_strdup_printf ("%d %s, ", hours, _("hours"));
                temp2 = g_strconcat (temp1, res, NULL);
                g_free (temp1);
                g_free (res);
                res = temp2;
        }

        if (days != 0) {
                temp1 = g_strdup_printf ("%d %s, ", days, _("days"));
                temp2 = g_strconcat (temp1, res, NULL);
                g_free (temp1);
                g_free (res);
                res = temp2;
        }

        return res;
}

gchar *
ario_util_format_track (const gchar *track)
{
        ARIO_LOG_FUNCTION_START
        gchar **splited_track;
        gchar *res;

        if (!track)
                return NULL;

        /* Some tracks are x/y, we only want to display x */
        splited_track = g_strsplit (track, "/", 0);
        res = g_strdup_printf ("%02i", atoi (splited_track[0]));
        g_strfreev (splited_track);
        return res;
}

gchar *
ario_util_format_title (const ArioMpdSong *mpd_song)
{
        ARIO_LOG_FUNCTION_START
        gchar **splited_titles;
        gchar **splited_title;
        gchar **next_splited_title;
        gchar **splited_filenames;
        gchar *res;

        if (!mpd_song)
                return g_strdup (ARIO_MPD_UNKNOWN);
        if (mpd_song->title) {
                return g_strdup(mpd_song->title);
        } else if (mpd_song->name) {
                return g_strdup(mpd_song->name);
        } else {
                /* Original format is : "path/to/filename.extension" or http://path/to/address:port
                 * We only want to display filename or http address */
                if (!g_ascii_strncasecmp (mpd_song->file, "http://", 7)) {
                        return g_strdup (mpd_song->file);
                } else {
                        splited_titles = g_strsplit (mpd_song->file, "/", -1);

                        splited_title = splited_titles;
                        next_splited_title = splited_title + 1;
                        while  (*next_splited_title) {
                                splited_title = next_splited_title;
                                next_splited_title = splited_title + 1;
                        }

                        if (*splited_title) {
                                splited_filenames = g_strsplit (*splited_title, ".", 2);
                                res = g_strdup (*splited_filenames);
                                g_strfreev (splited_filenames);
                        } else {
                                res = g_strdup (ARIO_MPD_UNKNOWN);
                        }

                        g_strfreev (splited_titles);
                        return res;
                }
        }
}

static GtkIconFactory *factory = NULL;

void
ario_util_add_stock_icons (const char *stock_id,
                           const char *filename)
{
        ARIO_LOG_FUNCTION_START

        static int icon_size = 0;
        GdkPixbuf *pb;
        GtkIconSet *set;

        if (!icon_size)
                gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &icon_size, NULL);

        pb = gdk_pixbuf_new_from_file (filename,
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, stock_id, set);
        g_object_unref (G_OBJECT (pb));
}

void
ario_util_init_stock_icons (void)
{
        ARIO_LOG_FUNCTION_START

        factory = gtk_icon_factory_new ();

        ario_util_add_stock_icons ("ario", PIXMAP_PATH "ario.png");
        ario_util_add_stock_icons ("ario-play", PIXMAP_PATH "ario-play.png");
        ario_util_add_stock_icons ("ario-pause", PIXMAP_PATH "ario-pause.png");
        ario_util_add_stock_icons ("volume-zero", PIXMAP_PATH "volume-zero.png");
        ario_util_add_stock_icons ("volume-min", PIXMAP_PATH "volume-min.png");
        ario_util_add_stock_icons ("volume-medium", PIXMAP_PATH "volume-medium.png");
        ario_util_add_stock_icons ("volume-max", PIXMAP_PATH "volume-max.png");
        ario_util_add_stock_icons ("repeat", PIXMAP_PATH "repeat.png");
        ario_util_add_stock_icons ("random", PIXMAP_PATH "shuffle.png");

        gtk_icon_factory_add_default (factory);
}

gboolean
ario_util_has_stock_icons (const char *stock_id)
{
        return (gtk_icon_factory_lookup_default (stock_id) != NULL);
}

gint
ario_util_abs (const gint a)
{
        ARIO_LOG_FUNCTION_START
        if (a > 0)
                return a;
        else
                return -a;
}

const char *
ario_util_config_dir (void)
{
        ARIO_LOG_FUNCTION_START
        if (config_dir == NULL) {
                config_dir = g_build_filename (g_get_user_config_dir (),
                                               "ario",
                                               NULL);
                if (!g_file_test (config_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
                        ario_util_mkdir (config_dir);
        }

        return config_dir;
}

gboolean
ario_util_uri_exists (const char *uri)
{        
        g_return_val_if_fail (uri != NULL, FALSE);

        return g_file_test(uri, G_FILE_TEST_EXISTS);
}


void
ario_util_unlink_uri (const char *uri)
{
        g_unlink (uri);
}

void
ario_util_mkdir (const char *uri)
{
        g_mkdir_with_parents (uri, 0750);
}

void
ario_util_copy_file (const char *src_uri,
                     const char *dest_uri)
{
        gchar *contents;
        gsize length;

        if (! g_file_get_contents (src_uri,
                                   &contents,
                                   &length,
                                   NULL)) {
                return;
        }

        g_file_set_contents (dest_uri,
                             contents,
                             length,
                             NULL);
}

typedef struct _download_struct{
        char *data;
        int size;
}download_struct;

#define MAX_SIZE 5*1024*1024

static size_t
ario_util_write_data(void *buffer,
                     size_t size,
                     size_t nmemb,
                     download_struct *download_data)
{
        if(!size || !nmemb)
                return 0;
        if(download_data->data == NULL)
        {
                download_data->size = 0;
        }
        download_data->data = g_realloc(download_data->data,(gulong)(size*nmemb+download_data->size)+1);

        memset(&(download_data->data)[download_data->size], '\0', (size*nmemb)+1);
        memcpy(&(download_data->data)[download_data->size], buffer, size*nmemb);

        download_data->size += size*nmemb;
        if(download_data->size >= MAX_SIZE)
        {
                return 0;
        }
        return size*nmemb;
}

void
ario_util_download_file (const char *uri,
                         const char *post_data,
                         const int post_size,
                         const struct curl_slist *headers,
                         int* size,
                         char** data)
{
        ARIO_LOG_FUNCTION_START
        ARIO_LOG_DBG ("Download:%s", uri);
        download_struct download_data;
        gchar* address = NULL;
        int port;

        CURL* curl = curl_easy_init ();
        if(!curl)
                return;

        *size = 0;
        *data = NULL;

        download_data.size = 0;
        download_data.data = NULL;

        /* set uri */
        curl_easy_setopt (curl, CURLOPT_URL, uri);
        /* set callback data */
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, &download_data);
        /* set callback function */
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, ario_util_write_data);
        /* set timeout */
        curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 5);
        /* set redirect */
        curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION ,1);
        /* set NO SIGNAL */
        curl_easy_setopt (curl, CURLOPT_NOSIGNAL, TRUE);

        if (ario_conf_get_boolean (PREF_USE_PROXY, PREF_USE_PROXY_DEFAULT)) {
                address = ario_conf_get_string (PREF_PROXY_ADDRESS, PREF_PROXY_ADDRESS_DEFAULT);
                port =  ario_conf_get_integer (PREF_PROXY_PORT, PREF_PROXY_PORT_DEFAULT);
                if (address) {
                        curl_easy_setopt (curl, CURLOPT_PROXY, address);
                        curl_easy_setopt (curl, CURLOPT_PROXYPORT, port);
                } else {
                        ARIO_LOG_DBG ("Proxy enabled, but no proxy defined");
                }
        }

        if (post_data) {
	        curl_easy_setopt(curl, CURLOPT_POST, TRUE); 
	        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_size);
        }

        if (headers) {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        curl_easy_perform (curl);

        *size = download_data.size;
        *data = download_data.data;

        g_free(address);
        curl_easy_cleanup (curl);
}

void
ario_util_string_replace (char **string,
                          const char *old,
                          const char *new)
{
        gchar **strsplit;
        GString *str;
        int i;

        strsplit = g_strsplit (*string, old, 0);

        if (!strsplit[0])
                return;

        str = g_string_new (strsplit[0]);

        for (i = 1; strsplit[i] != NULL && g_utf8_collate (strsplit[i], ""); ++i) {
                g_string_append(str, new);
                g_string_append(str, strsplit[i]);
        }
        g_strfreev (strsplit);

        g_free (*string);
        *string = str->str;
        g_string_free (str, FALSE);
}

void
ario_util_load_uri (const char *uri)
{
        ARIO_LOG_FUNCTION_START
#ifdef WIN32
        ShellExecute (GetDesktopWindow(), "open", uri, NULL, NULL, SW_SHOW);
#else
        gchar *command = g_strdup_printf ("x-www-browser %s", uri);
        g_spawn_command_line_sync (command, NULL, NULL, NULL, NULL);
        g_free (command);
#endif                
}
                   
int
ario_util_min (const int a,
               const int b)
{
        ARIO_LOG_FUNCTION_START
        return (a > b ? b : a);
}

int
ario_util_max (const int a,
               const int b)
{
        ARIO_LOG_FUNCTION_START
        return (a > b ? a : b);
}

char *
ario_util_format_keyword (const char *keyword)
{
        ARIO_LOG_FUNCTION_START
        gchar *tmp;
        int i, j;
        int length;
        gchar *ret;

        /* List of modifications done on the keuword used for the search */
        const gchar *to_remove[] = {"cd 1", "cd 2", "cd 3", "cd 4", "cd 5",
                                    "cd1", "cd2", "cd3", "cd4", "cd5",
                                    "disc", "disk", "disque", NULL};

        /* Normalize keyword */
        ret = g_utf8_normalize (keyword, -1, G_NORMALIZE_ALL);

        /* Converts all upper case ASCII letters to lower case ASCII letters */
        tmp = g_ascii_strdown (ret, -1);
        g_free (ret);
        ret = tmp;

        /* We remove some useless words to make more accurate requests */
        for (i = 0; to_remove[i]; ++i) {
                ario_util_string_replace (&ret, to_remove[i], " ");
        }

        /* We escape the special characters */
        length = g_utf8_strlen (ret, -1);
        tmp = (char *) g_malloc0 (length);

        j = 0;
        for(i = 0; ret[i]; ++i) {
                if (g_unichar_isalnum (ret[i]) ||
                    (g_unichar_isspace (ret[i]) &&  j > 0 && !g_unichar_isspace (tmp[j-1]))) {
                        tmp[j] = ret[i];
                        ++j;
                }
        }
        tmp = g_realloc (tmp, j+1);
        tmp[j] = '\0';

        g_free (ret);
        ret = tmp;

        /* We escape spaces */
        ario_util_string_replace (&ret, " ", "%20");

        return ret;
}

#ifndef WIN32
gchar *
ario_util_md5 (const char *string)
{
        ARIO_LOG_FUNCTION_START
        guchar md5pword[16];
        gchar md5_response[33];
        int j;

        memset (md5_response, 0, sizeof (md5_response));

        gcry_md_hash_buffer (GCRY_MD_MD5, md5pword, string, strlen (string));

        for (j = 0; j < 16; j++) {
                char a[3];
                sprintf (a, "%02x", md5pword[j]);
                md5_response[2*j] = a[0];
                md5_response[2*j+1] = a[1];
        }

        return (g_strdup (md5_response));
}
#endif

#define DRAG_SIZE 70
#define MAX_COVERS_IN_DRAG 3
#define DRAG_COVER_STEP 0.15

GdkPixbuf *
ario_util_get_dnd_pixbuf (GSList *albums)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        GSList *covers = NULL;
        gchar *cover_path;
        ArioMpdAlbum *ario_mpd_album;
        int len = 0;
        GdkPixbuf *pixbuf, *cover;
        int i = 0;
        gdouble scale;

        if (!albums)
                return NULL;

        for (tmp = albums; tmp && len < MAX_COVERS_IN_DRAG; tmp = g_slist_next (tmp)) {
                ario_mpd_album = tmp->data;

                cover_path = ario_cover_make_ario_cover_path (ario_mpd_album->artist, ario_mpd_album->album, SMALL_COVER);
                if (ario_util_uri_exists (cover_path)) {
                        covers = g_slist_append (covers, cover_path);
                        ++len;
                }
        }

        if (len == 0) {
                pixbuf = NULL;
        } else if (len == 1) {
                pixbuf = gdk_pixbuf_new_from_file_at_size (covers->data, DRAG_SIZE, DRAG_SIZE, NULL);
        } else {
                scale = (1 - DRAG_COVER_STEP*(len-1));

                pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, DRAG_SIZE, DRAG_SIZE);
                gdk_pixbuf_fill (pixbuf, 0);

                for (tmp = covers; tmp; tmp = g_slist_next (tmp)) {
                        cover = gdk_pixbuf_new_from_file_at_size (tmp->data, (int) (scale*DRAG_SIZE), (int) (scale*DRAG_SIZE), NULL);
                        if (!cover)
                                continue;

                        gdk_pixbuf_composite (cover, pixbuf,
                                              (int) (i*DRAG_COVER_STEP*DRAG_SIZE), (int) (i*DRAG_COVER_STEP*DRAG_SIZE),
                                              (int) (scale*DRAG_SIZE), (int) (scale*DRAG_SIZE),
                                              (int) (i*DRAG_COVER_STEP*DRAG_SIZE), (int) (i*DRAG_COVER_STEP*DRAG_SIZE),
                                              1.0, 1.0,
                                              GDK_INTERP_HYPER,
                                              255);
                        g_object_unref (cover);
                        ++i;
                }
        }

        g_slist_foreach (covers, (GFunc) g_free, NULL);
        g_slist_free (covers);

        return pixbuf;
}
