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

#include "covers/ario-cover-lastfm.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "covers/ario-cover.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define LASTFM_API_KEY "93bea35d40c4a58e034d14eb85e840c2"
#define LASTFM_URI  "http://ws.audioscrobbler.com/2.0/?api_key=%s&artist=%s&album=%s&method=album.getinfo"

#define COVER_SMALL "small"
#define COVER_MEDIUM "medium"
#define COVER_LARGE "large"

static char* ario_cover_lastfm_make_xml_uri (const char *artist, 
                                             const char *album);
static GSList* ario_cover_lastfm_parse_xml_file (char *xmldata,
                                                 int size,
                                                 ArioCoverProviderOperation operation,
                                                 const char *cover_size);
gboolean ario_cover_lastfm_get_covers (ArioCoverProvider *cover_provider,
                                       const char *artist,
                                       const char *album,
                                       const char *file,
                                       GArray **file_size,
                                       GSList **file_contents,
                                       ArioCoverProviderOperation operation);

G_DEFINE_TYPE (ArioCoverLastfm, ario_cover_lastfm, ARIO_TYPE_COVER_PROVIDER)

static gchar *
ario_cover_lastfm_get_id (ArioCoverProvider *cover_provider)
{
        return "lastfm";
}

static gchar *
ario_cover_lastfm_get_name (ArioCoverProvider *cover_provider)
{
        return "Last.fm";
}

static void
ario_cover_lastfm_class_init (ArioCoverLastfmClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverProviderClass *cover_provider_class = ARIO_COVER_PROVIDER_CLASS (klass);

        cover_provider_class->get_id = ario_cover_lastfm_get_id;
        cover_provider_class->get_name = ario_cover_lastfm_get_name;
        cover_provider_class->get_covers = ario_cover_lastfm_get_covers;
}

static void
ario_cover_lastfm_init (ArioCoverLastfm *cover_lastfm)
{
        ARIO_LOG_FUNCTION_START;
}

ArioCoverProvider*
ario_cover_lastfm_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioCoverLastfm *lastfm;

        lastfm = g_object_new (TYPE_ARIO_COVER_LASTFM,
                               NULL);

        return ARIO_COVER_PROVIDER (lastfm);
}

static GSList *
ario_cover_lastfm_parse_xml_file (char *xmldata,
                                  int size,
                                  ArioCoverProviderOperation operation,
                                  const char *cover_size)
{
        ARIO_LOG_FUNCTION_START;
        xmlDocPtr doc;
        xmlNodePtr cur;
        xmlChar *key;
        xmlChar *attr;
        GSList *ario_cover_uris = NULL;

        doc = xmlParseMemory (xmldata, size);

        if (doc == NULL ) {
                return NULL;
        }

        cur = xmlDocGetRootElement(doc);

        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* The root node is "lfm" and we check its status attribute */
        if (xmlStrcmp (cur->name, (const xmlChar *) "lfm") &&
            (attr = xmlGetProp (cur, (const xmlChar *) "status")) != NULL &&
            xmlStrcmp (attr, (const xmlChar *) "ok")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        /* The real representation is the first children of the "lfm" node */
        cur = cur->xmlChildrenNode->next;

        /* We check that the root node name is "album" */
        if (xmlStrcmp (cur->name, (const xmlChar *) "album")) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
                if (!xmlStrcmp (cur->name, (const xmlChar *) "image"))
                        break;
        }

        if (!cur) {
                xmlFreeDoc (doc);
                return NULL;
        }

        for (; cur; cur = cur->next) {
                if ((attr = xmlGetProp (cur, (const xmlChar *) "size")) != NULL &&
                     !xmlStrcmp (attr, (const xmlChar *) cover_size)) {
                        /* A possible cover uri has been found, we add it to the list*/
                        key = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
                        /* lastfm doesn't give any link if there isn't a cover for this size */
                        if (xmlStrcmp (key, (const xmlChar *) "")) {
                                ario_cover_uris = g_slist_append (ario_cover_uris, key);
                                if (operation == GET_FIRST_COVER) {
                                        /* If we only want one cover, we now stop to parse the file */
                                        xmlFreeDoc (doc);
                                        return ario_cover_uris;
                                }
                        }
                }
        }

        xmlFreeDoc (doc);

        return ario_cover_uris;
}

static char *
ario_cover_lastfm_make_xml_uri (const char *artist,
                                const char *album)
{
        ARIO_LOG_FUNCTION_START;
        char *xml_uri;
        char *formated_artist;
        char *formated_album;

        if (!album || !artist)
                return NULL;

        /* If the artist in unknown, we don't search for a cover */
        if (!strcmp (artist, ARIO_SERVER_UNKNOWN))
                return NULL;

        if (!strcmp (album, ARIO_SERVER_UNKNOWN))
                return NULL;

        formated_artist = ario_util_format_keyword_for_lastfm (artist);
        formated_album = ario_util_format_keyword_for_lastfm (album);

        /* We make the xml uri with all the parameters */
        xml_uri = g_strdup_printf (LASTFM_URI, LASTFM_API_KEY,
                                   formated_artist, formated_album);

        g_free (formated_artist);
        g_free (formated_album);

        return xml_uri;
}

gboolean
ario_cover_lastfm_get_covers (ArioCoverProvider *cover_provider,
                              const char *artist,
                              const char *album,
                              const char *file,
                              GArray **file_size,
                              GSList **file_contents,
                              ArioCoverProviderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        char *xml_uri;
        int xml_size;
        char *xml_data;
        GSList *temp;
        int temp_size;
        char *temp_contents;
        gboolean ret;
        GSList *ario_cover_uris;

        /* We construct the uri to make a request on the lastfm WebServices */
        xml_uri = ario_cover_lastfm_make_xml_uri (artist,
                                                  album);

        if (!xml_uri)
                return FALSE;

        /* We load the xml file in xml_data */
        ario_util_download_file (xml_uri,
                                 NULL, 0, NULL,
                                 &xml_size,
                                 &xml_data);
        g_free (xml_uri);

        if (xml_size == 0) {
                return FALSE;
        }

        /* We parse the xml file to extract the cover uris */
        ario_cover_uris = ario_cover_lastfm_parse_xml_file (xml_data,
                                                            xml_size,
                                                            operation,
                                                            COVER_LARGE);

        g_free (xml_data);

        /* By default, we return an error */
        ret = FALSE;

        for (temp = ario_cover_uris; temp; temp = temp->next) {
                if (temp->data) {
                        /* For each cover uri, we load the image data in temp_contents */
                        ario_util_download_file (temp->data,
                                                 NULL, 0, NULL,
                                                 &temp_size,
                                                 &temp_contents);
                        /* We append the cover to file_contents */
                        g_array_append_val (*file_size, temp_size);
                        *file_contents = g_slist_append (*file_contents, temp_contents);
                        /* If at least one cover is found, we return OK */
                        ret = TRUE;
                }
        }

        g_slist_foreach (ario_cover_uris, (GFunc) g_free, NULL);
        g_slist_free (ario_cover_uris);

        return ret;
}

