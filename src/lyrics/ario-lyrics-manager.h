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

#ifndef __ARIO_LYRICS_MANAGER_H
#define __ARIO_LYRICS_MANAGER_H

#include <glib-object.h>
#include "lyrics/ario-lyrics-provider.h"

G_BEGIN_DECLS

#define TYPE_ARIO_LYRICS_MANAGER         (ario_lyrics_manager_get_type ())
#define ARIO_LYRICS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_LYRICS_MANAGER, ArioLyricsManager))
#define ARIO_LYRICS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_LYRICS_MANAGER, ArioLyricsManagerClass))
#define IS_ARIO_LYRICS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_LYRICS_MANAGER))
#define IS_ARIO_LYRICS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_LYRICS_MANAGER))
#define ARIO_LYRICS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_LYRICS_MANAGER, ArioLyricsManagerClass))

typedef struct ArioLyricsManagerPrivate ArioLyricsManagerPrivate;

typedef struct
{
        GObject parent;

        ArioLyricsManagerPrivate *priv;
} ArioLyricsManager;

typedef struct
{
        GObjectClass parent;
} ArioLyricsManagerClass;

GType                   ario_lyrics_manager_get_type                     (void) G_GNUC_CONST;

ArioLyricsManager*      ario_lyrics_manager_get_instance                 (void);

void                    ario_lyrics_manager_add_provider                 (ArioLyricsManager *lyrics_manager,
                                                                          ArioLyricsProvider *lyrics_provider);
void                    ario_lyrics_manager_remove_provider              (ArioLyricsManager *lyrics_manager,
                                                                          ArioLyricsProvider *lyrics_provider);
void                    ario_lyrics_manager_update_providers             (ArioLyricsManager *lyrics_manager);
GSList*                 ario_lyrics_manager_get_providers                (ArioLyricsManager *lyrics_manager);
void                    ario_lyrics_manager_set_providers                (ArioLyricsManager *lyrics_manager,
                                                                          GSList *providers);
ArioLyricsProvider*     ario_lyrics_manager_get_provider_from_id         (ArioLyricsManager *lyrics_manager,
                                                                          const gchar *id);
void                    ario_lyrics_manager_shutdown                     (ArioLyricsManager *lyrics_manager);

ArioLyrics*             ario_lyrics_manager_get_lyrics                   (ArioLyricsManager *lyrics_manager,
                                                                          const char *artist,
                                                                          const char *song,
                                                                          const char *file);

void                    ario_lyrics_manager_get_lyrics_candidates        (ArioLyricsManager *lyrics_manager,
                                                                          const gchar *artist,
                                                                          const gchar *song,
                                                                          GSList **candidates);

G_END_DECLS

#endif /* __ARIO_LYRICS_MANAGER_H */
