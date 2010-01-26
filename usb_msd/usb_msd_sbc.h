#ifndef USB_SBC_H_
#define USB_SBC_H_

#include "config.h"
#include "usb_sbc_defs.h"
#include "usb_lun.h"
#include "usb_bot.h"


typedef enum
{
    //! \brief  Method was successful
    SBC_STATUS_SUCCESS = 0x00,
    //! \brief  There was an error when trying to perform a method
    SBC_STATUS_ERROR = 0x01,
    //! \brief  No error was encountered but the application should call the
    //!         method again to continue the operation
    SBC_STATUS_INCOMPLETE = 0x02,
    //! \brief  A wrong parameter has been passed to the method
    SBC_STATUS_PARAMETER = 0x03
} sbc_status_t;


bool sbc_get_command_information (usb_msd_cbw_t *pCbw, uint32_t *pLength, 
                                  uint8_t *pType);

sbc_status_t sbc_process_command (S_usb_bot_command_state *pCommandState);

void sbc_update_sense_data (S_sbc_request_sense_data *pRequestSenseData,
                            uint8_t bSenseKey,
                            uint8_t bAdditionalSenseCode,
                            uint8_t bAdditionalSenseCodeQualifier);

void sbc_reset (void);

void sbc_lun_init (msd_t *msd);

uint8_t sbc_lun_num_get (void);

#endif /*USB_SBC_H_*/
