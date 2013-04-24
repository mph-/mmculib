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


enum {VGA_HEIGHT = 480,
      CIF_HEIGHT = 288,
      QVGA_HEIGHT = 240,
      QCIF_HEIGHT = 144,
      QQVGA_HEIGHT = 120,
      SQCIF_HEIGHT = 96};
 
enum {VGA_WIDTH = 640,
      CIF_WIDTH = 352,
      QVGA_WIDTH = 320,
      QCIF_WIDTH = 176,
      QQVGA_WIDTH = 160,
      SQCIF_WIDTH = 128};


typedef struct tcm8230_cfg_struct
{
    tcm8230_picsize_t picsize;
    /* Add other parameters here.  */
} tcm8230_cfg_t;


int tcm8230_init (const tcm8230_cfg_t *cfg);


uint32_t tcm8230_capture (uint8_t *image, uint32_t bytes);


uint16_t tcm8230_width (void);


uint16_t tcm8230_height (void);


#endif
