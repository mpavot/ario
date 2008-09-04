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

#ifndef __ARIO_TREE_H
#define __ARIO_TREE_H

#include <gtk/gtkhbox.h>
#include "ario-mpd.h"
#include "widgets/ario-playlist.h"
#include "sources/ario-source.h"

G_BEGIN_DECLS

#define TYPE_ARIO_TREE         (ario_tree_get_type ())
#define ARIO_TREE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_TREE, ArioTree))
#define ARIO_TREE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_TREE, ArioTreeClass))
#define IS_ARIO_TREE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_TREE))
#define IS_ARIO_TREE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_TREE))
#define ARIO_TREE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_TREE, ArioTreeClass))

typedef struct ArioTreePrivate ArioTreePrivate;

typedef struct
{
        GtkScrolledWindow parent;

        ArioTreePrivate *priv;
} ArioTree;

typedef struct
{
        GtkScrolledWindowClass parent;

        void (*selection_changed) (ArioTree *tree);

        void (*menu_popup) (ArioTree *tree);
} ArioTreeClass;

GType                   ario_tree_get_type              (void) G_GNUC_CONST;

GtkWidget*              ario_tree_new                   (GtkUIManager *mgr,
                                                         ArioMpd *mpd,
                                                         ArioPlaylist *playlist,
                                                         ArioMpdTag tag,
                                                         gboolean is_first);
void                    ario_tree_fill                  (ArioTree *tree);

void                    ario_tree_clear_criterias       (ArioTree *tree);

void                    ario_tree_add_criteria          (ArioTree *tree,
                                                         ArioMpdCriteria *criteria);
GSList*                 ario_tree_get_criterias         (ArioTree *tree);

void                    ario_tree_cmd_add               (ArioTree *tree,
                                                         const gboolean play);
void                    ario_tree_cmd_get_cover         (ArioTree *tree);

void                    ario_tree_cmd_remove_cover      (ArioTree *tree);

void                    ario_tree_cmd_albums_properties (ArioTree *tree);

void                    ario_tree_cmd_songs_properties  (ArioTree *tree);

G_END_DECLS

#endif /* __ARIO_TREE_H */
