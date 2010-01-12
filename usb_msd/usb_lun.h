/**
 * \file usb_lun.h
 * Header: LUN functions
 * 
 * Logical Unit Number (LUN) functions
 * 
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 * 
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 * 
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 * 
 */
#ifndef USB_LUN_H_
#define USB_LUN_H_

#include "config.h"
#include "msd.h"
#include "usb_sbc_defs.h"

typedef enum
{
    LUN_STATUS_SUCCESS =  0x00,    //!< LUN operation success
    LUN_STATUS_ERROR = 0x02        //!< LUN operation error
} lun_status_t;


typedef uint64_t lun_size_t;
typedef uint32_t lun_addr_t;


/** \struct S_lun 
 * LUN description structure
 * 
 */ 
typedef struct 
{
    msd_t *msd;
    S_sbc_inquiry_data sInquiryData; //!< Inquiry data structure
    S_sbc_request_sense_data sRequestSenseData;  //!< Sense data structure
    S_sbc_read_capacity_10_data sReadCapacityData;  //!< Capacity data sturcture
    lun_size_t media_bytes;       //!< Size of LUN in bytes
    uint32_t block_bytes;         //!< Sector size of LUN in bytes
    uint8_t bMediaStatus;         //!< LUN status
} S_lun;

void lun_init (msd_t *msd);

lun_status_t lun_read (S_lun *pLun, lun_addr_t block, void *buffer,
                       msd_size_t bytes);

lun_status_t lun_write (S_lun *pLun, lun_addr_t block, const void *buffer,
                        msd_size_t bytes);

msd_status_t lun_status_get (S_lun *pLun);

void lun_update_sense_data (S_lun *pLun,
                            unsigned char bSenseKey,
                            unsigned char bAdditionalSenseCode,
                            unsigned char bAdditionalSenseCodeQualifier);

S_lun *lun_get (uint8_t num);

uint8_t lun_num_get (void);

#endif /*USB_LUN_H_*/
