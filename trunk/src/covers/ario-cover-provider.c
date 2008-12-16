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

#include "covers/ario-cover-provider.h"
#include <config.h>
#include "ario-debug.h"

G_DEFINE_TYPE (ArioCoverProvider, ario_cover_provider, G_TYPE_OBJECT)

static gboolean
dummy_boolean (ArioCoverProvider *cover_provider,
               const char *artist,
               const char *album,
               const char *file,
               GArray **file_size,
               GSList **file_contents,
               ArioCoverProviderOperation operation)
{
        return FALSE;
}

static gchar *
dummy_char (ArioCoverProvider *cover_provider)
{
        return NULL;
}

static void 
ario_cover_provider_class_init (ArioCoverProviderClass *klass)
{
        klass->get_id = dummy_char;
        klass->get_name = dummy_char;
        klass->get_covers = dummy_boolean;
}

static void
ario_cover_provider_init (ArioCoverProvider *cover_provider)
{
        cover_provider->is_active = FALSE;
}

gchar *
ario_cover_provider_get_id (ArioCoverProvider *cover_provider)
{
        g_return_val_if_fail (ARIO_IS_COVER_PROVIDER (cover_provider), FALSE);

        return ARIO_COVER_PROVIDER_GET_CLASS (cover_provider)->get_id (cover_provider);
}

gchar *
ario_cover_provider_get_name (ArioCoverProvider *cover_provider)
{
        g_return_val_if_fail (ARIO_IS_COVER_PROVIDER (cover_provider), FALSE);

        return ARIO_COVER_PROVIDER_GET_CLASS (cover_provider)->get_name (cover_provider);
}

gboolean
ario_cover_provider_get_covers (ArioCoverProvider *cover_provider,
                                const char *artist,
                                const char *album,
                                const char *file,
                                GArray **file_size,
                                GSList **file_contents,
                                ArioCoverProviderOperation operation)
{
        g_return_val_if_fail (ARIO_IS_COVER_PROVIDER (cover_provider), FALSE);

        return ARIO_COVER_PROVIDER_GET_CLASS (cover_provider)->get_covers (cover_provider,
                                                                           artist, album,
                                                                           file,
                                                                           file_size, file_contents,
                                                                           operation);
}


gboolean
ario_cover_provider_is_active (ArioCoverProvider *cover_provider)
{
        ARIO_LOG_FUNCTION_START
        return cover_provider->is_active;
}

void
ario_cover_provider_set_active (ArioCoverProvider *cover_provider,
                                gboolean is_active)
{
        ARIO_LOG_FUNCTION_START
        cover_provider->is_active = is_active;
}

