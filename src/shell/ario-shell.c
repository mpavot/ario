/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
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

#include "shell/ario-shell.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <config.h>
#include <string.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "covers/ario-cover-handler.h"
#include "covers/ario-cover-manager.h"
#include "lib/ario-conf.h"
#include "lyrics/ario-lyrics-manager.h"
#include "notification/ario-notification-manager.h"
#include "playlist/ario-playlist-manager.h"
#include "plugins/ario-plugin-manager.h"
#include "preferences/ario-preferences.h"
#include "servers/ario-server.h"
#include "shell/ario-shell-coverdownloader.h"
#include "shell/ario-shell-coverselect.h"
#include "shell/ario-shell-lyrics.h"
#include "shell/ario-shell-preferences.h"
#include "shell/ario-shell-similarartists.h"
#include "sources/ario-source-manager.h"
#include "widgets/ario-firstlaunch.h"
#include "widgets/ario-header.h"
#include "widgets/ario-playlist.h"
#include "widgets/ario-status-bar.h"
#include "widgets/ario-tray-icon.h"

static void ario_shell_finalize (GObject *object);
static void ario_shell_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void ario_shell_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);
static void ario_shell_show (ArioShell *shell,
                             gboolean minimized);
static void ario_shell_cmd_quit (GtkAction *action,
                                 ArioShell *shell);
static void ario_shell_cmd_connect (GtkAction *action,
                                    ArioShell *shell);
static void ario_shell_cmd_disconnect (GtkAction *action,
                                       ArioShell *shell);
static void ario_shell_cmd_plugins (GtkAction *action,
                                    ArioShell *shell);
static void ario_shell_cmd_preferences (GtkAction *action,
                                        ArioShell *shell);
static void ario_shell_cmd_lyrics (GtkAction *action,
                                   ArioShell *shell);
static void ario_shell_cmd_cover_select (GtkAction *action,
                                         ArioShell *shell);
static void ario_shell_cmd_covers (GtkAction *action,
                                   ArioShell *shell);
static void ario_shell_cmd_similar_artists (GtkAction *action,
                                            ArioShell *shell);
static void ario_shell_cmd_add_similar (GtkAction *action,
                                        ArioShell *shell);
static void ario_shell_cmd_about (GtkAction *action,
                                  ArioShell *shell);
static void ario_shell_cmd_translate (GtkAction *action,
                                      ArioShell *shell);
static void ario_shell_server_state_changed_cb (ArioServer *server,
                                                ArioShell *shell);
static void ario_shell_server_song_changed_cb (ArioServer *server,
                                               ArioShell *shell);
static gboolean ario_shell_window_state_cb (GtkWidget *widget,
                                            GdkEvent *event,
                                            ArioShell *shell);
static void ario_shell_sync_window_state (ArioShell *shell);
static void ario_shell_sync_paned (ArioShell *shell);
static void ario_shell_sync_server (ArioShell *shell);
static void ario_shell_firstlaunch_delete_cb (GtkObject *firstlaunch,
                                              ArioShell *shell);
static void ario_shell_view_statusbar_changed_cb (GtkAction *action,
                                                  ArioShell *shell);
static void ario_shell_view_upperpart_changed_cb (GtkAction *action,
                                                  ArioShell *shell);
static void ario_shell_view_playlist_changed_cb (GtkAction *action,
                                                 ArioShell *shell);
static void ario_shell_sync_statusbar_visibility (ArioShell *shell);
static void ario_shell_sync_upperpart_visibility (ArioShell *shell);
static void ario_shell_sync_playlist_visibility (ArioShell *shell);

struct ArioShellPrivate
{
        ArioCoverHandler *cover_handler;
        ArioPlaylistManager *playlist_manager;
        ArioNotificationManager *notification_manager;
        GtkWidget *header;
        GtkWidget *vpaned;
        GtkWidget *sourcemanager;
        GtkWidget *playlist;
        GtkWidget *status_bar;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        ArioTrayIcon *tray_icon;

        gboolean statusbar_hidden;
        gboolean upperpart_hidden;
        gboolean playlist_hidden;
        gboolean connected;
        gboolean shown;

