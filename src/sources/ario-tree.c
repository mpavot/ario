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

#include "sources/ario-tree.h"
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>

#include "ario-debug.h"
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "lib/ario-conf.h"
#include "preferences/ario-preferences.h"
#include "shell/ario-shell-coverdownloader.h"
#include "sources/ario-tree-albums.h"
#include "sources/ario-tree-songs.h"
#include "widgets/ario-dnd-tree.h"

/* Minimum number of items to fill tree asynchonously */
#define LIMIT_FOR_IDLE 800

typedef struct ArioTreeAddData
{
        ArioServerCriteria *criteria;
        GSList *tags;
        GSList *tmp;
}ArioTreeAddData;

static GObject *ario_tree_constructor (GType type, guint n_construct_properties,
                                       GObjectConstructParam *construct_properties);
static void ario_tree_finalize (GObject *object);
static void ario_tree_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void ario_tree_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void ario_tree_selection_changed_cb (GtkTreeSelection * selection,
                                            ArioTree *tree);
static void ario_tree_popup_menu_cb (ArioDndTree* dnd_tree,
                                     ArioTree *tree);
static void ario_tree_activate_cb (ArioDndTree* dnd_tree,
                                   ArioTree *tree);
static void ario_tree_selection_drag_foreach (GtkTreeModel *model,
                                              GtkTreePath *path,
                                              GtkTreeIter *iter,
                                              gpointer userdata);
static void ario_tree_drag_data_get_cb (GtkWidget *widget,
                                        GdkDragContext *context,
                                        GtkSelectionData *selection_data,
                                        guint info, guint time, gpointer userdata);
static void ario_tree_drag_begin_cb (GtkWidget *widget,
                                     GdkDragContext *context,
                                     ArioTree *tree);
static void ario_tree_build_tree (ArioTree *tree,
                                  GtkTreeView *treeview);
static void ario_tree_fill_tree (ArioTree *tree);
static GdkPixbuf* ario_tree_get_dnd_pixbuf (ArioTree *tree);
static void ario_tree_get_drag_source (const GtkTargetEntry** targets,
                                       int* n_targets);
static void ario_tree_append_drag_data (ArioTree *tree,
                                        GtkTreeModel *model,
                                        GtkTreeIter *iter,
                                        ArioTreeStringData *data);
static void ario_tree_add_to_playlist (ArioTree *tree,
                                       const PlaylistAction action);
static void ario_tree_add_data_free (ArioTreeAddData *data);

struct ArioTreePrivate
{
        gboolean idle_fill_running;
        ArioTreeAddData *data;

        GtkWidget *popup;
        GtkWidget *album_popup;
        GtkWidget *song_popup;
};

typedef struct
{
        GSList **criterias;
        ArioServerTag tag;
} ArioServerCriteriaData;

/* Object properties */
enum
{
        PROP_0,
        PROP_TAG
};

/* Object signals */
enum
{
        SELECTION_CHANGED,
        MENU_POPUP,
        LAST_SIGNAL
};
static guint ario_tree_signals[LAST_SIGNAL] = { 0 };

/* Tree columns */
enum
{
        VALUE_COLUMN,
        CRITERIA_COLUMN,
        N_COLUMN
};

/* Drag and drop targets */
static const GtkTargetEntry criterias_targets  [] = {
        { "text/criterias-list", 0, 0 },
};

G_DEFINE_TYPE_WITH_CODE (ArioTree, ario_tree, GTK_TYPE_SCROLLED_WINDOW, G_ADD_PRIVATE(ArioTree))

