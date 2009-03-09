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

/* Define if Ario must autoconnect to server or not */
#define PREF_AUTOCONNECT                        "autoconnect"
#define PREF_AUTOCONNECT_DEFAULT                TRUE

/* When enabled, ario hide the covers in the albums treeview */
#define PREF_COVER_TREE_HIDDEN                  "ario_cover_tree_hidden"
#define PREF_COVER_TREE_HIDDEN_DEFAULT          FALSE

/* If enabled, Ario will automaticaly get an album cover when a song is played */
#define PREF_AUTOMATIC_GET_COVER                "automatic_get_cover"
#define PREF_AUTOMATIC_GET_COVER_DEFAULT        TRUE

/* The amazon country to use to get the covers (.com .fr .de .uk or .ca..) */
#define PREF_COVER_AMAZON_COUNTRY               "ario_cover_amazon_country"
#define PREF_COVER_AMAZON_COUNTRY_DEFAULT       "com"

/* Define if Ario must use a proxy for remote connections */
#define PREF_USE_PROXY                          "use_proxy"
#define PREF_USE_PROXY_DEFAULT                  FALSE

/* Define the address of the proxy */
#define PREF_PROXY_ADDRESS                      "proxy_address"
#define PREF_PROXY_ADDRESS_DEFAULT              "192.168.0.1"

/* Define the port of the proxy */
#define PREF_PROXY_PORT                         "proxy_port"
#define PREF_PROXY_PORT_DEFAULT                 8080

/* Define if the view tabs are shown */
#define PREF_SHOW_TABS                          "show_tabs"
#define PREF_SHOW_TABS_DEFAULT                  TRUE

/* define default step for volume adjustment with mouse wheel over volume button and tray icon */
#define PREF_VOL_ADJUST_STEP                    "volume_adjust_step"
#define PREF_VOL_ADJUST_STEP_DEFAULT            5

/* Define the behavior of a double click on the tray icon (0 for 'play/pause', 1 for 'play next song', 2 for 'do nothing') */
#define PREF_TRAYICON_BEHAVIOR                  "trayicon_behavior"
#define PREF_TRAYICON_BEHAVIOR_DEFAULT          0

/* True if the initial assistant has already been run */
#define PREF_FIRST_TIME                         "first_time_flag"
#define PREF_FIRST_TIME_DEFAULT                 FALSE

/* Main window width */
#define PREF_WINDOW_WIDTH                       "window_width"
#define PREF_WINDOW_WIDTH_DEFAULT               600

/* Main window height */
#define PREF_WINDOW_HEIGHT                      "window_height"
#define PREF_WINDOW_HEIGHT_DEFAULT              600

/* If true, the main window is maximized */
#define PREF_WINDOW_MAXIMIZED                   "window_maximized"
#define PREF_WINDOW_MAXIMIZED_DEFAULT           TRUE

/* If true, the statusbar is hidden */
#define PREF_STATUSBAR_HIDDEN                   "statusbar_hidden"
#define PREF_STATUSBAR_HIDDEN_DEFAULT           FALSE

/* If true, the different music sources are hidden */
#define PREF_UPPERPART_HIDDEN                   "upperpart_hidden"
#define PREF_UPPERPART_HIDDEN_DEFAULT           FALSE

/* If true, the playlist is hidden */
#define PREF_PLAYLIST_HIDDEN                    "playlist_hidden"
#define PREF_PLAYLIST_HIDDEN_DEFAULT            FALSE

/* Position of main window pane */
#define PREF_VPANED_POSITION                    "vpaned_position"
#define PREF_VPANED_POSITION_DEFAULT            400

/* Position of playlists pane */
#define PREF_PLAYLISTS_HPANED_SIZE              "playlists_hpaned_position"
#define PREF_PLAYLISTS_HPANED_SIZE_DEFAULT      250

/* Position of filesystem pane */
#define PREF_FILSYSTEM_HPANED_SIZE              "filesystem_hpaned_position"
#define PREF_FILSYSTEM_HPANED_SIZE_DEFAULT      250

/* Define the source used in Ario (0 for the library, 1 for the radios, etc..) */
#define PREF_SOURCE                             "source"
#define PREF_SOURCE_DEFAULT                     0

/* Pixbuf column properties */
#define PREF_PIXBUF_COLUMN_ORDER                "pixbuf_column_order"
#define PREF_PIXBUF_COLUMN_ORDER_DEFAULT        1

