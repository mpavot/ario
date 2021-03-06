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

#include "ario-radio.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include <libxml/parser.h>

#include "ario-util.h"
#include "ario-debug.h"
#include "preferences/ario-preferences.h"
#include "lib/ario-conf.h"
#include "plugins/ario-plugin.h"
#include "servers/ario-server.h"
#include "widgets/ario-dnd-tree.h"

#define XML_ROOT_NAME (const unsigned char *)"ario-radios"

/**
 * ArioInternetRadio represents an internet
 * Radio with its name and URL.
 */
typedef struct ArioInternetRadio
{
        gchar *name;
        gchar *url;
} ArioInternetRadio;

static void ario_radio_class_init (ArioRadioClass *klass);
static void ario_radio_init (ArioRadio *radio);
static void ario_radio_finalize (GObject *object);
static void ario_radio_state_changed_cb (ArioServer *server,
                                         ArioRadio *radio);
static void ario_radio_add_in_playlist (ArioRadio *radio,
                                        PlaylistAction action);
static void ario_radio_cmd_add_radios (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer data);
static void ario_radio_cmd_add_play_radios (GSimpleAction *action,
                                            GVariant *parameter,
                                            gpointer data);
static void ario_radio_cmd_clear_add_play_radios (GSimpleAction *action,
                                                  GVariant *parameter,
                                                  gpointer data);
static void ario_radio_cmd_new_radio (GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data);
static void ario_radio_cmd_delete_radios (GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer data);
static void ario_radio_cmd_radio_properties (GSimpleAction *action,
                                             GVariant *parameter,
                                             gpointer data);
static void ario_radio_radios_selection_drag_foreach (GtkTreeModel *model,
                                                      GtkTreePath *path,
                                                      GtkTreeIter *iter,
                                                      gpointer userdata);

static void ario_radio_drag_data_get_cb (GtkWidget * widget,
                                         GdkDragContext * context,
                                         GtkSelectionData * selection_data,
                                         guint info, guint time, gpointer data);
static void ario_radio_free_internet_radio (ArioInternetRadio *internet_radio);
static void ario_radio_create_xml_file (char *xml_filename);
static char* ario_radio_get_xml_filename (void);
static void ario_radio_fill_radios (ArioRadio *radio);
static void ario_radio_popup_menu_cb (ArioDndTree* tree,
                                      ArioRadio *radio);
static void ario_radio_activate_cb (ArioDndTree* tree,
                                    ArioRadio *radio);
static void ario_radio_adder_changed_cb (GtkWidget *combobox,
                                         ArioRadio *radio);

/* Private attributes */
struct ArioRadioPrivate
{
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;

        gboolean connected;

        xmlDocPtr doc;

        GtkWidget *name_entry;
        GtkWidget *data_label;
        GtkWidget *data_entry;

        GtkWidget *none_popup;
        GtkWidget *single_popup;
        GtkWidget *multiple_popup;
};

/* Actions on radios */
static const GActionEntry ario_radio_actions [] =
{
        { "radio-new", ario_radio_cmd_new_radio },
        { "radio-add-to-pl", ario_radio_cmd_add_radios },
        { "radio-add-play", ario_radio_cmd_add_play_radios },
        { "radio-clear-add-play", ario_radio_cmd_clear_add_play_radios },
        { "radio-delete", ario_radio_cmd_delete_radios },
        { "radio-properties", ario_radio_cmd_radio_properties },
};
static guint ario_radio_n_actions = G_N_ELEMENTS (ario_radio_actions);

/* Object properties */
enum
{
        PROP_0,
};

/* Tree columns */
enum
{
        RADIO_NAME_COLUMN,
        RADIO_URL_COLUMN,
        N_COLUMN
};

/* Drag and drop targets */
static const GtkTargetEntry radios_targets  [] = {
        { "text/radios-list", 0, 0 },
};

typedef struct
{
        gchar *name;
        gchar *data_label;
        gchar *url;
} ArioRadioAdder;

