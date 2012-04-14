/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <gtk/gtk.h>

#ifndef __GTK_BUILDER_HELPERS_H
#define __GTK_BUILDER_HELPERS_H

G_BEGIN_DECLS

GtkBuilder *gtk_builder_helpers_new (const char *file,
                                     gpointer user_data);

void gtk_builder_helpers_boldify_label (GtkBuilder *builder,
                                        const char *name);

G_END_DECLS

#endif /* __GTK_BUILDER_HELPERS_H */
