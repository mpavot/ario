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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/eel-gconf-extensions.h"
#include "ario-lyrics.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define LEOSLYRICS_FIRST_URI "http://api.leoslyrics.com/api_search.php?auth=Ario&artist=%s&songtitle=%s"
#define LEOSLYRICS_SECOND_URI "http://api.leoslyrics.com/api_lyrics.php?auth=Ario&hid=%s"


static void ario_lyrics_create_ario_lyrics_dir (void);

gchar*
ario_lyrics_make_lyrics_path (const gchar *artist,
                              const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        char *ario_lyrics_path, *tmp;
        char *filename;
        const char *to_strip = "#/";

        filename = g_strdup_printf ("%s-%s.txt", artist, title);

        /* We replace some special characters with spaces. */
        for (tmp = filename; *tmp != '\0'; ++tmp) {
                if (strchr (to_strip, *tmp))
                        *tmp = ' ';
        }

        /* The returned path is ~/.config/ario/lyrics/filename */
        ario_lyrics_path = g_build_filename (ario_util_config_dir (), "lyrics", filename, NULL);
        g_free (filename);

        return ario_lyrics_path;
}

static void
ario_lyrics_create_ario_lyrics_dir (void)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_lyrics_dir;

        ario_lyrics_dir = g_build_filename (ario_util_config_dir (), "lyrics", NULL);

        /* If the lyrics directory doesn't exist, we create it */
        if (!ario_util_uri_exists (ario_lyrics_dir))
                ario_util_mkdir (ario_lyrics_dir);
        g_free (ario_lyrics_dir);
}

void
ario_lyrics_remove_lyrics (const gchar *artist,
                           const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_lyrics_path;

        if (!ario_lyrics_lyrics_exists (artist, title))
                return;

        /* Delete the lyrics*/
        ario_lyrics_path = ario_lyrics_make_lyrics_path (artist, title);
        if (ario_util_uri_exists (ario_lyrics_path))
                ario_util_unlink_uri (ario_lyrics_path);
        g_free (ario_lyrics_path);
}

static char *
ario_lyrics_make_first_xml_uri (const gchar *artist,
                                const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        int i;
        int length;
        gchar *conv_artist;
        gchar *conv_title;
        gchar *tmp;

        if (!artist || !title)
                return NULL;

        /* If the title in unknown, we don't search for a lyrics */
        if (!strcmp (title, ARIO_MPD_UNKNOWN))
                return NULL;

        conv_artist = g_strdup (artist);
        conv_title = g_strdup (title);

        /* Normalize */
        tmp = g_utf8_normalize (conv_artist, -1, G_NORMALIZE_ALL);
        g_free (conv_artist);
        conv_artist = tmp;

        /* Normalize */
        tmp = g_utf8_normalize (conv_title, -1, G_NORMALIZE_ALL);
        g_free (conv_title);
        conv_title = tmp;

        /* We escape special characters */
        length = g_utf8_strlen (conv_artist, -1);
        for(i = 0; i < length; ++i)
        {
                if (!g_unichar_isalnum (conv_artist[i])) {
                        conv_artist[i]=' ';
                }
        }
        length = g_utf8_strlen (conv_title, -1);
        for(i = 0; i < length; ++i)
        {
                if (!g_unichar_isalnum (conv_title[i])) {
                        conv_title[i]=' ';
                }
        }

        /* We escape spaces */
        ario_util_string_replace (&conv_artist, " ", "%20");
        ario_util_string_replace (&conv_title, " ", "%20");

        /* We make the xml uri with all the parameters */
        if (strcmp (artist, ARIO_MPD_UNKNOWN))
                xml_uri = g_strdup_printf (LEOSLYRICS_FIRST_URI, conv_artist, conv_title);
        else
                xml_uri = g_strdup_printf (LEOSLYRICS_FIRST_URI, "", conv_title);

        g_free (conv_artist);
        g_free (conv_title);

        return xml_uri;
}

