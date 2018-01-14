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

static void ario_shell_finalize (GObject *object);
static void ario_shell_show (ArioShell *shell,
                             gboolean minimized);
static void ario_shell_cmd_quit (GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data);
static void ario_shell_cmd_connect (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data);
static void ario_shell_cmd_disconnect (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer data);
static void ario_shell_cmd_update (GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data);
static void ario_shell_cmd_plugins (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer data);
static void ario_shell_cmd_preferences (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data);
static void ario_shell_cmd_lyrics (GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data);
static void ario_shell_cmd_cover_select (GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer data);
static void ario_shell_cmd_covers (GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer data);
static void ario_shell_cmd_similar_artists (GSimpleAction *action,
                                            GVariant *parameter,
                                            gpointer data);
static void ario_shell_cmd_add_similar (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer data);
static void ario_shell_cmd_about (GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer data);
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
static void ario_shell_sync_playlist_position (ArioShell *shell);
static void ario_shell_firstlaunch_delete_cb (ArioFirstlaunch *firstlaunch,
                                              ArioShell *shell);
static void ario_shell_view_statusbar_changed_cb (GSimpleAction *action,
                                                  GVariant *parameter,
                                                  gpointer data);
static void ario_shell_view_upperpart_changed_cb (GSimpleAction *action,
                                                  GVariant *parameter,
                                                  gpointer data);
static void ario_shell_view_playlist_changed_cb (GSimpleAction *action,
                                                 GVariant *parameter,
                                                 gpointer data);
static void ario_shell_sync_statusbar_visibility (ArioShell *shell);
static void ario_shell_sync_upperpart_visibility (ArioShell *shell);
static void ario_shell_sync_playlist_visibility (ArioShell *shell);
static void ario_shell_playlist_position_changed_cb (guint notification_id,
                                                     ArioShell *shell);

struct ArioShellPrivate
{
        GtkApplication * app;

        ArioCoverHandler *cover_handler;
        ArioPlaylistManager *playlist_manager;
        ArioNotificationManager *notification_manager;
        GtkWidget *header;
        GtkWidget *paned;
        GtkWidget *sourcemanager;
        GtkWidget *playlist;
        GtkWidget *status_bar;
        GtkWidget *vbox;
        GtkWidget *hbox;

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
};

static const GActionEntry shell_actions[] = {
        { "connect", ario_shell_cmd_connect},
        { "disconnect", ario_shell_cmd_disconnect},
        { "update", ario_shell_cmd_update},
        { "view-upperpart", ario_shell_view_upperpart_changed_cb, NULL, "false" },
        { "view-playlist", ario_shell_view_playlist_changed_cb, NULL, "false" },
        { "view-statusbar", ario_shell_view_statusbar_changed_cb, NULL, "false" },
        { "view-lyrics", ario_shell_cmd_lyrics},
        { "cover-select", ario_shell_cmd_cover_select},
        { "dlcovers", ario_shell_cmd_covers},
        { "similar-artists", ario_shell_cmd_similar_artists},
        { "add-similar", ario_shell_cmd_add_similar},
        { "preferences", ario_shell_cmd_preferences},
        { "plugins", ario_shell_cmd_plugins},
        { "about", ario_shell_cmd_about},
        { "quit", ario_shell_cmd_quit},
};

#define ARIO_SHELL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ARIO_TYPE_SHELL, ArioShellPrivate))
G_DEFINE_TYPE (ArioShell, ario_shell, GTK_TYPE_APPLICATION_WINDOW)

static void
ario_shell_class_init (ArioShellClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = (GObjectClass *) klass;

        /* Virtual methods */
        object_class->finalize = ario_shell_finalize;

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

        g_object_unref (shell->priv->playlist);
        g_object_unref (shell->priv->sourcemanager);
        g_object_unref (shell->priv->cover_handler);
        g_object_unref (shell->priv->playlist_manager);
        g_object_unref (shell->priv->notification_manager);

        g_object_unref (ario_server_get_instance ());

        G_OBJECT_CLASS (ario_shell_parent_class)->finalize (object);
}

