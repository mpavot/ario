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
#include "sources/ario-tree-albums.h"
#include "sources/ario-tree-songs.h"
#include <gtk/gtk.h>
#include <string.h>
#include "lib/ario-conf.h"
#include <glib/gi18n.h>
#include "ario-util.h"
#include "covers/ario-cover.h"
#include "shell/ario-shell-coverdownloader.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

#define DRAG_THRESHOLD 1
#define LIMIT_FOR_IDLE 800

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
static void ario_tree_popup_menu (ArioTree *tree);
static gboolean ario_tree_button_press_cb (GtkWidget *widget,
                                           GdkEventButton *event,
                                           ArioTree *tree);
static gboolean ario_tree_button_release_cb (GtkWidget *widget,
                                             GdkEventButton *event,
                                             ArioTree *tree);
static gboolean ario_tree_motion_notify (GtkWidget *widget,
                                         GdkEventMotion *event,
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
static void ario_tree_set_drag_source (ArioTree *tree);
static void ario_tree_append_drag_data (ArioTree *tree,
                                        GtkTreeModel *model,
                                        GtkTreeIter *iter,
                                        ArioTreeStringData *data);
static void ario_tree_add_to_playlist (ArioTree *tree,
                                       const gboolean play);

struct ArioTreePrivate
{
        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        GtkUIManager *ui_manager;
};

typedef struct
{
        GSList **criterias;
        ArioServerTag tag;
} ArioServerCriteriaData;

enum
{
        PROP_0,
        PROP_UI_MANAGER
};

enum
{
        SELECTION_CHANGED,
        MENU_POPUP,
        LAST_SIGNAL
};
static guint ario_tree_signals[LAST_SIGNAL] = { 0 };

enum
{
        VALUE_COLUMN,
        CRITERIA_COLUMN,
        N_COLUMN
};

static const GtkTargetEntry criterias_targets  [] = {
        { "text/criterias-list", 0, 0 },
};

#define ARIO_TREE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_TREE, ArioTreePrivate))
G_DEFINE_TYPE (ArioTree, ario_tree, GTK_TYPE_SCROLLED_WINDOW)

static void
ario_tree_class_init (ArioTreeClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_tree_finalize;
        object_class->constructor = ario_tree_constructor;

        object_class->set_property = ario_tree_set_property;
        object_class->get_property = ario_tree_get_property;

        klass->build_tree = ario_tree_build_tree;
        klass->fill_tree = ario_tree_fill_tree;
        klass->get_dnd_pixbuf = ario_tree_get_dnd_pixbuf;
        klass->set_drag_source = ario_tree_set_drag_source;
        klass->append_drag_data = ario_tree_append_drag_data;
        klass->add_to_playlist = ario_tree_add_to_playlist;

        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
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

        g_type_class_add_private (klass, sizeof (ArioTreePrivate));
}

static void
ario_tree_init (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        tree->priv = ARIO_TREE_GET_PRIVATE (tree);
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
        case PROP_UI_MANAGER:
                tree->priv->ui_manager = g_value_get_object (value);
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
        case PROP_UI_MANAGER:
                g_value_set_object (value, tree->priv->ui_manager);
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

        klass = ARIO_TREE_CLASS (g_type_class_peek (TYPE_ARIO_TREE));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        tree = ARIO_TREE (parent_class->constructor (type, n_construct_properties,
                                                     construct_properties));

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree), GTK_SHADOW_IN);

        tree->tree = gtk_tree_view_new ();

        ARIO_TREE_GET_CLASS (tree)->build_tree (tree, GTK_TREE_VIEW (tree->tree));

        gtk_tree_view_set_model (GTK_TREE_VIEW (tree->tree),
                                 GTK_TREE_MODEL (tree->model));
        tree->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->tree));
        gtk_tree_selection_set_mode (tree->selection,
                                     GTK_SELECTION_MULTIPLE);
        g_signal_connect (tree->selection,
                          "changed",
                          G_CALLBACK (ario_tree_selection_changed_cb),
                          tree);

        gtk_container_add (GTK_CONTAINER (tree), tree->tree);

        ARIO_TREE_GET_CLASS (tree)->set_drag_source (tree);

        g_signal_connect (tree->tree,
                          "drag_data_get",
                          G_CALLBACK (ario_tree_drag_data_get_cb), tree);

        g_signal_connect (tree->tree,
                          "drag_begin",
                          G_CALLBACK (ario_tree_drag_begin_cb), tree);

        g_signal_connect (tree->tree,
                          "button_press_event",
                          G_CALLBACK (ario_tree_button_press_cb),
                          tree);
        g_signal_connect (tree->tree,
                          "button_release_event",
                          G_CALLBACK (ario_tree_button_release_cb),
                          tree);
        g_signal_connect (tree->tree,
                          "motion_notify_event",
                          G_CALLBACK (ario_tree_motion_notify),
                          tree);

        return G_OBJECT (tree);
}

