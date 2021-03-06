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

#ifndef __ARIO_SERVER_PREFERENCES_H
#define __ARIO_SERVER_PREFERENCES_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_ARIO_SERVER_PREFERENCES         (ario_server_preferences_get_type ())
#define ARIO_SERVER_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SERVER_PREFERENCES, ArioServerPreferences))
#define ARIO_SERVER_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SERVER_PREFERENCES, ArioServerPreferencesClass))
#define IS_ARIO_SERVER_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SERVER_PREFERENCES))
#define IS_ARIO_SERVER_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SERVER_PREFERENCES))
#define ARIO_SERVER_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SERVER_PREFERENCES, ArioServerPreferencesClass))

typedef struct ArioServerPreferencesPrivate ArioServerPreferencesPrivate;

typedef struct
{
        GtkBox parent;

        ArioServerPreferencesPrivate *priv;
} ArioServerPreferences;

typedef struct
{
        GtkBoxClass parent_class;
} ArioServerPreferencesClass;

GType              ario_server_preferences_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_server_preferences_new              (void);

G_END_DECLS

#endif /* __ARIO_SERVER_PREFERENCES_H */
