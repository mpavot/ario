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
#include "ario-lyrics.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

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

ArioLyrics *
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
ario_lyrics_candidate_free (ArioLyricsCandidate *candidate)
{
        if (candidate) {
                g_free (candidate->artist);
                g_free (candidate->title);
                g_free (candidate->data);
                g_free (candidate);
        }
}

ArioLyricsCandidate *
ario_lyrics_candidate_copy (const ArioLyricsCandidate *candidate)
{
        ArioLyricsCandidate *ret;

        ret = (ArioLyricsCandidate *) g_malloc (sizeof (ArioLyricsCandidate));

        ret->artist = g_strdup (candidate->artist);
        ret->title = g_strdup (candidate->title);
        ret->data = g_strdup (candidate->data);
        ret->lyrics_provider = candidate->lyrics_provider;

        return ret;
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

void
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
