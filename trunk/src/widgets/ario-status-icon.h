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

#ifndef __ARIO_STATUS_ICON_H
#define __ARIO_STATUS_ICON_H

#include <gtk/gtkuimanager.h>
#include "ario-mpd.h"

G_BEGIN_DECLS

#define TYPE_ARIO_STATUS_ICON         (ario_status_icon_get_type ())
#define ARIO_STATUS_ICON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_STATUS_ICON, ArioStatusIcon))
#define ARIO_STATUS_ICON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_STATUS_ICON, ArioStatusIconClass))
#define IS_ARIO_STATUS_ICON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_STATUS_ICON))
#define IS_ARIO_STATUS_ICON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_STATUS_ICON))
#define ARIO_STATUS_ICON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_STATUS_ICON, ArioStatusIconClass))

typedef struct ArioStatusIconPrivate ArioStatusIconPrivate;

typedef struct
{
        GtkStatusIcon parent;

        ArioStatusIconPrivate *priv;
} ArioStatusIcon;

typedef struct
{
        GtkStatusIconClass parent_class;
} ArioStatusIconClass;

GType                   ario_status_icon_get_type       (void) G_GNUC_CONST;

ArioStatusIcon *        ario_status_icon_new            (GtkActionGroup *group,
                                                         GtkUIManager *mgr,
                                                         GtkWindow *window,
                                                         ArioMpd *mpd);

G_END_DECLS

#endif /* __ARIO_STATUS_ICON_H */
