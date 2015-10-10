//
// Created by olivier on 11/09/15.
//

#ifndef RTP_ZEROCONF_H
#define RTP_ZEROCONF_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// IF_NAMESIZE
#include <net/if.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/domain.h>
#include <avahi-common/llist.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#define NO_FAIL 1
#define VERBOSE 0
#define _PARSE 1
#define SERVICE_TYPE "_rtp._tcp"
#define IGNORE_LOCAL 0
#define RESOLVE 1
#define TERMINATE_CACHE 0
#define TERMINATE_ALL_FOR_NOW 1

typedef struct ServiceInfo ServiceInfo;
struct ServiceInfo {
    AvahiIfIndex interface;
    AvahiProtocol protocol;
    char *name, *type, *domain;

    AvahiServiceResolver *resolver;

    AVAHI_LLIST_FIELDS(ServiceInfo, info);
};

void zc_browse_print_service_line(char c, AvahiIfIndex interface, AvahiProtocol protocol,
                                  const char *name, const char *type, const char *domain,
                                  int nl);

void zc_browse_check_terminate();

void zc_browse_service_resolver_callback(
        AvahiServiceResolver *r,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiResolverEvent event,
        const char *name,
        const char *type,
        const char *domain,
        const char *host_name,
        const AvahiAddress *a,
        uint16_t port,
        AvahiStringList *txt,
        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
        void *userdata);

ServiceInfo *zc_browse_add_service(AvahiIfIndex interface, AvahiProtocol protocol,
                                   const char *name, const char *type, const char *domain);

void zc_browse_remove_service(ServiceInfo *i);

ServiceInfo *zc_browse_find_service(AvahiIfIndex interface, AvahiProtocol protocol,
                                    const char *name, const char *type, const char *domain);

void zc_browse_service_browser_callback(
        AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags,
        void *userdata);

void zc_browse_browse_service_type(const char *stype, const char *domain);

int zc_browse_start();

void zc_browse_client_callback(AvahiClient *c, AvahiClientState state,
                                      AVAHI_GCC_UNUSED void *userdata);

int zc_browse_main();

#endif //RTP_ZEROCONF_H
