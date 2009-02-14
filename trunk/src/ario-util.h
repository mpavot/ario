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

#include "servers/ario-server.h"
#include "glib/gslist.h"
#include "gdk/gdkpixbuf.h"

#define MAX_COVERS_IN_DRAG 3

struct curl_slist;

char*                   ario_util_format_time                (const int time) G_GNUC_CONST G_GNUC_MALLOC;

char*                   ario_util_format_total_time          (const int time) G_GNUC_CONST G_GNUC_MALLOC;

gchar*                  ario_util_format_track               (const gchar *track) G_GNUC_MALLOC;

gchar*                  ario_util_format_title               (const ArioServerSong *server_song) G_GNUC_MALLOC;

void                    ario_util_add_stock_icons            (const char *stock_id,
                                                              const char *filename);
void                    ario_util_init_stock_icons           (void);

gboolean                ario_util_has_stock_icons            (const char *stock_id);

const char*             ario_util_config_dir                 (void);

gboolean                ario_util_uri_exists                 (const char *uri);

void                    ario_util_unlink_uri                 (const char *uri);

void                    ario_util_mkdir                      (const char *uri);

void                    ario_util_copy_file                  (const char *src_uri,
                                                              const char *dest_uri);
void                    ario_util_download_file              (const char *uri,
                                                              const char *post_data,
                                                              const int post_size,
                                                              const struct curl_slist *headers,
                                                              int* size,
                                                              char** data);
void                    ario_util_string_replace             (char **string,
                                                              const char *old,
                                                              const char *new);
void                    ario_util_load_uri                   (const char *uri);

char *                  ario_util_format_keyword             (const char *keyword) G_GNUC_MALLOC;

gchar *                 ario_util_md5                        (const char *string) G_GNUC_MALLOC;

GdkPixbuf *             ario_util_get_dnd_pixbuf_from_albums (const GSList *albums) G_GNUC_MALLOC;

GdkPixbuf *             ario_util_get_dnd_pixbuf             (const GSList *criterias) G_GNUC_MALLOC;

gchar *                 ario_util_convert_from_iso8859       (const char *string) G_GNUC_MALLOC;

gboolean                ario_file_get_contents               (const gchar *filename,
                                                              gchar **contents,
                                                              gsize *length,
                                                              GError **error);
gboolean                ario_file_set_contents               (const gchar *filename,
                                                              const gchar *contents,
                                                              gsize length,
                                                              GError **error);
gboolean                ario_file_test                       (const gchar *filename,
                                                              GFileTest test);

/* Case insensitive strstr */
const char *            ario_util_stristr                    (const char *haystack,
                                                              const char *needle);

/* Inline methods */
static inline gint
ario_util_abs (const gint a)
{
        return (a > 0 ? a : -a);
}

static inline gint
ario_util_min (const gint a,
               const gint b)
{
        return (a > b ? b : a);
}

static inline gint
ario_util_max (const gint a,
               const gint b)
{
        return (a > b ? a : b);
}

static inline gint
ario_util_strcmp (const gchar *a,
                  const gchar* b)
{
        if (!a && !b)
                return 0;
        if (!a && b)
                return 1;
        if (a && !b)
                return -1;
        return g_utf8_collate (a, b);
}
