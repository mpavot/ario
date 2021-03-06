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

#ifndef __ARIO_SEARCH_H
#define __ARIO_SEARCH_H

#include <gtk/gtk.h>
#include <config.h>
#include "sources/ario-source.h"

#ifdef ENABLE_SEARCH

G_BEGIN_DECLS

#define TYPE_ARIO_SEARCH         (ario_search_get_type ())
#define ARIO_SEARCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SEARCH, ArioSearch))
#define ARIO_SEARCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SEARCH, ArioSearchClass))
#define IS_ARIO_SEARCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SEARCH))
#define IS_ARIO_SEARCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SEARCH))
#define ARIO_SEARCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SEARCH, ArioSearchClass))

typedef struct ArioSearchPrivate ArioSearchPrivate;

/**
 * ArioSearch is a ArioSource that enable user to search
 * for songs in music library by defining several search
 * criteria and to interact with playlist
 */
typedef struct
{
        ArioSource parent;

        ArioSearchPrivate *priv;
} ArioSearch;

typedef struct
{
        ArioSourceClass parent;
} ArioSearchClass;

GType                   ario_search_get_type   (void) G_GNUC_CONST;

GtkWidget*              ario_search_new        (void);

G_END_DECLS

#endif  /* ENABLE_SEARCH */

#endif /* __ARIO_SEARCH_H */
