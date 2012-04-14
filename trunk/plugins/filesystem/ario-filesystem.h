/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_FILESYSTEM_H
#define __ARIO_FILESYSTEM_H

#include <gtk/gtk.h>
#include <config.h>
#include "widgets/ario-playlist.h"
#include "sources/ario-source.h"

G_BEGIN_DECLS

#define TYPE_ARIO_FILESYSTEM         (ario_filesystem_get_type ())
#define ARIO_FILESYSTEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_FILESYSTEM, ArioFilesystem))
#define ARIO_FILESYSTEM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_FILESYSTEM, ArioFilesystemClass))
#define IS_ARIO_FILESYSTEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_FILESYSTEM))
#define IS_ARIO_FILESYSTEM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_FILESYSTEM))
#define ARIO_FILESYSTEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_FILESYSTEM, ArioFilesystemClass))

typedef struct ArioFilesystemPrivate ArioFilesystemPrivate;

typedef struct
{
        ArioSource parent;

        ArioFilesystemPrivate *priv;
} ArioFilesystem;

typedef struct
{
        ArioSourceClass parent;
} ArioFilesystemClass;

GType                   ario_filesystem_get_type   (void) G_GNUC_CONST;

GtkWidget*              ario_filesystem_new        (GtkUIManager *mgr,
                                                    GtkActionGroup *group);
G_END_DECLS

#endif /* __ARIO_FILESYSTEM_H */
