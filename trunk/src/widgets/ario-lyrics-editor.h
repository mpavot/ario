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

#ifndef __ARIO_LYRICS_EDITOR_H
#define __ARIO_LYRICS_EDITOR_H

G_BEGIN_DECLS

#define TYPE_ARIO_LYRICS_EDITOR         (ario_lyrics_editor_get_type ())
#define ARIO_LYRICS_EDITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_LYRICS_EDITOR, ArioLyricsEditor))
#define ARIO_LYRICS_EDITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_LYRICS_EDITOR, ArioLyricsEditorClass))
#define IS_ARIO_LYRICS_EDITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_LYRICS_EDITOR))
#define IS_ARIO_LYRICS_EDITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_LYRICS_EDITOR))
#define ARIO_LYRICS_EDITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_LYRICS_EDITOR, ArioLyricsEditorClass))

typedef struct ArioLyricsEditorPrivate ArioLyricsEditorPrivate;

typedef struct
{
        GtkVBox parent;

        ArioLyricsEditorPrivate *priv;
} ArioLyricsEditor;

typedef struct
{
        GtkVBoxClass parent_class;
} ArioLyricsEditorClass;

typedef struct ArioLyricsEditorData
{
        gchar *artist;
        gchar *title;
        gchar *hid;
        gboolean finalize;
} ArioLyricsEditorData;

GType              ario_lyrics_editor_get_type         (void) G_GNUC_CONST;

GtkWidget *        ario_lyrics_editor_new              (void);

void               ario_lyrics_editor_push             (ArioLyricsEditor *lyrics_editor,
                                                        ArioLyricsEditorData *data);

G_END_DECLS

#endif /* __ARIO_LYRICS_EDITOR_H */
