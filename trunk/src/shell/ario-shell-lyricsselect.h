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


#ifndef __ARIO_SHELL_LYRICSSELECT_H
#define __ARIO_SHELL_LYRICSSELECT_H

G_BEGIN_DECLS
#include "ario-lyrics.h"

#define TYPE_ARIO_SHELL_LYRICSSELECT         (ario_shell_lyricsselect_get_type ())
#define ARIO_SHELL_LYRICSSELECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SHELL_LYRICSSELECT, ArioShellLyricsselect))
#define ARIO_SHELL_LYRICSSELECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SHELL_LYRICSSELECT, ArioShellLyricsselectClass))
#define IS_ARIO_SHELL_LYRICSSELECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SHELL_LYRICSSELECT))
#define IS_ARIO_SHELL_LYRICSSELECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SHELL_LYRICSSELECT))
#define ARIO_SHELL_LYRICSSELECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SHELL_LYRICSSELECT, ArioShellLyricsselectClass))

typedef struct ArioShellLyricsselectPrivate ArioShellLyricsselectPrivate;

typedef struct
{
        GtkDialog parent;

        ArioShellLyricsselectPrivate *priv;
} ArioShellLyricsselect;

typedef struct
{
        GtkDialogClass parent_class;
} ArioShellLyricsselectClass;

GType                   ario_shell_lyricsselect_get_type                (void);

GtkWidget *             ario_shell_lyricsselect_new                     (const char *artist,
                                                                         const char *album);

ArioLyricsCandidate *   ario_shell_lyricsselect_get_lyrics_candidate    (ArioShellLyricsselect *ario_shell_lyricsselect);

G_END_DECLS

#endif /* __ARIO_SHELL_LYRICSSELECT_H */
