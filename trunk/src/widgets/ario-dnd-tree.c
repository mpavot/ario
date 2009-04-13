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

#include "widgets/ario-dnd-tree.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include "ario-util.h"
#include "ario-debug.h"

#define DRAG_THRESHOLD 1

static gboolean ario_dnd_tree_button_press_cb (GtkWidget *widget,
                                               GdkEventButton *event,
                                               ArioDndTree *dnd_tree);
static gboolean ario_dnd_tree_button_release_cb (GtkWidget *widget,
                                                 GdkEventButton *event,
                                                 ArioDndTree *dnd_tree);
static gboolean ario_dnd_tree_motion_notify (GtkWidget *widget,
                                             GdkEventMotion *event,
                                             ArioDndTree *dnd_tree);

enum
{
        POPUP,
        ACTIVATE,
        LAST_SIGNAL
};
static guint ario_dnd_tree_signals[LAST_SIGNAL] = { 0 };

struct ArioDndTreePrivate
{
        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        gboolean browse_mode;
};

#define ARIO_DND_TREE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_DND_TREE, ArioDndTreePrivate))
G_DEFINE_TYPE (ArioDndTree, ario_dnd_tree, GTK_TYPE_TREE_VIEW)

static void
ario_dnd_tree_class_init (ArioDndTreeClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        /* Signals */
        ario_dnd_tree_signals[POPUP] =
                g_signal_new ("popup",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioDndTreeClass, popup),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_dnd_tree_signals[ACTIVATE] =
                g_signal_new ("activate",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioDndTreeClass, activate),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioDndTreePrivate));
}

static void
ario_dnd_tree_init (ArioDndTree *dnd_tree)
{
        ARIO_LOG_FUNCTION_START;
        dnd_tree->priv = ARIO_DND_TREE_GET_PRIVATE (dnd_tree);
}

GtkWidget *
ario_dnd_tree_new (const GtkTargetEntry* targets,
                   const gint n_targets,
                   const gboolean browse_mode)
{
        ARIO_LOG_FUNCTION_START;
        ArioDndTree *dnd_tree;

        dnd_tree = g_object_new (TYPE_ARIO_DND_TREE, NULL);
        g_return_val_if_fail (dnd_tree->priv, NULL);

        /* Set the treeview as drag and drop source */
        gtk_drag_source_set (GTK_WIDGET (dnd_tree),
                             GDK_BUTTON1_MASK,
                             targets,
                             n_targets,
                             GDK_ACTION_MOVE | GDK_ACTION_COPY);

        dnd_tree->priv->browse_mode = browse_mode;

        /* Signals for drag & drop */
        g_signal_connect (dnd_tree,
                          "button_press_event",
                          G_CALLBACK (ario_dnd_tree_button_press_cb),
                          dnd_tree);
        g_signal_connect (dnd_tree,
                          "button_release_event",
                          G_CALLBACK (ario_dnd_tree_button_release_cb),
                          dnd_tree);
        g_signal_connect (dnd_tree,
                          "motion_notify_event",
                          G_CALLBACK (ario_dnd_tree_motion_notify),
                          dnd_tree);

        return GTK_WIDGET (dnd_tree);
}

static gboolean
ario_dnd_tree_button_press_cb (GtkWidget *widget,
                               GdkEventButton *event,
                               ArioDndTree *dnd_tree)
{
        ARIO_LOG_FUNCTION_START;
        GdkModifierType mods;
        GtkTreePath *path;
        int x, y, bx, by;
        gboolean selected;

        /* Grab focus if needed */
        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        /* Already in drag & drop we do nothing */
        if (dnd_tree->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        /* Double click: we emit the activate signal */
        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
                g_signal_emit (G_OBJECT (dnd_tree), ario_dnd_tree_signals[ACTIVATE], 0);
                return FALSE;
        }

        /* First button pressed */
        if (event->button == 1) {
                /* Get real coordinates */
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
                gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (widget), x, y, &bx, &by);

                if (dnd_tree->priv->browse_mode)
                        return FALSE;

                if (bx >= 0 && by >= 0) {
                        dnd_tree->priv->drag_start_x = x;
                        dnd_tree->priv->drag_start_y = y;
                        dnd_tree->priv->pressed = TRUE;

                        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                        if (path) {
                                selected = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), path);
                                gtk_tree_path_free (path);

                                return selected;
                        }

                        return TRUE;
                } else {
                        return FALSE;
                }

                return TRUE;
        }

        /* Third button pressed */
        if (event->button == 3) {
                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path) {
                        /* Select the clicked row */
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }

                        /* Emit the popup signal */
                        g_signal_emit (G_OBJECT (dnd_tree), ario_dnd_tree_signals[POPUP], 0);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
ario_dnd_tree_button_release_cb (GtkWidget *widget,
                                 GdkEventButton *event,
                                 ArioDndTree *dnd_tree)
{
        ARIO_LOG_FUNCTION_START;
        if (!dnd_tree->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                /* Get real coordinates */
                int bx, by;
                gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (widget), event->x, event->y, &bx, &by);
                if (bx >= 0 && by >= 0) {
                        /* Select the clicked row */
                        GtkTreePath *path;

                        gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                        if (path) {
                                GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                                gtk_tree_path_free (path);
                        }
                }
        }

        dnd_tree->priv->dragging = FALSE;
        dnd_tree->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
ario_dnd_tree_motion_notify (GtkWidget *widget,
                             GdkEventMotion *event,
                             ArioDndTree *dnd_tree)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START;
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((dnd_tree->priv->dragging) || !(dnd_tree->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - dnd_tree->priv->drag_start_x;
        dy = y - dnd_tree->priv->drag_start_y;

        /* Activate drag & drop if button pressed and mouse moved */
        if ((ario_util_abs (dx) > DRAG_THRESHOLD) || (ario_util_abs (dy) > DRAG_THRESHOLD))
                dnd_tree->priv->dragging = TRUE;

        return FALSE;
}
