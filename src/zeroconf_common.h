//
// Created by olivier on 25/09/15.
//

#ifndef RTP_ZEROCONF_COMMON_H
#define RTP_ZEROCONF_COMMON_H

#include <net/if.h>
#include <avahi-common/address.h>
#include <avahi-common/strlst.h>
#include <stdlib.h>

struct zc_element {
    char ifname[IF_NAMESIZE];
    AvahiProtocol iproto; // 0 for IPv4, 1 for IPv6, -1 for UNSPEC
    const char *hostname;
    char *type;
    uint16_t port;
    char address[AVAHI_ADDRESS_STR_MAX];
    AvahiStringList *txt_options;
};

void zc_free_zc_element(struct zc_element *zc_elem);



#endif //RTP_ZEROCONF_COMMON_H
