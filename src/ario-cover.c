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

#include <glib.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/eel-gconf-extensions.h"
#include "ario-cover.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define AMAZON_URI  "http://xml.amazon.%s/onca/xml3?t=webservices-20&dev-t=%s&KeywordSearch=%s&mode=%s&locale=%s&type=lite&page=1&f=xml"

#define COVER_SMALL "ImageUrlSmall"
#define COVER_MEDIUM "ImageUrlMedium"
#define COVER_LARGE "ImageUrlLarge"

static void ario_cover_create_ario_cover_dir (void);
static char* ario_cover_make_amazon_xml_uri (const char *artist, 
                                             const char *album);

static GSList* ario_cover_parse_amazon_xml_file (char *xmldata,
                                                int size,
                                                ArioCoverAmazonOperation operation,
                                                ArioCoverAmazonCoversSize ario_cover_size);

gchar *
ario_cover_make_ario_cover_path (const gchar *artist,
                                 const gchar *album,
                                 ArioCoverHomeCoversSize ario_cover_size)
{
        ARIO_LOG_FUNCTION_START
        char *ario_cover_path, *tmp;
        char *filename;
        const char *to_strip = "#/";

        /* There are 2 different files : a small one for the labums list
         * and a normal one for the album-cover widget */
        if (ario_cover_size == SMALL_COVER)
                filename = g_strdup_printf ("SMALL%s-%s.jpg", artist, album);
        else
                filename = g_strdup_printf ("%s-%s.jpg", artist, album);

        /* We replace some special characters with spaces. */
        for (tmp = filename; *tmp != '\0'; ++tmp) {
                if (strchr (to_strip, *tmp))
                        *tmp = ' ';
        }

        /* The returned path is ~/.config/ario/covers/filename */
        ario_cover_path = g_build_filename (ario_util_config_dir (), "covers", filename, NULL);
        g_free (filename);

        return ario_cover_path;
}

gboolean
ario_cover_cover_exists (const gchar *artist,
                         const gchar *album)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_cover_path, *small_ario_cover_path;
        gboolean result;

        /* The path for the normal cover */
        ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                            album,
                                            NORMAL_COVER);

        /* The path for the small cover */
        small_ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                                  album,
                                                  SMALL_COVER);

        /* We consider that the cover exists only if the normal and small covers exist */
        result = (ario_util_uri_exists (ario_cover_path) && ario_util_uri_exists (small_ario_cover_path));

        g_free (ario_cover_path);
        g_free (small_ario_cover_path);

        return result;
}

void
ario_cover_create_ario_cover_dir (void)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_cover_dir;

        ario_cover_dir = g_build_filename (ario_util_config_dir (), "covers", NULL);

        /* If the cover directory doesn't exist, we create it */
        if (!ario_util_uri_exists (ario_cover_dir))
                ario_util_mkdir (ario_cover_dir);
        g_free (ario_cover_dir);
}

gboolean
ario_cover_size_is_valid (int size)
{
        ARIO_LOG_FUNCTION_START
        /* return true if the cover isn't too big or too small (blank amazon image) */
        return (size < 1024 * 200 && size > 900);
}

static GSList *
ario_cover_parse_amazon_xml_file (char *xmldata,
                                  int size,
                                  ArioCoverAmazonOperation operation,
                                  ArioCoverAmazonCoversSize ario_cover_size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlChar *key;
        GSList *ario_cover_uris = NULL;
        char *ario_cover_size_text;

        /* Amazon provides 3 different covers sizes. By default, we use the medium one */
        switch (ario_cover_size)
        {
        case AMAZON_SMALL_COVER:
                ario_cover_size_text = COVER_SMALL;
                break;
        case AMAZON_MEDIUM_COVER:
                ario_cover_size_text = COVER_MEDIUM;
                break;
        case AMAZON_LARGE_COVER:
                ario_cover_size_text = COVER_LARGE;
                break;
        default:
                ario_cover_size_text = COVER_MEDIUM;
                break;
        }

        doc = xmlParseMemory (xmldata, size);

        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);

        if (cur == NULL) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* We check that the root node name is "ProductInfo" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "ProductInfo")) {
                xmlFreeDoc (doc);
                return NULL;
        }
        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *)"Details")){
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if ((!xmlStrcmp (cur2->name, (const xmlChar *) ario_cover_size_text))) {
                                        /* A possible cover uri has been found, we add it to the list*/
                                        key = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
                                        ario_cover_uris = g_slist_append (ario_cover_uris, key);
                                        if (operation == GET_FIRST_COVER) {
                                                /* If we only want one cover, we now stop to parse the file */
                                                xmlFreeDoc (doc);
                                                return ario_cover_uris;
                                        }
                                }
                        }
                }
        }

        xmlFreeDoc (doc);

        return ario_cover_uris;
}

