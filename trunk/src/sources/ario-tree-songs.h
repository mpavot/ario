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

#ifndef __ARIO_TREE_SONGS_H
#define __ARIO_TREE_SONGS_H

#include "sources/ario-tree.h"

G_BEGIN_DECLS

#define TYPE_ARIO_TREE_SONGS         (ario_tree_songs_get_type ())
#define ARIO_TREE_SONGS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_TREE_SONGS, ArioTreeSongs))
#define ARIO_TREE_SONGS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_TREE_SONGS, ArioTreeSongsClass))
#define IS_ARIO_TREE_SONGS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_TREE_SONGS))
#define IS_ARIO_TREE_SONGS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_TREE_SONGS))
#define ARIO_TREE_SONGS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_TREE_SONGS, ArioTreeSongsClass))

/**
 * ArioTreeSongs is a specialization of ArioTree to have a nicer
 * display of songs arts and special menus.
 */
typedef struct
{
        ArioTree parent;
} ArioTreeSongs;

typedef struct
{
        ArioTreeClass parent;
} ArioTreeSongsClass;

GType                   ario_tree_songs_get_type              (void) G_GNUC_CONST;

void                    ario_tree_songs_cmd_songs_properties  (ArioTreeSongs *tree);

G_END_DECLS

#endif /* __ARIO_TREE_SONGS_H */
