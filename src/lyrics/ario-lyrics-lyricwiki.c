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
#include "lyrics/ario-lyrics-lyricwiki.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"
#include <curl/curl.h>

#define LYRICWIKI_URI "http://lyricwiki.org/server.php"

static void ario_lyrics_lyricwiki_class_init (ArioLyricsLyricwikiClass *klass);
static void ario_lyrics_lyricwiki_init (ArioLyricsLyricwiki *lyrics_lyricwiki);
static void ario_lyrics_lyricwiki_finalize (GObject *object);
ArioLyrics* ario_lyrics_lyricwiki_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                              const char *artist,
                                              const char *song,
                                              const char *file);
static void ario_lyrics_lyricwiki_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                                         const gchar *artist,
                                                         const gchar *song,
                                                         GSList **candidates);
static ArioLyrics * ario_lyrics_lyricwiki_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                                     const ArioLyricsCandidate *candidate);

struct ArioLyricsLyricwikiPrivate
{
        gboolean dummy;
};

static GObjectClass *parent_class = NULL;

GType
ario_lyrics_lyricwiki_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioLyricsLyricwikiClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_lyrics_lyricwiki_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioLyricsLyricwiki),
                        0,
                        (GInstanceInitFunc) ario_lyrics_lyricwiki_init
                };

                type = g_type_register_static (ARIO_TYPE_LYRICS_PROVIDER,
                                               "ArioLyricsLyricwiki",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_lyrics_lyricwiki_get_id (ArioLyricsProvider *lyrics_provider)
{
        return "lyricwiki";
}

static gchar *
ario_lyrics_lyricwiki_get_name (ArioLyricsProvider *lyrics_provider)
{
        return "LyricWiki";
}

static void
ario_lyrics_lyricwiki_class_init (ArioLyricsLyricwikiClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioLyricsProviderClass *lyrics_provider_class = ARIO_LYRICS_PROVIDER_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_lyrics_lyricwiki_finalize;

        lyrics_provider_class->get_id = ario_lyrics_lyricwiki_get_id;
        lyrics_provider_class->get_name = ario_lyrics_lyricwiki_get_name;
        lyrics_provider_class->get_lyrics = ario_lyrics_lyricwiki_get_lyrics;
        lyrics_provider_class->get_lyrics_candidates = ario_lyrics_lyricwiki_get_lyrics_candidates;
        lyrics_provider_class->get_lyrics_from_candidate = ario_lyrics_lyricwiki_get_lyrics_from_candidate;
}

static void
ario_lyrics_lyricwiki_init (ArioLyricsLyricwiki *lyrics_lyricwiki)
{
        ARIO_LOG_FUNCTION_START
        lyrics_lyricwiki->priv = g_new0 (ArioLyricsLyricwikiPrivate, 1);
}

static void
ario_lyrics_lyricwiki_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsLyricwiki *lyrics_lyricwiki;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_LYRICS_LYRICWIKI (object));

        lyrics_lyricwiki = ARIO_LYRICS_LYRICWIKI (object);

        g_return_if_fail (lyrics_lyricwiki->priv != NULL);
        g_free (lyrics_lyricwiki->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioLyricsProvider*
ario_lyrics_lyricwiki_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsLyricwiki *lyricwiki;

        lyricwiki = g_object_new (TYPE_ARIO_LYRICS_LYRICWIKI,
                                  NULL);

        g_return_val_if_fail (lyricwiki->priv != NULL, NULL);

        return ARIO_LYRICS_PROVIDER (lyricwiki);
}