static char *
ario_cover_make_amazon_xml_uri (const char *artist,
                                const char *album)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        char *keywords;
        char *locale;
        const char *music_mode;
        const char *ext;
        char *tmp;
        int i;
        int length;

        /* This is the key used to send requests on the amazon WebServices */
        const char *mykey = "1BDCAEREYT743R9SXE02";

        /* List of modifications done on the keuword used for the search */
        const gchar *to_replace[] = {"é", "è", "ê", "à", "ù", "ç", "ö", "#", "/", "?", "'", "-", "\"", "&", ":", "*", "(", ")", NULL};
        const gchar *replace_to[] = {"e", "e", "e", "a", "u", "c", "o", " ", " ", " ", " ", " ", " ",  " ", " ", " ", " ", " ", NULL};
        const gchar *to_remove[] = {"cd 1", "cd 2", "cd 3", "cd 4", "CD 5", "disc", "disk", "disque", NULL};

        if (!album || !artist)
                return NULL;

        /* If the artist in unknown, we don't search for a cover */
        if (!strcmp (artist, ARIO_MPD_UNKNOWN))
                return NULL;

        /* If the album is unknown, we only use the artist to search for the cover */
        if (!strcmp (album, ARIO_MPD_UNKNOWN))
                keywords = g_strdup (artist);
        else
                keywords = g_strdup_printf ("%s %s", artist, album);

        /* Normalize keywords */
        tmp = g_utf8_normalize(keywords, -1, G_NORMALIZE_ALL);
        g_free(keywords);
        keywords = tmp;
        
        /* Converts all upper case ASCII letters to lower case ASCII letters */
        tmp = g_ascii_strdown(keywords, -1);
        g_free(keywords);
        keywords = tmp;

        /* We replace some special characters to make more accurate requests */
        for (i = 0; to_replace[i]; ++i) {
                if (replace_to[i])
                        ario_util_string_replace (&keywords, to_replace[i], replace_to[i]);
        }

        /* We remove some useless words to make more accurate requests */
        for (i = 0; to_remove[i]; ++i) {
                ario_util_string_replace (&keywords, to_remove[i], " ");
        }

        /* We escape the other special characters */
        length = g_utf8_strlen(keywords, -1);
	for(i = 0; i < length; ++i)
	{
		if (!g_unichar_isalnum(keywords[i])) {
		        keywords[i]=' ';
		}
	}
	
        /* We escape spaces */
        ario_util_string_replace (&keywords, " ", "%20");
        
        /*
        tmp = gnome_vfs_escape_string (keywords);
        g_free (keywords);
        keywords = tmp;
        */

        /* What is the amazon country choosen in the preferences? */
        locale = eel_gconf_get_string (CONF_COVER_AMAZON_COUNTRY, "com");

        /* The default mode is "music" and the default extension is ".com" */
        music_mode = "music";
        ext = "com";

        /* The french amazon need a different mode */
        if (!strcmp (locale, "fr"))
                music_mode = "music-fr";

        /* The japanese amazon need a different mode and a different extension */
        if (!strcmp (locale, "jp")) {
                music_mode = "music-jp";
                ext = "co.jp";
        }

        /* For amazon.com, we need to use "us" for the "locale" argument */
        if (!strcmp (locale, "com")) {
                g_free (locale);
                locale = g_strdup ("us");
        }

        /* We make the xml uri with all the parameters */
        xml_uri = g_strdup_printf (AMAZON_URI, ext, mykey, keywords, music_mode, locale);

        g_free (keywords);
        g_free (locale);

        return xml_uri;
}

gboolean
ario_cover_load_amazon_covers (const char *artist,
                               const char *album,
                               GSList **ario_cover_uris,
                               GArray **file_size,
                               GSList **file_contents,
                               ArioCoverAmazonOperation operation,
                               ArioCoverAmazonCoversSize ario_cover_size)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        int xml_size;
        char *xml_data;
        GSList *temp;
        int temp_size;
        char *temp_contents;
        gboolean ret;

        /* We construct the uri to make a request on the amazon WebServices */
        xml_uri = ario_cover_make_amazon_xml_uri (artist,
                                                  album);

        if (!xml_uri)
                return FALSE;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0) {
                return FALSE;
        }

        if (g_strrstr (xml_data, "404 Not Found")) {
                return FALSE;
                g_free (xml_data);
        }

        /* We parse the xml file to extract the cover uris */
        *ario_cover_uris = ario_cover_parse_amazon_xml_file (xml_data,
                                                             xml_size,
                                                             operation,
                                                             ario_cover_size);

        g_free (xml_data);

        /* By default, we return an error */
        ret = FALSE;

        for (temp = *ario_cover_uris; temp; temp = temp->next) {
                if (temp->data) {
                        /* For each cover uri, we load the image data in temp_contents */
                        ario_util_download_file (temp->data,
                                            &temp_size,
                                            &temp_contents);
                        if (ario_cover_size_is_valid (temp_size)) {
                                /* If the cover is not too big and not too small (blank amazon image), we append it to file_contents */
                                g_array_append_val (*file_size, temp_size);
                                *file_contents = g_slist_append (*file_contents, temp_contents);
                                /* If at least one cover is found, we return OK */
                                ret = TRUE;
                        }
                }
        }

        return ret;
}

