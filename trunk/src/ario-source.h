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

#ifndef __ARIO_SOURCE_H
#define __ARIO_SOURCE_H

#include <gtk/gtknotebook.h>
#include "ario-mpd.h"
#include "ario-playlist.h"

G_BEGIN_DECLS

#define TYPE_ARIO_SOURCE         (ario_source_get_type ())
#define ARIO_SOURCE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SOURCE, ArioSource))
#define ARIO_SOURCE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SOURCE, ArioSourceClass))
#define IS_ARIO_SOURCE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SOURCE))
#define IS_ARIO_SOURCE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SOURCE))
#define ARIO_SOURCE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SOURCE, ArioSourceClass))

typedef struct ArioSourcePrivate ArioSourcePrivate;

typedef struct
{
        GtkNotebook parent;

        ArioSourcePrivate *priv;
} ArioSource;

typedef struct
{
        GtkNotebookClass parent;
} ArioSourceClass;

typedef enum
{
        ARIO_SOURCE_BROWSER,
        ARIO_SOURCE_RADIO,
        ARIO_SOURCE_SEARCH
}ArioSourceType;

GType                   ario_source_get_type   (void);

GtkWidget*              ario_source_new        (GtkUIManager *mgr,
                                                GtkActionGroup *group,
                                                ArioMpd *mpd,
                                                ArioPlaylist *playlist);

void                    ario_source_set_source (ArioSource *source,
                                                ArioSourceType source_type);

G_END_DECLS

#endif /* __ARIO_SOURCE_H */
