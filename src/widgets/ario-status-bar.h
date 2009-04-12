/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
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

#include <gtk/gtk.h>

#ifndef __ARIO_STATUS_BAR_H
#define __ARIO_STATUS_BAR_H

G_BEGIN_DECLS

#define TYPE_ARIO_STATUS_BAR         (ario_status_bar_get_type ())
#define ARIO_STATUS_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_STATUS_BAR, ArioStatusBar))
#define ARIO_STATUS_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_STATUS_BAR, ArioStatusBarClass))
#define IS_ARIO_STATUS_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_STATUS_BAR))
#define IS_ARIO_STATUS_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_STATUS_BAR))
#define ARIO_STATUS_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_STATUS_BAR, ArioStatusBarClass))

typedef struct ArioStatusBarPrivate ArioStatusBarPrivate;

/**
 * ArioStatusBar is a GtkStatusbar that display information
 * about music server state
 */
typedef struct
{
        GtkStatusbar parent;

        ArioStatusBarPrivate *priv;
} ArioStatusBar;

typedef struct
{
        GtkStatusbarClass parent_class;
} ArioStatusBarClass;

GType           ario_status_bar_get_type        (void) G_GNUC_CONST;

GtkWidget *     ario_status_bar_new             (void);

G_END_DECLS

#endif /* __ARIO_STATUS_BAR_H */