static ArioRadioAdder radio_adders [] = {
        {"URL", "URL :", "%s"},
        {N_("Last.fm: Radio of similar artists"), N_("Artist :"), "lastfm://artist/%s/similarartists"},
        {N_("Last.fm: Radio of group"), N_("Group :"), "lastfm://artist/%s"},
        {N_("Last.fm: Personal radio"), N_("Username :"), "lastfm://user/%s"},
        {N_("Last.fm: Radio of genre"), N_("Genre :"), "lastfm://genre/%s"},
};

G_DEFINE_TYPE_WITH_CODE (ArioRadio, ario_radio, ARIO_TYPE_SOURCE, G_ADD_PRIVATE(ArioRadio))

static gchar *
ario_radio_get_id (ArioSource *source)
{
        return "radios";
}

static gchar *
ario_radio_get_name (ArioSource *source)
{
        return _("Web Radios");
}

static gchar *
ario_radio_get_icon (ArioSource *source)
{
        return "network-workgroup";
}

static void
ario_radio_class_init (ArioRadioClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioSourceClass *source_class = ARIO_SOURCE_CLASS (klass);

        /* GObject virtual methods */
        object_class->finalize = ario_radio_finalize;

        /* ArioSource virtual methods */
        source_class->get_id = ario_radio_get_id;
        source_class->get_name = ario_radio_get_name;
        source_class->get_icon = ario_radio_get_icon;
}

static void
ario_radio_init (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GtkWidget *scrolledwindow_radios;

        radio->priv = ario_radio_get_instance_private (radio);

        /* Radios list */
        scrolledwindow_radios = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_radios);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_radios), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_radios), GTK_SHADOW_IN);

        radio->priv->tree = ario_dnd_tree_new (radios_targets,
                                               G_N_ELEMENTS (radios_targets),
                                               FALSE);

        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (radio->priv->tree), TRUE);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Internet Radios"),
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (radio->priv->tree), column);
        radio->priv->model = gtk_list_store_new (N_COLUMN,
                                                 G_TYPE_STRING,
                                                 G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (radio->priv->model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (radio->priv->tree),
                                 GTK_TREE_MODEL (radio->priv->model));
        radio->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (radio->priv->tree));
        gtk_tree_selection_set_mode (radio->priv->selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_radios), radio->priv->tree);

        g_signal_connect (GTK_TREE_VIEW (radio->priv->tree),
                          "drag_data_get",
                          G_CALLBACK (ario_radio_drag_data_get_cb), radio);
        g_signal_connect (GTK_TREE_VIEW (radio->priv->tree),
                          "popup",
                          G_CALLBACK (ario_radio_popup_menu_cb), radio);
        g_signal_connect (GTK_TREE_VIEW (radio->priv->tree),
                          "activate",
                          G_CALLBACK (ario_radio_activate_cb), radio);

        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (radio), FALSE);
        gtk_box_set_spacing (GTK_BOX (radio), 4);

        gtk_box_pack_start (GTK_BOX (radio), scrolledwindow_radios, TRUE, TRUE, 0);
}

static void
ario_radio_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio;
        guint i;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_RADIO (object));

        radio = ARIO_RADIO (object);

        g_return_if_fail (radio->priv != NULL);
        if (radio->priv->doc)
                xmlFreeDoc (radio->priv->doc);
        radio->priv->doc = NULL;

        for (i = 0; i < ario_radio_n_actions; ++i) {
                g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                                            ario_radio_actions[i].name);
        }

        G_OBJECT_CLASS (ario_radio_parent_class)->finalize (object);
}

