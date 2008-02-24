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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <config.h>
#include <string.h>
#include "lib/eel-gconf-extensions.h"
#include <glib/gi18n.h>
#include "shell/ario-shell.h"
#include "sources/ario-source-manager.h"
#include "ario-mpd.h"
#include "ario-cover-handler.h"
#include "widgets/ario-playlist.h"
#include "widgets/ario-header.h"
#include "widgets/ario-tray-icon.h"
#include "widgets/ario-status-bar.h"
#include "preferences/ario-preferences.h"
#include "shell/ario-shell-preferences.h"
#include "shell/ario-shell-lyrics.h"
#include "shell/ario-shell-coverdownloader.h"
#include "shell/ario-shell-coverselect.h"
#include "widgets/ario-firstlaunch.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "plugins/ario-plugin-manager.h"

static void ario_shell_class_init (ArioShellClass *klass);
static void ario_shell_init (ArioShell *shell);
static void ario_shell_finalize (GObject *object);
static void ario_shell_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void ario_shell_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);
static void ario_shell_show (ArioShell *shell);
static void ario_shell_cmd_quit (GtkAction *action,
                                 ArioShell *shell);
static void ario_shell_cmd_save (GtkAction *action,
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
static void ario_shell_cmd_about (GtkAction *action,
                                  ArioShell *shell);
static void ario_shell_cmd_translate (GtkAction *action,
                                      ArioShell *shell);
static void ario_shell_mpd_state_changed_cb (ArioMpd *mpd,
                                             ArioShell *shell);
static void ario_shell_mpd_song_changed_cb (ArioMpd *mpd,
                                            ArioShell *shell);
static void ario_shell_window_show_cb (GtkWidget *widget,
                                       ArioShell *shell);
static void ario_shell_window_hide_cb (GtkWidget *widget,
                                       ArioShell *shell);
static gboolean ario_shell_window_state_cb (GtkWidget *widget,
                                            GdkEvent *event,
                                            ArioShell *shell);
static void ario_shell_sync_window_state (ArioShell *shell);
static void ario_shell_sync_paned (ArioShell *shell);
static void ario_shell_sync_mpd (ArioShell *shell);
static void ario_shell_firstlaunch_delete_cb (GtkObject *firstlaunch,
                                              ArioShell *shell);
static void ario_shell_view_statusbar_changed_cb (GtkAction *action,
				                  ArioShell *shell);
static void ario_shell_sync_statusbar_visibility (ArioShell *shell);

struct ArioShellPrivate
{
        GtkWidget *window;

        ArioMpd *mpd;
        ArioCoverHandler *cover_handler;
        GtkWidget *header;
        GtkWidget *vpaned;
        GtkWidget *sourcemanager;
        GtkWidget *playlist;
        GtkWidget *status_bar;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        ArioTrayIcon *tray_icon;

        GtkWidget *plugins;

	gboolean statusbar_hidden;
        gboolean connected;
        gboolean shown;
};

enum
{
        PROP_0,
        PROP_MPD,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
};

static GtkActionEntry ario_shell_actions [] =
{
        { "File", NULL, N_("_File") },
        { "Edit", NULL, N_("_Edit") },
        { "View", NULL, N_("_View") },
        { "Tool", NULL, N_("_Tool") },
        { "Help", NULL, N_("_Help") },

        { "FileSave", GTK_STOCK_SAVE, N_("_Save Playlist"), "<control>S",
                NULL,
                G_CALLBACK (ario_shell_cmd_save) },
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
       { "ToolCoverSelect", GTK_STOCK_FIND, N_("_Change current album cover"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_cover_select) },
        { "ToolCover", GTK_STOCK_EXECUTE, N_("Download album _covers"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_covers) },
        { "Lyrics", GTK_STOCK_EDIT, N_("Show _lyrics"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_lyrics) },
        { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_about) },
        { "HelpTranslate", GTK_STOCK_EDIT, N_("_Translate this application..."), NULL,
                NULL,
                G_CALLBACK (ario_shell_cmd_translate) },
};
static guint ario_shell_n_actions = G_N_ELEMENTS (ario_shell_actions);

static GtkToggleActionEntry ario_shell_toggle [] =
{
        { "ViewStatusbar", NULL, N_("S_tatusbar"), NULL,
	  NULL,
	  G_CALLBACK (ario_shell_view_statusbar_changed_cb), TRUE }
};
static guint ario_shell_n_toggle = G_N_ELEMENTS (ario_shell_toggle);

