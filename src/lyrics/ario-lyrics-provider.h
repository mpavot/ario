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

#ifndef __ARIO_LYRICS_PROVIDER_H__
#define __ARIO_LYRICS_PROVIDER_H__

typedef struct _ArioLyricsProvider ArioLyricsProvider;

#include <glib-object.h>
#include "lyrics/ario-lyrics.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_LYRICS_PROVIDER              (ario_lyrics_provider_get_type())
#define ARIO_LYRICS_PROVIDER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_LYRICS_PROVIDER, ArioLyricsProvider))
#define ARIO_LYRICS_PROVIDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_LYRICS_PROVIDER, ArioLyricsProviderClass))
#define ARIO_IS_LYRICS_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_LYRICS_PROVIDER))
#define ARIO_IS_LYRICS_PROVIDER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_LYRICS_PROVIDER))
#define ARIO_LYRICS_PROVIDER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_LYRICS_PROVIDER, ArioLyricsProviderClass))

/*
 * Main object structure
 */
struct _ArioLyricsProvider
{
        GObject parent;

        gboolean is_active;
};

/*
 * Class definition
 */
typedef struct 
{
        GObjectClass parent_class;

        /* Virtual public methods */

        gchar*          (*get_id)                       (ArioLyricsProvider *lyrics_provider);

        gchar*          (*get_name)                     (ArioLyricsProvider *lyrics_provider);

        ArioLyrics*     (*get_lyrics)                   (ArioLyricsProvider *lyrics_provider,
                                                         const char *artist,
                                                         const char *song,
                                                         const char *file);

        void            (*get_lyrics_candidates)        (ArioLyricsProvider *lyrics_provider,
                                                         const gchar *artist,
                                                         const gchar *song,
                                                         GSList **candidates);

        ArioLyrics*     (*get_lyrics_from_candidate)    (ArioLyricsProvider *lyrics_provider,
                                                         const ArioLyricsCandidate *candidate);
} ArioLyricsProviderClass;

/*
 * Public methods
 */
GType           ario_lyrics_provider_get_type                   (void) G_GNUC_CONST;

gchar*          ario_lyrics_provider_get_id                     (ArioLyricsProvider *lyrics_provider);

gchar*          ario_lyrics_provider_get_name                   (ArioLyricsProvider *lyrics_provider);

ArioLyrics*     ario_lyrics_provider_get_lyrics                 (ArioLyricsProvider *lyrics_provider,
                                                                 const char *artist,
                                                                 const char *song,
                                                                 const char *file);
void            ario_lyrics_provider_get_lyrics_candidates      (ArioLyricsProvider *lyrics_provider,
                                                                 const gchar *artist,
                                                                 const gchar *song,
                                                                 GSList **candidates);
ArioLyrics*     ario_lyrics_provider_get_lyrics_from_candidate  (ArioLyricsProvider *lyrics_provider,
                                                                 const ArioLyricsCandidate *candidate);

gboolean        ario_lyrics_provider_is_active                  (ArioLyricsProvider *lyrics_provider);

void            ario_lyrics_provider_set_active                 (ArioLyricsProvider *lyrics_provider,
                                                                 gboolean is_active);

G_END_DECLS

#endif  /* __ARIO_LYRICS_PROVIDER_H__ */


