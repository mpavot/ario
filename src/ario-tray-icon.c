/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *   Based on:
 *   Implementation of Rhythmbox tray icon object
 *   Copyright (C) 2003,2004 Colin Walters <walters@rhythmbox.org>
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
#include <config.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include "ario-tray-icon.h"
#include "ario-util.h"
#include "ario-preferences.h"
#include "eel-gconf-extensions.h"
#include "ario-debug.h"

static void ario_tray_icon_class_init (ArioTrayIconClass *klass);
static void ario_tray_icon_init (ArioTrayIcon *ario_shell_player);
static GObject *ario_tray_icon_constructor (GType type, guint n_construct_properties,
                                            GObjectConstructParam *construct_properties);
static void ario_tray_icon_finalize (GObject *object);
static void ario_tray_icon_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void ario_tray_icon_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);
static void ario_tray_icon_set_visibility (ArioTrayIcon *tray, int state);
static void ario_tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                                  ArioTrayIcon *icon);
static gboolean ario_tray_icon_scroll_cb (GtkWidget *widget, GdkEvent *event,
                                          ArioTrayIcon *icon);
static void ario_tray_icon_song_changed_cb (ArioMpd *mpd,
                                            ArioTrayIcon *icon);
static void ario_tray_icon_state_changed_cb (ArioMpd *mpd,
                                             ArioTrayIcon *icon);

struct ArioTrayIconPrivate
{
        GtkTooltips *tooltips;

        GtkUIManager *ui_manager;

        GtkWindow *main_window;
        GtkWidget *ebox;

        gboolean maximized;
        int window_x;
        int window_y;
        int window_w;
        int window_h;
        gboolean visible;

        ArioMpd *mpd;
};

enum
{
        PROP_0,
        PROP_UI_MANAGER,
        PROP_WINDOW,
        PROP_MPD
};

enum
{
        VISIBILITY_HIDDEN,
        VISIBILITY_VISIBLE,
        VISIBILITY_TOGGLE
};

static GObjectClass *parent_class = NULL;

GType
ario_tray_icon_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_tray_icon_type = 0;

        if (ario_tray_icon_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioTrayIconClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_tray_icon_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioTrayIcon),
                        0,
                        (GInstanceInitFunc) ario_tray_icon_init
                };

                ario_tray_icon_type = g_type_register_static (EGG_TYPE_TRAY_ICON,
                                                              "ArioTrayIcon",
                                                              &our_info, 0);
        }

        return ario_tray_icon_type;
}

static void
ario_tray_icon_class_init (ArioTrayIconClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_tray_icon_finalize;
        object_class->constructor = ario_tray_icon_constructor;

        object_class->set_property = ario_tray_icon_set_property;
        object_class->get_property = ario_tray_icon_get_property;

        g_object_class_install_property (object_class,
                                         PROP_WINDOW,
                                         g_param_spec_object ("window",
                                                              "GtkWindow",
                                                              "main GtkWindo",
                                                              GTK_TYPE_WINDOW,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_tray_icon_init (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *image;

        icon->priv = g_new0 (ArioTrayIconPrivate, 1);

        icon->priv->tooltips = gtk_tooltips_new ();

        gtk_tooltips_set_tip (icon->priv->tooltips, GTK_WIDGET (icon),
                              _("Not playing"), NULL);

        icon->priv->ebox = gtk_event_box_new ();
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "button_press_event",
                                 G_CALLBACK (ario_tray_icon_button_press_event_cb),
                                 icon, 0);
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "scroll_event",
                                 G_CALLBACK (ario_tray_icon_scroll_cb),
                                 icon, 0);
        image = gtk_image_new_from_stock ("volume-max",
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_container_add (GTK_CONTAINER (icon->priv->ebox), image);
        
        icon->priv->window_x = -1;
        icon->priv->window_y = -1;
        icon->priv->window_w = -1;
        icon->priv->window_h = -1;
        icon->priv->visible = TRUE;

        gtk_container_add (GTK_CONTAINER (icon), icon->priv->ebox);
}

static GObject *
ario_tray_icon_constructor (GType type, guint n_construct_properties,
                            GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START
        ArioTrayIcon *tray;
        ArioTrayIconClass *klass;
        GObjectClass *parent_class;  

        klass = ARIO_TRAY_ICON_CLASS (g_type_class_peek (TYPE_ARIO_TRAY_ICON));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        tray = ARIO_TRAY_ICON (parent_class->constructor (type, n_construct_properties,
                                                        construct_properties));

        ario_tray_icon_set_visibility (tray, VISIBILITY_VISIBLE);
        return G_OBJECT (tray);
}

static void
ario_tray_icon_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioTrayIcon *tray;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_TRAY_ICON (object));

        tray = ARIO_TRAY_ICON (object);

        g_return_if_fail (tray->priv != NULL);
        
        gtk_object_destroy (GTK_OBJECT (tray->priv->tooltips));

        g_free (tray->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_tray_icon_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioTrayIcon *tray = ARIO_TRAY_ICON (object);

        switch (prop_id)
        {
        case PROP_WINDOW:
                tray->priv->main_window = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                tray->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_MPD:
                tray->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "song_changed", G_CALLBACK (ario_tray_icon_song_changed_cb),
                                         tray, 0);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_tray_icon_state_changed_cb),
                                         tray, 0);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "playlist_changed", G_CALLBACK (ario_tray_icon_state_changed_cb),
                                         tray, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_tray_icon_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioTrayIcon *tray = ARIO_TRAY_ICON (object);

        switch (prop_id)
        {
        case PROP_WINDOW:
                g_value_set_object (value, tray->priv->main_window);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, tray->priv->ui_manager);
                break;
        case PROP_MPD:
                g_value_set_object (value, tray->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

ArioTrayIcon *
ario_tray_icon_new (GtkUIManager *mgr,
                    GtkWindow *window,
                    ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return g_object_new (TYPE_ARIO_TRAY_ICON,
                             "title", _("Ario tray icon"),
                             "ui-manager", mgr,
                             "window", window,
                             "mpd", mpd,
                             NULL);
}

static void
ario_tray_icon_double_click (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        int trayicon_behavior;

        trayicon_behavior = eel_gconf_get_integer (CONF_TRAYICON_BEHAVIOR);

        switch (trayicon_behavior) {
        case TRAY_ICON_PLAY_PAUSE:
                        if (ario_mpd_is_paused (icon->priv->mpd))
                                ario_mpd_do_play (icon->priv->mpd);
                        else
                                ario_mpd_do_pause (icon->priv->mpd);
                break;

        case TRAY_ICON_NEXT_SONG:
                ario_mpd_do_next (icon->priv->mpd);
                break;

        case TRAY_ICON_DO_NOTHING:
        default:
                break;
        }



}

static void
ario_tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                      ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *popup;

        /* filter out double, triple clicks */
        if (event->type != GDK_BUTTON_PRESS)
                return;

        switch (event->button) {
        case 1:
                ario_tray_icon_set_visibility (icon, VISIBILITY_TOGGLE);
                break;

        case 2:
                ario_tray_icon_double_click (icon);
                break;

        case 3:
                popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (icon->priv->ui_manager),
                                                   "/TrayPopup");
                gtk_menu_set_screen (GTK_MENU (popup), gtk_widget_get_screen (GTK_WIDGET (icon)));
                gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                                NULL, NULL, 2,
                                gtk_get_current_event_time ());
                break;
        default:
                break;
        }
}

