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

#ifndef __ARIO_BROWSER_PREFERENCES_H
#define __ARIO_BROWSER_PREFERENCES_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_ARIO_BROWSER_PREFERENCES         (ario_browser_preferences_get_type ())
#define ARIO_BROWSER_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_BROWSER_PREFERENCES, ArioBrowserPreferences))
#define ARIO_BROWSER_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_BROWSER_PREFERENCES, ArioBrowserPreferencesClass))
#define IS_ARIO_BROWSER_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_BROWSER_PREFERENCES))
#define IS_ARIO_BROWSER_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_BROWSER_PREFERENCES))
#define ARIO_BROWSER_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_BROWSER_PREFERENCES, ArioBrowserPreferencesClass))

typedef struct ArioBrowserPreferencesPrivate ArioBrowserPreferencesPrivate;

typedef struct
{
        GtkBox parent;

        ArioBrowserPreferencesPrivate *priv;
} ArioBrowserPreferences;

typedef struct
{
        GtkBoxClass parent_class;
} ArioBrowserPreferencesClass;

GType              ario_browser_preferences_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_browser_preferences_new              (void);

G_END_DECLS

#endif /* __ARIO_BROWSER_PREFERENCES_H */
