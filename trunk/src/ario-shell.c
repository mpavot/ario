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
#include "eel-gconf-extensions.h"
#include <glib/gi18n.h>
#include "ario-shell.h"
#include "ario-source.h"
#include "ario-mpd.h"
#include "ario-playlist.h"
#include "ario-header.h"
#include "ario-tray-icon.h"
#include "ario-status-bar.h"
#include "ario-preferences.h"
#include "ario-shell-coverdownloader.h"
#include "ario-debug.h"

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
static void ario_shell_cmd_quit (GtkAction *action,
                                 ArioShell *shell);
static void ario_shell_cmd_save (GtkAction *action,
                                 ArioShell *shell);
static void ario_shell_cmd_connect (GtkAction *action,
                                    ArioShell *shell);
static void ario_shell_cmd_disconnect (GtkAction *action,
                                       ArioShell *shell);
static void ario_shell_cmd_preferences (GtkAction *action,
                                        ArioShell *shell);
#ifdef MULTIPLE_VIEW
static void ario_shell_cmd_radio_view (GtkRadioAction *action,
                                       GtkRadioAction *current,
                                       ArioShell *shell);
#endif  /* MULTIPLE_VIEW */
static void ario_shell_cmd_covers (GtkAction *action,
                                   ArioShell *shell);
static void ario_shell_cmd_about (GtkAction *action,
                                  ArioShell *shell);
static void ario_shell_mpd_state_changed_cb (ArioMpd *mpd,
                                             ArioShell *shell);
static void ario_shell_source_changed_cb (GConfClient *client,
                                          guint cnxn_id,
                                          GConfEntry *entry,
                                          ArioShell *shell);
static gboolean ario_shell_window_state_cb (GtkWidget *widget,
                                            GdkEvent *event,
                                            ArioShell *shell);
static void ario_shell_sync_window_state (ArioShell *shell);
static void ario_shell_sync_paned (ArioShell *shell);
static void ario_shell_sync_source (ArioShell *shell);
static void ario_shell_sync_mpd (ArioShell *shell);
enum
{
        PROP_NONE,
        PROP_UI_MANAGER
};

struct ArioShellPrivate
{
        GtkWidget *window;

        ArioMpd *mpd;
        GtkWidget *header;
        GtkWidget *vpaned;
        GtkWidget *source;
        GtkWidget *playlist;
        GtkWidget *status_bar;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        ArioTrayIcon *tray_icon;

        gboolean connected;
};

static GtkActionEntry ario_shell_actions [] =
{
        { "File", NULL, N_("_File") },
        { "Edit", NULL, N_("_Edit") },
        { "View", NULL, N_("_View") },
        { "Tool", NULL, N_("_Tool") },
        { "Help", NULL, N_("_Help") },

        { "FileSave", GTK_STOCK_SAVE, N_("_Save Playlist"), "<control>S",
          N_("Save Playlist"),
          G_CALLBACK (ario_shell_cmd_save) },
        { "FileConnect", GTK_STOCK_CONNECT, N_("_Connect"), "<control>C",
          N_("Connect"),
          G_CALLBACK (ario_shell_cmd_connect) },
        { "FileDisconnect", GTK_STOCK_DISCONNECT, N_("_Disconnect"), "<control>D",
          N_("Disconnect"),
          G_CALLBACK (ario_shell_cmd_disconnect) },
        { "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
          N_("Quit"),
          G_CALLBACK (ario_shell_cmd_quit) },
        { "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
          N_("Edit music player preferences"),
          G_CALLBACK (ario_shell_cmd_preferences) },
        { "ToolCover", GTK_STOCK_EXECUTE, N_("Download album _covers"), NULL,
          N_("Download covers form amazon"),
          G_CALLBACK (ario_shell_cmd_covers) },
        { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
          N_("Show information about the music player"),
          G_CALLBACK (ario_shell_cmd_about) }
};
static guint ario_shell_n_actions = G_N_ELEMENTS (ario_shell_actions);

static GtkRadioActionEntry ario_shell_radio [] =
{
        { "LibraryView", NULL, N_("_Library"), NULL,
          N_("Library view"),
          ARIO_SOURCE_BROWSER }
#ifdef ENABLE_RADIOS
        ,
        { "RadioView", NULL, N_("_Web Radios"), NULL,
          N_("Web Radios view"),
          ARIO_SOURCE_RADIO }
#endif  /* ENABLE_RADIOS */
#ifdef ENABLE_SEARCH
        ,
        { "SearchView", NULL, N_("_Search"), NULL,
          N_("Search view"),
          ARIO_SOURCE_SEARCH }
#endif  /* ENABLE_SEARCH */
#ifdef ENABLE_STOREDPLAYLISTS
        ,
        { "StoredplaylistsView", NULL, N_("_Playlists"), NULL,
          N_("Playlists view"),
          ARIO_SOURCE_PLAYLISTS }
#endif  /* ENABLE_STOREDPLAYLISTS */
};
static guint ario_shell_n_radio = G_N_ELEMENTS (ario_shell_radio);

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
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager", 
                                                              "GtkUIManager", 
                                                              "GtkUIManager object", 
                                                              GTK_TYPE_UI_MANAGER,
                                                               G_PARAM_READABLE));
}

static void
ario_shell_init (ArioShell *shell) 
{
        ARIO_LOG_FUNCTION_START
        shell->priv = g_new0 (ArioShellPrivate, 1);
        shell->priv->connected = FALSE;
}

