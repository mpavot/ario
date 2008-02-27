/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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

#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <libxml/parser.h>

#include "lib/ario-conf.h"
#include "ario-debug.h"
#include "ario-util.h"

static xmlDocPtr doc;
static char *xml_filename;
static GSList *notifications;
static guint notification_counter = 0;

typedef struct
{
        guint notification_id;
        char *key;
        ArioNotifyFunc notification_callback;
        gpointer callback_data;
} ArioConfNotifyData;

#define XML_ROOT_NAME (const unsigned char *)"ario-options"
#define XML_VERSION (const unsigned char *)"1.0"

static void
ario_conf_free_notify_data (ArioConfNotifyData *data)
{
        ARIO_LOG_FUNCTION_START
        if (data) {
                g_free (data->key);
                g_free (data);
        }
}

static char *
ario_conf_get (const char *key)
{
        ARIO_LOG_FUNCTION_START
        xmlNodePtr cur;
        xmlChar *xml_key;
        xmlChar *xml_value;
        gchar *ret = NULL;

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL)
                return NULL;

        for (cur = cur->children; cur; cur = cur->next) {
                /* For each "option" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *) "option")) {

                        xml_key = xmlGetProp (cur, (const unsigned char *) "key");
                        if (!xmlStrcmp (xml_key, (const xmlChar *) key)) {
                                xml_value = xmlNodeGetContent (cur);
                                ret = g_strdup ((char *) xml_value);
                                xmlFree(xml_value);
                                xmlFree(xml_key);
                                break;
                        }
                        xmlFree(xml_key);
                }
        }

        return ret;
}

static void
ario_conf_set (const char *key,
               const char *value)
{
        ARIO_LOG_FUNCTION_START
        xmlNodePtr cur;
        xmlChar *xml_key;
        xmlChar *xml_value;
        GSList *tmp;
        ArioConfNotifyData *data;

        cur = xmlDocGetRootElement(doc);
        if (cur == NULL)
                return;

        for (cur = cur->children; cur; cur = cur->next) {
                /* For each "option" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *) "option")) {

                        xml_key = xmlGetProp (cur, (const unsigned char *) "key");
                        if (!xmlStrcmp (xml_key, (const xmlChar *) key)) {
                                xml_value = xmlEncodeEntitiesReentrant (doc, (const xmlChar *) value);
                                xmlNodeSetContent (cur, xml_value);
                                xmlFree (xml_value);
                                xmlFree (xml_key);
                                break;
                        }
                        xmlFree (xml_key);
                }
        }

        if (!cur) {
                /* We add a new "option" entry */
                cur = xmlNewChild (xmlDocGetRootElement(doc), NULL, (const xmlChar *) "option", NULL);
                xmlSetProp (cur, (const xmlChar *) "key", (const xmlChar *) key);
                xml_value = xmlEncodeEntitiesReentrant (doc, (const xmlChar *) value);
                xmlNodeAddContent (cur, (const xmlChar *) xml_value);
                xmlFree (xml_value);
        }

        /* Notifications */
        for (tmp = notifications; tmp; tmp = g_slist_next (tmp)) {
                data = tmp->data;
                if (!strcmp (data->key, key)) {
                        data->notification_callback (NULL,
                                                     data->notification_id,
                                                     NULL,
                                                     data->callback_data);
                }
        }
}

void
ario_conf_set_boolean (const char *key,
                       gboolean boolean_value)
{
        if (boolean_value)
                ario_conf_set (key, "1");
        else
                ario_conf_set (key, "0");
}

gboolean
ario_conf_get_boolean (const char *key,
                       const gboolean default_value)
{
        gchar *value = ario_conf_get (key);
        gboolean ret;

        if (!value)
                return default_value;

        ret = !strcmp (value, "1");
        g_free (value);

        return ret;
}

void
ario_conf_set_integer (const char *key,
                            int int_value)
{
        gchar *char_value = g_strdup_printf ("%d", int_value);

        ario_conf_set (key, char_value);

        g_free (char_value);
}