ArioShell *
ario_shell_new (GtkApplication * app)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell;

        shell = g_object_new (ARIO_TYPE_SHELL, NULL);
        shell->priv->app = app;

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
        g_application_quit (G_APPLICATION (shell->priv->app));
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
        GtkWidget *separator;
        GAction *action;
        ArioFirstlaunch *firstlaunch;
        GtkBuilder *builder;
        GMenuModel *menu;

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
        builder = gtk_builder_new_from_file (UI_PATH "ario-shell-menu.ui");
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
        if (!g_application_get_is_remote (G_APPLICATION (shell->priv->app))) {
                gtk_application_set_app_menu (shell->priv->app,
                                              menu);
        }
        g_object_unref (builder);

        /* Main window actions */
        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         shell_actions, G_N_ELEMENTS (shell_actions),
                                         shell);

        /* Initialize server object (MPD, XMMS, ....) */
        ario_server_get_instance ();

        /* Initialize cover art handler */
        shell->priv->cover_handler = ario_cover_handler_new ();

        /* Initialize playlist manager */
        shell->priv->playlist_manager = ario_playlist_manager_get_instance ();

        /* Initialize notification manager */
        shell->priv->notification_manager = ario_notification_manager_get_instance ();

        /* Add widgets to main window.
         * Structure is:
         * vbox
         * ---header
         * ---separator
         * ---hbox
         * -----hpaned/vpaned
         * -------sourcemanager
         * -------playlist
         * ---status_bar
         */

        /* Create main vbox */
        shell->priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

        /* Create header */
        shell->priv->header = ario_header_new ();

        /* Create separator */
        separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

        /* Create playlist */
        shell->priv->playlist = ario_playlist_new ();
        g_object_ref (shell->priv->playlist);

        /* Create source manager */
        shell->priv->sourcemanager = ario_source_manager_get_instance ();
        g_object_ref (shell->priv->sourcemanager);

        /* Create the hbox(for tabs and playlist) */
        shell->priv->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

        /* Create status bar */
        shell->priv->status_bar = ario_status_bar_new ();

        /* Synchronize status bar checkbox in menu with preferences */
        shell->priv->statusbar_hidden = ario_conf_get_boolean (PREF_STATUSBAR_HIDDEN, PREF_STATUSBAR_HIDDEN_DEFAULT);
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                             "view-statusbar");
        g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (!shell->priv->statusbar_hidden));

        /* Synchronize upper part checkbox in menu with preferences */
        shell->priv->upperpart_hidden = ario_conf_get_boolean (PREF_UPPERPART_HIDDEN, PREF_UPPERPART_HIDDEN_DEFAULT);
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                             "view-upperpart");
        g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (!shell->priv->upperpart_hidden));

        /* Synchronize playlist checkbox in menu with preferences */
        shell->priv->playlist_hidden = ario_conf_get_boolean (PREF_PLAYLIST_HIDDEN, PREF_PLAYLIST_HIDDEN_DEFAULT);
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                             "view-playlist");
        g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (!shell->priv->playlist_hidden));

        /* Add widgets to vbox */
        gtk_box_pack_start (GTK_BOX (shell->priv->vbox),
                            shell->priv->header,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (shell->priv->vbox),
                            separator,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (shell->priv->vbox),
                            shell->priv->hbox,
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (shell->priv->vbox),
                            shell->priv->status_bar,
                            FALSE, FALSE, 0);

        /* Add vbox to main window */
        gtk_container_add (GTK_CONTAINER (shell), shell->priv->vbox);

        /* First launch assistant */
        if (!ario_conf_get_boolean (PREF_FIRST_TIME, PREF_FIRST_TIME_DEFAULT)) {
                firstlaunch = ario_firstlaunch_new (shell->priv->app);
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
                if (shell->priv->paned) {
                        /* Save paned position */
                        ario_conf_set_integer (PREF_VPANED_POSITION,
                                               gtk_paned_get_position (GTK_PANED (shell->priv->paned)));
                }

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

        /* Show main window */
        gtk_widget_show_all (GTK_WIDGET(shell));
        shell->priv->shown = TRUE;

        /* Synchonize the main window with server state */
        ario_shell_sync_server (shell);

        /* Synchronize playlist position with preferences */
        ario_shell_sync_playlist_position (shell);

        /* Synchronize paned with preferences */
        ario_shell_sync_paned (shell);

        /* Minimize window if needed */
        if (minimized)
                ario_shell_set_visibility (shell, VISIBILITY_HIDDEN);

        /* Connect signal for window state changes */
        g_signal_connect_object (shell,
                                 "window-state-event",
                                 G_CALLBACK (ario_shell_window_state_cb),
                                 shell,
                                 G_CONNECT_AFTER);

        /* Update server db on startup if needed */
        if (ario_conf_get_boolean (PREF_UPDATE_STARTUP, PREF_UPDATE_STARTUP_DEFAULT))
                ario_server_update_db (NULL);

        /* Notification for trees configuration changes */
        ario_conf_notification_add (PREF_PLAYLIST_POSITION,
                                    (ArioNotifyFunc) ario_shell_playlist_position_changed_cb,
                                    shell);
}

