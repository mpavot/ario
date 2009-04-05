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

#include "ario-avahi.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>
#include <avahi-common/domain.h>
#include <avahi-common/error.h>
#include <avahi-common/simple-watch.h>
#include "ario-debug.h"

#define SERVICE_TYPE "_mpd._tcp"
#define DOMAIN "local"

static void ario_avahi_finalize (GObject *object);
static void ario_avahi_resolve_callback (AvahiServiceResolver *r,
                                         AVAHI_GCC_UNUSED AvahiIfIndex interface,
                                         AVAHI_GCC_UNUSED AvahiProtocol protocol,
                                         AvahiResolverEvent event,
                                         const char *name,
                                         const char *type,
                                         const char *domain,
                                         const char *host_name,
                                         const AvahiAddress *address,
                                         uint16_t port,
                                         AvahiStringList *txt,
                                         AvahiLookupResultFlags flags,
                                         AVAHI_GCC_UNUSED void *userdata);
static void ario_avahi_browse_callback (AvahiServiceBrowser *b,
                                        AvahiIfIndex interface,
                                        AvahiProtocol protocol,
                                        AvahiBrowserEvent event,
                                        const char *name,
                                        const char *type,
                                        const char *domain,
                                        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                                        void *userdata);
static void ario_avahi_client_callback (AvahiClient *c,
                                        AvahiClientState state,
                                        AVAHI_GCC_UNUSED void *userdata);

struct ArioAvahiPrivate
{
        AvahiGLibPoll *glib_poll;
        const AvahiPoll *poll_api;
        AvahiClient *client;
        AvahiServiceBrowser *browser;

        GSList *hosts;
};

enum
{
        HOSTS_CHANGED,
        LAST_SIGNAL
};
static guint ario_avahi_signals[LAST_SIGNAL] = { 0 };

#define ARIO_AVAHI_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_AVAHI, ArioAvahiPrivate))
G_DEFINE_TYPE (ArioAvahi, ario_avahi, G_TYPE_OBJECT)

static void
ario_avahi_class_init (ArioAvahiClass *klass)
{
        ARIO_LOG_FUNCTION_START;
        GObjectClass *object_class = (GObjectClass *) klass;

        /* Virtual methods */
        object_class->finalize = ario_avahi_finalize;

        /* Signals */
        ario_avahi_signals[HOSTS_CHANGED] =
                g_signal_new ("hosts_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioAvahiClass, hosts_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /* Private attributes */
        g_type_class_add_private (klass, sizeof (ArioAvahiPrivate));
}

static void
ario_avahi_init (ArioAvahi *avahi)
{
        ARIO_LOG_FUNCTION_START;
        int error;

        avahi->priv = ARIO_AVAHI_GET_PRIVATE (avahi);
        avahi->priv->hosts = NULL;

        /* Allocate main loop object */
        avahi->priv->glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
        avahi->priv->poll_api = avahi_glib_poll_get (avahi->priv->glib_poll);
        if (!avahi->priv->glib_poll) {
                ARIO_LOG_ERROR ("Failed to create simple poll object.");
                return;
        }

        /* Allocate a new client */
        avahi->priv->client = avahi_client_new (avahi->priv->poll_api, 0, ario_avahi_client_callback, avahi, &error);
        if (!avahi->priv->client) {
                ARIO_LOG_ERROR ("Failed to create client: %s", avahi_strerror (error));
                return;
        }

        /* Allocate a browser */
        avahi->priv->browser = avahi_service_browser_new (avahi->priv->client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
                                                          SERVICE_TYPE, DOMAIN, 0, ario_avahi_browse_callback, avahi);
        if (!avahi->priv->browser) {
                ARIO_LOG_ERROR ("Failed to create service browser for domain %s: %s", DOMAIN,
                                avahi_strerror (avahi_client_errno (avahi->priv->client)));
                return;
        }
}

static void
ario_avahi_free_hosts (ArioHost *host)
{
        ARIO_LOG_FUNCTION_START;

        if (host) {
                g_free (host->name);
                g_free (host->host);
                g_free (host);
        }
}

static void
ario_avahi_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START;
        ArioAvahi *avahi = ARIO_AVAHI (object);

        if (avahi->priv->client)
                avahi_client_free (avahi->priv->client);
        if (avahi->priv->glib_poll)
                avahi_glib_poll_free (avahi->priv->glib_poll);
        g_slist_foreach (avahi->priv->hosts, (GFunc) ario_avahi_free_hosts, NULL);
        g_slist_free (avahi->priv->hosts);

        G_OBJECT_CLASS (ario_avahi_parent_class)->finalize (object);
}

ArioAvahi *
ario_avahi_new (void)
{
        ARIO_LOG_FUNCTION_START;
        ArioAvahi *avahi;

        avahi = g_object_new (TYPE_ARIO_AVAHI, NULL);

        return avahi;
}

/**
 * Returns a list of local IP address
 */
static GSList *
ario_avahi_get_local_addr ()
{
        char ip[200];
        socklen_t salen;
        struct ifaddrs *ifa = NULL, *ifp = NULL;
        GSList *addrs = NULL;

        if (getifaddrs (&ifp) < 0) {
                return NULL;
        }

        for (ifa = ifp; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr)
                        continue;

                if (ifa->ifa_addr->sa_family == AF_INET)
                        salen = sizeof (struct sockaddr_in);
                else if (ifa->ifa_addr->sa_family == AF_INET6)
                        salen = sizeof (struct sockaddr_in6);
                else
                        continue;

                if (getnameinfo (ifa->ifa_addr, salen, ip, sizeof (ip), NULL, 0, NI_NUMERICHOST) < 0) {
                        continue;
                }
                /* Add the address to the list */
                addrs = g_slist_append (addrs, g_strdup (ip));
        }

        freeifaddrs (ifp);

        return addrs;
}