static gboolean
ario_tray_icon_scroll_cb (GtkWidget *widget, GdkEvent *event,
                          ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        gint vol = ario_mpd_get_current_volume (icon->priv->mpd);

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

        ario_mpd_set_current_volume (icon->priv->mpd, vol);

        return FALSE;
}

static void
ario_tray_icon_sync_tooltip (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        gchar *tooltip;
        gchar *title;

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                title = ario_util_format_title(ario_mpd_get_current_song (icon->priv->mpd));
                tooltip = g_strdup_printf (_("Artist: %s\nAlbum: %s\nTitle: %s"), 
                                            ario_mpd_get_current_artist (icon->priv->mpd) ? ario_mpd_get_current_artist (icon->priv->mpd) : ARIO_MPD_UNKNOWN,
                                            ario_mpd_get_current_album (icon->priv->mpd) ? ario_mpd_get_current_album (icon->priv->mpd) : ARIO_MPD_UNKNOWN,
                                            title);
                g_free (title);
                break;
        default:
                tooltip = g_strdup (_("Not playing"));
                break;
        }

        gtk_tooltips_set_tip (icon->priv->tooltips,
                              GTK_WIDGET (icon),
                              tooltip, NULL);

        g_free (tooltip);
}

static void
ario_tray_icon_song_changed_cb (ArioMpd *mpd,
                                ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip (icon);
}

static void
ario_tray_icon_state_changed_cb (ArioMpd *mpd,
                                 ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip (icon);
}

static void
ario_tray_icon_restore_main_window (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        if ((icon->priv->window_x >= 0 && icon->priv->window_y >= 0) || (icon->priv->window_h >= 0 && icon->priv->window_w >=0 )) {
                gtk_widget_realize (GTK_WIDGET (icon->priv->main_window));
                gdk_flush ();

                if (icon->priv->window_x >= 0 && icon->priv->window_y >= 0) {
                        gtk_window_move (icon->priv->main_window,
                                         icon->priv->window_x,
                                         icon->priv->window_y);
                }
                if (icon->priv->window_w >= 0 && icon->priv->window_y >=0) {
                        gtk_window_resize (icon->priv->main_window,
                                           icon->priv->window_w,
                                           icon->priv->window_h);
                }
        }

        if (icon->priv->maximized)
                gtk_window_maximize (GTK_WINDOW (icon->priv->main_window));
}

static void
ario_tray_icon_set_visibility (ArioTrayIcon *icon,
                               int state)
{
        ARIO_LOG_FUNCTION_START
        switch (state)
        {
        case VISIBILITY_HIDDEN:
               case VISIBILITY_VISIBLE:
                if (icon->priv->visible != state)
                        ario_tray_icon_set_visibility (icon, VISIBILITY_TOGGLE);
                break;
        case VISIBILITY_TOGGLE:
                icon->priv->visible = !icon->priv->visible;

                if (icon->priv->visible == TRUE) {
                        ario_tray_icon_restore_main_window (icon);
                        gtk_widget_show (GTK_WIDGET (icon->priv->main_window));
                } else {
                        icon->priv->maximized = eel_gconf_get_boolean (CONF_WINDOW_MAXIMIZED);
                        gtk_window_get_position (icon->priv->main_window,
                                                 &icon->priv->window_x,
                                                 &icon->priv->window_y);
                        gtk_window_get_size (icon->priv->main_window,
                                             &icon->priv->window_w,
                                             &icon->priv->window_h);
                        gtk_widget_hide (GTK_WIDGET (icon->priv->main_window));
                }
        }
}