static void
ario_tree_set_drag_source (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        gtk_drag_source_set (tree->tree,
                             GDK_BUTTON1_MASK,
                             criterias_targets,
                             G_N_ELEMENTS (criterias_targets),
                             GDK_ACTION_COPY);
}

static void
ario_tree_build_tree (ArioTree *tree,
                      GtkTreeView *treeview)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (treeview), TRUE);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (ario_server_get_items_names ()[tree->tag],
                                                           renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        tree->model = gtk_list_store_new (N_COLUMN,
                                          G_TYPE_STRING,
                                          G_TYPE_POINTER);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree->model),
                                              0, GTK_SORT_ASCENDING);
}

GtkWidget *
ario_tree_new (GtkUIManager *mgr,
               ArioServerTag tag,
               gboolean is_first)
{
        ARIO_LOG_FUNCTION_START;
        ArioTree *tree;
        GType type;

        if (tag == MPD_TAG_ITEM_ALBUM && !is_first) {
                type = TYPE_ARIO_TREE_ALBUMS;
        } else if (tag == MPD_TAG_ITEM_TITLE && !is_first) {
                type = TYPE_ARIO_TREE_SONGS;
        } else {
                type = TYPE_ARIO_TREE;
        }

        tree = ARIO_TREE (g_object_new (type,
                                        "ui-manager", mgr,
                                        NULL));

        g_return_val_if_fail (tree->priv != NULL, NULL);
        tree->tag = tag;
        tree->is_first = is_first;

        return GTK_WIDGET (tree);
}

static void
ario_tree_popup_menu (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *menu;

        if ((tree->tag == MPD_TAG_ITEM_ALBUM)
            && (gtk_tree_selection_count_selected_rows (tree->selection) == 1)) {
                menu = gtk_ui_manager_get_widget (tree->priv->ui_manager, "/BrowserAlbumsPopupSingle");
        } else if (tree->tag == MPD_TAG_ITEM_TITLE) {
                menu = gtk_ui_manager_get_widget (tree->priv->ui_manager, "/BrowserSongsPopup");
        } else {
                menu = gtk_ui_manager_get_widget (tree->priv->ui_manager, "/BrowserPopup");
        }
        g_signal_emit (G_OBJECT (tree), ario_tree_signals[MENU_POPUP], 0);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3,
                        gtk_get_current_event_time ());
}