        gboolean maximized;
        gboolean visible;

        int window_x;
        int window_y;
        int window_w;
        int window_h;
};

enum
{
        PROP_0,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
};

static GtkActionEntry shell_actions [] =
{
        { "File", NULL, N_("_File"), NULL, NULL, NULL },
        { "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
        { "View", NULL, N_("_View"), NULL, NULL, NULL },
        { "Tool", NULL, N_("_Tool"), NULL, NULL, NULL },
        { "Help", NULL, N_("_Help"), NULL, NULL, NULL },

        { "FileConnect", GTK_STOCK_CONNECT, N_("_Connect"), "<control>C",
                NULL,
                G_CALLBACK (ario_shell_cmd_connect) },
        { "FileDisconnect", GTK_STOCK_DISCONNECT, N_("_Disconnect"), "<control>D",
                NULL,
                G_CALLBACK (ario_shell_cmd_disconnect) },
        { "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
                NULL,
                G_CALLBACK (ario_shell_cmd_quit) },
        { "EditPlugins", GTK_STOCK_EXECUTE, N_("Plu_gins"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_plugins) },
        { "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_preferences) },
        { "ToolCoverSelect", GTK_STOCK_CDROM, N_("_Change current album cover"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_cover_select) },
        { "ToolCover", GTK_STOCK_EXECUTE, N_("Download album _covers"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_covers) },
        { "ToolSimilarArtist", GTK_STOCK_INDEX, N_("Find similar artists"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_similar_artists) },
        { "ToolAddSimilar", GTK_STOCK_ADD, N_("Add similar songs to playlist"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_add_similar) },
        { "ViewLyrics", GTK_STOCK_EDIT, N_("Show _lyrics"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_lyrics) },
        { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_about) },
        { "HelpTranslate", GTK_STOCK_EDIT, N_("_Translate this application..."), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_translate) },
};

static GtkToggleActionEntry shell_toggle [] =
{
        { "ViewStatusbar", NULL, N_("S_tatusbar"), NULL,
                NULL,
                G_CALLBACK (ario_shell_view_statusbar_changed_cb), TRUE },
        { "ViewUpperPart", NULL, N_("Upper part"), NULL,
                NULL,
                G_CALLBACK (ario_shell_view_upperpart_changed_cb), TRUE },
        { "ViewPlaylist", NULL, N_("Playlist"), NULL,
                NULL,
                G_CALLBACK (ario_shell_view_playlist_changed_cb), TRUE }
};

#define ARIO_SHELL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ARIO_TYPE_SHELL, ArioShellPrivate))
G_DEFINE_TYPE (ArioShell, ario_shell, GTK_TYPE_WINDOW)

static void
ario_shell_class_init (ArioShellClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = (GObjectClass *) klass;

        /* Virtual methods */
        object_class->set_property = ario_shell_set_property;
        object_class->get_property = ario_shell_get_property;
        object_class->finalize = ario_shell_finalize;

        /* Properties */
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
        g_type_class_add_private (klass, sizeof (ArioShellPrivate));
}

static void
ario_shell_init (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        shell->priv = ARIO_SHELL_GET_PRIVATE (shell);
        shell->priv->connected = FALSE;
        shell->priv->shown = FALSE;

        shell->priv->visible = TRUE;

        shell->priv->window_x = -1;
        shell->priv->window_y = -1;
        shell->priv->window_w = -1;
        shell->priv->window_h = -1;
}

static void
ario_shell_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell = ARIO_SHELL (object);

        gtk_widget_hide (GTK_WIDGET(shell));

        g_object_unref (shell->priv->cover_handler);
        g_object_unref (shell->priv->playlist_manager);
        g_object_unref (shell->priv->notification_manager);
        g_object_unref (G_OBJECT (shell->priv->tray_icon));

        g_object_unref (shell->priv->ui_manager);
        g_object_unref (shell->priv->actiongroup);
        g_object_unref (ario_server_get_instance ());

        G_OBJECT_CLASS (ario_shell_parent_class)->finalize (object);
}

