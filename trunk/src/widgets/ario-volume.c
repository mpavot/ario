/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
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

#include "widgets/ario-volume.h"
#include <gtk/gtk.h>
#include <config.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "servers/ario-server.h"

static void ario_volume_value_changed_cb (GtkScaleButton *button,
                                          gdouble value,
                                          ArioVolume *volume);
static void ario_volume_changed_cb (ArioServer *server,
                                    int vol,
                                    ArioVolume *volume);

#define ARIO_VOLUME_MAX 100

struct ArioVolumePrivate
{
        GtkWidget *volume_button;
        gboolean loading;
};

#define ARIO_VOLUME_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_VOLUME, ArioVolumePrivate))
G_DEFINE_TYPE (ArioVolume, ario_volume, GTK_TYPE_EVENT_BOX)

static void
ario_volume_class_init (ArioVolumeClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        g_type_class_add_private (klass, sizeof (ArioVolumePrivate));
}

static void
ario_volume_init (ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START;
        GtkAdjustment *adj;

        volume->priv = ARIO_VOLUME_GET_PRIVATE (volume);

        /* Volume button */
        volume->priv->volume_button = gtk_volume_button_new ();
        adj = gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (volume->priv->volume_button));

        /* Connect signal for user action */
        g_signal_connect (volume->priv->volume_button,
                          "value-changed",
                          (GCallback) ario_volume_value_changed_cb,
                          volume);

        /* Add volume button in widget */
        gtk_container_add (GTK_CONTAINER (volume), volume->priv->volume_button);
        gtk_widget_show_all (GTK_WIDGET (volume));

        /* Set adjustments values */
        volume->priv->loading = TRUE;
        gtk_adjustment_set_value (adj, 50);
        g_object_set (adj,
                      "lower", (gdouble) 0.0,
                      "upper", (gdouble) ARIO_VOLUME_MAX,
                      "step-increment", (gdouble) ARIO_VOLUME_MAX/20,
                      "page-increment", (gdouble) ARIO_VOLUME_MAX/10,
                      "page-size", (gdouble) 0.0,
                      NULL);

        volume->priv->loading = FALSE;
}

static void
ario_volume_changed_cb (ArioServer *server,
                        int vol,
                        ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START;

        if (vol == -1)
                return;

        /* Update volume button with server value */
        volume->priv->loading = TRUE;
        gtk_scale_button_set_value (GTK_SCALE_BUTTON (volume->priv->volume_button), (gdouble) vol);
        volume->priv->loading = FALSE;
}

ArioVolume *
ario_volume_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioVolume *volume;

        volume = ARIO_VOLUME (g_object_new (TYPE_ARIO_VOLUME, NULL));

        g_return_val_if_fail (volume->priv != NULL, NULL);

        /* Connect signal for changes on server */
        g_signal_connect_object (ario_server_get_instance (),
                                 "volume_changed",
                                 G_CALLBACK (ario_volume_changed_cb),
                                 volume, 0);
        return volume;
}

static void
ario_volume_value_changed_cb (GtkScaleButton *button,
                              gdouble value,
                              ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START;

        /* Change value on server */
        if (!volume->priv->loading)
                ario_server_set_current_volume ((gint) value);
}