static void
ario_shell_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START

        switch (prop_id)
        {
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

        switch (prop_id)
        {
        case PROP_UI_MANAGER:
                g_value_set_object (value, shell->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_shell_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShell *shell = ARIO_SHELL (object);

        gtk_widget_hide (shell->priv->window);

        gtk_widget_destroy (shell->priv->window);
        
        g_free (shell->priv);

        parent_class->finalize (G_OBJECT (shell));
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
        GError *error = NULL;

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
        shell->priv->header = ario_header_new (shell->priv->actiongroup, shell->priv->mpd);
        separator = gtk_hseparator_new ();
        shell->priv->playlist = ario_playlist_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd);
        shell->priv->source = ario_source_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd, ARIO_PLAYLIST (shell->priv->playlist));

        gtk_action_group_add_actions (shell->priv->actiongroup,
                                      ario_shell_actions,
                                      ario_shell_n_actions, shell);
#ifdef MULTIPLE_VIEW
        gtk_action_group_add_radio_actions (shell->priv->actiongroup,
                                            ario_shell_radio,
                                            ario_shell_n_radio,
                                            0, G_CALLBACK (ario_shell_cmd_radio_view),
                                            shell);
#endif  /* MULTIPLE_VIEW */
        gtk_ui_manager_insert_action_group (shell->priv->ui_manager,
                                            shell->priv->actiongroup, 0);
        gtk_ui_manager_add_ui_from_file (shell->priv->ui_manager,
                                         UI_PATH "ario-ui.xml", &error);
        gtk_window_add_accel_group (GTK_WINDOW (shell->priv->window),
                                    gtk_ui_manager_get_accel_group (shell->priv->ui_manager));

        menubar = gtk_ui_manager_get_widget (shell->priv->ui_manager, "/MenuBar");
        shell->priv->vpaned = gtk_vpaned_new ();
        shell->priv->status_bar = ario_status_bar_new (shell->priv->mpd);


        gtk_paned_pack1 (GTK_PANED (shell->priv->vpaned),
                         shell->priv->source,
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

        ario_shell_sync_window_state (shell);
        gtk_widget_show_all (GTK_WIDGET (win));
        ario_shell_sync_paned (shell);
        ario_shell_sync_source (shell);
        ario_shell_sync_mpd (shell);

        /* initialize tray icon */
        shell->priv->tray_icon = ario_tray_icon_new (shell->priv->ui_manager,
                                                     win,
                                                     shell->priv->mpd);
        gtk_widget_show_all (GTK_WIDGET (shell->priv->tray_icon));

        eel_gconf_notification_add (CONF_SOURCE,
                                    (GConfClientNotifyFunc) ario_shell_source_changed_cb,
                                    shell);

        g_signal_connect_object (G_OBJECT (win), "window-state-event",
                                 G_CALLBACK (ario_shell_window_state_cb),
                                 shell, 0);

        g_signal_connect_object (G_OBJECT (shell->priv->mpd),
                                 "state_changed", G_CALLBACK (ario_shell_mpd_state_changed_cb),
                                 shell, 0);

        g_timeout_add (500, (GSourceFunc) ario_mpd_update_status, shell->priv->mpd);
}

void
ario_shell_shutdown (ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        int width, height;

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
        ario_source_shutdown (ARIO_SOURCE (shell->priv->source));
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

        prefs = ario_preferences_new (shell->priv->mpd);

        gtk_window_set_transient_for (GTK_WINDOW (prefs),
                                      GTK_WINDOW (shell->priv->window));

        gtk_widget_show_all (prefs);
}
#ifdef MULTIPLE_VIEW
static void
ario_shell_cmd_radio_view (GtkRadioAction *action,
                           GtkRadioAction *current,
                           ArioShell *shell)
{
        eel_gconf_set_integer (CONF_SOURCE,
                               gtk_radio_action_get_current_value(current));
}
#endif  /* MULTIPLE_VIEW */
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

        gtk_show_about_dialog (GTK_WINDOW (shell->priv->window),
                               "name", "Ario",
                               "version", PACKAGE_VERSION,
                               "copyright", "Copyright \xc2\xa9 2005-2007 Marc Pavot",
                               "comments", "Music player and browser for MPD",
                               "authors", (const char **) authors,
                               NULL);
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
ario_shell_sync_source (ArioShell *shell)
{
#ifdef MULTIPLE_VIEW
        ARIO_LOG_FUNCTION_START
        ArioSourceType source_type;
        GtkAction *action;

        source_type = eel_gconf_get_integer (CONF_SOURCE, 0);
        action = gtk_action_group_get_action (shell->priv->actiongroup,
                                              "LibraryView");
        if (source_type == ARIO_SOURCE_RADIO) {
                gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action),
                                                    ARIO_SOURCE_RADIO);
        } else if (source_type == ARIO_SOURCE_SEARCH) {
                gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action),
                                                    ARIO_SOURCE_SEARCH);
        } else if (source_type == ARIO_SOURCE_PLAYLISTS) {
                gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action),
                                                    ARIO_SOURCE_PLAYLISTS);
        } else {
                gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action),
                                                    ARIO_SOURCE_BROWSER);
        }
#endif  /* MULTIPLE_VIEW */
}

static void
ario_shell_source_changed_cb (GConfClient *client,
                              guint cnxn_id,
                              GConfEntry *entry,
                              ArioShell *shell)
{
        ARIO_LOG_FUNCTION_START
        ario_shell_sync_source (shell);
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
        GtkAction *connect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                                 "FileConnect");
        GtkAction *disconnect_action = gtk_action_group_get_action (shell->priv->actiongroup,
                                                                    "FileDisconnect");

        gtk_action_set_visible (connect_action, !shell->priv->connected);
        gtk_action_set_visible (disconnect_action, shell->priv->connected);
}