static gchar *
ario_lyrics_parse_first_xml_file (gchar *xmldata,
                                  int size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlChar *hid = NULL;
        gchar *char_hid = NULL;

        doc = xmlParseMemory (xmldata, size);
        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* We check that the root node name is "leoslyrics" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "leoslyrics")) {
                xmlFreeDoc (doc);
                return NULL;
        }
        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "searchResults")){
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if (!xmlStrcmp (cur2->name, (const xmlChar *) "result")) {
                                        hid = xmlGetProp (cur2, (const unsigned char *)"hid");
                                        if (hid) {
                                                char_hid = g_strdup ((const gchar *) hid);
                                                xmlFree (hid);
                                                xmlFreeDoc (doc);
                                                return char_hid;
                                        }
                                }
                        }
                }
        }

        xmlFreeDoc (doc);

        return NULL;
}

static GSList *
ario_lyrics_parse_first_xml_file_for_candidates (gchar *xmldata,
                                                 int size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlNodePtr cur3;
        xmlNodePtr cur4;
        xmlChar *hid = NULL;
        xmlChar *xml_title = NULL;
        xmlChar *xml_artist = NULL;
        GSList *candidates = NULL;
        ArioLyricsCandidate *lyrics = NULL;

        doc = xmlParseMemory (xmldata, size);
        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* We check that the root node name is "leoslyrics" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "leoslyrics")) {
                xmlFreeDoc (doc);
                return NULL;
        }
        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "searchResults")){
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if (!xmlStrcmp (cur2->name, (const xmlChar *) "result")) {
                                        hid = xmlGetProp (cur2, (const xmlChar *)"hid");
                                        if (hid) {
                                                lyrics = (ArioLyricsCandidate *) g_malloc0 (sizeof (ArioLyricsCandidate));
                                                lyrics->hid = g_strdup ((const gchar *) hid);
                                                xmlFree (hid);
                                                for (cur3 = cur2->xmlChildrenNode; cur3; cur3 = cur3->next) {
                                                        if ((cur3->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur3->name, (const xmlChar *) "title"))) {
                                                                xml_title = xmlNodeGetContent (cur3);
                                                                if (xml_title) {
                                                                        lyrics->title = g_strdup ((const gchar *) xml_title);
                                                                        xmlFree (xml_title);
                                                                }
                                                        } else if ((cur3->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur3->name, (const xmlChar *) "artist"))) {
                                                                for (cur4 = cur3->xmlChildrenNode; cur4; cur4 = cur4->next) {
                                                                        if ((cur4->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur4->name, (const xmlChar *) "name"))) {
                                                                                xml_artist = xmlNodeGetContent (cur4);
                                                                                if (xml_artist) {
                                                                                        lyrics->artist = g_strdup ((const gchar *) xml_artist);
                                                                                        xmlFree (xml_artist);
                                                                                }
                                                                        }
                                                                }
                                                        }
                                                }
                                                if (lyrics) {
                                                        candidates = g_slist_append (candidates, lyrics);
                                                        lyrics = NULL;
                                                }
                                        }
                                }
                        }
                }
        }

        xmlFreeDoc (doc);

        return candidates;
}