GtkWidget *
ario_radio_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio;
        GtkBuilder *builder;
        GMenuModel *menu;
        gchar *file;

        radio = g_object_new (TYPE_ARIO_RADIO,
                              NULL);

        g_return_val_if_fail (radio->priv != NULL, NULL);

        /* Signals to synchronize the radio with server */
        g_signal_connect_object (ario_server_get_instance (),
                                 "state_changed",
                                 G_CALLBACK (ario_radio_state_changed_cb),
                                 radio, 0);
        radio->priv->connected = ario_server_is_connected ();

        /* Create menu */
        file = ario_plugin_find_file ("ario-radios-menu.ui");
        g_return_val_if_fail (file != NULL, NULL);
        builder = gtk_builder_new_from_file (file);
        g_free (file);
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "none-menu"));
        radio->priv->none_popup = gtk_menu_new_from_model (menu);
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "single-menu"));
        radio->priv->single_popup = gtk_menu_new_from_model (menu);
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "multiple-menu"));
        radio->priv->multiple_popup = gtk_menu_new_from_model (menu);
        gtk_menu_attach_to_widget  (GTK_MENU (radio->priv->none_popup),
                                    GTK_WIDGET (radio->priv->tree),
                                    NULL);
        gtk_menu_attach_to_widget  (GTK_MENU (radio->priv->single_popup),
                                    GTK_WIDGET (radio->priv->tree),
                                    NULL);
        gtk_menu_attach_to_widget  (GTK_MENU (radio->priv->multiple_popup),
                                    GTK_WIDGET (radio->priv->tree),
                                    NULL);

        g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                         ario_radio_actions,
                                         ario_radio_n_actions, radio);

        ario_radio_fill_radios (radio);

        return GTK_WIDGET (radio);
}

static void
ario_radio_free_internet_radio (ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        if (internet_radio) {
                g_free (internet_radio->name);
                g_free (internet_radio->url);
                g_free (internet_radio);
        }
}

static void
ario_radio_create_xml_file (char *xml_filename)
{
        gchar *radios_dir;
        gchar *file;

        /* if the file exists, we do nothing */
        if (ario_util_uri_exists (xml_filename))
                return;

        radios_dir = g_build_filename (ario_util_config_dir (), "radios", NULL);

        /* If the radios directory doesn't exist, we create it */
        if (!ario_util_uri_exists (radios_dir))
                ario_util_mkdir (radios_dir);

        file = ario_plugin_find_file ("radios.xml.default");
        if (file) {
                ario_util_copy_file (file, xml_filename);
                g_free (file);
        }
}

static char*
ario_radio_get_xml_filename (void)
{
        static char *xml_filename = NULL;

        if (!xml_filename)
                xml_filename = g_build_filename (ario_util_config_dir (), "radios" , "radios.xml", NULL);

        return xml_filename;
}

// Return TRUE on success
static gboolean
ario_radio_fill_doc (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        char *xml_filename;
        xmlNodePtr cur;

        /* Fill XML doc if not already done */
        if (!radio->priv->doc) {
                xml_filename = ario_radio_get_xml_filename();

                /* If radios.xml file doesn't exist, we create it */
                ario_radio_create_xml_file (xml_filename);

                /* This option is necessary to save a well formated xml file */
                xmlKeepBlanksDefault (0);

                radio->priv->doc = xmlParseFile (xml_filename);
                if (radio->priv->doc == NULL )
                        return FALSE;

                /* We check that the root node has the right name */
                cur = xmlDocGetRootElement (radio->priv->doc);
                if (cur == NULL) {
                        xmlFreeDoc (radio->priv->doc);
                        radio->priv->doc = NULL;
                        return FALSE;
                }
                if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NAME)) {
                        xmlFreeDoc (radio->priv->doc);
                        radio->priv->doc = NULL;
                        return FALSE;
                }
        }
        return TRUE;
}

static GSList*
ario_radio_get_radios (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GSList* radios = NULL;
        ArioInternetRadio *internet_radio;
        xmlNodePtr cur;
        xmlChar *xml_name;
        xmlChar *xml_url;

        /* Fill XML doc if not already done */
        if (!ario_radio_fill_doc (radio))
                return NULL;

        cur = xmlDocGetRootElement (radio->priv->doc);
        for (cur = cur->children; cur; cur = cur->next) {
                /* For each "radio" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"radio")){
                        /* Instanstiate a new radio */
                        internet_radio = (ArioInternetRadio *) g_malloc (sizeof (ArioInternetRadio));

                        /* Set radio name */
                        xml_name = xmlNodeGetContent (cur);
                        internet_radio->name = g_strdup ((char *) xml_name);
                        xmlFree(xml_name);

                        /* Set radio URL */
                        xml_url = xmlGetProp (cur, (const unsigned char *)"url");
                        internet_radio->url = g_strdup ((char *) xml_url);
                        xmlFree(xml_url);

                        /* Append radio to the list */
                        radios = g_slist_append (radios, internet_radio);
                }
        }

        return radios;
}

