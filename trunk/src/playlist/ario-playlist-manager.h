/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_PLAYLIST_MANAGER_H
#define __ARIO_PLAYLIST_MANAGER_H

#include <glib-object.h>
#include "playlist/ario-playlist-mode.h"

G_BEGIN_DECLS

#define TYPE_ARIO_PLAYLIST_MANAGER         (ario_playlist_manager_get_type ())
#define ARIO_PLAYLIST_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_PLAYLIST_MANAGER, ArioPlaylistManager))
#define ARIO_PLAYLIST_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_PLAYLIST_MANAGER, ArioPlaylistManagerClass))
#define IS_ARIO_PLAYLIST_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_PLAYLIST_MANAGER))
#define IS_ARIO_PLAYLIST_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_PLAYLIST_MANAGER))
#define ARIO_PLAYLIST_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_PLAYLIST_MANAGER, ArioPlaylistManagerClass))

typedef struct ArioPlaylistManagerPrivate ArioPlaylistManagerPrivate;

typedef struct
{
        GObject parent;

        ArioPlaylistManagerPrivate *priv;
} ArioPlaylistManager;

typedef struct
{
        GObjectClass parent;
} ArioPlaylistManagerClass;

GType                   ario_playlist_manager_get_type                  (void) G_GNUC_CONST;

ArioPlaylistManager*    ario_playlist_manager_get_instance              (void);

void                    ario_playlist_manager_add_mode                  (ArioPlaylistManager *playlist_manager,
                                                                         ArioPlaylistMode *playlist_mode);
void                    ario_playlist_manager_remove_mode               (ArioPlaylistManager *playlist_manager,
                                                                         ArioPlaylistMode *playlist_mode);
GSList*                 ario_playlist_manager_get_modes                 (ArioPlaylistManager *playlist_manager);
ArioPlaylistMode*       ario_playlist_manager_get_mode_from_id          (ArioPlaylistManager *playlist_manager,
                                                                         const gchar *id);

G_END_DECLS

#endif /* __ARIO_PLAYLIST_MANAGER_H */
