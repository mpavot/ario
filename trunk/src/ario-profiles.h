/*
 *  Copyright (C) 2007 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_PROFILES_H
#define __ARIO_PROFILES_H

#include <glib/gslist.h>

G_BEGIN_DECLS

typedef struct
{
        gchar *name;
        gchar *host;
        int port;
        gchar *password;
        gchar *musicdir;
        gboolean local;
        gboolean current;
} ArioProfile;

GSList*                  ario_profiles_read              (void);

void                    ario_profiles_save              (GSList* profiles);

void                    ario_profiles_free              (ArioProfile* profile);

ArioProfile*            ario_profiles_get_current       (GSList* profiles);

void                    ario_profiles_set_current       (GSList* profiles,
                                                         ArioProfile* profile);

G_END_DECLS

#endif /* __ARIO_PROFILES_H */
