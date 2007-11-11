/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
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


#ifndef __ARIO_COVER_H
#define __ARIO_COVER_H

#define COVER_SIZE 70

G_BEGIN_DECLS

typedef enum
{
        GET_FIRST_COVER,
        GET_ALL_COVERS
}ArioCoverAmazonOperation;

typedef enum
{
        AMAZON_SMALL_COVER,
        AMAZON_MEDIUM_COVER,
        AMAZON_LARGE_COVER
}ArioCoverAmazonCoversSize;

typedef enum
{
        SMALL_COVER,
        NORMAL_COVER
}ArioCoverHomeCoversSize;

typedef enum
{
        OVERWRITE_MODE_ASK,
        OVERWRITE_MODE_REPLACE,
        OVERWRITE_MODE_SKIP
}ArioCoverOverwriteMode;

gboolean                     ario_cover_load_amazon_covers   (const char *artist,
                                                              const char *album,
                                                              GSList **ario_cover_uris,
                                                              GArray **file_size,
                                                              GSList **file_contents,
                                                              ArioCoverAmazonOperation operation,
                                                              ArioCoverAmazonCoversSize ario_cover_size);
gboolean                     ario_cover_save_cover           (const gchar *artist,
                                                              const gchar *album,
                                                              char *data,
                                                              int size,
                                                              ArioCoverOverwriteMode overwrite_mode);
void                         ario_cover_remove_cover         (const gchar *artist,
                                                              const gchar *album);
gboolean                     ario_cover_size_is_valid        (int size);

gboolean                     ario_cover_ario_cover_exists    (const gchar *artist,
                                                              const gchar *album);
gchar*                       ario_cover_make_ario_cover_path (const gchar *artist,
                                                              const gchar *album,
                                                              ArioCoverHomeCoversSize ario_cover_size);

G_END_DECLS

#endif /* __ARIO_COVER_H */