static void
ario_radio_append_radio (ArioRadio *radio,
                         ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter radio_iter;

        /* Append radio to tree model */
        gtk_list_store_append (radio->priv->model, &radio_iter);
        gtk_list_store_set (radio->priv->model, &radio_iter,
                            RADIO_NAME_COLUMN, internet_radio->name,
                            RADIO_URL_COLUMN, internet_radio->url,
                            -1);
}

static void
ario_radio_fill_radios (ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GSList *radios;
        GSList *tmp;
        GtkTreeIter radio_iter;
        GList* paths;
        GtkTreePath *path;
        GtkTreeModel *models = GTK_TREE_MODEL (radio->priv->model);
        ArioInternetRadio *internet_radio;

        /* Remember which rows are selected to select them again at the end */
        paths = gtk_tree_selection_get_selected_rows (radio->priv->selection, &models);

        /* Empty radio list */
        gtk_list_store_clear (radio->priv->model);

        if (!radio->priv->connected)
                return;

        /* Get all radios from config */
        radios = ario_radio_get_radios (radio);

        /* Append each radio to the list */
        for (tmp = radios; tmp; tmp = g_slist_next (tmp)) {
                internet_radio = (ArioInternetRadio *) tmp->data;
                ario_radio_append_radio (radio, internet_radio);
        }
        g_slist_foreach (radios, (GFunc) ario_radio_free_internet_radio, NULL);
        g_slist_free (radios);

        /* Unselect all rows */
        gtk_tree_selection_unselect_all (radio->priv->selection);

        if (paths) {
                /* Select first previsouly selected radio */
                path = paths->data;
                if (path) {
                        gtk_tree_selection_select_path (radio->priv->selection, path);
                }
        } else {
                /* Select first row */
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (radio->priv->model), &radio_iter))
                        gtk_tree_selection_select_iter (radio->priv->selection, &radio_iter);
        }

        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);
}

static void
ario_radio_state_changed_cb (ArioServer *server,
                             ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;

        if (radio->priv->connected != ario_server_is_connected ()) {
                radio->priv->connected = ario_server_is_connected ();
                /* Fill radio list */
                ario_radio_fill_radios (radio);
        }
}

static void
radios_foreach (GtkTreeModel *model,
                GtkTreePath *path,
                GtkTreeIter *iter,
                gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **radios = (GSList **) userdata;
        gchar *val = NULL;

        /* Append radio url to list */
        gtk_tree_model_get (model, iter, RADIO_URL_COLUMN, &val, -1);
        *radios = g_slist_append (*radios, val);
}

static void
radios_foreach2 (GtkTreeModel *model,
                 GtkTreePath *path,
                 GtkTreeIter *iter,
                 gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GSList **internet_radios = (GSList **) userdata;
        ArioInternetRadio *internet_radio = (ArioInternetRadio *) g_malloc (sizeof (ArioInternetRadio));

        gtk_tree_model_get (model, iter, RADIO_NAME_COLUMN, &internet_radio->name, -1);
        gtk_tree_model_get (model, iter, RADIO_URL_COLUMN, &internet_radio->url, -1);

        /* Append radio to list */
        *internet_radios = g_slist_append (*internet_radios, internet_radio);
}

static void
ario_radio_add_in_playlist (ArioRadio *radio,
                            PlaylistAction action)
{
        ARIO_LOG_FUNCTION_START;
        GSList *radios = NULL;

        /* Get list of radio URL */
        gtk_tree_selection_selected_foreach (radio->priv->selection,
                                             radios_foreach,
                                             &radios);

        /* Append radios to playlist */
        ario_server_playlist_append_songs (radios, action);

        g_slist_foreach (radios, (GFunc) g_free, NULL);
        g_slist_free (radios);
}

static void
ario_radio_cmd_add_radios (GSimpleAction *action,
                           GVariant *parameter,
                           gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio = ARIO_RADIO (data);
        /* Append radios to playlist */
        ario_radio_add_in_playlist (radio, PLAYLIST_ADD);
}

