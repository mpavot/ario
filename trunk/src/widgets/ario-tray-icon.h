/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *   Based on:
 *   Implementation of Rhythmbox tray icon object
 *   Copyright (C) 2003,2004 Colin Walters <walters@rhythmbox.org>
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

#include <gtk/gtkuimanager.h>

#include "lib/eggtrayicon.h"
#include "ario-mpd.h"

#ifndef __ARIO_TRAY_ICON_H
#define __ARIO_TRAY_ICON_H

G_BEGIN_DECLS

#define TYPE_ARIO_TRAY_ICON         (ario_tray_icon_get_type ())
#define ARIO_TRAY_ICON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_TRAY_ICON, ArioTrayIcon))
#define ARIO_TRAY_ICON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_TRAY_ICON, ArioTrayIconClass))
#define IS_ARIO_TRAY_ICON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_TRAY_ICON))
#define IS_ARIO_TRAY_ICON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_TRAY_ICON))
#define ARIO_TRAY_ICON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_TRAY_ICON, ArioTrayIconClass))

typedef struct ArioTrayIconPrivate ArioTrayIconPrivate;

typedef struct
{
        EggTrayIcon parent;

        ArioTrayIconPrivate *priv;
} ArioTrayIcon;

typedef struct
{
        EggTrayIconClass parent_class;
} ArioTrayIconClass;

GType                   ario_tray_icon_get_type         (void);

ArioTrayIcon *          ario_tray_icon_new              (GtkUIManager *mgr,
                                                         GtkWindow *window,
                                                         ArioMpd *mpd);

G_END_DECLS

#endif /* __ARIO_TRAY_ICON_H */
