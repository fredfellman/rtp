//
// Created by olivier on 11/09/15.
//

#include <unistd.h>
#include "src/zeroconf_browse.h"
#include "src/zeroconf_publish.h"
#include "src/zeroconf_rtp.h"


#include <avahi-common/llist.h>
#include <avahi-common/error.h>

#include <avahi-common/malloc.h>

#include "avahi-client/client.h"



#include "src/zeroconf_queue.h"

int main(int argc, char *argv[]) {
    struct zc_published_service *service = malloc(sizeof(struct zc_published_service));
    int ret = zc_rtp_publish(service);

    struct zc_queue *queue = create_queue_pointer();
    ret = (zc_rtp_browse(queue) == 0) ? ret : 1;

    struct zc_queue_elem *cursor = NULL;
    struct zc_element *zc_elem = NULL;
    char *t= NULL;
    cursor = queue_pop_front(queue);
    while (cursor != NULL){
        zc_elem = (struct zc_element *)cursor;
        t = avahi_string_list_to_string(zc_elem->txt_options);
        fprintf(stdout, "Elem : %s:%d %s %s %s %s \n", zc_elem->address, zc_elem->port, zc_elem->ifname, zc_elem->hostname, zc_elem->type, t);
        cursor = queue_pop_front(queue);
    }

    zc_rtp_free_service(service);
    queue_delete_queue_pointer(queue);
    return ret;
}

int old_main(int argc, char *argv[]) {
    struct zc_published_service service = {
            .client = NULL,
            .entry_group = NULL,
            .config = NULL
    };
    char *zcp_argv[] = {"avahi-publish", "-s", "myservice", "_rtp._tcp",  "12345", "Here it is"};
    zc_publish_main(&service, 6, zcp_argv);

    struct zc_queue *queue = create_queue_pointer();
    char *zcb_argv[] = {"avahi-browse", "-a", "-p", "-r", "-t", "-v"};
    zc_browse_main(queue, 6, zcb_argv);

    struct zc_queue_elem *cursor = NULL;
    struct zc_element *zc_elem = NULL;
    char *t= NULL;
    cursor = queue_pop_front(queue);
    while (cursor != NULL){
        zc_elem = (struct zc_element *)cursor;
        t = avahi_string_list_to_string(zc_elem->txt_options);
        fprintf(stdout, "Elem : %s:%d %s %s %s %s \n", zc_elem->address, zc_elem->port, zc_elem->ifname, zc_elem->hostname, zc_elem->type, t);
        cursor = queue_pop_front(queue);
    }

    if (!service.entry_group) {
        if (!(service.entry_group = avahi_entry_group_new(*(service.config->client), zcp_entry_group_callback, service.config))) {
            fprintf(stdout, ("Failed to create entry group: %s\n"), avahi_strerror(avahi_client_errno(*(service.config->client))));
            return -1;
        }else{
            fprintf(stdout, ("OK to create entry group: \n"));
        }
    }

    //assert(avahi_entry_group_is_empty(service.entry_group));

    fprintf(stderr, ("Name %s: \n"), service.config->name);
//
    service.config->txt = avahi_string_list_add(NULL, "plop=ok");
    avahi_entry_group_update_service_txt_strlst(service.entry_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, service.config->name, service.config->stype, service.config->domain, service.config->txt);

    //sleep(1);
    char *zcb_argv2[] = {"avahi-browse", "-a", "-p", "-r", "-t", "-v"};
    zc_browse_main(queue, 6, zcb_argv2);

    cursor = queue_pop_front(queue);
    while (cursor != NULL){
        zc_elem = (struct zc_element *)cursor;
        t = avahi_string_list_to_string(zc_elem->txt_options);
        fprintf(stdout, "Elem : %s:%d %s %s %s \n", zc_elem->address, zc_elem->port, zc_elem->hostname, zc_elem->type, t);
        cursor = queue_pop_front(queue);
    }

//    sleep(2);
//    char *zcp_argv3[] = {"avahi-publish", "-s", "myservice", "_http._tcp",  "12345", "Here it is 2 le retour"};
//    zc_publish_main(&client, &simple_poll, &service2, 6, zcp_argv3);
//
//    char *zcb_argv2[] = {"avahi-browse", "-a", "-p", "-r", "-t", "-v"};
//    sleep(5);
//    zc_browse_main(queue, 6, zcb_argv2);

    queue_delete_queue_pointer(queue);

    if (service.client){
        fprintf(stderr, "Clean up client \n");
        avahi_client_free(service.client);
    }
    if(service.config->simple_poll){
        fprintf(stderr, "Clean up poll \n");
        avahi_simple_poll_free(service.config->simple_poll);
    }
    if(service.config){
        fprintf(stderr, "Clean up config \n");
        free(service.config);
    }
    return 0;
}