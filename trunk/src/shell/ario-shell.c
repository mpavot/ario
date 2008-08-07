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
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "shell/ario-shell.h"
#include "sources/ario-source-manager.h"
#include "ario-mpd.h"
#include "covers/ario-cover-handler.h"
#include "covers/ario-cover-manager.h"
#include "lyrics/ario-lyrics-manager.h"
#include "widgets/ario-playlist.h"
#include "widgets/ario-header.h"
#include "widgets/ario-status-bar.h"
#include "preferences/ario-preferences.h"
#include "shell/ario-shell-preferences.h"
#include "shell/ario-shell-lyrics.h"
#include "shell/ario-shell-similarartists.h"
#include "shell/ario-shell-coverdownloader.h"
#include "shell/ario-shell-coverselect.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "plugins/ario-plugin-manager.h"
#include "widgets/ario-firstlaunch.h"
#ifdef ENABLE_EGGTRAYICON
#include "widgets/ario-tray-icon.h"
#else
#include "widgets/ario-status-icon.h"
#endif

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
static void ario_shell_cmd_similar_artists (GtkAction *action,
                                            ArioShell *shell);
static void ario_shell_cmd_add_similar (GtkAction *action,
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
static void ario_shell_view_upperpart_changed_cb (GtkAction *action,
                                                  ArioShell *shell);
static void ario_shell_view_playlist_changed_cb (GtkAction *action,
                                                 ArioShell *shell);
static void ario_shell_sync_statusbar_visibility (ArioShell *shell);
static void ario_shell_sync_upperpart_visibility (ArioShell *shell);
static void ario_shell_sync_playlist_visibility (ArioShell *shell);

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
#ifdef ENABLE_EGGTRAYICON
        ArioTrayIcon *tray_icon;
#else
        ArioStatusIcon *status_icon;
#endif
        gboolean statusbar_hidden;
        gboolean upperpart_hidden;
        gboolean playlist_hidden;
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
static guint ario_shell_n_actions = G_N_ELEMENTS (ario_shell_actions);

static GtkToggleActionEntry ario_shell_toggle [] =
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
#ifdef ENABLE_EGGTRAYICON
        gtk_widget_destroy (GTK_WIDGET (shell->priv->tray_icon));;
#else
        g_object_unref (G_OBJECT (shell->priv->status_icon));
#endif

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
        if (ario_conf_get_boolean (PREF_HIDE_ON_CLOSE, PREF_HIDE_ON_CLOSE_DEFAULT)) {
#ifdef ENABLE_EGGTRAYICON
                ario_tray_icon_set_visibility (shell->priv->tray_icon, VISIBILITY_TOGGLE);
#else
                ario_status_icon_set_visibility (shell->priv->status_icon, VISIBILITY_TOGGLE);
#endif
        } else {
                gtk_main_quit ();
        }
        return TRUE;
};

void
ario_shell_construct (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *menubar;
        GtkWidget *separator;
        GtkWidget *vbox;
        GdkPixbuf *pixbuf;
        GtkAction *action;
        ArioFirstlaunch *firstlaunch;

        g_return_if_fail (IS_ARIO_SHELL (shell));

        /* initialize UI */
        shell->priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (shell->priv->window), "Ario");
        pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "ario.png", NULL);
        gtk_window_set_default_icon (pixbuf);

        g_signal_connect_object (G_OBJECT (shell->priv->window), "delete_event",
                                 G_CALLBACK (ario_shell_window_delete_cb),
                                 shell, 0);

        shell->priv->ui_manager = gtk_ui_manager_new ();
        shell->priv->actiongroup = gtk_action_group_new ("MainActions");

        gtk_ui_manager_add_ui_from_file (shell->priv->ui_manager,
                                         UI_PATH "ario-ui.xml", NULL);
        gtk_window_add_accel_group (GTK_WINDOW (shell->priv->window),
                                    gtk_ui_manager_get_accel_group (shell->priv->ui_manager));

        gtk_action_group_set_translation_domain (shell->priv->actiongroup,
                                                 GETTEXT_PACKAGE);
#ifdef WIN32
        gtk_action_group_set_translate_func (shell->priv->actiongroup,
                                             (GtkTranslateFunc) gettext,
                                             NULL, NULL);
