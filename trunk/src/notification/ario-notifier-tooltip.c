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

#include "notification/ario-notifier-tooltip.h"
#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "widgets/ario-tray-icon.h"
#include "ario-debug.h"

G_DEFINE_TYPE (ArioNotifierTooltip, ario_notifier_tooltip, ARIO_TYPE_NOTIFIER)

static gchar *
ario_notifier_tooltip_get_id (ArioNotifier *notifier)
{
        return "tooltip";
}

static gchar *
ario_notifier_tooltip_get_name (ArioNotifier *notifier)
{
        return "Tooltip";
}

static void
ario_notifier_tooltip_notify (ArioNotifier *notifier)
{
        ario_tray_icon_notify ();
}

static void
ario_notifier_tooltip_class_init (ArioNotifierTooltipClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierClass *notifier_class = ARIO_NOTIFIER_CLASS (klass);

        notifier_class->get_id = ario_notifier_tooltip_get_id;
        notifier_class->get_name = ario_notifier_tooltip_get_name;
        notifier_class->notify = ario_notifier_tooltip_notify;
}

static void
ario_notifier_tooltip_init (ArioNotifierTooltip *notifier_tooltip)
{
        ARIO_LOG_FUNCTION_START;
}

ArioNotifier*
ario_notifier_tooltip_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierTooltip *local;

        local = g_object_new (TYPE_ARIO_NOTIFIER_TOOLTIP, NULL);

        return ARIO_NOTIFIER (local);
}

