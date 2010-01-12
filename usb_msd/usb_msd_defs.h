/**
 * \file usb_msd_defs.h
 * Header: Mass Storage Class
 * 
 * Mass Storage Class definitions
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
#ifndef USB_MSD_DEFS_H_
#define USB_MSD_DEFS_H_


//! MSD Interface Class Code
#define MSD_INTF            0x08

//! MSD Interface Class SubClass Codes
#define MSD_INTF_SUBCLASS   0x06

//! MSD Interface Class Protocol Codes
#define MSD_PROTOCOL        0x50

// Class-specific requests
//! Bulk transfer reset
#define MSD_BULK_ONLY_RESET                     0xFF
//! Get maximum number of LUNs
#define MSD_GET_MAX_LUN                         0xFE

// Subclass codes
//! Reduced Block Commands (RBC) T10
#define MSD_SUBCLASS_RBC                        0x01
//! C/DVD devices
#define MSD_SUBCLASS_SFF_MCC                    0x02
//! Tape device
#define MSD_SUBCLASS_QIC                        0x03
//! Floppy disk drive (FDD) device
#define MSD_SUBCLASS_UFI                        0x04
//! Floppy disk drive (FDD) device
#define MSD_SUBCLASS_SFF                        0x05
//! SCSI transparent command set
#define MSD_SUBCLASS_SCSI                       0x06

/**
 * \name Table 3.1 - Mass Storage Transport Protocol (see usb_msc_overview_1.2.pdf)
 * 
*/
//@{
#define MSD_PROTOCOL_CBI_COMPLETION             0x00
#define MSD_PROTOCOL_CBI                        0x01
#define MSD_PROTOCOL_BULK_ONLY                  0x50
//@}

/**
 * \name Test unit control:
 * 
 */
//@{
#define CTRL_NOT_READY                          0x00
#define CTRL_GOOD                               0x01
#define CTRL_BUSY                               0x02
//@}

// Command block wrapper
#define MSD_CBW_SIZE                            31          //!< Command Block Wrapper Size
#define MSD_CBW_SIGNATURE                       0x43425355  //!< 'USBC'

// Command status wrapper
#define MSD_CSW_SIZE                            13          //!< Command status wrapper size
#define MSD_CSW_SIGNATURE                       0x53425355  //!< 'USBS'

/**
 * \name Table 5.3 - Command Block Status Values (usbmassbulk_10.pdf)
 * 
 */
typedef enum
{
    MSD_CSW_COMMAND_PASSED,
    MSD_CSW_COMMAND_FAILED,
    MSD_CSW_PHASE_ERROR
} msd_csw_status_t;


//! CBW bmCBWFlags field
#define MSD_CBW_DEVICE_TO_HOST                  (1 << 7)

//! Table 5.1 - Command Block Wrapper (see usbmassbulk_10.pdf)
//! The CBW shall start on a packet boundary and shall end as a
//! short packet with exactly 31 (1Fh) bytes transferred.
typedef struct
{

    uint32_t dCBWSignature;          //!< 'USBC' 0x43425355 (little endian)
    uint32_t dCBWTag;                //!< Must be the same as dCSWTag
    uint32_t dCBWDataTransferLength; //!< Number of bytes transfer
    uint8_t  bmCBWFlags;             //!< indicates the directin of the
                                     //!< transfer: 0x80=IN=device-to-host,
                                     //!< 0x00=OUT=host-to-device
    uint8_t bCBWLUN   :4,            //!< bits 0->3: bCBWLUN
            bReserved1:4;            //!< reserved
    uint8_t bCBWCBLength:5,          //!< bits 0->4: bCBWCBLength
            bReserved2  :3;          //!< reserved
    uint8_t pCommand[16];            //!< Command block

} S_msd_cbw;

//! Table 5.2 - Command Status Wrapper (CSW) (usbmassbulk_10.pdf)
typedef struct
{
    uint32_t  dCSWSignature;   //!< 'USBS' 0x53425355 (little endian)
    uint32_t  dCSWTag;         //!< Must be the same as dCBWTag
    uint32_t  dCSWDataResidue; //!< For Data-Out the device shall report in the dCSWDataResidue the difference between the amount of
                                   // data expected as stated in the dCBWDataTransferLength, and the actual amount of data processed by
                                   // the device. For Data-In the device shall report in the dCSWDataResidue the difference between the
                                   // amount of data expected as stated in the dCBWDataTransferLength and the actual amount of relevant
                                   // data sent by the device. The dCSWDataResidue shall not exceed the value sent in the dCBWDataTransferLength.
    uint8_t bCSWStatus;     //!< Indicates the success or failure of the command.

} S_msd_csw;

#endif /*USB_MSD_H_*/
