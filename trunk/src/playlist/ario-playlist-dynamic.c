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
#include "playlist/ario-playlist-dynamic.h"
#include "shell/ario-shell-similarartists.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_playlist_dynamic_class_init (ArioPlaylistDynamicClass *klass);
static void ario_playlist_dynamic_init (ArioPlaylistDynamic *playlist_dynamic);
static void ario_playlist_dynamic_finalize (GObject *object);
static void ario_playlist_dynamic_last_song (ArioPlaylistMode *playlist_mode,
                                            ArioPlaylist *playlist);
static GtkWidget* ario_playlist_dynamic_get_config (ArioPlaylistMode *playlist_mode);

struct ArioPlaylistDynamicPrivate
{
        gboolean dummy;
};

static GObjectClass *parent_class = NULL;

GType
ario_playlist_dynamic_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioPlaylistDynamicClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_playlist_dynamic_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioPlaylistDynamic),
                        0,
                        (GInstanceInitFunc) ario_playlist_dynamic_init
                };

                type = g_type_register_static (ARIO_TYPE_PLAYLIST_MODE,
                                               "ArioPlaylistDynamic",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_playlist_dynamic_get_id (ArioPlaylistMode *playlist_mode)
{
        return "dynamic";
}

static gchar *
ario_playlist_dynamic_get_name (ArioPlaylistMode *playlist_mode)
{
        return _("Dynamic Playlist");
}

static void
ario_playlist_dynamic_class_init (ArioPlaylistDynamicClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPlaylistModeClass *playlist_mode_class = ARIO_PLAYLIST_MODE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_playlist_dynamic_finalize;

        playlist_mode_class->get_id = ario_playlist_dynamic_get_id;
        playlist_mode_class->get_name = ario_playlist_dynamic_get_name;
        playlist_mode_class->last_song = ario_playlist_dynamic_last_song;
        playlist_mode_class->get_config = ario_playlist_dynamic_get_config;
}

static void
ario_playlist_dynamic_init (ArioPlaylistDynamic *playlist_dynamic)
{
        ARIO_LOG_FUNCTION_START
        playlist_dynamic->priv = g_new0 (ArioPlaylistDynamicPrivate, 1);
}

static void
ario_playlist_dynamic_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistDynamic *playlist_dynamic;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_PLAYLIST_DYNAMIC (object));

        playlist_dynamic = ARIO_PLAYLIST_DYNAMIC (object);

        g_return_if_fail (playlist_dynamic->priv != NULL);
        g_free (playlist_dynamic->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioPlaylistMode*
ario_playlist_dynamic_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistDynamic *dynamic;

        dynamic = g_object_new (TYPE_ARIO_PLAYLIST_DYNAMIC,
                               NULL);

        g_return_val_if_fail (dynamic->priv != NULL, NULL);

        return ARIO_PLAYLIST_MODE (dynamic);
}

static void
ario_playlist_dynamic_last_song (ArioPlaylistMode *playlist_mode,
                                ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_similarartists_add_similar_to_playlist (ario_server_get_current_artist (),
                                                           10);
}

static GtkWidget*
ario_playlist_dynamic_get_config (ArioPlaylistMode *playlist_mode)
{
        /* TODO */
        return NULL;
}
