/*
 *  Copyright (C) 2009 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "playlist/ario-playlist-dynamic.h"
#include "shell/ario-shell-similarartists.h"
#include "servers/ario-server.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_playlist_dynamic_class_init (ArioPlaylistDynamicClass *klass);
static void ario_playlist_dynamic_init (ArioPlaylistDynamic *playlist_dynamic);
static void ario_playlist_dynamic_finalize (GObject *object);
static void ario_playlist_dynamic_last_song (ArioPlaylistMode *playlist_mode,
                                             ArioPlaylist *playlist);
static GtkWidget* ario_playlist_dynamic_get_config (ArioPlaylistMode *playlist_mode);

struct ArioPlaylistDynamicPrivate
{
        gboolean dummy;
};

static GObjectClass *parent_class = NULL;

typedef enum
{
        SAME_ARTIST,
        SAME_ALBUM,
        SIMILAR_ARTISTS
} ArioDynamicType;

static const char *dynamic_type[] = {
        N_("same artist"),
        N_("same album"),
        N_("similar artists"),
        NULL
};

GType
ario_playlist_dynamic_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioPlaylistDynamicClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_playlist_dynamic_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioPlaylistDynamic),
                        0,
                        (GInstanceInitFunc) ario_playlist_dynamic_init
                };

                type = g_type_register_static (ARIO_TYPE_PLAYLIST_MODE,
                                               "ArioPlaylistDynamic",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_playlist_dynamic_get_id (ArioPlaylistMode *playlist_mode)
{
        return "dynamic";
}

static gchar *
ario_playlist_dynamic_get_name (ArioPlaylistMode *playlist_mode)
{
        return _("Dynamic Playlist");
}

static void
ario_playlist_dynamic_class_init (ArioPlaylistDynamicClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPlaylistModeClass *playlist_mode_class = ARIO_PLAYLIST_MODE_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_playlist_dynamic_finalize;

        playlist_mode_class->get_id = ario_playlist_dynamic_get_id;
        playlist_mode_class->get_name = ario_playlist_dynamic_get_name;
        playlist_mode_class->last_song = ario_playlist_dynamic_last_song;
        playlist_mode_class->get_config = ario_playlist_dynamic_get_config;
}

static void
ario_playlist_dynamic_init (ArioPlaylistDynamic *playlist_dynamic)
{
        ARIO_LOG_FUNCTION_START
        playlist_dynamic->priv = g_new0 (ArioPlaylistDynamicPrivate, 1);
}

static void
ario_playlist_dynamic_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistDynamic *playlist_dynamic;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_PLAYLIST_DYNAMIC (object));

        playlist_dynamic = ARIO_PLAYLIST_DYNAMIC (object);

        g_return_if_fail (playlist_dynamic->priv != NULL);
        g_free (playlist_dynamic->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioPlaylistMode*
ario_playlist_dynamic_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioPlaylistDynamic *dynamic;

        dynamic = g_object_new (TYPE_ARIO_PLAYLIST_DYNAMIC,
                                NULL);

        g_return_val_if_fail (dynamic->priv != NULL, NULL);

        return ARIO_PLAYLIST_MODE (dynamic);
}

static void
ario_playlist_dynamic_last_song (ArioPlaylistMode *playlist_mode,
                                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        GSList *artists = NULL;
        ArioServerAtomicCriteria atomic_criteria1;
        ArioServerAtomicCriteria atomic_criteria2;
        ArioServerCriteria *criteria = NULL;
        GSList *criterias = NULL;
        int nbsongs = ario_conf_get_integer (PREF_DYNAMIC_NBSONGS, PREF_DYNAMIC_NBSONGS_DEFAULT);

        switch (ario_conf_get_integer (PREF_DYNAMIC_TYPE, PREF_DYNAMIC_TYPE_DEFAULT)) {
        case SAME_ARTIST:
                artists = g_slist_append (artists, ario_server_get_current_artist ());
                ario_server_playlist_append_artists (artists, FALSE, nbsongs);
                g_slist_free (artists);
                break;
        case SAME_ALBUM:
                atomic_criteria1.tag = MPD_TAG_ITEM_ARTIST;
                atomic_criteria1.value = ario_server_get_current_artist ();
                atomic_criteria2.tag = MPD_TAG_ITEM_ALBUM;
                atomic_criteria2.value = ario_server_get_current_album ();

                criteria = g_slist_append (criteria, &atomic_criteria1);
                criteria = g_slist_append (criteria, &atomic_criteria2);

                criterias = g_slist_append (criterias, criteria);

                ario_server_playlist_append_criterias (criterias, FALSE, nbsongs);

                g_slist_free (criteria);
                g_slist_free (criterias);
                break;
        case SIMILAR_ARTISTS:
                ario_shell_similarartists_add_similar_to_playlist (ario_server_get_current_artist (),
                                                                   nbsongs);
                break;
        }
}

static void
ario_playlist_dynamic_type_combobox_changed_cb (GtkComboBox *combobox,
                                                ArioPlaylistMode *playlist_mode)
{
        ARIO_LOG_FUNCTION_START
        GtkTreeIter iter;
        int type;

        gtk_combo_box_get_active_iter (combobox, &iter);
        gtk_tree_model_get (gtk_combo_box_get_model (combobox),
                            &iter,
                            1, &type,
                            -1);
        ario_conf_set_integer (PREF_DYNAMIC_TYPE, type);
}

static void
ario_playlist_dynamic_nbsongs_changed_cb (GtkWidget *widget,
                                          ArioPlaylistMode *playlist_mode)
{
        ARIO_LOG_FUNCTION_START
        ario_conf_set_integer (PREF_DYNAMIC_NBSONGS,
                               (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));
}

static GtkWidget*
ario_playlist_dynamic_get_config (ArioPlaylistMode *playlist_mode)
{
        GtkWidget *hbox;
        GtkWidget *spinbutton;
        GtkWidget *combobox;
        GtkAdjustment *adj;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;

        hbox = gtk_hbox_new (FALSE, 4);
        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new ("Automatically add"),
                            FALSE, FALSE,
                            0);

        adj = GTK_ADJUSTMENT (gtk_adjustment_new (10,
                                                  0.0,
                                                  1000.0,
                                                  1.0,
                                                  10.0,
                                                  0.0));

        spinbutton = gtk_spin_button_new (adj,
                                          1.0, 0);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton),
                                   (double) ario_conf_get_integer (PREF_DYNAMIC_NBSONGS, PREF_DYNAMIC_NBSONGS_DEFAULT));

        g_signal_connect (G_OBJECT (spinbutton),
                          "value_changed",
                          G_CALLBACK (ario_playlist_dynamic_nbsongs_changed_cb), playlist_mode);

        gtk_box_pack_start (GTK_BOX (hbox),
                            spinbutton,
                            FALSE, FALSE,
                            0);

        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new ("songs of"),
                            FALSE, FALSE,
                            0);

        combobox = gtk_combo_box_new ();

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
        for (i = 0; dynamic_type[i]; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (dynamic_type[i]),
                                    1, i,
                                    -1);
        }
        gtk_combo_box_set_model (GTK_COMBO_BOX (combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0, NULL);

        gtk_combo_box_set_active (GTK_COMBO_BOX (combobox),
                                  ario_conf_get_integer (PREF_DYNAMIC_TYPE, PREF_DYNAMIC_TYPE_DEFAULT));

        g_signal_connect (G_OBJECT (combobox),
                          "changed",
                          G_CALLBACK (ario_playlist_dynamic_type_combobox_changed_cb), playlist_mode);

        gtk_box_pack_start (GTK_BOX (hbox),
                            combobox,
                            FALSE, FALSE,
                            0);

        gtk_box_pack_start (GTK_BOX (hbox),
                            gtk_label_new ("to playlist."),
                            FALSE, FALSE,
                            0);
        return hbox;
}
