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
#include "lib/ario-conf.h"
#include "lib/libsexy/sexy-tooltip.h"
#include "widgets/ario-tray-icon.h"
#include "shell/ario-shell.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"
#include "covers/ario-cover-handler.h"

#define TRAY_ICON_DEFAULT_TOOLTIP _("Not playing")
#define FROM_MARKUP(xALBUM, xARTIST) g_markup_printf_escaped (_("<i>from</i> %s <i>by</i> %s"), xALBUM, xARTIST);

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
static gboolean ario_tray_icon_update_tooltip_visibility (ArioTrayIcon *icon);
#ifdef ENABLE_EGGTRAYICON
static void ario_tray_icon_enter_notify_event_cb (ArioTrayIcon *icon,
                                                  GdkEvent *event,
                                                  GtkWidget *widget);
static void ario_tray_icon_leave_notify_event_cb (ArioTrayIcon *icon,
                                                  GdkEvent *event,
                                                  GtkWidget *widget);
static gboolean ario_tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                                      ArioTrayIcon *icon);
static gboolean ario_tray_icon_scroll_cb (GtkWidget *widget, GdkEvent *event,
                                          ArioTrayIcon *icon);
static void ario_tray_icon_changed_cb (guint notification_id,
                                       ArioTrayIcon *icon);
#else
static void ario_tray_icon_activate_cb (GtkStatusIcon *tray_icon,
                                        ArioTrayIcon *icon);
static void ario_tray_icon_popup_cb (GtkStatusIcon *tray_icon,
                                     guint button,
                                     guint activate_time,
                                     ArioTrayIcon *icon);
#endif
static void ario_tray_icon_tooltip_size_allocate_cb (ArioTrayIcon *icon,
                                                     GtkAllocation *allocation,
                                                     GtkWidget *tooltip);
