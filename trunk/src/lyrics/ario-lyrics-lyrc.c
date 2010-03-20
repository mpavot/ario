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

#include "lyrics/ario-lyrics-lyrc.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"
#include <curl/curl.h>

#define LYRC_URI "http://lyrc.com.ar/en/tema1en.php?artist=%s&songname=%s"
#define LYRC_ARTIST_URI "http://lyrc.com.ar/en/tema1en.php?artist=%s"
#define LYRC_SONG_URI "http://lyrc.com.ar/en/tema1en.php?songname=%s"

ArioLyrics* ario_lyrics_lyrc_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                         const char *artist,
                                         const char *song,
                                         const char *file);
static void ario_lyrics_lyrc_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                                    const gchar *artist,
                                                    const gchar *song,
                                                    GSList **candidates);
static ArioLyrics * ario_lyrics_lyrc_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                                const ArioLyricsCandidate *candidate);

G_DEFINE_TYPE (ArioLyricsLyrc, ario_lyrics_lyrc, ARIO_TYPE_LYRICS_PROVIDER)

static gchar *
ario_lyrics_lyrc_get_id (ArioLyricsProvider *lyrics_provider)
{
        return "lyrc";
}

static gchar *
ario_lyrics_lyrc_get_name (ArioLyricsProvider *lyrics_provider)
{
        return "Lyrc";
}

static void
ario_lyrics_lyrc_class_init (ArioLyricsLyrcClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsProviderClass *lyrics_provider_class = ARIO_LYRICS_PROVIDER_CLASS (klass);

        lyrics_provider_class->get_id = ario_lyrics_lyrc_get_id;
        lyrics_provider_class->get_name = ario_lyrics_lyrc_get_name;
        lyrics_provider_class->get_lyrics = ario_lyrics_lyrc_get_lyrics;
        lyrics_provider_class->get_lyrics_candidates = ario_lyrics_lyrc_get_lyrics_candidates;
        lyrics_provider_class->get_lyrics_from_candidate = ario_lyrics_lyrc_get_lyrics_from_candidate;
}

static void
ario_lyrics_lyrc_init (ArioLyricsLyrc *lyrics_lyrc)
{
        ARIO_LOG_FUNCTION_START;
}

ArioLyricsProvider*
ario_lyrics_lyrc_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsLyrc *lyrc;

        lyrc = g_object_new (TYPE_ARIO_LYRICS_LYRC,
                             NULL);

        return ARIO_LYRICS_PROVIDER (lyrc);
}

static ArioLyrics *
ario_lyrics_lyrc_parse_file (gchar *data,
                             int size)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyrics *lyrics = NULL;
        gchar *begin, *end;

        if (g_strrstr (data, "<br><b> Nothing found :</b>"))
                return NULL;

        begin = strstr (data, "</td></tr></table>");
        if (!begin)
                return NULL;
        begin += strlen("</td></tr></table>");

        end = strstr (begin, "<br><br><a href=\"#\" onClick=");
        if (!end)
                return NULL;

        lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
        lyrics->lyrics = g_strndup (begin, end - begin);
        ario_util_string_replace (&lyrics->lyrics, "<br />", "");

        return lyrics;
}

ArioLyrics *
ario_lyrics_lyrc_get_lyrics (ArioLyricsProvider *lyrics_provider,
                             const char *artist,
                             const char *title,
                             const char *file)
{
        ARIO_LOG_FUNCTION_START;
        char *uri;
        int size;
        char *data;
        gchar *conv_artist;
        gchar *conv_title;
        ArioLyrics *lyrics = NULL;

        conv_artist = ario_util_format_for_http (artist);
        conv_title = ario_util_format_for_http (title);

        if (artist && title)
                uri = g_strdup_printf(LYRC_URI, conv_artist, conv_title);
        else if (artist)
                uri = g_strdup_printf(LYRC_ARTIST_URI, conv_artist);
        else if (title)
                uri = g_strdup_printf(LYRC_SONG_URI, conv_title);
        else
                return NULL;

        g_free (conv_artist);
        g_free (conv_title);

        /* We load file */
        ario_util_download_file (uri,
                                 NULL, 0, NULL,
                                 &size,
                                 &data);
        g_free (uri);

        if (size == 0)
                return NULL;

        lyrics = ario_lyrics_lyrc_parse_file (data, size);
        if (lyrics) {
                lyrics->title = g_strdup (title);
                lyrics->artist = g_strdup (artist);
        }

        g_free (data);

        return lyrics;
}

