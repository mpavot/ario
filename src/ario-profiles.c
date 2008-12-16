/*
 *  Copyright (C) 2007 Marc Pavot <marc.pavot@gmail.com>
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

#include "ario-profiles.h"
#include <string.h>
#include <config.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <glib/gi18n.h>
#include "ario-util.h"
#include "ario-debug.h"

#define XML_ROOT_NAME (const unsigned char *)"ario-profiles"
#define XML_VERSION (const unsigned char *)"1.0"

static void ario_profiles_create_xml_file (char *xml_filename);
static char* ario_profiles_get_xml_filename (void);


static void
ario_profiles_create_xml_file (char *xml_filename)
{
        gchar *profiles_dir;

        /* if the file exists, we do nothing */
        if (ario_util_uri_exists (xml_filename))
                return;

        profiles_dir = g_build_filename (ario_util_config_dir (), "profiles", NULL);

        /* If the profiles directory doesn't exist, we create it */
        if (!ario_util_uri_exists (profiles_dir))
                ario_util_mkdir (profiles_dir);

        ario_util_copy_file (DATA_PATH "profiles.xml.default",
                             xml_filename);
}

static char*
ario_profiles_get_xml_filename (void)
{
        char *xml_filename;

        xml_filename = g_build_filename (ario_util_config_dir (), "profiles" , "profiles.xml", NULL);

        return xml_filename;
}

void
ario_profiles_free (ArioProfile* profile)
{
        ARIO_LOG_FUNCTION_START
        if (profile) {
                g_free (profile->name);
                g_free (profile->host);
                g_free (profile->password);
                g_free (profile->musicdir);
                g_free (profile);
        }
}

