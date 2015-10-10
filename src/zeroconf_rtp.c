//
// Created by olivier on 10/10/15.
//

#include "zeroconf_rtp.h"

int zc_rtp_publish(struct zc_published_service *service) {
    service->client = NULL;
    service->entry_group = NULL;
    service->config = NULL;
    char *zcp_argv[] = {"avahi-publish", "-s", "Relative Time Sync", "_rtp._tcp",  "34567", "synced=false"};
    return zc_publish_main(service, 6, zcp_argv);
}

int zc_rtp_browse(struct zc_queue *queue){
    struct zc_queue _queue = create_queue();
    char *zcb_argv[] = {"avahi-browse", "-a", "-p", "-r", "-t", "-v"};
    int ret = zc_browse_main(&_queue, 6, zcb_argv);
    if (ret == 0) {
        struct zc_queue_elem *cursor = NULL;
        struct zc_element *zc_elem = NULL;
        cursor = queue_pop_front(&_queue);
        while (cursor != NULL) {
            zc_elem = (struct zc_element *) cursor;
            if(strcmp(zc_elem->type,"_rtp._tcp") == 0){  // It's a RTP element
                queue_push_back_more(queue, zc_elem, sizeof(struct zc_element), 0);
            }else{
                zc_free_zc_element(zc_elem);
            }
            cursor = queue_pop_front(&_queue);
        }
    }
    return ret;
}
