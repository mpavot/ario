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
char*                   ario_util_format_time                (const int time) G_GNUC_CONST G_GNUC_MALLOC;

/**
 * Format a track time to the form 1:23:45 in a buffer
 *
 * @param time The time to format, in seconds
 * @param buf The buffer to fill
 * @param buf_len The len of the buffer
 */
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
char*                   ario_util_format_total_time          (const int time) G_GNUC_CONST G_GNUC_MALLOC;

/**
 * Format a track number to be displayed
 *
 * @param track The track number
 * @param buf The buffer to fill
 * @param buf_len The len of the buffer
 */
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
gchar*                  ario_util_format_title               (ArioServerSong *server_song);

/**
 * Add an icon to the default icon factory
 *
 * @param stock_id The id of the icon to add
 * @param filename The icon filename
 */
void                    ario_util_add_stock_icons            (const char *stock_id,
                                                              const char *filename);
/**
 * Initialise default icon factory with a few icons
 */
void                    ario_util_init_stock_icons           (void);

/**
 * Check if an icon is already present
 *
 * @param stock_id The id of the icon to check
 *
 * @return True if the icon is present, FALSE otherwise
 */
gboolean                ario_util_has_stock_icons            (const char *stock_id);

/**
 * Get the path of Ario configuration
 *
 * @return The path of the directory. It should not be freed.
 */
const char*             ario_util_config_dir                 (void);

/**
 * Check whether a file exists or not
 *
 * @param uri The uri of the file
 *
 * @return True if the file exists, FALSE otherwise
 */
gboolean                ario_util_uri_exists                 (const char *uri);

/**
 * Delete a file on the disk
 *
 * @param uri The uri of the file to delete
 */
void                    ario_util_unlink_uri                 (const char *uri);

/**
 * Create a new directory
 *
 * @param uri The uri of the directory to create
 */
void                    ario_util_mkdir                      (const char *uri);

/**
 * Copy a file on disk
 *
 * @param src_uri The source file to copy
 * @param dest_uri The destination place to copy the file
 */
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
void                    ario_util_string_replace             (char **string,
                                                              const char *old,
                                                              const char *new);
/**
 * Load a URL in user's web browser
 *
 * @param uri The URL to load
 */
void                    ario_util_load_uri                   (const char *uri);

/**
 * Format a keyword to be used for example in an online search
 *
 * @param keyword The keyword to format
 *
 * @return A newly allocated formated keyword
 */
char *                  ario_util_format_keyword             (const char *keyword) G_GNUC_MALLOC;

/**
 * Compute the MD5 hash of a string
 *
 * @param string The string to use for MD5 computation
 *
 * @return A newly allocated hash
 */
gchar *                 ario_util_md5                        (const char *string) G_GNUC_MALLOC;

/**
 * Generate an icon to use for Drag & Drop from a list of albums
 *
 * @param albums The list of albums
 *
 * @return A newly allocated pixbuf
 */
GdkPixbuf *             ario_util_get_dnd_pixbuf_from_albums (const GSList *albums) G_GNUC_MALLOC;

/**
 * Generate an icon to use for Drag & Drop from a list of criteria
 *
 * @param criterias The list of ArioServerCriteria
 *
 * @return A newly allocated pixbuf
 */
GdkPixbuf *             ario_util_get_dnd_pixbuf             (const GSList *criterias) G_GNUC_MALLOC;

/**
 * Convert a string from iso8859 to locale
 *
 * @param string The string to convert
 *
 * @return A newly allocated string
 */
gchar *                 ario_util_convert_from_iso8859       (const char *string) G_GNUC_MALLOC;

/**
 * Remove bad caracters from a filename
 *
 * @param filename The filename to convert
 */
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
GSList *                ario_util_gslist_randomize           (GSList **list,
                                                              const int max);

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
