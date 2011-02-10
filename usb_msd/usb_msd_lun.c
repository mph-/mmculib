#include "msd.h"
#include "usb_trace.h"
#include "usb_msd_lun.h"
#include "usb_msd_sbc.h"
#include "byteorder.h"

#include <string.h>

/* Maximum number of LUNs.  The bigger the number the more memory chewed up.  */
#ifndef USB_MSD_LUN_NUM
#define USB_MSD_LUN_NUM 1
#endif

#ifndef USB_MSD_VENDOR_STRING
#error  USB_MSD_VENDOR_STRING undefined in config.h
#endif

#ifndef USB_MSD_PRODUCT_STRING
#error  USB_MSD_PRODUCT_STRING undefined in config.h
#endif

#ifndef USB_MSD_REVISION_STRING
#error  USB_MSD_REVISION_STRING undefined in config.h
#endif

#ifndef USB_MSD_DATA_STRING
#define USB_MSD_DATA_STRING {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}
#endif


static usb_msd_lun_t Luns[USB_MSD_LUN_NUM];     //!< LUNs used by the BOT driver
static uint8_t lun_num = 0;

/**
 * Inquiry data used to describe the device
 */
static const S_sbc_inquiry_data sInquiryData = 
{
    .bPeripheralDeviceType = SBC_DIRECT_ACCESS_BLOCK_DEVICE,
    .bPeripheralQualifier = SBC_PERIPHERAL_DEVICE_CONNECTED,
    .bVersion = SBC_SPC_VERSION_4,
    .isRMB = 1,
    .bResponseDataFormat = 0x2,
    .bAdditionalLength = sizeof (S_sbc_inquiry_data) - 5,
    .bTPGS = SBC_TPGS_NONE,
    .pVendorID = USB_MSD_VENDOR_STRING,
    .pProductID = USB_MSD_PRODUCT_STRING,
    .pProductRevisionLevel = USB_MSD_REVISION_STRING,
    .pVendorSpecific = USB_MSD_DATA_STRING,
    .pVersionDescriptors = {SBC_VERSION_DESCRIPTOR_SBC_3}
};


/**
 * Initializes a LUN instance.
 */
usb_msd_lun_t *lun_init (msd_t *msd)
{
    usb_msd_lun_t *pLun;
    usb_msd_lun_addr_t block_max;

    TRACE_INFO (USB_MSD_LUN, "LUN:Init\n");

    if (lun_num >= USB_MSD_LUN_NUM)
        return NULL;
    
    pLun = &Luns[lun_num++];

    // Initialize LUN
    pLun->msd = msd;
    pLun->media_bytes = msd->media_bytes;
    // Only 512 seems to work for Linux.   The msd wrapper works
    // with byte addresses so this number is independent of msd->block_bytes.
    pLun->block_bytes = 512;
    pLun->write_protect = false;

    block_max = (msd->media_bytes / pLun->block_bytes) - 1;

    // Initialize request sense data
    pLun->sRequestSenseData.bResponseCode = SBC_SENSE_DATA_FIXED_CURRENT;
    pLun->sRequestSenseData.isValid = true;
    pLun->sRequestSenseData.bObsolete1 = 0;
    pLun->sRequestSenseData.bSenseKey = SBC_SENSE_KEY_NO_SENSE;
    pLun->sRequestSenseData.bReserved1 = 0;
    pLun->sRequestSenseData.isILI = false;
    pLun->sRequestSenseData.isEOM = false;
    pLun->sRequestSenseData.isFilemark = false;
    pLun->sRequestSenseData.pInformation[0] = 0;
    pLun->sRequestSenseData.pInformation[1] = 0;
    pLun->sRequestSenseData.pInformation[2] = 0;
    pLun->sRequestSenseData.pInformation[3] = 0;
    pLun->sRequestSenseData.bAdditionalSenseLength
        = sizeof (S_sbc_request_sense_data) - 8;
    pLun->sRequestSenseData.bAdditionalSenseCode = 0;
    pLun->sRequestSenseData.bAdditionalSenseCodeQualifier = 0;
    pLun->sRequestSenseData.bFieldReplaceableUnitCode = 0;
    pLun->sRequestSenseData.bSenseKeySpecific = 0;
    pLun->sRequestSenseData.pSenseKeySpecific[0] = 0;
    pLun->sRequestSenseData.pSenseKeySpecific[0] = 0;
    pLun->sRequestSenseData.isSKSV = false;

    // Copy default inquiry data
    memcpy (&pLun->sInquiryData, &sInquiryData, sizeof(sInquiryData));

    // Modify inquiry data
    pLun->sInquiryData.isRMB = msd->flags.removable;
    if (msd->name)
        strncpy ((char *)&pLun->sInquiryData.pVendorSpecific, msd->name,
                 sizeof (pLun->sInquiryData.pVendorSpecific));

    // Initialize read capacity data
    STORE_DWORDB (block_max, pLun->sReadCapacityData.pLogicalBlockAddress);
    STORE_DWORDB (pLun->block_bytes, pLun->sReadCapacityData.pLogicalBlockLength);
    return pLun;
}


