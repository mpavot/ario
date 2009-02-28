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

#include "covers/ario-cover-manager.h"
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "covers/ario-cover-amazon.h"
#include "covers/ario-cover-lastfm.h"
#include "covers/ario-cover-local.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

struct ArioCoverManagerPrivate
{
        GSList *providers;
};

#define ARIO_COVER_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_COVER_MANAGER, ArioCoverManagerPrivate))
G_DEFINE_TYPE (ArioCoverManager, ario_cover_manager, G_TYPE_OBJECT)

static void
ario_cover_manager_class_init (ArioCoverManagerClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        g_type_class_add_private (klass, sizeof (ArioCoverManagerPrivate));
}

static void
ario_cover_manager_init (ArioCoverManager *cover_manager)
{
        ARIO_LOG_FUNCTION_START;

        cover_manager->priv = ARIO_COVER_MANAGER_GET_PRIVATE (cover_manager);
}

ArioCoverManager *
ario_cover_manager_get_instance (void)
{
        ARIO_LOG_FUNCTION_START;
        static ArioCoverManager *cover_manager = NULL;

        if (!cover_manager) {
                ArioCoverProvider *cover_provider;

                cover_manager = g_object_new (TYPE_ARIO_COVER_MANAGER,
                                              NULL);
                g_return_val_if_fail (cover_manager->priv != NULL, NULL);

                cover_provider = ario_cover_local_new ();
                ario_cover_manager_add_provider (cover_manager,
                                                 ARIO_COVER_PROVIDER (cover_provider));

                cover_provider = ario_cover_amazon_new ();
                ario_cover_manager_add_provider (cover_manager,
                                                 ARIO_COVER_PROVIDER (cover_provider));

                cover_provider = ario_cover_lastfm_new ();
                ario_cover_manager_add_provider (cover_manager,
                                                 ARIO_COVER_PROVIDER (cover_provider));

                ario_cover_manager_update_providers (cover_manager);
        }

        return cover_manager;
}

static void
ario_cover_manager_shutdown_foreach (ArioCoverProvider *cover_provider,
                                     GSList **providers)
{
        *providers = g_slist_append (*providers, ario_cover_provider_get_id (cover_provider));
}

static void
ario_cover_manager_shutdown_active_foreach (ArioCoverProvider *cover_provider,
                                            GSList **providers)
{
        if (ario_cover_provider_is_active (cover_provider))
                *providers = g_slist_append (*providers, ario_cover_provider_get_id (cover_provider));
}

void
ario_cover_manager_shutdown (ArioCoverManager *cover_manager)
{

        GSList *providers = NULL;
        GSList *active_providers = NULL;

        g_slist_foreach (cover_manager->priv->providers, (GFunc) ario_cover_manager_shutdown_foreach, &providers);
        g_slist_foreach (cover_manager->priv->providers, (GFunc) ario_cover_manager_shutdown_active_foreach, &active_providers);

        ario_conf_set_string_slist (PREF_COVER_PROVIDERS_LIST, providers);
        ario_conf_set_string_slist (PREF_COVER_ACTIVE_PROVIDERS_LIST, active_providers);
        g_slist_free (providers);
        g_slist_free (active_providers);
}

static gint
ario_cover_manager_compare_providers (ArioCoverProvider *cover_provider,
                                      const gchar *id)
{
        return strcmp (ario_cover_provider_get_id (cover_provider), id);
}

