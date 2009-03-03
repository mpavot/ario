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

#ifndef __ARIO_SERVER_INTERFACE_H
#define __ARIO_SERVER_INTERFACE_H

#include <glib-object.h>
#include "ario-server.h"

G_BEGIN_DECLS

#define TYPE_ARIO_SERVER_INTERFACE         (ario_server_interface_get_type ())
#define ARIO_SERVER_INTERFACE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SERVER_INTERFACE, ArioServerInterface))
#define ARIO_SERVER_INTERFACE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SERVER_INTERFACE, ArioServerInterfaceClass))
#define IS_ARIO_SERVER_INTERFACE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SERVER_INTERFACE))
#define IS_ARIO_SERVER_INTERFACE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SERVER_INTERFACE))
#define ARIO_SERVER_INTERFACE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SERVER_INTERFACE, ArioServerInterfaceClass))

typedef struct
{
        GObject parent;

        int song_id;
        int state;
        int volume;
        int elapsed;

        ArioServerSong *server_song;
        gint64 playlist_id;
        int playlist_length;

        gboolean random;
        gboolean repeat;

        int updatingdb;
        int crossfade;

        GSList *queue;

        gboolean connecting;

        int signals_to_emit;
} ArioServerInterface;

typedef struct
{
        GObjectClass parent;

        /* Virtual public methods */
        void              (*connect)                                  (void);

        void              (*disconnect)                               (void);

        gboolean          (*is_connected)                             (void);

        gboolean            (*update_status)                          (void);

        void                (*update_db)                              (void);

        GSList *            (*list_tags)                              (const ArioServerTag tag,
                                                                       const ArioServerCriteria *criteria);

        GSList *            (*get_albums)                             (const ArioServerCriteria *criteria);

        GSList *            (*get_songs)                              (const ArioServerCriteria *criteria,
                                                                       const gboolean exact);
        GSList *            (*get_songs_from_playlist)                (char *playlist);

        GSList *            (*get_playlists)                          (void);

        GSList *            (*get_playlist_changes)                   (gint64 playlist_id);

        ArioServerSong *    (*get_current_song_on_server)             (void);

        int                 (*get_current_playlist_total_time)        (void);

        unsigned long       (*get_last_update)                        (void);

        void                (*do_next)                                (void);

        void                (*do_prev)                                (void);

        void                (*do_play)                                (void);

        void                (*do_play_pos)                            (gint id);
        void                (*do_pause)                               (void);

        void                (*do_stop)                                (void);
        void                (*set_current_elapsed)                    (const gint elapsed);
        void                (*set_current_volume)                     (const gint volume);
        void                (*set_current_random)                     (const gboolean random);
        void                (*set_current_repeat)                     (const gboolean repeat);
        void                (*set_crossfadetime)                      (const int crossfadetime);
        void                (*clear)                                  (void);
        void                (*shuffle)                                (void);

        void                (*queue_commit)                           (void);

        void                (*insert_at)                              (const GSList *songs,
                                                                       const gint pos);
        int                 (*save_playlist)                          (const char *name);
        void                (*delete_playlist)                        (const char *name);

        GSList *            (*get_outputs)                            (void);

        void                (*enable_output)                          (const int id,
                                                                       const gboolean enabled);
        ArioServerStats *      (*get_stats)                           (void);

        GList *             (*get_songs_info)                         (GSList *paths);

        ArioServerFileList*    (*list_files)                          (const char *path,
                                                                       const gboolean recursive);
} ArioServerInterfaceClass;

GType                   ario_server_interface_get_type                (void) G_GNUC_CONST;

void                    ario_server_interface_set_default             (ArioServerInterface *server_interface);

void                    ario_server_interface_emit                    (ArioServerInterface *server_interface,
                                                                       ArioServer *server);
G_END_DECLS

#endif /* __ARIO_SERVER_INTERFACE_H */
