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

#include <config.h>
#include "lyrics/ario-lyrics-provider.h"
#include "ario-debug.h"

G_DEFINE_TYPE (ArioLyricsProvider, ario_lyrics_provider, G_TYPE_OBJECT)

static ArioLyrics *
dummy_lyrics (ArioLyricsProvider *lyrics_provider,
              const char *artist,
              const char *song,
              const char *file)
{
        return NULL;
}

static gchar *
dummy_char (ArioLyricsProvider *lyrics_provider)
{
        return NULL;
}

static void
dummy_void (ArioLyricsProvider *lyrics_provider,
            const gchar *artist,
            const gchar *song,
            GSList **candidates)
{

}

static ArioLyrics *
dummy_lyrics_candidate (ArioLyricsProvider *lyrics_provider,
                        const ArioLyricsCandidate *candidate)
{
        return NULL;
}

static void 
ario_lyrics_provider_class_init (ArioLyricsProviderClass *klass)
{
        klass->get_id = dummy_char;
        klass->get_name = dummy_char;
        klass->get_lyrics = dummy_lyrics;
        klass->get_lyrics_candidates = dummy_void;
        klass->get_lyrics_from_candidate = dummy_lyrics_candidate;
}

static void
ario_lyrics_provider_init (ArioLyricsProvider *lyrics_provider)
{
        lyrics_provider->is_active = FALSE;
}

gchar *
ario_lyrics_provider_get_id (ArioLyricsProvider *lyrics_provider)
{
        g_return_val_if_fail (ARIO_IS_LYRICS_PROVIDER (lyrics_provider), FALSE);

        return ARIO_LYRICS_PROVIDER_GET_CLASS (lyrics_provider)->get_id (lyrics_provider);
}

gchar *
ario_lyrics_provider_get_name (ArioLyricsProvider *lyrics_provider)
{
        g_return_val_if_fail (ARIO_IS_LYRICS_PROVIDER (lyrics_provider), FALSE);

        return ARIO_LYRICS_PROVIDER_GET_CLASS (lyrics_provider)->get_name (lyrics_provider);
}

void
ario_lyrics_provider_get_lyrics_candidates (ArioLyricsProvider *lyrics_provider,
                                            const gchar *artist,
                                            const gchar *song,
                                            GSList **candidates)
{
        g_return_if_fail (ARIO_IS_LYRICS_PROVIDER (lyrics_provider));

        ARIO_LYRICS_PROVIDER_GET_CLASS (lyrics_provider)->get_lyrics_candidates (lyrics_provider,
                                                                                 artist, song,
                                                                                 candidates);
}

ArioLyrics *
ario_lyrics_provider_get_lyrics_from_candidate (ArioLyricsProvider *lyrics_provider,
                                                const ArioLyricsCandidate *candidate)
{
        g_return_val_if_fail (ARIO_IS_LYRICS_PROVIDER (lyrics_provider), NULL);

        return ARIO_LYRICS_PROVIDER_GET_CLASS (lyrics_provider)->get_lyrics_from_candidate (lyrics_provider,
                                                                                            candidate);
}

ArioLyrics *
ario_lyrics_provider_get_lyrics (ArioLyricsProvider *lyrics_provider,
                                 const char *artist,
                                 const char *song,
                                 const char *file)
{
        g_return_val_if_fail (ARIO_IS_LYRICS_PROVIDER (lyrics_provider), NULL);

        return ARIO_LYRICS_PROVIDER_GET_CLASS (lyrics_provider)->get_lyrics (lyrics_provider,
                                                                             artist, song,
                                                                             file);
}


gboolean
ario_lyrics_provider_is_active (ArioLyricsProvider *lyrics_provider)
{
        ARIO_LOG_FUNCTION_START
        return lyrics_provider->is_active;
}

void
ario_lyrics_provider_set_active (ArioLyricsProvider *lyrics_provider,
                                 gboolean is_active)
{
        ARIO_LOG_FUNCTION_START
        lyrics_provider->is_active = is_active;
}

