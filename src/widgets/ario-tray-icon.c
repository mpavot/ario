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

#include "widgets/ario-tray-icon.h"
#include <gtk/gtk.h>
#include <config.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "widgets/ario-tooltip.h"

/* Windows cannot display a custom widget as tooltip */
#ifndef WIN32
#define CUSTOM_TOOLTIP 1
#endif

static void ario_tray_icon_finalize (GObject *object);
static void ario_tray_icon_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void ario_tray_icon_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);
static void ario_tray_icon_activate_cb (GtkStatusIcon *tray_icon,
                                        ArioTrayIcon *icon);
static void ario_tray_icon_changed_cb (guint notification_id,
                                       ArioTrayIcon *icon);
static gboolean ario_tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                                      ArioTrayIcon *icon);
static gboolean ario_tray_icon_scroll_cb (GtkWidget *widget, GdkEvent *event,
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
static void ario_tray_icon_sync_icon (ArioTrayIcon *icon);
#ifdef CUSTOM_TOOLTIP
static gboolean ario_tray_icon_query_tooltip_cb (GtkStatusIcon *status_icon,
                                                 gint x, gint y,
                                                 gboolean keyboard_mode,
                                                 GtkTooltip *tooltip,
                                                 ArioTrayIcon *icon);
#else
static void ario_tray_icon_sync_tooltip_text (ArioTrayIcon *icon);
static void ario_tray_icon_song_changed_cb (ArioServer *server,
                                            ArioTrayIcon *icon);
#endif
static void ario_tray_icon_state_changed_cb (ArioServer *server,
                                             ArioTrayIcon *icon);

struct ArioTrayIconPrivate
{
        GtkWidget *tooltip;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        ArioShell *shell;

        int volume;
};

static ArioTrayIcon *instance = NULL;

/* Actions */
static GtkActionEntry ario_tray_icon_actions [] =
{
        { "ControlPlay", GTK_STOCK_MEDIA_PLAY, N_("_Play"), "<control>Up",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_play) },
        { "ControlPause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "<control>Down",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_pause) },
        { "ControlStop", GTK_STOCK_MEDIA_STOP, N_("_Stop"), "<control>space",
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

/* Object properties */
enum
{
        PROP_0,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP,
        PROP_SHELL
};

#define ARIO_TRAY_ICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TRAY_ICON, ArioTrayIconPrivate))
G_DEFINE_TYPE (ArioTrayIcon, ario_tray_icon, GTK_TYPE_STATUS_ICON)

static void
ario_tray_icon_class_init (ArioTrayIconClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* Virtual methods */
        object_class->finalize = ario_tray_icon_finalize;
        object_class->set_property = ario_tray_icon_set_property;
        object_class->get_property = ario_tray_icon_get_property;

        /* Object properties */
        g_object_class_install_property (object_class,
                                         PROP_SHELL,
                                         g_param_spec_object ("shell",
                                                              "ArioShell",
                                                              "shell",
                                                              ARIO_TYPE_SHELL,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_ACTION_GROUP,
                                         g_param_spec_object ("action-group",
                                                              "GtkActionGroup",
                                                              "GtkActionGroup object",
                                                              GTK_TYPE_ACTION_GROUP,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioTrayIconPrivate));
}

static void
ario_tray_icon_init (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;

        icon->priv = ARIO_TRAY_ICON_GET_PRIVATE (icon);

        /* Set icon */
        gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario");

        /* Connext signals for actions on icon */
        g_signal_connect (icon,
                          "activate",
                          G_CALLBACK (ario_tray_icon_activate_cb),
                          icon);
        g_signal_connect (icon,
                          "button_press_event",
                          G_CALLBACK (ario_tray_icon_button_press_event_cb),
                          icon);
        g_signal_connect (icon,
                          "scroll_event",
                          G_CALLBACK (ario_tray_icon_scroll_cb),
                          icon);

#ifdef CUSTOM_TOOLTIP
        /* This is needed for query-tooltip signal to work */
        gtk_status_icon_set_has_tooltip (GTK_STATUS_ICON (icon), TRUE);

        g_signal_connect (icon,
                          "query-tooltip",
                          G_CALLBACK (ario_tray_icon_query_tooltip_cb),
                          icon);
#else
        /* Set tooltip text */
        gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (icon), TRAY_ICON_DEFAULT_TOOLTIP);
#endif
}

static void
ario_tray_icon_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioTrayIcon *tray;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_TRAY_ICON (object));

        tray = ARIO_TRAY_ICON (object);

        g_return_if_fail (tray->priv != NULL);

        if (tray->priv->tooltip)
                gtk_object_destroy (GTK_OBJECT (tray->priv->tooltip));

        G_OBJECT_CLASS (ario_tray_icon_parent_class)->finalize (object);
}

static void
ario_tray_icon_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioTrayIcon *tray = ARIO_TRAY_ICON (object);

        switch (prop_id)
        {
        case PROP_SHELL:
                tray->priv->shell = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                tray->priv->ui_manager = g_value_get_object (value);
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
        ARIO_LOG_FUNCTION_START;
        ArioTrayIcon *tray = ARIO_TRAY_ICON (object);

        switch (prop_id)
        {
        case PROP_SHELL:
                g_value_set_object (value, tray->priv->shell);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, tray->priv->ui_manager);
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
                    ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        ArioTrayIcon *icon = g_object_new (TYPE_ARIO_TRAY_ICON,
                                           "action-group", group,
                                           "ui-manager", mgr,
                                           "shell", shell,
                                           NULL);
        instance = icon;

        ArioServer *server = ario_server_get_instance ();

#ifndef CUSTOM_TOOLTIP
        /* Connect signals for synchonization with server */
        g_signal_connect_object (server,
                                 "song_changed",
                                 G_CALLBACK (ario_tray_icon_song_changed_cb),
                                 icon, 0);
#endif
        g_signal_connect_object (server,
                                 "state_changed",
                                 G_CALLBACK (ario_tray_icon_state_changed_cb),
                                 icon, 0);
        g_signal_connect_object (server,
                                 "playlist_changed",
                                 G_CALLBACK (ario_tray_icon_state_changed_cb),
                                 icon, 0);

        /* Synchonization with preferences */
        ario_conf_notification_add (PREF_TRAY_ICON,
                                    (ArioNotifyFunc) ario_tray_icon_changed_cb,
                                    icon);
        ario_tray_icon_changed_cb (0, icon);

        return icon;
}

static void
ario_tray_icon_activate_cb (GtkStatusIcon *tray_icon,
                            ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Show/hide main window */
        ario_shell_set_visibility (icon->priv->shell, VISIBILITY_TOGGLE);
}

static void
ario_tray_icon_middle_click (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        int trayicon_behavior;
        int volume;

        /* Get user preference */
        trayicon_behavior = ario_conf_get_integer (PREF_TRAYICON_BEHAVIOR, PREF_TRAYICON_BEHAVIOR_DEFAULT);

        switch (trayicon_behavior) {
        case TRAY_ICON_PLAY_PAUSE:
                /* Play/Pause */
                if (ario_server_is_paused ())
                        ario_server_do_play ();
                else
                        ario_server_do_pause ();
                break;

        case TRAY_ICON_NEXT_SONG:
                /* Go to next song */
                ario_server_do_next ();
                break;

        case TRAY_ICON_MUTE:
                volume = ario_server_get_current_volume ();
                if (volume > 0) {
                        /* Mute */
                        icon->priv->volume = volume;
                        ario_server_set_current_volume (0);
                } else {
                        /* Restore volume */
                        ario_server_set_current_volume (icon->priv->volume);
                }
                break;

        case TRAY_ICON_DO_NOTHING:
        default:
                /* Do nothing */
                break;
        }
}

static gboolean
ario_tray_icon_button_press_event_cb (GtkWidget *ebox,
                                      GdkEventButton *event,
                                      ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *popup;

        /* Filter out double, triple clicks */
        if (event->type != GDK_BUTTON_PRESS)
                return FALSE;

        switch (event->button) {
        case 1:
                /* First button shows/hides Ario */
                ario_shell_set_visibility (icon->priv->shell, VISIBILITY_TOGGLE);
                break;
        case 2:
                /* Second button action is configurable */
                ario_tray_icon_middle_click (icon);
                break;
        case 3:
                /* Third button shows popup menu */
                popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (icon->priv->ui_manager),
                                                   "/TrayPopup");
                gtk_menu_set_screen (GTK_MENU (popup), gdk_screen_get_default());
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
        ARIO_LOG_FUNCTION_START;
        int volume_adjust_step;
        gint vol;

        volume_adjust_step = ario_conf_get_integer (PREF_VOL_ADJUST_STEP, PREF_VOL_ADJUST_STEP_DEFAULT);
        vol = ario_server_get_current_volume ();

        switch (event->scroll.direction) {
        case GDK_SCROLL_UP:
                /* Increment volume */
                vol += volume_adjust_step;
                if (vol > 100)
                        vol = 100;
                break;
        case GDK_SCROLL_DOWN:
                /* Decrement volume */
                vol -= volume_adjust_step;
                if (vol < 0)
                        vol = 0;
                break;
        default:
                break;
        }

        /* Change volume on server */
        ario_server_set_current_volume (vol);

        return FALSE;
}

static void
ario_tray_icon_changed_cb (guint notification_id,
                           ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;

        /* Synchronize icon visibility with user preferences */
        gtk_status_icon_set_visible (GTK_STATUS_ICON (icon),
                                     ario_conf_get_boolean (PREF_TRAY_ICON, PREF_TRAY_ICON_DEFAULT));
}

static void
ario_tray_icon_cmd_play (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Send play command to server */
        ario_server_do_play ();
        ario_server_update_status ();
}

static void
ario_tray_icon_cmd_pause (GtkAction *action,
                          ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Send pause command to server */
        ario_server_do_pause ();
        ario_server_update_status ();
}

static void
ario_tray_icon_cmd_stop (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Send stop command to server */
        ario_server_do_stop ();
        ario_server_update_status ();
}

static void
ario_tray_icon_cmd_next (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Send next song command to server */
        ario_server_do_next ();
}

static void
ario_tray_icon_cmd_previous (GtkAction *action,
                             ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Send previous song command to server */
        ario_server_do_prev ();
}

ArioTrayIcon *
ario_tray_icon_get_instance (void)
{
        ARIO_LOG_FUNCTION_START;
        return instance;
}

static void
ario_tray_icon_sync_icon (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        switch (ario_server_get_current_state ()) {
        case ARIO_STATE_PLAY:
                /* Add 'play' icon */
                gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario-play");
                break;
        case ARIO_STATE_PAUSE:
                /* Add 'pause' icon */
                gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario-pause");
                break;
        default:
                /* Add default icon */
                gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario");
                break;
        }
}

#ifdef CUSTOM_TOOLTIP
static gboolean
ario_tray_icon_query_tooltip_cb (GtkStatusIcon *status_icon,
                                 gint x, gint y,
                                 gboolean keyboard_mode,
                                 GtkTooltip *tooltip,
                                 ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        if (!icon->priv->tooltip)
        {
                icon->priv->tooltip = ario_tooltip_new ();
                g_object_ref (icon->priv->tooltip);
        }

        gtk_tooltip_set_custom (tooltip, icon->priv->tooltip);
        return TRUE;
}
#else
static void
ario_tray_icon_sync_tooltip_text (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        gchar *tooltip;
        gchar *title;

        switch (ario_server_get_current_state ()) {
        case ARIO_STATE_PLAY:
        case ARIO_STATE_PAUSE:
                /* Change tooltip with server information */
                title = ario_util_format_title(ario_server_get_current_song ());
                tooltip = g_strdup_printf ("%s: %s\n%s: %s\n%s: %s",
                                           _("Artist"),
                                           ario_server_get_current_artist () ? ario_server_get_current_artist () : ARIO_SERVER_UNKNOWN,
                                           _("Album"),
                                           ario_server_get_current_album () ? ario_server_get_current_album () : ARIO_SERVER_UNKNOWN,
                                           _("Title"),
                                           title);

                gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (icon), tooltip);
                g_free (tooltip);
                break;
        default:
                /* Set default tooltip */
                gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (icon), TRAY_ICON_DEFAULT_TOOLTIP);
                break;
        }
}

static void
ario_tray_icon_song_changed_cb (ArioServer *server,
                                ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize tooltip info */
        ario_tray_icon_sync_tooltip_text (icon);
}
#endif

static void
ario_tray_icon_state_changed_cb (ArioServer *server,
                                 ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START;
        /* Synchronize icon */
        ario_tray_icon_sync_icon (icon);

#ifndef CUSTOM_TOOLTIP
        /* Synchronize tooltip info */
        ario_tray_icon_sync_tooltip_text (icon);
#endif
}
