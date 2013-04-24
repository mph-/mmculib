/** @file   tcm8230.h
    @author M. P. Hayes, UCECE
    @date   21 Apr 2013
    @brief  Simple TCM8230 driver.
*/
#ifndef TCM8230_H
#define TCM8230_H

#include "config.h"

typedef enum 
{
    TCM8230_PICSIZE_VGA,
    TCM8230_PICSIZE_QVGA,
    TCM8230_PICSIZE_QVGA_ZOOM,
    TCM8230_PICSIZE_QQVGA,
    TCM8230_PICSIZE_QQGA_ZOOM,
    TCM8230_PICSIZE_CIF,
    TCM8230_PICSIZE_QCIF,
    TCM8230_PICSIZE_QCIF_ZOOM,
    TCM8230_PICSIZE_SQCIF,
    TCM8230_PICSIZE_SQCIF_ZOOM
} tcm8230_picsize_t;


int tcm8230_init (tcm8230_picsize_t picsize);

uint32_t tcm8230_capture (uint8_t *image, uint32_t bytes);

#endif
