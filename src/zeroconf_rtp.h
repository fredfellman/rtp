//
// Created by olivier on 10/10/15.
//

#ifndef RTP_ZEROCONF_RTP_H
#define RTP_ZEROCONF_RTP_H

#include "zeroconf_common.h"
#include "zeroconf_publish.h"
#include "zeroconf_browse.h"
#include "zeroconf_queue.h"

int zc_rtp_publish(struct zc_published_service* service);

int zc_rtp_browse(struct zc_queue *queue);


#endif //RTP_ZEROCONF_RTP_H
