#ifndef USB_MSD_LUN_H_
#define USB_MSD_LUN_H_

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "msd.h"
#include "usb_sbc_defs.h"

typedef enum
{
    LUN_STATUS_SUCCESS =  0x00,    //!< LUN operation success
    LUN_STATUS_ERROR = 0x02        //!< LUN operation error
} lun_status_t;


typedef uint64_t usb_msd_lun_capacity_t;
typedef uint32_t usb_msd_lun_addr_t; /* Block address.  */


/** \struct usb_msd_lun_t 
 * LUN description structure
 * 
 */ 
typedef struct 
{
    msd_t *msd;
    //!< Inquiry data structure
    S_sbc_inquiry_data sInquiryData;
    //!< Sense data structure
    S_sbc_request_sense_data sRequestSenseData;
    //!< Capacity data sturcture
    S_sbc_read_capacity_10_data sReadCapacityData;
    //!< Size of LUN in bytes
    usb_msd_lun_capacity_t media_bytes;
    //!< Sector size of LUN in bytes
    uint16_t block_bytes;
    //!< LUN status
    uint8_t bMediaStatus;
    bool write_protect;
} usb_msd_lun_t;


usb_msd_lun_t *lun_init (msd_t *msd);

lun_status_t lun_read (usb_msd_lun_t *pLun, usb_msd_lun_addr_t block,
                       void *buffer, msd_size_t bytes);

lun_status_t lun_write (usb_msd_lun_t *pLun, usb_msd_lun_addr_t block,
                        const void *buffer, msd_size_t bytes);

msd_status_t lun_status_get (usb_msd_lun_t *pLun);

void lun_sense_data_update (usb_msd_lun_t *pLun,
                            unsigned char bSenseKey,
                            unsigned char bAdditionalSenseCode,
                            unsigned char bAdditionalSenseCodeQualifier);

usb_msd_lun_t *lun_get (uint8_t num);

uint8_t lun_num_get (void);

void lun_write_protect_set (usb_msd_lun_t *pLun, bool enable);


#ifdef __cplusplus
}
#endif    
#endif /*USB_MSD_LUN_H_*/