/**
 * Reads data from LUN, starting at the specified block address.
 * 
 * \param  pLun    Pointer to LUN to read
 * \param  block   First block address to read
 * \param  buffer  Pointer to a data buffer in which to store the data
 * \param  blocks  Number of blocks to read
 * \return Operation result code
 */
lun_status_t
lun_read (usb_msd_lun_t *pLun, usb_msd_lun_addr_t block, void *buffer, 
          msd_size_t blocks)
{
    msd_size_t bytes;
    msd_size_t result;

    bytes = blocks * pLun->block_bytes;

    TRACE_INFO (USB_MSD_LUN, "LUN:Read (%u)[%u]\n",
               (unsigned int)block, (unsigned int)blocks);
    
    // Check that the data is not too big
    if (bytes > (pLun->media_bytes - pLun->block_bytes * block))
    {
        TRACE_ERROR (USB_MSD_LUN, "LUN:Read too big\n");
        return LUN_STATUS_ERROR;
    }

    result = msd_read (pLun->msd, block * pLun->block_bytes, buffer, bytes);
    if (result == bytes)
        return LUN_STATUS_SUCCESS;

    TRACE_ERROR (USB_MSD_LUN, "LUN:Read error %u/%u bytes\n", result, bytes);
    return LUN_STATUS_ERROR;
}


/**
 * Write data to LUN starting at the specified block address.
 * 
 * \param  pLun    Pointer to LUN to write
 * \param  block   First block address to write
 * \param  buffer  Pointer to the data to write
 * \param  blocks  Number of blocks to write
 * \return Operation result code
 */
lun_status_t
lun_write (usb_msd_lun_t *pLun, usb_msd_lun_addr_t block, const void *buffer,
           msd_size_t blocks)
{
    msd_size_t bytes;
    msd_size_t result;

    bytes = blocks * pLun->block_bytes;

    TRACE_INFO (USB_MSD_LUN, "LUN:Write (%u)[%u]\n", 
                (unsigned int)block, (unsigned int)blocks);

    // Check that the data is not too big
    if (bytes > (pLun->media_bytes - pLun->block_bytes * block))
    {
        TRACE_ERROR (USB_MSD_LUN, "LUN:Write too big\n");
        return LUN_STATUS_ERROR;
    }

    result = msd_write (pLun->msd, block * pLun->block_bytes, buffer, bytes);

    if (result == bytes)
        return LUN_STATUS_SUCCESS;

    TRACE_ERROR (USB_MSD_LUN, "LUN:Write error %u/%u bytes\n", result, bytes);
    return LUN_STATUS_ERROR;
}


/**
 * Get status of LUN.
 * 
 * \param  pLun    Pointer to LUN
 * \return Operation result code
 */
msd_status_t
lun_status_get (usb_msd_lun_t *pLun)
{
    return msd_status_get (pLun->msd);
}


/**
 * Updates the sense data of a LUN with the given key and codes
 * 
 * \param   pLun                          Pointer to LUN to update
 * \param   bSenseKey                     Sense key
 * \param   bAdditionalSenseCode          Additional sense code
 * \param   bAdditionalSenseCodeQualifier Additional sense code qualifier
 */
void 
lun_sense_data_update (usb_msd_lun_t *pLun,
                       unsigned char bSenseKey,
                       unsigned char bAdditionalSenseCode,
                       unsigned char bAdditionalSenseCodeQualifier)
{
    S_sbc_request_sense_data *pRequestSenseData;

    pRequestSenseData = &pLun->sRequestSenseData;

    pRequestSenseData->bSenseKey = bSenseKey;
    pRequestSenseData->bAdditionalSenseCode = bAdditionalSenseCode;
    pRequestSenseData->bAdditionalSenseCodeQualifier
        = bAdditionalSenseCodeQualifier;
}


usb_msd_lun_t *lun_get (uint8_t num)
{
    if (num >= lun_num)
        return 0;

    return Luns + num;
}


uint8_t lun_num_get (void)
{
    return lun_num;
}


void lun_write_protect_set (usb_msd_lun_t *pLun, bool enable)
{
    pLun->write_protect = enable;
}
