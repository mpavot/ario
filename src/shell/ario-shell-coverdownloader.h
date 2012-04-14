/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef __ARIO_SHELL_COVERDOWNLOADER_H
#define __ARIO_SHELL_COVERDOWNLOADER_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_ARIO_SHELL_COVERDOWNLOADER         (ario_shell_coverdownloader_get_type ())
#define ARIO_SHELL_COVERDOWNLOADER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SHELL_COVERDOWNLOADER, ArioShellCoverdownloader))
#define ARIO_SHELL_COVERDOWNLOADER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SHELL_COVERDOWNLOADER, ArioShellCoverdownloaderClass))
#define IS_ARIO_SHELL_COVERDOWNLOADER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SHELL_COVERDOWNLOADER))
#define IS_ARIO_SHELL_COVERDOWNLOADER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SHELL_COVERDOWNLOADER))
#define ARIO_SHELL_COVERDOWNLOADER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SHELL_COVERDOWNLOADER, ArioShellCoverdownloaderClass))

typedef struct ArioShellCoverdownloaderPrivate ArioShellCoverdownloaderPrivate;

/**
 * ArioShellCoverdownloader is a dialog window used to see the
 * progress of cover arts downloading.
 */
typedef struct
{
        GtkWindow parent;

        ArioShellCoverdownloaderPrivate *priv;
} ArioShellCoverdownloader;

typedef struct
{
        GtkWindowClass parent_class;
} ArioShellCoverdownloaderClass;

typedef enum
{
        GET_COVERS,
        REMOVE_COVERS
} ArioShellCoverdownloaderOperation;

GType           ario_shell_coverdownloader_get_type                     (void) G_GNUC_CONST;

GtkWidget *     ario_shell_coverdownloader_new                          (void);

void            ario_shell_coverdownloader_get_covers                   (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                                         const ArioShellCoverdownloaderOperation operation);
void            ario_shell_coverdownloader_get_covers_from_albums       (ArioShellCoverdownloader *ario_shell_coverdownloader,
                                                                         const GSList *albums,
                                                                         const ArioShellCoverdownloaderOperation operation);

G_END_DECLS

#endif /* __ARIO_SHELL_COVERDOWNLOADER_H */