static void
ario_radio_cmd_add_play_radios (GSimpleAction *action,
                                GVariant *parameter,
                                gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio = ARIO_RADIO (data);
        /* Append radios to playlist and play */
        ario_radio_add_in_playlist (radio, PLAYLIST_ADD_PLAY);
}

static void
ario_radio_cmd_clear_add_play_radios (GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio = ARIO_RADIO (data);
        /* Empyt playlist, append radios to playlist and play */
        ario_radio_add_in_playlist (radio, PLAYLIST_REPLACE);
}

static void
ario_radio_popup_menu_cb (ArioDndTree* tree,
                          ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;

        switch (gtk_tree_selection_count_selected_rows (radio->priv->selection)) {
        case 0:
                /* No selected row */
                menu = radio->priv->none_popup;
                break;
        case 1:
                /* One row selected */
                menu = radio->priv->single_popup;
                break;
        default:
                /* Multiple rows */
                menu = radio->priv->multiple_popup;
                break;
        }

        /* Show popup menu */
        gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
ario_radio_activate_cb (ArioDndTree* tree,
                        ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        /* Append radios to playlist */
        ario_radio_add_in_playlist (radio,
                                    ario_conf_get_integer (PREF_DOUBLECLICK_BEHAVIOR,
                                                           PREF_DOUBLECLICK_BEHAVIOR_DEFAULT));
}

static void
ario_radio_radios_selection_drag_foreach (GtkTreeModel *model,
                                          GtkTreePath *path,
                                          GtkTreeIter *iter,
                                          gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        GString *radios = (GString *) userdata;
        g_return_if_fail (radios != NULL);

        gchar* val = NULL;

        /* Append selected radio URL to drag string */
        gtk_tree_model_get (model, iter, RADIO_URL_COLUMN, &val, -1);
        g_string_append (radios, val);
        g_string_append (radios, "\n");
        g_free (val);
}

static void
ario_radio_drag_data_get_cb (GtkWidget * widget,
                             GdkDragContext * context,
                             GtkSelectionData * selection_data,
                             guint info, guint time, gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioRadio *radio;
        GString* radios = NULL;

        radio = ARIO_RADIO (data);

        g_return_if_fail (IS_ARIO_RADIO (radio));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        /* Get string representing all selected radios */
        radios = g_string_new("");
        gtk_tree_selection_selected_foreach (radio->priv->selection,
                                             ario_radio_radios_selection_drag_foreach,
                                             radios);

        /* Set drag data */
        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (const guchar *) radios->str,
                                strlen (radios->str) * sizeof(guchar));

        g_string_free (radios, TRUE);
}

static void
ario_radio_add_new_radio (ArioRadio *radio,
                          ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        xmlNodePtr cur, cur2;

        /* Fill XML doc if not already done */
        if (!ario_radio_fill_doc (radio))
                return;

        cur = xmlDocGetRootElement (radio->priv->doc);

        /* Add a new "radio" entry */
        cur2 = xmlNewChild (cur, NULL, (const xmlChar *)"radio", NULL);
        xmlSetProp (cur2, (const xmlChar *)"url", (const xmlChar *) internet_radio->url);
        xmlNodeAddContent (cur2, (const xmlChar *) internet_radio->name);

        /* Save the xml file */
        xmlSaveFormatFile (ario_radio_get_xml_filename(), radio->priv->doc, TRUE);

        ario_radio_append_radio (radio, internet_radio);
}

