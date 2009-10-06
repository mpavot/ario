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

#ifndef __ARIO_TOOLTIP_H
#define __ARIO_TOOLTIP_H

#include <config.h>
#include <gtk/gtkhbox.h>

G_BEGIN_DECLS

#define TYPE_ARIO_TOOLTIP         (ario_tooltip_get_type ())
#define ARIO_TOOLTIP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_TOOLTIP, ArioTooltip))
#define ARIO_TOOLTIP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_TOOLTIP, ArioTooltipClass))
#define IS_ARIO_TOOLTIP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_TOOLTIP))
#define IS_ARIO_TOOLTIP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_TOOLTIP))
#define ARIO_TOOLTIP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_TOOLTIP, ArioTooltipClass))

typedef struct ArioTooltipPrivate ArioTooltipPrivate;

/**
 * ArioTooltip provides a widget representing music server state
 * It can be used for notification or as a tooltip.
 */
typedef struct
{
        GtkHBox parent;

        ArioTooltipPrivate *priv;
} ArioTooltip;

typedef struct
{
        GtkHBoxClass parent_class;
} ArioTooltipClass;

GType                  ario_tooltip_get_type         (void) G_GNUC_CONST;

GtkWidget *            ario_tooltip_new              (void);

G_END_DECLS

#endif /* __ARIO_TOOLTIP_H */