static ArioLyrics *
ario_lyrics_lyricwiki_parse_xml_file (gchar *xmldata,
                                      int size)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur;
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

        /* We check that the root node name is "SOAP-ENV:Envelope" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "Envelope")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        cur = cur->xmlChildrenNode;
        if (xmlStrcmp (cur->name, (const xmlChar *) "Body")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        cur = cur->xmlChildrenNode;
        if (xmlStrcmp (cur->name, (const xmlChar *) "getSongResponse")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        cur = cur->xmlChildrenNode;
        if (xmlStrcmp (cur->name, (const xmlChar *) "return")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if ((cur->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur->name, (const xmlChar *) "lyrics"))) {
                        xml_lyrics = xmlNodeGetContent (cur);
                        if (xml_lyrics) {
                                lyrics->lyrics = g_strdup ((const gchar *) xml_lyrics);
                                xmlFree (xml_lyrics);
                        }
                } else if ((cur->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur->name, (const xmlChar *) "song"))) {
                        xml_title = xmlNodeGetContent (cur);
                        if (xml_title) {
                                lyrics->title = g_strdup ((const gchar *) xml_title);
                                xmlFree (xml_title);
                        }
                } else if ((cur->type == XML_ELEMENT_NODE) && (xmlStrEqual (cur->name, (const xmlChar *) "artist"))) {
                        xml_artist = xmlNodeGetContent (cur);
                        if (xml_artist) {
                                lyrics->artist = g_strdup ((const gchar *) xml_artist);
                                xmlFree (xml_artist);
                        }
                }
        }

        xmlFreeDoc (doc);

        return lyrics;
}

ArioLyrics *
ario_lyrics_lyricwiki_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                  const char *artist,
                                  const char *title,
                                  const char *file)
{
        ARIO_LOG_FUNCTION_START
        int lyrics_size;
        char *lyrics_data;
        ArioLyrics *lyrics = NULL;
        xmlChar *xml_artist;
        xmlChar *xml_title;
        GString *msg;

        if (artist)
                xml_artist = xmlEncodeEntitiesReentrant (NULL, (const xmlChar *) artist);
        else
                xml_artist = (xmlChar *) g_strdup (ARIO_MPD_UNKNOWN);

        if (title)
                xml_title = xmlEncodeEntitiesReentrant (NULL, (const xmlChar *) title);
        else
                xml_title = (xmlChar *) g_strdup (ARIO_MPD_UNKNOWN);

        msg = g_string_new ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                            "<SOAP-ENV:Envelope SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" "
                            "xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                            "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
                            "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                            "xmlns:SOAP-ENC=\"http://schemas.xmlsoapL.org/soap/encoding/\" "
                            "xmlns:tns=\"urn:LyricWiki\">"
                            "<SOAP-ENV:Body>\n<tns:getSong xmlns:tns=\"urn:LyricWiki\">"
                            "<artist xsi:type=\"xsd:string\">");
        g_string_append(msg, (const gchar *) xml_artist); 
        g_string_append(msg, "</artist><song xsi:type=\"xsd:string\">");
        g_string_append(msg, (const gchar *) xml_title);
        g_string_append(msg, "</song></tns:getSong></SOAP-ENV:Body></SOAP-ENV:Envelope>\n");
        xmlFree (xml_artist);
        xmlFree (xml_title);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append (headers, "SOAPAction: urn:LyricWiki#getSong");
        headers = curl_slist_append (headers, "Content-Type: text/xml; charset=UTF-8");

        /* We load the xml file in xml_data */
        ario_util_download_file (LYRICWIKI_URI,
                                 msg->str, msg->len,
                                 headers,
                                 &lyrics_size,
                                 &lyrics_data);
        curl_slist_free_all (headers);
        g_string_free (msg, FALSE);

        if (lyrics_size == 0)
                return NULL;

        if (g_strrstr (lyrics_data, "<lyrics xsi:type=\"xsd:string\">Not found</lyrics>")) {
                g_free (lyrics_data);
                return NULL;
        }

        lyrics = ario_lyrics_lyricwiki_parse_xml_file (lyrics_data,
                                                       lyrics_size);

        g_free (lyrics_data);

        return lyrics;
}

static void
ario_lyrics_lyricwiki_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                             const gchar *artist,
                                             const gchar *title,
                                             GSList **candidates)
{
        ARIO_LOG_FUNCTION_START
        ArioLyrics *lyrics;
        ArioLyricsCandidate *candidate;

        lyrics = ario_lyrics_lyricwiki_get_lyrics (lyrics_provider,
                                                   artist,
                                                   title,
                                                   NULL);
        if (!lyrics)
                return;

        candidate = (ArioLyricsCandidate *) g_malloc0 (sizeof (ArioLyricsCandidate));
        candidate->artist = g_strdup (lyrics->artist);
        candidate->title = g_strdup (lyrics->title);
        candidate->data = g_strdup (lyrics->lyrics);
        candidate->lyrics_provider = lyrics_provider;

        *candidates = g_slist_append (*candidates, candidate);

        ario_lyrics_free (lyrics);
}

static ArioLyrics *
ario_lyrics_lyricwiki_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                 const ArioLyricsCandidate *candidate)
{
        ARIO_LOG_FUNCTION_START
        ArioLyrics *lyrics;

        lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
        lyrics->artist = g_strdup (candidate->artist);
        lyrics->title = g_strdup (candidate->title);
        lyrics->lyrics = g_strdup (candidate->data);

        ario_lyrics_prepend_infos (lyrics);
        ario_lyrics_save_lyrics (candidate->artist,
                                 candidate->title,
                                 lyrics->lyrics);

        return lyrics;
}
