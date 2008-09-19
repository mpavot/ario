/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *   Based on:
 *   Implementation of Rhythmbox volume control button
 *   Copyright (C) 2003 Colin Walters <walters@rhythmbox.org>
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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include <glib/gi18n.h>
#include "widgets/ario-volume.h"
#include "ario-debug.h"
#include "ario-mpd.h"

static void clicked_cb (GtkButton *button, ArioVolume *volume);
static gboolean scroll_cb (GtkWidget *widget, GdkEvent *event, ArioVolume *volume);
static gboolean scale_button_release_event_cb (GtkWidget *widget,
                                               GdkEventButton *event, ArioVolume *volume);
static gboolean scale_button_event_cb (GtkWidget *widget, GdkEventButton *event,
                                       ArioVolume *volume);
static gboolean scale_key_press_event_cb (GtkWidget *widget, GdkEventKey *event,
                                          ArioVolume *volume);
static void mixer_value_changed_cb (GtkAdjustment *adj,
                                    ArioVolume *volume);
static void ario_volume_changed_cb (ArioMpd *mpd,
                                    int vol,
                                    ArioVolume *volume);

#define ARIO_VOLUME_MAX 100

struct ArioVolumePrivate
{
        GtkWidget *button;

        GtkWidget *window;

        GtkWidget *scale;
        GtkAdjustment *adj;

        GtkWidget *max_image;
        GtkWidget *medium_image;
        GtkWidget *min_image;
        GtkWidget *zero_image;

        guint notify_id;
};

#define ARIO_VOLUME_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_VOLUME, ArioVolumePrivate))
G_DEFINE_TYPE (ArioVolume, ario_volume, GTK_TYPE_EVENT_BOX)

static void
ario_volume_class_init (ArioVolumeClass *klass)
{
        ARIO_LOG_FUNCTION_START
        g_type_class_add_private (klass, sizeof (ArioVolumePrivate));
}

