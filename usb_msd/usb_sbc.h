/**
 * \file usb_sbc.h
 * Header: SBC functions
 * 
 * SCSI block command functions
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


bool sbc_get_command_information (S_msd_cbw *pCbw, uint32_t *pLength, 
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