static GObjectClass *parent_class;

GType
ario_shell_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (type == 0)
        { 
                static GTypeInfo info =
                {
                        sizeof (ArioShellClass),
                        NULL, 
                        NULL,
                        (GClassInitFunc) ario_shell_class_init, 
                        NULL,
                        NULL, 
                        sizeof (ArioShell),
                        0,
                        (GInstanceInitFunc) ario_shell_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "ArioShell",
                                               &info, 0);
        }

        return type;
}

static void
ario_shell_class_init (ArioShellClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        object_class->set_property = ario_shell_set_property;
        object_class->get_property = ario_shell_get_property;
        object_class->finalize = ario_shell_finalize;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
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
}

static void
ario_shell_init (ArioShell *shell) 
{
        ARIO_LOG_FUNCTION_START
        shell->priv = g_new0 (ArioShellPrivate, 1);
        shell->priv->connected = FALSE;
        shell->priv->shown = FALSE;
}

static void
ario_shell_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShell *shell = ARIO_SHELL (object);

        gtk_widget_hide (shell->priv->window);

        gtk_widget_destroy (shell->priv->window);
        g_object_unref (shell->priv->cover_handler);
        g_object_unref (shell->priv->mpd);
        gtk_widget_destroy (GTK_WIDGET (shell->priv->tray_icon));
        g_object_unref (shell->priv->ui_manager);
        g_object_unref (shell->priv->actiongroup);
        g_free (shell->priv);

        parent_class->finalize (G_OBJECT (shell));
}

static void
ario_shell_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioShell *shell = ARIO_SHELL (object);

        switch (prop_id) {
        case PROP_MPD:
                shell->priv->mpd = g_value_get_object (value);
                break;
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
        ARIO_LOG_FUNCTION_START
        ArioShell *shell = ARIO_SHELL (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, shell->priv->mpd);
                break;
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
        ARIO_LOG_FUNCTION_START
        ArioShell *s;

        s = g_object_new (TYPE_ARIO_SHELL, NULL);

        return s;
}

static gboolean
ario_shell_window_delete_cb (GtkWidget *win,
                             GdkEventAny *event,
                             ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        gtk_main_quit ();
        return TRUE;
};

