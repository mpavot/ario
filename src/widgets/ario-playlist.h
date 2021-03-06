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

#ifndef __ARIO_PLAYLIST_H
#define __ARIO_PLAYLIST_H

#include <gtk/gtk.h>
#include "sources/ario-source.h"

G_BEGIN_DECLS

#define TYPE_ARIO_PLAYLIST         (ario_playlist_get_type ())
#define ARIO_PLAYLIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_PLAYLIST, ArioPlaylist))
#define ARIO_PLAYLIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_PLAYLIST, ArioPlaylistClass))
#define IS_ARIO_PLAYLIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_PLAYLIST))
#define IS_ARIO_PLAYLIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_PLAYLIST))
#define ARIO_PLAYLIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_PLAYLIST, ArioPlaylistClass))

typedef struct ArioPlaylistPrivate ArioPlaylistPrivate;

/*
 * ArioPlaylist represents the current playlist of music server
 * and allows users to modify the playlist content, change the
 * current song,...
 */
typedef struct
{
        ArioSource parent;

        ArioPlaylistPrivate *priv;
} ArioPlaylist;

typedef struct
{
        ArioSourceClass parent;
} ArioPlaylistClass;

GType           ario_playlist_get_type          (void) G_GNUC_CONST;

GtkWidget *     ario_playlist_new               (void);

void            ario_playlist_shutdown          (void);

gint            ario_playlist_get_total_time    (void);

G_END_DECLS

#endif /* __ARIO_PLAYLIST_H */