static gboolean
ario_avahi_is_local_addr (const gchar *addr)
{
        static GSList *addrs = NULL;

        if (!addrs)
                addrs = ario_avahi_get_local_addr ();

        if (!addrs)
                return FALSE;

        return (g_slist_find_custom (addrs, addr, (GCompareFunc) strcmp) != NULL);
}

static void ario_avahi_resolve_callback (AvahiServiceResolver *r,
                                         AVAHI_GCC_UNUSED AvahiIfIndex interface,
                                         AVAHI_GCC_UNUSED AvahiProtocol protocol,
                                         AvahiResolverEvent event,
                                         const char *name,
                                         const char *type,
                                         const char *domain,
                                         const char *host_name,
                                         const AvahiAddress *address,
                                         uint16_t port,
                                         AvahiStringList *txt,
                                         AvahiLookupResultFlags flags,
                                         AVAHI_GCC_UNUSED void *userdata)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (r);
        ArioAvahi *avahi = ARIO_AVAHI (userdata);

        /* Called whenever a service has been resolved successfully or timed out */
        switch (event) {
        case AVAHI_RESOLVER_FAILURE:
                ARIO_LOG_ERROR ("(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s",
                                name, type, domain, avahi_strerror(avahi_client_errno (avahi_service_resolver_get_client (r))));
                break;

        case AVAHI_RESOLVER_FOUND:
                {
                        /* Create a new ArioHost with host info */
                        char addr[AVAHI_ADDRESS_STR_MAX];
                        ArioHost *host;

                        avahi_address_snprint(addr, sizeof(addr), address);

                        host = (ArioHost *) g_malloc (sizeof (ArioHost));
                        host->name = g_strdup (name);
                        if (ario_avahi_is_local_addr (addr))
                                host->host = g_strdup ("localhost");
                        else
                                host->host = g_strdup (addr);
                        host->port = port;

                        avahi->priv->hosts = g_slist_append (avahi->priv->hosts, host);

                        g_signal_emit (G_OBJECT (avahi), ario_avahi_signals[HOSTS_CHANGED], 0);
                }
        }

        avahi_service_resolver_free (r);
}

static void ario_avahi_remove_host (ArioAvahi *avahi,
                                    const gchar *name)
{
        ARIO_LOG_FUNCTION_START;
        GSList *tmp;

        for (tmp = avahi->priv->hosts; tmp; tmp = g_slist_next (tmp)) {
                ArioHost *host = tmp->data;
                if (name && host->name && !strcmp (host->name, name)) {
                        avahi->priv->hosts = g_slist_remove (avahi->priv->hosts, host);
                        g_signal_emit (G_OBJECT (avahi), ario_avahi_signals[HOSTS_CHANGED], 0);
                }
        }
}

static void ario_avahi_browse_callback (AvahiServiceBrowser *b,
                                        AvahiIfIndex interface,
                                        AvahiProtocol protocol,
                                        AvahiBrowserEvent event,
                                        const char *name,
                                        const char *type,
                                        const char *domain,
                                        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                                        void *userdata)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (b);
        ArioAvahi *avahi = ARIO_AVAHI (userdata);

        /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

        switch (event) {
        case AVAHI_BROWSER_FAILURE:
                ARIO_LOG_ERROR ("(Browser) %s", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
                return;

        case AVAHI_BROWSER_NEW:
                ARIO_LOG_DBG ("(Browser) NEW: service '%s' of type '%s' in domain '%s'", name, type, domain);

                /* We ignore the returned resolver object. In the callback
                   function we free it. If the server is terminated before
                   the callback function is called the server will free
                   the resolver for us. */
                if (!(avahi_service_resolver_new (avahi->priv->client, interface, protocol, name, type, domain, protocol, 0, ario_avahi_resolve_callback, avahi)))
                        ARIO_LOG_ERROR ("Failed to resolve service '%s': %s", name, avahi_strerror (avahi_client_errno (avahi->priv->client)));
                break;

        case AVAHI_BROWSER_REMOVE:
                ARIO_LOG_DBG ("(Browser) REMOVE: service '%s' of type '%s' in domain '%s'", name, type, domain);
                ario_avahi_remove_host (avahi, name);
                break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
                ARIO_LOG_DBG ("(Browser) %s", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
                break;
        }
}

static void ario_avahi_client_callback (AvahiClient *c,
                                        AvahiClientState state,
                                        AVAHI_GCC_UNUSED void *userdata)
{
        ARIO_LOG_FUNCTION_START;
        g_return_if_fail (c);

        /* Called whenever the client or server state changes */

        if (state == AVAHI_CLIENT_FAILURE) {
                ARIO_LOG_ERROR ("Server connection failure: %s", avahi_strerror (avahi_client_errno (c)));
        }
}

GSList *
ario_avahi_get_hosts (ArioAvahi *avahi)
{
        ARIO_LOG_FUNCTION_START;

        return avahi->priv->hosts;
}
