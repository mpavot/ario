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

#ifndef __ARIO_LYRICS_PREFERENCES_H
#define __ARIO_LYRICS_PREFERENCES_H

G_BEGIN_DECLS

#define TYPE_ARIO_LYRICS_PREFERENCES         (ario_lyrics_preferences_get_type ())
#define ARIO_LYRICS_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_LYRICS_PREFERENCES, ArioLyricsPreferences))
#define ARIO_LYRICS_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_LYRICS_PREFERENCES, ArioLyricsPreferencesClass))
#define IS_ARIO_LYRICS_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_LYRICS_PREFERENCES))
#define IS_ARIO_LYRICS_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_LYRICS_PREFERENCES))
#define ARIO_LYRICS_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_LYRICS_PREFERENCES, ArioLyricsPreferencesClass))

typedef struct ArioLyricsPreferencesPrivate ArioLyricsPreferencesPrivate;

typedef struct
{
        GtkVBox parent;

        ArioLyricsPreferencesPrivate *priv;
} ArioLyricsPreferences;

typedef struct
{
        GtkDialogClass parent_class;
} ArioLyricsPreferencesClass;

GType              ario_lyrics_preferences_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_lyrics_preferences_new              (void);

G_END_DECLS

#endif /* __ARIO_LYRICS_PREFERENCES_H */