int
ario_conf_get_integer (const char *key,
                       const int default_value)
{
        gchar *value = ario_conf_get (key);
        int ret;

        if (!value)
                return default_value;

        ret = atoi (value);
        g_free (value);

        return ret;
}

void
ario_conf_set_float (const char *key,
                            gfloat float_value)
{
        gchar *char_value = g_strdup_printf ("%f", float_value);

        ario_conf_set (key, char_value);

        g_free (char_value);
}

gfloat
ario_conf_get_float (const char *key,
                     const gfloat default_value)
{
        gchar *value = ario_conf_get (key);
        gfloat ret;

        if (!value)
                return default_value;

        ret = atof (value);
        g_free (value);

        return ret;
}

void
ario_conf_set_string (const char *key,
                      const char *string_value)
{
        ario_conf_set (key, string_value);
}

char *
ario_conf_get_string (const char *key,
                      const char *default_value)
{
        gchar *value = ario_conf_get (key);

        if (!value)
                return g_strdup (default_value);

        return value;
}

void
ario_conf_set_string_slist (const char *key,
                                const GSList *slist)
{
        GString* value = NULL;
        const GSList *tmp;
        gboolean first = TRUE;

        value = g_string_new("");

        for (tmp = slist; tmp; tmp = g_slist_next (tmp)) {
                if (!first) {
                        g_string_append (value, ",");
                }
                g_string_append (value, tmp->data);
                first = FALSE;
        }

        ario_conf_set (key, value->str);

        g_string_free (value, TRUE);
}

GSList *
ario_conf_get_string_slist (const char *key)
{
        gchar *value = ario_conf_get (key);
        gchar **values;
        GSList *ret = NULL;
        int i;

        if (!value)
                return NULL;

        values = g_strsplit ((const gchar *) value, ",", 0);

        for (i=0; values[i]!=NULL && g_utf8_collate (values[i], ""); ++i)
                ret = g_slist_append (ret, g_strdup (values[i]));

        g_strfreev (values);

        return ret;
}

void
ario_conf_init (void)
{
        ARIO_LOG_FUNCTION_START
        xmlNodePtr cur;

        xml_filename = g_build_filename (ario_util_config_dir (), "options.xml", NULL);

        /* This option is necessary to save a well formated xml file */
        xmlKeepBlanksDefault (0);

        if (ario_util_uri_exists (xml_filename)) {
                doc = xmlParseFile (xml_filename);
        } else {
                doc = xmlNewDoc (XML_VERSION);
                cur = xmlNewNode (NULL, (const xmlChar *) XML_ROOT_NAME);
                xmlDocSetRootElement (doc, cur);
        }
}

void
ario_conf_shutdown (void)
{
        ARIO_LOG_FUNCTION_START
        /* We save the xml file */
        xmlSaveFormatFile (xml_filename, doc, TRUE);

        g_free (xml_filename);
        xmlFreeDoc (doc);

        g_slist_foreach (notifications, (GFunc) ario_conf_free_notify_data, NULL);
}

guint
ario_conf_notification_add (const char *key,
                            ArioNotifyFunc notification_callback,
                            gpointer callback_data)
{
        ArioConfNotifyData *data = (ArioConfNotifyData *) g_malloc0 (sizeof (ArioConfNotifyData));
        ++notification_counter;

        data->notification_id = notification_counter;
        data->key = g_strdup (key);
        data->notification_callback = notification_callback;
        data->callback_data = callback_data;

        notifications = g_slist_append (notifications, data);

        return notification_counter;
}

void
ario_conf_notification_remove (guint notification_id)
{
        GSList *tmp;
        ArioConfNotifyData *data;

        for (tmp = notifications; tmp; tmp = g_slist_next (tmp)) {
                data = tmp->data;
                if (data->notification_id == notification_id) {
                        notifications = g_slist_remove (notifications, data);
                        ario_conf_free_notify_data (data);
                }
        }
}

