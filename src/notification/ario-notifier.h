/*
 *  Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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

#ifndef __ARIO_NOTIFIER_H__
#define __ARIO_NOTIFIER_H__

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ARIO_TYPE_NOTIFIER              (ario_notifier_get_type())
#define ARIO_NOTIFIER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ARIO_TYPE_NOTIFIER, ArioNotifier))
#define ARIO_NOTIFIER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ARIO_TYPE_NOTIFIER, ArioNotifierClass))
#define ARIO_IS_NOTIFIER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ARIO_TYPE_NOTIFIER))
#define ARIO_IS_NOTIFIER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ARIO_TYPE_NOTIFIER))
#define ARIO_NOTIFIER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ARIO_TYPE_NOTIFIER, ArioNotifierClass))

/*
 * Main object structure
 */
typedef struct _ArioNotifier 
{
        GObject parent;
} ArioNotifier;

/*
 * Class definition
 */
typedef struct 
{
        GObjectClass parent_class;

        /* Virtual public methods */

        gchar*          (*get_id)                       (ArioNotifier *notifier);

        gchar*          (*get_name)                     (ArioNotifier *notifier);

        void            (*notify)                       (ArioNotifier *notifier);
} ArioNotifierClass;

/*
 * Public methods
 */
GType           ario_notifier_get_type            (void) G_GNUC_CONST;

gchar*          ario_notifier_get_id              (ArioNotifier *notifier);

gchar*          ario_notifier_get_name            (ArioNotifier *notifier);

void            ario_notifier_notify              (ArioNotifier *notifier);

G_END_DECLS

#endif  /* __ARIO_NOTIFIER_H__ */


