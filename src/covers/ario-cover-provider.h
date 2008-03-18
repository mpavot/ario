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

#ifndef __ARIO_COVER_PROVIDER_H__
#define __ARIO_COVER_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_COVER_PROVIDER              (ario_cover_provider_get_type())
#define ARIO_COVER_PROVIDER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_COVER_PROVIDER, ArioCoverProvider))
#define ARIO_COVER_PROVIDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_COVER_PROVIDER, ArioCoverProviderClass))
#define ARIO_IS_COVER_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_COVER_PROVIDER))
#define ARIO_IS_COVER_PROVIDER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_COVER_PROVIDER))
#define ARIO_COVER_PROVIDER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_COVER_PROVIDER, ArioCoverProviderClass))

/*
 * Main object structure
 */
typedef struct _ArioCoverProvider 
{
        GObject parent;

        gboolean is_active;
} ArioCoverProvider;

typedef enum
{
        GET_FIRST_COVER,
        GET_ALL_COVERS
}ArioCoverProviderOperation;

/*
 * Class definition
 */
typedef struct 
{
        GObjectClass parent_class;

        /* Virtual public methods */

        gchar*          (*get_id)                       (ArioCoverProvider *cover_provider);

        gchar*          (*get_name)                     (ArioCoverProvider *cover_provider);

        gboolean        (*get_covers)                   (ArioCoverProvider *cover_provider,
                                                         const char *artist,
                                                         const char *album,
                                                         const char *file,
                                                         GArray **file_size,
                                                         GSList **file_contents,
                                                         ArioCoverProviderOperation operation);
} ArioCoverProviderClass;

/*
 * Public methods
 */
GType           ario_cover_provider_get_type            (void) G_GNUC_CONST;

gchar*          ario_cover_provider_get_id              (ArioCoverProvider *cover_provider);

gchar*          ario_cover_provider_get_name            (ArioCoverProvider *cover_provider);

gboolean        ario_cover_provider_get_covers          (ArioCoverProvider *cover_provider,
                                                         const char *artist,
                                                         const char *album,
                                                         const char *file,
                                                         GArray **file_size,
                                                         GSList **file_contents,
                                                         ArioCoverProviderOperation operation);

gboolean        ario_cover_provider_is_active           (ArioCoverProvider *cover_provider);

void            ario_cover_provider_set_active          (ArioCoverProvider *cover_provider,
                                                         gboolean is_active);

G_END_DECLS

#endif  /* __ARIO_COVER_PROVIDER_H__ */


