//
// Created by olivier on 11/09/15.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <unistd.h>
#include "pthread.h"
#include <stdio.h>

#include <unistd.h>
#include "src/zeroconf_browse.h"
#include "src/zeroconf_publish.h"
#include "src/zeroconf_common.h"
#include "src/zeroconf_rtp.h"


#include <avahi-common/llist.h>
#include <avahi-common/error.h>

#include <avahi-common/malloc.h>
#include "avahi-client/client.h"

#include "src/zeroconf_queue.h"

// WARNING ! zc_elem->hostname provoke errors when read because dbus do not specify the right size of the pointer (or something like that), we can do the same fix that with the zc_elem->type, but do we really need hostname ?

void test_zc_rtp_publish(void **state){
    struct zc_published_service *service = malloc(sizeof(struct zc_published_service));
    int ret = zc_rtp_publish(service);
    zc_rtp_free_service(service);
    assert_int_equal(ret, 0);
}

void test_zc_rtp_browse(void **state){
    struct zc_queue *queue = create_queue_pointer();
    int ret = zc_rtp_browse(queue);

    struct zc_queue_elem *cursor = NULL;
    struct zc_element *zc_elem = NULL;
    char *t= NULL;
    cursor = queue_pop_front(queue);
    while (cursor != NULL){
        zc_elem = (struct zc_element *)cursor;
        t = avahi_string_list_to_string(zc_elem->txt_options);
        fprintf(stdout, "Elem : %s:%d %s \n", zc_elem->address, zc_elem->port, zc_elem->ifname);
        //%s %s %s %s
        //, zc_elem->hostname, zc_elem->type, t
        cursor = queue_pop_front(queue);
        free(t);
        zc_free_zc_element(zc_elem);
    }

    zc_delete_service_queue(queue);
    queue_delete_queue_pointer(queue);
    assert_int_equal(ret, 0);
}

void test_zc_rtp_global(void **state) {
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
        fprintf(stdout, "Elem : %s:%d %s \n", zc_elem->address, zc_elem->port, zc_elem->ifname);
        cursor = queue_pop_front(queue);
        free(t);
        zc_free_zc_element(zc_elem);
    }

    zc_rtp_free_service(service);
    zc_delete_service_queue(queue);
    queue_delete_queue_pointer(queue);
    assert_int_equal(ret, 0);
}

int main(void)
{
    srand ((unsigned int) time(NULL));
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_zc_rtp_publish),
            cmocka_unit_test(test_zc_rtp_browse),
            cmocka_unit_test(test_zc_rtp_global),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}