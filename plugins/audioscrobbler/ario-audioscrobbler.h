/*
 *  Based on Rhythmbox audioscrobbler plugin:
 *  Copyright (C) 2005 Alex Revo <xiphoiadappendix@gmail.com>
 *                                           Ruben Vermeersch <ruben@Lambda1.be>
 *
 *  Adapted to Ario:
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 */

#ifndef __ARIO_AUDIOSCROBBLER_H
#define __ARIO_AUDIOSCROBBLER_H

G_BEGIN_DECLS

#include <glib.h>

#include <gmodule.h>
#include "ario-mpd.h"
#include "plugins/ario-plugin.h"

#define ARIO_TYPE_AUDIOSCROBBLER                (ario_audioscrobbler_get_type ())
#define ARIO_AUDIOSCROBBLER(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_AUDIOSCROBBLER, ArioAudioscrobbler))
#define ARIO_AUDIOSCROBBLER_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_AUDIOSCROBBLER, ArioAudioscrobblerClass))
#define ARIO_IS_AUDIOSCROBBLER(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_AUDIOSCROBBLER))
#define ARIO_IS_AUDIOSCROBBLER_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_AUDIOSCROBBLER))
#define ARIO_AUDIOSCROBBLER_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_AUDIOSCROBBLER, ArioAudioscrobblerClass))


typedef struct _ArioAudioscrobblerPrivate ArioAudioscrobblerPrivate;

typedef struct
{
        GObject parent;

        ArioAudioscrobblerPrivate *priv;
} ArioAudioscrobbler;

typedef struct
{
        GObjectClass parent_class;
} ArioAudioscrobblerClass;


GType                   ario_audioscrobbler_get_type (void);

ArioAudioscrobbler*     ario_audioscrobbler_new (ArioMpd *mpd);

GtkWidget *             ario_audioscrobbler_get_config_widget (ArioAudioscrobbler *audioscrobbler,
                                                               ArioPlugin *plugin);

G_END_DECLS

#endif
