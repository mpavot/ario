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

#include "ario-util.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <glib/gi18n.h>
#include <gcrypt.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "ario-debug.h"
#include "covers/ario-cover.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"

/* Maximum number of covers to put in drag and drop icon */
#define MAX_COVERS_IN_DRAG 3

char *
ario_util_format_time (const int time)
{
        ARIO_LOG_FUNCTION_START;
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

void
ario_util_format_time_buf (const int time,
                           char *buf,
                           int buf_len)
{
        ARIO_LOG_FUNCTION_START;
        int sec, min, hours;

        if (time < 0)
                g_snprintf (buf, buf_len, _("n/a"));

        hours = (int)(time / 3600);
        min = (int)((time % 3600) / 60) ;
        sec = (time % 60);

        if (hours > 0)
                g_snprintf (buf, buf_len, "%d:%02i:%02i", hours, min, sec);
        else
                g_snprintf (buf, buf_len, "%02i:%02i", min, sec);
}

char *
ario_util_format_total_time (const int time)
{
        ARIO_LOG_FUNCTION_START;
        gchar *res;
        gchar *tmp;
        int temp_time;
        int sec, min, hours, days;

        if (time < 0)
                return g_strdup_printf (_("n/a"));

        /* Compute number of days */
        days = (int)(time / 86400);
        temp_time = (time % 86400);

        /* Compute number of hours */
        hours = (int)(temp_time / 3600);
        temp_time = (temp_time % 3600);

        /* Compute number of minutes */
        min = (int)(temp_time / 60);

        /* Compute number of seconds */
        sec = (temp_time % 60);

        /* Format result string */
        res = g_strdup_printf ("%d %s", sec, _("seconds"));
        if (min != 0) {
                tmp = g_strdup_printf ("%d %s, %s", min, _("minutes"), res);
                g_free (res);
                res = tmp;
        }

        if (hours != 0) {
                tmp = g_strdup_printf ("%d %s, %s", hours, _("hours"), res);
                g_free (res);
                res = tmp;
        }

        if (days != 0) {
                tmp = g_strdup_printf ("%d %s, %s", days, _("days"), res);
                g_free (res);
                res = tmp;
        }

        return res;
}

void
ario_util_format_track_buf (const gchar *track,
                            char *buf,
                            int buf_len)
{
        ARIO_LOG_FUNCTION_START;
        gchar *slash;
        gchar tmp[INTLEN];

        if (!track) {
                *buf = '\0';
                return;
        }

        /* Some tracks are x/y, we only want to display x */
        slash = g_strrstr (track, "/");
        if (slash) {
                g_snprintf (tmp, ario_util_min (INTLEN, slash - track + 1), "%s", track);
                g_snprintf (buf, buf_len, "%02i", atoi (tmp));
        } else {
                g_snprintf (buf, buf_len, "%02i", atoi (track));
        }
}

gchar *
ario_util_format_title (ArioServerSong *server_song)
{
        ARIO_LOG_FUNCTION_START;
        gchar *dot;
        gchar *slash;
        gchar *res = NULL;

        if (!server_song) {
                res = ARIO_SERVER_UNKNOWN;
        } else if (server_song->title) {
                res = server_song->title;
        } else if (server_song->name) {
                res = server_song->name;
        } else {
                /* Original format is : "path/to/filename.extension" or http://path/to/address:port
                 * or lastfm://path/to/address
                 * We only want to display filename or http/last.fm address */
                if (!g_ascii_strncasecmp (server_song->file, "http://", 7)
                    || !g_ascii_strncasecmp (server_song->file, "lastfm://", 7)) {
                        res = server_song->file;
                } else {
                        slash = g_strrstr (server_song->file, "/");
                        if (slash) {
                                dot = g_strrstr (slash + 1, ".");
                                if (dot)
                                        server_song->title = g_strndup (slash+1, dot - slash - 1);
                                else
                                        server_song->title = g_strdup (slash+1);
                                res = server_song->title;
                        } else {
                                res = server_song->file;
                        }
                }
        }
        return res;
}

static GtkIconFactory *factory = NULL;

void
ario_util_add_stock_icons (const char *stock_id,
                           const char *filename)
{
        ARIO_LOG_FUNCTION_START;

        static int icon_size = 0;
        GdkPixbuf *pb;
        GtkIconSet *set;

        if (!icon_size)
                gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &icon_size, NULL);

        pb = gdk_pixbuf_new_from_file (filename,
                                       NULL);
        set = gtk_icon_set_new_from_pixbuf (pb);
        gtk_icon_factory_add (factory, stock_id, set);
        g_object_unref (pb);
}

