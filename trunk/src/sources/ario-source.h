/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_SOURCE_H__
#define __ARIO_SOURCE_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_SOURCE              (ario_source_get_type())
#define ARIO_SOURCE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_SOURCE, ArioSource))
#define ARIO_SOURCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_SOURCE, ArioSourceClass))
#define ARIO_IS_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_SOURCE))
#define ARIO_IS_SOURCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_SOURCE))
#define ARIO_SOURCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_SOURCE, ArioSourceClass))

/*
 * Main object structure
 */
typedef struct _ArioSource 
{
        GtkHBox parent;
} ArioSource;

/*
 * Class definition
 */
typedef struct 
{
        GtkHBoxClass parent_class;

        /* Virtual public methods */
       
        gchar*          (*get_id)                       (ArioSource *source);
 
        gchar*          (*get_name)                     (ArioSource *source);

        gchar*          (*get_icon)                     (ArioSource *source);

        void            (*shutdown)                     (ArioSource *source);
} ArioSourceClass;

/*
 * Public methods
 */
GType           ario_source_get_type            (void) G_GNUC_CONST;

gchar*          ario_source_get_id              (ArioSource *source);

gchar*          ario_source_get_name            (ArioSource *source);

gchar*          ario_source_get_icon            (ArioSource *source);

void            ario_source_shutdown            (ArioSource *source);

G_END_DECLS

#endif  /* __ARIO_SOURCE_H__ */


