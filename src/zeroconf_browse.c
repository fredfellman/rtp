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

#include "zeroconf_browse.h"



void zc_check_terminate(zcb_Config *config) {

    assert(config->n_all_for_now >= 0);
    assert(config->n_cache_exhausted >= 0);
    assert(config->n_resolving >= 0);

    if (config->n_all_for_now <= 0 && config->n_resolving <= 0) {

        if (config->verbose && !config->parsable) {
            printf((": All for now\n"));
            config->n_all_for_now++; /* Make sure that this event is not repeated */
        }

        if (config->terminate_on_all_for_now)
            avahi_simple_poll_quit(config->simple_poll);
    }

    if (config->n_cache_exhausted <= 0 && config->n_resolving <= 0) {

        if (config->verbose && !config->parsable) {
            printf((": Cache exhausted\n"));
            config->n_cache_exhausted++; /* Make sure that this event is not repeated */
        }

        if (config->terminate_on_cache_exhausted)
            avahi_simple_poll_quit(config->simple_poll);
    }
}

zcb_ServiceInfo *zc_find_service(zcb_Config *config, AvahiIfIndex interface,
                                       AvahiProtocol protocol, const char *name,
                                       const char *type, const char *domain) {
    zcb_ServiceInfo *i;

    for (i = config->services; i; i = i->info_next)
        if (i->interface == interface &&
            i->protocol == protocol &&
            strcasecmp(i->name, name) == 0 &&
            avahi_domain_equal(i->type, type) &&
            avahi_domain_equal(i->domain, domain))

            return i;

    return NULL;
}

char *zc_make_printable(const char *from, char *to) {
    const char *f;
    char *t;

    for (f = from, t = to; *f; f++, t++)
        *t = isprint(*f) ? *f : '_';

    *t = 0;

    return to;
}

void zc_print_service_line(zcb_Config *config, char c, AvahiIfIndex interface, AvahiProtocol protocol, const char *name, const char *type, const char *domain, int nl) {
    char ifname[IF_NAMESIZE];


#if defined(HAVE_GDBM) || defined(HAVE_DBM)
    if (!config->no_db_lookup)
        type = stdb_lookup(type);
#endif

    if (config->parsable) {
        char sn[AVAHI_DOMAIN_NAME_MAX], *e = sn;
        size_t l = sizeof(sn);

        printf("%c;%s;%s;%s;%s;%s%s",
               c,
               interface != AVAHI_IF_UNSPEC ? if_indextoname(interface, ifname) : ("n/a"),
               protocol != AVAHI_PROTO_UNSPEC ? avahi_proto_to_string(protocol) : ("n/a"),
               avahi_escape_label(name, strlen(name), &e, &l), type, domain, nl ? "\n" : "");

    } else {
        char label[AVAHI_LABEL_MAX];
        zc_make_printable(name, label);

        printf("%c %6s %4s %-*s %-20s %s\n",
               c,
               interface != AVAHI_IF_UNSPEC ? if_indextoname(interface, ifname) : ("n/a"),
               protocol != AVAHI_PROTO_UNSPEC ? avahi_proto_to_string(protocol) : ("n/a"),
               config->n_columns-35, label, type, domain);
    }

    fflush(stdout);
}

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
        void *userdata) {

    zcb_ServiceInfo *i = userdata;

    assert(r);
    assert(i);

    switch (event) {
        case AVAHI_RESOLVER_FOUND: {
            char address[AVAHI_ADDRESS_STR_MAX], *t;
            struct zc_element *zc_elem = malloc(sizeof(struct zc_element));

            avahi_address_snprint(address, sizeof(address), a);

            //t = avahi_string_list_to_string(txt);

            //zc_print_service_line(i->config, '=', interface, protocol, name, type, domain, 0);
            avahi_address_snprint(zc_elem->address, sizeof(zc_elem->address), a);
            zc_elem->hostname = host_name;
            zc_elem->port = port;
            zc_elem->iproto = protocol;
            interface != AVAHI_IF_UNSPEC ? if_indextoname((unsigned int)interface, zc_elem->ifname) : strcpy(zc_elem->ifname, "n/a");
//            zc_elem->type = type;
            zc_elem->type = malloc(AVAHI_LABEL_MAX);
            strcpy(zc_elem->type, type);
            zc_elem->txt_options = avahi_string_list_copy(txt);
            queue_push_back_more(i->config->result_queue, zc_elem,
                                     sizeof(struct zc_element), 0);


//            if (i->config->parsable)
//                printf(";%s;%s;%u;%s\n",
//                       host_name,
//                       address,
//                       port,
//                       t);
//            else
//                printf("   hostname = [%s]\n"
//                               "   address = [%s]\n"
//                               "   port = [%u]\n"
//                               "   txt = [%s]\n",
//                       host_name,
//                       address,
//                       port,
//                       t);

           // avahi_free(t);

            break;
        }

        case AVAHI_RESOLVER_FAILURE:

            fprintf(stderr, ("Failed to resolve service '%s' of type '%s' in domain '%s': %s\n"), name, type, domain, avahi_strerror(avahi_client_errno(i->config->client)));
            break;
    }


    avahi_service_resolver_free(i->resolver);
    i->resolver = NULL;

    assert(i->config->n_resolving > 0);
    i->config->n_resolving--;
    zc_check_terminate(i->config);
    fflush(stdout);
}

