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

#ifndef __ARIO_SOURCEMANAGER_H
#define __ARIO_SOURCEMANAGER_H

#include <gtk/gtknotebook.h>
#include "sources/ario-source.h"

G_BEGIN_DECLS

typedef enum
{
        ARIO_SOURCE_BROWSER,
        ARIO_SOURCE_RADIO,
        ARIO_SOURCE_SEARCH,
        ARIO_SOURCE_PLAYLISTS,
        ARIO_SOURCE_FILESYSTEM
}ArioSourceType;

#define TYPE_ARIO_SOURCEMANAGER         (ario_sourcemanager_get_type ())
#define ARIO_SOURCEMANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SOURCEMANAGER, ArioSourceManager))
#define ARIO_SOURCEMANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SOURCEMANAGER, ArioSourceManagerClass))
#define IS_ARIO_SOURCEMANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SOURCEMANAGER))
#define IS_ARIO_SOURCEMANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SOURCEMANAGER))
#define ARIO_SOURCEMANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SOURCEMANAGER, ArioSourceManagerClass))

typedef struct ArioSourceManagerPrivate ArioSourceManagerPrivate;

typedef struct
{
        GtkNotebook parent;

        ArioSourceManagerPrivate *priv;
} ArioSourceManager;

typedef struct
{
        GtkNotebookClass parent;

} ArioSourceManagerClass;

GType                   ario_sourcemanager_get_type     (void) G_GNUC_CONST;

GtkWidget*              ario_sourcemanager_get_instance (GtkUIManager *mgr,
                                                         GtkActionGroup *group);

void                    ario_sourcemanager_append       (ArioSource *source);
void                    ario_sourcemanager_remove       (ArioSource *source);
void                    ario_sourcemanager_reorder      (void);
void                    ario_sourcemanager_shutdown     (void);

G_END_DECLS

#endif /* __ARIO_SOURCEMANAGER_H */
