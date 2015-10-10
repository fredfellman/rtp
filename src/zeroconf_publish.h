//
// Created by olivier on 25/09/15.
//

#ifndef RTP_ZEROCONF_PUBLISH_H
#define RTP_ZEROCONF_PUBLISH_H

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <locale.h>
#include <pthread.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/alternative.h>
//#include <avahi-common/i18n.h>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>

//#include "sigint.h"

typedef enum {
    ZCP_COMMAND_UNSPEC,
    ZCP_COMMAND_HELP,
    ZCP_COMMAND_VERSION,
    ZCP_COMMAND_PUBLISH_SERVICE,
    ZCP_COMMAND_PUBLISH_ADDRESS
} zcp_Command;

typedef struct zcp_Config zcp_Config;

struct zc_published_service {
    AvahiClient *client;
    AvahiEntryGroup *entry_group;
    zcp_Config *config;
};


typedef struct zcp_Config {
    int verbose, no_fail, no_reverse;
    zcp_Command command;
    char *name, *stype, *domain, *host;
    uint16_t port;
    AvahiStringList *txt, *subtypes;
    AvahiAddress address;
    int *published;
    AvahiSimplePoll *simple_poll;
    AvahiClient **client;
    struct zc_published_service *service;
} zcp_Config;





int zcp_register_stuff(zcp_Config *config);

void zcp_entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                     void *userdata);

int zcp_register_stuff(zcp_Config *config);

void zcp_client_callback(AvahiClient *c, AvahiClientState state,
                                AVAHI_GCC_UNUSED void *userdata);

void zcp_help(FILE *f, const char *argv0);

int zcp_parse_command_line(zcp_Config *c, const char *argv0, int argc, char *argv[]);

int zc_publish_main(struct zc_published_service *zc_service, int argc, char *argv[]);

void zc_rtp_free_service(struct zc_published_service *zc_service);


#endif //RTP_ZEROCONF_PUBLISH_H