static gboolean
ario_tree_button_press_cb (GtkWidget *widget,
                           GdkEventButton *event,
                           ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y;
        gboolean selected;

        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (tree->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                ario_tree_cmd_add (tree, FALSE);
                return FALSE;
        }

        if (event->button == 1) {
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                tree->priv->drag_start_x = x;
                tree->priv->drag_start_y = y;
                tree->priv->pressed = TRUE;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        selected = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), path);
                        gtk_tree_path_free (path);

                        return selected;
                }

                return TRUE;
        }

        if (event->button == 3) {
                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        ario_tree_popup_menu (tree);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_tree_button_release_cb (GtkWidget *widget,
                             GdkEventButton *event,
                             ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        if (!tree->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

                        if (gtk_tree_selection_path_is_selected (selection, path)
                            && (gtk_tree_selection_count_selected_rows (selection) != 1)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        gtk_tree_path_free (path);
                }
        }

        tree->priv->dragging = FALSE;
        tree->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_tree_motion_notify (GtkWidget *widget,
                         GdkEventMotion *event,
                         ArioTree *tree)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START;
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((tree->priv->dragging) || !(tree->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - tree->priv->drag_start_x;
        dy = y - tree->priv->drag_start_y;

        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                tree->priv->dragging = TRUE;

        return FALSE;
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

        gtk_tree_model_get (model, iter, CRITERIA_COLUMN, &criteria, VALUE_COLUMN, &val, -1);
        g_snprintf (buf, INTLEN, "%d", g_slist_length (criteria) + 1);
        g_string_append (data->string, buf);
        g_string_append (data->string, "\n");

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                g_snprintf (buf, INTLEN, "%d", atomic_criteria->tag);
                g_string_append (data->string, buf);
                g_string_append (data->string, "\n");
                g_string_append (data->string, atomic_criteria->value);
                g_string_append (data->string, "\n");
        }

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

        string = g_string_new ("");
        data.string = string;
        data.tree = tree;
        gtk_tree_selection_selected_foreach (tree->selection,
                                             ario_tree_selection_drag_foreach,
                                             &data);
        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) string->str,
                                strlen (string->str) * sizeof(guchar));

        g_string_free (string, TRUE);
}

static GdkPixbuf*
ario_tree_get_dnd_pixbuf (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GSList *criterias = NULL;
        GdkPixbuf *pixbuf;

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

        pixbuf = ARIO_TREE_GET_CLASS (tree)->get_dnd_pixbuf (tree);
        if (pixbuf) {
                gtk_drag_source_set_icon_pixbuf (widget, pixbuf);
                g_object_unref (pixbuf);
        } else {
                gtk_drag_source_set_icon_stock (widget, GTK_STOCK_DND);
        }
}

static void
ario_tree_selection_changed_cb (GtkTreeSelection *selection,
                                ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        g_signal_emit (G_OBJECT (tree), ario_tree_signals[SELECTION_CHANGED], 0);
}

typedef struct ArioTreeAddData
{
        ArioTree *tree;
        ArioServerCriteria *criteria;
        GSList *tags;
        GSList *tmp;
}ArioTreeAddData;

