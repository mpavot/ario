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

#ifndef __ARIO_COVER_PREFERENCES_H
#define __ARIO_COVER_PREFERENCES_H

#include <glib.h>
#include <gtk/gtkvbox.h>

G_BEGIN_DECLS

#define TYPE_ARIO_COVER_PREFERENCES         (ario_cover_preferences_get_type ())
#define ARIO_COVER_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_COVER_PREFERENCES, ArioCoverPreferences))
#define ARIO_COVER_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_COVER_PREFERENCES, ArioCoverPreferencesClass))
#define IS_ARIO_COVER_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_COVER_PREFERENCES))
#define IS_ARIO_COVER_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_COVER_PREFERENCES))
#define ARIO_COVER_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_COVER_PREFERENCES, ArioCoverPreferencesClass))

typedef struct ArioCoverPreferencesPrivate ArioCoverPreferencesPrivate;

typedef struct
{
        GtkVBox parent;

        ArioCoverPreferencesPrivate *priv;
} ArioCoverPreferences;

typedef struct
{
        GtkVBoxClass parent_class;
} ArioCoverPreferencesClass;

GType              ario_cover_preferences_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_cover_preferences_new              (void);

G_END_DECLS

#endif /* __ARIO_COVER_PREFERENCES_H */