static gboolean
ario_radio_launch_dialog (ArioInternetRadio *internet_radio,
                          ArioInternetRadio *new_internet_radio)
{
        GtkWidget *dialog, *error_dialog;
        GtkWidget *grid;
        GtkWidget *label1, *label2;
        GtkWidget *entry1, *entry2;
        gint retval = GTK_RESPONSE_CANCEL;

        /* Create dialog window */
        dialog = gtk_dialog_new_with_buttons (_("Edit a WebRadio"),
                                              NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
					      _("_Cancel"),
                                              GTK_RESPONSE_CANCEL,
					      _("_OK"),
                                              GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_OK);

        /* Create the widgets */
        label1 = gtk_label_new (_("Name :"));
        label2 = gtk_label_new (_("URL :"));

        entry1 = gtk_entry_new ();
        entry2 = gtk_entry_new ();

        /* Fill text boxes if needed */
        if (internet_radio) {
                gtk_entry_set_text (GTK_ENTRY (entry1), internet_radio->name);
                gtk_entry_set_text (GTK_ENTRY (entry2), internet_radio->url);
        }
        gtk_entry_set_activates_default (GTK_ENTRY (entry1), TRUE);
        gtk_entry_set_activates_default (GTK_ENTRY (entry2), TRUE);

        /* Create grid */
        grid = gtk_grid_new ();
        gtk_container_set_border_width (GTK_CONTAINER (grid), 12);

        /* Add widgets to grid */
        gtk_grid_attach (GTK_GRID(grid),
                         label1,
                         0, 0,
                         1, 1);

        gtk_grid_attach (GTK_GRID(grid),
                         label2,
                         0, 1,
                         1, 1);

        gtk_grid_attach (GTK_GRID(grid),
                         entry1,
                         1, 0,
                         1, 1);

        gtk_grid_attach (GTK_GRID(grid),
                         entry2,
                         1, 1,
                         1, 1);

        /* Add grid to dialog */
        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                           grid);
        gtk_widget_show_all (dialog);

        /* Run dialog */
        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        if (retval != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                return FALSE;
        }

        /* Get text boxes content */
        new_internet_radio->name = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry1)));
        new_internet_radio->url = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry2)));

        /* Detect bad values */
        if (!new_internet_radio->name
            || !new_internet_radio->url
            || !strcmp(new_internet_radio->name, "")
            || !strcmp(new_internet_radio->url, "")) {
                error_dialog = gtk_message_dialog_new(NULL,
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      _("Bad parameters. You must specify a name and a URL for the radio."));
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
                gtk_widget_destroy(dialog);
                return FALSE;
        }

        gtk_widget_destroy (dialog);

        return TRUE;
}

static gboolean
ario_radio_launch_creation_dialog (ArioRadio * radio,
                                   ArioInternetRadio *new_internet_radio)
{
        GtkWidget *dialog, *error_dialog;
        GtkWidget *grid;
        gint retval = GTK_RESPONSE_CANCEL;

        GtkWidget *vbox;
        GtkTreeIter iter;
        GtkWidget *combo_box;
        GtkCellRenderer *renderer;
        guint i;
        GtkWidget *label1;

        GtkListStore *list_store;

        /* Create dialog window */
        dialog = gtk_dialog_new_with_buttons (_("Add a WebRadio"),
                                              NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              _("_Cancel"),
                                              GTK_RESPONSE_CANCEL,
                                              _("_OK"),
                                              GTK_RESPONSE_OK,
                                              NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_OK);

        /* Main vbox */
        vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

        /* Create model */
        list_store = gtk_list_store_new (1, G_TYPE_STRING);

        /* Add tags to model */
        for (i = 0; i < G_N_ELEMENTS (radio_adders); ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (radio_adders[i].name),
                                    -1);
        }
        combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list_store));

        /* Create renderer */
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_box));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
                                        "text", 0, NULL);

        gtk_box_pack_start (GTK_BOX (vbox),
                            combo_box,
                            FALSE, FALSE, 0);

        /* Create the widgets */
        label1 = gtk_label_new (_("Name :"));
        radio->priv->data_label = gtk_label_new (_("URL :"));

        radio->priv->name_entry = gtk_entry_new ();
        radio->priv->data_entry = gtk_entry_new ();

        gtk_entry_set_activates_default (GTK_ENTRY (radio->priv->name_entry), TRUE);
        gtk_entry_set_activates_default (GTK_ENTRY (radio->priv->data_entry), TRUE);

        /* Create grid */
        grid = gtk_grid_new ();
        gtk_container_set_border_width (GTK_CONTAINER (grid), 12);

        /* Add widgets to grid */
        gtk_grid_attach (GTK_GRID (grid),
                         label1,
                         0, 0,
                         1, 1);
        gtk_grid_attach (GTK_GRID (grid),
                         radio->priv->name_entry,
                         1, 0,
                         1, 1);
        gtk_grid_attach (GTK_GRID (grid),
                         radio->priv->data_label,
                         0, 1,
                         1, 1);
        gtk_grid_attach (GTK_GRID (grid),
                         radio->priv->data_entry,
                         1, 1,
                         1, 1);

        /* Add grid to vbox */
        gtk_box_pack_start (GTK_BOX (vbox),
                            grid,
                            FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                           vbox);

        /* Select first item */
        g_signal_connect (combo_box,
                          "changed",
                          G_CALLBACK (ario_radio_adder_changed_cb), radio);
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter);
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box),
                                       &iter);

        /* Run dialog */
        gtk_widget_show_all (dialog);
        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        if (retval != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                return FALSE;
        }

        /* Get text boxes content */
        new_internet_radio->name = g_strdup (gtk_entry_get_text (GTK_ENTRY(radio->priv->name_entry)));
        new_internet_radio->url = g_strdup_printf (radio_adders[gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box))].url,
                                                   gtk_entry_get_text (GTK_ENTRY(radio->priv->data_entry)));

        /* Detect bad values */
        if (!new_internet_radio->name
            || !new_internet_radio->url
            || !strcmp(new_internet_radio->name, "")
            || !strcmp(new_internet_radio->url, "")) {
                error_dialog = gtk_message_dialog_new(NULL,
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      _("Bad parameters. You must specify a name and a URL for the radio."));
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
                gtk_widget_destroy(dialog);
                return FALSE;
        }

        gtk_widget_destroy (dialog);

        return TRUE;
}