void
ario_shell_construct (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWindow *win;
        GtkWidget *menubar;
        GtkWidget *separator;
        GtkWidget *vbox;
        GdkPixbuf *pixbuf;
        ArioFirstlaunch *firstlaunch;
        GtkAction *action;

        g_return_if_fail (IS_ARIO_SHELL (shell));

        /* initialize UI */
        win = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
        gtk_window_set_title (win, "Ario");
        pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "ario.png", NULL);
        gtk_window_set_default_icon (pixbuf);

        shell->priv->window = GTK_WIDGET (win);

        g_signal_connect_object (G_OBJECT (win), "delete_event",
                                 G_CALLBACK (ario_shell_window_delete_cb),
                                 shell, 0);

        shell->priv->ui_manager = gtk_ui_manager_new ();
        shell->priv->actiongroup = gtk_action_group_new ("MainActions");

        gtk_action_group_set_translation_domain (shell->priv->actiongroup,
                                                 GETTEXT_PACKAGE);

        /* initialize shell services */
        vbox = gtk_vbox_new (FALSE, 0);

        shell->priv->mpd = ario_mpd_new ();
        shell->priv->cover_handler = ario_cover_handler_new (shell->priv->mpd);
        shell->priv->header = ario_header_new (shell->priv->mpd,
                                               shell->priv->cover_handler);
        separator = gtk_hseparator_new ();
        shell->priv->playlist = ario_playlist_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd);
        shell->priv->sourcemanager = ario_sourcemanager_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd, ARIO_PLAYLIST (shell->priv->playlist));

        gtk_action_group_add_actions (shell->priv->actiongroup,
                                      ario_shell_actions,
                                      ario_shell_n_actions, shell);

	gtk_action_group_add_toggle_actions (shell->priv->actiongroup,
					     ario_shell_toggle,
					     ario_shell_n_toggle,
					     shell);
        gtk_ui_manager_insert_action_group (shell->priv->ui_manager,
                                            shell->priv->actiongroup, 0);
        gtk_ui_manager_add_ui_from_file (shell->priv->ui_manager,
                                         UI_PATH "ario-ui.xml", NULL);
        gtk_window_add_accel_group (GTK_WINDOW (shell->priv->window),
                                    gtk_ui_manager_get_accel_group (shell->priv->ui_manager));

        /* initialize tray icon */
        shell->priv->tray_icon = ario_tray_icon_new (shell->priv->actiongroup,
                                                     shell->priv->ui_manager,
                                                     win,
                                                     shell->priv->mpd,
                                                     shell->priv->cover_handler);
        gtk_widget_show_all (GTK_WIDGET (shell->priv->tray_icon));

        menubar = gtk_ui_manager_get_widget (shell->priv->ui_manager, "/MenuBar");
        shell->priv->vpaned = gtk_vpaned_new ();
        shell->priv->status_bar = ario_status_bar_new (shell->priv->mpd);
	shell->priv->statusbar_hidden = eel_gconf_get_boolean (CONF_STATUSBAR_HIDDEN, FALSE);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewStatusbar");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->statusbar_hidden);

        gtk_paned_pack1 (GTK_PANED (shell->priv->vpaned),
                         shell->priv->sourcemanager,
                         FALSE, FALSE);

        gtk_paned_pack2 (GTK_PANED (shell->priv->vpaned),
                         shell->priv->playlist,
                         TRUE, FALSE);

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

        gtk_container_add (GTK_CONTAINER (win), vbox);

        g_signal_connect_object (G_OBJECT (shell->priv->window), "show",
                                 G_CALLBACK (ario_shell_window_show_cb),
                                 shell, 0);

        g_signal_connect_object (G_OBJECT (shell->priv->window), "hide",
                                 G_CALLBACK (ario_shell_window_hide_cb),
                                 shell, 0);

        ario_shell_sync_window_state (shell);

        /* First launch assistant */
        if (!eel_gconf_get_boolean (CONF_FIRST_TIME, FALSE)) {
                firstlaunch = ario_firstlaunch_new ();
                g_signal_connect_object (G_OBJECT (firstlaunch), "destroy",
                                         G_CALLBACK (ario_shell_firstlaunch_delete_cb),
                                         shell, 0);
                gtk_widget_show_all (GTK_WIDGET (firstlaunch));
        } else {
                ario_shell_show (shell);
        }
        ario_shell_sync_statusbar_visibility (shell);
}

void
ario_shell_shutdown (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int width, height;

        if (shell->priv->shown) {
                eel_gconf_set_integer (CONF_VPANED_POSITION,
                                       gtk_paned_get_position (GTK_PANED (shell->priv->vpaned)));

                gtk_window_get_size (GTK_WINDOW (shell->priv->window),
                                     &width,
                                     &height);

                if (!eel_gconf_get_boolean (CONF_WINDOW_MAXIMIZED, TRUE)) {
                        eel_gconf_set_integer (CONF_WINDOW_WIDTH, width);
                        eel_gconf_set_integer (CONF_WINDOW_HEIGHT, height);
                }

                ario_playlist_shutdown (ARIO_PLAYLIST (shell->priv->playlist));
                ario_sourcemanager_shutdown (ARIO_SOURCEMANAGER (shell->priv->sourcemanager));
        }
}

static void
ario_shell_show (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START

        g_signal_connect_object (G_OBJECT (shell->priv->mpd),
                                 "state_changed", G_CALLBACK (ario_shell_mpd_state_changed_cb),
                                 shell, 0);

        g_signal_connect_object (G_OBJECT (shell->priv->mpd),
                                 "song_changed", G_CALLBACK (ario_shell_mpd_song_changed_cb),
                                 shell, 0);
                                 
        if (eel_gconf_get_boolean (CONF_AUTOCONNECT, TRUE))
                ario_mpd_connect (shell->priv->mpd);

        ario_shell_sync_paned (shell);
        ario_shell_sync_mpd (shell);

        gtk_widget_show_all (GTK_WIDGET (shell->priv->window));

        g_signal_connect_object (G_OBJECT (shell->priv->window), "window-state-event",
                                 G_CALLBACK (ario_shell_window_state_cb),
                                 shell, 0);

        shell->priv->shown = TRUE;
}

static void
ario_shell_cmd_quit (GtkAction *action,
                     ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        gtk_main_quit ();
}

static void
ario_shell_cmd_save (GtkAction *action,
                     ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_playlist_cmd_save (action, ARIO_PLAYLIST (shell->priv->playlist));
}

