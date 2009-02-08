/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_PLAYLIST_PREFERENCES_H
#define __ARIO_PLAYLIST_PREFERENCES_H

#include <glib.h>
#include <gtk/gtkvbox.h>

G_BEGIN_DECLS

#define TYPE_ARIO_PLAYLIST_PREFERENCES         (ario_playlist_preferences_get_type ())
#define ARIO_PLAYLIST_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_PLAYLIST_PREFERENCES, ArioPlaylistPreferences))
#define ARIO_PLAYLIST_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_PLAYLIST_PREFERENCES, ArioPlaylistPreferencesClass))
#define IS_ARIO_PLAYLIST_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_PLAYLIST_PREFERENCES))
#define IS_ARIO_PLAYLIST_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_PLAYLIST_PREFERENCES))
#define ARIO_PLAYLIST_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_PLAYLIST_PREFERENCES, ArioPlaylistPreferencesClass))

typedef struct ArioPlaylistPreferencesPrivate ArioPlaylistPreferencesPrivate;

typedef struct
{
        GtkVBox parent;

        ArioPlaylistPreferencesPrivate *priv;
} ArioPlaylistPreferences;

typedef struct
{
        GtkVBoxClass parent_class;
} ArioPlaylistPreferencesClass;

GType              ario_playlist_preferences_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_playlist_preferences_new              (void);

G_END_DECLS

#endif /* __ARIO_PLAYLIST_PREFERENCES_H */
