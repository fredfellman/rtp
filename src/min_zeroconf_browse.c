//
// Created by olivier on 11/09/15.
//

#include "min_zeroconf_browse.h"

static AvahiSimplePoll *simple_poll = NULL;
static AvahiClient *client = NULL;
static int n_all_for_now = 0, n_cache_exhausted = 0, n_resolving = 0;
static AvahiStringList *browsed_types = NULL;
static ServiceInfo *services = NULL;
static int n_columns = 80;
static int browsing = 0;

void zc_browse_print_service_line(char c, AvahiIfIndex interface, AvahiProtocol protocol,
                                  const char *name, const char *type, const char *domain,
                                  int nl) {
    char ifname[IF_NAMESIZE];


    if (_PARSE) {
        char sn[AVAHI_DOMAIN_NAME_MAX], *e = sn;
        size_t l = sizeof(sn);

        printf("%c;%s;%s;%s;%s;%s%s",
               c,
               interface != AVAHI_IF_UNSPEC ? if_indextoname(interface, ifname) : "n/a",
               protocol != AVAHI_PROTO_UNSPEC ? avahi_proto_to_string(protocol) : "n/a",
               avahi_escape_label(name, strlen(name), &e, &l), type, domain, nl ? "\n" : "");

    } else {
        char label[AVAHI_LABEL_MAX];
        //make_printable(name, label);

        printf("%c %6s %4s %-*s %-20s %s\n",
               c,
               interface != AVAHI_IF_UNSPEC ? if_indextoname(interface, ifname) : "n/a",
               protocol != AVAHI_PROTO_UNSPEC ? avahi_proto_to_string(protocol) : "n/a",
               n_columns-35, label, type, domain);
    }

    fflush(stdout);
}