static void ario_tray_icon_construct_tooltip (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_song (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_album (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_cover (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_time (ArioTrayIcon *icon);
static void ario_tray_icon_sync_icon (ArioTrayIcon *icon);
static void ario_tray_icon_sync_popup (ArioTrayIcon *icon);
static void ario_tray_icon_song_changed_cb (ArioMpd *mpd,
                                            ArioTrayIcon *icon);
static void ario_tray_icon_album_changed_cb (ArioMpd *mpd,
                                             ArioTrayIcon *icon);
static void ario_tray_icon_state_changed_cb (ArioMpd *mpd,
                                             ArioTrayIcon *icon);
static void ario_tray_icon_time_changed_cb (ArioMpd *mpd,
                                            ArioTrayIcon *icon);
static void ario_tray_icon_cover_changed_cb (ArioCoverHandler *cover_handler,
                                             ArioTrayIcon *icon);
static void ario_tray_icon_cmd_play (GtkAction *action,
                                     ArioTrayIcon *icon);
static void ario_tray_icon_cmd_pause (GtkAction *action,
                                      ArioTrayIcon *icon);
static void ario_tray_icon_cmd_stop (GtkAction *action,
                                     ArioTrayIcon *icon);
static void ario_tray_icon_cmd_next (GtkAction *action,
                                     ArioTrayIcon *icon);
static void ario_tray_icon_cmd_previous (GtkAction *action,
                                         ArioTrayIcon *icon);

struct ArioTrayIconPrivate
{
        GtkWidget *tooltip;
        gboolean tooltips_pointer_above;
        GtkWidget *tooltip_primary;
        GtkWidget *tooltip_secondary;
        GtkWidget *tooltip_progress_bar;
        GtkWidget *tooltip_image_box;
        GtkWidget *cover_image;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;
#ifdef ENABLE_EGGTRAYICON
        GtkWidget *ebox;

        GtkWidget *image;
        GtkWidget *image_play;
        GtkWidget *image_pause;
#endif
        ArioMpd *mpd;
        ArioShell *shell;

        guint timeout_id;
        guint notification_id;

        gboolean notified;
        gboolean shown;
};

static GtkActionEntry ario_tray_icon_actions [] =
{
        { "ControlPlay", GTK_STOCK_MEDIA_PLAY, N_("_Play"), "<control>Up",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_play) },
        { "ControlPause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "<control>Down",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_pause) },
        { "ControlStop", GTK_STOCK_MEDIA_STOP, N_("_Stop"), NULL,
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_stop) },
        { "ControlNext", GTK_STOCK_MEDIA_NEXT, N_("_Next"), "<control>Right",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_next) },
        { "ControlPrevious", GTK_STOCK_MEDIA_PREVIOUS, N_("P_revious"), "<control>Left",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_previous) },
};

static guint ario_tray_icon_n_actions = G_N_ELEMENTS (ario_tray_icon_actions);

enum
{
        PROP_0,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP,
        PROP_SHELL,
        PROP_MPD
};

#define ARIO_TRAY_ICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TRAY_ICON, ArioTrayIconPrivate))
#ifdef ENABLE_EGGTRAYICON
G_DEFINE_TYPE (ArioTrayIcon, ario_tray_icon, EGG_TYPE_TRAY_ICON)
#else
G_DEFINE_TYPE (ArioTrayIcon, ario_tray_icon, GTK_TYPE_STATUS_ICON)
#endif

static void
ario_tray_icon_class_init (ArioTrayIconClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_tray_icon_finalize;
        object_class->constructor = ario_tray_icon_constructor;

        object_class->set_property = ario_tray_icon_set_property;
        object_class->get_property = ario_tray_icon_get_property;

        g_object_class_install_property (object_class,
                                         PROP_SHELL,
                                         g_param_spec_object ("shell",
                                                              "ArioShell",
                                                              "shell",
                                                              TYPE_ARIO_SHELL,
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
        g_object_class_install_property (object_class,
                                         PROP_ACTION_GROUP,
                                         g_param_spec_object ("action-group",
                                                              "GtkActionGroup",
                                                              "GtkActionGroup object",
                                                              GTK_TYPE_ACTION_GROUP,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (klass, sizeof (ArioTrayIconPrivate));
}

static void
ario_tray_icon_init (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START

        icon->priv = ARIO_TRAY_ICON_GET_PRIVATE (icon);

        ario_tray_icon_construct_tooltip (icon);

#ifdef ENABLE_EGGTRAYICON
        icon->priv->ebox = gtk_event_box_new ();
        g_signal_connect (icon->priv->ebox,
                          "button_press_event",
                          G_CALLBACK (ario_tray_icon_button_press_event_cb),
                          icon);
        g_signal_connect (icon->priv->ebox,
                          "scroll_event",
                          G_CALLBACK (ario_tray_icon_scroll_cb),
                          icon);
        g_signal_connect_swapped (icon->priv->ebox,
                                  "enter-notify-event",
                                  G_CALLBACK (ario_tray_icon_enter_notify_event_cb),
                                  icon);
        g_signal_connect_swapped (icon->priv->ebox,
                                  "leave-notify-event",
                                  G_CALLBACK (ario_tray_icon_leave_notify_event_cb),
                                  icon);
        icon->priv->image = gtk_image_new_from_stock ("ario",
                                                      GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_object_ref (icon->priv->image);
        icon->priv->image_play = gtk_image_new_from_stock ("ario-play",
                                                           GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_object_ref (icon->priv->image_play);
        icon->priv->image_pause = gtk_image_new_from_stock ("ario-pause",
                                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
        g_object_ref (icon->priv->image_pause);

        gtk_container_add (GTK_CONTAINER (icon->priv->ebox), icon->priv->image);

        gtk_container_add (GTK_CONTAINER (icon), icon->priv->ebox);
        gtk_widget_show_all (GTK_WIDGET (icon->priv->ebox));
#else
        gtk_status_icon_set_tooltip (GTK_STATUS_ICON (icon), TRAY_ICON_DEFAULT_TOOLTIP);
        gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario");
        g_signal_connect (icon,
                          "activate",
                          G_CALLBACK (ario_tray_icon_activate_cb),
                          icon);
        g_signal_connect (icon,
                          "popup-menu",
                          G_CALLBACK (ario_tray_icon_popup_cb),
                          icon);
#endif
        g_signal_connect_object (ario_cover_handler_get_instance (),
                                 "cover_changed",
                                 G_CALLBACK (ario_tray_icon_cover_changed_cb),
                                 icon, 0);
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

        /* TODO : Not very efficient */
        ario_mpd_use_count_dec (tray->priv->mpd);

        gtk_object_destroy (GTK_OBJECT (tray->priv->tooltip));
#ifdef ENABLE_EGGTRAYICON
        g_object_unref (tray->priv->image);
        g_object_unref (tray->priv->image_play);
        g_object_unref (tray->priv->image_pause);
#endif
        G_OBJECT_CLASS (ario_tray_icon_parent_class)->finalize (object);
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
        case PROP_SHELL:
                tray->priv->shell = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                tray->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_MPD:
                tray->priv->mpd = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                tray->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (tray->priv->actiongroup,
                                              ario_tray_icon_actions,
                                              ario_tray_icon_n_actions, tray);
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
        case PROP_SHELL:
                g_value_set_object (value, tray->priv->shell);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, tray->priv->ui_manager);
                break;
        case PROP_MPD:
                g_value_set_object (value, tray->priv->mpd);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, tray->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

ArioTrayIcon *
ario_tray_icon_new (GtkActionGroup *group,
                    GtkUIManager *mgr,
                    ArioShell *shell,
                    ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START

        ArioTrayIcon *icon = g_object_new (TYPE_ARIO_TRAY_ICON,
                                           "action-group", group,
#ifdef ENABLE_EGGTRAYICON
                                           "title", "Ario",
#endif
                                           "ui-manager", mgr,
                                           "shell", shell,
                                           "mpd", mpd,
                                           NULL);

        /* TODO : Not very efficient */
        ario_mpd_use_count_inc (icon->priv->mpd);

        g_signal_connect_object (icon->priv->mpd,
                                 "song_changed",
                                 G_CALLBACK (ario_tray_icon_song_changed_cb),
                                 icon, 0);
        g_signal_connect_object (icon->priv->mpd,
                                 "album_changed",
                                 G_CALLBACK (ario_tray_icon_album_changed_cb),
                                 icon, 0);
        g_signal_connect_object (icon->priv->mpd,
                                 "state_changed",
                                 G_CALLBACK (ario_tray_icon_state_changed_cb),
                                 icon, 0);
        g_signal_connect_object (icon->priv->mpd,
                                 "playlist_changed",
                                 G_CALLBACK (ario_tray_icon_state_changed_cb),
                                 icon, 0);
        g_signal_connect_object (icon->priv->mpd,
                                 "elapsed_changed",
                                 G_CALLBACK (ario_tray_icon_time_changed_cb),
                                 icon, 0);
#ifdef ENABLE_EGGTRAYICON
        ario_conf_notification_add (PREF_TRAY_ICON,
                                    (ArioNotifyFunc) ario_tray_icon_changed_cb,
                                    icon);
#endif

        return icon;
}

static gboolean
ario_tray_icon_update_tooltip_visibility (ArioTrayIcon *icon)
{
        if (icon->priv->tooltips_pointer_above || icon->priv->notified) {
                icon->priv->shown = TRUE;
                ario_tray_icon_sync_tooltip_time (icon);
#ifdef ENABLE_EGGTRAYICON
                sexy_tooltip_position_to_widget (SEXY_TOOLTIP (icon->priv->tooltip),
                                                 icon->priv->ebox);
#else
                GdkRectangle rect;
                GdkScreen *screen;
                gtk_status_icon_get_geometry (GTK_STATUS_ICON (icon),
                                              &screen,
                                              &rect,
                                              NULL);

                sexy_tooltip_position_to_rect (SEXY_TOOLTIP (icon->priv->tooltip), &rect, screen);
#endif
                gtk_widget_show (icon->priv->tooltip);
        } else {
                icon->priv->shown = FALSE;
                gtk_widget_hide (icon->priv->tooltip);
        }

        icon->priv->notified = FALSE;
        icon->priv->timeout_id = 0;
        icon->priv->notification_id = 0;

        return FALSE;
}
#ifdef ENABLE_EGGTRAYICON
static void
ario_tray_icon_enter_notify_event_cb (ArioTrayIcon *icon,
                                      GdkEvent *event,
                                      GtkWidget *widget)
{
        icon->priv->tooltips_pointer_above = TRUE;
        ario_mpd_use_count_inc (icon->priv->mpd);

        if (icon->priv->timeout_id)
                g_source_remove (icon->priv->timeout_id);
        icon->priv->timeout_id = g_timeout_add (500,
                                                (GSourceFunc) ario_tray_icon_update_tooltip_visibility,
                                                icon);
}

static void
ario_tray_icon_leave_notify_event_cb (ArioTrayIcon *icon,
                                      GdkEvent *event,
                                      GtkWidget *widget)
{
        if (icon->priv->tooltips_pointer_above) {
                ario_mpd_use_count_dec (icon->priv->mpd);
                icon->priv->tooltips_pointer_above = FALSE;
        }

        if (icon->priv->timeout_id)
                g_source_remove (icon->priv->timeout_id);
        ario_tray_icon_update_tooltip_visibility (icon);
}

static void
ario_tray_icon_middle_click (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        int trayicon_behavior;

        trayicon_behavior = ario_conf_get_integer (PREF_TRAYICON_BEHAVIOR, PREF_TRAYICON_BEHAVIOR_DEFAULT);

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

static gboolean
ario_tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                      ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *popup;

        /* filter out double, triple clicks */
        if (event->type != GDK_BUTTON_PRESS)
                return FALSE;

        switch (event->button) {
        case 1:
                ario_shell_set_visibility (icon->priv->shell, VISIBILITY_TOGGLE);
                break;

        case 2:
                ario_tray_icon_middle_click (icon);
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

        return TRUE;
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
ario_tray_icon_changed_cb (guint notification_id,
                           ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        if (ario_conf_get_boolean (PREF_TRAY_ICON, PREF_TRAY_ICON_DEFAULT))
                gtk_widget_show_all (GTK_WIDGET (icon));
        else
                gtk_widget_hide (GTK_WIDGET (icon));
}
#else
static void
ario_tray_icon_activate_cb (GtkStatusIcon *tray_icon,
                            ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START

        ario_shell_set_visibility (icon->priv->shell, VISIBILITY_TOGGLE);
}

static void
ario_tray_icon_popup_cb (GtkStatusIcon *tray_icon,
                         guint button,
                         guint activate_time,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *popup;

        popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (icon->priv->ui_manager),
                                           "/TrayPopup");

        gtk_menu_set_screen (GTK_MENU (popup), gdk_screen_get_default());

        gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                        NULL, NULL, button,
                        gtk_get_current_event_time ());
}
#endif

static void
ario_tray_icon_tooltip_size_allocate_cb (ArioTrayIcon *icon,
                                         GtkAllocation *allocation,
                                         GtkWidget *tooltip)
{
#ifdef ENABLE_EGGTRAYICON
        sexy_tooltip_position_to_widget (SEXY_TOOLTIP (icon->priv->tooltip),
                                         icon->priv->ebox);
#else
        GdkRectangle rect;
        GdkScreen *screen;
        gtk_status_icon_get_geometry (GTK_STATUS_ICON (icon),
                                      &screen,
                                      &rect,
                                      NULL);

        sexy_tooltip_position_to_rect (SEXY_TOOLTIP (icon->priv->tooltip), &rect, screen);
#endif
}

static void
ario_tray_icon_construct_tooltip (ArioTrayIcon *icon)
{
        GtkWidget *hbox, *vbox;
        gint size;
        PangoFontDescription *font_desc;

        icon->priv->tooltip = sexy_tooltip_new ();

        g_signal_connect_object (icon->priv->tooltip, "size-allocate",
                                 (GCallback) ario_tray_icon_tooltip_size_allocate_cb,
                                 icon, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

        icon->priv->tooltip_primary = gtk_label_new (TRAY_ICON_DEFAULT_TOOLTIP);
        gtk_widget_modify_font (icon->priv->tooltip_primary, NULL);
        size = pango_font_description_get_size (icon->priv->tooltip_primary->style->font_desc);
        font_desc = pango_font_description_new ();
        pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
        pango_font_description_set_size (font_desc, size * PANGO_SCALE_LARGE);
        gtk_widget_modify_font (icon->priv->tooltip_primary, font_desc);
        pango_font_description_free (font_desc);
        gtk_label_set_line_wrap (GTK_LABEL (icon->priv->tooltip_primary),
                                 TRUE);
        gtk_misc_set_alignment  (GTK_MISC  (icon->priv->tooltip_primary),
                                 0.0, 0.0);

        icon->priv->tooltip_secondary = gtk_label_new (NULL);
        gtk_label_set_line_wrap (GTK_LABEL (icon->priv->tooltip_secondary),
                                 TRUE);
        gtk_misc_set_alignment  (GTK_MISC  (icon->priv->tooltip_secondary),
                                 0.0, 0.0);
        icon->priv->cover_image = gtk_image_new ();
        icon->priv->tooltip_image_box = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (icon->priv->tooltip_image_box), icon->priv->cover_image,
                            TRUE, FALSE, 0);

        hbox = gtk_hbox_new (FALSE, 6);
        vbox = gtk_vbox_new (FALSE, 2);
        icon->priv->tooltip_progress_bar = gtk_progress_bar_new ();

        gtk_box_pack_start (GTK_BOX (vbox), icon->priv->tooltip_primary,
                            FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), icon->priv->tooltip_secondary,
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), icon->priv->tooltip_progress_bar,
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), icon->priv->tooltip_image_box,
                            FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), vbox,
                            TRUE, TRUE, 0);
        gtk_widget_show_all (hbox);

        gtk_container_add (GTK_CONTAINER (icon->priv->tooltip), hbox);
}
#ifndef ENABLE_EGGTRAYICON
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
                tooltip = g_strdup_printf ("%s: %s\n%s: %s\n%s: %s",
                                           _("Artist"),
                                           ario_mpd_get_current_artist (icon->priv->mpd) ? ario_mpd_get_current_artist (icon->priv->mpd) : ARIO_MPD_UNKNOWN,
                                           _("Album"),
                                           ario_mpd_get_current_album (icon->priv->mpd) ? ario_mpd_get_current_album (icon->priv->mpd) : ARIO_MPD_UNKNOWN,
                                           _("Title"),
                                           title);
                g_free (title);
                break;
        default:
                tooltip = g_strdup (TRAY_ICON_DEFAULT_TOOLTIP);
                break;
        }

        gtk_status_icon_set_tooltip (GTK_STATUS_ICON (icon), tooltip);

        g_free (tooltip);
}
#endif
static void
ario_tray_icon_sync_tooltip_song (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        gchar *title;

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Title */
                title = ario_util_format_title (ario_mpd_get_current_song (icon->priv->mpd));
                gtk_label_set_text (GTK_LABEL (icon->priv->tooltip_primary),
                                    title);
                g_free (title);
                break;
        default:
                gtk_label_set_text (GTK_LABEL (icon->priv->tooltip_primary),
                                    TRAY_ICON_DEFAULT_TOOLTIP);
                break;
        }
}


static void
ario_tray_icon_sync_tooltip_album (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        gchar *artist;
        gchar *album;
        gchar *secondary;

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Artist - Album */
                artist = ario_mpd_get_current_artist (icon->priv->mpd);
                album = ario_mpd_get_current_album (icon->priv->mpd);

                if (!album)
                        album = ARIO_MPD_UNKNOWN;
                if (!artist)
                        artist = ARIO_MPD_UNKNOWN;

                secondary = FROM_MARKUP (album, artist);
                gtk_label_set_markup (GTK_LABEL (icon->priv->tooltip_secondary),
                                      secondary);
                gtk_widget_show (icon->priv->tooltip_secondary);
                g_free(secondary);
                break;
        default:
                gtk_label_set_markup (GTK_LABEL (icon->priv->tooltip_secondary),
                                      "");
                gtk_widget_hide (icon->priv->tooltip_secondary);
                break;
        }
}


