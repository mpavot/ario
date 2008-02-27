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
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define TRAY_ICON_DEFAULT_TOOLTIP _("Not playing")
#define FROM_MARKUP(xALBUM, xARTIST) g_markup_printf_escaped (_("<i>from</i> %s <i>by</i> %s"), xALBUM, xARTIST);

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
static void ario_tray_icon_update_tooltip_visibility (ArioTrayIcon *icon);
static void ario_tray_icon_enter_notify_event_cb (ArioTrayIcon *icon,
                                                  GdkEvent *event,
                                                  GtkWidget *widget);
static void ario_tray_icon_leave_notify_event_cb (ArioTrayIcon *icon,
                                                  GdkEvent *event,
                                                  GtkWidget *widget);
static void ario_tray_icon_tooltip_size_allocate_cb (ArioTrayIcon *icon,
                                                     GtkAllocation *allocation,
                                                     GtkWidget *tooltip);
static void ario_tray_icon_construct_tooltip (ArioTrayIcon *icon);
static void ario_tray_icon_set_visibility (ArioTrayIcon *tray, int state);
static void ario_tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                                  ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_song (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_album (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_cover (ArioTrayIcon *icon);
static void ario_tray_icon_sync_tooltip_time (ArioTrayIcon *icon);
static void ario_tray_icon_sync_popup (ArioTrayIcon *icon);
static gboolean ario_tray_icon_scroll_cb (GtkWidget *widget, GdkEvent *event,
                                          ArioTrayIcon *icon);
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
        GtkWidget *image;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        GtkWindow *main_window;
        GtkWidget *ebox;

        gboolean maximized;
        int window_x;
        int window_y;
        int window_w;
        int window_h;
        gboolean visible;

        ArioMpd *mpd;
        ArioCoverHandler *cover_handler;
};

static GtkActionEntry ario_tray_icon_actions [] =
{
        { "ControlPlay", GTK_STOCK_MEDIA_PLAY, N_("_Play"), "<control>space",
                NULL,
                G_CALLBACK (ario_tray_icon_cmd_play) },
        { "ControlPause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "<control>space",
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
        PROP_WINDOW,
        PROP_MPD,
        PROP_COVERHANDLER
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
        g_object_class_install_property (object_class,
                                         PROP_COVERHANDLER,
                                         g_param_spec_object ("cover_handler",
                                                              "ArioCoverHandler",
                                                              "ArioCoverHandler object",
                                                              TYPE_ARIO_COVER_HANDLER,
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
ario_tray_icon_init (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *image;

        icon->priv = g_new0 (ArioTrayIconPrivate, 1);

        ario_tray_icon_construct_tooltip (icon);

        icon->priv->ebox = gtk_event_box_new ();
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "button_press_event",
                                 G_CALLBACK (ario_tray_icon_button_press_event_cb),
                                 icon, 0);
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "scroll_event",
                                 G_CALLBACK (ario_tray_icon_scroll_cb),
                                 icon, 0);
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "enter-notify-event",
                                 G_CALLBACK (ario_tray_icon_enter_notify_event_cb),
                                 icon, G_CONNECT_SWAPPED);
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "leave-notify-event",
                                 G_CALLBACK (ario_tray_icon_leave_notify_event_cb),
                                 icon, G_CONNECT_SWAPPED);
        image = gtk_image_new_from_stock ("ario",
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_container_add (GTK_CONTAINER (icon->priv->ebox), image);

        icon->priv->window_x = -1;
        icon->priv->window_y = -1;
        icon->priv->window_w = -1;
        icon->priv->window_h = -1;
        icon->priv->visible = TRUE;

        gtk_container_add (GTK_CONTAINER (icon), icon->priv->ebox);
        gtk_widget_show_all (GTK_WIDGET (icon->priv->ebox));
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

        gtk_object_destroy (GTK_OBJECT (tray->priv->tooltip));

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
                                         "album_changed", G_CALLBACK (ario_tray_icon_album_changed_cb),
                                         tray, 0);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_tray_icon_state_changed_cb),
                                         tray, 0);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "playlist_changed", G_CALLBACK (ario_tray_icon_state_changed_cb),
                                         tray, 0);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "elapsed_changed", G_CALLBACK (ario_tray_icon_time_changed_cb),
                                         tray, 0);
                break;
        case PROP_COVERHANDLER:
                tray->priv->cover_handler = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (tray->priv->cover_handler),
                                         "cover_changed", G_CALLBACK (ario_tray_icon_cover_changed_cb),
                                         tray, 0);
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
        case PROP_WINDOW:
                g_value_set_object (value, tray->priv->main_window);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, tray->priv->ui_manager);
                break;
        case PROP_MPD:
                g_value_set_object (value, tray->priv->mpd);
                break;
        case PROP_COVERHANDLER:
                g_value_set_object (value, tray->priv->cover_handler);
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
                    GtkWindow *window,
                    ArioMpd *mpd,
                    ArioCoverHandler *cover_handler)
{
        ARIO_LOG_FUNCTION_START
        return g_object_new (TYPE_ARIO_TRAY_ICON,
                             "action-group", group,
                             "title", "Ario",
                             "ui-manager", mgr,
                             "window", window,
                             "mpd", mpd,
                             "cover_handler", cover_handler,
                             NULL);
}