void
ario_shell_present (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        if (ario_conf_get_boolean (PREF_FIRST_TIME, PREF_FIRST_TIME_DEFAULT)) {
                ario_shell_set_visibility(shell, VISIBILITY_VISIBLE);
                gtk_window_present (GTK_WINDOW (shell));
        }
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
                        gtk_widget_show (GTK_WIDGET(shell));

                        /* Restore paned position
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

                        /* Save paned position */
                        ario_conf_set_integer (PREF_VPANED_POSITION,
                                               gtk_paned_get_position (GTK_PANED (shell->priv->paned)));

                        gtk_widget_hide (GTK_WIDGET(shell));
                }
        }
}

static void
ario_shell_cmd_quit (GSimpleAction *action,
                     GVariant *parameter,
                     gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell = ARIO_SHELL (data);
        ario_shell_quit (shell);
}

static void
ario_shell_cmd_connect (GSimpleAction *action,
                        GVariant *parameter,
                        gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_connect ();
}

static void
ario_shell_cmd_disconnect (GSimpleAction *action,
                           GVariant *parameter,
                           gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_disconnect ();
}

static void
ario_shell_cmd_update (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ario_server_update_db (NULL);
}

static void
ario_shell_cmd_preferences (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *prefs;
        ArioShell *shell = ARIO_SHELL (data);

        /* Create preferences dialog window */
        prefs = ario_shell_preferences_new ();

        gtk_window_set_transient_for (GTK_WINDOW (prefs),
                                      GTK_WINDOW (shell));

        gtk_widget_show_all (prefs);
}

static void
ario_shell_cmd_lyrics (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *lyrics;

        /* Create lyrics dialog window */
        lyrics = ario_shell_lyrics_new ();
        if (lyrics)
                gtk_widget_show_all (lyrics);
}

static void
ario_shell_cmd_about (GSimpleAction *action,
                      GVariant *parameter,
                      gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioShell *shell = ARIO_SHELL (data);

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
                               "copyright", "Copyright \xc2\xa9 2005-" CURRENT_DATE " Marc Pavot",
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
ario_shell_cmd_cover_select (GSimpleAction *action,
                             GVariant *parameter,
                             gpointer data)
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
ario_shell_cmd_covers (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer data)
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
ario_shell_cmd_similar_artists (GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *similarartists;

        /* Launch similar artist dialog */
        similarartists = ario_shell_similarartists_new ();
        if (similarartists)
                gtk_widget_show_all (similarartists);
}

static void
ario_shell_cmd_add_similar (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer data)
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

        /* Set paned position */
        pos = ario_conf_get_integer (PREF_VPANED_POSITION, PREF_VPANED_POSITION_DEFAULT);
        if (pos > 0 && shell->priv->paned)
                gtk_paned_set_position (GTK_PANED (shell->priv->paned),
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
        GAction *connect_action;
        GAction *disconnect_action;
        gboolean is_playing;
        GAction *action;

        /* Set connect entry visibility */
        connect_action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                                      "connect");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (connect_action), !shell->priv->connected);

        /* Set disconnect entry visibility */
        disconnect_action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                                         "disconnect");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (disconnect_action), shell->priv->connected);

        is_playing = ((shell->priv->connected)
                      && ((ario_server_get_current_state () == ARIO_STATE_PLAY)
                          || (ario_server_get_current_state () == ARIO_STATE_PAUSE)));

        /* Set lyrics entry sensitivty */
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                              "view-lyrics");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_playing);

        /* Set cover selection entry sensitivty */
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                              "cover-select");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_playing);

        /* Set similar artists entry sensitivty */
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                              "similar-artists");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_playing);

        /* Set similar artists addition entry sensitivty */
        action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                              "add-similar");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_playing);
}