static void
ario_tree_class_init (ArioTreeClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* GObject virtual methods */
        object_class->finalize = ario_tree_finalize;
        object_class->constructor = ario_tree_constructor;
        object_class->set_property = ario_tree_set_property;
        object_class->get_property = ario_tree_get_property;

        /* ArioSource virtual methods */
        klass->build_tree = ario_tree_build_tree;
        klass->fill_tree = ario_tree_fill_tree;
        klass->get_dnd_pixbuf = ario_tree_get_dnd_pixbuf;
        klass->get_drag_source = ario_tree_get_drag_source;
        klass->append_drag_data = ario_tree_append_drag_data;
        klass->add_to_playlist = ario_tree_add_to_playlist;

        /* Object properties */
        g_object_class_install_property (object_class,
                                         PROP_TAG,
                                         g_param_spec_uint ("tag",
                                                            "Tag",
                                                            "Tag ID",
                                                            0, G_MAXUINT,
                                                            0,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        /* Object signals */
        ario_tree_signals[SELECTION_CHANGED] =
                g_signal_new ("selection_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioTreeClass, selection_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_tree_signals[MENU_POPUP] =
                g_signal_new ("menu_popup",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioTreeClass, menu_popup),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
ario_tree_init (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        tree->priv = ario_tree_get_instance_private (tree);
}

static void
ario_tree_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_TREE (object));

        tree = ARIO_TREE (object);

        g_return_if_fail (tree->priv != NULL);

        gtk_list_store_clear (tree->model);
        g_slist_foreach (tree->criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (tree->criterias);

        ario_tree_add_data_free (tree->priv->data);
        tree->priv->data = NULL;

        G_OBJECT_CLASS (ario_tree_parent_class)->finalize (object);
}

static void
ario_tree_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree = ARIO_TREE (object);

        switch (prop_id) {
        case PROP_TAG:
                tree->tag = g_value_get_uint (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_tree_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree = ARIO_TREE (object);

        switch (prop_id) {
        case PROP_TAG:
                g_value_set_uint (value, tree->tag);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
ario_tree_constructor (GType type, guint n_construct_properties,
                       GObjectConstructParam *construct_properties)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree;
        ArioTreeClass *klass;
        GObjectClass *parent_class;
        const GtkTargetEntry* targets;
        int n_targets;
        GtkBuilder *builder;
        GMenuModel *menu;

        /* Call parent constructor */
        klass = ARIO_TREE_CLASS (g_type_class_peek (TYPE_ARIO_TREE));
        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        tree = ARIO_TREE (parent_class->constructor (type, n_construct_properties,
                                                     construct_properties));

        /* Scrolled window property */
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree), GTK_SHADOW_IN);

        /* Get drag and drop targets */
        ARIO_TREE_GET_CLASS (tree)->get_drag_source (&targets,
                                                     &n_targets);

        /* Create drag and drop tree */
        tree->tree = ario_dnd_tree_new (targets, n_targets, FALSE);
        ARIO_TREE_GET_CLASS (tree)->build_tree (tree, GTK_TREE_VIEW (tree->tree));

        /* Set tree model */
        gtk_tree_view_set_model (GTK_TREE_VIEW (tree->tree),
                                 GTK_TREE_MODEL (tree->model));

        /* Get tree selection */
        tree->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->tree));
        gtk_tree_selection_set_mode (tree->selection,
                                     GTK_SELECTION_MULTIPLE);

        gtk_container_add (GTK_CONTAINER (tree), tree->tree);

        /* Create menu */
        builder = gtk_builder_new_from_file (UI_PATH "ario-browser-menu.ui");
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
        tree->priv->popup = gtk_menu_new_from_model (menu);
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "album-menu"));
        tree->priv->album_popup = gtk_menu_new_from_model (menu);
        menu = G_MENU_MODEL (gtk_builder_get_object (builder, "song-menu"));
        tree->priv->song_popup = gtk_menu_new_from_model (menu);
        g_object_unref (builder);
        gtk_menu_attach_to_widget  (GTK_MENU (tree->priv->popup),
                                    GTK_WIDGET (tree),
                                    NULL);
        gtk_menu_attach_to_widget  (GTK_MENU (tree->priv->album_popup),
                                    GTK_WIDGET (tree),
                                    NULL);
        gtk_menu_attach_to_widget  (GTK_MENU (tree->priv->song_popup),
                                    GTK_WIDGET (tree),
                                    NULL);

        /* Connect signals for actions on dnd tree */
        g_signal_connect (tree->selection,
                          "changed",
                          G_CALLBACK (ario_tree_selection_changed_cb),
                          tree);
        g_signal_connect (tree->tree,
                          "drag_data_get",
                          G_CALLBACK (ario_tree_drag_data_get_cb), tree);
        g_signal_connect (tree->tree,
                          "drag_begin",
                          G_CALLBACK (ario_tree_drag_begin_cb), tree);
        g_signal_connect (GTK_TREE_VIEW (tree->tree),
                          "popup",
                          G_CALLBACK (ario_tree_popup_menu_cb), tree);
        g_signal_connect (GTK_TREE_VIEW (tree->tree),
                          "activate",
                          G_CALLBACK (ario_tree_activate_cb), tree);

        return G_OBJECT (tree);
}