zcb_ServiceInfo *zc_add_service(zcb_Config *config, AvahiIfIndex interface,
                                   AvahiProtocol protocol, const char *name, const char *type,
                                   const char *domain) {
    zcb_ServiceInfo *i;

    i = avahi_new(zcb_ServiceInfo, 1);

    if (config->resolve) {
        if (!(i->resolver = avahi_service_resolver_new(config->client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, zc_service_resolver_callback, i))) {
            avahi_free(i);
            fprintf(stderr, ("Failed to resolve service '%s' of type '%s' in domain '%s': %s\n"), name, type, domain, avahi_strerror(avahi_client_errno(config->client)));
            return NULL;
        }

        config->n_resolving++;
    } else
        i->resolver = NULL;

    i->interface = interface;
    i->protocol = protocol;
    i->name = avahi_strdup(name);
    i->type = avahi_strdup(type);
    i->domain = avahi_strdup(domain);
    i->config = config;

    AVAHI_LLIST_PREPEND(zcb_ServiceInfo, info, config->services, i);

    return i;
}

void zc_remove_service(zcb_Config *config, zcb_ServiceInfo *i) {
    assert(config);
    assert(i);

    AVAHI_LLIST_REMOVE(zcb_ServiceInfo, info, config->services, i);

    if (i->resolver)
        avahi_service_resolver_free(i->resolver);

    avahi_free(i->name);
    avahi_free(i->type);
    avahi_free(i->domain);
    avahi_free(i);
}