static void
ario_shell_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell = ARIO_SHELL (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                shell->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                shell->priv->actiongroup = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_shell_get_property (GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell = ARIO_SHELL (object);

        switch (prop_id) {
        case PROP_UI_MANAGER:
                g_value_set_object (value, shell->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, shell->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

ArioShell *
ario_shell_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell;

        shell = g_object_new (ARIO_TYPE_SHELL, NULL);

        return shell;
}

static void
ario_shell_quit (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;

        /* Stop music on exit if needed */
        if (ario_conf_get_boolean (PREF_STOP_EXIT, PREF_STOP_EXIT_DEFAULT))
                ario_server_do_stop ();

        /* Stop main loop */
        gtk_main_quit ();
}

static gboolean
ario_shell_window_delete_cb (GtkWidget *win,
                             GdkEventAny *event,
                             ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_conf_get_boolean (PREF_HIDE_ON_CLOSE, PREF_HIDE_ON_CLOSE_DEFAULT)) {
                ario_shell_set_visibility (shell, VISIBILITY_TOGGLE);
        } else {
                ario_shell_quit (shell);
        }
        return TRUE;
};

void
ario_shell_construct (ArioShell *shell,
                      gboolean minimized)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menubar;
        GtkWidget *separator;
        GtkWidget *vbox;
        GtkAction *action;
        ArioFirstlaunch *firstlaunch;

        g_return_if_fail (IS_ARIO_SHELL (shell));

        /* Set main window properties */
        gtk_window_set_title (GTK_WINDOW (shell), "Ario");
        gtk_window_set_position (GTK_WINDOW (shell), GTK_WIN_POS_CENTER);

        /* Create program icon */
        gtk_window_set_default_icon_name ("ario");

        /* Connect window destruction signal to exit program */
        g_signal_connect (shell,
                          "delete_event",
                          G_CALLBACK (ario_shell_window_delete_cb),
                          shell);

        /* Initialize UI */
        shell->priv->ui_manager = gtk_ui_manager_new ();
        gtk_ui_manager_add_ui_from_file (shell->priv->ui_manager,
                                         UI_PATH "ario-ui.xml", NULL);

        /* Create Action Group */
        shell->priv->actiongroup = gtk_action_group_new ("MainActions");
        gtk_window_add_accel_group (GTK_WINDOW (shell),
                                    gtk_ui_manager_get_accel_group (shell->priv->ui_manager));
        gtk_action_group_set_translation_domain (shell->priv->actiongroup,
                                                 GETTEXT_PACKAGE);
        gtk_ui_manager_insert_action_group (shell->priv->ui_manager,
                                            shell->priv->actiongroup, 0);
#ifdef WIN32
        /* This seems to be needed on win32 */
        gtk_action_group_set_translate_func (shell->priv->actiongroup,
                                             (GtkTranslateFunc) gettext,
                                             NULL, NULL);
#endif
        /* Main window actions */
        gtk_action_group_add_actions (shell->priv->actiongroup,
                                      shell_actions, G_N_ELEMENTS (shell_actions),
                                      shell);
        gtk_action_group_add_toggle_actions (shell->priv->actiongroup,
                                             shell_toggle, G_N_ELEMENTS (shell_toggle),
                                             shell);

        /* Initialize server object (MPD, XMMS, ....) */
        ario_server_get_instance ();

        /* Initialize cover art handler */
        shell->priv->cover_handler = ario_cover_handler_new ();

        /* Initialize tray icon */
        shell->priv->tray_icon = ario_tray_icon_new (shell->priv->actiongroup,
                                                     shell->priv->ui_manager,
                                                     shell);

        /* Initialize playlist manager */
        shell->priv->playlist_manager = ario_playlist_manager_get_instance ();

        /* Initialize notification manager */
        shell->priv->notification_manager = ario_notification_manager_get_instance ();

        /* Add widgets to main window.
         * Structure is:
         * vbox
         * ---menubar
         * ---header
         * ---separator
         * ---vpaned
         * ------sourcemanager
         * ------playlist
         * ---status_bar
         */

        /* Create main vbox */
        vbox = gtk_vbox_new (FALSE, 0);

        /* Create header */
        shell->priv->header = ario_header_new ();

        /* Create separator */
        separator = gtk_hseparator_new ();

        /* Create playlist */
        shell->priv->playlist = ario_playlist_new (shell->priv->ui_manager, shell->priv->actiongroup);

        /* Create source manager */
        shell->priv->sourcemanager = ario_source_manager_get_instance (shell->priv->ui_manager, shell->priv->actiongroup);

        /* Create vpaned (separation between upper part and plyalist) */
        shell->priv->vpaned = gtk_vpaned_new ();

        /* Create status bar */
        shell->priv->status_bar = ario_status_bar_new ();

        /* Synchronize status bar checkbox in menu with preferences */
        shell->priv->statusbar_hidden = ario_conf_get_boolean (PREF_STATUSBAR_HIDDEN, PREF_STATUSBAR_HIDDEN_DEFAULT);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewStatusbar");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->statusbar_hidden);

        /* Synchronize upper part checkbox in menu with preferences */
        shell->priv->upperpart_hidden = ario_conf_get_boolean (PREF_UPPERPART_HIDDEN, PREF_UPPERPART_HIDDEN_DEFAULT);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewUpperPart");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->upperpart_hidden);

        /* Synchronize playlist checkbox in menu with preferences */
        shell->priv->playlist_hidden = ario_conf_get_boolean (PREF_PLAYLIST_HIDDEN, PREF_PLAYLIST_HIDDEN_DEFAULT);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewPlaylist");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->playlist_hidden);

        /* Create main window menu */
        menubar = gtk_ui_manager_get_widget (shell->priv->ui_manager, "/MenuBar");

        /* Add widgets to vpaned */
        gtk_paned_pack1 (GTK_PANED (shell->priv->vpaned),
                         shell->priv->sourcemanager,
                         FALSE, FALSE);

        gtk_paned_pack2 (GTK_PANED (shell->priv->vpaned),
                         shell->priv->playlist,
                         TRUE, FALSE);

        /* Add widgets to vbox */
        gtk_box_pack_start (GTK_BOX (vbox),
                            menubar,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            shell->priv->header,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            separator,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            shell->priv->vpaned,
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            shell->priv->status_bar,
                            FALSE, FALSE, 0);

        /* Add vbox to main window */
        gtk_container_add (GTK_CONTAINER (shell), vbox);

        /* First launch assistant */
        if (!ario_conf_get_boolean (PREF_FIRST_TIME, PREF_FIRST_TIME_DEFAULT)) {
                firstlaunch = ario_firstlaunch_new ();
                g_signal_connect (firstlaunch,
                                  "destroy",
                                  G_CALLBACK (ario_shell_firstlaunch_delete_cb),
                                  shell);
                gtk_widget_show_all (GTK_WIDGET (firstlaunch));
        } else {
                ario_shell_show (shell, minimized);
        }

        /* Synchronize main window state with preferences */
        ario_shell_sync_window_state (shell);

        /* Synchronize status bar visibility with preferences */
        ario_shell_sync_statusbar_visibility (shell);

        /* Synchronize upper part visibility with preferences */
        ario_shell_sync_upperpart_visibility (shell);

        /* Synchronize playlist visibility with preferences */
        ario_shell_sync_playlist_visibility (shell);
}

void
ario_shell_shutdown (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        int width, height;

        /* If the main window is visible, we save a few preferences */
        if (shell->priv->shown) {
                /* Save vpaned position */
                ario_conf_set_integer (PREF_VPANED_POSITION,
                                       gtk_paned_get_position (GTK_PANED (shell->priv->vpaned)));

                /* Save window size */
                if (!ario_conf_get_boolean (PREF_WINDOW_MAXIMIZED, PREF_WINDOW_MAXIMIZED_DEFAULT)) {
                        gtk_window_get_size (GTK_WINDOW (shell),
                                             &width,
                                             &height);

                        ario_conf_set_integer (PREF_WINDOW_WIDTH, width);
                        ario_conf_set_integer (PREF_WINDOW_HEIGHT, height);
                }
        }

        /* Shutdown the playlist */
        ario_playlist_shutdown ();

        /* Shutdown the source manager */
        ario_source_manager_shutdown ();

        /* Shutdown the cover_manager */
        ario_cover_manager_shutdown (ario_cover_manager_get_instance ());

        /* Shutdown the lyrics_manager */
        ario_lyrics_manager_shutdown (ario_lyrics_manager_get_instance ());

        /* Shutdown the server object */
        ario_server_shutdown ();
}

static void
ario_shell_show (ArioShell *shell,
                 gboolean minimized)
{
        ARIO_LOG_FUNCTION_START;
        ArioServer *server = ario_server_get_instance ();

        /* Connect signals for server state changes */
        g_signal_connect (server,
                          "state_changed",
                          G_CALLBACK (ario_shell_server_state_changed_cb),
                          shell);

        g_signal_connect (server,
                          "song_changed",
                          G_CALLBACK (ario_shell_server_song_changed_cb),
                          shell);

        /* Autoconnect on startup if needed */
        if (ario_conf_get_boolean (PREF_AUTOCONNECT, PREF_AUTOCONNECT_DEFAULT))
                ario_server_connect ();

        /* Synchonize the main window with server state */
        ario_shell_sync_server (shell);

        /* Minimize window if needed */
        if (minimized) {
                ario_shell_set_visibility (shell, VISIBILITY_HIDDEN);
        } else {
                gtk_widget_show_all (GTK_WIDGET(shell));
                shell->priv->shown = TRUE;
        }

        /* Connect signal for window state changes */
        g_signal_connect_object (shell,
                                 "window-state-event",
                                 G_CALLBACK (ario_shell_window_state_cb),
                                 shell,
                                 G_CONNECT_AFTER);

        /* Synchronize vpaned with preferences */
        ario_shell_sync_paned (shell);

        /* Update server db on startup if needed */
        if (ario_conf_get_boolean (PREF_UPDATE_STARTUP, PREF_UPDATE_STARTUP_DEFAULT))
                ario_server_update_db (NULL);
}

void
ario_shell_present (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        gtk_window_present (GTK_WINDOW (shell));
}

void
ario_shell_set_visibility (ArioShell *shell,
                           ArioVisibility state)
{
        ARIO_LOG_FUNCTION_START;
        switch (state)
        {
        case VISIBILITY_HIDDEN:
                if (shell->priv->visible)
                        ario_shell_set_visibility (shell, VISIBILITY_TOGGLE);
                break;
        case VISIBILITY_VISIBLE:
                if (!shell->priv->visible)
                        ario_shell_set_visibility (shell, VISIBILITY_TOGGLE);
                break;
        case VISIBILITY_TOGGLE:
                shell->priv->visible = !shell->priv->visible;

                if (shell->priv->visible == TRUE) {
                        /* Restore window state, size and position */
                        if (shell->priv->window_x >= 0 && shell->priv->window_y >= 0) {
                                gtk_window_move (GTK_WINDOW (shell),
                                                 shell->priv->window_x,
                                                 shell->priv->window_y);
                        }
                        if (!shell->priv->maximized
                            && (shell->priv->window_w >= 0 && shell->priv->window_y >=0)) {
                                gtk_window_resize (GTK_WINDOW (shell),
                                                   shell->priv->window_w,
                                                   shell->priv->window_h);
                        }

                        if (shell->priv->maximized)
                                gtk_window_maximize (GTK_WINDOW (shell));
                        gtk_widget_show_all (GTK_WIDGET(shell));

                        /* Restore vpaned position
                         * This is in a g_timeout_add because there seems to have a bug (#2798748) if I call 
                         * ario_shell_sync_paned directly. I really don't understand why but this seems to work...*/
                         g_timeout_add (100, (GSourceFunc) ario_shell_sync_paned, shell);
                } else {
                        /* Save window state, size and position */
                        shell->priv->maximized = ario_conf_get_boolean (PREF_WINDOW_MAXIMIZED, PREF_WINDOW_MAXIMIZED_DEFAULT);
                        gtk_window_get_position (GTK_WINDOW (shell),
                                                 &shell->priv->window_x,
                                                 &shell->priv->window_y);
                        gtk_window_get_size (GTK_WINDOW (shell),
                                             &shell->priv->window_w,
                                             &shell->priv->window_h);

                        /* Save vpaned position */
                        ario_conf_set_integer (PREF_VPANED_POSITION,
                                               gtk_paned_get_position (GTK_PANED (shell->priv->vpaned)));

                        gtk_widget_hide (GTK_WIDGET(shell));
                }
        }
}

static void
ario_shell_cmd_quit (GtkAction *action,
                     ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        ario_shell_quit (shell);
}

static void
ario_shell_cmd_connect (GtkAction *action,
                        ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_connect ();
}

static void
ario_shell_cmd_disconnect (GtkAction *action,
                           ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_disconnect ();
}

static void
ario_shell_cmd_preferences (GtkAction *action,
                            ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *prefs;

        /* Create preferences dialog window */
        prefs = ario_shell_preferences_new ();

        gtk_window_set_transient_for (GTK_WINDOW (prefs),
                                      GTK_WINDOW (shell));

        gtk_widget_show_all (prefs);
}

static void
ario_shell_cmd_lyrics (GtkAction *action,
                       ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *lyrics;

        /* Create lyrics dialog window */
        lyrics = ario_shell_lyrics_new ();
        if (lyrics)
                gtk_widget_show_all (lyrics);
}

static void
ario_shell_cmd_about (GtkAction *action,
                      ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;

        /* Create about dialog window */
        const char *authors[] = {
#include "AUTHORS.tab"
                "",
                NULL
        };
        const char *artists[] = {
                "Luc Pavot",
                NULL
        };
        GdkPixbuf *logo_pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "logo.png", NULL);
        gtk_show_about_dialog (GTK_WINDOW (shell),
                               "name", "Ario",
                               "program-name", "Ario",
                               "version", PACKAGE_VERSION,
                               "copyright", "Copyright \xc2\xa9 2005-2008 Marc Pavot",
                               "comments", _("GTK client for MPD"),
                               "translator-credits", _("translator-credits"),
                               "authors", (const char **) authors,
                               "artists", (const char **) artists,
                               "logo", logo_pixbuf,
                               NULL);
        if (logo_pixbuf)
                g_object_unref (logo_pixbuf);
}

