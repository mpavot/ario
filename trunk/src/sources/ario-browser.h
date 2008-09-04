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

#ifndef __ARIO_BROWSER_H
#define __ARIO_BROWSER_H

#include <gtk/gtkhbox.h>
#include "ario-mpd.h"
#include "widgets/ario-playlist.h"
#include "sources/ario-source.h"

#define MAX_TREE_NB 5

G_BEGIN_DECLS

#define TYPE_ARIO_BROWSER         (ario_browser_get_type ())
#define ARIO_BROWSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_BROWSER, ArioBrowser))
#define ARIO_BROWSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_BROWSER, ArioBrowserClass))
#define IS_ARIO_BROWSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_BROWSER))
#define IS_ARIO_BROWSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_BROWSER))
#define ARIO_BROWSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_BROWSER, ArioBrowserClass))

typedef struct ArioBrowserPrivate ArioBrowserPrivate;

typedef struct
{
        ArioSource parent;

        ArioBrowserPrivate *priv;
} ArioBrowser;

typedef struct
{
        ArioSourceClass parent;
} ArioBrowserClass;

GType                   ario_browser_get_type   (void) G_GNUC_CONST;

GtkWidget*              ario_browser_new        (GtkUIManager *mgr,
                                                 GtkActionGroup *group,
                                                 ArioMpd *mpd,
                                                 ArioPlaylist *playlist);

G_END_DECLS

#endif /* __ARIO_BROWSER_H */
