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

#include <config.h>
#include "notification/ario-notifier.h"
#include "ario-debug.h"

G_DEFINE_TYPE (ArioNotifier, ario_notifier, G_TYPE_OBJECT)

static void
dummy (ArioNotifier *notifier)
{

}

static gchar *
dummy_char (ArioNotifier *notifier)
{
        return NULL;
}

static void 
ario_notifier_class_init (ArioNotifierClass *klass)
{
        klass->get_id = dummy_char;
        klass->get_name = dummy_char;
        klass->notify = dummy;
}

static void
ario_notifier_init (ArioNotifier *notifier)
{
}

gchar *
ario_notifier_get_id (ArioNotifier *notifier)
{
        g_return_val_if_fail (ARIO_IS_NOTIFIER (notifier), FALSE);

        return ARIO_NOTIFIER_GET_CLASS (notifier)->get_id (notifier);
}

gchar *
ario_notifier_get_name (ArioNotifier *notifier)
{
        g_return_val_if_fail (ARIO_IS_NOTIFIER (notifier), FALSE);

        return ARIO_NOTIFIER_GET_CLASS (notifier)->get_name (notifier);
}

void
ario_notifier_notify (ArioNotifier *notifier)
{
        g_return_if_fail (ARIO_IS_NOTIFIER (notifier));

        return ARIO_NOTIFIER_GET_CLASS (notifier)->notify (notifier);
}

