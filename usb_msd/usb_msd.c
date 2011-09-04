#include "usb_msd.h"
#include "usb_bot.h"
#include "usb_msd_sbc.h"
#include "usb_trace.h"

/* This is the toplevel of the USB mass storage driver (MSD).

   The usb_bot (Bulk Only Transfer) provides a transport layer using
   usb_drv (the basic USB driver).  The SCSI block command (SCB)
   protocol uses this transport layer for communication with the USB
   host and uses the logical unit number (LUN) driver to interface
   with a mass storage device (MSD), usually a nand flash device.

   There is still too much taking place in the USB interrupt handler.
   Currently usb_lun is synchronous but I'm not sure whether making
   this asynchronous with callbacks has any advantage; perhaps with
   slow writes?
*/


typedef enum 
{
    USB_MSD_STATE_UNINIT,
    USB_MSD_STATE_INIT,
    USB_MSD_STATE_COMMAND_READ,
    USB_MSD_STATE_PREPROCESS,
    USB_MSD_STATE_PROCESS,
    USB_MSD_STATE_STATUS_SEND
} usb_msd_state_t;


#ifndef USB_VENDOR_ID
#define USB_VENDOR_ID 0x03EB
#endif


#ifndef USB_PRODUCT_ID
//#define USB_PRODUCT_ID 0x6202
#define USB_PRODUCT_ID 0x6129
#endif


#ifndef USB_RELEASE_ID
#define USB_RELEASE_ID 0x110
#endif


#ifndef USB_CURRENT_MA
#define USB_CURRENT_MA 100
#endif


extern const usb_dsc_t usb_msd_descriptors;


typedef struct
{
    usb_msd_state_t state;
    S_usb_bot_command_state command;
} usb_msd_dev_t;

static usb_msd_dev_t usb_msd_dev;
static usb_msd_dev_t *usb_msd = &usb_msd_dev;


/**
 * Pre-processes a command by checking the differences between the
 * host and device expectations in term of transfer type and length.
 * 
 * Once one of the thirteen cases is identified, the actions to do
 * during the post-processing phase are stored in the dCase variable
 * of the command state.
 * 
 * \return  True if the command is supported, false otherwise
 * 
 */