void
ario_util_init_stock_icons (void)
{
        ARIO_LOG_FUNCTION_START;

        factory = gtk_icon_factory_new ();

        ario_util_add_stock_icons ("ario", PIXMAP_PATH "ario.png");
        ario_util_add_stock_icons ("ario-play", PIXMAP_PATH "ario-play.png");
        ario_util_add_stock_icons ("ario-pause", PIXMAP_PATH "ario-pause.png");
        ario_util_add_stock_icons ("repeat", PIXMAP_PATH "repeat.png");
        ario_util_add_stock_icons ("random", PIXMAP_PATH "shuffle.png");

        gtk_icon_factory_add_default (factory);
}

gboolean
ario_util_has_stock_icons (const char *stock_id)
{
        return (gtk_icon_factory_lookup_default (stock_id) != NULL);
}

const char *
ario_util_config_dir (void)
{
        ARIO_LOG_FUNCTION_START;
        static char *config_dir = NULL;

        if (!config_dir) {
                config_dir = g_build_filename (g_get_user_config_dir (),
                                               "ario",
                                               NULL);
                if (!ario_file_test (config_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
                        ario_util_mkdir (config_dir);
        }

        return config_dir;
}

gboolean
ario_util_uri_exists (const char *uri)
{
        g_return_val_if_fail (uri != NULL, FALSE);

        return ario_file_test (uri, G_FILE_TEST_EXISTS);
}


void
ario_util_unlink_uri (const char *uri)
{
        ARIO_LOG_FUNCTION_START;
        gchar *uri_fse = g_filename_from_utf8 (uri, -1, NULL, NULL, NULL);
        if (!uri_fse)
                return;

        g_unlink (uri_fse);

        g_free (uri_fse);
}

void
ario_util_mkdir (const char *uri)
{
        ARIO_LOG_FUNCTION_START;
        gchar *uri_fse = g_filename_from_utf8 (uri, -1, NULL, NULL, NULL);
        if (!uri_fse)
                return;

        g_mkdir_with_parents (uri_fse, 0750);

        g_free (uri_fse);
}

void
ario_util_copy_file (const char *src_uri,
                     const char *dest_uri)
{
        ARIO_LOG_FUNCTION_START;
        gchar *contents;
        gsize length;

        /* Get file content */
        if (! ario_file_get_contents (src_uri,
                                      &contents,
                                      &length,
                                      NULL)) {
                return;
        }

        /* Write file content */
        ario_file_set_contents (dest_uri,
                                contents,
                                length,
                                NULL);
        g_free (contents);
}

typedef struct _download_struct{
        char *data;
        int size;
} download_struct;

/* Limit downloaded file to 5MB */
#define MAX_SIZE 5*1024*1024

static size_t
ario_util_write_data(void *buffer,
                     size_t size,
                     size_t nmemb,
                     download_struct *download_data)
{
        ARIO_LOG_FUNCTION_START;

        if (!size || !nmemb)
                return 0;

        if (download_data->data == NULL)
                download_data->size = 0;

        /* Increase buffer size if needed */
        download_data->data = g_realloc (download_data->data,
                                         (gulong)(size*nmemb+download_data->size) + 1);

        /* Append received data to buffer */
        memset (&(download_data->data)[download_data->size], '\0', (size*nmemb)+1);
        memcpy (&(download_data->data)[download_data->size], buffer, size*nmemb);

        /* Increase size */
        download_data->size += size*nmemb;
        if (download_data->size >= MAX_SIZE)
                return 0;

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
        ARIO_LOG_FUNCTION_START;
        ARIO_LOG_DBG ("Download:%s", uri);
        download_struct download_data;
        const gchar* address;
        int port;

        /* Initialize curl */
        CURL* curl = curl_easy_init ();
        if (!curl)
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
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)ario_util_write_data);
        /* set timeout */
        curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 5);
        /* set redirect */
        curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION ,1);
        /* set NO SIGNAL */
        curl_easy_setopt (curl, CURLOPT_NOSIGNAL, TRUE);

        /* Use a proxy if one is configured */
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

        /* Handles data for POST requests */
        if (post_data) {
                curl_easy_setopt (curl, CURLOPT_POST, TRUE);
                curl_easy_setopt (curl, CURLOPT_POSTFIELDS, post_data);
                curl_easy_setopt (curl, CURLOPT_POSTFIELDSIZE, post_size);
        }

        if (headers) {
                curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
        }

        /* Performs the request */
        curl_easy_perform (curl);

        *size = download_data.size;
        *data = download_data.data;

        curl_easy_cleanup (curl);
}

