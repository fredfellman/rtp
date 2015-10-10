//
// Created by olivier on 25/09/15.
//

#include "zeroconf_common.h"


void zc_free_zc_element(struct zc_element *zc_elem) {
    free(zc_elem->txt_options);
    free(zc_elem->type);
    free(zc_elem);
}