static void
ario_shell_cmd_translate (GtkAction *action,
                          ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        /* Load launchpad translation page */
        const gchar *uri = "https://translations.launchpad.net/ario/trunk/";
        ario_util_load_uri (uri);
}

static void
ario_shell_server_song_set_title (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        gchar *window_title;
        gchar *tmp;

        switch (ario_server_get_current_state ()) {
        case ARIO_STATE_PLAY:
        case ARIO_STATE_PAUSE:
                /* Window title containing song name */
                tmp = ario_util_format_title (ario_server_get_current_song ());
                window_title = g_strdup_printf ("Ario - %s", tmp);
                gtk_window_set_title (GTK_WINDOW (shell), window_title);
                g_free (window_title);
                break;
        default:
                /* Default window title */
                gtk_window_set_title (GTK_WINDOW (shell), "Ario");
                break;
        }
}

static void
ario_shell_server_song_changed_cb (ArioServer *server,
                                   ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        /* Change window title on song change */
        ario_shell_server_song_set_title (shell);
}

static void
ario_shell_server_state_changed_cb (ArioServer *server,
                                    ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        shell->priv->connected = ario_server_is_connected ();

        /* Synchronize main window with server state */
        ario_shell_sync_server (shell);

        /* Change window title on song change */
        ario_shell_server_song_set_title (shell);
}

