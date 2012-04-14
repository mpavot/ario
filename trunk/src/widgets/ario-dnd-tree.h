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

#ifndef __ARIO_DND_TREE_H
#define __ARIO_DND_TREE_H

#include <gtk/gtk.h>
#include <config.h>

G_BEGIN_DECLS

#define TYPE_ARIO_DND_TREE         (ario_dnd_tree_get_type ())
#define ARIO_DND_TREE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_DND_TREE, ArioDndTree))
#define ARIO_DND_TREE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_DND_TREE, ArioDndTreeClass))
#define IS_ARIO_DND_TREE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_DND_TREE))
#define IS_ARIO_DND_TREE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_DND_TREE))
#define ARIO_DND_TREE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_DND_TREE, ArioDndTreeClass))

typedef struct ArioDndTreePrivate ArioDndTreePrivate;

/*
 * ArioDndTree is a GtkTreeView with enhanced drag and drop
 * features.
 */
typedef struct
{
        GtkTreeView parent;

        ArioDndTreePrivate *priv;
} ArioDndTree;

typedef struct
{
        GtkTreeViewClass parent;

        /* Signals */
        void (*popup)           (ArioDndTree *tree);

        void (*activate)        (ArioDndTree *tree);
} ArioDndTreeClass;

GType                   ario_dnd_tree_get_type   (void) G_GNUC_CONST;

GtkWidget*              ario_dnd_tree_new        (const GtkTargetEntry* targets,
                                                  const gint n_targets,
                                                  const gboolean browse_mode);

G_END_DECLS

#endif /* __ARIO_DND_TREE_H */
