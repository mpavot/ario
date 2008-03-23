/*
 *  Copyright (C) 2007 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_FIRSTLAUNCH_H
#define __ARIO_FIRSTLAUNCH_H

#include <gtk/gtkassistant.h>

G_BEGIN_DECLS

#define TYPE_ARIO_FIRSTLAUNCH         (ario_firstlaunch_get_type ())
#define ARIO_FIRSTLAUNCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_FIRSTLAUNCH, ArioFirstlaunch))
#define ARIO_FIRSTLAUNCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_FIRSTLAUNCH, ArioFirstlaunchClass))
#define IS_ARIO_FIRSTLAUNCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_FIRSTLAUNCH))
#define IS_ARIO_FIRSTLAUNCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_FIRSTLAUNCH))
#define ARIO_FIRSTLAUNCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_FIRSTLAUNCH, ArioFirstlaunchClass))

typedef struct ArioFirstlaunchPrivate ArioFirstlaunchPrivate;

typedef struct
{
        GtkAssistant parent;

        ArioFirstlaunchPrivate *priv;
} ArioFirstlaunch;

typedef struct
{
        GtkAssistantClass parent_class;
} ArioFirstlaunchClass;

GType                   ario_firstlaunch_get_type     (void) G_GNUC_CONST;

ArioFirstlaunch *       ario_firstlaunch_new          (void);

G_END_DECLS

#endif /* __ARIO_FIRSTLAUNCH_H */
