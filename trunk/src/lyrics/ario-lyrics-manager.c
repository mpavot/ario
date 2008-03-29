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
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "lyrics/ario-lyrics-manager.h"
#include "lyrics/ario-lyrics-leoslyrics.h"
#include "lyrics/ario-lyrics-lyricwiki.h"
#include "lyrics/ario-lyrics.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_lyrics_manager_class_init (ArioLyricsManagerClass *klass);
static void ario_lyrics_manager_init (ArioLyricsManager *lyrics_manager);
static void ario_lyrics_manager_finalize (GObject *object);

struct ArioLyricsManagerPrivate
{
        GSList *providers;
};

static GObjectClass *parent_class = NULL;

GType
ario_lyrics_manager_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioLyricsManagerClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_lyrics_manager_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioLyricsManager),
                        0,
                        (GInstanceInitFunc) ario_lyrics_manager_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "ArioLyricsManager",
                                               &our_info, 0);
        }
        return type;
}

static void
ario_lyrics_manager_class_init (ArioLyricsManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_lyrics_manager_finalize;
}

static void
ario_lyrics_manager_init (ArioLyricsManager *lyrics_manager)
{
        ARIO_LOG_FUNCTION_START

        lyrics_manager->priv = g_new0 (ArioLyricsManagerPrivate, 1);
}

