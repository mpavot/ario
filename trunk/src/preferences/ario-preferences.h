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

#define PREF_HOST                               "host"
#define PREF_HOST_DEFAULT                       "localhost"

#define PREF_PORT                               "port"
#define PREF_PORT_DEFAULT                       6600

#define PREF_AUTOCONNECT                        "autoconnect"
#define PREF_AUTOCONNECT_DEFAULT                TRUE

#define PREF_PASSWORD                           "password"
#define PREF_PASSWORD_DEFAULT                   NULL

#define PREF_COVER_TREE_HIDDEN                  "ario_cover_tree_hidden"
#define PREF_COVER_TREE_HIDDEN_DEFAULT          FALSE

#define PREF_AUTOMATIC_GET_COVER                "automatic_get_cover"
#define PREF_AUTOMATIC_GET_COVER_DEFAULT        TRUE

#define PREF_COVER_AMAZON_COUNTRY               "ario_cover_amazon_country"
#define PREF_COVER_AMAZON_COUNTRY_DEFAULT       "com"

#define PREF_USE_PROXY                          "use_proxy"
#define PREF_USE_PROXY_DEFAULT                  FALSE

#define PREF_PROXY_ADDRESS                      "proxy_address"
#define PREF_PROXY_ADDRESS_DEFAULT              "192.168.0.1"

#define PREF_PROXY_PORT                         "proxy_port"
#define PREF_PROXY_PORT_DEFAULT                 8080

#define PREF_SHOW_TABS                          "show_tabs"
#define PREF_SHOW_TABS_DEFAULT                  TRUE

#define PREF_TRAYICON_BEHAVIOR                  "trayicon_behavior"
#define PREF_TRAYICON_BEHAVIOR_DEFAULT          0

#define PREF_FIRST_TIME                         "first_time_flag"
#define PREF_FIRST_TIME_DEFAULT                 FALSE

#define PREF_WINDOW_WIDTH                       "window_width"
#define PREF_WINDOW_WIDTH_DEFAULT               600

#define PREF_WINDOW_HEIGHT                      "window_height"
#define PREF_WINDOW_HEIGHT_DEFAULT              600

#define PREF_WINDOW_MAXIMIZED                   "window_maximized"
#define PREF_WINDOW_MAXIMIZED_DEFAULT           TRUE

#define PREF_STATUSBAR_HIDDEN                   "statusbar_hidden"
#define PREF_STATUSBAR_HIDDEN_DEFAULT           FALSE

#define PREF_VPANED_POSITION                    "vpaned_position"
#define PREF_VPANED_POSITION_DEFAULT            400

#define PREF_PLAYLISTS_HPANED_SIZE              "playlists_hpaned_position"
#define PREF_PLAYLISTS_HPANED_SIZE_DEFAULT      250

#define PREF_FILSYSTEM_HPANED_SIZE              "filesystem_hpaned_position"
#define PREF_FILSYSTEM_HPANED_SIZE_DEFAULT      250

#define PREF_SOURCE                             "source"
#define PREF_SOURCE_DEFAULT                     0

#define PREF_TRACK_COLUMN_SIZE                  "track_column_size"
#define PREF_TRACK_COLUMN_SIZE_DEFAULT          50

#define PREF_TITLE_COLUMN_SIZE                  "title_column_size"
#define PREF_TITLE_COLUMN_SIZE_DEFAULT          200

#define PREF_ARTIST_COLUMN_SIZE                 "artist_column_size"
#define PREF_ARTIST_COLUMN_SIZE_DEFAULT         200

#define PREF_ALBUM_COLUMN_SIZE                  "album_column_size"
#define PREF_ALBUM_COLUMN_SIZE_DEFAULT          200

#define PREF_PLUGINS_LIST                       "active-plugins"
#define PREF_PLUGINS_LIST_DEFAULT               "filesystem,radios,wikipedia"

#define PREF_SOURCE_LIST                        "sources-order"
#define PREF_SOURCE_LIST_DEFAULT                "library,search,radios,storedplaylists,filesystem"

#define PREF_COVER_PROVIDERS_LIST               "cover-providers"
#define PREF_COVER_PROVIDERS_LIST_DEFAULT       "local,amazon,lastfm"

#define PREF_COVER_ACTIVE_PROVIDERS_LIST         "active-cover-providers"
#define PREF_COVER_ACTIVE_PROVIDERS_LIST_DEFAULT "local,amazon,lastfm"

#define PREF_LYRICS_PROVIDERS_LIST              "lyrics-providers"
#define PREF_LYRICS_PROVIDERS_LIST_DEFAULT      "lyricwiki,leoslyrics"

#define PREF_LYRICS_ACTIVE_PROVIDERS_LIST         "active-lyrics-providers"
#define PREF_LYRICS_ACTIVE_PROVIDERS_LIST_DEFAULT "lyricwiki,leoslyrics"

#define PREF_MUSIC_DIR                          "music-dir"
#define PREF_MUSIC_DIR_DEFAULT                  NULL

enum
{
        TRAY_ICON_PLAY_PAUSE,
        TRAY_ICON_NEXT_SONG,
        TRAY_ICON_DO_NOTHING,
        TRAY_ICON_N_BEHAVIOR
};

#endif /* __ARIO_PREFERENCES_H */
