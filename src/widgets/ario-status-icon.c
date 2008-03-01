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

#include <gtk/gtk.h>
#include <config.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "widgets/ario-status-icon.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define STATUS_ICON_DEFAULT_TOOLTIP _("Not playing")
#define FROM_MARKUP(xALBUM, xARTIST) g_markup_printf_escaped (_("<i>from</i> %s <i>by</i> %s"), xALBUM, xARTIST);

static void ario_status_icon_class_init (ArioStatusIconClass *klass);
static void ario_status_icon_init (ArioStatusIcon *ario_shell_player);
static GObject *ario_status_icon_constructor (GType type, guint n_construct_properties,
                                            GObjectConstructParam *construct_properties);
static void ario_status_icon_finalize (GObject *object);
static void ario_status_icon_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void ario_status_icon_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);
static void ario_status_icon_set_visibility (ArioStatusIcon *status, int state);
static void ario_status_icon_sync_tooltip (ArioStatusIcon *icon);
static void ario_status_icon_sync_popup (ArioStatusIcon *icon);
static void ario_status_icon_activate_cb (GtkStatusIcon *status_icon,
                                          ArioStatusIcon *icon);
static void ario_status_icon_popup_cb (GtkStatusIcon *status_icon,
                                       guint button,
                                       guint activate_time,
                                       ArioStatusIcon *icon);
static void ario_status_icon_song_changed_cb (ArioMpd *mpd,
                                            ArioStatusIcon *icon);
static void ario_status_icon_state_changed_cb (ArioMpd *mpd,
                                             ArioStatusIcon *icon);
static void ario_status_icon_cmd_play (GtkAction *action,
                                     ArioStatusIcon *icon);
static void ario_status_icon_cmd_pause (GtkAction *action,
                                      ArioStatusIcon *icon);
static void ario_status_icon_cmd_stop (GtkAction *action,
                                     ArioStatusIcon *icon);
static void ario_status_icon_cmd_next (GtkAction *action,
                                     ArioStatusIcon *icon);
static void ario_status_icon_cmd_previous (GtkAction *action,
                                         ArioStatusIcon *icon);

struct ArioStatusIconPrivate
{
        gboolean tooltips_pointer_above;

        GtkWindow *main_window;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        gboolean maximized;
        int window_x;
        int window_y;
        int window_w;
        int window_h;
        gboolean visible;

        ArioMpd *mpd;
};

static GtkActionEntry ario_status_icon_actions [] =
{
        { "ControlPlay", GTK_STOCK_MEDIA_PLAY, N_("_Play"), "<control>space",
                NULL,
                G_CALLBACK (ario_status_icon_cmd_play) },
        { "ControlPause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "<control>space",
                NULL,
                G_CALLBACK (ario_status_icon_cmd_pause) },
        { "ControlStop", GTK_STOCK_MEDIA_STOP, N_("_Stop"), NULL,
                NULL,
                G_CALLBACK (ario_status_icon_cmd_stop) },
        { "ControlNext", GTK_STOCK_MEDIA_NEXT, N_("_Next"), "<control>Right",
                NULL,
                G_CALLBACK (ario_status_icon_cmd_next) },
        { "ControlPrevious", GTK_STOCK_MEDIA_PREVIOUS, N_("P_revious"), "<control>Left",
                NULL,
                G_CALLBACK (ario_status_icon_cmd_previous) },
};

static guint ario_status_icon_n_actions = G_N_ELEMENTS (ario_status_icon_actions);

enum
{
        PROP_0,
        PROP_UI_MANAGER,
        PROP_WINDOW,
        PROP_ACTION_GROUP,
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
ario_status_icon_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_status_icon_type = 0;

        if (ario_status_icon_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioStatusIconClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_status_icon_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioStatusIcon),
                        0,
                        (GInstanceInitFunc) ario_status_icon_init
                };

                ario_status_icon_type = g_type_register_static (GTK_TYPE_STATUS_ICON,
                                                              "ArioStatusIcon",
                                                              &our_info, 0);
        }

        return ario_status_icon_type;
}