static void
ario_shell_cmd_connect (GtkAction *action,
                        ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_connect (shell->priv->mpd);
}

static void
ario_shell_cmd_disconnect (GtkAction *action,
                           ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_disconnect (shell->priv->mpd);
}

static void
ario_shell_cmd_preferences (GtkAction *action,
                            ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *prefs;

        prefs = ario_shell_preferences_new (shell->priv->mpd);

        gtk_window_set_transient_for (GTK_WINDOW (prefs),
                                      GTK_WINDOW (shell->priv->window));

        gtk_widget_show_all (prefs);
}

static void
ario_shell_cmd_lyrics (GtkAction *action,
                       ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *lyrics;

        lyrics = ario_shell_lyrics_new (shell->priv->mpd);
        if (lyrics)
                gtk_widget_show_all (lyrics);
}

static void
ario_shell_cmd_about (GtkAction *action,
                      ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        const char *authors[] = {
#include "AUTHORS"
                "",
                NULL
        };
        const char *artists[] = {
                "Luc Pavot",
                NULL
        };
        GdkPixbuf *logo_pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "logo.png", NULL);
        gtk_show_about_dialog (GTK_WINDOW (shell->priv->window),
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
                g_object_unref(logo_pixbuf);
}

static void
ario_shell_cmd_translate (GtkAction *action,
                          ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        const gchar *command = "x-www-browser https://translations.launchpad.net/ario/trunk/";
        g_spawn_command_line_async (command, NULL);
}

static void
ario_shell_mpd_song_set_title (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        gchar *window_title;
        gchar *tmp;
        
        switch (ario_mpd_get_current_state (shell->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                tmp = ario_util_format_title (ario_mpd_get_current_song (shell->priv->mpd));
                window_title = g_strdup_printf ("Ario - %s", tmp);
                g_free (tmp);
                break;
        default:
                window_title = g_strdup("Ario");
                break;
        }
        
        gtk_window_set_title (GTK_WINDOW (shell->priv->window), window_title);
        g_free (window_title);
}

static void
ario_shell_mpd_song_changed_cb (ArioMpd *mpd,
                                ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_mpd_song_set_title (shell);
}

static void
ario_shell_mpd_state_changed_cb (ArioMpd *mpd,
                                 ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        if (shell->priv->connected != ario_mpd_is_connected (mpd)) {
                shell->priv->connected = ario_mpd_is_connected (mpd);
                ario_shell_sync_mpd (shell);
        }
        ario_shell_mpd_song_set_title (shell);
}

static void
ario_shell_cmd_cover_select (GtkAction *action,
                             ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverselect;
        char *artist;
        char *album;

        artist = ario_mpd_get_current_artist (shell->priv->mpd);
        album = ario_mpd_get_current_album (shell->priv->mpd);

        if (!album)
                album = ARIO_MPD_UNKNOWN;

        if (!artist)
                artist = ARIO_MPD_UNKNOWN;

        coverselect = ario_shell_coverselect_new (artist,
                                                  album);
        gtk_dialog_run (GTK_DIALOG (coverselect));
        gtk_widget_destroy (coverselect);
}

static void
ario_shell_cmd_covers (GtkAction *action,
                       ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverdownloader;

        coverdownloader = ario_shell_coverdownloader_new (shell->priv->mpd);

        ario_shell_coverdownloader_get_covers (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                               GET_AMAZON_COVERS);

        gtk_widget_destroy (coverdownloader);
}

static void
ario_shell_sync_paned (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int pos;

        pos = eel_gconf_get_integer (CONF_VPANED_POSITION, 400);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (shell->priv->vpaned),
                                        pos);
}

static void
ario_shell_window_show_cb (GtkWidget *widget,
                           ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_use_count_inc (shell->priv->mpd);
}

static void
ario_shell_window_hide_cb (GtkWidget *widget,
                           ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_mpd_use_count_dec (shell->priv->mpd);
}

static gboolean
ario_shell_window_state_cb (GtkWidget *widget,
                            GdkEvent *event,
                            ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int width, height;
        g_return_val_if_fail (widget != NULL, FALSE);

        if (event->type == GDK_WINDOW_STATE) {
                eel_gconf_set_boolean (CONF_WINDOW_MAXIMIZED,
                                       event->window_state.new_window_state &
                                       GDK_WINDOW_STATE_MAXIMIZED);

                gtk_window_get_size (GTK_WINDOW (shell->priv->window),
                                     &width,
                                     &height);

                eel_gconf_set_integer (CONF_WINDOW_WIDTH, width);
                eel_gconf_set_integer (CONF_WINDOW_HEIGHT, height);
        }

        return FALSE;
}

