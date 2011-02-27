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

#include "lyrics/ario-lyrics-letras.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "lib/ario-conf.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"
#include <curl/curl.h>

#define LETRAS_URI "http://letras.terra.com.br/winamp.php?t=%s%%20-%%20%s"
#define LETRAS_ARTIST_URI "http://letras.terra.com.br/winamp.php?t=%s"
#define LETRAS_SONG_URI "http://letras.terra.com.br/winamp.php?t=%s"

ArioLyrics* ario_lyrics_letras_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                           const char *artist,
                                           const char *song,
                                           const char *file);
static void ario_lyrics_letras_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                                      const gchar *artist,
                                                      const gchar *song,
                                                      GSList **candidates);
static ArioLyrics * ario_lyrics_letras_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                                  const ArioLyricsCandidate *candidate);

G_DEFINE_TYPE (ArioLyricsLetras, ario_lyrics_letras, ARIO_TYPE_LYRICS_PROVIDER)

static gchar *
ario_lyrics_letras_get_id (ArioLyricsProvider *lyrics_provider)
{
        return "letras";
}

static gchar *
ario_lyrics_letras_get_name (ArioLyricsProvider *lyrics_provider)
{
        return "Letras";
}

static void
ario_lyrics_letras_class_init (ArioLyricsLetrasClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsProviderClass *lyrics_provider_class = ARIO_LYRICS_PROVIDER_CLASS (klass);

        lyrics_provider_class->get_id = ario_lyrics_letras_get_id;
        lyrics_provider_class->get_name = ario_lyrics_letras_get_name;
        lyrics_provider_class->get_lyrics = ario_lyrics_letras_get_lyrics;
        lyrics_provider_class->get_lyrics_candidates = ario_lyrics_letras_get_lyrics_candidates;
        lyrics_provider_class->get_lyrics_from_candidate = ario_lyrics_letras_get_lyrics_from_candidate;
}

static void
ario_lyrics_letras_init (ArioLyricsLetras *lyrics_letras)
{
        ARIO_LOG_FUNCTION_START;
}

ArioLyricsProvider*
ario_lyrics_letras_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyricsLetras *letras;

        letras = g_object_new (TYPE_ARIO_LYRICS_LETRAS,
                               NULL);

        return ARIO_LYRICS_PROVIDER (letras);
}

static ArioLyrics *
ario_lyrics_letras_parse_file (gchar *data,
                               int size)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyrics *lyrics = NULL;
        gchar *begin, *end, *tmp;
        char *buf;
        guint i = 0, offset = 0;;

        if (g_strrstr (data, "cute;sica n&atilde;o encontrada'"))
                return NULL;

        begin = strstr (data, "<div id=\"letra\">");
        if (!begin)
                return NULL;

        begin = strstr (begin, "<p>");
        begin += strlen("<p>");

        end = strstr (begin, "</p>");
        if (!end)
                return NULL;

        lyrics = (ArioLyrics *) g_malloc0 (sizeof (ArioLyrics));
        lyrics->lyrics = g_strndup (begin, end - begin);
        ario_util_string_replace (&lyrics->lyrics, "<br/>", "");

        tmp = ario_util_convert_from_iso8859 (lyrics->lyrics);
        g_free (lyrics->lyrics);
        lyrics->lyrics = tmp;

        /* Convert encoded characters (like &#039; -> ') */
        buf = (char *) g_malloc0 (strlen(lyrics->lyrics));
        for (i = 0; i + offset < strlen(lyrics->lyrics); ++i)
        {
                if (!strncmp (lyrics->lyrics + i + offset,  "&#", 2))
                {
                        int char_nb = atoi (lyrics->lyrics + i + offset + 2);
                        if (char_nb > 0)
                        {
                                buf[i] = char_nb;
                                offset += 5;
                        }
                        else
                        {
                                buf[i] = lyrics->lyrics[i + offset];
                        }
                }
                else
                {
                     buf[i] = lyrics->lyrics[i + offset];
                }
        }

        g_free (lyrics->lyrics);
        lyrics->lyrics = buf;

        return lyrics;
}

ArioLyrics *
ario_lyrics_letras_get_lyrics (ArioLyricsProvider *lyrics_provider,
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
                uri = g_strdup_printf(LETRAS_URI, conv_artist, conv_title);
        else if (artist)
                uri = g_strdup_printf(LETRAS_ARTIST_URI, conv_artist);
        else if (title)
                uri = g_strdup_printf(LETRAS_SONG_URI, conv_title);
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

        lyrics = ario_lyrics_letras_parse_file (data, size);
        if (lyrics) {
                lyrics->title = g_strdup (title);
                lyrics->artist = g_strdup (artist);
        }

        g_free (data);

        return lyrics;
}

static void
ario_lyrics_letras_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                          const gchar *artist,
                                          const gchar *title,
                                          GSList **candidates)
{
        ARIO_LOG_FUNCTION_START;
        ArioLyrics *lyrics;
        ArioLyricsCandidate *candidate;

        lyrics = ario_lyrics_letras_get_lyrics (lyrics_provider,
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
ario_lyrics_letras_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                              const ArioLyricsCandidate *candidate)
{
        ARIO_LOG_FUNCTION_START;
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
