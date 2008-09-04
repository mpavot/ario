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
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "lyrics/ario-lyrics-leoslyrics.h"
#include "lyrics/ario-lyrics.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define LEOSLYRICS_FIRST_URI "http://api.leoslyrics.com/api_search.php?auth=Ario&artist=%s&songtitle=%s"
#define LEOSLYRICS_SECOND_URI "http://api.leoslyrics.com/api_lyrics.php?auth=Ario&hid=%s"

static void ario_lyrics_leoslyrics_class_init (ArioLyricsLeoslyricsClass *klass);
static void ario_lyrics_leoslyrics_init (ArioLyricsLeoslyrics *lyrics_leoslyrics);
static void ario_lyrics_leoslyrics_finalize (GObject *object);
static ArioLyrics* ario_lyrics_leoslyrics_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                                      const char *artist,
                                                      const char *song,
                                                      const char *file);
static void ario_lyrics_leoslyrics_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                                          const gchar *artist,
                                                          const gchar *song,
                                                          GSList **candidates);
static ArioLyrics * ario_lyrics_leoslyrics_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                                      const ArioLyricsCandidate *candidate);

struct ArioLyricsLeoslyricsPrivate
{
        gboolean dummy;
};

static GObjectClass *parent_class = NULL;

GType
ario_lyrics_leoslyrics_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioLyricsLeoslyricsClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_lyrics_leoslyrics_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioLyricsLeoslyrics),
                        0,
                        (GInstanceInitFunc) ario_lyrics_leoslyrics_init
                };

                type = g_type_register_static (ARIO_TYPE_LYRICS_PROVIDER,
                                               "ArioLyricsLeoslyrics",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_lyrics_leoslyrics_get_id (ArioLyricsProvider *lyrics_provider)
{
        return "leoslyrics";
}

static gchar *
ario_lyrics_leoslyrics_get_name (ArioLyricsProvider *lyrics_provider)
{
        return "Leo's Lyrics";
}

static void
ario_lyrics_leoslyrics_class_init (ArioLyricsLeoslyricsClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioLyricsProviderClass *lyrics_provider_class = ARIO_LYRICS_PROVIDER_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_lyrics_leoslyrics_finalize;

        lyrics_provider_class->get_id = ario_lyrics_leoslyrics_get_id;
        lyrics_provider_class->get_name = ario_lyrics_leoslyrics_get_name;
        lyrics_provider_class->get_lyrics = ario_lyrics_leoslyrics_get_lyrics;
        lyrics_provider_class->get_lyrics_candidates = ario_lyrics_leoslyrics_get_lyrics_candidates;
        lyrics_provider_class->get_lyrics_from_candidate = ario_lyrics_leoslyrics_get_lyrics_from_candidate;
}

static void
ario_lyrics_leoslyrics_init (ArioLyricsLeoslyrics *lyrics_leoslyrics)
{
        ARIO_LOG_FUNCTION_START
        lyrics_leoslyrics->priv = g_new0 (ArioLyricsLeoslyricsPrivate, 1);
}

static void
ario_lyrics_leoslyrics_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsLeoslyrics *lyrics_leoslyrics;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_LYRICS_LEOSLYRICS (object));

        lyrics_leoslyrics = ARIO_LYRICS_LEOSLYRICS (object);

        g_return_if_fail (lyrics_leoslyrics->priv != NULL);
        g_free (lyrics_leoslyrics->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioLyricsProvider*
ario_lyrics_leoslyrics_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsLeoslyrics *leoslyrics;

        leoslyrics = g_object_new (TYPE_ARIO_LYRICS_LEOSLYRICS,
                                   NULL);

        g_return_val_if_fail (leoslyrics->priv != NULL, NULL);

        return ARIO_LYRICS_PROVIDER (leoslyrics);
}

static char *
ario_lyrics_leoslyrics_make_first_xml_uri (const gchar *artist,
                                           const gchar *song)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        int i;
        int length;
        gchar *conv_artist;
        gchar *conv_song;
        gchar *tmp;

        if (!artist || !song)
                return NULL;

        /* If the song in unknown, we don't search for a lyrics */
        if (!strcmp (song, ARIO_MPD_UNKNOWN))
                return NULL;

        conv_artist = g_strdup (artist);
        conv_song = g_strdup (song);

        /* Normalize */
        tmp = g_utf8_normalize (conv_artist, -1, G_NORMALIZE_ALL);
        g_free (conv_artist);
        conv_artist = tmp;

        /* Normalize */
        tmp = g_utf8_normalize (conv_song, -1, G_NORMALIZE_ALL);
        g_free (conv_song);
        conv_song = tmp;

        /* We escape special characters */
        length = g_utf8_strlen (conv_artist, -1);
        for(i = 0; i < length; ++i)
        {
                if (!g_unichar_isalnum (conv_artist[i])) {
                        conv_artist[i]=' ';
                }
        }
        length = g_utf8_strlen (conv_song, -1);
        for(i = 0; i < length; ++i)
        {
                if (!g_unichar_isalnum (conv_song[i])) {
                        conv_song[i]=' ';
                }
        }

        /* We escape spaces */
        ario_util_string_replace (&conv_artist, " ", "%20");
        ario_util_string_replace (&conv_song, " ", "%20");

        /* We make the xml uri with all the parameters */
        if (strcmp (artist, ARIO_MPD_UNKNOWN))
                xml_uri = g_strdup_printf (LEOSLYRICS_FIRST_URI, conv_artist, conv_song);
        else
                xml_uri = g_strdup_printf (LEOSLYRICS_FIRST_URI, "", conv_song);

        g_free (conv_artist);
        g_free (conv_song);

        return xml_uri;
}