static void
ario_volume_init (ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *frame;
        GtkWidget *inner_frame;
        GtkWidget *pluslabel, *minuslabel;
        GtkWidget *event;
        GtkWidget *box;

        volume->priv = ARIO_VOLUME_GET_PRIVATE (volume);

        volume->priv->button = gtk_button_new ();

        gtk_container_add (GTK_CONTAINER (volume), volume->priv->button);

        volume->priv->max_image = gtk_image_new_from_stock ("volume-max",
                                                            GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (G_OBJECT (volume->priv->max_image));
        volume->priv->medium_image = gtk_image_new_from_stock ("volume-medium",
                                                               GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (G_OBJECT (volume->priv->medium_image));
        volume->priv->min_image = gtk_image_new_from_stock ("volume-min",
                                                            GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (G_OBJECT (volume->priv->min_image));
        volume->priv->zero_image = gtk_image_new_from_stock ("volume-zero",
                                                             GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (G_OBJECT (volume->priv->zero_image));

        gtk_container_add (GTK_CONTAINER (volume->priv->button), volume->priv->max_image);

        g_signal_connect (volume->priv->button,
                          "clicked",
                          G_CALLBACK (clicked_cb),
                          volume);
        g_signal_connect (volume->priv->button,
                          "scroll_event",
                          G_CALLBACK (scroll_cb),
                          volume);
        gtk_widget_show_all (GTK_WIDGET (volume));

        volume->priv->window = gtk_window_new (GTK_WINDOW_POPUP);

        volume->priv->adj = GTK_ADJUSTMENT (gtk_adjustment_new (50,
                                                                0.0,
                                                                (gdouble) ARIO_VOLUME_MAX,
                                                                (gdouble) ARIO_VOLUME_MAX/20,
                                                                (gdouble) ARIO_VOLUME_MAX/10,
                                                                0.0));
        g_signal_connect (volume->priv->adj,
                          "value-changed",
                          (GCallback) mixer_value_changed_cb,
                          volume);

        frame = gtk_frame_new (NULL);
        gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

        inner_frame = gtk_frame_new (NULL);
        gtk_container_set_border_width (GTK_CONTAINER (inner_frame), 0);
        gtk_frame_set_shadow_type (GTK_FRAME (inner_frame), GTK_SHADOW_NONE);

        event = gtk_event_box_new ();
        /* This signal is to not let button press close the popup when the press is
         ** in the scale */
        g_signal_connect_after (event, "button_press_event",
                                G_CALLBACK (scale_button_event_cb), volume);

        box = gtk_vbox_new (FALSE, 0);
        volume->priv->scale = gtk_vscale_new (volume->priv->adj);
        gtk_range_set_inverted (GTK_RANGE (volume->priv->scale), TRUE);
        gtk_widget_set_size_request (volume->priv->scale, -1, 100);

        g_signal_connect (volume->priv->window,
                          "scroll_event",
                          G_CALLBACK (scroll_cb),
                          volume);

        g_signal_connect (volume->priv->window,
                          "button-press-event",
                          (GCallback) scale_button_release_event_cb,
                          volume);

        /* button event on the scale widget are not catched by its parent window
         ** so we must connect to this widget as well */
        g_signal_connect (volume->priv->scale,
                          "button-release-event",
                          (GCallback) scale_button_release_event_cb,
                          volume);

        g_signal_connect (volume->priv->scale,
                          "key-press-event",
                          (GCallback) scale_key_press_event_cb,
                          volume);

        gtk_scale_set_draw_value (GTK_SCALE (volume->priv->scale), FALSE);

        gtk_range_set_update_policy (GTK_RANGE (volume->priv->scale),
                                     GTK_UPDATE_CONTINUOUS);

        gtk_container_add (GTK_CONTAINER (volume->priv->window), frame);

        gtk_container_add (GTK_CONTAINER (frame), inner_frame);

        pluslabel = gtk_label_new ("+");
        minuslabel = gtk_label_new ("-");

        gtk_box_pack_start (GTK_BOX (box), pluslabel, FALSE, FALSE, 0);
        gtk_box_pack_end (GTK_BOX (box), minuslabel, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (box), volume->priv->scale, TRUE, TRUE, 0);

        gtk_container_add (GTK_CONTAINER (event), box);
        gtk_container_add (GTK_CONTAINER (inner_frame), event);
}

static void
ario_volume_changed_cb (ArioMpd *mpd,
                        int vol,
                        ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *image;

        if (vol == -1)
                return;

        gtk_container_remove (GTK_CONTAINER (volume->priv->button),
                              gtk_bin_get_child (GTK_BIN (volume->priv->button)));

        if (vol <= 0)
                image = volume->priv->zero_image;
        else if (vol <= (ARIO_VOLUME_MAX / 3.0))
                image = volume->priv->min_image;
        else if (vol <= 2.0 * (ARIO_VOLUME_MAX / 3.0))
                image = volume->priv->medium_image;
        else
                image = volume->priv->max_image;

        gtk_widget_show (image);
        gtk_container_add (GTK_CONTAINER (volume->priv->button), image);

        gtk_adjustment_set_value (volume->priv->adj, (gdouble) vol);
}

ArioVolume *
ario_volume_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioVolume *volume;

        volume = ARIO_VOLUME (g_object_new (TYPE_ARIO_VOLUME, NULL));

        g_return_val_if_fail (volume->priv != NULL, NULL);

        g_signal_connect_object (ario_mpd_get_instance (),
                                 "volume_changed",
                                 G_CALLBACK (ario_volume_changed_cb),
                                 volume, 0);
        return volume;
}

static void
clicked_cb (GtkButton *button,
            ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        GtkRequisition  req;
        GdkGrabStatus pointer, keyboard;
        gint x, y;
        gint button_width, button_height;
        gint window_width, window_height;
        gint spacing = 5;
        gint max_y;

        gint volume_slider_x;
        gint volume_slider_y;


        /*         if (GTK_WIDGET_VISIBLE (GTK_WIDGET (volume->priv->window))) */
        /*                 return; */

        /*
         * Position the popup right next to the button.
         */

        max_y = gdk_screen_height ();

        gtk_widget_size_request (GTK_WIDGET (volume->priv->window), &req);

        gdk_window_get_origin (gtk_widget_get_parent_window (GTK_BIN (volume->priv->button)->child), &x, &y);
        gdk_drawable_get_size (gtk_widget_get_parent_window (GTK_BIN (volume->priv->button)->child), &button_width, &button_height);




        gtk_widget_show_all (volume->priv->window);
        gdk_drawable_get_size (gtk_widget_get_parent_window (GTK_BIN (volume->priv->window)->child), &window_width, &window_height);

        volume_slider_x = x + (button_width - window_width) / 2;

        if (y + button_width + window_height + spacing < max_y) {
                /* if volume slider will fit on the screen, display it under
                 * the volume button
                 */
                volume_slider_y = y + button_width + spacing;
        } else {
                /* otherwise display it above the volume button */
                volume_slider_y = y - window_height - spacing;
        }

        gtk_window_move (GTK_WINDOW (volume->priv->window), volume_slider_x, volume_slider_y);

        /*
         * Grab focus and pointer.
         */
        gtk_widget_grab_focus (volume->priv->window);
        gtk_grab_add (volume->priv->window);

        pointer = gdk_pointer_grab (volume->priv->window->window,
                                    TRUE,
                                    (GDK_BUTTON_PRESS_MASK |
                                     GDK_BUTTON_RELEASE_MASK |
                                     GDK_POINTER_MOTION_MASK |
                                     GDK_SCROLL_MASK),
                                    NULL, NULL, GDK_CURRENT_TIME);

        keyboard = gdk_keyboard_grab (volume->priv->window->window,
                                      TRUE,
                                      GDK_CURRENT_TIME);

        if (pointer != GDK_GRAB_SUCCESS || keyboard != GDK_GRAB_SUCCESS) {
                /* We could not grab. */
                gtk_grab_remove (volume->priv->window);
                gtk_widget_hide (volume->priv->window);

                if (pointer == GDK_GRAB_SUCCESS) {
                        gdk_keyboard_ungrab (GDK_CURRENT_TIME);
                }
                if (keyboard == GDK_GRAB_SUCCESS) {
                        gdk_pointer_ungrab (GDK_CURRENT_TIME);
                }

                g_warning ("Could not grab X server!");
                return;
        }

        /* gtk_frame_set_shadow_type (GTK_FRAME (volume->priv->frame), GTK_SHADOW_IN); */
}

static gboolean
scroll_cb (GtkWidget *widget, GdkEvent *event, ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        gint vol = ario_mpd_get_current_volume ();

        switch (event->scroll.direction) {
        case GDK_SCROLL_UP:
                vol += 10;
                if (vol > 100)
                        vol = 100;
                break;
        case GDK_SCROLL_DOWN:
                vol -= 10;
                if (vol < 0)
                        vol = 0;
                break;
        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_RIGHT:
                break;
        }

        ario_mpd_set_current_volume (vol);

        return FALSE;
}


static void
ario_volume_popup_hide (ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        gtk_grab_remove (volume->priv->window);
        gdk_pointer_ungrab (GDK_CURRENT_TIME);
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);

        gtk_widget_hide (GTK_WIDGET (volume->priv->window));

        /*         gtk_frame_set_shadow_type (GTK_FRAME (data->frame), GTK_SHADOW_NONE); */
}

static gboolean
scale_button_release_event_cb (GtkWidget *widget, GdkEventButton *event, ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        ario_volume_popup_hide (volume);
        return FALSE;
}

static gboolean
scale_button_event_cb (GtkWidget *widget, GdkEventButton *event, ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        return TRUE;
}

static gboolean
scale_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        switch (event->keyval) {
        case GDK_KP_Enter:
        case GDK_ISO_Enter:
        case GDK_3270_Enter:
        case GDK_Return:
        case GDK_space:
        case GDK_KP_Space:
                ario_volume_popup_hide (volume);
                return TRUE;
        default:
                break;
        }

        return FALSE;
}

static void
mixer_value_changed_cb (GtkAdjustment *adj, ArioVolume *volume)
{
        ARIO_LOG_FUNCTION_START
        gint vol = (gint) gtk_adjustment_get_value (volume->priv->adj);

        ario_mpd_set_current_volume (vol);
}
