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

#include "zeroconf_publish.h"
#include <unistd.h>


void zcp_entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                     void *userdata) {
    zcp_Config *config = userdata;

    assert(g);
    assert(config);

    switch (state) {

        case AVAHI_ENTRY_GROUP_ESTABLISHED:

            fprintf(stderr, ("Established under name '%s'\n"), config->name);
            *config->published = 1;
//            config->service->name = config->name;
//            config->service->type = config->stype;
//            config->service->domain = config->domain;
            break;

        case AVAHI_ENTRY_GROUP_FAILURE:

            fprintf(stderr, ("Failed to register: %s\n"), avahi_strerror(avahi_client_errno(*(config->client))));
            break;

        case AVAHI_ENTRY_GROUP_COLLISION: {
            char *n;

            if (config->command == ZCP_COMMAND_PUBLISH_SERVICE)
                n = avahi_alternative_service_name(config->name);
            else {
                assert(config->command == ZCP_COMMAND_PUBLISH_ADDRESS);
                n = avahi_alternative_host_name(config->name);
            }

            fprintf(stderr, ("Name collision, picking new name '%s'.\n"), n);
            avahi_free(config->name);
            config->name = n;

            zcp_register_stuff(config);

            break;
        }

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

int zcp_register_stuff(zcp_Config *config) {
    assert(config);

    if (!config->service->entry_group) {
        if (!(config->service->entry_group = avahi_entry_group_new(*(config->client), zcp_entry_group_callback, config))) {
            fprintf(stderr, ("Failed to create entry group: %s\n"), avahi_strerror(avahi_client_errno(*(config->client))));
            return -1;
        }
    }

    assert(avahi_entry_group_is_empty(config->service->entry_group));

    if (config->command == ZCP_COMMAND_PUBLISH_ADDRESS) {

        if (avahi_entry_group_add_address(config->service->entry_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, config->no_reverse ? AVAHI_PUBLISH_NO_REVERSE : 0, config->name, &config->address) < 0) {
            fprintf(stderr, ("Failed to add address: %s\n"), avahi_strerror(avahi_client_errno(*(config->client))));
            return -1;
        }

    } else {
        AvahiStringList *i;

        assert(config->command == ZCP_COMMAND_PUBLISH_SERVICE);

        if (avahi_entry_group_add_service_strlst(config->service->entry_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, config->name, config->stype, config->domain, config->host, config->port, config->txt) < 0) {
            fprintf(stderr, ("Failed to add service: %s\n"), avahi_strerror(avahi_client_errno(*(config->client))));
            return -1;
        }

//        config->service->interface = AVAHI_IF_UNSPEC;
//        config->service->protocol = AVAHI_PROTO_UNSPEC;
//        config->service->flags = 0;

        for (i = config->subtypes; i; i = i->next)
            if (avahi_entry_group_add_service_subtype(config->service->entry_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, config->name, config->stype, config->domain, (char*) i->text) < 0) {
                fprintf(stderr, ("Failed to add subtype '%s': %s\n"), i->text, avahi_strerror(avahi_client_errno(*(config->client))));
                return -1;
            }
    }

    avahi_entry_group_commit(config->service->entry_group);
//    config->txt = avahi_string_list_add(NULL, "plop=ok");
//    avahi_entry_group_update_service_txt_strlst(config->service->entry_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, config->name, config->stype, config->domain, config->txt);

    return 0;
}

void zcp_client_callback(AvahiClient *c, AvahiClientState state,
                                AVAHI_GCC_UNUSED void *userdata) {
    zcp_Config *config = userdata;

    *(config->client) = c;

    switch (state) {
        case AVAHI_CLIENT_FAILURE:

            if (config->no_fail && avahi_client_errno(c) == AVAHI_ERR_DISCONNECTED) {
                int error;

                /* We have been disconnected, so let reconnect */

                fprintf(stderr, ("Disconnected, reconnecting ...\n"));

                avahi_client_free(*(config->client));
                *(config->client) = NULL;
                config->service->entry_group = NULL;

                if (!(*(config->client) = avahi_client_new(avahi_simple_poll_get((config->simple_poll)),
                                                AVAHI_CLIENT_NO_FAIL, zcp_client_callback,
                                                config, &error))) {
                    fprintf(stderr, ("Failed to create client object: %s\n"), avahi_strerror(error));
                    avahi_simple_poll_quit((config->simple_poll));
                }

            } else {
                fprintf(stderr, ("Client failure, exiting: %s\n"), avahi_strerror(avahi_client_errno(c)));
                avahi_simple_poll_quit((config->simple_poll));
            }

            break;

        case AVAHI_CLIENT_S_RUNNING:

            if (zcp_register_stuff(config) < 0)
                avahi_simple_poll_quit((config->simple_poll));
//            else
//                fprintf(stderr, ("Done ... \n"));

            break;

        case AVAHI_CLIENT_S_COLLISION:

            if (config->verbose)
                fprintf(stderr, ("Host name conflict\n"));

            /* Fall through */

        case AVAHI_CLIENT_S_REGISTERING:

            if (config->service->entry_group) {
                avahi_entry_group_free(config->service->entry_group);
                config->service->entry_group = NULL;
            }
            break;

        case AVAHI_CLIENT_CONNECTING:

            if (config->verbose)
                fprintf(stderr, ("Waiting for daemon ...\n"));

            break;

            ;
    }
}

void zcp_help(FILE *f, const char *argv0) {
    fprintf(f,
            ("%s [options] %s <name> <type> <port> [<txt ...>]\n"
                      "%s [options] %s <host-name> <address>\n\n"
                      "    -h --help            Show this help\n"
                      "    -V --version         Show version\n"
                      "    -s --service         Publish service\n"
                      "    -a --address         Publish address\n"
                      "    -v --verbose         Enable verbose mode\n"
                      "    -d --domain=DOMAIN   Domain to publish service in\n"
                      "    -H --host=DOMAIN     Host where service resides\n"
                      "       --subtype=SUBTYPE An additional subtype to register this service with\n"
                      "    -R --no-reverse      Do not publish reverse entry with address\n"
                      "    -f --no-fail         Don't fail if the daemon is not available\n"),
            argv0, strstr(argv0, "service") ? "[-s]" : "-s",
            argv0, strstr(argv0, "address") ? "[-a]" : "-a");
}

int zcp_parse_command_line(zcp_Config *c, const char *argv0, int argc, char *argv[]) {
    optind = 0;
    int o;

    enum {
        ARG_SUBTYPE = 256
    };

    const struct option long_options[] = {
            { "help",           no_argument,       NULL, 'h' },
            { "version",        no_argument,       NULL, 'V' },
            { "service",        no_argument,       NULL, 's' },
            { "address",        no_argument,       NULL, 'a' },
            { "verbose",        no_argument,       NULL, 'v' },
            { "domain",         required_argument, NULL, 'd' },
            { "host",           required_argument, NULL, 'H' },
            { "subtype",        required_argument, NULL, ARG_SUBTYPE},
            { "no-reverse",     no_argument,       NULL, 'R' },
            { "no-fail",        no_argument,       NULL, 'f' },
            { NULL, 0, NULL, 0 }
    };

    assert(c);

    c->command = strstr(argv0, "address") ? ZCP_COMMAND_PUBLISH_ADDRESS : (strstr(argv0, "service") ? ZCP_COMMAND_PUBLISH_SERVICE
                                                                                                : ZCP_COMMAND_UNSPEC);
    c->verbose = c->no_fail = c->no_reverse = 0;
    c->host = c->name = c->domain = c->stype = NULL;
    c->port = 0;
    c->txt = c->subtypes = NULL;

    while ((o = getopt_long(argc, argv, "hVsavRd:H:f", long_options, NULL)) >= 0) {

        switch(o) {
            case 'h':
                c->command = ZCP_COMMAND_HELP;
                break;
            case 'V':
                c->command = ZCP_COMMAND_VERSION;
                break;
            case 's':
                c->command = ZCP_COMMAND_PUBLISH_SERVICE;
                break;
            case 'a':
                c->command = ZCP_COMMAND_PUBLISH_ADDRESS;
                break;
            case 'v':
                c->verbose = 1;
                break;
            case 'R':
                c->no_reverse = 1;
                break;
            case 'd':
                avahi_free(c->domain);
                c->domain = avahi_strdup(optarg);
                break;
            case 'H':
                avahi_free(c->host);
                c->host = avahi_strdup(optarg);
                break;
            case 'f':
                c->no_fail = 1;
                break;
            case ARG_SUBTYPE:
                c->subtypes = avahi_string_list_add(c->subtypes, optarg);
                break;
            default:
                return -1;
        }
    }

    if (c->command == ZCP_COMMAND_PUBLISH_ADDRESS) {
        if (optind+2 !=  argc) {
            fprintf(stderr, ("Bad number of arguments\n"));
            return -1;
        }

        avahi_free(c->name);
        c->name = avahi_strdup(argv[optind]);
        avahi_address_parse(argv[optind+1], AVAHI_PROTO_UNSPEC, &c->address);

    } else if (c->command == ZCP_COMMAND_PUBLISH_SERVICE) {

        char *e;
        long int p;
        int i;

        if (optind+3 > argc) {
            fprintf(stderr, ("Bad number of arguments\n"));
            return -1;
        }

        c->name = avahi_strdup(argv[optind]);
        c->stype = avahi_strdup(argv[optind+1]);

        errno = 0;
        p = strtol(argv[optind+2], &e, 0);

        if (errno != 0 || *e || p < 0 || p > 0xFFFF) {
            fprintf(stderr, ("Failed to parse port number: %s\n"), argv[optind+2]);
            return -1;
        }

        c->port = p;

        for (i = optind+3; i < argc; i++)
            c->txt = avahi_string_list_add(c->txt, argv[i]);
    }

    return 0;
}

int zc_publish_main(struct zc_published_service *zc_service, int argc, char *argv[]) {
    int ret = 1, error;
    int pub = 0;
    zc_service->config = malloc(sizeof(zcp_Config));
    zc_service->config->simple_poll = NULL;
    zc_service->config->client = &zc_service->client;
    zc_service->config->service = zc_service;
    zc_service->config->published = &pub;
    const char *argv0;

    //avahi_init_i18n();
    //setlocale(LC_ALL, "");

    if ((argv0 = strrchr(argv[0], '/')))
        argv0++;
    else
        argv0 = argv[0];

    if (zcp_parse_command_line(zc_service->config, argv0, argc, argv) < 0)
        goto fail;

    switch (zc_service->config->command) {
        case ZCP_COMMAND_UNSPEC:
            ret = 1;
            fprintf(stderr, ("No command specified.\n"));
            break;

        case ZCP_COMMAND_HELP:
            zcp_help(stdout, argv0);
            ret = 0;
            break;

        case ZCP_COMMAND_VERSION:
            printf("%s ? \n", argv0);
            ret = 0;
            break;

        case ZCP_COMMAND_PUBLISH_SERVICE:
        case ZCP_COMMAND_PUBLISH_ADDRESS:

            if (!((zc_service->config->simple_poll) = avahi_simple_poll_new())) {
                fprintf(stderr, ("Failed to create simple poll object.\n"));
                goto fail;
            }

//            if (sigint_install(simple_poll) < 0)
//                goto fail;

            if (!((zc_service->client) = avahi_client_new(avahi_simple_poll_get((zc_service->config->simple_poll)),
                                            zc_service->config->no_fail ? AVAHI_CLIENT_NO_FAIL : 0,
                                            zcp_client_callback, zc_service->config, &error))) {
                fprintf(stderr, ("Failed to create client object: %s\n"), avahi_strerror(error));
                goto fail;
            }

            if (avahi_client_get_state(*(zc_service->config->client)) != AVAHI_CLIENT_CONNECTING && zc_service->config->verbose) {
                const char *version, *hn;

                if (!(version = avahi_client_get_version_string(*(zc_service->config->client)))) {
                    fprintf(stderr, ("Failed to query version string: %s\n"), avahi_strerror(avahi_client_errno(*(zc_service->config->client))));
                    goto fail;
                }

                if (!(hn = avahi_client_get_host_name_fqdn(*(zc_service->config->client)))) {
                    fprintf(stderr, ("Failed to query host name: %s\n"), avahi_strerror(avahi_client_errno(*(zc_service->config->client))));
                    goto fail;
                }

                fprintf(stderr, ("Server version: %s; Host name: %s\n"), version, hn);
            }
            int simple_poll_ret = 0;
            while(1){
                simple_poll_ret = avahi_simple_poll_iterate((zc_service->config->simple_poll), 100000);
                if (simple_poll_ret == 1){
                    fprintf(stderr, ("simple poll quit \n"));
                    break;
                }
                if(simple_poll_ret == 0){
//                    fprintf(stderr, ("simple poll event.. \n"));
                    if(*zc_service->config->published == 1){
//                        fprintf(stderr, ("simple poll finish ! \n"));
                        break;
                    }
                }
            }

            ret = 0;
            break;
    }

    //sleep(1);


    fail:
//    pthread_mutex_lock(&mutex);
//    pthread_cond_signal(&condition);
//    pthread_mutex_unlock(&mutex);

//    if (zc_service->client != NULL)
//        fprintf(stderr, "Clean client..");
//        avahi_client_free((zc_service->client));
//
////    sigint_uninstall();
//
//    if (zc_service->config->simple_poll != NULL)
//        fprintf(stderr, "Clean simple poll..");
//        avahi_simple_poll_free((zc_service->config->simple_poll));
//
//    avahi_free(zc_service->config->host);
//    avahi_free(zc_service->config->name);
//    avahi_free(zc_service->config->stype);
//    avahi_free(zc_service->config->domain);
//    avahi_string_list_free(zc_service->config->subtypes);
//    avahi_string_list_free(zc_service->config->txt);

    return ret;
}

void zc_rtp_free_service(struct zc_published_service *zc_service) {
    if(zc_service == NULL)
        return;

    //avahi_client_free(*zc_service->config->client);
//    avahi_entry_group_free(zc_service->entry_group);
//    avahi_client_free(zc_service->client);
//    avahi_simple_poll_free(zc_service->config->simple_poll);
//    avahi_free(zc_service->config->host);
//    avahi_free(zc_service->config->name);
//    avahi_free(zc_service->config->stype);
//    avahi_free(zc_service->config->domain);
//    avahi_string_list_free(zc_service->config->subtypes);
//    avahi_string_list_free(zc_service->config->txt);

    if (zc_service->client != NULL) {
        //fprintf(stderr, "Clean client..");
        avahi_client_free((zc_service->client));
    }

    if (zc_service->config->simple_poll != NULL) {
        //fprintf(stderr, "Clean simple poll..");
        avahi_simple_poll_free((zc_service->config->simple_poll));
    }

    avahi_free(zc_service->config->host);
    avahi_free(zc_service->config->name);
    avahi_free(zc_service->config->stype);
    avahi_free(zc_service->config->domain);
    avahi_string_list_free(zc_service->config->subtypes);
    avahi_string_list_free(zc_service->config->txt);


    free(zc_service->config);
    free(zc_service);
}