static void
ario_tree_get_drag_source (const GtkTargetEntry** targets,
                           int* n_targets)
{
        ARIO_LOG_FUNCTION_START;
        *targets = criterias_targets;
        *n_targets = G_N_ELEMENTS (criterias_targets);
}

static void
ario_tree_build_tree (ArioTree *tree,
                      GtkTreeView *treeview)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (treeview), TRUE);

        /* Create the only column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (ario_server_get_items_names ()[tree->tag],
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        /* Create tree model */
        tree->model = gtk_list_store_new (N_COLUMN,
                                          G_TYPE_STRING,
                                          G_TYPE_POINTER);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->model),
                                              0, GTK_SORT_ASCENDING);
}

GtkWidget *
ario_tree_new (ArioServerTag tag,
               gboolean is_first)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree;
        GType type;

        /* Tree factory: Create a tree of the appropriate type */
        if (tag == ARIO_TAG_ALBUM && !is_first) {
                type = TYPE_ARIO_TREE_ALBUMS;
        } else if (tag == ARIO_TAG_TITLE && !is_first) {
                type = TYPE_ARIO_TREE_SONGS;
        } else {
                type = TYPE_ARIO_TREE;
        }

        tree = ARIO_TREE (g_object_new (type,
                                        "tag", tag,
                                        NULL));

        g_return_val_if_fail (tree->priv != NULL, NULL);
        tree->is_first = is_first;

        return GTK_WIDGET (tree);
}

static void
ario_tree_popup_menu_cb (ArioDndTree* dnd_tree,
                         ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;

        /* Get the most appropriate menu */
        if ((tree->tag == ARIO_TAG_ALBUM)
            && (gtk_tree_selection_count_selected_rows (tree->selection) == 1)) {
                menu = tree->priv->album_popup;
        } else if (tree->tag == ARIO_TAG_TITLE) {
                menu = tree->priv->song_popup;
        } else {
                menu = tree->priv->popup;
        }

        /* Emit popup signal */
        g_signal_emit (G_OBJECT (tree), ario_tree_signals[MENU_POPUP], 0);

        /* Show popup menu */
        gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
ario_tree_activate_cb (ArioDndTree* dnd_tree,
                       ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        /* Add songs to main playlist */
        ario_tree_cmd_add (tree, 
                           ario_conf_get_integer (PREF_DOUBLECLICK_BEHAVIOR,
                                                  PREF_DOUBLECLICK_BEHAVIOR_DEFAULT));
}

static void
ario_tree_append_drag_data (ArioTree *tree,
                            GtkTreeModel *model,
                            GtkTreeIter *iter,
                            ArioTreeStringData *data)
{
        ARIO_LOG_FUNCTION_START;
        gchar *val;
        gchar buf[INTLEN];
        ArioServerCriteria *criteria, *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        /* Get data of selected row */
        gtk_tree_model_get (model, iter,
                            CRITERIA_COLUMN, &criteria,
                            VALUE_COLUMN, &val, -1);

        /* Append number of criteria */
        g_snprintf (buf, INTLEN, "%d", g_slist_length (criteria) + 1);
        g_string_append (data->string, buf);
        g_string_append (data->string, "\n");

        /* Append each criteria with its type */
        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                g_snprintf (buf, INTLEN, "%d", atomic_criteria->tag);
                g_string_append (data->string, buf);
                g_string_append (data->string, "\n");
                g_string_append (data->string, atomic_criteria->value);
                g_string_append (data->string, "\n");
        }

        /* Append current tree type and value */
        g_snprintf (buf, INTLEN, "%d", data->tree->tag);
        g_string_append (data->string, buf);
        g_string_append (data->string, "\n");
        g_string_append (data->string, val);
        g_string_append (data->string, "\n");

        g_free (val);
}