void zc_browse_check_terminate() {

    assert(n_all_for_now >= 0);
    assert(n_cache_exhausted >= 0);
    assert(n_resolving >= 0);

    if (n_all_for_now <= 0 && n_resolving <= 0) {

        if (VERBOSE) {
            printf(": All for now\n");
            n_all_for_now++; /* Make sure that this event is not repeated */
        }

        if(TERMINATE_ALL_FOR_NOW)
            avahi_simple_poll_quit(simple_poll);
    }

    if (n_cache_exhausted <= 0 && n_resolving <= 0) {

        if (VERBOSE) {
            printf(": Cache exhausted\n");
            n_cache_exhausted++; /* Make sure that this event is not repeated */
        }

        if (TERMINATE_CACHE)
            avahi_simple_poll_quit(simple_poll);
    }
}

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
        void *userdata) {

    ServiceInfo *i = userdata;

    assert(r);
    assert(i);

    switch (event) {
        case AVAHI_RESOLVER_FOUND: {
            char address[AVAHI_ADDRESS_STR_MAX], *t;

            avahi_address_snprint(address, sizeof(address), a);

            t = avahi_string_list_to_string(txt);

            zc_browse_print_service_line('=', interface, protocol, name, type, domain, 0);

            if (_PARSE)
                printf(";%s;%s;%u;%s\n",
                       host_name,
                       address,
                       port,
                       t);
            else
                printf("   hostname = [%s]\n"
                               "   address = [%s]\n"
                               "   port = [%u]\n"
                               "   txt = [%s]\n",
                       host_name,
                       address,
                       port,
                       t);

            avahi_free(t);

            break;
        }

        case AVAHI_RESOLVER_FAILURE:

            fprintf(stderr, "Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(client)));
            break;
    }


    avahi_service_resolver_free(i->resolver);
    i->resolver = NULL;

    assert(n_resolving > 0);
    n_resolving--;
    zc_browse_check_terminate();
    fflush(stdout);
}

ServiceInfo *zc_browse_add_service(AvahiIfIndex interface, AvahiProtocol protocol,
                                   const char *name, const char *type, const char *domain) {
    ServiceInfo *i;

    i = avahi_new(ServiceInfo, 1);

    if (RESOLVE) {
        if (!(i->resolver = avahi_service_resolver_new(client, interface, protocol, name, type,
                                                       domain, AVAHI_PROTO_UNSPEC, 0,
                                                       zc_browse_service_resolver_callback, i))) {
            avahi_free(i);
            fprintf(stderr, "Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(client)));
            return NULL;
        }

        n_resolving++;
    } else
        i->resolver = NULL;

    i->interface = interface;
    i->protocol = protocol;
    i->name = avahi_strdup(name);
    i->type = avahi_strdup(type);
    i->domain = avahi_strdup(domain);

    AVAHI_LLIST_PREPEND(ServiceInfo, info, services, i);

    return i;
}

void zc_browse_remove_service(ServiceInfo *i) {
    assert(i);

    AVAHI_LLIST_REMOVE(ServiceInfo, info, services, i);

    if (i->resolver)
        avahi_service_resolver_free(i->resolver);

    avahi_free(i->name);
    avahi_free(i->type);
    avahi_free(i->domain);
    avahi_free(i);
}

ServiceInfo *zc_browse_find_service(AvahiIfIndex interface, AvahiProtocol protocol,
                                    const char *name, const char *type, const char *domain) {
    ServiceInfo *i;

    for (i = services; i; i = i->info_next)
        if (i->interface == interface &&
            i->protocol == protocol &&
            strcasecmp(i->name, name) == 0 &&
            avahi_domain_equal(i->type, type) &&
            avahi_domain_equal(i->domain, domain))

            return i;

    return NULL;
}

void zc_browse_service_browser_callback(
        AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags,
        void *userdata) {

    //Config *c = userdata;

    assert(b);
    //assert(c);

    switch (event) {
        case AVAHI_BROWSER_NEW: {
            if (IGNORE_LOCAL && (flags & AVAHI_LOOKUP_RESULT_LOCAL))
                break;

            if (zc_browse_find_service(interface, protocol, name, type, domain))
                return;

            zc_browse_add_service(interface, protocol, name, type, domain);

            //zc_browse_print_service_line('+', interface, protocol, name, type, domain, 1);
            break;

        }

        case AVAHI_BROWSER_REMOVE: {
            ServiceInfo *info;

            if (!(info = zc_browse_find_service(interface, protocol, name, type, domain)))
                return;

            zc_browse_remove_service(info);

            //zc_browse_print_service_line(c, '-', interface, protocol, name, type, domain, 1);
            break;
        }

        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, "service_browser failed: %s\n", avahi_strerror(avahi_client_errno(client)));
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            n_cache_exhausted --;
            zc_browse_check_terminate();
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
            n_all_for_now --;
            zc_browse_check_terminate();
            break;
    }
}

void zc_browse_browse_service_type(const char *stype, const char *domain) {
    AvahiServiceBrowser *b;
    AvahiStringList *i;

    assert(client);
    assert(stype);

    for (i = browsed_types; i; i = i->next)
        if (avahi_domain_equal(stype, (char*) i->text)){
            fprintf(stderr, "Already browsed");
            return;}

    if (!(b = avahi_service_browser_new(
            client,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            stype,
            domain,
            0,
            zc_browse_service_browser_callback,
            NULL))) {

        fprintf(stderr, "avahi_service_browser_new() failed: %s\n", avahi_strerror(avahi_client_errno(client)));
        avahi_simple_poll_quit(simple_poll);
    }

    browsed_types = avahi_string_list_add(browsed_types, stype);

    n_all_for_now++;
    n_cache_exhausted++;
}

int zc_browse_start() {

    assert(!browsing);

    if (VERBOSE) {
        const char *version, *hn;

        if (!(version = avahi_client_get_version_string(client))) {
            fprintf(stderr, "Failed to query version string: %s\n", avahi_strerror(avahi_client_errno(client)));
            return -1;
        }

        if (!(hn = avahi_client_get_host_name_fqdn(client))) {
            fprintf(stderr, "Failed to query host name: %s\n", avahi_strerror(avahi_client_errno(client)));
            return -1;
        }

        fprintf(stderr, "Server version: %s; Host name: %s\n", version, hn);

        /* Translators: This is a column heading with abbreviations for
         *   Event (+/-), Network Interface, Protocol (IPv4/v6), Domain */
        fprintf(stderr, "E Ifce Prot %-*s %-20s Domain\n", n_columns-35, "Name", "Type");
    }

    zc_browse_browse_service_type(avahi_strdup(SERVICE_TYPE), NULL);

    browsing = 1;
    return 0;
}

void zc_browse_client_callback(AvahiClient *c, AvahiClientState state,
                                      AVAHI_GCC_UNUSED void *userdata) {
    // Config *config = userdata;

    /* This function might be called when avahi_client_new() has not
     * returned yet.*/
    client = c;

    switch (state) {
        case AVAHI_CLIENT_FAILURE:

            if (NO_FAIL && avahi_client_errno(c) == AVAHI_ERR_DISCONNECTED) {
                int error;

                /* We have been disconnected, so let reconnect */

                fprintf(stderr, "Disconnected, reconnecting ...\n");

                avahi_client_free(client);
                client = NULL;

                avahi_string_list_free(browsed_types);
                browsed_types = NULL;

                while (services)
                    zc_browse_remove_service(services);

                browsing = 0;

                if (!(client = avahi_client_new(avahi_simple_poll_get(simple_poll),
                                                AVAHI_CLIENT_NO_FAIL,
                                                zc_browse_client_callback, NULL, &error))) {
                    fprintf(stderr, "Failed to create client object: %s\n", avahi_strerror(error));
                    avahi_simple_poll_quit(simple_poll);
                }

            } else {
                fprintf(stderr, "Client failure, exiting: %s\n", avahi_strerror(avahi_client_errno(c)));
                avahi_simple_poll_quit(simple_poll);
            }

            break;

        case AVAHI_CLIENT_S_REGISTERING:
        case AVAHI_CLIENT_S_RUNNING:
        case AVAHI_CLIENT_S_COLLISION:

            if (!browsing)
            if (zc_browse_start() < 0)
                avahi_simple_poll_quit(simple_poll);

            break;

        case AVAHI_CLIENT_CONNECTING:

            //if (config->verbose && !config->parsable)
            if(VERBOSE)
                fprintf(stderr, "Waiting for daemon ...\n");

            break;
    }
}

int zc_browse_main(){

    int ret = 1, error;

    if (!(simple_poll = avahi_simple_poll_new())) {
        fprintf(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

/*            if (sigint_install(simple_poll) < 0)
                goto fail;*/

    if (!(client = avahi_client_new(avahi_simple_poll_get(simple_poll), AVAHI_CLIENT_NO_FAIL,
                                    zc_browse_client_callback, NULL, &error))) {
        fprintf(stderr, "Failed to create client object: %s\n", avahi_strerror(error));
        goto fail;
    }

    avahi_simple_poll_loop(simple_poll);
    ret = 0;

    fail:

    while (services)
        zc_browse_remove_service(services);

    if (client)
        avahi_client_free(client);

    //sigint_uninstall();

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    //avahi_free(config.domain);
    //avahi_free(config.stype);

    avahi_string_list_free(browsed_types);

    return ret;

}