#endif
        gtk_action_group_add_actions (shell->priv->actiongroup,
                                      ario_shell_actions,
                                      ario_shell_n_actions, shell);

        gtk_action_group_add_toggle_actions (shell->priv->actiongroup,
                                             ario_shell_toggle,
                                             ario_shell_n_toggle,
                                             shell);
        gtk_ui_manager_insert_action_group (shell->priv->ui_manager,
                                            shell->priv->actiongroup, 0);

        /* initialize shell services */
        vbox = gtk_vbox_new (FALSE, 0);

        shell->priv->mpd = ario_mpd_new ();
        shell->priv->cover_handler = ario_cover_handler_new (shell->priv->mpd);
        shell->priv->header = ario_header_new (shell->priv->mpd);
        separator = gtk_hseparator_new ();
        shell->priv->playlist = ario_playlist_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd);
        shell->priv->sourcemanager = ario_sourcemanager_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd, ARIO_PLAYLIST (shell->priv->playlist));

#ifdef ENABLE_EGGTRAYICON
        /* initialize tray icon */
        shell->priv->tray_icon = ario_tray_icon_new (shell->priv->actiongroup,
                                                     shell->priv->ui_manager,
                                                     GTK_WINDOW (shell->priv->window),
                                                     shell->priv->mpd);
        gtk_widget_show_all (GTK_WIDGET (shell->priv->tray_icon));
#else
        /* initialize tray icon */
        shell->priv->status_icon = ario_status_icon_new (shell->priv->actiongroup,
                                                         shell->priv->ui_manager,
                                                         GTK_WINDOW (shell->priv->window),
                                                         shell->priv->mpd);
#endif
        shell->priv->vpaned = gtk_vpaned_new ();
        shell->priv->status_bar = ario_status_bar_new (shell->priv->mpd);
        shell->priv->statusbar_hidden = ario_conf_get_boolean (PREF_STATUSBAR_HIDDEN, PREF_STATUSBAR_HIDDEN_DEFAULT);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewStatusbar");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->statusbar_hidden);

        shell->priv->upperpart_hidden = ario_conf_get_boolean (PREF_UPPERPART_HIDDEN, PREF_UPPERPART_HIDDEN_DEFAULT);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewUpperPart");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->upperpart_hidden);

        shell->priv->playlist_hidden = ario_conf_get_boolean (PREF_PLAYLIST_HIDDEN, PREF_PLAYLIST_HIDDEN_DEFAULT);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewPlaylist");
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                      !shell->priv->playlist_hidden);

        menubar = gtk_ui_manager_get_widget (shell->priv->ui_manager, "/MenuBar");

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

        ario_shell_sync_window_state (shell);
        gtk_window_set_position (GTK_WINDOW (shell->priv->window), GTK_WIN_POS_CENTER);

        gtk_container_add (GTK_CONTAINER (shell->priv->window), vbox);

        g_signal_connect_object (G_OBJECT (shell->priv->window), "show",
                                 G_CALLBACK (ario_shell_window_show_cb),
                                 shell, 0);

        g_signal_connect_object (G_OBJECT (shell->priv->window), "hide",
                                 G_CALLBACK (ario_shell_window_hide_cb),
                                 shell, 0);

        /* First launch assistant */
        if (!ario_conf_get_boolean (PREF_FIRST_TIME, PREF_FIRST_TIME_DEFAULT)) {
                firstlaunch = ario_firstlaunch_new ();
                g_signal_connect_object (G_OBJECT (firstlaunch), "destroy",
                                         G_CALLBACK (ario_shell_firstlaunch_delete_cb),
                                         shell, 0);
                gtk_widget_show_all (GTK_WIDGET (firstlaunch));
        } else {
                 ario_shell_show (shell);
        }

        ario_shell_sync_statusbar_visibility (shell);
        ario_shell_sync_upperpart_visibility (shell);
        ario_shell_sync_playlist_visibility (shell);
}

