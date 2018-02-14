/** @file   tcm8230.h
    @author M. P. Hayes, UCECE
    @date   21 Apr 2013
    @brief  Simple TCM8230 driver.
*/
#ifndef TCM8230_H
#define TCM8230_H

#ifdef __cplusplus
extern "C" {
#endif
    

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
    bool colour;
    /* Add other parameters here.  */
} tcm8230_cfg_t;


/** Status return codes.  */
typedef enum tcm8230_error
{
    /** No data to read.  */
    TCM8230_NONE = 0,
    /** Timeout waiting for vsync going high.  */
    TCM8230_VSYNC_HIGH_TIMEOUT = -1,
    /** Timeout waiting for vsync going low.  */
    TCM8230_VSYNC_LOW_TIMEOUT = -2,
    /** Timeout waiting for hsync going high.  */
    TCM8230_HSYNC_HIGH_TIMEOUT = -3,
    /** Timeout waiting for hsync going low.  */
    TCM8230_HSYNC_LOW_TIMEOUT = -4,
    /** Image buffer too small.  */
    TCM8230_BUFFER_SMALL = -5,
    /** Line not ready.  */
    TCM8230_LINE_NOT_READY = -6,
    /** Missed VSYNC.  */
    TCM8230_VSYNC_MISSED = -7
} tcm8230_error_t;


/** Initialise image sensor.  */
int tcm8230_init (const tcm8230_cfg_t *cfg);


/** This blocks until it captures a frame.  This may be up to nearly two image
    capture periods.  You should poll tcm8230_frame_ready_p first to see when
    VSYNC (VD) goes low.   If you call this function with VSYNC high you will
    miss the start of the image.  */
int32_t tcm8230_capture (uint8_t *image, uint32_t bytes, uint32_t timeout_us);


int16_t tcm8230_line_read (uint8_t *row, uint16_t bytes);


uint16_t tcm8230_width (void);


uint16_t tcm8230_height (void);


bool tcm8230_frame_ready_p (void);


bool tcm8230_line_ready_p (void);


bool tcm8230_vsync_high_wait (uint32_t timeout_us);


bool tcm8230_vsync_low_wait (uint32_t timeout_us);


bool tcm8230_hsync_high_wait (uint32_t timeout_us);


bool tcm8230_hsync_low_wait (uint32_t timeout_us);


#ifdef __cplusplus
}
#endif    
#endif

