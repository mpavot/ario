/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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

#include "playlist/ario-playlist-mode.h"
#include <config.h>
#include "ario-debug.h"

static void ario_playlist_mode_class_init (ArioPlaylistModeClass *klass);
static void ario_playlist_mode_init (ArioPlaylistMode *playlist_mode);

#define ARIO_PLAYLIST_MODE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_PLAYLIST_MODE, ArioPlaylistModePrivate))
G_DEFINE_TYPE (ArioPlaylistMode, ario_playlist_mode, G_TYPE_OBJECT)

static void
dummy_void (ArioPlaylistMode *playlist_mode,
            ArioPlaylist *playlist)
{
}

static GtkWidget*
dummy_widget (ArioPlaylistMode *playlist_mode)
{
        return NULL;
}

static gchar *
dummy_char (ArioPlaylistMode *playlist_mode)
{
        return NULL;
}

static void
ario_playlist_mode_class_init (ArioPlaylistModeClass *klass)
{
        klass->next_song = dummy_void;
        klass->last_song = dummy_void;
        klass->get_config = dummy_widget;
        klass->get_id = dummy_char;
        klass->get_name = dummy_char;
}

static void
ario_playlist_mode_init (ArioPlaylistMode *playlist_mode)
{
}

void
ario_playlist_mode_next_song (ArioPlaylistMode *playlist_mode,
                              ArioPlaylist *playlist)
{
        g_return_if_fail (ARIO_IS_PLAYLIST_MODE (playlist_mode));

        ARIO_PLAYLIST_MODE_GET_CLASS (playlist_mode)->next_song (playlist_mode, playlist);
}

void
ario_playlist_mode_last_song (ArioPlaylistMode *playlist_mode,
                              ArioPlaylist *playlist)
{
        g_return_if_fail (ARIO_IS_PLAYLIST_MODE (playlist_mode));

        ARIO_PLAYLIST_MODE_GET_CLASS (playlist_mode)->last_song (playlist_mode, playlist);
}

GtkWidget*
ario_playlist_mode_get_config (ArioPlaylistMode *playlist_mode)
{
        g_return_val_if_fail (ARIO_IS_PLAYLIST_MODE (playlist_mode), NULL);

        return ARIO_PLAYLIST_MODE_GET_CLASS (playlist_mode)->get_config (playlist_mode);
}

gchar *
ario_playlist_mode_get_id (ArioPlaylistMode *playlist_mode)
{
        g_return_val_if_fail (ARIO_IS_PLAYLIST_MODE (playlist_mode), FALSE);

        return ARIO_PLAYLIST_MODE_GET_CLASS (playlist_mode)->get_id (playlist_mode);
}

gchar *
ario_playlist_mode_get_name (ArioPlaylistMode *playlist_mode)
{
        g_return_val_if_fail (ARIO_IS_PLAYLIST_MODE (playlist_mode), FALSE);

        return ARIO_PLAYLIST_MODE_GET_CLASS (playlist_mode)->get_name (playlist_mode);
}
