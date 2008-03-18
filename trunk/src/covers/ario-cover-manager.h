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

#ifndef __ARIO_COVER_MANAGER_H
#define __ARIO_COVER_MANAGER_H

#include <glib-object.h>
#include "covers/ario-cover-provider.h"

G_BEGIN_DECLS

#define TYPE_ARIO_COVER_MANAGER         (ario_cover_manager_get_type ())
#define ARIO_COVER_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_COVER_MANAGER, ArioCoverManager))
#define ARIO_COVER_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_COVER_MANAGER, ArioCoverManagerClass))
#define IS_ARIO_COVER_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_COVER_MANAGER))
#define IS_ARIO_COVER_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_COVER_MANAGER))
#define ARIO_COVER_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_COVER_MANAGER, ArioCoverManagerClass))

typedef struct ArioCoverManagerPrivate ArioCoverManagerPrivate;

typedef struct
{
        GObject parent;

        ArioCoverManagerPrivate *priv;
} ArioCoverManager;

typedef struct
{
        GObjectClass parent;
} ArioCoverManagerClass;

GType                   ario_cover_manager_get_type                     (void);

ArioCoverManager*       ario_cover_manager_get_instance                 (void);

void                    ario_cover_manager_add_provider                 (ArioCoverManager *cover_manager,
                                                                         ArioCoverProvider *cover_provider);
void                    ario_cover_manager_remove_provider              (ArioCoverManager *cover_manager,
                                                                         ArioCoverProvider *cover_provider);
void                    ario_cover_manager_update_providers             (ArioCoverManager *cover_manage);
GSList*                 ario_cover_manager_get_providers                (ArioCoverManager *cover_manager);
void                    ario_cover_manager_set_providers                (ArioCoverManager *cover_manager,
                                                                         GSList *providers);
ArioCoverProvider*      ario_cover_manager_get_provider_from_id         (ArioCoverManager *cover_manager,
                                                                         const gchar *id);
void                    ario_cover_manager_shutdown                     (ArioCoverManager *cover_manager);

gboolean                ario_cover_manager_get_covers                   (ArioCoverManager *cover_manager,
                                                                         const char *artist,
                                                                         const char *album,
                                                                         const char *file,
                                                                         GArray **file_size,
                                                                         GSList **file_contents,
                                                                         ArioCoverProviderOperation operation);
G_END_DECLS

#endif /* __ARIO_COVER_MANAGER_H */