void
ario_shell_shutdown (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int width, height;

        if (shell->priv->shown) {
                ario_conf_set_integer (PREF_VPANED_POSITION,
                                       gtk_paned_get_position (GTK_PANED (shell->priv->vpaned)));

                gtk_window_get_size (GTK_WINDOW (shell->priv->window),
                                     &width,
                                     &height);

                if (!ario_conf_get_boolean (PREF_WINDOW_MAXIMIZED, PREF_WINDOW_MAXIMIZED_DEFAULT)) {
                        ario_conf_set_integer (PREF_WINDOW_WIDTH, width);
                        ario_conf_set_integer (PREF_WINDOW_HEIGHT, height);
                }

                ario_playlist_shutdown (ARIO_PLAYLIST (shell->priv->playlist));
                ario_sourcemanager_shutdown (ARIO_SOURCEMANAGER (shell->priv->sourcemanager));

                ario_cover_manager_shutdown (ario_cover_manager_get_instance ());
                ario_lyrics_manager_shutdown (ario_lyrics_manager_get_instance ());
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

        if (ario_conf_get_boolean (PREF_AUTOCONNECT, PREF_AUTOCONNECT_DEFAULT))
                ario_mpd_connect (shell->priv->mpd);

        ario_shell_sync_paned (shell);
        ario_shell_sync_mpd (shell);

        gtk_widget_show_all (shell->priv->window);

        g_signal_connect_object (G_OBJECT (shell->priv->window), "window-state-event",
                                 G_CALLBACK (ario_shell_window_state_cb),
                                 shell, 0);

        shell->priv->shown = TRUE;
}

void
ario_shell_present (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        gtk_window_present (GTK_WINDOW (shell->priv->window));
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
        const gchar *uri = "https://translations.launchpad.net/ario/trunk/";
        ario_util_load_uri (uri);
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
        shell->priv->connected = ario_mpd_is_connected (mpd);

        ario_shell_sync_mpd (shell);
        ario_shell_mpd_song_set_title (shell);
}

static void
ario_shell_cmd_cover_select (GtkAction *action,
                             ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverselect;
        ArioMpdAlbum mpd_album;

        mpd_album.artist = ario_mpd_get_current_artist (shell->priv->mpd);
        mpd_album.album = ario_mpd_get_current_album (shell->priv->mpd);
        mpd_album.path = g_path_get_dirname ((ario_mpd_get_current_song (shell->priv->mpd))->file);

        if (!mpd_album.album)
                mpd_album.album = ARIO_MPD_UNKNOWN;

        if (!mpd_album.artist)
                mpd_album.artist = ARIO_MPD_UNKNOWN;

        coverselect = ario_shell_coverselect_new (&mpd_album);
        gtk_dialog_run (GTK_DIALOG (coverselect));
        gtk_widget_destroy (coverselect);
        g_free (mpd_album.path);
}

static void
ario_shell_cmd_covers (GtkAction *action,
                       ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *coverdownloader;

        coverdownloader = ario_shell_coverdownloader_new (shell->priv->mpd);

        if (coverdownloader) {
                ario_shell_coverdownloader_get_covers (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                       GET_COVERS);
        }
}

static void
ario_shell_cmd_similar_artists (GtkAction *action,
                                ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *similarartists;

        similarartists = ario_shell_similarartists_new (shell->priv->mpd, ARIO_PLAYLIST (shell->priv->playlist));
        if (similarartists)
                gtk_widget_show_all (similarartists);
}

static void
ario_shell_cmd_add_similar (GtkAction *action,
                            ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START

        ario_shell_similarartists_add_similar_to_playlist (shell->priv->mpd,
                                                           ARIO_PLAYLIST (shell->priv->playlist),
                                                           ario_mpd_get_current_artist (shell->priv->mpd));
}

static void
ario_shell_sync_paned (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int pos;

        pos = ario_conf_get_integer (PREF_VPANED_POSITION, PREF_VPANED_POSITION_DEFAULT);
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

        if ((event->type == GDK_WINDOW_STATE)
            && !(event->window_state.new_window_state & GDK_WINDOW_STATE_WITHDRAWN)) {
                ario_conf_set_boolean (PREF_WINDOW_MAXIMIZED,
                                       event->window_state.new_window_state &
                                       GDK_WINDOW_STATE_MAXIMIZED);

                gtk_window_get_size (GTK_WINDOW (shell->priv->window),
                                     &width,
                                     &height);

                ario_conf_set_integer (PREF_WINDOW_WIDTH, width);
                ario_conf_set_integer (PREF_WINDOW_HEIGHT, height);
        }

        return FALSE;
}

static void
ario_shell_sync_window_state (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int width = ario_conf_get_integer (PREF_WINDOW_WIDTH, PREF_WINDOW_WIDTH_DEFAULT); 
        int height = ario_conf_get_integer (PREF_WINDOW_HEIGHT, PREF_WINDOW_HEIGHT_DEFAULT);
        gboolean maximized = ario_conf_get_boolean (PREF_WINDOW_MAXIMIZED, PREF_WINDOW_MAXIMIZED_DEFAULT);
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
        GtkAction *connect_action;
        GtkAction *disconnect_action;
        gboolean is_playing;
        GtkAction *action;

        connect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                      "FileConnect");
        disconnect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                         "FileDisconnect");

        gtk_action_set_visible (connect_action, !shell->priv->connected);
        gtk_action_set_visible (disconnect_action, shell->priv->connected);

        is_playing = ((shell->priv->connected)
                      && ((ario_mpd_get_current_state (shell->priv->mpd) == MPD_STATUS_STATE_PLAY)
                           || (ario_mpd_get_current_state (shell->priv->mpd) == MPD_STATUS_STATE_PAUSE)));

        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ViewLyrics");
        gtk_action_set_sensitive (action, is_playing);

        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ToolCoverSelect");
        gtk_action_set_sensitive (action, is_playing);

        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ToolSimilarArtist");
        gtk_action_set_sensitive (action, is_playing);

        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "ToolAddSimilar");
        gtk_action_set_sensitive (action, is_playing);
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
        ARIO_LOG_FUNCTION_START
        shell->priv->statusbar_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
        ario_conf_set_boolean (PREF_STATUSBAR_HIDDEN, shell->priv->statusbar_hidden);

        ario_shell_sync_statusbar_visibility (shell);
}

