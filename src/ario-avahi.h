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

#ifndef __ARIO_AVAHI_H
#define __ARIO_AVAHI_H

G_BEGIN_DECLS

#define TYPE_ARIO_AVAHI         (ario_avahi_get_type ())
#define ARIO_AVAHI(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_AVAHI, ArioAvahi))
#define ARIO_AVAHI_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_AVAHI, ArioAvahiClass))
#define IS_ARIO_AVAHI(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_AVAHI))
#define IS_ARIO_AVAHI_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_AVAHI))
#define ARIO_AVAHI_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_AVAHI, ArioAvahiClass))

typedef struct ArioAvahiPrivate ArioAvahiPrivate;

typedef struct
{
        GObject parent;

        ArioAvahiPrivate *priv;
} ArioAvahi;

typedef struct
{
        GObjectClass parent_class;

        void (*hosts_changed)            (ArioAvahi *avahi);
} ArioAvahiClass;

typedef struct ArioHost
{
        gchar *name;
        gchar *host;
        int port;
} ArioHost;

GType           ario_avahi_get_type     (void) G_GNUC_CONST;

ArioAvahi *     ario_avahi_new          (void);

GSList *         ario_avahi_get_hosts    (ArioAvahi *avahi);

G_END_DECLS

#endif /* __ARIO_AVAHI_H */