static void
ario_tree_selection_drag_foreach (GtkTreeModel *model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeStringData *data = (ArioTreeStringData *) userdata;

        /* Call virtual method to append drag data */
        ARIO_TREE_GET_CLASS (data->tree)->append_drag_data (data->tree,
                                                            model,
                                                            iter,
                                                            data);
}

static void
ario_tree_drag_data_get_cb (GtkWidget *widget,
                            GdkDragContext *context,
                            GtkSelectionData *selection_data,
                            guint info, guint time, gpointer userdata)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree;
        GString* string = NULL;
        ArioTreeStringData data;

        tree = ARIO_TREE (userdata);

        g_return_if_fail (IS_ARIO_TREE (tree));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        /* Create string for drag data */
        string = g_string_new ("");
        data.string = string;
        data.tree = tree;

        /* Append criteria of each selected row */
        gtk_tree_selection_selected_foreach (tree->selection,
                                             ario_tree_selection_drag_foreach,
                                             &data);

        /* Set drag data */
        gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (const guchar *) string->str,
                                strlen (string->str) * sizeof(guchar));

        g_string_free (string, TRUE);
}

static GdkPixbuf*
ario_tree_get_dnd_pixbuf (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GSList *criterias = NULL;
        GdkPixbuf *pixbuf;

        /* Get drag and drop icon depending on criteria */
        criterias = ario_tree_get_criterias (tree);
        pixbuf = ario_util_get_dnd_pixbuf (criterias);

        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);

        return pixbuf;
}

static
void ario_tree_drag_begin_cb (GtkWidget *widget,
                              GdkDragContext *context,
                              ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GdkPixbuf *pixbuf;

        /* Call virtual method to get drag and drop pixbuf */
        pixbuf = ARIO_TREE_GET_CLASS (tree)->get_dnd_pixbuf (tree);
        if (pixbuf) {
                /* Set icon */
                gtk_drag_source_set_icon_pixbuf (widget, pixbuf);
                g_object_unref (pixbuf);
        }
}

static void
ario_tree_selection_changed_cb (GtkTreeSelection *selection,
                                ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        /* Emit signal for action by ArioBrowser */
        g_signal_emit (G_OBJECT (tree), ario_tree_signals[SELECTION_CHANGED], 0);
}

static void
ario_tree_add_data_free (ArioTreeAddData *data)
{
        ARIO_LOG_FUNCTION_START;
        if (data) {
                g_slist_foreach (data->tags, (GFunc) g_free, NULL);
                g_slist_free (data->tags);
                ario_server_criteria_free (data->criteria);
                g_free (data);
        }
}

static gboolean
ario_tree_add_tags_idle (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        ArioTreeAddData *data = tree->priv->data;

        if (!data) {
                /* Stop iterations */
                tree->priv->idle_fill_running = FALSE;
                return FALSE;
        }

        if (!data->tmp) {
                /* Last element reached, we free all data */
                ario_tree_add_data_free (tree->priv->data);
                tree->priv->data = NULL;

                /* Stop iterations */
                tree->priv->idle_fill_running = FALSE;
                return FALSE;
        }

        /* Append row */
        gtk_list_store_append (tree->model, &iter);
        gtk_list_store_set (tree->model, &iter,
                            VALUE_COLUMN, data->tmp->data,
                            CRITERIA_COLUMN, data->criteria,
                            -1);

        /* Select first item in row when we add it */
        if (data->tmp == data->tags) {
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree->model), &iter))
                        gtk_tree_selection_select_iter (tree->selection, &iter);
        }

        data->tmp = g_slist_next (data->tmp);

        /* Continue iterations */
        tree->priv->idle_fill_running = TRUE;
        return TRUE;
}