static void
ario_shell_cmd_cover_select (GtkAction *action,
                             ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *coverselect;
        ArioServerAlbum server_album;

        /* Launch cover selection dialog for current album */
        server_album.artist = ario_server_get_current_artist ();
        server_album.album = ario_server_get_current_album ();
        server_album.path = g_path_get_dirname ((ario_server_get_current_song ())->file);

        if (!server_album.album)
                server_album.album = ARIO_SERVER_UNKNOWN;

        if (!server_album.artist)
                server_album.artist = ARIO_SERVER_UNKNOWN;

        coverselect = ario_shell_coverselect_new (&server_album);
        gtk_dialog_run (GTK_DIALOG (coverselect));
        gtk_widget_destroy (coverselect);
        g_free (server_album.path);
}

static void
ario_shell_cmd_covers (GtkAction *action,
                       ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *coverdownloader;

        /* Launch cover art download dialog */
        coverdownloader = ario_shell_coverdownloader_new ();

        if (coverdownloader) {
                ario_shell_coverdownloader_get_covers (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                       GET_COVERS);
        }
}

static void
ario_shell_cmd_similar_artists (GtkAction *action,
                                ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *similarartists;

        /* Launch similar artist dialog */
        similarartists = ario_shell_similarartists_new ();
        if (similarartists)
                gtk_widget_show_all (similarartists);
}

