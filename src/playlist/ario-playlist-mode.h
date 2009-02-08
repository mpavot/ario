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

#ifndef __ARIO_PLAYLIST_MODE_H__
#define __ARIO_PLAYLIST_MODE_H__

#include <glib-object.h>
#include "widgets/ario-playlist.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_PLAYLIST_MODE              (ario_playlist_mode_get_type())
#define ARIO_PLAYLIST_MODE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_PLAYLIST_MODE, ArioPlaylistMode))
#define ARIO_PLAYLIST_MODE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_PLAYLIST_MODE, ArioPlaylistModeClass))
#define ARIO_IS_PLAYLIST_MODE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_PLAYLIST_MODE))
#define ARIO_IS_PLAYLIST_MODE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_PLAYLIST_MODE))
#define ARIO_PLAYLIST_MODE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_PLAYLIST_MODE, ArioPlaylistModeClass))

/*
 * Main object structure
 */
typedef struct _ArioPlaylistMode 
{
        GObject parent;
} ArioPlaylistMode;

/*
 * Class definition
 */
typedef struct 
{
        GObjectClass parent_class;

        /* Virtual public methods */

        gchar*          (*get_id)                               (ArioPlaylistMode *playlist_mode);

        gchar*          (*get_name)                             (ArioPlaylistMode *playlist_mode);

        void            (*next_song)                            (ArioPlaylistMode *playlist_mode,
                                                                 ArioPlaylist *playlist);
        void            (*last_song)                            (ArioPlaylistMode *playlist_mode,
                                                                 ArioPlaylist *playlist);
        GtkWidget*      (*get_config)                           (ArioPlaylistMode *playlist_mode);

} ArioPlaylistModeClass;

/*
 * Public methods
 */
GType           ario_playlist_mode_get_type            (void) G_GNUC_CONST;

gchar*          ario_playlist_mode_get_id              (ArioPlaylistMode *playlist_mode);

gchar*          ario_playlist_mode_get_name            (ArioPlaylistMode *playlist_mode);

void            ario_playlist_mode_next_song           (ArioPlaylistMode *playlist_mode,
                                                        ArioPlaylist *playlist);
void            ario_playlist_mode_last_song           (ArioPlaylistMode *playlist_mode,
                                                        ArioPlaylist *playlist);
GtkWidget*      ario_playlist_mode_get_config          (ArioPlaylistMode *playlist_mode);

G_END_DECLS

#endif  /* __ARIO_PLAYLIST_MODE_H__ */


