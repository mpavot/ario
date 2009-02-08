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

#include "gtk-builder-helpers.h"
#include <gtk/gtk.h>

GtkBuilder *
gtk_builder_helpers_new (const char *file,
                         gpointer user_data)
{
        GtkBuilder *builder;

        builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, file, NULL);
        gtk_builder_connect_signals (builder, user_data);

        return builder;
}

void
gtk_builder_helpers_boldify_label (GtkBuilder *builder,
                                   const char *name)
{
        GObject *object;

        object = gtk_builder_get_object (builder, name);

        if (!object || !GTK_IS_LABEL (object)) {
                g_warning ("object '%s' not found", name);
                return;
        }

        /* this way is probably better, but for some reason doesn't work with
         * labels with mnemonics.*/

        static PangoAttrList *pattrlist = NULL;

        if (pattrlist == NULL) {
                PangoAttribute *attr;

                pattrlist = pango_attr_list_new ();
                attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
                attr->start_index = 0;
                attr->end_index = G_MAXINT;
                pango_attr_list_insert (pattrlist, attr);
        }
        gtk_label_set_attributes (GTK_LABEL (object), pattrlist);

        /*
           gchar *str_final;
           str_final = g_strdup_printf ("<b>%s</b>", gtk_label_get_label (GTK_LABEL (object)));
           gtk_label_set_markup_with_mnemonic (GTK_LABEL (object), str_final);
           g_free (str_final);
           */
}