static void
ario_radio_cmd_new_radio (GSimpleAction *action,
                          GVariant *parameter,
                          gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        ArioInternetRadio new_internet_radio;
        ArioRadio *radio = ARIO_RADIO (data);

        new_internet_radio.name = NULL;
        new_internet_radio.url = NULL;

        /* Laucn dialog */
        if (ario_radio_launch_creation_dialog (radio,
                                               &new_internet_radio)) {
                /* Add radio to config */
                ario_radio_add_new_radio(radio,
                                         &new_internet_radio);
        }
        g_free (new_internet_radio.name);
        g_free (new_internet_radio.url);
}

static void
ario_radio_delete_radio (ArioInternetRadio *internet_radio,
                         ArioRadio *radio)
{
        ARIO_LOG_FUNCTION_START;
        xmlNodePtr cur, next_cur;
        xmlChar *xml_name;
        xmlChar *xml_url;

        /* Fill XML doc if not already done */
        if (!ario_radio_fill_doc (radio))
                return;

        cur = xmlDocGetRootElement (radio->priv->doc);
        for (cur = cur->children; cur; cur = next_cur) {
                next_cur = cur->next;
                /* For each "radio" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"radio")){
                        xml_name = xmlNodeGetContent (cur);
                        xml_url = xmlGetProp (cur, (const unsigned char *)"url");
                        if (!xmlStrcmp (xml_name, (const xmlChar *)internet_radio->name)
                            && !xmlStrcmp (xml_url, (const xmlChar *)internet_radio->url)) {
                                /* Radio found: remove it from XML file */
                                xmlUnlinkNode(cur);
                                xmlFreeNode(cur);
                        }

                        xmlFree(xml_name);
                        xmlFree(xml_url);
                }
        }

        /* We save the xml file */
        xmlSaveFormatFile (ario_radio_get_xml_filename(), radio->priv->doc, TRUE);

        /* Update radios list */
        ario_radio_fill_radios (radio);
}

static void
ario_radio_cmd_delete_radios (GSimpleAction *action,
                              GVariant *parameter,
                              gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GSList *internet_radios = NULL;
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;
        ArioRadio *radio = ARIO_RADIO (data);

        /* Create confirmation dialog window */
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure you want to delete all the selected radios?"));

        /* Launch dialog */
        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        /* Get list of selected radios */
        gtk_tree_selection_selected_foreach (radio->priv->selection,
                                             radios_foreach2,
                                             &internet_radios);

        /* Delete each selected radio */
        g_slist_foreach (internet_radios, (GFunc) ario_radio_delete_radio, radio);
        g_slist_foreach (internet_radios, (GFunc) ario_radio_free_internet_radio, NULL);
        g_slist_free (internet_radios);
}