static void
ario_shell_sync_playlist_position (ArioShell *shell)
{
        if (!shell->priv->playlist)
                return;

        /* Remove all widgets if playlist is in tabs */
        int page_num = gtk_notebook_page_num (GTK_NOTEBOOK (shell->priv->sourcemanager), shell->priv->playlist);
        if (page_num >= 0) {
                ario_source_manager_remove (ARIO_SOURCE (shell->priv->playlist));
                gtk_container_remove (GTK_CONTAINER (shell->priv->hbox), shell->priv->sourcemanager);
        }

        /* Remove all widgets if playlist is in a paned */
        if (shell->priv->paned) {
                /* Save paned position */
                ario_conf_set_integer (PREF_VPANED_POSITION,
                                       gtk_paned_get_position (GTK_PANED (shell->priv->paned)));

                gtk_container_remove (GTK_CONTAINER (shell->priv->paned), shell->priv->playlist);
                gtk_container_remove (GTK_CONTAINER (shell->priv->paned), shell->priv->sourcemanager);
                gtk_container_remove (GTK_CONTAINER (shell->priv->hbox), shell->priv->paned);
                shell->priv->paned = NULL;
        }

        switch (ario_conf_get_integer (PREF_PLAYLIST_POSITION, PREF_PLAYLIST_POSITION_DEFAULT))
        {
        case PLAYLIST_POSITION_BELOW:
                /* Create paned (separation between upper part and playlist) */
                shell->priv->paned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);

                /* Add widgets to paned */
                gtk_paned_pack1 (GTK_PANED (shell->priv->paned),
                                 shell->priv->sourcemanager,
                                 FALSE, FALSE);

                gtk_paned_pack2 (GTK_PANED (shell->priv->paned),
                                 shell->priv->playlist,
                                 TRUE, FALSE);

                gtk_box_pack_start (GTK_BOX (shell->priv->hbox),
                                    shell->priv->paned,
                                    TRUE, TRUE, 0);
                gtk_widget_show_all (shell->priv->paned);

                /* Synchronize paned with preferences */
                ario_shell_sync_paned (shell);
                break;
        case PLAYLIST_POSITION_RIGHT:
                /* Create paned (separation between left part and playlist) */
                shell->priv->paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

                /* Add widgets to paned */
                gtk_paned_pack1 (GTK_PANED (shell->priv->paned),
                                 shell->priv->sourcemanager,
                                 FALSE, FALSE);

                gtk_paned_pack2 (GTK_PANED (shell->priv->paned),
                                 shell->priv->playlist,
                                 TRUE, FALSE);

                gtk_box_pack_start (GTK_BOX (shell->priv->hbox),
                                    shell->priv->paned,
                                    TRUE, TRUE, 0);
                gtk_widget_show_all (shell->priv->paned);

                /* Synchronize paned with preferences */
                ario_shell_sync_paned (shell);
                break;
        case PLAYLIST_POSITION_INSIDE:
        default:
                /* Add playlist to tabs */
                ario_source_manager_append (ARIO_SOURCE (shell->priv->playlist));
                gtk_box_pack_start (GTK_BOX (shell->priv->hbox),
                                    shell->priv->sourcemanager,
                                    TRUE, TRUE, 0);
                gtk_widget_show_all (shell->priv->sourcemanager);
                break;
        }
}

static void
ario_shell_firstlaunch_delete_cb (ArioFirstlaunch *firstlaunch,
                                  ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START;
        /* Show main window when first launch assistant is deleted */
        ario_shell_show (shell, FALSE);
}

static void
ario_shell_view_statusbar_changed_cb (GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GVariant *old_state, *new_state;
        ArioShell *shell = ARIO_SHELL (data);

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        shell->priv->statusbar_hidden = !g_variant_get_boolean (new_state);
        ario_conf_set_boolean (PREF_STATUSBAR_HIDDEN, shell->priv->statusbar_hidden);
 
        /* Synchronize status bar visibility */
        ario_shell_sync_statusbar_visibility (shell);
}

static void
ario_shell_view_upperpart_changed_cb (GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GVariant *old_state, *new_state;
        ArioShell *shell = ARIO_SHELL (data);

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        shell->priv->upperpart_hidden = !g_variant_get_boolean (new_state);
        ario_conf_set_boolean (PREF_UPPERPART_HIDDEN, shell->priv->upperpart_hidden);

        /* Synchronize upper part visibility */
        ario_shell_sync_upperpart_visibility (shell);
}

static void
ario_shell_view_playlist_changed_cb (GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GVariant *old_state, *new_state;
        ArioShell *shell = ARIO_SHELL (data);

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        shell->priv->playlist_hidden = !g_variant_get_boolean (new_state);
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
ario_shell_cmd_plugins (GSimpleAction *action,
                        GVariant *parameter,
                        gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *window;
        GtkWidget *manager;
        ArioShell *shell = ARIO_SHELL (data);

        /* Create plugins configuration dialog window */
        window = gtk_dialog_new_with_buttons (_("Configure Plugins"),
                                              GTK_WINDOW (shell),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              _("_Close"),
                                              GTK_RESPONSE_CLOSE,
                                              NULL);
        gtk_container_set_border_width (GTK_CONTAINER (window), 5);
        gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window))), 2);

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

static void
ario_shell_playlist_position_changed_cb (guint notification_id,
                                         ArioShell *shell)
{
        ario_shell_sync_playlist_position(shell);
}