static bool 
usb_msd_preprocess (S_usb_bot_command_state *pCommandState)
{
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;
    usb_msd_cbw_t *pCbw = &pCommandState->sCbw;
    uint32_t dHostLength;
    uint32_t dDeviceLength;
    uint8_t bHostType;
    uint8_t bDeviceType;
    bool isCommandSupported = false;

    // Get information about the command
    // Host-side
    usb_bot_get_command_information (pCbw, &dHostLength, &bHostType);

    // Device-side
    isCommandSupported
        = sbc_get_command_information (pCbw, &dDeviceLength, &bDeviceType);

    // Initialize data residue and result status
    pCsw->dCSWDataResidue = 0;
    pCsw->bCSWStatus = MSD_CSW_COMMAND_PASSED;

    // Check if the command is supported
    if (!isCommandSupported)
        return false;

    // Identify the command case.  H is the host expectation,
    // D is the device intent.  n is for no data transfers, i is for input to
    //host, o is for output from host.

    // Case 1  (Hn = Dn)
    if ((bHostType == USB_BOT_NO_TRANSFER)
        && (bDeviceType == USB_BOT_NO_TRANSFER))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hn = Dn\n");
        pCommandState->bCase = 0;
        pCommandState->dLength = 0;
    }
    // Case 2  (Hn < Di)
    else if ((bHostType == USB_BOT_NO_TRANSFER)
             && (bDeviceType == USB_BOT_DEVICE_TO_HOST))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hn < Di\n");
            pCommandState->bCase = USB_BOT_CASE_PHASE_ERROR;
            pCommandState->dLength = 0;
    }
    // Case 3  (Hn < Do)
    else if ((bHostType == USB_BOT_NO_TRANSFER)
             && (bDeviceType == USB_BOT_HOST_TO_DEVICE))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hn < Do\n");
        pCommandState->bCase = USB_BOT_CASE_PHASE_ERROR;
        pCommandState->dLength = 0;
    }
    // Case 4  (Hi > Dn)
    else if ((bHostType == USB_BOT_DEVICE_TO_HOST)
                 && (bDeviceType == USB_BOT_NO_TRANSFER))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hi > Dn\n");
        pCommandState->bCase = USB_BOT_CASE_STALL_IN;
        pCommandState->dLength = 0;
        pCsw->dCSWDataResidue = dHostLength;
    }
    // Case 5  (Hi > Di)
    else if ((bHostType == USB_BOT_DEVICE_TO_HOST)
             && (bDeviceType == USB_BOT_DEVICE_TO_HOST)
             && (dHostLength > dDeviceLength))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hi > Di\n");
        pCommandState->bCase = USB_BOT_CASE_STALL_IN;
        pCommandState->dLength = dDeviceLength;
        pCsw->dCSWDataResidue = dHostLength - dDeviceLength;
    }
    // Case 6  (Hi = Di)
    else if ((bHostType == USB_BOT_DEVICE_TO_HOST)
             && (bDeviceType == USB_BOT_DEVICE_TO_HOST)
             && (dHostLength == dDeviceLength))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hi = Di\n");
        pCommandState->bCase = 0;
        pCommandState->dLength = dDeviceLength;
    }
    // Case 7  (Hi < Di)
    else if ((bHostType == USB_BOT_DEVICE_TO_HOST)
             && (bDeviceType == USB_BOT_DEVICE_TO_HOST)
             && (dHostLength < dDeviceLength))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hi < Di\n");
        pCommandState->bCase = USB_BOT_CASE_PHASE_ERROR;
        pCommandState->dLength = dHostLength;
    }
    // Case 8  (Hi <> Do)
    else if ((bHostType == USB_BOT_DEVICE_TO_HOST)
             && (bDeviceType == USB_BOT_HOST_TO_DEVICE))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Hi <> Do\n");
        pCommandState->bCase = USB_BOT_CASE_STALL_IN | USB_BOT_CASE_PHASE_ERROR;
        pCommandState->dLength = 0;
    }
    // Case 9  (Ho > Dn)
    else if ((bHostType == USB_BOT_HOST_TO_DEVICE)
             && (bDeviceType == USB_BOT_NO_TRANSFER))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Ho > Dn\n");
        pCommandState->bCase = USB_BOT_CASE_STALL_OUT;
        pCommandState->dLength = 0;
        pCsw->dCSWDataResidue = dHostLength;
    }
    // Case 10 (Ho <> Di)
    else if ((bHostType == USB_BOT_HOST_TO_DEVICE)
             && (bDeviceType == USB_BOT_DEVICE_TO_HOST))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Ho <> Di\n");
        pCommandState->bCase = USB_BOT_CASE_STALL_OUT | USB_BOT_CASE_PHASE_ERROR;
        pCommandState->dLength = 0;
    }
    // Case 11 (Ho > Do)
    else if ((bHostType == USB_BOT_HOST_TO_DEVICE)
             && (bDeviceType == USB_BOT_HOST_TO_DEVICE)
             && (dHostLength > dDeviceLength))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Ho > Do\n");
        pCommandState->bCase = USB_BOT_CASE_STALL_OUT;
        pCommandState->dLength = dDeviceLength;
        pCsw->dCSWDataResidue = dHostLength - dDeviceLength;
    }
    // Case 12 (Ho = Do)
    else if ((bHostType == USB_BOT_HOST_TO_DEVICE)
             && (bDeviceType == USB_BOT_HOST_TO_DEVICE)
             && (dHostLength == dDeviceLength))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Ho = Do\n");
        pCommandState->bCase = 0;
        pCommandState->dLength = dDeviceLength;
    }
    // Case 13 (Ho < Do)
    else if ((bHostType == USB_BOT_HOST_TO_DEVICE)
             && (bDeviceType == USB_BOT_HOST_TO_DEVICE)
             && (dHostLength < dDeviceLength))
    {
        TRACE_DEBUG (USB_MSD, "MSD:Ho < Do\n");
        pCommandState->bCase = USB_BOT_CASE_PHASE_ERROR;
        pCommandState->dLength = dHostLength;
    }
    return true;
}