static gboolean
ario_tree_add_tags_idle (ArioTreeAddData *data)
{
        ARIO_LOG_FUNCTION_START;
        GtkTreeIter iter;

        if (!data->tmp) {
                g_slist_foreach (data->tags, (GFunc) g_free, NULL);
                g_slist_free (data->tags);
                ario_server_criteria_free (data->criteria);
                g_free (data);
                return FALSE;
        }

        gtk_list_store_append (data->tree->model, &iter);
        gtk_list_store_set (data->tree->model, &iter,
                            VALUE_COLUMN, data->tmp->data,
                            CRITERIA_COLUMN, data->criteria,
                            -1);

        if (data->tmp == data->tags) {
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (data->tree->model), &iter))
                        gtk_tree_selection_select_iter (data->tree->selection, &iter);
        }

        data->tmp = g_slist_next (data->tmp);

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

        if (tree->is_first && g_slist_length (tags) > LIMIT_FOR_IDLE) {
                /* Asynchronous */
                data = (ArioTreeAddData *) g_malloc0 (sizeof (ArioTreeAddData));
                data->tree = tree;
                data->criteria = ario_server_criteria_copy (criteria);
                data->tags = tags;
                data->tmp = tags;

                g_idle_add ((GSourceFunc) ario_tree_add_tags_idle, data);
        } else {
                /* Synchronous */
                for (tmp = tags; tmp; tmp = g_slist_next (tmp)) {
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
                tags = ario_server_list_tags (tree->tag, NULL);
                ario_tree_add_tags (tree, NULL, tags);
        } else {
                for (tmp = tree->criterias; tmp; tmp = g_slist_next (tmp)) {
                        criteria = tmp->data;
                        tags = ario_server_list_tags (tree->tag, criteria);
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

        g_signal_handlers_block_by_func (G_OBJECT (tree->selection),
                                         G_CALLBACK (ario_tree_selection_changed_cb),
                                         tree);

        if (tree->is_first)
                paths = gtk_tree_selection_get_selected_rows (tree->selection, &model);

        ARIO_TREE_GET_CLASS (tree)->fill_tree (tree);

        gtk_tree_selection_unselect_all (tree->selection);
        if (paths) {
                path = paths->data;
                if (path) {
                        gtk_tree_selection_select_path (tree->selection, path);
                }
        } else {
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

        g_signal_handlers_unblock_by_func (G_OBJECT (tree->selection),
                                           G_CALLBACK (ario_tree_selection_changed_cb),
                                           tree);

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

        gtk_tree_model_get (model, iter,
                            VALUE_COLUMN, &value,
                            CRITERIA_COLUMN, &criteria,
                            -1);

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

        gtk_tree_selection_selected_foreach (tree->selection,
                                             (GtkTreeSelectionForeachFunc) ario_tree_selection_foreach,
                                             &data);

        return criterias;
}

static void
ario_tree_add_to_playlist (ArioTree *tree,
                           const gboolean play)
{
        ARIO_LOG_FUNCTION_START;
        GSList *criterias = NULL;

        criterias = ario_tree_get_criterias (tree);

        ario_server_playlist_append_criterias (criterias, play, -1);

        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
}

void
ario_tree_cmd_add (ArioTree *tree,
                   const gboolean play)
{
        ARIO_LOG_FUNCTION_START;
        ARIO_TREE_GET_CLASS (tree)->add_to_playlist (tree, play);
}

static void
get_cover (ArioTree *tree,
           const guint operation)
{
        ARIO_LOG_FUNCTION_START;
        GSList *criterias = NULL, *tmp;
        GtkWidget *coverdownloader;
        GSList *albums = NULL;

        criterias = ario_tree_get_criterias (tree);

        coverdownloader = ario_shell_coverdownloader_new ();
        if (coverdownloader) {
                for (tmp = criterias; tmp; tmp = g_slist_next (tmp)) {
                        albums = g_slist_concat (albums, ario_server_get_albums (tmp->data));
                }
                ario_shell_coverdownloader_get_covers_from_albums (ARIO_SHELL_COVERDOWNLOADER (coverdownloader),
                                                                   albums,
                                                                   operation);

                g_slist_foreach (albums, (GFunc) ario_server_free_album, NULL);
                g_slist_free (albums);
        }
        g_slist_foreach (criterias, (GFunc) ario_server_criteria_free, NULL);
        g_slist_free (criterias);
}

void
ario_tree_cmd_get_cover (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        get_cover (tree, GET_COVERS);
}

void
ario_tree_cmd_remove_cover (ArioTree *tree)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Are you sure that you want to remove all the selected covers?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        get_cover (tree, REMOVE_COVERS);
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

        if (ario_util_strcmp (data->tag, value) == 0) {
                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (data->tree->tree),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5, 0);
                gtk_tree_view_set_cursor (GTK_TREE_VIEW (data->tree->tree),
                                          path,
                                          NULL, FALSE);
                g_free (value);
                return TRUE;
        }
        g_free (value);

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

        gtk_tree_model_foreach (GTK_TREE_MODEL (tree->model),
                                (GtkTreeModelForeachFunc) ario_tree_model_foreach,
                                &data);
}
