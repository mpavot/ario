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
#include <glib.h>
#include <gdk/gdk.h>

/* Number of covers used to generate the drag & drop image */
#define MAX_COVERS_IN_DRAG 3

/* Maximum length of the string representing a int */
#define INTLEN (sizeof(int) * CHAR_BIT + 1) / 3 + 1

/* Maximum size of a time like 1:23:45 */
#define ARIO_MAX_TIME_SIZE 3*INTLEN+2

/* Maximum size of a track */
#define ARIO_MAX_TRACK_SIZE INTLEN

struct curl_slist;

/**
 * Format a track time to the form 1:23:45.
 *
 * @param time The time to format, in seconds
 *
 * @return A newly allocated string with the formated time
 */
G_MODULE_EXPORT
char*                   ario_util_format_time                (const int time) G_GNUC_CONST;

/**
 * Format a track time to the form 1:23:45 in a buffer
 *
 * @param time The time to format, in seconds
 * @param buf The buffer to fill
 * @param buf_len The len of the buffer
 */
G_MODULE_EXPORT
void                    ario_util_format_time_buf            (const int time,
                                                              char *buf,
                                                              int buf_len);
/**
 * Format a time to the form : x days, y hours, z minutes, n secondes
 *
 * @param time The time to format, in seconds
 *
 * @return A newly allocated string with the formated time
 */
G_MODULE_EXPORT
char*                   ario_util_format_total_time          (const int time) G_GNUC_CONST;

/**
 * Format a track number to be displayed
 *
 * @param track The track number
 * @param buf The buffer to fill
 * @param buf_len The len of the buffer
 */
G_MODULE_EXPORT
void                    ario_util_format_track_buf           (const gchar *track,
                                                              char *buf,
                                                              int buf_len);
/**
 * Try to get the best title to display from an ArioServerSong
 *
 * @param server_song The server song
 *
 * @return A pointer to the title (should not be freed)
 */
G_MODULE_EXPORT
gchar*                  ario_util_format_title               (ArioServerSong *server_song);

/**
 * Initialise default icon factory with a few icons
 */
G_MODULE_EXPORT
void                    ario_util_init_icons                 (void);

/**
 * Get the path of Ario configuration
 *
 * @return The path of the directory. It should not be freed.
 */
G_MODULE_EXPORT
const char*             ario_util_config_dir                 (void);

/**
 * Check whether a file exists or not
 *
 * @param uri The uri of the file
 *
 * @return True if the file exists, FALSE otherwise
 */
G_MODULE_EXPORT
gboolean                ario_util_uri_exists                 (const char *uri);

/**
 * Delete a file on the disk
 *
 * @param uri The uri of the file to delete
 */
G_MODULE_EXPORT
void                    ario_util_unlink_uri                 (const char *uri);

/**
 * Create a new directory
 *
 * @param uri The uri of the directory to create
 */
G_MODULE_EXPORT
void                    ario_util_mkdir                      (const char *uri);

/**
 * Copy a file on disk
 *
 * @param src_uri The source file to copy
 * @param dest_uri The destination place to copy the file
 */
G_MODULE_EXPORT
void                    ario_util_copy_file                  (const char *src_uri,
                                                              const char *dest_uri);
/**
 * Download a file on internet
 *
 * @param uri The uri of the file to download
 * @param post_data Post data to use for POST requests or NULL
 * @param post_size The size of post data (no used if post_data is NULL)
 * @param headers Http headers to use or NULL
 * @param size A pointer to a int that will contain the size of the downloaded data
 * @param data Newly allocated data containing the downloaded file
 */
G_MODULE_EXPORT
void                    ario_util_download_file              (const char *uri,
                                                              const char *post_data,
                                                              const int post_size,
                                                              const struct curl_slist *headers,
                                                              int* size,
                                                              char** data);
/**
 * Replace string 'old' by string 'new' in 'string'
 *
 * @param string The string where the replacement will be done
 * @param old The string to replace
 * @param new The string that will replace 'old'
 */
G_MODULE_EXPORT
void                    ario_util_string_replace             (char **string,
                                                              const char *old,
                                                              const char *new);
/**
 * Load a URL in user's web browser
 *
 * @param uri The URL to load
 */
G_MODULE_EXPORT
void                    ario_util_load_uri                   (const char *uri);

/**
 * Format a keyword to be used for example in an online search
 *
 * @param keyword The keyword to format
 *
 * @return A newly allocated formated keyword
 */
G_MODULE_EXPORT
char *                  ario_util_format_keyword             (const char *keyword);