static void
ario_lyrics_lyrc_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                        const gchar *artist,
                                        const gchar *title,
                                        GSList **candidates)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyrics *lyrics;
        ArioLyricsCandidate *candidate;
        char *uri;
        int size;
        char *data;
        gchar *conv_artist;
        gchar *conv_title;
        gchar *suggestions;
        gchar * hash, *end_hash;
        gchar * begin_artist, *end_artist;
        gchar * begin_title, *end_title;

        conv_artist = ario_util_format_for_http (artist);
        conv_title = ario_util_format_for_http (title);

        if (artist && title)
                uri = g_strdup_printf(LYRC_URI, conv_artist, conv_title);
        else if (artist)
                uri = g_strdup_printf(LYRC_ARTIST_URI, conv_artist);
        else if (title)
                uri = g_strdup_printf(LYRC_SONG_URI, conv_title);
        else
                return;

        g_free (conv_artist);
        g_free (conv_title);

        /* We load file */
        ario_util_download_file (uri,
                                 NULL, 0, NULL,
                                 &size,
                                 &data);
        g_free (uri);

        if (size == 0)
                return;

        suggestions = g_strrstr (data, "Suggestions : <br><a h");
        if (suggestions) {
                hash = suggestions;
                while ((hash = strstr (hash + 1, "tema1en.php?hash="))) {
                        end_hash = strstr (hash, "\"><font color='white'>");
                        if (!end_hash)
                                continue;
                        begin_artist = end_hash + strlen("\"><font color='white'>");
                        end_artist = strstr (begin_artist, " - ");
                        if (!end_artist)
                                continue;
                        begin_title = end_artist + strlen (" - ");
                        end_title = strstr (begin_title, "</font></a><br>");
                        if (!end_title)
                                continue;

                        candidate = (ArioLyricsCandidate *) g_malloc0 (sizeof (ArioLyricsCandidate));
                        candidate->artist =  g_strndup (begin_artist, end_artist - begin_artist);
                        candidate->title = g_strndup (begin_title, end_title - begin_title);
                        candidate->data = g_strndup (hash, end_hash - hash);
                        candidate->lyrics_provider = lyrics_provider;

                        *candidates = g_slist_append (*candidates, candidate);
                }
        } else {
                lyrics = ario_lyrics_lyrc_parse_file (data, size);
                if (lyrics) {
                        candidate = (ArioLyricsCandidate *) g_malloc0 (sizeof (ArioLyricsCandidate));
                        candidate->artist = g_strdup (artist);
                        candidate->title = g_strdup (title);
                        candidate->data = g_strdup (lyrics->lyrics);
                        candidate->lyrics_provider = lyrics_provider;

                        *candidates = g_slist_append (*candidates, candidate);

                        ario_lyrics_free (lyrics);
                }
        }

        g_free (data);
}

static ArioLyrics *
ario_lyrics_lyrc_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                            const ArioLyricsCandidate *candidate)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyrics *lyrics = NULL;
        gchar * uri;
        int size;
        char *data;

        if (!strncmp (candidate->data, "tema1en.php", strlen("tema1en.php")))
        {
                uri = g_strdup_printf ("http://lyrc.com.ar/%s", candidate->data); 

                /* We load file */
                ario_util_download_file (uri,
                                         NULL, 0, NULL,
                                         &size,
                                         &data);
                g_free (uri);

                if (size == 0)
                        return NULL;

                lyrics = ario_lyrics_lyrc_parse_file (data, size);
                if (lyrics) {
                        lyrics->artist = g_strdup (candidate->artist);
                        lyrics->title = g_strdup (candidate->title);
                }

                g_free (data);

        }
        else
        {
                lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
                lyrics->artist = g_strdup (candidate->artist);
                lyrics->title = g_strdup (candidate->title);
                lyrics->lyrics = g_strdup (candidate->data);
        }

        if (lyrics) {
                ario_lyrics_prepend_infos (lyrics);
                ario_lyrics_save_lyrics (candidate->artist,
                                         candidate->title,
                                         lyrics->lyrics);
        }

        return lyrics;
}