void
ario_util_string_replace (char **string,
                          const char *old,
                          const char *new)
{
        ARIO_LOG_FUNCTION_START;
        gchar **strsplit;
        GString *str;
        int i;

        /* Check if old is present in string */
        if (!g_strstr_len (*string, -1, old))
                return;

        /* Split string around 'old' */
        strsplit = g_strsplit (*string, old, 0);

        if (!strsplit)
                return;

        if (!strsplit[0]) {
                g_strfreev (strsplit);
                return;
        }

        /* Create a new string */
        str = g_string_new (strsplit[0]);

        /* Append splited parts to the new string */
        for (i = 1; strsplit[i] && g_utf8_collate (strsplit[i], ""); ++i) {
                g_string_append (str, new);
                g_string_append (str, strsplit[i]);
        }
        g_strfreev (strsplit);

        g_free (*string);
        *string = str->str;
        g_string_free (str, FALSE);
}

void
ario_util_load_uri (const char *uri)
{
        ARIO_LOG_FUNCTION_START;
#ifdef WIN32
        ShellExecute (GetDesktopWindow(), "open", uri, NULL, NULL, SW_SHOW);
#else
        gchar *command = g_strdup_printf ("xdg-open %s", uri);
        g_spawn_command_line_async (command, NULL);
        g_free (command);
#endif
}

