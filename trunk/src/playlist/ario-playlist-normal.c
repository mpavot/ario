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

#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "playlist/ario-playlist-normal.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_playlist_normal_class_init (ArioPlaylistNormalClass *klass);
static void ario_playlist_normal_init (ArioPlaylistNormal *playlist_normal);

static GObjectClass *parent_class = NULL;

#define ARIO_PLAYLIST_NORMAL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PLAYLIST_NORMAL, ArioPlaylistNormalPrivate))
G_DEFINE_TYPE (ArioPlaylistNormal, ario_playlist_normal, ARIO_TYPE_PLAYLIST_MODE)

static gchar *
ario_playlist_normal_get_id (ArioPlaylistMode *playlist_mode)
{
        return "normal";
}

static gchar *
ario_playlist_normal_get_name (ArioPlaylistMode *playlist_mode)
{
        return _("Normal");
}

static void
ario_playlist_normal_class_init (ArioPlaylistNormalClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistModeClass *playlist_mode_class = ARIO_PLAYLIST_MODE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        playlist_mode_class->get_id = ario_playlist_normal_get_id;
        playlist_mode_class->get_name = ario_playlist_normal_get_name;
}

static void
ario_playlist_normal_init (ArioPlaylistNormal *playlist_normal)
{
        ARIO_LOG_FUNCTION_START;
}

ArioPlaylistMode*
ario_playlist_normal_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioPlaylistNormal *normal;

        normal = g_object_new (TYPE_ARIO_PLAYLIST_NORMAL,
                               NULL);

        return ARIO_PLAYLIST_MODE (normal);
}