/* Track column properties */
#define PREF_TRACK_COLUMN_SIZE                  "track_column_size"
#define PREF_TRACK_COLUMN_SIZE_DEFAULT          50
#define PREF_TRACK_COLUMN_VISIBLE               "track_column_visible"
#define PREF_TRACK_COLUMN_VISIBLE_DEFAULT       TRUE
#define PREF_TRACK_COLUMN_ORDER                 "track_column_order"
#define PREF_TRACK_COLUMN_ORDER_DEFAULT         2

/* Title column properties */
#define PREF_TITLE_COLUMN_SIZE                  "title_column_size"
#define PREF_TITLE_COLUMN_SIZE_DEFAULT          200
#define PREF_TITLE_COLUMN_VISIBLE               "title_column_visible"
#define PREF_TITLE_COLUMN_VISIBLE_DEFAULT       TRUE
#define PREF_TITLE_COLUMN_ORDER                 "title_column_order"
#define PREF_TITLE_COLUMN_ORDER_DEFAULT         3

/* Artist column properties */
#define PREF_ARTIST_COLUMN_SIZE                 "artist_column_size"
#define PREF_ARTIST_COLUMN_SIZE_DEFAULT         200
#define PREF_ARTIST_COLUMN_VISIBLE              "artist_column_visible"
#define PREF_ARTIST_COLUMN_VISIBLE_DEFAULT      TRUE
#define PREF_ARTIST_COLUMN_ORDER                "artist_column_order"
#define PREF_ARTIST_COLUMN_ORDER_DEFAULT        4

/* Album column properties */
#define PREF_ALBUM_COLUMN_SIZE                  "album_column_size"
#define PREF_ALBUM_COLUMN_SIZE_DEFAULT          200
#define PREF_ALBUM_COLUMN_VISIBLE               "album_column_visible"
#define PREF_ALBUM_COLUMN_VISIBLE_DEFAULT       TRUE
#define PREF_ALBUM_COLUMN_ORDER                 "album_column_order"
#define PREF_ALBUM_COLUMN_ORDER_DEFAULT         5

/* Duration column properties */
#define PREF_DURATION_COLUMN_SIZE               "duration_column_size"
#define PREF_DURATION_COLUMN_SIZE_DEFAULT       50
#define PREF_DURATION_COLUMN_VISIBLE            "duration_column_visible"
#define PREF_DURATION_COLUMN_VISIBLE_DEFAULT    TRUE
#define PREF_DURATION_COLUMN_ORDER              "duration_column_order"
#define PREF_DURATION_COLUMN_ORDER_DEFAULT      6

/* File column properties */
#define PREF_FILE_COLUMN_SIZE                   "file_column_size"
#define PREF_FILE_COLUMN_SIZE_DEFAULT           200
#define PREF_FILE_COLUMN_VISIBLE                "file_column_visible"
#define PREF_FILE_COLUMN_VISIBLE_DEFAULT        FALSE
#define PREF_FILE_COLUMN_ORDER                  "file_column_order"
#define PREF_FILE_COLUMN_ORDER_DEFAULT          7

/* Genre column properties */
#define PREF_GENRE_COLUMN_SIZE                  "genre_column_size"
#define PREF_GENRE_COLUMN_SIZE_DEFAULT          200
#define PREF_GENRE_COLUMN_VISIBLE               "genre_column_visible"
#define PREF_GENRE_COLUMN_VISIBLE_DEFAULT       FALSE
#define PREF_GENRE_COLUMN_ORDER                 "genre_column_order"
#define PREF_GENRE_COLUMN_ORDER_DEFAULT         8

/* Date column properties */
#define PREF_DATE_COLUMN_SIZE                   "date_column_size"
#define PREF_DATE_COLUMN_SIZE_DEFAULT           70
#define PREF_DATE_COLUMN_VISIBLE                "date_column_visible"
#define PREF_DATE_COLUMN_VISIBLE_DEFAULT        FALSE
#define PREF_DATE_COLUMN_ORDER                  "date_column_order"
#define PREF_DATE_COLUMN_ORDER_DEFAULT          9

/* List of active plugins. It contains the "Location" of the active plugins. See the .ario-plugin file for obtaining the "Location" of a given plugin. */
#define PREF_PLUGINS_LIST                       "active-plugins"
#define PREF_PLUGINS_LIST_DEFAULT               "filesystem,radios,wikipedia"