static void
ario_shell_view_upperpart_changed_cb (GtkAction *action,
                                      ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        shell->priv->upperpart_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
        ario_conf_set_boolean (PREF_UPPERPART_HIDDEN, shell->priv->upperpart_hidden);

        ario_shell_sync_upperpart_visibility (shell);
}

static void
ario_shell_view_playlist_changed_cb (GtkAction *action,
                                     ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        shell->priv->playlist_hidden = !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
        ario_conf_set_boolean (PREF_PLAYLIST_HIDDEN, shell->priv->playlist_hidden);

        ario_shell_sync_playlist_visibility (shell);
}

static void
ario_shell_sync_statusbar_visibility (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        if (shell->priv->statusbar_hidden)
                gtk_widget_hide (GTK_WIDGET (shell->priv->status_bar));
        else
                gtk_widget_show (GTK_WIDGET (shell->priv->status_bar));
}

static void
ario_shell_sync_upperpart_visibility (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        if (shell->priv->upperpart_hidden)
                gtk_widget_hide (GTK_WIDGET (shell->priv->sourcemanager));
        else
                gtk_widget_show (GTK_WIDGET (shell->priv->sourcemanager));
}

static void
ario_shell_sync_playlist_visibility (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
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
        ARIO_LOG_FUNCTION_START
        gtk_widget_destroy (GTK_WIDGET (window));
        return TRUE;
}

static void
ario_shell_plugins_response_cb (GtkDialog *dialog,
                                int response_id,
                                gpointer data)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
ario_shell_cmd_plugins (GtkAction *action,
                        ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *window;
        GtkWidget *manager;

        window = gtk_dialog_new_with_buttons (_("Configure Plugins"),
                                              GTK_WINDOW (shell->priv->window),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CLOSE,
                                              GTK_RESPONSE_CLOSE,
                                              NULL);
        gtk_container_set_border_width (GTK_CONTAINER (window), 5);
        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (window)->vbox), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (window), FALSE);

        g_signal_connect_object (G_OBJECT (window),
                                 "delete_event",
                                 G_CALLBACK (ario_shell_plugins_window_delete_cb),
                                 NULL, 0);
        g_signal_connect_object (G_OBJECT (window),
                                 "response",
                                 G_CALLBACK (ario_shell_plugins_response_cb),
                                 NULL, 0);

        manager = ario_plugin_manager_new ();
        gtk_widget_show_all (GTK_WIDGET (manager));
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox),
                           manager);

        gtk_window_set_default_size (GTK_WINDOW (window), 300, 350);

        gtk_window_present (GTK_WINDOW (window));
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
