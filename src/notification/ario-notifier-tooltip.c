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

#include "ario-debug.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "widgets/ario-tray-icon.h"
#include "widgets/ario-tooltip.h"

struct ArioNotifierTooltipPrivate
{
        GtkWidget *window;
        guint id;
};

#define ARIO_NOTIFIER_TOOLTIP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_NOTIFIER_TOOLTIP, ArioNotifierTooltipPrivate))
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

static gboolean
ario_notifier_destroy_tooltip (ArioNotifierTooltip *notifier)
{
        notifier->priv->id = 0;
        gtk_widget_destroy (notifier->priv->window);
        notifier->priv->window = NULL;
        return FALSE;
}

static void
ario_notifier_tooltip_notify (ArioNotifier *notifier_parent)
{
        GtkWidget *tooltip;
        gint width;
        gint height;
        ArioNotifierTooltip *notifier = ARIO_NOTIFIER_TOOLTIP (notifier_parent);

        if (notifier->priv->id)
        {
                g_source_remove (notifier->priv->id);
        }
        else
        {
                tooltip = ario_tooltip_new ();
                notifier->priv->window = gtk_window_new (GTK_WINDOW_POPUP);
                gtk_widget_set_app_paintable (GTK_WIDGET (notifier->priv->window), TRUE);
                gtk_window_set_resizable (GTK_WINDOW (notifier->priv->window), FALSE);
                gtk_container_set_border_width (GTK_CONTAINER (notifier->priv->window), 4);

                gtk_container_add (GTK_CONTAINER (notifier->priv->window), tooltip);
        }
        g_return_if_fail (notifier->priv->window);
        gtk_window_get_size (GTK_WINDOW (notifier->priv->window),
                             &width,
                             &height);
        gtk_window_move (GTK_WINDOW (notifier->priv->window),
                         gdk_screen_width() - width - 25,
                         gdk_screen_height() - height - 25);
        gtk_widget_show_all (notifier->priv->window);

        notifier->priv->id = g_timeout_add (ario_conf_get_integer (PREF_NOTIFICATION_TIME, PREF_NOTIFICATION_TIME_DEFAULT) * 1000,
                                            (GSourceFunc) ario_notifier_destroy_tooltip,
                                            notifier);
}

static void
ario_notifier_tooltip_class_init (ArioNotifierTooltipClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierClass *notifier_class = ARIO_NOTIFIER_CLASS (klass);

        notifier_class->get_id = ario_notifier_tooltip_get_id;
        notifier_class->get_name = ario_notifier_tooltip_get_name;
        notifier_class->notify = ario_notifier_tooltip_notify;

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioNotifierTooltipPrivate));
}

static void
ario_notifier_tooltip_init (ArioNotifierTooltip *notifier_tooltip)
{
        ARIO_LOG_FUNCTION_START;

        notifier_tooltip->priv = ARIO_NOTIFIER_TOOLTIP_GET_PRIVATE (notifier_tooltip);
}

ArioNotifier*
ario_notifier_tooltip_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioNotifierTooltip *local;

        local = g_object_new (TYPE_ARIO_NOTIFIER_TOOLTIP, NULL);

        return ARIO_NOTIFIER (local);
}