void zc_service_browser_callback(
        AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags,
        void *userdata) {

    zcb_Config *config = userdata;

    assert(b);
    assert(config);

    switch (event) {
        case AVAHI_BROWSER_NEW: {
            if (config->ignore_local && (flags & AVAHI_LOOKUP_RESULT_LOCAL))
                break;

            if (zc_find_service(config, interface, protocol, name, type, domain))
                return;

            zc_add_service(config, interface, protocol, name, type, domain);

            //zc_print_service_line(config, '+', interface, protocol, name, type, domain, 1);
            break;

        }

        case AVAHI_BROWSER_REMOVE: {
            zcb_ServiceInfo *info;

            if (!(info = zc_find_service(config, interface, protocol, name, type, domain)))
                return;

            zc_remove_service(config, info);

            //zc_print_service_line(config, '-', interface, protocol, name, type, domain, 1);
            break;
        }

        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, ("service_browser failed: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
            avahi_simple_poll_quit(config->simple_poll);
            break;

        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            config->n_cache_exhausted --;
            zc_check_terminate(config);
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
            config->n_all_for_now --;
            zc_check_terminate(config);
            break;
    }
}

void zc_browse_service_type(zcb_Config *config, const char *stype, const char *domain) {
    AvahiServiceBrowser *b;
    AvahiStringList *i;

    assert(config);
    assert(config->client);
    assert(stype);

    for (i = config->browsed_types; i; i = i->next)
        if (avahi_domain_equal(stype, (char*) i->text))
            return;

    if (!(b = avahi_service_browser_new(
            config->client,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            stype,
            domain,
            0,
            zc_service_browser_callback,
            config))) {

        fprintf(stderr, ("avahi_service_browser_new() failed: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
        avahi_simple_poll_quit(config->simple_poll);
    }

    config->browsed_types = avahi_string_list_add(config->browsed_types, stype);

    config->n_all_for_now++;
    config->n_cache_exhausted++;
}

void zc_service_type_browser_callback(
        AvahiServiceTypeBrowser *b,
        AVAHI_GCC_UNUSED AvahiIfIndex interface,
        AVAHI_GCC_UNUSED AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *type,
        const char *domain,
        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
        void *userdata) {

    zcb_Config *config = userdata;

    assert(b);
    assert(config);

    switch (event) {

        case AVAHI_BROWSER_NEW:
            zc_browse_service_type(config, type, domain);
            break;

        case AVAHI_BROWSER_REMOVE:
            /* We're dirty and never remove the browser again */
            break;

        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, ("service_type_browser failed: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
            avahi_simple_poll_quit(config->simple_poll);
            break;

        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            config->n_cache_exhausted --;
            zc_check_terminate(config);
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
            config->n_all_for_now --;
            zc_check_terminate(config);
            break;
    }
}

void zc_browse_all(zcb_Config *config) {
    AvahiServiceTypeBrowser *b;

    assert(config);

    if (!(b = avahi_service_type_browser_new(
            config->client,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            config->domain,
            0,
            zc_service_type_browser_callback,
            config))) {

        fprintf(stderr, ("avahi_service_type_browser_new() failed: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
        avahi_simple_poll_quit(config->simple_poll);
    }

    config->n_cache_exhausted++;
    config->n_all_for_now++;
}

void zc_domain_browser_callback(
        AvahiDomainBrowser *b,
        AVAHI_GCC_UNUSED AvahiIfIndex interface,
        AVAHI_GCC_UNUSED AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *domain,
        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
        void *userdata) {

    zcb_Config *config = userdata;

    assert(b);
    assert(config);

    switch (event) {

        case AVAHI_BROWSER_NEW:
        case AVAHI_BROWSER_REMOVE: {
            char ifname[IF_NAMESIZE];

            if (config->parsable)
                printf("%c;%s;%s;%s\n",
                       event == AVAHI_BROWSER_NEW ? '+' : '-',
                       interface != AVAHI_IF_UNSPEC ? if_indextoname(interface, ifname) : "",
                       protocol != AVAHI_PROTO_UNSPEC ? avahi_proto_to_string(protocol) : "",
                       domain);
            else
                printf("%c %4s %4s %s\n",
                       event == AVAHI_BROWSER_NEW ? '+' : '-',
                       interface != AVAHI_IF_UNSPEC ? if_indextoname(interface, ifname) : "n/a",
                       protocol != AVAHI_PROTO_UNSPEC ? avahi_proto_to_string(protocol) : "n/a",
                       domain);
            break;
        }

        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, ("domain_browser failed: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
            avahi_simple_poll_quit(config->simple_poll);
            break;

        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            config->n_cache_exhausted --;
            zc_check_terminate(config);
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
            config->n_all_for_now --;
            zc_check_terminate(config);
            break;
    }
}

void zc_browse_domains(zcb_Config *config) {
    AvahiDomainBrowser *b;

    assert(config);

    if (!(b = avahi_domain_browser_new(
            config->client,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            config->domain,
            AVAHI_DOMAIN_BROWSER_BROWSE,
            0,
            zc_domain_browser_callback,
            config))) {

        fprintf(stderr, ("avahi_domain_browser_new() failed: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
        avahi_simple_poll_quit(config->simple_poll);
    }

    config->n_cache_exhausted++;
    config->n_all_for_now++;
}

int zc_start(zcb_Config *config) {

    assert(!config->browsing);

    if (config->verbose && !config->parsable) {
        const char *version, *hn;

        if (!(version = avahi_client_get_version_string(config->client))) {
            fprintf(stderr, ("Failed to query version string: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
            return -1;
        }

        if (!(hn = avahi_client_get_host_name_fqdn(config->client))) {
            fprintf(stderr, ("Failed to query host name: %s\n"), avahi_strerror(avahi_client_errno(config->client)));
            return -1;
        }

        fprintf(stderr, ("Server version: %s; Host name: %s\n"), version, hn);

        if (config->command == ZCB_COMMAND_BROWSE_DOMAINS) {
            /* Translators: This is a column heading with abbreviations for
             *   Event (+/-), Network Interface, Protocol (IPv4/v6), Domain */
            fprintf(stderr, ("E Ifce Prot Domain\n"));
        } else {
            /* Translators: This is a column heading with abbreviations for
             *   Event (+/-), Network Interface, Protocol (IPv4/v6), Domain */
            fprintf(stderr, ("E Ifce Prot %-*s %-20s Domain\n"), config->n_columns-35, ("Name"), ("Type"));
        }
    }

    if (config->command == ZCB_COMMAND_BROWSE_SERVICES)
        zc_browse_service_type(config, config->stype, config->domain);
    else if (config->command == ZCB_COMMAND_BROWSE_ALL_SERVICES)
        zc_browse_all(config);
    else {
        assert(config->command == ZCB_COMMAND_BROWSE_DOMAINS);
        zc_browse_domains(config);
    }

    config->browsing = 1;
    return 0;
}

void zc_client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    zcb_Config *config = userdata;

    /* This function might be called when avahi_client_new() has not
     * returned yet.*/
    config->client = c;

    switch (state) {
        case AVAHI_CLIENT_FAILURE:

            if (config->no_fail && avahi_client_errno(c) == AVAHI_ERR_DISCONNECTED) {
                int error;

                /* We have been disconnected, so let reconnect */

                fprintf(stderr, ("Disconnected, reconnecting ...\n"));

                avahi_client_free(config->client);
                config->client = NULL;

                avahi_string_list_free(config->browsed_types);
                config->browsed_types = NULL;

                while (config->services)
                    zc_remove_service(config, config->services);

                config->browsing = 0;

                if (!(config->client = avahi_client_new(avahi_simple_poll_get(config->simple_poll), AVAHI_CLIENT_NO_FAIL, zc_client_callback, config, &error))) {
                    fprintf(stderr, ("Failed to create client object: %s\n"), avahi_strerror(error));
                    avahi_simple_poll_quit(config->simple_poll);
                }

            } else {
                fprintf(stderr, ("Client failure, exiting: %s\n"), avahi_strerror(avahi_client_errno(c)));
                avahi_simple_poll_quit(config->simple_poll);
            }

            break;

        case AVAHI_CLIENT_S_REGISTERING:
        case AVAHI_CLIENT_S_RUNNING:
        case AVAHI_CLIENT_S_COLLISION:

            if (!config->browsing)
            if (zc_start(config) < 0)
                avahi_simple_poll_quit(config->simple_poll);

            break;

        case AVAHI_CLIENT_CONNECTING:

            if (config->verbose && !config->parsable)
                fprintf(stderr, ("Waiting for daemon ...\n"));

            break;
    }
}

void zc_help(FILE *f, const char *argv0) {
    if (strstr(argv0, "domain"))
        fprintf(f, "%s [options] \n\n", argv0);
    else
        fprintf(f,
                "%s [options] <service type>\n"
                        "%s [options] -a\n"
                        "%s [options] -D\n"
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
                "%s [options] -b\n"
#endif
                        "\n",
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
                argv0,
#endif
                argv0, argv0, argv0);

    fprintf(f, "%s%s",
            ("    -h --help            Show this help\n"
                      "    -V --version         Show version\n"
                      "    -D --browse-domains  Browse for browsing domains instead of services\n"
                      "    -a --all             Show all services, regardless of the type\n"
                      "    -d --domain=DOMAIN   The domain to browse in\n"
                      "    -v --verbose         Enable verbose mode\n"
                      "    -t --terminate       Terminate after dumping a more or less complete list\n"
                      "    -c --cache           Terminate after dumping all entries from the cache\n"
                      "    -l --ignore-local    Ignore local services\n"
                      "    -r --resolve         Resolve services found\n"
                      "    -f --no-fail         Don't fail if the daemon is not available\n"
                      "    -p --parsable        Output in parsable format\n"),
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
            ("    -k --no-db-lookup    Don't lookup service types\n"
              "    -b --dump-db         Dump service type database\n")
#else
            ""
#endif
    );
}

int zc_parse_command_line(zcb_Config *config, const char *argv0, int argc, char *argv[]) {
    int o;
    optind = 0;

    const struct option long_options[] = {
            { "help",           no_argument,       NULL, 'h' },
            { "version",        no_argument,       NULL, 'V' },
            { "browse-domains", no_argument,       NULL, 'D' },
            { "domain",         required_argument, NULL, 'd' },
            { "all",            no_argument,       NULL, 'a' },
            { "verbose",        no_argument,       NULL, 'v' },
            { "terminate",      no_argument,       NULL, 't' },
            { "cache",          no_argument,       NULL, 'c' },
            { "ignore-local",   no_argument,       NULL, 'l' },
            { "resolve",        no_argument,       NULL, 'r' },
            { "no-fail",        no_argument,       NULL, 'f' },
            { "parsable",      no_argument,       NULL, 'p' },
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
        { "no-db-lookup",   no_argument,       NULL, 'k' },
        { "dump-db",        no_argument,       NULL, 'b' },
#endif
            { NULL, 0, NULL, 0 }
    };

    assert(config);

    config->command = strstr(argv0, "domain") ? ZCB_COMMAND_BROWSE_DOMAINS : ZCB_COMMAND_BROWSE_SERVICES;
    config->verbose =
    config->terminate_on_cache_exhausted =
    config->terminate_on_all_for_now =
    config->ignore_local =
    config->resolve =
    config->no_fail =
    config->parsable = 0;
    config->domain = config->stype = NULL;

#if defined(HAVE_GDBM) || defined(HAVE_DBM)
    c->no_db_lookup = 0;
#endif

    while ((o = getopt_long(argc, argv, "hVd:avtclrDfp"
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
                            "kb"
#endif
            , long_options, NULL)) >= 0) {

        switch(o) {
            case 'h':
                config->command = ZCB_COMMAND_HELP;
                break;
            case 'V':
                config->command = ZCB_COMMAND_VERSION;
                break;
            case 'a':
                config->command = ZCB_COMMAND_BROWSE_ALL_SERVICES;
                break;
            case 'D':
                config->command = ZCB_COMMAND_BROWSE_DOMAINS;
                break;
            case 'd':
                avahi_free(config->domain);
                config->domain = avahi_strdup(optarg);
                break;
            case 'v':
                config->verbose = 1;
                break;
            case 't':
                config->terminate_on_all_for_now = 1;
                break;
            case 'c':
                config->terminate_on_cache_exhausted = 1;
                break;
            case 'l':
                config->ignore_local = 1;
                break;
            case 'r':
                config->resolve = 1;
                break;
            case 'f':
                config->no_fail = 1;
                break;
            case 'p':
                config->parsable = 1;
                break;
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
            case 'k':
                c->no_db_lookup = 1;
                break;
            case 'b':
                c->command = COMMAND_DUMP_STDB;
                break;
#endif
            default:
                return -1;
        }
    }

    if (config->command == ZCB_COMMAND_BROWSE_SERVICES) {
        if (optind >= argc) {
            fprintf(stderr, ("Plop Too few arguments\n"));
            return -1;
        }

        config->stype = avahi_strdup(argv[optind]);
        optind++;
    }

    if (optind < argc) {
        fprintf(stderr, ("Too many arguments\n"));
        return -1;
    }

    return 0;
}

int zc_browse_main(struct zc_queue *result_queue, int argc, char *argv[]) {
    int ret = 1, error;
    zcb_Config config;
    config.result_queue = result_queue;
    config.simple_poll = NULL;
    config.client = NULL;
    config.n_all_for_now = 0;
    config.n_cache_exhausted = 0;
    config.n_resolving = 0;
    config.browsed_types = NULL;
    config.n_columns = 80;
    config.browsing = 0;
    config.services = NULL;
    const char *argv0;
    char *ec;

    //avahi_init_i18n();
    //setlocale(LC_ALL, "");

    if ((argv0 = strrchr(argv[0], '/')))
        argv0++;
    else
        argv0 = argv[0];

    if ((ec = getenv("COLUMNS")))
        config.n_columns = atoi(ec);

    if (config.n_columns < 40)
        config.n_columns = 40;

    if (zc_parse_command_line(&config, argv0, argc, argv) < 0)
        goto fail;

    switch (config.command) {
        case ZCB_COMMAND_HELP:
            zc_help(stdout, argv0);
            ret = 0;
            break;

        case ZCB_COMMAND_VERSION:
            printf("%s ? \n", argv0);
            ret = 0;
            break;

        case ZCB_COMMAND_BROWSE_SERVICES:
        case ZCB_COMMAND_BROWSE_ALL_SERVICES:
        case ZCB_COMMAND_BROWSE_DOMAINS:

            if (!(config.simple_poll = avahi_simple_poll_new())) {
                fprintf(stderr, ("Failed to create simple poll object.\n"));
                goto fail;
            }

//            if (sigint_install(simple_poll) < 0)
//                goto fail;

            if (!(config.client = avahi_client_new(avahi_simple_poll_get(config.simple_poll), config.no_fail ? AVAHI_CLIENT_NO_FAIL : 0, zc_client_callback, &config, &error))) {
                fprintf(stderr, ("Failed to create client object: %s\n"), avahi_strerror(error));
                goto fail;
            }

            avahi_simple_poll_loop(config.simple_poll);
            avahi_simple_poll_quit(config.simple_poll);
//            int simple_poll_ret = 0;
//            while(1){
//                simple_poll_ret = avahi_simple_poll_iterate(config.simple_poll, 100000);
//                if (simple_poll_ret == 1){
//                    fprintf(stderr, ("simple poll quit \n"));
//                    break;
//                }
//                if(simple_poll_ret == 0){
//                    fprintf(stderr, ("simple poll event.. \n"));
//                    break;
//                }
//            }
//
//            ret = 0;
//            break;
            ret = 0;
            break;

#if defined(HAVE_GDBM) || defined(HAVE_DBM)
        case COMMAND_DUMP_STDB: {
            char *t;
            stdb_setent();

            while ((t = stdb_getent())) {
                if (config.no_db_lookup)
                    printf("%s\n", t);
                else
                    printf("%s\n", stdb_lookup(t));
            }

            ret = 0;
            break;
        }
#endif
    }

//    struct zc_element *elem = NULL;
//    struct zc_queue_elem *q_elem = NULL;
//    while(result_queue->first != NULL){
//        elem = (struct zc_element *)queue_pop_front(result_queue);
//        if(strcmp(elem->type,"_rtp._tcp") == 0){  // It's a RTP element
//            fprintf(stdout, "Delete RTP service elem ..");
//        }
//        free(elem->txt_options);
//        free(elem->type);
//        free(elem);
//    }

    fail:

    while (config.services)
        zc_remove_service(&config, config.services);

    if (config.client != NULL)
        avahi_client_free(config.client);

//    sigint_uninstall();

    if (config.simple_poll != NULL)
        avahi_simple_poll_free(config.simple_poll);

    avahi_free(config.domain);
    avahi_free(config.stype);

    avahi_string_list_free(config.browsed_types);

//    queue_cleanup(result_queue);


#if defined(HAVE_GDBM) || defined(HAVE_DBM)
    stdb_shutdown();
#endif

    return ret;
}

void zc_delete_service_queue(struct zc_queue *result_queue) {
    struct zc_element *elem = NULL;
    struct zc_queue_elem *q_elem = NULL;
    while(result_queue->first != NULL){
        elem = (struct zc_element *)queue_pop_front(result_queue);
        zc_free_zc_element(elem);
    }

}