static void
ario_shell_cmd_add_similar (GtkAction *action,
                            ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;

        /* Add similar artists (from last.fm) to current playlist */
        ario_shell_similarartists_add_similar_to_playlist (ario_server_get_current_artist (), -1);
}

static void
ario_shell_sync_paned (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        int pos;

        /* Set vpaned position */
        pos = ario_conf_get_integer (PREF_VPANED_POSITION, PREF_VPANED_POSITION_DEFAULT);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (shell->priv->vpaned),
                                        pos);
}

static gboolean
ario_shell_window_state_cb (GtkWidget *widget,
                            GdkEvent *event,
                            ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        int width, height;
        g_return_val_if_fail (widget != NULL, FALSE);

        if ((event->type == GDK_WINDOW_STATE)
            && !(event->window_state.new_window_state & GDK_WINDOW_STATE_WITHDRAWN)) {
                /* Save window maximization state */
                ario_conf_set_boolean (PREF_WINDOW_MAXIMIZED,
                                       event->window_state.new_window_state &
                                       GDK_WINDOW_STATE_MAXIMIZED);

                if (event->window_state.changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
                        /* Save previous window size on maximization */
                        gtk_window_get_size (GTK_WINDOW (shell),
                                             &width,
                                             &height);

                        ario_conf_set_integer (PREF_WINDOW_WIDTH, width);
                        ario_conf_set_integer (PREF_WINDOW_HEIGHT, height);
                }
        }

        return FALSE;
}