void
ario_tree_add_tags (ArioTree *tree,
                    ArioServerCriteria *criteria,
                    GSList *tags)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeAddData *data;
        GSList *tmp;
        GtkTreeIter iter;

        /* Free previous async data */
        ario_tree_add_data_free (tree->priv->data);
        tree->priv->data = NULL;

        if (g_slist_length (tags) > LIMIT_FOR_IDLE) {
                /* Asynchronous fill of tree */
                data = (ArioTreeAddData *) g_malloc0 (sizeof (ArioTreeAddData));
                data->tags = tags;
                data->tmp = tags;

                /* Copy criteria as they could be destroyed before the end */
                data->criteria = ario_server_criteria_copy (criteria);

                /*Replace previous data with new ones */
                tree->priv->data = data;

                /* Launch asynchronous fill if needed */
                if (!tree->priv->idle_fill_running)
                        g_idle_add ((GSourceFunc) ario_tree_add_tags_idle, tree);
        } else {
                /* Synchronous fill of tree */
                for (tmp = tags; tmp; tmp = g_slist_next (tmp)) {
                        /* Append row */
                        gtk_list_store_append (tree->model, &iter);
                        gtk_list_store_set (tree->model, &iter,
                                            VALUE_COLUMN, tmp->data,
                                            CRITERIA_COLUMN, criteria,
                                            -1);
                }
                g_slist_foreach (tags, (GFunc) g_free, NULL);
                g_slist_free (tags);
        }
}

static void
ario_tree_fill_tree (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp, *tags;
        ArioServerCriteria *criteria;

        gtk_list_store_clear (tree->model);
        if (tree->is_first) {
                /* First tree has not criteria */
                tags = ario_server_list_tags (tree->tag, NULL);

                /* Fill tree */
                ario_tree_add_tags (tree, NULL, tags);
        } else {
                for (tmp = tree->criterias; tmp; tmp = g_slist_next (tmp)) {
                        /* Add critreria to filter iems in tree */
                        criteria = tmp->data;
                        tags = ario_server_list_tags (tree->tag, criteria);

                        /* Fill tree */
                        ario_tree_add_tags (tree, criteria, tags);
                }
        }
}

void
ario_tree_fill (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;
        GList *paths = NULL;
        GtkTreePath *path;
        GtkTreeModel *model = GTK_TREE_MODEL (tree->model);

        /* Block signal handler to avoid useless actions */
        g_signal_handlers_block_by_func (G_OBJECT (tree->selection),
                                         G_CALLBACK (ario_tree_selection_changed_cb),
                                         tree);

        /* Remember the selected row in first tree */
        if (tree->is_first)
                paths = gtk_tree_selection_get_selected_rows (tree->selection, &model);

        /* Call virtual method to fill tree */
        ARIO_TREE_GET_CLASS (tree)->fill_tree (tree);

        gtk_tree_selection_unselect_all (tree->selection);
        if (paths) {
                /* First tree : select previously selected row */
                path = paths->data;
                if (path) {
                        gtk_tree_selection_select_path (tree->selection, path);
                }
        } else {
                /* Select first row and move to it */
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree->model), &iter)) {
                        gtk_tree_selection_select_iter (tree->selection, &iter);
                        path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree->model), &iter);
                        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree->tree),
                                                      path,
                                                      NULL,
                                                      TRUE,
                                                      0, 0);
                        gtk_tree_path_free (path);
                }
        }

        g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (paths);

        /* Unblock signal handler */
        g_signal_handlers_unblock_by_func (G_OBJECT (tree->selection),
                                           G_CALLBACK (ario_tree_selection_changed_cb),
                                           tree);

        /* Emit selection change signal */
        g_signal_emit_by_name (G_OBJECT (tree->selection), "changed", 0);
}

