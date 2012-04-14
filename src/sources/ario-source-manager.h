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

#ifndef __ARIO_SOURCE_MANAGER_H
#define __ARIO_SOURCE_MANAGER_H

#include <gtk/gtk.h>
#include "sources/ario-source.h"

G_BEGIN_DECLS

typedef enum
{
        ARIO_SOURCE_BROWSER,
        ARIO_SOURCE_RADIO,
        ARIO_SOURCE_SEARCH,
        ARIO_SOURCE_PLAYLISTS,
        ARIO_SOURCE_FILESYSTEM
}ArioSourceType;

#define ARIO_TYPE_SOURCE_MANAGER         (ario_source_manager_get_type ())
#define ARIO_SOURCE_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_SOURCE_MANAGER, ArioSourceManager))
#define ARIO_SOURCE_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_SOURCE_MANAGER, ArioSourceManagerClass))
#define ARIO_IS_SOURCE_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_SOURCE_MANAGER))
#define ARIO_IS_SOURCE_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_SOURCE_MANAGER))
#define ARIO_SOURCE_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_SOURCE_MANAGER, ArioSourceManagerClass))

typedef struct ArioSourceManagerPrivate ArioSourceManagerPrivate;

/**
 * ArioSourceManager is a widget used to display,
 * reorder, activate, deactivate the different
 * ArioSource
 */
typedef struct
{
        GtkNotebook parent;

        ArioSourceManagerPrivate *priv;
} ArioSourceManager;

typedef struct
{
        GtkNotebookClass parent;

} ArioSourceManagerClass;

GType                   ario_source_manager_get_type     (void) G_GNUC_CONST;

GtkWidget*              ario_source_manager_get_instance (GtkUIManager *mgr,
                                                          GtkActionGroup *group);

void                    ario_source_manager_append       (ArioSource *source);
void                    ario_source_manager_remove       (ArioSource *source);
void                    ario_source_manager_reorder      (void);
void                    ario_source_manager_shutdown     (void);
void                    ario_source_manager_goto_playling_song (void);

G_END_DECLS

#endif /* __ARIO_SOURCE_MANAGER_H */
