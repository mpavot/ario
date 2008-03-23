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

#include <gtk/gtkwindow.h>
#include "ario-mpd.h"

#ifndef __ARIO_SHELL_LYRICS_H
#define __ARIO_SHELL_LYRICS_H

G_BEGIN_DECLS

#define TYPE_ARIO_SHELL_LYRICS         (ario_shell_lyrics_get_type ())
#define ARIO_SHELL_LYRICS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SHELL_LYRICS, ArioShellLyrics))
#define ARIO_SHELL_LYRICS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SHELL_LYRICS, ArioShellLyricsClass))
#define IS_ARIO_SHELL_LYRICS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SHELL_LYRICS))
#define IS_ARIO_SHELL_LYRICS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SHELL_LYRICS))
#define ARIO_SHELL_LYRICS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SHELL_LYRICS, ArioShellLyricsClass))

typedef struct ArioShellLyricsPrivate ArioShellLyricsPrivate;

typedef struct
{
        GtkWindow parent;

        ArioShellLyricsPrivate *priv;
} ArioShellLyrics;

typedef struct
{
        GtkWindowClass parent_class;
} ArioShellLyricsClass;

GType              ario_shell_lyrics_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_shell_lyrics_new              (ArioMpd *mpd);

G_END_DECLS

#endif /* __ARIO_SHELL_LYRICS_H */