static gchar *
ario_lyrics_leoslyrics_parse_first_xml_file (gchar *xmldata,
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

static ArioLyrics *
ario_lyrics_leoslyrics_parse_second_xml_file (gchar *xmldata,
                                              int size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlNodePtr cur3;
        xmlChar *xml_lyrics = NULL;
        xmlChar *xml_song = NULL;
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
                                        xml_song = xmlNodeGetContent (cur2);
                                        if (xml_song) {
                                                lyrics->title = g_strdup ((const gchar *) xml_song);
                                                xmlFree (xml_song);
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
ario_lyrics_leoslyrics_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                   const char *artist,
                                   const char *song,
                                   const char *file)
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
        xml_uri = ario_lyrics_leoslyrics_make_first_xml_uri (artist,
                                                             song);

        if (!xml_uri)
                return NULL;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 NULL, 0, NULL,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0)
                return NULL;

        if (g_strrstr (xml_data, " - Error report</title>")) {
                g_free (xml_data);
                return NULL;
        }

        /* We parse the xml file to extract the lyrics hid */
        hid = ario_lyrics_leoslyrics_parse_first_xml_file (xml_data,
                                                           xml_size);

        g_free (xml_data);
        if (!hid)
                return NULL;

        lyrics_uri = g_strdup_printf (LEOSLYRICS_SECOND_URI, hid);
        g_free (hid);

        ario_util_download_file (lyrics_uri,
                                 NULL, 0, NULL,
                                 &lyrics_size,
                                 &lyrics_data);
        g_free (lyrics_uri);
        if (lyrics_size == 0)
                return NULL;

        lyrics = ario_lyrics_leoslyrics_parse_second_xml_file (lyrics_data,
                                                               lyrics_size);

        g_free (lyrics_data);

        return lyrics;
}

static void
ario_lyrics_leoslyrics_parse_first_xml_file_for_candidates (ArioLyricsProvider *lyrics_provider,
                                                            gchar *xmldata,
                                                            int size,
                                                            GSList **candidates)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlNodePtr cur2;
        xmlNodePtr cur3;
        xmlNodePtr cur4;
        xmlChar *hid = NULL;
        xmlChar *xml_song = NULL;
        xmlChar *xml_artist = NULL;
        ArioLyricsCandidate *lyrics = NULL;

        doc = xmlParseMemory (xmldata, size);
        if (doc == NULL ) {
                return;
        }

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL) {
                xmlFreeDoc (doc);
                return;
        }

        /* We check that the root node name is "leoslyrics" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "leoslyrics")) {
                xmlFreeDoc (doc);
                return;
        }
        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "searchResults")){
                        for (cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next) {
                                if (!xmlStrcmp (cur2->name, (const xmlChar *) "result")) {
                                        hid = xmlGetProp (cur2, (const xmlChar *)"hid");
                                        if (hid) {
                                                lyrics = (ArioLyricsCandidate *) g_malloc0 (sizeof (ArioLyricsCandidate));
                                                lyrics->lyrics_provider = lyrics_provider;
                                                lyrics->data = g_strdup ((const gchar *) hid);
                                                xmlFree (hid);
                                                for (cur3 = cur2->xmlChildrenNode; cur3; cur3 = cur3->next) {
                                                        if ((cur3->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur3->name, (const xmlChar *) "title"))) {
                                                                xml_song = xmlNodeGetContent (cur3);
                                                                if (xml_song) {
                                                                        lyrics->title = g_strdup ((const gchar *) xml_song);
                                                                        xmlFree (xml_song);
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
                                                        *candidates = g_slist_append (*candidates, lyrics);
                                                        lyrics = NULL;
                                                }
                                        }
                                }
                        }
                }
        }

        xmlFreeDoc (doc);
}

static void
ario_lyrics_leoslyrics_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                              const gchar *artist,
                                              const gchar *title,
                                              GSList **candidates)
{
        ARIO_LOG_FUNCTION_START
        char *xml_uri;
        int xml_size;
        char *xml_data;

        /* We construct the uri to make a request */
        xml_uri = ario_lyrics_leoslyrics_make_first_xml_uri (artist,
                                                             title);

        if (!xml_uri)
                return;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 NULL, 0, NULL,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0)
                return;

        if (g_strrstr (xml_data, " - Error report</title>")) {
                return;
                g_free (xml_data);
        }

        /* We parse the xml file to extract the lyrics hid */
        ario_lyrics_leoslyrics_parse_first_xml_file_for_candidates (lyrics_provider,
                                                                    xml_data,
                                                                    xml_size,
                                                                    candidates);

        g_free (xml_data);
}

static ArioLyrics *
ario_lyrics_leoslyrics_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                  const ArioLyricsCandidate *candidate)
{
        ARIO_LOG_FUNCTION_START
        int lyrics_size;
        char *lyrics_data;
        ArioLyrics *lyrics = NULL;
        gchar *lyrics_uri;

        lyrics_uri = g_strdup_printf (LEOSLYRICS_SECOND_URI, candidate->data);

        ario_util_download_file (lyrics_uri,
                                 NULL, 0, NULL,
                                 &lyrics_size,
                                 &lyrics_data);
        g_free (lyrics_uri);
        if (lyrics_size == 0)
                return NULL;

        lyrics = ario_lyrics_leoslyrics_parse_second_xml_file (lyrics_data,
                                                               lyrics_size);

        g_free (lyrics_data);

        if (lyrics) {
                ario_lyrics_prepend_infos (lyrics);
                ario_lyrics_save_lyrics (candidate->artist,
                                         candidate->title,
                                         lyrics->lyrics);
        }

        return lyrics;
}