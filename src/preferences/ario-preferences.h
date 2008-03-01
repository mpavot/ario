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

#ifndef __ARIO_PREFERENCES_H
#define __ARIO_PREFERENCES_H

#define CONF_HOST                       "/apps/ario/host"
#define CONF_PORT                       "/apps/ario/port"
#define CONF_TIMEOUT                    "/apps/ario/timeout"
#define CONF_AUTOCONNECT                "/apps/ario/autoconnect"
#define CONF_PASSWORD                   "/apps/ario/password"
#define CONF_COVER_TREE_HIDDEN          "/apps/ario/ario_cover_tree_hidden"
#define CONF_COVER_AMAZON_COUNTRY       "/apps/ario/ario_cover_amazon_country"
#define CONF_USE_PROXY                  "/apps/ario/use_proxy"
#define CONF_PROXY_ADDRESS              "/apps/ario/proxy_address"
#define CONF_PROXY_PORT                 "/apps/ario/proxy_port"
#define CONF_SHOW_TABS                  "/apps/ario/show_tabs"
#define CONF_TRAYICON_BEHAVIOR          "/apps/ario/trayicon_behavior"
#define CONF_FIRST_TIME                 "/apps/ario/first_time_flag"

#define CONF_WINDOW_WIDTH               "/apps/ario/state/window_width"
#define CONF_WINDOW_HEIGHT              "/apps/ario/state/window_height"
#define CONF_WINDOW_MAXIMIZED           "/apps/ario/state/window_maximized"
#define CONF_STATUSBAR_HIDDEN           "/apps/ario/state/statusbar_hidden"
#define CONF_VPANED_POSITION            "/apps/ario/state/vpaned_position"
#define CONF_PLAYLISTS_HPANED_SIZE      "/apps/ario/state/playlists_hpaned_position"
#define CONF_FILSYSTEM_HPANED_SIZE      "/apps/ario/state/filesystem_hpaned_position"
#define CONF_SOURCE                     "/apps/ario/state/source"
#define CONF_TRACK_COLUMN_SIZE          "/apps/ario/state/track_column_size"
#define CONF_TITLE_COLUMN_SIZE          "/apps/ario/state/title_column_size"
#define CONF_ARTIST_COLUMN_SIZE         "/apps/ario/state/artist_column_size"
#define CONF_ALBUM_COLUMN_SIZE          "/apps/ario/state/album_column_size"
#define CONF_PLUGINS_LIST               "/apps/ario/plugins/active-plugins"
#define CONF_SOURCE_LIST                "/apps/ario/state/sources-order"
#define CONF_MUSIC_DIR                  "/apps/ario/music-dir"

enum
{
        TRAY_ICON_PLAY_PAUSE,
        TRAY_ICON_NEXT_SONG,
        TRAY_ICON_DO_NOTHING,
        TRAY_ICON_N_BEHAVIOR
};

#endif /* __ARIO_PREFERENCES_H */