/**
 * Format a keyword to be used for a search on last.fm
 *
 * @param keyword The keyword to format
 *
 * @return A newly allocated formated keyword
 */
G_MODULE_EXPORT
char *                  ario_util_format_keyword_for_lastfm  (const char *keyword);

/**
 * Generate an icon to use for Drag & Drop from a list of albums
 *
 * @param albums The list of albums
 *
 * @return A newly allocated pixbuf
 */
G_MODULE_EXPORT
GdkPixbuf *             ario_util_get_dnd_pixbuf_from_albums (const GSList *albums);

/**
 * Generate an icon to use for Drag & Drop from a list of criteria
 *
 * @param criterias The list of ArioServerCriteria
 *
 * @return A newly allocated pixbuf
 */
G_MODULE_EXPORT
GdkPixbuf *             ario_util_get_dnd_pixbuf             (const GSList *criterias);

/**
 * Convert a string from iso8859 to locale
 *
 * @param string The string to convert
 *
 * @return A newly allocated string
 */
G_MODULE_EXPORT
gchar *                 ario_util_convert_from_iso8859       (const char *string);

/**
 * Remove bad caracters from a filename
 *
 * @param filename The filename to convert
 */
G_MODULE_EXPORT
void                    ario_util_sanitize_filename          (char *filename);

/**
 * Get the content of a file from disk
 *
 * @param filename The filename
 * @param contents A pointer to a newly allocated file content
 * @param length A pointer to the size of file content
 * @param error return location for a GError, or NULL
 *
 * @return TRUE on success, FALSE if an error occurred
 */
G_MODULE_EXPORT
gboolean                ario_file_get_contents               (const gchar *filename,
                                                              gchar **contents,
                                                              gsize *length,
                                                              GError **error);
/**
 * Writes all of contents to a file named filename
 *
 * @param filename name of a file to write contents
 * @param contents string to write to the file
 * @param length length of contents, or -1 if contents is a nul-terminated string
 * @param error return location for a GError, or NULL
 *
 * @return TRUE on success, FALSE if an error occurred
 */
G_MODULE_EXPORT
gboolean                ario_file_set_contents               (const gchar *filename,
                                                              const gchar *contents,
                                                              gsize length,
                                                              GError **error);
/**
 * Returns TRUE if any of the tests in the bitfield test are TRUE.
 *
 * @param filename a filename to test
 * @param test bitfield of GFileTest flags
 *
 * @return whether a test was TRUE
 */
G_MODULE_EXPORT
gboolean                ario_file_test                       (const gchar *filename,
                                                              GFileTest test);

/**
 * Case insensitive strstr (locate a substring in a string)
 *
 * @param haystack The string to do the location
 * @param needle String to locate
 *
 * @return
 */
G_MODULE_EXPORT
const char *            ario_util_stristr                    (const char *haystack,
                                                              const char *needle);

/**
 * Randomize a GSList
 *
 * @param The GSList to randomize. This list should not be used anymore after
 *        this function has been called. Elements of the list should be freed if
 *        needed and the list should be freed in addition to the one returned by
 *        ario_util_gslist_randomize.
 * @param max The number of items to randomize in the list
 *
 * @return A pointer to the GSList to use instead of 'list'
 */
G_MODULE_EXPORT
GSList *                ario_util_gslist_randomize           (GSList **list,
                                                              const int max);

/**
 * Format a string so that it can be used in an HTTP requests
 *
 * @param text The string to format
 *
 * @return A newly allocated string formated for HTTP requests
 */
G_MODULE_EXPORT
gchar *                 ario_util_format_for_http            (const gchar *text);

/**
 * Computes the absolute value of a int
 *
 * @param a An integer
 *
 * @return a if a is positive, -a if a is negative
 */
static inline gint
ario_util_abs (const gint a)
{
        return (a > 0 ? a : -a);
}

/**
 * Returns the min of two values
 *
 * @param a First value
 * @param b Second value
 *
 * @return Minimum of a and b
 */
static inline gint
ario_util_min (const gint a,
               const gint b)
{
        return (a > b ? b : a);
}

/**
 * Returns the max of two values
 *
 * @param a First value
 * @param b Second value
 *
 * @return Maximum of a and b
 */
static inline gint
ario_util_max (const gint a,
               const gint b)
{
        return (a > b ? a : b);
}

/**
 * Compare two strings. These two strings can be NULL
 *
 * @param a First string or NULL
 * @param b Second string or NULL
 *
 * @return < 0 if a compares before b, 0 if they compare equal, > 0 if a compares after b
 */
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