GSList*
ario_profiles_get (void)
{
        ARIO_LOG_FUNCTION_START
        static GSList *profiles = NULL;
        ArioProfile *profile;
        xmlDocPtr doc;
        xmlNodePtr cur;
        char *xml_filename;
        xmlChar *xml_name;
        xmlChar *xml_host;
        xmlChar *xml_port;
        xmlChar *xml_password;
        xmlChar *xml_musicdir;
        xmlChar *xml_local;
        xmlChar *xml_current;
        xmlChar *xml_type;

        if (profiles)
                return profiles;

        xml_filename = ario_profiles_get_xml_filename();

        /* If profiles.xml file doesn't exist, we create it */
        ario_profiles_create_xml_file (xml_filename);

        /* This option is necessary to save a well formated xml file */
        xmlKeepBlanksDefault (0);

        doc = xmlParseFile (xml_filename);
        g_free (xml_filename);
        if (doc == NULL )
                return profiles;

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL) {
                xmlFreeDoc (doc);
                return profiles;
        }

        /* We check that the root node has the right name */
        if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NAME)) {
                xmlFreeDoc (doc);
                return profiles;
        }

        for (cur = cur->children; cur; cur = cur->next) {
                /* For each "profiles" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"profile")){
                        profile = (ArioProfile *) g_malloc0 (sizeof (ArioProfile));

                        xml_name = xmlNodeGetContent (cur);
                        profile->name = g_strdup ((char *) xml_name);
                        xmlFree(xml_name);

                        xml_host = xmlGetProp (cur, (const unsigned char *)"host");
                        profile->host = g_strdup ((char *) xml_host);
                        xmlFree(xml_host);

                        xml_port = xmlGetProp (cur, (const unsigned char *)"port");
                        profile->port = atoi ((char *) xml_port);
                        xmlFree(xml_port);

                        xml_password = xmlGetProp (cur, (const unsigned char *)"password");
                        if (xml_password) {
                                profile->password = g_strdup ((char *) xml_password);
                                xmlFree(xml_password);
                        }

                        xml_musicdir = xmlGetProp (cur, (const unsigned char *)"musicdir");
                        if (xml_musicdir) {
                                profile->musicdir = g_strdup ((char *) xml_musicdir);
                                xmlFree(xml_musicdir);
                        }

                        xml_local = xmlGetProp (cur, (const unsigned char *)"local");
                        if (xml_local) {
                                profile->local = TRUE;
                                xmlFree(xml_local);
                        } else {
                                profile->local = FALSE;
                        }

                        xml_current = xmlGetProp (cur, (const unsigned char *)"current");
                        if (xml_current) {
                                profile->current = TRUE;
                                xmlFree(xml_current);
                        } else {
                                profile->current = FALSE;
                        }

                        xml_type = xmlGetProp (cur, (const unsigned char *)"type");
                        if (xml_type) {
                                profile->type = atoi ((char *) xml_type);
                                if (profile->type != ArioServerMpd && profile->type != ArioServerXmms)
                                        profile->type = ArioServerMpd;
                                xmlFree(xml_type);
                        } else {
                                profile->type = ArioServerMpd;
                        }

                        profiles = g_slist_append (profiles, profile);
                }
        }
        xmlFreeDoc (doc);

        return profiles;
}

void
ario_profiles_save (GSList* profiles)
{
        ARIO_LOG_FUNCTION_START
        xmlDocPtr doc;
        xmlNodePtr cur, cur2;
        char *xml_filename;
        ArioProfile *profile;
        GSList *tmp;
        gchar *port_char;
        gchar *type_char;

        xml_filename = ario_profiles_get_xml_filename();

        /* If profiles.xml file doesn't exist, we create it */
        ario_profiles_create_xml_file (xml_filename);

        /* This option is necessary to save a well formated xml file */
        xmlKeepBlanksDefault (0);

        doc = xmlNewDoc (XML_VERSION);
        if (doc == NULL ) {
                g_free (xml_filename);
                return;
        }

        /* Create the root node */
        cur = xmlNewNode (NULL, (const xmlChar *) XML_ROOT_NAME);
        if (cur == NULL) {
                g_free (xml_filename);
                xmlFreeDoc (doc);
                return;
        }
        xmlDocSetRootElement (doc, cur);

        for (tmp = profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                port_char = g_strdup_printf ("%d",  profile->port);
                type_char = g_strdup_printf ("%d",  profile->type);

                /* We add a new "profiles" entry */
                cur2 = xmlNewChild (cur, NULL, (const xmlChar *)"profile", NULL);
                xmlNodeAddContent (cur2, (const xmlChar *) profile->name);
                xmlSetProp (cur2, (const xmlChar *)"host", (const xmlChar *) profile->host);
                xmlSetProp (cur2, (const xmlChar *)"port", (const xmlChar *) port_char);
                if (profile->password) {
                        xmlSetProp (cur2, (const xmlChar *)"password", (const xmlChar *) profile->password);
                }
                if (profile->musicdir) {
                        xmlSetProp (cur2, (const xmlChar *)"musicdir", (const xmlChar *) profile->musicdir);
                }
                if (profile->local) {
                        xmlSetProp (cur2, (const xmlChar *)"local", (const xmlChar *) "true");
                }
                if (profile->current) {
                        xmlSetProp (cur2, (const xmlChar *)"current", (const xmlChar *) "true");
                }
                xmlSetProp (cur2, (const xmlChar *)"type", (const xmlChar *) type_char);
                g_free (port_char);
                g_free (type_char);
        }

        /* We save the xml file */
        xmlSaveFormatFile (xml_filename, doc, TRUE);
        g_free (xml_filename);
        xmlFreeDoc (doc);
}

ArioProfile*
ario_profiles_get_current (GSList* profiles)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioProfile *profile;

        for (tmp = profiles; tmp; tmp = g_slist_next (tmp)) {
                profile = (ArioProfile *) tmp->data;
                if (profile->current)
                        return profile;
        }
        return NULL;
}

void
ario_profiles_set_current (GSList* profiles,
                           ArioProfile* profile)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioProfile *tmp_profile;

        if (g_slist_find (profiles, profile)) {
                for (tmp = profiles; tmp; tmp = g_slist_next (tmp)) {
                        tmp_profile = (ArioProfile *) tmp->data;
                        if (tmp_profile == profile) {
                                tmp_profile->current = TRUE;
                        } else {
                                tmp_profile->current = FALSE;
                        }
                }
        }
}


