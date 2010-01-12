#ifndef USB_MSD_H
#define USB_MSD_H

#include "msd.h"

bool usb_msd_init (msd_t **luns, uint8_t num_luns);

bool usb_msd_update (void);


#endif