static void
ario_shell_sync_window_state (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        int width = ario_conf_get_integer (PREF_WINDOW_WIDTH, PREF_WINDOW_WIDTH_DEFAULT);
        int height = ario_conf_get_integer (PREF_WINDOW_HEIGHT, PREF_WINDOW_HEIGHT_DEFAULT);
        gboolean maximized = ario_conf_get_boolean (PREF_WINDOW_MAXIMIZED, PREF_WINDOW_MAXIMIZED_DEFAULT);

        /* Set main window size */
        gtk_window_set_default_size (GTK_WINDOW (shell),
                                     width, height);
        gtk_window_resize (GTK_WINDOW (shell),
                           width, height);

        /* Maximize main window if needed */
        if (maximized)
                gtk_window_maximize (GTK_WINDOW (shell));
        else
                gtk_window_unmaximize (GTK_WINDOW (shell));
}

static void
ario_shell_sync_server (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkAction *connect_action;
        GtkAction *disconnect_action;
        gboolean is_playing;
        GtkAction *action;

        /* Set connect entry visibility */
        connect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                      "FileConnect");
        gtk_action_set_visible (connect_action, !shell->priv->connected);

        /* Set disconnect entry visibility */
        disconnect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                         "FileDisconnect");
        gtk_action_set_visible (disconnect_action, shell->priv->connected);

        is_playing = ((shell->priv->connected)
                      && ((ario_server_get_current_state () == ARIO_STATE_PLAY)
                          || (ario_server_get_current_state () == ARIO_STATE_PAUSE)));

        /* Set lyrics entry sensitivty */
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewLyrics");
        gtk_action_set_sensitive (action, is_playing);

        /* Set cover selection entry sensitivty */
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ToolCoverSelect");
        gtk_action_set_sensitive (action, is_playing);

        /* Set similar artists entry sensitivty */
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ToolSimilarArtist");
        gtk_action_set_sensitive (action, is_playing);

        /* Set similar artists addition entry sensitivty */
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ToolAddSimilar");
        gtk_action_set_sensitive (action, is_playing);
}

