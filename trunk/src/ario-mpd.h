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

#ifndef __ARIO_MPD_H
#define __ARIO_MPD_H

#include <glib-object.h>
#include "lib/libmpdclient.h"

G_BEGIN_DECLS

#define ARIO_MPD_UNKNOWN     _("Unknown")

#define TYPE_ARIO_MPD         (ario_mpd_get_type ())
#define ARIO_MPD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_MPD, ArioMpd))
#define ARIO_MPD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_MPD, ArioMpdClass))
#define IS_ARIO_MPD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_MPD))
#define IS_ARIO_MPD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_MPD))
#define ARIO_MPD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_MPD, ArioMpdClass))

typedef struct ArioMpdPrivate ArioMpdPrivate;

typedef struct
{
        GObject parent;

        ArioMpdPrivate *priv;
} ArioMpd;

typedef struct ArioMpdAlbum
{
        gchar *artist;
        gchar *album;
        gchar *path;
        gchar *date;
} ArioMpdAlbum;

typedef struct ArioMpdFileList
{
        GSList *directories;
        GSList *songs;
} ArioMpdFileList;

typedef mpd_Song ArioMpdSong;
typedef mpd_TagItems ArioMpdTag;
typedef mpd_OutputEntity ArioMpdOutput;
typedef mpd_Stats ArioMpdStats;

typedef struct
{
        ArioMpdTag tag;
        gchar *value;
} ArioMpdAtomicCriteria;

typedef GSList ArioMpdCriteria; /* A criteria is a list of atomic criterias */

#define ario_mpd_free_song mpd_freeSong
#define ario_mpd_free_output mpd_freeOutputElement

typedef struct
{
        GObjectClass parent;

        void (*song_changed)            (ArioMpd *mpd);

        void (*album_changed)           (ArioMpd *mpd);

        void (*state_changed)           (ArioMpd *mpd);

        void (*volume_changed)          (ArioMpd *mpd);

        void (*elapsed_changed)         (ArioMpd *mpd);

        void (*playlist_changed)        (ArioMpd *mpd);

        void (*random_changed)          (ArioMpd *mpd);

        void (*repeat_changed)          (ArioMpd *mpd);

        void (*updatingdb_changed)      (ArioMpd *mpd);

        void (*storedplaylists_changed) (ArioMpd *mpd);
} ArioMpdClass;

GType                   ario_mpd_get_type                               (void) G_GNUC_CONST;

ArioMpd *               ario_mpd_get_instance                           (void);

gboolean                ario_mpd_connect                                (void);

void                    ario_mpd_disconnect                             (void);

gboolean                ario_mpd_is_connected                           (void);

gboolean                ario_mpd_update_status                          (void);

void                    ario_mpd_update_db                              (void);

GSList *                ario_mpd_list_tags                              (const ArioMpdTag tag,
                                                                         const ArioMpdCriteria *criteria);
GSList *                ario_mpd_get_artists                            (void);

GSList *                ario_mpd_get_albums                             (const ArioMpdCriteria *criteria);
GSList *                ario_mpd_get_songs                              (const ArioMpdCriteria *criteria,
                                                                         const gboolean exact);
GSList *                ario_mpd_get_songs_from_playlist                (char *playlist);
GSList *                ario_mpd_get_playlists                          (void);

GSList *                ario_mpd_get_playlist_changes                   (int playlist_id);
ArioMpdSong *           ario_mpd_get_playlist_info                      (int song_pos);
char *                  ario_mpd_get_current_title                      (void);

char *                  ario_mpd_get_current_name                       (void);

ArioMpdSong *           ario_mpd_get_current_song                       (void);

char *                  ario_mpd_get_current_artist                     (void);

char *                  ario_mpd_get_current_album                      (void);

char *                  ario_mpd_get_current_song_path                  (void);

int                     ario_mpd_get_current_song_id                    (void);

int                     ario_mpd_get_current_state                      (void);

int                     ario_mpd_get_current_elapsed                    (void);

int                     ario_mpd_get_current_volume                     (void);

int                     ario_mpd_get_current_total_time                 (void);

int                     ario_mpd_get_current_playlist_id                (void);

int                     ario_mpd_get_current_playlist_length            (void);

int                     ario_mpd_get_current_playlist_total_time        (void);

int                     ario_mpd_get_crossfadetime                      (void);

gboolean                ario_mpd_get_current_random                     (void);

gboolean                ario_mpd_get_current_repeat                     (void);

gboolean                ario_mpd_get_updating                           (void);

unsigned long           ario_mpd_get_last_update                        (void);

void                    ario_mpd_set_current_elapsed                    (const gint elapsed);
void                    ario_mpd_set_current_volume                     (const gint volume);
GSList *                ario_mpd_get_outputs                            (void);

void                    ario_mpd_set_current_random                     (const gboolean random);
void                    ario_mpd_set_current_repeat                     (const gboolean repeat);
void                    ario_mpd_set_crossfadetime                      (const int crossfadetime);
gboolean                ario_mpd_is_paused                              (void);

void                    ario_mpd_do_next                                (void);

void                    ario_mpd_do_prev                                (void);

void                    ario_mpd_do_play                                (void);

void                    ario_mpd_do_play_id                             (gint id);
void                    ario_mpd_do_pause                               (void);

void                    ario_mpd_do_stop                                (void);

void                    ario_mpd_free_album                             (ArioMpdAlbum *ario_mpd_album);

ArioMpdAlbum *          ario_mpd_copy_album                             (const ArioMpdAlbum *ario_mpd_album);

void                    ario_mpd_clear                                  (void);

void                    ario_mpd_queue_add                              (const char *path);
void                    ario_mpd_queue_delete_id                        (const int id);
void                    ario_mpd_queue_delete_pos                       (const int pos);
void                    ario_mpd_queue_move                             (const int old_pos,
                                                                         const int new_pos);
void                    ario_mpd_queue_commit                           (void);

// returns 0 if OK, 1 if playlist already exists
int                     ario_mpd_save_playlist                          (const char *name);
void                    ario_mpd_delete_playlist                        (const char *name);
void                    ario_mpd_use_count_inc                          (void);

void                    ario_mpd_use_count_dec                          (void);

GSList *                ario_mpd_get_outputs                            (void);

void                    ario_mpd_enable_output                          (const int id,
                                                                         const gboolean enabled);
ArioMpdStats *          ario_mpd_get_stats                              (void);

GList *                 ario_mpd_get_songs_info                         (GSList *paths);

ArioMpdFileList*        ario_mpd_list_files                             (const char *path,
                                                                         const gboolean recursive);
void                    ario_mpd_free_file_list                         (ArioMpdFileList *files);

ArioMpdCriteria *       ario_mpd_criteria_copy                          (const ArioMpdCriteria *criteria);

void                    ario_mpd_criteria_free                          (ArioMpdCriteria *criteria);

gchar **                ario_mpd_get_items_names                        (void);

G_END_DECLS

#endif /* __ARIO_MPD_H */
