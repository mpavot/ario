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

#ifndef __ARIO_SERVER_H
#define __ARIO_SERVER_H

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define ARIO_SERVER_UNKNOWN     _("Unknown")

#define ARIO_TYPE_SERVER         (ario_server_get_type ())
#define ARIO_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_SERVER, ArioServer))
#define ARIO_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_SERVER, ArioServerClass))
#define IS_ARIO_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_SERVER))
#define IS_ARIO_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_SERVER))
#define ARIO_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_SERVER, ArioServerClass))

typedef struct {
        /* filename of song */
        char * file;
        /* artist, maybe NULL if there is no tag */
        char * artist;
        /* title, maybe NULL if there is no tag */
        char * title;
        /* album, maybe NULL if there is no tag */
        char * album;
        /* album artist, maybe NULL if there is no tag */
        char * album_artist;
        /* track, maybe NULL if there is no tag */
        char * track;
        /* name, maybe NULL if there is no tag; it's the name of the current
         * song, f.e. the icyName of the stream */
        char * name;
        /* date */
        char *date;
        /* Genre */
        char *genre;
        /* Composer */
        char *composer;
        /* Performer */
        char *performer;
        /* Disc */
        char *disc;
        /* Comment */
        char *comment;
        /* length of song in seconds */
        int time;
        /* if plchanges/playlistinfo/playlistid used, is the position of the
         * song in the playlist */
        int pos;
        /* song id for a song in the playlist */
        int id;
} ArioServerSong;

typedef struct {
        int numberOfArtists;
        int numberOfAlbums;
        int numberOfSongs;
        unsigned long uptime;
        unsigned long dbUpdateTime;
        unsigned long playTime;
        unsigned long dbPlayTime;
} ArioServerStats;

typedef struct {
        int id;
        char * name;
        int enabled;
} ArioServerOutput;

typedef enum {
        /** no information available */
        ARIO_STATE_UNKNOWN = 0,

        /** not playing */
        ARIO_STATE_STOP = 1,

        /** playing */
        ARIO_STATE_PLAY = 2,

        /** playing, but paused */
        ARIO_STATE_PAUSE = 3,
} ArioServerState;

typedef enum
{
        ARIO_TAG_ARTIST,
        ARIO_TAG_ALBUM,
        ARIO_TAG_ALBUM_ARTIST,
        ARIO_TAG_TITLE,
        ARIO_TAG_TRACK,
        ARIO_TAG_NAME,
        ARIO_TAG_GENRE,
        ARIO_TAG_DATE,
        ARIO_TAG_COMPOSER,
        ARIO_TAG_PERFORMER,
        ARIO_TAG_COMMENT,
        ARIO_TAG_DISC,
        ARIO_TAG_FILENAME,
        ARIO_TAG_ANY,
        ARIO_TAG_COUNT
}ArioServerTag;

typedef struct
{
        gchar *artist;
        gchar *album;
        gchar *path;
        gchar *date;
} ArioServerAlbum;

typedef struct
{
        GSList *directories;
        GSList *songs;
} ArioServerFileList;

typedef struct
{
        ArioServerTag tag;
        gchar *value;
} ArioServerAtomicCriteria;

typedef GSList ArioServerCriteria; /* A criteria is a list of atomic criterias */

typedef enum
{
        ArioServerMpd,
        ArioServerXmms
} ArioServerType;

typedef enum
{
        ARIO_SERVER_ACTION_ADD,
        ARIO_SERVER_ACTION_DELETE_ID,
        ARIO_SERVER_ACTION_DELETE_POS,
        ARIO_SERVER_ACTION_MOVE,
        ARIO_SERVER_ACTION_MOVEID
}ArioServerActionType;

typedef struct ArioServerQueueAction {
        ArioServerActionType type;
        union {
                const char *path;       // For ARIO_SERVER_ACTION_ADD
                int id;                 // For ARIO_SERVER_ACTION_DELETE_ID
                int pos;                // For ARIO_SERVER_ACTION_DELETE_POS
                struct {                // For ARIO_SERVER_ACTION_MOVE and ARIO_SERVER_ACTION_MOVEID
                        int old_pos;
                        int new_pos;
                };
        };
} ArioServerQueueAction;