static void
ario_tray_icon_sync_tooltip_cover (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GdkPixbuf *cover;

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                /* Icon */
                cover = ario_cover_handler_get_cover();
                if (cover) {
                        gtk_image_set_from_pixbuf (GTK_IMAGE (icon->priv->cover_image), cover);
                        gtk_widget_show (icon->priv->cover_image);
                } else {
                        gtk_widget_hide (icon->priv->cover_image);
                }
                break;
        default:
                gtk_widget_hide (icon->priv->cover_image);
                break;
        }
}

static void
ario_tray_icon_sync_tooltip_time (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        int elapsed;
        int total;
        gchar *elapsed_char;
        gchar *total_char;
        gchar *time;

        if (!icon->priv->shown)
                return;

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                elapsed = ario_mpd_get_current_elapsed (icon->priv->mpd);
                elapsed_char = ario_util_format_time (elapsed);
                total = ario_mpd_get_current_total_time (icon->priv->mpd);
                if (total) {
                        total_char = ario_util_format_time (total);
                        time = g_strdup_printf ("%s%s%s", elapsed_char, _(" of "), total_char);
                        g_free (total_char);
                } else {
                        time = g_strdup_printf ("%s", elapsed_char);
                }
                g_free (elapsed_char);
                gtk_progress_bar_set_text (GTK_PROGRESS_BAR (icon->priv->tooltip_progress_bar), time);
                g_free (time);
                if (total > 0)
                        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (icon->priv->tooltip_progress_bar),
                                                       (double) elapsed / (double) total);
                else
                        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (icon->priv->tooltip_progress_bar),
                                                       0);
                gtk_widget_show (icon->priv->tooltip_progress_bar);
                break;
        default:
                gtk_progress_bar_set_text (GTK_PROGRESS_BAR (icon->priv->tooltip_progress_bar), NULL);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (icon->priv->tooltip_progress_bar),
                                               0);
                gtk_widget_hide (icon->priv->tooltip_progress_bar);
                break;
        }
}

