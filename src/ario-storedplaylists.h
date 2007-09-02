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

#ifndef __ARIO_STOREDPLAYLISTS_H
#define __ARIO_STOREDPLAYLISTS_H

#include <gtk/gtkhpaned.h>
#include <config.h>
#include "ario-mpd.h"
#include "ario-playlist.h"

#ifdef ENABLE_STOREDPLAYLISTS

G_BEGIN_DECLS

#define TYPE_ARIO_STOREDPLAYLISTS         (ario_storedplaylists_get_type ())
#define ARIO_STOREDPLAYLISTS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_STOREDPLAYLISTS, ArioStoredplaylists))
#define ARIO_STOREDPLAYLISTS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_STOREDPLAYLISTS, ArioStoredplaylistsClass))
#define IS_ARIO_STOREDPLAYLISTS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_STOREDPLAYLISTS))
#define IS_ARIO_STOREDPLAYLISTS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_STOREDPLAYLISTS))
#define ARIO_STOREDPLAYLISTS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_STOREDPLAYLISTS, ArioStoredplaylistsClass))

typedef struct ArioStoredplaylistsPrivate ArioStoredplaylistsPrivate;

typedef struct
{
        GtkHPaned parent;

        ArioStoredplaylistsPrivate *priv;
} ArioStoredplaylists;

typedef struct
{
        GtkHPanedClass parent;
} ArioStoredplaylistsClass;

GType                   ario_storedplaylists_get_type   (void);

GtkWidget*              ario_storedplaylists_new        (GtkUIManager *mgr,
                                                         GtkActionGroup *group,
                                                         ArioMpd *mpd,
                                                         ArioPlaylist *playlist);
void                    ario_storedplaylists_shutdown   (ArioStoredplaylists *storedplaylists);
G_END_DECLS

#endif  /* ENABLE_STOREDPLAYLISTS */

#endif /* __ARIO_STOREDPLAYLISTS_H */