/* Ordered list of sources */
#define PREF_SOURCE_LIST                        "sources-order"
#define PREF_SOURCE_LIST_DEFAULT                "library,search,radios,storedplaylists,filesystem"

/* Ordered list of covers providers */
#define PREF_COVER_PROVIDERS_LIST               "cover-providers"
#define PREF_COVER_PROVIDERS_LIST_DEFAULT       "local,amazon,lastfm"

/* List of active covers providers */
#define PREF_COVER_ACTIVE_PROVIDERS_LIST         "active-cover-providers"
#define PREF_COVER_ACTIVE_PROVIDERS_LIST_DEFAULT "local,amazon,lastfm"

/* Ordered list of lyrics providers */
#define PREF_LYRICS_PROVIDERS_LIST              "lyrics-providers"
#define PREF_LYRICS_PROVIDERS_LIST_DEFAULT      "lyricwiki,leoslyrics"

/* List of active lyrics providers */
#define PREF_LYRICS_ACTIVE_PROVIDERS_LIST         "active-lyrics-providers"
#define PREF_LYRICS_ACTIVE_PROVIDERS_LIST_DEFAULT "lyricwiki,leoslyrics"

/* If true, Ario windows is hidden when closed */
#define PREF_HIDE_ON_CLOSE                      "hide-on-close"
#define PREF_HIDE_ON_CLOSE_DEFAULT              FALSE

/* Define how albums are sorted (alphabetically, by year, ...) */
#define PREF_ALBUM_SORT                         "album-sort"
#define PREF_ALBUM_SORT_DEFAULT                 SORT_ALPHABETICALLY

/* If true, the playlist automaticaly scrolls to the playing song */
#define PREF_PLAYLIST_AUTOSCROLL                "playlist_autoscroll"
#define PREF_PLAYLIST_AUTOSCROLL_DEFAULT        FALSE

/* If true, a notification window appears when the song changes */
#define PREF_HAVE_NOTIFICATION                  "notification"
#define PREF_HAVE_NOTIFICATION_DEFAULT          FALSE

/* The notifier id */
#define PREF_NOTIFIER                           "notifier"
#define PREF_NOTIFIER_DEFAULT                   "tooltip"

/* The duration of the notification (in seconds) */
#define PREF_NOTIFICATION_TIME                  "notification-time"
#define PREF_NOTIFICATION_TIME_DEFAULT          5

/* If true, Ario can have only one instance launched at the same time */
#define PREF_ONE_INSTANCE                       "one-instance"
#define PREF_ONE_INSTANCE_DEFAULT               TRUE

/* If true, the tray icon is showed */
#define PREF_TRAY_ICON                          "tray-icon"
#define PREF_TRAY_ICON_DEFAULT                  TRUE

/* The different lists used in the library browser (0 for artist, 1 for album, 2 for title...) */
#define PREF_BROWSER_TREES                      "browser-trees"
#define PREF_BROWSER_TREES_DEFAULT              "0,1,2"

/* If true, Ario asks to SERVER to update the database when it is started */
#define PREF_UPDATE_STARTUP                     "update-startup"
#define PREF_UPDATE_STARTUP_DEFAULT              FALSE

/* If true, Ario stops the music on exit */
#define PREF_STOP_EXIT                          "stop-exit"
#define PREF_STOP_EXIT_DEFAULT                  FALSE

/* Playlist Mode */
#define PREF_PLAYLIST_MODE                     "playlist-mode"
#define PREF_PLAYLIST_MODE_DEFAULT             "normal"

/* Number of songs added to the playlist in dynamic mode*/
#define PREF_DYNAMIC_NBITEMS                   "dynamic-nbitems"
#define PREF_DYNAMIC_NBITEMS_DEFAULT           10

/* Type of songs added to the playlist in dynamic mode*/
#define PREF_DYNAMIC_TYPE                      "dynamic-type"
#define PREF_DYNAMIC_TYPE_DEFAULT              0

enum
{
        TRAY_ICON_PLAY_PAUSE,
        TRAY_ICON_NEXT_SONG,
        TRAY_ICON_MUTE,
        TRAY_ICON_DO_NOTHING,
        TRAY_ICON_N_BEHAVIOR
};

enum
{
        SORT_ALPHABETICALLY,
        SORT_YEAR,
        SORT_N_BEHAVIOR
};

#endif /* __ARIO_PREFERENCES_H */