static bool 
usb_msd_process (S_usb_bot_command_state *pCommandState)
{
    usb_bot_status_t bStatus;
    usb_msd_csw_t *pCsw = &pCommandState->sCsw;

    bStatus = sbc_process_command (pCommandState);

    switch (bStatus)
    {
    case USB_BOT_STATUS_ERROR_CBW_PARAMETER:
    case USB_BOT_STATUS_ERROR_CBW_FORMAT:
        pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
        usb_bot_abort (pCommandState);
        return true;
        break;
    
    case USB_BOT_STATUS_ERROR_USB_READ:
    case USB_BOT_STATUS_ERROR_USB_WRITE:
    case USB_BOT_STATUS_ERROR_LUN_READ:
    case USB_BOT_STATUS_ERROR_LUN_WRITE:
    case USB_BOT_STATUS_ERROR_LUN_BUSY:
    case USB_BOT_STATUS_ERROR_LUN_NODEVICE:
        pCsw->bCSWStatus = MSD_CSW_COMMAND_FAILED;
        return true;
        break;

    case USB_BOT_STATUS_INCOMPLETE:
        return false;
        break;

    case USB_BOT_STATUS_SUCCESS:
        return true;
        break;
    }
    return false;
}


/* Poll the state machines.   */
usb_msd_ret_t
usb_msd_update (void)
{
    usb_msd_ret_t ret;

    /* This implements a simple state machine to get a CBW from the
       BOT, send a command to the SBC, then send a CSW to the BOT.
       Note that the BOT (and possibly the SBC) is asynchronous so
       some functions are called multiple times; the first time to
       initiate an action, then subequently to poll their completion.
       An example of this is bot_command_read.   */

    if (usb_bot_ready_p ())
    {
        if (usb_msd->state == USB_MSD_STATE_UNINIT)
        {
            usb_msd->state = USB_MSD_STATE_INIT;            
            ret = USB_MSD_CONNECTED;
        }
        else
        {
            ret = USB_MSD_ACTIVE;
        }
    }
    else
    {
        if (usb_msd->state != USB_MSD_STATE_UNINIT)
        {
            ret = USB_MSD_DISCONNECTED;
            usb_msd->state = USB_MSD_STATE_UNINIT;
        }
        else
        {
            ret = USB_MSD_INACTIVE;
        }
    }


    switch (usb_msd->state)
    {
    case USB_MSD_STATE_UNINIT:
        break;

    case USB_MSD_STATE_INIT:
        sbc_reset ();
        usb_msd->state = USB_MSD_STATE_COMMAND_READ;
        break;

    case USB_MSD_STATE_COMMAND_READ:
        if (usb_bot_command_read (&usb_msd->command))
            usb_msd->state = USB_MSD_STATE_PREPROCESS;
        break;

    case USB_MSD_STATE_PREPROCESS:
        usb_msd_preprocess (&usb_msd->command);
        usb_msd->state = USB_MSD_STATE_PROCESS;
        break;

    case USB_MSD_STATE_PROCESS:
        if (usb_msd_process (&usb_msd->command))
            usb_msd->state = USB_MSD_STATE_STATUS_SEND;
        break;

    case USB_MSD_STATE_STATUS_SEND:
        if (usb_bot_status_write (&usb_msd->command))
            usb_msd->state = USB_MSD_STATE_COMMAND_READ;
        break;
    }

    return ret;
}


bool
usb_msd_init (msd_t **luns, uint8_t num_luns)
{
    int i;

    for (i = 0; i < num_luns; i++)
        sbc_lun_init (luns[i]);

    usb_bot_init (num_luns, &usb_msd_descriptors);

    usb_msd_update ();

    usb_msd->state = USB_MSD_STATE_INIT;

    return true;
};


void 
usb_msd_shutdown (void)
{
    usb_shutdown ();
    usb_msd->state = USB_MSD_STATE_UNINIT;
}


void 
usb_msd_write_protect_set (uint8_t lun_id, bool enable)
{
    sbc_lun_write_protect_set (lun_id, enable);
}