static void
ario_radio_modify_radio (ArioRadio *radio,
                         ArioInternetRadio *old_internet_radio,
                         ArioInternetRadio *new_internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        xmlNodePtr cur, next_cur;
        xmlChar *xml_name;
        xmlChar *xml_url;
        xmlChar *new_xml_name;

        /* Fill XML doc if not already done */
        if (!ario_radio_fill_doc (radio))
                return;

        cur = xmlDocGetRootElement (radio->priv->doc);
        for (cur = cur->children; cur; cur = next_cur) {
                next_cur = cur->next;
                /* For each "radio" entry */
                if (!xmlStrcmp (cur->name, (const xmlChar *)"radio")){
                        xml_name = xmlNodeGetContent (cur);
                        xml_url = xmlGetProp (cur, (const unsigned char *)"url");
                        if (!xmlStrcmp (xml_name, (const xmlChar *)old_internet_radio->name)
                            && !xmlStrcmp (xml_url, (const xmlChar *)old_internet_radio->url)) {
                                /* Radio found: Modify name and URL */
                                xmlSetProp (cur, (const xmlChar *)"url", (const xmlChar *) new_internet_radio->url);
                                new_xml_name = xmlEncodeEntitiesReentrant (radio->priv->doc, (const xmlChar *) new_internet_radio->name);
                                xmlNodeSetContent (cur, new_xml_name);
                                xmlFree(new_xml_name);
                        }

                        xmlFree(xml_name);
                        xmlFree(xml_url);
                }
        }

        /* We save the xml file */
        xmlSaveFormatFile (ario_radio_get_xml_filename(), radio->priv->doc, TRUE);

        /* Update radios list */
        ario_radio_fill_radios (radio);
}

static void
ario_radio_edit_radio_properties (ArioRadio *radio,
                                  ArioInternetRadio *internet_radio)
{
        ARIO_LOG_FUNCTION_START;
        ArioInternetRadio new_internet_radio;

        new_internet_radio.name = NULL;
        new_internet_radio.url = NULL;

        /* Launch dialog window */
        if (ario_radio_launch_dialog (internet_radio,
                                      &new_internet_radio)) {
                /* Update radio config if needed */
                ario_radio_modify_radio (radio,
                                         internet_radio,
                                         &new_internet_radio);
        }
        g_free (new_internet_radio.name);
        g_free (new_internet_radio.url);
}

static void
ario_radio_cmd_radio_properties (GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer data)
{
        ARIO_LOG_FUNCTION_START;
        GList* paths;
        GtkTreeIter iter;
        ArioInternetRadio *internet_radio;
        GtkTreePath *path;
        ArioRadio *radio = ARIO_RADIO (data);
        GtkTreeModel *tree_model = GTK_TREE_MODEL (radio->priv->model);

        /* Get path to first selected radio */
        paths = gtk_tree_selection_get_selected_rows (radio->priv->selection,
                                                      &tree_model);
        if (!paths)
                return;
        path = g_list_first(paths)->data;

        /* Get iter of first selected radio */
        gtk_tree_model_get_iter (tree_model,
                                 &iter,
                                 path);
        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        /* Create ArioInternetRadio with values of selected row */
        internet_radio = (ArioInternetRadio *) g_malloc (sizeof (ArioInternetRadio));;
        gtk_tree_model_get (tree_model, &iter, RADIO_NAME_COLUMN, &internet_radio->name, -1);
        gtk_tree_model_get (tree_model, &iter, RADIO_URL_COLUMN, &internet_radio->url, -1);

        /* Edit radio properties */
        ario_radio_edit_radio_properties (radio, internet_radio);

        ario_radio_free_internet_radio (internet_radio);
}

static void
ario_radio_adder_changed_cb (GtkWidget *combo_box,
                             ArioRadio *radio)
{
        gtk_label_set_text (GTK_LABEL (radio->priv->data_label),
                            radio_adders[gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box))].data_label);
}