static ArioLyrics *
ario_lyrics_parse_second_xml_file (gchar *xmldata,
                                   int size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlNodePtr cur3;
        xmlChar *xml_lyrics = NULL;
        xmlChar *xml_title = NULL;
        xmlChar *xml_artist = NULL;
        ArioLyrics *lyrics = NULL;

        doc = xmlParseMemory (xmldata, size);
        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* We check that the root node name is "leoslyrics" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "leoslyrics")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (xmlStrEqual (cur->name, (const xmlChar *) "lyric")){;
                        lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if ((cur2->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur2->name, (const xmlChar *) "text"))) {
                                        xml_lyrics = xmlNodeGetContent (cur2);
                                        if (xml_lyrics) {
                                                lyrics->lyrics = g_strdup ((const gchar *) xml_lyrics);
                                                xmlFree (xml_lyrics);
                                        }
                                } else if ((cur2->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur2->name, (const xmlChar *) "title"))) {
                                        xml_title = xmlNodeGetContent (cur2);
                                        if (xml_title) {
                                                lyrics->title = g_strdup ((const gchar *) xml_title);
                                                xmlFree (xml_title);
                                        }
                                } else if ((cur2->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur2->name, (const xmlChar *) "artist"))) {
                                        for (cur3 = cur2->xmlChildrenNode; cur3; cur3 = cur3->next) {
                                                if ((cur3->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur3->name, (const xmlChar *) "name"))) {
                                                        xml_artist = xmlNodeGetContent (cur3);
                                                        if (xml_artist) {
                                                                lyrics->artist = g_strdup ((const gchar *) xml_artist);
                                                                xmlFree (xml_artist);
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }

        xmlFreeDoc (doc);

        return lyrics;
}

static ArioLyrics *
ario_lyrics_get_local_lyrics (const gchar *artist,
                              const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        ArioLyrics *lyrics = NULL;
        gchar *ario_lyrics_path;
        gchar *read_data;

        if (!ario_lyrics_lyrics_exists (artist, title))
                return NULL;

        ario_lyrics_path = ario_lyrics_make_lyrics_path (artist, title);

        if (g_file_get_contents (ario_lyrics_path,
                                 &read_data, NULL, NULL)) {
                lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
                lyrics->lyrics = read_data;
                lyrics->artist = g_strdup (artist);
                lyrics->title = g_strdup (title);         
        }
        g_free (ario_lyrics_path);

        return lyrics;
}

static void
ario_lyrics_prepend_infos (ArioLyrics *lyrics)
{
        GString *string;
        gchar *toprepend;

        if (!lyrics)
                return;

        if (lyrics->artist && lyrics->title) {
                toprepend = g_strdup_printf ("%s - %s\n\n", lyrics->artist, lyrics->title);
        } else if (lyrics->artist) {
                toprepend = g_strdup_printf ("%s\n\n", lyrics->artist);
        } else if (lyrics->title) {
                toprepend = g_strdup_printf ("%s\n\n", lyrics->title);
        } else {
                return;
        }

        string = g_string_new (lyrics->lyrics);
        string = g_string_prepend (string, toprepend);
        g_free (lyrics->lyrics);
        lyrics->lyrics = g_string_free (string, FALSE);
        g_free (toprepend);
}

static ArioLyrics *
ario_lyrics_get_leoslyrics_lyrics (const gchar *artist,
                                   const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        int xml_size;
        char *xml_data;
        int lyrics_size;
        char *lyrics_data;
        gchar *hid;
        ArioLyrics *lyrics = NULL;
        gchar *lyrics_uri;

        /* We construct the uri to make a request */
        xml_uri = ario_lyrics_make_first_xml_uri (artist,
                                                  title);

        if (!xml_uri)
                return NULL;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0)
                return NULL;

        if (g_strrstr (xml_data, " - Error report</title>")) {
                return NULL;
                g_free (xml_data);
        }

        /* We parse the xml file to extract the lyrics hid */
        hid = ario_lyrics_parse_first_xml_file (xml_data,
                                                xml_size);

        g_free (xml_data);
        if (!hid)
                return NULL;

        lyrics_uri = g_strdup_printf (LEOSLYRICS_SECOND_URI, hid);
        g_free (hid);

        ario_util_download_file (lyrics_uri,
                                 &lyrics_size,
                                 &lyrics_data);
        g_free (lyrics_uri);
        if (lyrics_size == 0)
                return NULL;

        lyrics = ario_lyrics_parse_second_xml_file (lyrics_data,
                                                    lyrics_size);

        g_free (lyrics_data);

        return lyrics;
}