static void
ario_tray_icon_sync_icon (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
#ifdef ENABLE_EGGTRAYICON
        gtk_container_remove (GTK_CONTAINER (icon->priv->ebox),
                              GTK_WIDGET (gtk_container_get_children (GTK_CONTAINER (icon->priv->ebox))->data));

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
                gtk_container_add (GTK_CONTAINER (icon->priv->ebox), icon->priv->image_play);
                break;
        case MPD_STATUS_STATE_PAUSE:
                gtk_container_add (GTK_CONTAINER (icon->priv->ebox), icon->priv->image_pause);
                break;
        default:
                gtk_container_add (GTK_CONTAINER (icon->priv->ebox), icon->priv->image);
                break;
        }
        gtk_widget_show_all (GTK_WIDGET (icon->priv->ebox));
#else
        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
                gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario-play");
                break;
        case MPD_STATUS_STATE_PAUSE:
                gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario-pause");
                break;
        default:
                gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario");
                break;
        }
#endif
}

static void
ario_tray_icon_sync_popup (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        int state = ario_mpd_get_current_state (icon->priv->mpd);

        gtk_action_set_visible (gtk_action_group_get_action (icon->priv->actiongroup, "ControlPlay"),
                                state != MPD_STATUS_STATE_PLAY);
        gtk_action_set_visible (gtk_action_group_get_action (icon->priv->actiongroup, "ControlPause"),
                                state == MPD_STATUS_STATE_PLAY);
}

