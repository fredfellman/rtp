//
// Created by olivier on 25/09/15.
//

#ifndef RTP_ZEROCONF_BROWSE_H
#define RTP_ZEROCONF_BROWSE_H
/***
  This file is part of avahi.

  avahi is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  avahi is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
  Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with avahi; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#define zc_browse(X) char *zcb_argv[] = {"avahi-browse", "-a", "-p", "-r", "-t", "-v"}; zc_browse_main(X, 6, zcb_argv);

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <locale.h>
#include <ctype.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/domain.h>
#include <avahi-common/llist.h>
//#include <avahi-common/i18n.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include "zeroconf_queue.h"
#include "zeroconf_common.h"

//#include "sigint.h"

#if defined(HAVE_GDBM) || defined(HAVE_DBM)
#include "stdb.h"
#endif

typedef enum {
    ZCB_COMMAND_HELP,
    ZCB_COMMAND_VERSION,
    ZCB_COMMAND_BROWSE_SERVICES,
    ZCB_COMMAND_BROWSE_ALL_SERVICES,
    ZCB_COMMAND_BROWSE_DOMAINS
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
    , COMMAND_DUMP_STDB
#endif
} zcb_Command;

typedef struct zcb_Config zcb_Config;
typedef struct zcb_ServiceInfo zcb_ServiceInfo;

struct zcb_ServiceInfo {
    AvahiIfIndex interface;
    AvahiProtocol protocol;
    char *name, *type, *domain;

    AvahiServiceResolver *resolver;
    zcb_Config *config;

    AVAHI_LLIST_FIELDS(zcb_ServiceInfo, info);
};

struct zcb_Config {
    int verbose;
    int terminate_on_all_for_now;
    int terminate_on_cache_exhausted;
    char *domain;
    char *stype;
    int ignore_local;
    zcb_Command command;
    int resolve;
    int no_fail;
    int parsable;
    struct zc_queue *result_queue;
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
    int no_db_lookup;
#endif
    AvahiSimplePoll *simple_poll;
    AvahiClient *client;
    int n_all_for_now, n_cache_exhausted, n_resolving;
    AvahiStringList *browsed_types;
    int n_columns;
    int browsing;
    zcb_ServiceInfo *services;

};



void zc_check_terminate(zcb_Config *config);

zcb_ServiceInfo *zc_find_service(zcb_Config *config, AvahiIfIndex interface,
                                       AvahiProtocol protocol, const char *name,
                                       const char *type, const char *domain);

char *zc_make_printable(const char *from, char *to);

void zc_print_service_line(zcb_Config *config, char c, AvahiIfIndex interface, AvahiProtocol protocol, const char *name, const char *type, const char *domain, int nl);

void zc_service_resolver_callback(
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

zcb_ServiceInfo *zc_add_service(zcb_Config *config, AvahiIfIndex interface,
                                   AvahiProtocol protocol, const char *name, const char *type,
                                   const char *domain);

void zc_remove_service(zcb_Config *config, zcb_ServiceInfo *i);

void zc_service_browser_callback(
        AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags,
        void *userdata);

void zc_browse_service_type(zcb_Config *config, const char *stype, const char *domain);

void zc_service_type_browser_callback(
        AvahiServiceTypeBrowser *b,
        AVAHI_GCC_UNUSED AvahiIfIndex interface,
        AVAHI_GCC_UNUSED AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *type,
        const char *domain,
        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
        void *userdata);

void zc_browse_all(zcb_Config *config);

void zc_domain_browser_callback(
        AvahiDomainBrowser *b,
        AVAHI_GCC_UNUSED AvahiIfIndex interface,
        AVAHI_GCC_UNUSED AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *domain,
        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
        void *userdata);

void zc_browse_domains(zcb_Config *config);

int zc_start(zcb_Config *config);

void zc_client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata);

void zc_help(FILE *f, const char *argv0);

int zc_parse_command_line(zcb_Config *config, const char *argv0, int argc, char *argv[]);

int zc_browse_main(struct zc_queue *result_queue, int argc, char *argv[]);

void zc_delete_service_queue(struct zc_queue *result_queue);


#endif //RTP_ZEROCONF_BROWSE_H
