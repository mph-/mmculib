#ifndef USB_MSD_H
#define USB_MSD_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "msd.h"

typedef enum
{
    USB_MSD_INACTIVE,
    USB_MSD_CONNECTED, 
    USB_MSD_ACTIVE,
    USB_MSD_DISCONNECTED
} usb_msd_ret_t;

bool usb_msd_init (msd_t **luns, uint8_t num_luns);

usb_msd_ret_t usb_msd_update (void);

void usb_msd_shutdown (void);

void usb_msd_write_protect_set (uint8_t lun_id, bool enable);


#ifdef __cplusplus
}
#endif    
#endif