static void
ario_tray_icon_song_changed_cb (ArioMpd *mpd,
                                ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip_song (icon);
        ario_tray_icon_sync_tooltip_time (icon);

#ifndef ENABLE_EGGTRAYICON
        ario_tray_icon_sync_tooltip (icon);
#endif

        if (ario_conf_get_boolean (PREF_HAVE_NOTIFICATION, PREF_HAVE_NOTIFICATION_DEFAULT)) {
                icon->priv->notified = TRUE;
                ario_tray_icon_update_tooltip_visibility (icon);

                if (icon->priv->notification_id)
                        g_source_remove (icon->priv->notification_id);

                icon->priv->notification_id = g_timeout_add (ario_conf_get_integer (PREF_NOTIFICATION_TIME, PREF_NOTIFICATION_TIME_DEFAULT) * 1000,
                                                             (GSourceFunc) ario_tray_icon_update_tooltip_visibility,
                                                             icon);
        }
}

static void
ario_tray_icon_album_changed_cb (ArioMpd *mpd,
                                 ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip_album (icon);
}

static void
ario_tray_icon_state_changed_cb (ArioMpd *mpd,
                                 ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip_song (icon);
        ario_tray_icon_sync_tooltip_album (icon);
        ario_tray_icon_sync_tooltip_time (icon);
        ario_tray_icon_sync_icon (icon);
        ario_tray_icon_sync_popup (icon);

#ifndef ENABLE_EGGTRAYICON
        ario_tray_icon_sync_tooltip (icon);
#endif
}

static void
ario_tray_icon_time_changed_cb (ArioMpd *mpd,
                                ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip_time (icon);
}

static void
ario_tray_icon_cover_changed_cb (ArioCoverHandler *cover_handler,
                                 ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_tray_icon_sync_tooltip_cover (icon);
}

static void
ario_tray_icon_cmd_play (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_play (icon->priv->mpd);
        ario_mpd_update_status (icon->priv->mpd);
}

static void
ario_tray_icon_cmd_pause (GtkAction *action,
                          ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_pause (icon->priv->mpd);
        ario_mpd_update_status (icon->priv->mpd);
}

static void
ario_tray_icon_cmd_stop (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_stop (icon->priv->mpd);
        ario_mpd_update_status (icon->priv->mpd);
}

static void
ario_tray_icon_cmd_next (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_next (icon->priv->mpd);
}

static void
ario_tray_icon_cmd_previous (GtkAction *action,
                             ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_prev (icon->priv->mpd);
}