static void
ario_tray_icon_update_tooltip_visibility (ArioTrayIcon *icon)
{
        if (icon->priv->tooltips_pointer_above) {
                sexy_tooltip_position_to_widget (SEXY_TOOLTIP (icon->priv->tooltip),
                                                 icon->priv->ebox);
                gtk_widget_show (icon->priv->tooltip);
                ario_tray_icon_sync_tooltip_time (icon);
        } else {
                gtk_widget_hide (icon->priv->tooltip);
        }
}

static void
ario_tray_icon_enter_notify_event_cb (ArioTrayIcon *icon,
                                      GdkEvent *event,
                                      GtkWidget *widget)
{
        icon->priv->tooltips_pointer_above = TRUE;
        ario_tray_icon_update_tooltip_visibility (icon);
        ario_mpd_use_count_inc (icon->priv->mpd);
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
        ario_tray_icon_update_tooltip_visibility (icon);
}

static void
ario_tray_icon_tooltip_size_allocate_cb (ArioTrayIcon *icon,
                                         GtkAllocation *allocation,
                                         GtkWidget *tooltip)
{
        sexy_tooltip_position_to_widget (SEXY_TOOLTIP (icon->priv->tooltip),
                                         icon->priv->ebox);
}

static void
ario_tray_icon_construct_tooltip (ArioTrayIcon *icon)
{
        GtkWidget *hbox, *vbox;
        gint size;
        PangoFontDescription *font_desc;

        icon->priv->tooltips_pointer_above = FALSE;
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
        icon->priv->image = gtk_image_new ();
        icon->priv->tooltip_image_box = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (icon->priv->tooltip_image_box), icon->priv->image,
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

static void
ario_tray_icon_double_click (ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        int trayicon_behavior;

        trayicon_behavior = ario_conf_get_integer (CONF_TRAYICON_BEHAVIOR, 0);

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
                cover = ario_cover_handler_get_cover(icon->priv->cover_handler);
                if (cover) {
                        gtk_image_set_from_pixbuf (GTK_IMAGE (icon->priv->image), cover);
                        gtk_widget_show (icon->priv->image);
                } else {
                        gtk_widget_hide (icon->priv->image);
                }
                break;
        default:
                gtk_widget_hide (icon->priv->image);
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

        if (!icon->priv->tooltips_pointer_above)
                return;

        switch (ario_mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                elapsed = ario_mpd_get_current_elapsed (icon->priv->mpd);
                elapsed_char = ario_util_format_time (elapsed);
                total = ario_mpd_get_current_total_time (icon->priv->mpd);
                total_char = ario_util_format_time (total);
                time = g_strdup_printf ("%s%s%s", elapsed_char, _(" of "), total_char);
                g_free (elapsed_char);
                g_free (total_char);
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
        ario_tray_icon_sync_popup (icon);
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
ario_tray_icon_cmd_play (GtkAction *action,
                         ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_play (icon->priv->mpd);
}

static void
ario_tray_icon_cmd_pause (GtkAction *action,
                          ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_pause (icon->priv->mpd);
}

static void
ario_tray_icon_cmd_stop (GtkAction *action,
                        ArioTrayIcon *icon)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_do_stop (icon->priv->mpd);
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
