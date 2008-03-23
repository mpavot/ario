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

#include <gtk/gtkvbox.h>
#include "ario-mpd.h"

#ifndef __ARIO_CONNECTION_PREFERENCES_H
#define __ARIO_CONNECTION_PREFERENCES_H

G_BEGIN_DECLS

#define TYPE_ARIO_CONNECTION_PREFERENCES         (ario_connection_preferences_get_type ())
#define ARIO_CONNECTION_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_CONNECTION_PREFERENCES, ArioConnectionPreferences))
#define ARIO_CONNECTION_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_CONNECTION_PREFERENCES, ArioConnectionPreferencesClass))
#define IS_ARIO_CONNECTION_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_CONNECTION_PREFERENCES))
#define IS_ARIO_CONNECTION_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_CONNECTION_PREFERENCES))
#define ARIO_CONNECTION_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_CONNECTION_PREFERENCES, ArioConnectionPreferencesClass))

typedef struct ArioConnectionPreferencesPrivate ArioConnectionPreferencesPrivate;

typedef struct
{
        GtkVBox parent;

        ArioConnectionPreferencesPrivate *priv;
} ArioConnectionPreferences;

typedef struct
{
        GtkDialogClass parent_class;
} ArioConnectionPreferencesClass;

GType              ario_connection_preferences_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_connection_preferences_new              (ArioMpd *mpd);

G_END_DECLS

#endif /* __ARIO_CONNECTION_PREFERENCES_H */