static void
ario_shell_firstlaunch_delete_cb (GtkObject *firstlaunch,
                                  ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        /* Show main window when first launch assistant is deleted */
        ario_shell_show (shell, FALSE);
}

static void
ario_shell_view_statusbar_changed_cb (GtkAction *action,
                                      ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        shell->priv->statusbar_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
        ario_conf_set_boolean (PREF_STATUSBAR_HIDDEN, shell->priv->statusbar_hidden);

        /* Synchronize status bar visibility */
        ario_shell_sync_statusbar_visibility (shell);
}

static void
ario_shell_view_upperpart_changed_cb (GtkAction *action,
                                      ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        shell->priv->upperpart_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
        ario_conf_set_boolean (PREF_UPPERPART_HIDDEN, shell->priv->upperpart_hidden);

        /* Synchronize upper part visibility */
        ario_shell_sync_upperpart_visibility (shell);
}

static void
ario_shell_view_playlist_changed_cb (GtkAction *action,
                                     ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        shell->priv->playlist_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
        ario_conf_set_boolean (PREF_PLAYLIST_HIDDEN, shell->priv->playlist_hidden);

        /* Synchronize playlist visibility */
        ario_shell_sync_playlist_visibility (shell);
}

static void
ario_shell_sync_statusbar_visibility (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        if (shell->priv->statusbar_hidden)
                gtk_widget_hide (GTK_WIDGET (shell->priv->status_bar));
        else
                gtk_widget_show (GTK_WIDGET (shell->priv->status_bar));
}

static void
ario_shell_sync_upperpart_visibility (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        if (shell->priv->upperpart_hidden)
                gtk_widget_hide (GTK_WIDGET (shell->priv->sourcemanager));
        else
                gtk_widget_show (GTK_WIDGET (shell->priv->sourcemanager));
}

static void
ario_shell_sync_playlist_visibility (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        if (shell->priv->playlist_hidden)
                gtk_widget_hide (GTK_WIDGET (shell->priv->playlist));
        else
                gtk_widget_show (GTK_WIDGET (shell->priv->playlist));
}

static gboolean
ario_shell_plugins_window_delete_cb (GtkWidget *window,
                                     GdkEventAny *event,
                                     gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        gtk_widget_destroy (GTK_WIDGET (window));
        return TRUE;
}

static void
ario_shell_plugins_response_cb (GtkDialog *dialog,
                                int response_id,
                                gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
ario_shell_cmd_plugins (GtkAction *action,
                        ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *window;
        GtkWidget *manager;

        /* Create plugins configuration dialog window */
        window = gtk_dialog_new_with_buttons (_("Configure Plugins"),
                                              GTK_WINDOW (shell),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CLOSE,
                                              GTK_RESPONSE_CLOSE,
                                              NULL);
        gtk_container_set_border_width (GTK_CONTAINER (window), 5);
        gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window))), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (window), FALSE);

        /* Connect signals for window destruction */
        g_signal_connect (window,
                          "delete_event",
                          G_CALLBACK (ario_shell_plugins_window_delete_cb),
                          NULL);
        g_signal_connect (window,
                          "response",
                          G_CALLBACK (ario_shell_plugins_response_cb),
                          NULL);

        manager = ario_plugin_manager_new ();
        gtk_widget_show_all (GTK_WIDGET (manager));
        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (window))),
                           manager);

        gtk_window_set_default_size (GTK_WINDOW (window), 300, 350);

        gtk_window_present (GTK_WINDOW (window));
}