enum
{
        SERVER_SONG_CHANGED,
        SERVER_ALBUM_CHANGED,
        SERVER_CONNECTIVITY_CHANGED,
        SERVER_STATE_CHANGED,
        SERVER_VOLUME_CHANGED,
        SERVER_ELAPSED_CHANGED,
        SERVER_PLAYLIST_CHANGED,
        SERVER_CONSUME_CHANGED,
        SERVER_RANDOM_CHANGED,
        SERVER_REPEAT_CHANGED,
        SERVER_UPDATINGDB_CHANGED,
        SERVER_STOREDPLAYLISTS_CHANGED,
        SERVER_LAST_SIGNAL
};

enum
{
        SERVER_SONG_CHANGED_FLAG = 2 << SERVER_SONG_CHANGED,
        SERVER_ALBUM_CHANGED_FLAG = 2 << SERVER_ALBUM_CHANGED,
        SERVER_CONNECTIVITY_CHANGED_FLAG = 2 << SERVER_CONNECTIVITY_CHANGED,
        SERVER_STATE_CHANGED_FLAG = 2 << SERVER_STATE_CHANGED,
        SERVER_VOLUME_CHANGED_FLAG = 2 << SERVER_VOLUME_CHANGED,
        SERVER_ELAPSED_CHANGED_FLAG = 2 << SERVER_ELAPSED_CHANGED,
        SERVER_PLAYLIST_CHANGED_FLAG = 2 << SERVER_PLAYLIST_CHANGED,
        SERVER_CONSUME_CHANGED_FLAG = 2 << SERVER_CONSUME_CHANGED,
        SERVER_RANDOM_CHANGED_FLAG = 2 << SERVER_RANDOM_CHANGED,
        SERVER_REPEAT_CHANGED_FLAG = 2 << SERVER_REPEAT_CHANGED,
        SERVER_UPDATINGDB_CHANGED_FLAG = 2 << SERVER_UPDATINGDB_CHANGED,
        SERVER_STOREDPLAYLISTS_CHANGED_FLAG = 2 << SERVER_STOREDPLAYLISTS_CHANGED
};

typedef enum
{
        PLAYLIST_ADD,
        PLAYLIST_ADD_PLAY,
        PLAYLIST_REPLACE,
        PLAYLIST_ADD_AFTER_PLAYING,
        PLAYLIST_N_BEHAVIOR
} PlaylistAction;

typedef struct
{
        GObject parent;
} ArioServer;

typedef struct
{
        GObjectClass parent;

        /* Signals */
        void (*song_changed)            (ArioServer *server);

        void (*album_changed)           (ArioServer *server);

        void (*connectivity_changed)    (ArioServer *server);

        void (*state_changed)           (ArioServer *server);

        void (*volume_changed)          (ArioServer *server,
                                         int volume);
        void (*elapsed_changed)         (ArioServer *server,
                                         int elapsed);
        void (*playlist_changed)        (ArioServer *server);

        void (*consume_changed)         (ArioServer *server);

        void (*random_changed)          (ArioServer *server);

        void (*repeat_changed)          (ArioServer *server);

        void (*updatingdb_changed)      (ArioServer *server);

        void (*storedplaylists_changed) (ArioServer *server);
} ArioServerClass;
G_MODULE_EXPORT
GType                   ario_server_get_type                               (void) G_GNUC_CONST;
G_MODULE_EXPORT
ArioServer *            ario_server_get_instance                           (void);
G_MODULE_EXPORT
gboolean                ario_server_connect                                (void);
G_MODULE_EXPORT
void                    ario_server_disconnect                             (void);
G_MODULE_EXPORT
void                    ario_server_reconnect                              (void);
G_MODULE_EXPORT
void                    ario_server_shutdown                               (void);
G_MODULE_EXPORT
gboolean                ario_server_is_connected                           (void);
G_MODULE_EXPORT
gboolean                ario_server_update_status                          (void);
G_MODULE_EXPORT
void                    ario_server_update_db                              (const gchar *path);
G_MODULE_EXPORT
GSList *                ario_server_list_tags                              (const ArioServerTag tag,
                                                                            const ArioServerCriteria *criteria);
