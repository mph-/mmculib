#ifndef USB_MSD_SBC_H_
#define USB_MSD_SBC_H_

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "usb_sbc_defs.h"
#include "usb_msd_lun.h"
#include "usb_bot.h"


bool sbc_get_command_information (usb_msd_cbw_t *pCbw, uint32_t *pLength, 
                                  uint8_t *pType);

usb_bot_status_t sbc_process_command (S_usb_bot_command_state *pCommandState);

void sbc_update_sense_data (S_sbc_request_sense_data *pRequestSenseData,
                            uint8_t bSenseKey,
                            uint8_t bAdditionalSenseCode,
                            uint8_t bAdditionalSenseCodeQualifier);

void sbc_reset (void);

void sbc_lun_init (msd_t *msd);

uint8_t sbc_lun_num_get (void);

void sbc_lun_write_protect_set (uint8_t lun_id, bool enable);


#ifdef __cplusplus
}
#endif    
#endif /*USB_MSD_SBC_H_*/

