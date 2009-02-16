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

#include <gtk/gtkwindow.h>

#ifndef __ARIO_SHELL_SIMILARARTISTS_H
#define __ARIO_SHELL_SIMILARARTISTS_H

G_BEGIN_DECLS

#define TYPE_ARIO_SHELL_SIMILARARTISTS         (ario_shell_similarartists_get_type ())
#define ARIO_SHELL_SIMILARARTISTS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SHELL_SIMILARARTISTS, ArioShellSimilarartists))
#define ARIO_SHELL_SIMILARARTISTS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SHELL_SIMILARARTISTS, ArioShellSimilarartistsClass))
#define IS_ARIO_SHELL_SIMILARARTISTS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SHELL_SIMILARARTISTS))
#define IS_ARIO_SHELL_SIMILARARTISTS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SHELL_SIMILARARTISTS))
#define ARIO_SHELL_SIMILARARTISTS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SHELL_SIMILARARTISTS, ArioShellSimilarartistsClass))

typedef struct ArioShellSimilarartistsPrivate ArioShellSimilarartistsPrivate;

typedef struct
{
        GtkWindow parent;

        ArioShellSimilarartistsPrivate *priv;
} ArioShellSimilarartists;

typedef struct
{
        GtkWindowClass parent_class;
} ArioShellSimilarartistsClass;

typedef struct
{
        guchar *name;
        guchar *image;
        guchar *url;
} ArioSimilarArtist;

GType              ario_shell_similarartists_get_type                   (void) G_GNUC_CONST;

GtkWidget *        ario_shell_similarartists_new                        (void);

GSList *           ario_shell_similarartists_get_similar_artists        (const gchar *artist);

void               ario_shell_similarartists_add_similar_to_playlist    (const gchar *artist,
                                                                         const int nb_entries);
void               ario_shell_similarartists_free_similarartist         (ArioSimilarArtist *similar_artist);

G_END_DECLS

#endif /* __ARIO_SHELL_SIMILARARTISTS_H */