G_MODULE_EXPORT
GSList *                ario_server_get_albums                             (const ArioServerCriteria *criteria);
G_MODULE_EXPORT
GSList *                ario_server_get_songs                              (const ArioServerCriteria *criteria,
                                                                            const gboolean exact);
G_MODULE_EXPORT
GSList *                ario_server_get_songs_from_playlist                (char *playlist);
G_MODULE_EXPORT
GSList *                ario_server_get_playlists                          (void);
G_MODULE_EXPORT
GSList *                ario_server_get_playlist_changes                   (gint64 playlist_id);
G_MODULE_EXPORT
ArioServerSong *        ario_server_get_current_song_on_server             (void);
G_MODULE_EXPORT
ArioServerSong *        ario_server_get_current_song                       (void);
G_MODULE_EXPORT
char *                  ario_server_get_current_artist                     (void);
G_MODULE_EXPORT
char *                  ario_server_get_current_album                      (void);
G_MODULE_EXPORT
char *                  ario_server_get_current_song_path                  (void);
G_MODULE_EXPORT
int                     ario_server_get_current_song_id                    (void);
G_MODULE_EXPORT
int                     ario_server_get_current_state                      (void);
G_MODULE_EXPORT
int                     ario_server_get_current_elapsed                    (void);
G_MODULE_EXPORT
int                     ario_server_get_current_volume                     (void);
G_MODULE_EXPORT
int                     ario_server_get_current_total_time                 (void);
G_MODULE_EXPORT
gint64                  ario_server_get_current_playlist_id                (void);
G_MODULE_EXPORT
int                     ario_server_get_current_playlist_length            (void);
G_MODULE_EXPORT
int                     ario_server_get_current_playlist_total_time        (void);
G_MODULE_EXPORT
int                     ario_server_get_crossfadetime                      (void);
G_MODULE_EXPORT
gboolean                ario_server_get_current_consume                     (void);
G_MODULE_EXPORT
gboolean                ario_server_get_current_random                     (void);
G_MODULE_EXPORT
gboolean                ario_server_get_current_repeat                     (void);
G_MODULE_EXPORT
gboolean                ario_server_get_updating                           (void);
G_MODULE_EXPORT
unsigned long           ario_server_get_last_update                        (void);
G_MODULE_EXPORT
void                    ario_server_set_current_elapsed                    (const gint elapsed);
G_MODULE_EXPORT
void                    ario_server_set_current_volume                     (const gint volume);
G_MODULE_EXPORT
GSList *                ario_server_get_outputs                            (void);
G_MODULE_EXPORT
void                    ario_server_set_current_consume                     (const gboolean consume);
G_MODULE_EXPORT
void                    ario_server_set_current_random                     (const gboolean random);
G_MODULE_EXPORT
void                    ario_server_set_current_repeat                     (const gboolean repeat);
G_MODULE_EXPORT
void                    ario_server_set_crossfadetime                      (const int crossfadetime);
G_MODULE_EXPORT
gboolean                ario_server_is_paused                              (void);
G_MODULE_EXPORT
void                    ario_server_do_next                                (void);
G_MODULE_EXPORT
void                    ario_server_do_prev                                (void);
G_MODULE_EXPORT
void                    ario_server_do_play                                (void);
G_MODULE_EXPORT
void                    ario_server_do_play_pos                            (gint id);
G_MODULE_EXPORT
void                    ario_server_do_pause                               (void);
G_MODULE_EXPORT
void                    ario_server_do_stop                                (void);
G_MODULE_EXPORT
void                    ario_server_free_album                             (ArioServerAlbum *server_album);
G_MODULE_EXPORT
ArioServerAlbum *       ario_server_copy_album                             (const ArioServerAlbum *server_album);
G_MODULE_EXPORT
void                    ario_server_clear                                  (void);
G_MODULE_EXPORT
void                    ario_server_shuffle                                (void);
G_MODULE_EXPORT
void                    ario_server_queue_add                              (const char *path);
G_MODULE_EXPORT
void                    ario_server_queue_delete_id                        (const int id);
G_MODULE_EXPORT
void                    ario_server_queue_delete_pos                       (const int pos);
G_MODULE_EXPORT
void                    ario_server_queue_move                             (const int old_pos,
                                                                            const int new_pos);
