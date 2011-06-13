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

#include "preferences/ario-browser-preferences.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-preferences.h"
#include "lib/gtk-builder-helpers.h"
#include "lib/ario-conf.h"
#include "sources/ario-browser.h"
#include "sources/ario-tree.h"
#include "ario-debug.h"

static void ario_browser_preferences_sync_browser (ArioBrowserPreferences *browser_preferences);
G_MODULE_EXPORT void ario_browser_preferences_sort_changed_cb (GtkComboBox *combobox,
                                                               ArioBrowserPreferences *browser_preferences);
G_MODULE_EXPORT void ario_browser_preferences_treesnb_changed_cb (GtkWidget *widget,
                                                                  ArioBrowserPreferences *browser_preferences);
static void ario_browser_preferences_tree_combobox_changed_cb (GtkComboBox *widget,
                                                               ArioBrowserPreferences *browser_preferences);


struct ArioBrowserPreferencesPrivate
{
        GtkWidget *sort_combobox;
        GtkWidget *treesnb_spinbutton;
        GtkWidget *hbox;
        GSList *tree_comboboxs;
};

#define ARIO_BROWSER_PREFERENCES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_BROWSER_PREFERENCES, ArioBrowserPreferencesPrivate))
G_DEFINE_TYPE (ArioBrowserPreferences, ario_browser_preferences, GTK_TYPE_VBOX)

static void
ario_browser_preferences_class_init (ArioBrowserPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        g_type_class_add_private (klass, sizeof (ArioBrowserPreferencesPrivate));
}

static void
ario_browser_preferences_init (ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START;
        browser_preferences->priv = ARIO_BROWSER_PREFERENCES_GET_PRIVATE (browser_preferences);
}

GtkWidget *
ario_browser_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioBrowserPreferences *browser_preferences;
        GtkBuilder *builder;

        browser_preferences = g_object_new (TYPE_ARIO_BROWSER_PREFERENCES, NULL);

        g_return_val_if_fail (browser_preferences->priv != NULL, NULL);

        builder = gtk_builder_helpers_new (UI_PATH "browser-prefs.ui",
                                           browser_preferences);

        browser_preferences->priv->sort_combobox =
                GTK_WIDGET (gtk_builder_get_object (builder, "sort_combobox"));
        browser_preferences->priv->hbox =
                GTK_WIDGET (gtk_builder_get_object (builder, "trees_hbox"));
        browser_preferences->priv->treesnb_spinbutton =
                GTK_WIDGET (gtk_builder_get_object (builder, "treesnb_spinbutton"));

        gtk_builder_helpers_boldify_label (builder, "options_label");
        gtk_builder_helpers_boldify_label (builder, "organisation_label");

        ario_browser_preferences_sync_browser (browser_preferences);

        gtk_box_pack_start (GTK_BOX (browser_preferences), GTK_WIDGET (gtk_builder_get_object (builder, "browser_vbox")), TRUE, TRUE, 0);

        g_object_unref (builder);

        return GTK_WIDGET (browser_preferences);
}