static void
ario_shell_sync_window_state (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int width = eel_gconf_get_integer (CONF_WINDOW_WIDTH, 600); 
        int height = eel_gconf_get_integer (CONF_WINDOW_HEIGHT, 600);
        gboolean maximized = eel_gconf_get_boolean (CONF_WINDOW_MAXIMIZED, TRUE);
        GdkGeometry hints;

        gtk_window_set_default_size (GTK_WINDOW (shell->priv->window),
                                     width, height);
        gtk_window_resize (GTK_WINDOW (shell->priv->window),
                           width, height);
        gtk_window_set_geometry_hints (GTK_WINDOW (shell->priv->window),
                                       NULL,
                                       &hints,
                                       0);

        if (maximized)
                gtk_window_maximize (GTK_WINDOW (shell->priv->window));
        else
                gtk_window_unmaximize (GTK_WINDOW (shell->priv->window));
}

static void
ario_shell_sync_mpd (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkAction *connect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                                 "FileConnect");
        GtkAction *disconnect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                                    "FileDisconnect");

        gtk_action_set_visible (connect_action, !shell->priv->connected);
        gtk_action_set_visible (disconnect_action, shell->priv->connected);
}

static void
ario_shell_firstlaunch_delete_cb (GtkObject *firstlaunch,
                                  ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_show (shell);
}

static void
ario_shell_view_statusbar_changed_cb (GtkAction *action,
				      ArioShell *shell)
{
	shell->priv->statusbar_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	eel_gconf_set_boolean (CONF_STATUSBAR_HIDDEN, shell->priv->statusbar_hidden);

	ario_shell_sync_statusbar_visibility (shell);
}

static void
ario_shell_sync_statusbar_visibility (ArioShell *shell)
{
	if (shell->priv->statusbar_hidden)
		gtk_widget_hide (GTK_WIDGET (shell->priv->status_bar));
	else
		gtk_widget_show (GTK_WIDGET (shell->priv->status_bar));
}

static gboolean
ario_shell_plugins_window_delete_cb (GtkWidget *window,
				     GdkEventAny *event,
				     gpointer data)
{
	gtk_widget_hide (window);

	return TRUE;
}

static void
ario_shell_plugins_response_cb (GtkDialog *dialog,
			        int response_id,
			        gpointer data)
{
	if (response_id == GTK_RESPONSE_CLOSE)
		gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
ario_shell_cmd_plugins (GtkAction *action,
		        ArioShell *shell)
{
	if (shell->priv->plugins == NULL) {
		GtkWidget *manager;

		shell->priv->plugins = gtk_dialog_new_with_buttons (_("Configure Plugins"),
								    GTK_WINDOW (shell->priv->window),
								    GTK_DIALOG_DESTROY_WITH_PARENT,
								    GTK_STOCK_CLOSE,
								    GTK_RESPONSE_CLOSE,
								    NULL);
	    	gtk_container_set_border_width (GTK_CONTAINER (shell->priv->plugins), 5);
		gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (shell->priv->plugins)->vbox), 2);
		gtk_dialog_set_has_separator (GTK_DIALOG (shell->priv->plugins), FALSE);

		g_signal_connect_object (G_OBJECT (shell->priv->plugins),
					 "delete_event",
					 G_CALLBACK (ario_shell_plugins_window_delete_cb),
					 NULL, 0);
		g_signal_connect_object (G_OBJECT (shell->priv->plugins),
					 "response",
					 G_CALLBACK (ario_shell_plugins_response_cb),
					 NULL, 0);

		manager = ario_plugin_manager_new ();
		gtk_widget_show_all (GTK_WIDGET (manager));
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell->priv->plugins)->vbox),
				   manager);

                
                gtk_window_set_default_size (GTK_WINDOW (shell->priv->plugins), 300, 350);
	}

	gtk_window_present (GTK_WINDOW (shell->priv->plugins));
}

GtkWidget *
ario_shell_get_playlist (ArioShell *shell)
{
        return shell->priv->playlist;
}

GtkWidget *
ario_shell_get_sourcemanager (ArioShell *shell)
{
        return shell->priv->sourcemanager;
}
