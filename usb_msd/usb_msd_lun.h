#ifndef USB_MSD_LUN_H_
#define USB_MSD_LUN_H_

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

#endif /*USB_MSD_LUN_H_*/