G_MODULE_EXPORT
void                    ario_server_queue_moveid                           (const int id,
                                                                            const int pos);
G_MODULE_EXPORT
void                    ario_server_queue_commit                           (void);
G_MODULE_EXPORT
void                    ario_server_insert_at                              (const GSList *songs,
                                                                            const gint pos);
G_MODULE_EXPORT
// returns 0 if OK, 1 if playlist already exists
int                     ario_server_save_playlist                          (const char *name);
G_MODULE_EXPORT
void                    ario_server_delete_playlist                        (const char *name);
G_MODULE_EXPORT
GSList *                ario_server_get_outputs                            (void);
G_MODULE_EXPORT
void                    ario_server_enable_output                          (const int id,
                                                                            const gboolean enabled);
G_MODULE_EXPORT
ArioServerStats *       ario_server_get_stats                              (void);
G_MODULE_EXPORT
GList *                 ario_server_get_songs_info                         (GSList *paths);
G_MODULE_EXPORT
ArioServerFileList*     ario_server_list_files                             (const char *path,
                                                                            const gboolean recursive);
G_MODULE_EXPORT
void                    ario_server_free_file_list                         (ArioServerFileList *files);
G_MODULE_EXPORT
ArioServerCriteria *    ario_server_criteria_copy                          (const ArioServerCriteria *criteria);
G_MODULE_EXPORT
void                    ario_server_criteria_free                          (ArioServerCriteria *criteria);
G_MODULE_EXPORT
gchar **                ario_server_get_items_names                        (void);
G_MODULE_EXPORT
const gchar*            ario_server_song_get_tag                           (const ArioServerSong *song,
                                                                            ArioServerTag tag);
G_MODULE_EXPORT
void                    ario_server_playlist_add_songs                     (const GSList *songs,
                                                                            const gint pos,
                                                                            const PlaylistAction action);
G_MODULE_EXPORT
void                    ario_server_playlist_add_dir                       (const gchar *dir,
                                                                            const gint pos,
                                                                            const PlaylistAction action);
G_MODULE_EXPORT
void                    ario_server_playlist_add_criterias                 (const GSList *criterias,
                                                                            const gint pos,
                                                                            const PlaylistAction action,
                                                                            const gint nb_entries);
G_MODULE_EXPORT
void                    ario_server_playlist_append_artists                (const GSList *artists,
                                                                            const PlaylistAction action,
                                                                            const gint nb_entries);
G_MODULE_EXPORT
void                    ario_server_playlist_append_songs                  (const GSList *songs,
                                                                            const PlaylistAction action);
G_MODULE_EXPORT
void                    ario_server_playlist_append_server_songs           (const GSList *songs,
                                                                            const PlaylistAction action);
G_MODULE_EXPORT
void                    ario_server_playlist_append_dir                    (const gchar *dir,
                                                                            const PlaylistAction action);
G_MODULE_EXPORT
void                    ario_server_playlist_append_criterias              (const GSList *criterias,
                                                                            const PlaylistAction action,
                                                                            const gint nb_entries);
G_MODULE_EXPORT
void                    ario_server_free_song                              (ArioServerSong *song);
G_MODULE_EXPORT
void                    ario_server_free_output                            (ArioServerOutput *output);

G_END_DECLS

#endif /* __ARIO_SERVER_H */
