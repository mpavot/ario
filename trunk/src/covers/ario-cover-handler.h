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

#include <gtk/gtkdialog.h>
#include "ario-mpd.h"

#ifndef __ARIO_COVER_HANDLER_H
#define __ARIO_COVER_HANDLER_H

G_BEGIN_DECLS

#define TYPE_ARIO_COVER_HANDLER         (ario_cover_handler_get_type ())
#define ARIO_COVER_HANDLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_COVER_HANDLER, ArioCoverHandler))
#define ARIO_COVER_HANDLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_COVER_HANDLER, ArioCoverHandlerClass))
#define IS_ARIO_COVER_HANDLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_COVER_HANDLER))
#define IS_ARIO_COVER_HANDLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_COVER_HANDLER))
#define ARIO_COVER_HANDLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_COVER_HANDLER, ArioCoverHandlerClass))

typedef struct ArioCoverHandlerPrivate ArioCoverHandlerPrivate;

typedef struct
{
        GtkWindow parent;

        ArioCoverHandlerPrivate *priv;
} ArioCoverHandler;

typedef struct
{
        GtkWindowClass parent_class;

        void (*cover_changed)            (ArioCoverHandler *cover_handler);
} ArioCoverHandlerClass;

GType              ario_cover_handler_get_type         (void) G_GNUC_CONST;

ArioCoverHandler * ario_cover_handler_new              (ArioMpd *mpd);

void               ario_cover_handler_force_reload     (void);

ArioCoverHandler * ario_cover_handler_get_instance     (void);

GdkPixbuf *        ario_cover_handler_get_cover        (void);

GdkPixbuf *        ario_cover_handler_get_large_cover  (void);

G_END_DECLS

#endif /* __ARIO_COVER_HANDLER_H */
