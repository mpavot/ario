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

#ifndef __ARIO_HEADER_H
#define __ARIO_HEADER_H

#include <gtk/gtkhbox.h>
#include "ario-mpd.h"
#include "ario-cover-handler.h"

G_BEGIN_DECLS

#define TYPE_ARIO_HEADER         (ario_header_get_type ())
#define ARIO_HEADER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_HEADER, ArioHeader))
#define ARIO_HEADER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_HEADER, ArioHeaderClass))
#define IS_ARIO_HEADER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_HEADER))
#define IS_ARIO_HEADER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_HEADER))
#define ARIO_HEADER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_HEADER, ArioHeaderClass))

typedef struct ArioHeaderPrivate ArioHeaderPrivate;

typedef struct
{
        GtkHBox parent;

        ArioHeaderPrivate *priv;
} ArioHeader;

typedef struct
{
        GtkHBoxClass parent;
} ArioHeaderClass;

GType           ario_header_get_type                 (void);

GtkWidget *     ario_header_new                      (GtkActionGroup *group,
                                                      ArioMpd *mpd,
                                                      ArioCoverHandler *cover_handler);

void            ario_header_do_next                  (ArioHeader *header);

void            ario_header_do_previous              (ArioHeader *header);

void            ario_header_playpause                (ArioHeader *header);

void            ario_header_stop                     (ArioHeader *header);


G_END_DECLS

#endif /* __ARIO_HEADER_H */