void
ario_cover_manager_update_providers (ArioCoverManager *cover_manager)
{
        ARIO_LOG_FUNCTION_START;
        GSList *conf_tmp;
        GSList *conf_providers;
        GSList *conf_active_providers;
        GSList *found;
        GSList *providers = NULL;
        ArioCoverProvider *cover_provider;

        conf_providers = ario_conf_get_string_slist (PREF_COVER_PROVIDERS_LIST, PREF_COVER_PROVIDERS_LIST_DEFAULT);
        for (conf_tmp = conf_providers; conf_tmp; conf_tmp = g_slist_next (conf_tmp)) {
                found = g_slist_find_custom (cover_manager->priv->providers,
                                             conf_tmp->data,
                                             (GCompareFunc) ario_cover_manager_compare_providers);
                if (found) {
                        providers = g_slist_append (providers, found->data);
                }
        }
        g_slist_foreach (conf_providers, (GFunc) g_free, NULL);
        g_slist_free (conf_providers);

        conf_active_providers = ario_conf_get_string_slist (PREF_COVER_ACTIVE_PROVIDERS_LIST, PREF_COVER_ACTIVE_PROVIDERS_LIST_DEFAULT);
        for (conf_tmp = conf_active_providers; conf_tmp; conf_tmp = g_slist_next (conf_tmp)) {
                found = g_slist_find_custom (providers,
                                             conf_tmp->data,
                                             (GCompareFunc) ario_cover_manager_compare_providers);
                if (found) {
                        cover_provider = found->data;
                        ario_cover_provider_set_active (cover_provider, TRUE);
                }
        }
        g_slist_foreach (conf_active_providers, (GFunc) g_free, NULL);
        g_slist_free (conf_active_providers);

        for (conf_tmp = cover_manager->priv->providers; conf_tmp; conf_tmp = g_slist_next (conf_tmp)) {
                if (!g_slist_find (providers, conf_tmp->data)) {
                        providers = g_slist_append (providers, conf_tmp->data);
                }
        }

        g_slist_free (cover_manager->priv->providers);
        cover_manager->priv->providers = providers;
}

GSList*
ario_cover_manager_get_providers (ArioCoverManager *cover_manager)
{
        ARIO_LOG_FUNCTION_START;
        return cover_manager->priv->providers;
}

void
ario_cover_manager_set_providers (ArioCoverManager *cover_manager,
                                  GSList *providers)
{
        ARIO_LOG_FUNCTION_START;
        cover_manager->priv->providers = providers;
}

ArioCoverProvider*
ario_cover_manager_get_provider_from_id (ArioCoverManager *cover_manager,
                                         const gchar *id)
{
        ARIO_LOG_FUNCTION_START;
        GSList *found;

        found = g_slist_find_custom (cover_manager->priv->providers,
                                     id,
                                     (GCompareFunc) ario_cover_manager_compare_providers);

        return ARIO_COVER_PROVIDER (found->data);
}


void
ario_cover_manager_add_provider (ArioCoverManager *cover_manager,
                                 ArioCoverProvider *cover_provider)
{
        ARIO_LOG_FUNCTION_START;
        cover_manager->priv->providers = g_slist_append (cover_manager->priv->providers, cover_provider);
}

void
ario_cover_manager_remove_provider (ArioCoverManager *cover_manager,
                                    ArioCoverProvider *cover_provider)
{
        ARIO_LOG_FUNCTION_START;
        cover_manager->priv->providers = g_slist_remove (cover_manager->priv->providers, cover_provider);
}

gboolean
ario_cover_manager_get_covers (ArioCoverManager *cover_manager,
                               const char *artist,
                               const char *album,
                               const char *file,
                               GArray **file_size,
                               GSList **file_contents,
                               ArioCoverProviderOperation operation)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp;
        ArioCoverProvider *cover_provider;
        gboolean ret = FALSE;

        for (tmp = cover_manager->priv->providers; tmp; tmp = g_slist_next (tmp)) {
                cover_provider = tmp->data;
                if (!ario_cover_provider_is_active (cover_provider))
                        continue;
                ARIO_LOG_DBG ("looking for a cover using provider:%s for album:%s\n", ario_cover_provider_get_name (cover_provider), album);

                if (ario_cover_provider_get_covers (cover_provider,
                                                    artist, album,
                                                    file,
                                                    file_size, file_contents,
                                                    operation)) {
                        ret = TRUE;
                        if (operation == GET_FIRST_COVER)
                                break;
                }
        }
        return ret;
}


