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

#ifndef __ARIO_RADIO_H
#define __ARIO_RADIO_H

#include <gtk/gtkhbox.h>
#include <config.h>
#include "ario-mpd.h"
#include "widgets/ario-playlist.h"
#include "sources/ario-source.h"

#ifdef ENABLE_RADIOS

G_BEGIN_DECLS

#define TYPE_ARIO_RADIO         (ario_radio_get_type ())
#define ARIO_RADIO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_RADIO, ArioRadio))
#define ARIO_RADIO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_RADIO, ArioRadioClass))
#define IS_ARIO_RADIO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_RADIO))
#define IS_ARIO_RADIO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_RADIO))
#define ARIO_RADIO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_RADIO, ArioRadioClass))

typedef struct ArioRadioPrivate ArioRadioPrivate;

typedef struct
{
        ArioSource parent;

        ArioRadioPrivate *priv;
} ArioRadio;

typedef struct
{
        ArioSourceClass parent;
} ArioRadioClass;

GType                   ario_radio_get_type   (void);

GtkWidget*              ario_radio_new        (GtkUIManager *mgr,
                                               GtkActionGroup *group,
                                               ArioMpd *mpd,
                                               ArioPlaylist *playlist);

G_END_DECLS

#endif  /* ENABLE_RADIOS */

#endif /* __ARIO_RADIO_H */