ArioLyrics *
ario_lyrics_get_lyrics (const gchar *artist,
                        const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        ArioLyrics *lyrics = NULL;

        if (ario_lyrics_lyrics_exists (artist, title)) {
                lyrics = ario_lyrics_get_local_lyrics (artist, title);
        } else {
                lyrics = ario_lyrics_get_leoslyrics_lyrics (artist, title);
                if (lyrics) {
                        ario_lyrics_prepend_infos (lyrics);
                        ario_lyrics_save_lyrics (artist,
                                                 title,
                                                 lyrics->lyrics);
                }
        }

        return lyrics;
}

ArioLyrics *
ario_lyrics_get_lyrics_from_hid (const gchar *artist,
                                 const gchar *title,
                                 const gchar *hid)
{
        ARIO_LOG_FUNCTION_START
        int lyrics_size;
        char *lyrics_data;
        ArioLyrics *lyrics = NULL;
        gchar *lyrics_uri;

        lyrics_uri = g_strdup_printf (LEOSLYRICS_SECOND_URI, hid);

        ario_util_download_file (lyrics_uri,
                                 &lyrics_size,
                                 &lyrics_data);
        g_free (lyrics_uri);
        if (lyrics_size == 0)
                return NULL;

        lyrics = ario_lyrics_parse_second_xml_file (lyrics_data,
                                                    lyrics_size);

        g_free (lyrics_data);

        if (lyrics) {
                ario_lyrics_prepend_infos (lyrics);
                ario_lyrics_save_lyrics (artist,
                                         title,
                                         lyrics->lyrics);
        }

        return lyrics;
}

GSList *
ario_lyrics_get_lyrics_candidates (const gchar *artist,
                                   const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        int xml_size;
        char *xml_data;
        GSList *candidates;

        /* We construct the uri to make a request */
        xml_uri = ario_lyrics_make_first_xml_uri (artist,
                                                  title);

        if (!xml_uri)
                return NULL;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0)
                return NULL;

        if (g_strrstr (xml_data, " - Error report</title>")) {
                return NULL;
                g_free (xml_data);
        }

        /* We parse the xml file to extract the lyrics hid */
        candidates = ario_lyrics_parse_first_xml_file_for_candidates (xml_data,
                                                                      xml_size);

        g_free (xml_data);

        return candidates;
}

void
ario_lyrics_free (ArioLyrics *lyrics)
{
        if (lyrics) {
                g_free (lyrics->artist);
                g_free (lyrics->title);
                g_free (lyrics->lyrics);
                g_free (lyrics);
        }
}

void
ario_lyrics_candidates_free (ArioLyricsCandidate *candidate)
{
        if (candidate) {
                g_free (candidate->artist);
                g_free (candidate->title);
                g_free (candidate->hid);
                g_free (candidate);
        }
}

gboolean
ario_lyrics_save_lyrics (const gchar *artist,
                         const gchar *title,
                         const gchar *lyrics)
{
        ARIO_LOG_FUNCTION_START
        gboolean ret;
        gchar *ario_lyrics_path;

        if (!artist || !title || !lyrics)
                return FALSE;

        /* If the lyrics directory doesn't exist, we create it */
        ario_lyrics_create_ario_lyrics_dir ();

        /* The path for the lyrics */
        ario_lyrics_path = ario_lyrics_make_lyrics_path (artist, title);

        ret = g_file_set_contents (ario_lyrics_path,
                                   lyrics, -1,
                                   NULL);

        g_free (ario_lyrics_path);

        return ret;
}

gboolean
ario_lyrics_lyrics_exists (const gchar *artist,
                           const gchar *title)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_lyrics_path;
        gboolean result;

        /* The path for the lyrics */
        ario_lyrics_path = ario_lyrics_make_lyrics_path (artist, title);

        result = ario_util_uri_exists (ario_lyrics_path);

        g_free (ario_lyrics_path);

        return result;
}

