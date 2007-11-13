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


#ifndef __ARIO_LYRICS_H
#define __ARIO_LYRICS_H

typedef struct ArioLyrics
{
        gchar *artist;
        gchar *title;
        gchar *lyrics;
} ArioLyrics;

ArioLyrics *    ario_lyrics_get_lyrics          (const gchar *artist,
                                                 const gchar *title);

void            ario_lyrics_free                (ArioLyrics *lyrics);

gboolean        ario_lyrics_save_lyrics         (const gchar *artist,
                                                 const gchar *title,
                                                 const gchar *lyrics);

void            ario_lyrics_remove_lyrics       (const gchar *artist,
                                                 const gchar *title);

gboolean        ario_lyrics_lyrics_exists       (const gchar *artist,
                                                 const gchar *title);

gchar*          ario_lyrics_make_lyrics_path    (const gchar *artist,
                                                 const gchar *title);

G_END_DECLS

#endif /* __ARIO_LYRICS_H */
