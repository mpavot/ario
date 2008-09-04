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

#ifndef __ARIO_SONGLIST_H
#define __ARIO_SONGLIST_H

#include <gtk/gtktreeview.h>
#include <config.h>
#include "ario-mpd.h"
#include "widgets/ario-playlist.h"

G_BEGIN_DECLS

#define TYPE_ARIO_SONGLIST         (ario_songlist_get_type ())
#define ARIO_SONGLIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SONGLIST, ArioSonglist))
#define ARIO_SONGLIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SONGLIST, ArioSonglistClass))
#define IS_ARIO_SONGLIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SONGLIST))
#define IS_ARIO_SONGLIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SONGLIST))
#define ARIO_SONGLIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SONGLIST, ArioSonglistClass))

typedef struct ArioSonglistPrivate ArioSonglistPrivate;

typedef struct
{
        GtkTreeView parent;

        ArioSonglistPrivate *priv;
} ArioSonglist;

typedef struct
{
        GtkTreeViewClass parent;
} ArioSonglistClass;

enum
{
        SONGS_TITLE_COLUMN,
        SONGS_ARTIST_COLUMN,
        SONGS_ALBUM_COLUMN,
        SONGS_FILENAME_COLUMN,
        SONGS_N_COLUMN
};

GType                   ario_songlist_get_type                  (void) G_GNUC_CONST;

GtkWidget*              ario_songlist_new                       (GtkUIManager *mgr,
                                                                 ArioMpd *mpd,
                                                                 ArioPlaylist *playlist,
                                                                 gchar *popup,
                                                                 gboolean is_sortable);
GtkListStore*           ario_songlist_get_liststore             (ArioSonglist *songlist);

void                    ario_songlist_cmd_add_songlists         (GtkAction *action,
                                                                 ArioSonglist *songlist);

void                    ario_songlist_cmd_add_play_songlists    (GtkAction *action,
                                                                 ArioSonglist *songlist);

void                    ario_songlist_cmd_songs_properties      (GtkAction *action,
                                                                 ArioSonglist *songlist);

void                    ario_songlist_cmd_clear_add_play_songlists (GtkAction *action,
                                                                    ArioSonglist *songlist);

G_END_DECLS

#endif /* __ARIO_SONGLIST_H */