static void
ario_lyrics_manager_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioLyricsManager *lyrics_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_LYRICS_MANAGER (object));

        lyrics_manager = ARIO_LYRICS_MANAGER (object);

        g_return_if_fail (lyrics_manager->priv != NULL);
        g_free (lyrics_manager->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioLyricsManager *
ario_lyrics_manager_get_instance (void)
{
        ARIO_LOG_FUNCTION_START
        static ArioLyricsManager *lyrics_manager = NULL;

        if (!lyrics_manager) {
                ArioLyricsProvider *lyrics_provider;

                lyrics_manager = g_object_new (TYPE_ARIO_LYRICS_MANAGER,
                                              NULL);
                g_return_val_if_fail (lyrics_manager->priv != NULL, NULL);

                lyrics_provider = ario_lyrics_leoslyrics_new ();
                ario_lyrics_manager_add_provider (lyrics_manager,
                                                 ARIO_LYRICS_PROVIDER (lyrics_provider));

                lyrics_provider = ario_lyrics_lyricwiki_new ();
                ario_lyrics_manager_add_provider (lyrics_manager,
                                                 ARIO_LYRICS_PROVIDER (lyrics_provider));

                ario_lyrics_manager_update_providers (lyrics_manager);
        }

        return ARIO_LYRICS_MANAGER (lyrics_manager);
}

static void
ario_lyrics_manager_shutdown_foreach (ArioLyricsProvider *lyrics_provider,
                                     GSList **providers)
{
        *providers = g_slist_append (*providers, ario_lyrics_provider_get_id (lyrics_provider));
}

static void
ario_lyrics_manager_shutdown_active_foreach (ArioLyricsProvider *lyrics_provider,
                                            GSList **providers)
{
        if (ario_lyrics_provider_is_active (lyrics_provider))
                *providers = g_slist_append (*providers, ario_lyrics_provider_get_id (lyrics_provider));
}

void
ario_lyrics_manager_shutdown (ArioLyricsManager *lyrics_manager)
{

        GSList *providers = NULL;
        GSList *active_providers = NULL;

        g_slist_foreach (lyrics_manager->priv->providers, (GFunc) ario_lyrics_manager_shutdown_foreach, &providers);
        g_slist_foreach (lyrics_manager->priv->providers, (GFunc) ario_lyrics_manager_shutdown_active_foreach, &active_providers);

        ario_conf_set_string_slist (LYRICS_PROVIDERS_LIST, providers);
        ario_conf_set_string_slist (LYRICS_ACTIVE_PROVIDERS_LIST, active_providers);
        g_slist_free (providers);
        g_slist_free (active_providers);
}

static gint
ario_lyrics_manager_compare_providers (ArioLyricsProvider *lyrics_provider,
                                      const gchar *id)
{
        return strcmp (ario_lyrics_provider_get_id (lyrics_provider), id);
}

void
ario_lyrics_manager_update_providers (ArioLyricsManager *lyrics_manager)
{
        ARIO_LOG_FUNCTION_START
        GSList *conf_tmp;
        GSList *conf_providers;
        GSList *conf_active_providers;
        GSList *found;
        GSList *providers = NULL;
        ArioLyricsProvider *lyrics_provider;

        conf_providers = ario_conf_get_string_slist (LYRICS_PROVIDERS_LIST);
        for (conf_tmp = conf_providers; conf_tmp; conf_tmp = g_slist_next (conf_tmp)) {
                found = g_slist_find_custom (lyrics_manager->priv->providers,
                                             conf_tmp->data,
                                             (GCompareFunc) ario_lyrics_manager_compare_providers);
                if (found) {
                        providers = g_slist_append (providers, found->data);
                }
        }
        g_slist_foreach (conf_providers, (GFunc) g_free, NULL);
        g_slist_free (conf_providers);

        conf_active_providers = ario_conf_get_string_slist (LYRICS_ACTIVE_PROVIDERS_LIST);
        for (conf_tmp = conf_active_providers; conf_tmp; conf_tmp = g_slist_next (conf_tmp)) {
                found = g_slist_find_custom (providers,
                                             conf_tmp->data,
                                             (GCompareFunc) ario_lyrics_manager_compare_providers);
                if (found) {
                        lyrics_provider = found->data;
                        ario_lyrics_provider_set_active (lyrics_provider, TRUE);
                }
        }
        g_slist_foreach (conf_active_providers, (GFunc) g_free, NULL);
        g_slist_free (conf_active_providers);

        for (conf_tmp = lyrics_manager->priv->providers; conf_tmp; conf_tmp = g_slist_next (conf_tmp)) {
                if (!g_slist_find (providers, conf_tmp->data)) {
                        providers = g_slist_append (providers, conf_tmp->data);
                }
        }

        g_slist_free (lyrics_manager->priv->providers);
        lyrics_manager->priv->providers = providers;
}

GSList*
ario_lyrics_manager_get_providers (ArioLyricsManager *lyrics_manager)
{
        ARIO_LOG_FUNCTION_START
        return lyrics_manager->priv->providers;
}

void
ario_lyrics_manager_set_providers (ArioLyricsManager *lyrics_manager,
                                  GSList *providers)
{
        ARIO_LOG_FUNCTION_START
        lyrics_manager->priv->providers = providers;
}

ArioLyricsProvider*
ario_lyrics_manager_get_provider_from_id (ArioLyricsManager *lyrics_manager,
                                         const gchar *id)
{
        ARIO_LOG_FUNCTION_START
        GSList *found;

        found = g_slist_find_custom (lyrics_manager->priv->providers,
                                     id,
                                     (GCompareFunc) ario_lyrics_manager_compare_providers);

        return ARIO_LYRICS_PROVIDER (found->data);
}


void
ario_lyrics_manager_add_provider (ArioLyricsManager *lyrics_manager,
                                 ArioLyricsProvider *lyrics_provider)
{
        ARIO_LOG_FUNCTION_START
        lyrics_manager->priv->providers = g_slist_append (lyrics_manager->priv->providers, lyrics_provider);
}

void
ario_lyrics_manager_remove_provider (ArioLyricsManager *lyrics_manager,
                                    ArioLyricsProvider *lyrics_provider)
{
        ARIO_LOG_FUNCTION_START
        lyrics_manager->priv->providers = g_slist_remove (lyrics_manager->priv->providers, lyrics_provider);
}

ArioLyrics *
ario_lyrics_manager_get_lyrics (ArioLyricsManager *lyrics_manager,
                               const char *artist,
                               const char *song,
                               const char *file)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioLyricsProvider *lyrics_provider;
        ArioLyrics *lyrics = NULL;

        if (ario_lyrics_lyrics_exists (artist, song)) {
                lyrics = ario_lyrics_get_local_lyrics (artist, song);
        }
        if (lyrics)
                return lyrics;

        for (tmp = lyrics_manager->priv->providers; tmp; tmp = g_slist_next (tmp)) {
                lyrics_provider = tmp->data;
                if (!ario_lyrics_provider_is_active (lyrics_provider))
                        continue;
                ARIO_LOG_DBG ("looking for lyrics using provider:%s for song:%s\n", ario_lyrics_provider_get_name (lyrics_provider), song);

                lyrics = ario_lyrics_provider_get_lyrics (lyrics_provider,
                                                          artist, song,
                                                          file);
                if (lyrics)
                        break;
        }
        if (lyrics) {
                ario_lyrics_prepend_infos (lyrics);
                ario_lyrics_save_lyrics (artist,
                                         song,
                                         lyrics->lyrics);
        }

        return lyrics;
}


void
ario_lyrics_manager_get_lyrics_candidates (ArioLyricsManager *lyrics_manager,
                                           const gchar *artist,
                                           const gchar *song,
                                           GSList **candidates)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioLyricsProvider *lyrics_provider;

        for (tmp = lyrics_manager->priv->providers; tmp; tmp = g_slist_next (tmp)) {
                lyrics_provider = tmp->data;
                if (!ario_lyrics_provider_is_active (lyrics_provider))
                        continue;

                ario_lyrics_provider_get_lyrics_candidates (lyrics_provider,
                                                            artist, song,
                                                            candidates);
        }
}