void
ario_tree_clear_criterias (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;

        g_slist_foreach (tree->criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (tree->criterias);
        tree->criterias = NULL;
}

void
ario_tree_add_criteria (ArioTree *tree,
                        ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START;
        /* Add a criteria to the list */
        tree->criterias = g_slist_append (tree->criterias, criteria);
}

static void
ario_tree_selection_foreach (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             ArioServerCriteriaData *data)
{
        ARIO_LOG_FUNCTION_START;
        gchar *value;
        ArioServerCriteria *criteria;
        ArioServerCriteria *ret;
        ArioServerAtomicCriteria *atomic_criteria;

        /* Get values */
        gtk_tree_model_get (model, iter,
                            VALUE_COLUMN, &value,
                            CRITERIA_COLUMN, &criteria,
                            -1);

        /* Append copy of criteria to the list */
        ret = ario_server_criteria_copy (criteria);
        atomic_criteria = (ArioServerAtomicCriteria *) g_malloc0 (sizeof (ArioServerAtomicCriteria));
        atomic_criteria->tag = data->tag;
        atomic_criteria->value = value;
        ret = g_slist_append (ret, atomic_criteria);

        *data->criterias = g_slist_append (*data->criterias, ret);
}

GSList *
ario_tree_get_criterias (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        ArioServerCriteriaData data;
        GSList *criterias = NULL;

        data.criterias = &criterias;
        data.tag = tree->tag;

        /* Append criteria for each selected song */
        gtk_tree_selection_selected_foreach (tree->selection,
                                             (GtkTreeSelectionForeachFunc) ario_tree_selection_foreach,
                                             &data);

        return criterias;
}

static void
ario_tree_add_to_playlist (ArioTree *tree,
                           const PlaylistAction action)
{
        ARIO_LOG_FUNCTION_START;
        GSList *criterias = NULL;

        criterias = ario_tree_get_criterias (tree);

        /* Append songs corresponfind to current criteria to main playlist */
        ario_server_playlist_append_criterias (criterias, action, -1);

        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
}

void
ario_tree_cmd_add (ArioTree *tree,
                   const PlaylistAction action)
{
        ARIO_LOG_FUNCTION_START;
        /* Call virtual method for songs addition in main playlist */
        ARIO_TREE_GET_CLASS (tree)->add_to_playlist (tree, action);
}

void
ario_tree_get_cover (ArioTree *tree,
                     const ArioShellCoverdownloaderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        GSList *criterias = NULL, *tmp;
        GtkWidget *coverdownloader;
        GSList *albums = NULL;
        GtkWidget *dialog;
        gint retval;

        /* Ask for confirmation before cover removal */
        if (operation == REMOVE_COVERS) {
                dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("Are you sure that you want to remove all the selected covers?"));

                retval = gtk_dialog_run (GTK_DIALOG(dialog));
                gtk_widget_destroy (dialog);
                if (retval != GTK_RESPONSE_YES)
                        return;
        }

        /* Get criteria of current selection */
        criterias = ario_tree_get_criterias (tree);

        /* Create coverdownloader dialog */
        coverdownloader = ario_shell_coverdownloader_new ();
        if (coverdownloader) {
                /* Get albums corresponding to criteria */
                for (tmp = criterias; tmp; tmp = g_slist_next (tmp)) {
                        albums = g_slist_concat (albums, ario_server_get_albums (tmp->data));
                }
                /* Get covers corresponding to albums */
                ario_shell_coverdownloader_get_covers_from_albums (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                                   albums,
                                                                   operation);

                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
        }
        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
}

typedef struct
{
        const char *tag;
        ArioTree *tree;
} ArioTreeForeachData;

static gboolean
ario_tree_model_foreach (GtkTreeModel *model,
                         GtkTreePath *path,
                         GtkTreeIter *iter,
                         ArioTreeForeachData *data)
{
        gchar *value;

        gtk_tree_model_get (model, iter, VALUE_COLUMN, &value, -1);

        /* Look for row corresponding to current song*/
        if (ario_util_strcmp (data->tag, value) == 0) {
                /* Scroll to the right row */
                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (data->tree->tree),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5, 0);
                gtk_tree_view_set_cursor (GTK_TREE_VIEW (data->tree->tree),
                                          path,
                                          NULL, FALSE);
                g_free (value);
                /* Stop iterations */
                return TRUE;
        }
        g_free (value);

        /* Continue iterations */
        return FALSE;
}

void
ario_tree_goto_playling_song (ArioTree *tree,
                              const ArioServerSong *song)
{
        ARIO_LOG_FUNCTION_START;
        ArioTreeForeachData data;
        data.tree = tree;
        data.tag = ario_server_song_get_tag (song, tree->tag);
        if (!data.tag)
                data.tag = ARIO_SERVER_UNKNOWN;

        /* Loop on each row of the model */
        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->model),
                                (GtkTreeModelForeachFunc) ario_tree_model_foreach,
                                &data);
}