void
ario_cover_remove_cover (const gchar *artist,
                         const gchar *album)
{
        ARIO_LOG_FUNCTION_START
        gchar *small_ario_cover_path;
        gchar *ario_cover_path;

        if (!ario_cover_cover_exists (artist, album))
                return;

        /* Delete the small cover*/
        small_ario_cover_path = ario_cover_make_ario_cover_path (artist, album, SMALL_COVER);
        if (ario_util_uri_exists (small_ario_cover_path))
                ario_util_unlink_uri (small_ario_cover_path);
        g_free (small_ario_cover_path);

        /* Delete the normal cover*/
        ario_cover_path = ario_cover_make_ario_cover_path (artist, album, NORMAL_COVER);
        if (ario_util_uri_exists (ario_cover_path))
                ario_util_unlink_uri (ario_cover_path);
        g_free (ario_cover_path);
}

static gboolean
ario_cover_can_overwrite_cover (const gchar *artist,
                                const gchar *album,
                                ArioCoverOverwriteMode overwrite_mode)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval;

        /* If the cover already exists, we can ask, replace or skip depending of the overwrite_mode argument */
        if (ario_cover_cover_exists (artist, album)) {
                switch (overwrite_mode) {
                case OVERWRITE_MODE_ASK:
                        dialog = gtk_message_dialog_new (NULL,
                                                         GTK_DIALOG_MODAL,
                                                         GTK_MESSAGE_QUESTION,
                                                         GTK_BUTTONS_YES_NO,
                                                         _("The cover already exists. Do you want to replace it?"));

                        retval = gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                        /* If the user doesn't say "yes", we don't overwrite the cover */
                        if (retval != GTK_RESPONSE_YES)
                                return FALSE;
                        break;
                case OVERWRITE_MODE_REPLACE:
                        /* We overwrite the cover */
                        return TRUE;
                        break;
                case OVERWRITE_MODE_SKIP:
                        /* We don't overwrite the cover */
                        return FALSE;
                default:
                        /* By default, we don't overwrite the cover */
                        return FALSE;
                }
        }

        /*If the cover doesn't exists, we return TRUE */
        return TRUE;
}

gboolean
ario_cover_save_cover (const gchar *artist,
                       const gchar *album,
                       char *data,
                       int size,
                       ArioCoverOverwriteMode overwrite_mode)
{
        ARIO_LOG_FUNCTION_START
        gboolean ret;
        gchar *ario_cover_path, *small_ario_cover_path;
        GdkPixbufLoader *loader;
        GdkPixbuf *pixbuf, *small_pixbuf;
        int width, height;

        if (!artist || !album || !data)
                return FALSE;

        /* If the cover directory doesn't exist, we create it */
        ario_cover_create_ario_cover_dir ();

        /* If the cover already exists, can we overwrite it? */
        if (!ario_cover_can_overwrite_cover (artist, album, overwrite_mode))
                return FALSE;

        /* The path for the normal cover */
        ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                               album,
                                               NORMAL_COVER);

        /* The path for the small cover */
        small_ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                                     album,
                                                     SMALL_COVER);

        loader = gdk_pixbuf_loader_new ();

        /*By default, we return an error */
        ret = FALSE;

        if (gdk_pixbuf_loader_write (loader,
                                     (const guchar *)data,
                                     size,
                                     NULL)) {
                gdk_pixbuf_loader_close (loader, NULL);

                pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
                width = gdk_pixbuf_get_width (pixbuf);
                height = gdk_pixbuf_get_height (pixbuf);

                /* We resize the pixbuf to save the small cover.
                 * To do it, we keep the original aspect ratio and
                 * we limit max (height, width) by COVER_SIZE */
                if (width > height) {
                        small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                                COVER_SIZE,
                                                                height * COVER_SIZE / width,
                                                                GDK_INTERP_BILINEAR);
                } else {
                        small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                                width * COVER_SIZE / height,
                                                                COVER_SIZE,
                                                                GDK_INTERP_BILINEAR);
                }

                /* We save the normal and the small covers */
                if (gdk_pixbuf_save (pixbuf, ario_cover_path, "jpeg", NULL, NULL) &&
                    gdk_pixbuf_save (small_pixbuf, small_ario_cover_path, "jpeg", NULL, NULL)) {
                        /* If we succeed in the 2 operations, we return OK */
                        ret = TRUE;
                }
                g_object_unref (G_OBJECT (pixbuf));
                g_object_unref (G_OBJECT (small_pixbuf));
        }

        g_free (ario_cover_path);
        g_free (small_ario_cover_path);

        return ret;
}