static void
ario_browser_preferences_sync_browser (ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GtkWidget *tree_combobox;
        int i, j;
        gchar **splited_conf;
        const gchar *conf;
        GSList *tmp;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int a, b;
        gchar **items;

        gtk_combo_box_set_active (GTK_COMBO_BOX (browser_preferences->priv->sort_combobox),
                                  ario_conf_get_integer (PREF_ALBUM_SORT, PREF_ALBUM_SORT_DEFAULT));

        /* Remove all trees */
        for (tmp = browser_preferences->priv->tree_comboboxs; tmp; tmp = g_slist_next (tmp)) {
                gtk_container_remove (GTK_CONTAINER (browser_preferences->priv->hbox), GTK_WIDGET (tmp->data));
        }
        g_slist_free (browser_preferences->priv->tree_comboboxs);
        browser_preferences->priv->tree_comboboxs = NULL;

        conf = ario_conf_get_string (PREF_BROWSER_TREES, PREF_BROWSER_TREES_DEFAULT);
        splited_conf = g_strsplit (conf, ",", MAX_TREE_NB);
        items = ario_server_get_items_names ();
        for (i = 0; splited_conf[i]; ++i) {
                tree_combobox = gtk_combo_box_new ();
                browser_preferences->priv->tree_comboboxs = g_slist_append (browser_preferences->priv->tree_comboboxs, tree_combobox);

                list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
                for (j = 0; j < ARIO_TAG_COUNT - 1; ++j) {
                        if (items[j]) {
                                gtk_list_store_append (list_store, &iter);
                                gtk_list_store_set (list_store, &iter,
                                                    0, gettext (items[j]),
                                                    1, j,
                                                    -1);
                        }
                }
                gtk_combo_box_set_model (GTK_COMBO_BOX (tree_combobox),
                                         GTK_TREE_MODEL (list_store));
                g_object_unref (list_store);

                renderer = gtk_cell_renderer_text_new ();
                gtk_cell_layout_clear (GTK_CELL_LAYOUT (tree_combobox));
                gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (tree_combobox), renderer, TRUE);
                gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (tree_combobox), renderer,
                                                "text", 0, NULL);

                a = atoi (splited_conf[i]);
                b = 0;
                for (j = 0; j < ARIO_TAG_COUNT - 1 && j < a; ++j) {
                        if (items[j])
                                ++b;
                }
                gtk_combo_box_set_active (GTK_COMBO_BOX (tree_combobox), b);

                g_signal_connect (G_OBJECT (tree_combobox),
                                  "changed", G_CALLBACK (ario_browser_preferences_tree_combobox_changed_cb), browser_preferences);

                gtk_box_pack_start (GTK_BOX (browser_preferences->priv->hbox), tree_combobox, TRUE, TRUE, 0);
        }
        gtk_widget_show_all (browser_preferences->priv->hbox);
        g_strfreev (splited_conf);

        gtk_spin_button_set_value (GTK_SPIN_BUTTON (browser_preferences->priv->treesnb_spinbutton), (gdouble) i);
}

void
ario_browser_preferences_sort_changed_cb (GtkComboBox *combobox,
                                          ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START;
        int i;

        i = gtk_combo_box_get_active (combobox);

        ario_conf_set_integer (PREF_ALBUM_SORT, i);
}

static gboolean
ario_browser_preferences_treesnb_changed_idle (ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START;
        gchar **splited_conf;
        const gchar *conf;
        gchar *new_conf, *tmp;
        int old_nb, new_nb, i;
        gdouble new_nb_double;

        conf = ario_conf_get_string (PREF_BROWSER_TREES, PREF_BROWSER_TREES_DEFAULT);

        splited_conf = g_strsplit (conf, ",", MAX_TREE_NB);
        for (old_nb = 0; splited_conf[old_nb]; ++old_nb) {}

        new_nb_double = gtk_spin_button_get_value (GTK_SPIN_BUTTON (browser_preferences->priv->treesnb_spinbutton));
        new_nb = (int) new_nb_double;
        if (new_nb > old_nb) {
                new_conf = g_strdup_printf ("%s,0", conf);
                ario_conf_set_string (PREF_BROWSER_TREES, new_conf);
                g_free (new_conf);
                ario_browser_preferences_sync_browser (browser_preferences);
        } else if (new_nb < old_nb) {
                new_conf = g_strdup (splited_conf[0]);
                for (i = 1; i < new_nb; ++i) {
                        tmp = g_strdup_printf ("%s,%s", new_conf, splited_conf[i]);
                        g_free (new_conf);
                        new_conf = tmp;
                }
                ario_conf_set_string (PREF_BROWSER_TREES, new_conf);
                g_free (new_conf);
                ario_browser_preferences_sync_browser (browser_preferences);
        }
        g_strfreev (splited_conf);

        return FALSE;
}

void
ario_browser_preferences_treesnb_changed_cb (GtkWidget *widget,
                                             ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START;
        g_idle_add ((GSourceFunc) ario_browser_preferences_treesnb_changed_idle, browser_preferences);
}

static void
ario_browser_preferences_tree_combobox_changed_cb (GtkComboBox *widget,
                                                   ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START;
        GSList *temp;
        GtkComboBox *combobox;
        gchar *conf = NULL, *tmp;
        int value;
        GtkTreeIter iter;

        for (temp = browser_preferences->priv->tree_comboboxs; temp; temp = g_slist_next (temp)) {
                combobox = temp->data;
                gtk_combo_box_get_active_iter (combobox, &iter);
                gtk_tree_model_get (gtk_combo_box_get_model (combobox),
                                    &iter,
                                    1, &value,
                                    -1);
                if (!conf) {
                        conf = g_strdup_printf ("%d", value);
                } else {
                        tmp = g_strdup_printf ("%s,%d", conf, value);
                        g_free (conf);
                        conf = tmp;
                }
        }
        ario_conf_set_string (PREF_BROWSER_TREES, conf);
        g_free (conf);
}

