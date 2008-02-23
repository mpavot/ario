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

#include <config.h>
#include "sources/ario-source.h"

G_DEFINE_TYPE(ArioSource, ario_source, GTK_TYPE_HBOX)

static void
dummy (ArioSource *source)
{
        /* Empty */
}

static gchar *
dummy_char (ArioSource *source)
{
        return NULL;
}

static void 
ario_source_class_init (ArioSourceClass *klass)
{
        klass->get_name = dummy_char;
        klass->get_icon = dummy_char;
        klass->get_id = dummy_char;

        klass->shutdown = dummy;
}

static void
ario_source_init (ArioSource *source)
{
        /* Empty */
}

gchar *
ario_source_get_id (ArioSource *source)
{
        g_return_val_if_fail (ARIO_IS_SOURCE (source), FALSE);

        return ARIO_SOURCE_GET_CLASS (source)->get_id (source);
}

gchar *
ario_source_get_name (ArioSource *source)
{
        g_return_val_if_fail (ARIO_IS_SOURCE (source), FALSE);

        return ARIO_SOURCE_GET_CLASS (source)->get_name (source);
}

gchar *
ario_source_get_icon (ArioSource *source)
{
        g_return_val_if_fail (ARIO_IS_SOURCE (source), FALSE);

        return ARIO_SOURCE_GET_CLASS (source)->get_icon (source);
}

void
ario_source_shutdown (ArioSource *source)
{
        g_return_if_fail (ARIO_IS_SOURCE (source));

        ARIO_SOURCE_GET_CLASS (source)->shutdown (source);
}