char *
ario_util_format_keyword (const char *keyword)
{
        ARIO_LOG_FUNCTION_START;
        gchar *tmp;
        int i, j;
        int length;
        gchar *ret;

        /* List of modifications done on the keuword used for the search */
        const gchar *to_remove[] = {"cd 1", "cd 2", "cd 3", "cd 4", "cd 5",
                "cd1", "cd2", "cd3", "cd4", "cd5",
                "disc", "disk", "disque", "remastered", NULL};

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
        for (i = 0; ret[i]; ++i) {
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

gchar *
ario_util_md5 (const char *string)
{
        ARIO_LOG_FUNCTION_START;
        guchar md5pword[16];
        gchar md5_response[33] = {0};
        int j;

        gcry_md_hash_buffer (GCRY_MD_MD5, md5pword, string, strlen (string));

        for (j = 0; j < 16; j++) {
                char a[3];
                sprintf (a, "%02x", md5pword[j]);
                md5_response[2*j] = a[0];
                md5_response[2*j+1] = a[1];
        }

        return (g_strdup (md5_response));
}

#define DRAG_SIZE 70
#define DRAG_COVER_STEP 0.15

static GdkPixbuf *
ario_util_get_dnd_pixbuf_from_cover_paths (GSList *covers)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp;
        int len = g_slist_length (covers);
        GdkPixbuf *pixbuf, *cover;
        int i = 0;
        gdouble scale;

        if (len == 0) {
                /* No cover means no icon */
                pixbuf = NULL;
        } else if (len == 1) {
                /* Only one cover, the icon is made of this cover */
                pixbuf = gdk_pixbuf_new_from_file_at_size (covers->data, DRAG_SIZE, DRAG_SIZE, NULL);
        } else {
                /* Several covers */

                /* Compute scale */
                scale = (1 - DRAG_COVER_STEP*(len-1));

                /* Create empyt pixbuf */
                pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, DRAG_SIZE, DRAG_SIZE);
                gdk_pixbuf_fill (pixbuf, 0);

                for (tmp = covers; tmp; tmp = g_slist_next (tmp)) {
                        /* Integrate cover in pixbuf */
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

        return pixbuf;
}

GdkPixbuf *
ario_util_get_dnd_pixbuf_from_albums (const GSList *albums)
{
        ARIO_LOG_FUNCTION_START;
        const GSList *tmp;
        GSList *covers = NULL;
        gchar *cover_path;
        ArioServerAlbum *ario_server_album;
        int len = 0;
        GdkPixbuf *pixbuf;

        if (!albums)
                return NULL;

        /* Get cover of each album */
        for (tmp = albums; tmp && len < MAX_COVERS_IN_DRAG; tmp = g_slist_next (tmp)) {
                ario_server_album = tmp->data;

                cover_path = ario_cover_make_cover_path (ario_server_album->artist, ario_server_album->album, SMALL_COVER);
                if (ario_util_uri_exists (cover_path)) {
                        covers = g_slist_append (covers, cover_path);
                        ++len;
                } else {
                        g_free (cover_path);
                }
        }

        /* Get the icon from covers */
        pixbuf = ario_util_get_dnd_pixbuf_from_cover_paths (covers);

        g_slist_foreach (covers, (GFunc) g_free, NULL);
        g_slist_free (covers);

        return pixbuf;
}

GdkPixbuf *
ario_util_get_dnd_pixbuf (const GSList *criterias)
{
        ARIO_LOG_FUNCTION_START;
        const GSList *tmp;
        ArioServerAlbum *server_album;
        int len = 0;
        ArioServerCriteria *criteria;
        GSList *albums, *album_tmp;
        GdkPixbuf *pixbuf;
        gchar *cover_path;
        GSList *covers = NULL;

        if (!criterias)
                return NULL;

        /* Get covers from criterias */
        for (tmp = criterias; tmp && len < MAX_COVERS_IN_DRAG; tmp = g_slist_next (tmp)) {
                criteria = tmp->data;

                /* Get albums from criteria */
                albums = ario_server_get_albums (criteria);

                /* Get covers of albums */
                for (album_tmp = albums; album_tmp && len < MAX_COVERS_IN_DRAG; album_tmp = g_slist_next (album_tmp)) {
                        server_album = album_tmp->data;
                        cover_path = ario_cover_make_cover_path (server_album->artist, server_album->album, SMALL_COVER);
                        if (ario_util_uri_exists (cover_path)) {
                                covers = g_slist_append (covers, cover_path);
                                ++len;
                        } else {
                                g_free (cover_path);
                        }
                }
                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
        }

        /* Get the icon from covers */
        pixbuf = ario_util_get_dnd_pixbuf_from_cover_paths (covers);

        g_slist_foreach (covers, (GFunc) g_free, NULL);
        g_slist_free (covers);

        return pixbuf;
}

gchar *
ario_util_convert_from_iso8859 (const char *string)
{
        ARIO_LOG_FUNCTION_START;
        char *ret, *tmp;

        tmp = g_convert (string, -1, (const gchar *) "ISO-8859-1", "UTF8", NULL, NULL, NULL);
        ret = g_locale_from_utf8 (tmp, -1, NULL, NULL, NULL);
        g_free (tmp);

        return ret;
}

void
ario_util_sanitize_filename (char *filename)
{
        ARIO_LOG_FUNCTION_START;
        const char *to_strip = "#/*\"\\[]:;|=";
        char *tmp;

        /* We replace some special characters with spaces. */
        for (tmp = filename; *tmp != '\0'; ++tmp) {
                if (strchr (to_strip, *tmp))
                        *tmp = ' ';
        }
}

gboolean
ario_file_get_contents (const gchar *filename, gchar **contents,
                        gsize *length, GError **error)
{
        ARIO_LOG_FUNCTION_START;
        gboolean ret;
        gchar *filename_fse;

        /* Convert filename to locale */
        filename_fse = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
        if (!filename_fse) {
                if (error)
                        *error = g_error_new (G_FILE_ERROR, G_FILE_ERROR_NOENT,
                                              "File `%s' not found", filename);
                return FALSE;
        }

        /* Get file content */
        ret = g_file_get_contents (filename_fse, contents, length, error);

        g_free (filename_fse);

        return ret;
}

gboolean
ario_file_set_contents (const gchar *filename, const gchar *contents,
                        gsize length, GError **error)
{
        ARIO_LOG_FUNCTION_START;
        gboolean ret;
        gchar *filename_fse;

        /* Convert filename to locale */
        filename_fse = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
        if (!filename_fse) {
                if (error)
                        *error = g_error_new (G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                              "Could not write to file `%s'", filename);
                return FALSE;
        }

        /* Set file content */
        ret = g_file_set_contents (filename_fse, contents, length, error);

        g_free (filename_fse);

        return ret;
}

gboolean
ario_file_test (const gchar *filename, GFileTest test)
{
        ARIO_LOG_FUNCTION_START;
        gboolean ret;
        gchar *filename_fse;

        /* Convert filename to locale */
        filename_fse = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
        if (!filename_fse)
                return FALSE;

        /* Test file */
        ret = g_file_test (filename_fse, test);

        g_free (filename_fse);

        return ret;
}

const char *
ario_util_stristr (const char *haystack,
                   const char *needle)
{
        ARIO_LOG_FUNCTION_START;

        if (!needle || !*needle)
                return haystack;

        for (; *haystack; ++haystack) {
                if (toupper(*haystack) == toupper(*needle)) {
                        /*
                         * Matched starting char -- loop through remaining chars.
                         */
                        const char *h, *n;

                        for (h = haystack, n = needle; *h && *n; ++h, ++n) {
                                if (toupper(*h) != toupper(*n)) {
                                        break;
                                }
                        }
                        /* matched all of 'needle' to null termination */
                        if (!*n) {
                                /* return the start of the match */
                                return haystack;
                        }
                }
        }
        return 0;
}

GSList *
ario_util_gslist_randomize (GSList **list,
                            const int max)
{
        ARIO_LOG_FUNCTION_START;
        GSList *ret = NULL, *tmp;
        int i = 0;
        int len = g_slist_length (*list);

        for (i = 0; i < max; ++i) {
                if (len <= 0)
                        break;

                /* Get a random element in list */
                tmp = g_slist_nth (*list, rand()%len);

                /* Remove the element from list */
                *list = g_slist_remove_link (*list, tmp);

                /* Append the element to the new list */
                ret = g_slist_concat (ret, tmp);
                len--;
        }

        return ret;
}