static void
ario_status_icon_class_init (ArioStatusIconClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_status_icon_finalize;
        object_class->constructor = ario_status_icon_constructor;

        object_class->set_property = ario_status_icon_set_property;
        object_class->get_property = ario_status_icon_get_property;

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
        g_object_class_install_property (object_class,
                                         PROP_ACTION_GROUP,
                                         g_param_spec_object ("action-group",
                                                              "GtkActionGroup",
                                                              "GtkActionGroup object",
                                                              GTK_TYPE_ACTION_GROUP,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ario_status_icon_init (ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START

        icon->priv = g_new0 (ArioStatusIconPrivate, 1);

        gtk_status_icon_set_tooltip (GTK_STATUS_ICON (icon), STATUS_ICON_DEFAULT_TOOLTIP);

        gtk_status_icon_set_from_stock (GTK_STATUS_ICON (icon), "ario");

        g_signal_connect_object (G_OBJECT (icon),
                                 "activate",
                                 G_CALLBACK (ario_status_icon_activate_cb),
                                 icon, 0);
        g_signal_connect_object (G_OBJECT (icon),
                                 "popup-menu",
                                 G_CALLBACK (ario_status_icon_popup_cb),
                                 icon, 0);

        icon->priv->window_x = -1;
        icon->priv->window_y = -1;
        icon->priv->window_w = -1;
        icon->priv->window_h = -1;
        icon->priv->visible = TRUE;
}

static GObject *
ario_status_icon_constructor (GType type, guint n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusIcon *status;
        ArioStatusIconClass *klass;
        GObjectClass *parent_class;  

        klass = ARIO_STATUS_ICON_CLASS (g_type_class_peek (TYPE_ARIO_STATUS_ICON));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        status = ARIO_STATUS_ICON (parent_class->constructor (type, n_construct_properties,
                                                          construct_properties));

        ario_status_icon_set_visibility (status, VISIBILITY_VISIBLE);

        return G_OBJECT (status);
}

static void
ario_status_icon_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusIcon *status;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_STATUS_ICON (object));

        status = ARIO_STATUS_ICON (object);

        g_return_if_fail (status->priv != NULL);

        /* TODO : Not very efficient */
        ario_mpd_use_count_dec (status->priv->mpd);

        g_free (status->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_status_icon_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusIcon *status = ARIO_STATUS_ICON (object);

        switch (prop_id)
        {
        case PROP_WINDOW:
                status->priv->main_window = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                status->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_MPD:
                status->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (status->priv->mpd),
                                         "song_changed", G_CALLBACK (ario_status_icon_song_changed_cb),
                                         status, 0);
                g_signal_connect_object (G_OBJECT (status->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_status_icon_state_changed_cb),
                                         status, 0);
                g_signal_connect_object (G_OBJECT (status->priv->mpd),
                                         "playlist_changed", G_CALLBACK (ario_status_icon_state_changed_cb),
                                         status, 0);
                break;
        case PROP_ACTION_GROUP:
                status->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (status->priv->actiongroup,
                                              ario_status_icon_actions,
                                              ario_status_icon_n_actions, status);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_status_icon_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusIcon *status = ARIO_STATUS_ICON (object);

        switch (prop_id)
        {
        case PROP_WINDOW:
                g_value_set_object (value, status->priv->main_window);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, status->priv->ui_manager);
                break;
        case PROP_MPD:
                g_value_set_object (value, status->priv->mpd);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, status->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

ArioStatusIcon *
ario_status_icon_new (GtkActionGroup *group,
                      GtkUIManager *mgr,
                      GtkWindow *window,
                      ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        ArioStatusIcon *icon;
        icon = g_object_new (TYPE_ARIO_STATUS_ICON,
                             "action-group", group,
                             "ui-manager", mgr,
                             "window", window,
                             "mpd", mpd,
                             NULL);

        /* TODO : Not very efficient */
        ario_mpd_use_count_inc (icon->priv->mpd);

        return icon;
}

static void
ario_status_icon_activate_cb (GtkStatusIcon *status_icon,
                              ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START

        ario_status_icon_set_visibility (icon, VISIBILITY_TOGGLE);
}

static void
ario_status_icon_popup_cb (GtkStatusIcon *status_icon,
                           guint button,
                           guint activate_time,
                           ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *popup;

        popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (icon->priv->ui_manager),
                                           "/TrayPopup");
        gtk_menu_set_screen (GTK_MENU (popup), gtk_status_icon_get_screen (status_icon));
        gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                        gtk_status_icon_position_menu, status_icon, 2,
                        gtk_get_current_event_time ());
}

static void
ario_status_icon_sync_tooltip (ArioStatusIcon *icon)
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
                tooltip = g_strdup (STATUS_ICON_DEFAULT_TOOLTIP);
                break;
        }

        gtk_status_icon_set_tooltip (GTK_STATUS_ICON (icon), tooltip);

        g_free (tooltip);
}

static void
ario_status_icon_sync_popup (ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        int state = ario_mpd_get_current_state (icon->priv->mpd);

        gtk_action_set_visible (gtk_action_group_get_action (icon->priv->actiongroup, "ControlPlay"),
                                state != MPD_STATUS_STATE_PLAY);
        gtk_action_set_visible (gtk_action_group_get_action (icon->priv->actiongroup, "ControlPause"),
                                state == MPD_STATUS_STATE_PLAY);
}

static void
ario_status_icon_song_changed_cb (ArioMpd *mpd,
                                ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_status_icon_sync_tooltip (icon);
}

static void
ario_status_icon_state_changed_cb (ArioMpd *mpd,
                                 ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_status_icon_sync_tooltip (icon);
        ario_status_icon_sync_popup (icon);
}

static void
ario_status_icon_restore_main_window (ArioStatusIcon *icon)
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
ario_status_icon_set_visibility (ArioStatusIcon *icon,
                               int state)
{
        ARIO_LOG_FUNCTION_START
        switch (state)
        {
        case VISIBILITY_HIDDEN:
        case VISIBILITY_VISIBLE:
                if (icon->priv->visible != state)
                        ario_status_icon_set_visibility (icon, VISIBILITY_TOGGLE);
                break;
        case VISIBILITY_TOGGLE:
                icon->priv->visible = !icon->priv->visible;

                if (icon->priv->visible == TRUE) {
                        ario_status_icon_restore_main_window (icon);
                        gtk_widget_show (GTK_WIDGET (icon->priv->main_window));
                } else {
                        icon->priv->maximized = ario_conf_get_boolean (CONF_WINDOW_MAXIMIZED, TRUE);
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

static void
ario_status_icon_cmd_play (GtkAction *action,
                         ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_play (icon->priv->mpd);
}

static void
ario_status_icon_cmd_pause (GtkAction *action,
                          ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_pause (icon->priv->mpd);
}

static void
ario_status_icon_cmd_stop (GtkAction *action,
                        ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_stop (icon->priv->mpd);
}

static void
ario_status_icon_cmd_next (GtkAction *action,
                         ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_next (icon->priv->mpd);
}

static void
ario_status_icon_cmd_previous (GtkAction *action,
                             ArioStatusIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_prev (icon->priv->mpd);
}
