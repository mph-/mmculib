#ifndef USB_SBC_DEFS_H_
#define USB_SBC_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

/** 
 * \name  Operation codes of commands described in the SBC-3 standard
 * 
 *          Note that most commands are actually defined in other standards,
 *          like SPC-4. Optional commands are not included here.
 * \see     sbc3r07.pdf - Section 5.1 - Table 12
 * \see     spc4r06.pdf
 * \see     S_sbc_command
 *
 * The numbers after the commands refer to the number of bytes in the 
 * command packet, for example read (10) has a 10 byte packet.
 * 
 */
typedef enum
{
    SBC_INQUIRY = 0x12,
    SBC_READ_10 = 0x28,
    SBC_READ_CAPACITY_10 = 0x25,
    SBC_REQUEST_SENSE = 0x03,
    SBC_TEST_UNIT_READY = 0x00,
    SBC_WRITE_10 = 0x2A,
/**
 * \name Optional according to the standard, required by Windows
 */
    SBC_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
    SBC_MODE_SENSE_6 = 0x1A,
    SBC_VERIFY_10 = 0x2F
} sbc_command_t;

/**
 * \name  Peripheral qualifier values specified in the INQUIRY data
 * \see   spc4r06.pdf - Section 6.4.2 - Table 83
 * \see   S_sbc_inquiry_data
 * 
 */
//@{
#define SBC_PERIPHERAL_DEVICE_CONNECTED                 0x00
#define SBC_PERIPHERAL_DEVICE_NOT_CONNECTED             0x01
#define SBC_PERIPHERAL_DEVICE_NOT_SUPPORTED             0x03
//@}

/**
 * \name  Peripheral device types specified in the INQUIRY data
 * \see    spc4r06.pdf - Section 6.4.2 - Table 84
 * \see    S_sbc_inquiry_data
 * 
 */
//@{
#define SBC_DIRECT_ACCESS_BLOCK_DEVICE              0x00
#define SBC_SEQUENTIAL_ACCESS_DEVICE                0x01
#define SBC_PRINTER_DEVICE                          0x02
#define SBC_PROCESSOR_DEVICE                        0x03
#define SBC_WRITE_ONCE_DEVICE                       0x04
#define SBC_CD_DVD_DEVICE                           0x05
#define SBC_SCANNER_DEVICE                          0x06
#define SBC_OPTICAL_MEMORY_DEVICE                   0x07
#define SBC_MEDIA_CHANGER_DEVICE                    0x08
#define SBC_COMMUNICATION_DEVICE                    0x09
#define SBC_STORAGE_ARRAY_CONTROLLER_DEVICE         0x0C
#define SBC_ENCLOSURE_SERVICES_DEVICE               0x0D
#define SBC_SIMPLIFIED_DIRECT_ACCESS_DEVICE         0x0E
#define SBC_OPTICAL_CARD_READER_WRITER_DEVICE       0x0F
#define SBC_BRIDGE_CONTROLLER_COMMANDS              0x10
#define SBC_OBJECT_BASED_STORAGE_DEVICE             0x11
//@}

//! \brief  Version value for the SBC-3 specification
//! \see    spc4r06.pdf - Section 6.4.2 - Table 85
#define SBC_SPC_VERSION_4                           0x06

/**
 * \name  Values for the TPGS field returned in INQUIRY data
 * \see    spc4r06.pdf - Section 6.4.2 - Table 86
 */
//@{
#define SBC_TPGS_NONE                               0x0
#define SBC_TPGS_ASYMMETRIC                         0x1
#define SBC_TPGS_SYMMETRIC                          0x2
#define SBC_TPGS_BOTH                               0x3
//@}

//! \brief  Version descriptor value for the SBC-3 specification
//! \see    spc4r06.pdf - Section 6.4.2 - Table 87
#define SBC_VERSION_DESCRIPTOR_SBC_3                0x04C0

/**
 * \name  Sense data response codes returned in REQUEST SENSE data
 * \see    spc4r06.pdf - Section 4.5.1 - Table 12
 * 
 */
//@{
#define SBC_SENSE_DATA_FIXED_CURRENT                0x70
#define SBC_SENSE_DATA_FIXED_DEFERRED               0x71
#define SBC_SENSE_DATA_DESCRIPTOR_CURRENT           0x72
#define SBC_SENSE_DATA_DESCRIPTOR_DEFERRED          0x73
//@}

/**
 * \name Sense key values returned in the REQUEST SENSE data
 * \see  spc4r06.pdf - Section 4.5.6 - Table 27
 * 
 */
//@{ 
#define SBC_SENSE_KEY_NO_SENSE                        0x00
#define SBC_SENSE_KEY_RECOVERED_ERROR                 0x01
#define SBC_SENSE_KEY_NOT_READY                       0x02
#define SBC_SENSE_KEY_MEDIUM_ERROR                    0x03
#define SBC_SENSE_KEY_HARDWARE_ERROR                  0x04
#define SBC_SENSE_KEY_ILLEGAL_REQUEST                 0x05
#define SBC_SENSE_KEY_UNIT_ATTENTION                  0x06
#define SBC_SENSE_KEY_DATA_PROTECT                    0x07
#define SBC_SENSE_KEY_BLANK_CHECK                     0x08
#define SBC_SENSE_KEY_VENDOR_SPECIFIC                 0x09
#define SBC_SENSE_KEY_COPY_ABORTED                    0x0A
#define SBC_SENSE_KEY_ABORTED_COMMAND                 0x0B
#define SBC_SENSE_KEY_VOLUME_OVERFLOW                 0x0D
#define SBC_SENSE_KEY_MISCOMPARE                      0x0E
//@}

/**
 * \name Additional sense code values returned in REQUEST SENSE data
 * \see  spc4r06.pdf - Section 4.5.6 - Table 28
 * 
 */
//@{
#define SBC_ASC_LOGICAL_UNIT_NOT_READY                0x04
#define SBC_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE    0x21
#define SBC_ASC_INVALID_FIELD_IN_CDB                  0x24
#define SBC_ASC_WRITE_PROTECTED                       0x27
#define SBC_ASC_FORMAT_CORRUPTED                      0x31
#define SBC_ASC_INVALID_COMMAND_OPERATION_CODE        0x20
#define SBC_ASC_TOO_MUCH_WRITE_DATA                   0x26
#define SBC_ASC_NOT_READY_TO_READY_CHANGE             0x28
#define SBC_ASC_MEDIUM_NOT_PRESENT                    0x3A
//@}

//! \brief  MEDIUM TYPE field value for direct-access block devices
//! \see    sbc3r06.pdf - Section 6.3.1
#define SBC_MEDIUM_TYPE_DIRECT_ACCESS_BLOCK_DEVICE    0x00

/**
 * \name MRIE field values
 * \see  sbc3r06.pdf - Section 7.4.11 - Table 286
 * 
 */
//@{
#define SBC_MRIE_NO_REPORTING                         0x00
#define SBC_MRIE_ASYNCHRONOUS                         0x01
#define SBC_MRIE_GENERATE_UNIT_ATTENTION              0x02
#define SBC_MRIE_COND_GENERATE_RECOVERED_ERROR        0x03
#define SBC_MRIE_UNCOND_GENERATE_RECOVERED_ERROR      0x04
#define SBC_MRIE_GENERATE_NO_SENSE                    0x05
#define SBC_MRIE_ON_REQUEST                           0x06
//@}

/**
 * \name Supported mode pages
 * \see  sbc3r06.pdf - Section 6.3.1 - Table 115
 * 
 */
//@{
#define SBC_PAGE_READ_WRITE_ERROR_RECOVERY            0x01
#define SBC_PAGE_INFORMATIONAL_EXCEPTIONS_CONTROL     0x1C
#define SBC_PAGE_RETURN_ALL                           0x3F
#define SBC_PAGE_VENDOR_SPECIFIC                      0x00
//@}

//! \brief  Structure for the INQUIRY command
//! \see    spc4r06.pdf - Section 6.4.1 - Table 81
typedef struct
{
    uint8_t bOperationCode;       //!< 0x12 : SBC_INQUIRY
    uint8_t isEVPD:1,             //!< Type of requested data
            bReserved1:7;         //!< Reserved bits
    uint8_t bPageCode;            //!< Specifies the VPD to return
    uint8_t pAllocationLength[2]; //!< Size of host buffer
    uint8_t bControl;             //!< 0x00
} __packed__ S_sbc_inquiry;


//! \brief  Standard INQUIRY data format returned by the device
//! \see    spc4r06.pdf - Section 6.4.2 - Table 82
typedef struct 
{
    uint8_t  bPeripheralDeviceType:5, //!< Peripheral device type
             bPeripheralQualifier :3; //!< Peripheral qualifier
    uint8_t  bReserved1:7,            //!< Reserved bits
             isRMB     :1;            //!< Is media removable ?
    uint8_t  bVersion;                //!< SPC version used
    uint8_t  bResponseDataFormat:4,   //!< Must be 0x2
             isHIGHSUP          :1,   //!< Hierarchical addressing used ?
             isNORMACA          :1,   //!< ACA attribute supported ?
             bObsolete1         :2;   //!< Obsolete bits
    uint8_t  bAdditionalLength;       //!< Length of remaining INQUIRY data
    uint8_t  isSCCS    :1,            //!< Embedded SCC ?
             isACC     :1,            //!< Access control coordinator ?
             bTPGS     :2,            //!< Target port support group
             is3PC     :1,            //!< Third-party copy supported ?
             bReserved2:2,            //!< Reserved bits
             isProtect :1;            //!< Protection info supported ?
    uint8_t  bObsolete2:1,            //!< Obsolete bit
             isEncServ :1,            //!< Embedded enclosure service comp?
             isVS      :1,            //!< ???
             isMultiP  :1,            //!< Multi-port device ?
             bObsolete3:3,            //!< Obsolete bits
             bUnused1  :1;            //!< Unused feature
    uint8_t  bUnused2:6,              //!< Unused features
             isCmdQue:1,              //!< Task management model supported ?
             isVS2   :1;              //!< ???
    uint8_t  pVendorID[8];            //!< T10 vendor identification
    uint8_t  pProductID[16];          //!< Vendor-defined product ID
    uint8_t  pProductRevisionLevel[4];//!< Vendor-defined product revision
    uint8_t  pVendorSpecific[20];     //!< Vendor-specific data
    uint8_t  bUnused3;                //!< Unused features
    uint8_t  bReserved3;              //!< Reserved bits
    uint16_t pVersionDescriptors[8];  //!< Standards the device complies to
    uint8_t  pReserved4[22];          //!< Reserved bytes
} __packed__ S_sbc_inquiry_data;



//! \brief  Data structure for the READ (10) command
//! \see    sbc3r07.pdf - Section 5.7 - Table 34
typedef struct
{
    uint8_t bOperationCode;          //!< 0x28 : SBC_READ_10
    uint8_t bObsolete1:1,            //!< Obsolete bit
            isFUA_NV:1,              //!< Cache control bit
            bReserved1:1,            //!< Reserved bit
            isFUA:1,                 //!< Cache control bit
            isDPO:1,                 //!< Cache control bit
            bRdProtect:3;            //!< Protection information to send
    uint8_t pLogicalBlockAddress[4]; //!< Index of first block to read
    uint8_t bGroupNumber:5,          //!< Information grouping
            bReserved2:3;            //!< Reserved bits
    uint8_t pTransferLength[2];      //!< Number of blocks to transmit
    uint8_t bControl;                //!< 0x00
} __packed__ S_sbc_read_10;


//! \brief  Structure for the READ CAPACITY (10) command
//! \see    sbc3r07.pdf - Section 5.11.1 - Table 40
typedef struct
{
    uint8_t bOperationCode;          //!< 0x25 : RBC_READ_CAPACITY
    uint8_t bObsolete1:1,            //!< Obsolete bit
            bReserved1:7;            //!< Reserved bits
    uint8_t pLogicalBlockAddress[4]; //!< Block to evaluate if PMI is set
    uint8_t pReserved2[2];           //!< Reserved bytes
    uint8_t isPMI:1,                 //!< Partial medium indicator bit
            bReserved3:7;            //!< Reserved bits
    uint8_t bControl;                //!< 0x00
} S_sbc_read_capacity_10;


//! \brief  Data returned by the device after a READ CAPACITY (10) command
//! \see    sbc3r07.pdf - Section 5.11.2 - Table 41
typedef struct
{
    uint8_t pLogicalBlockAddress[4]; //!< Address of last logical block
    uint8_t pLogicalBlockLength[4];  //!< Length of last logical block

} S_sbc_read_capacity_10_data;


//! \brief  Structure for the REQUEST SENSE command
//! \see    spc4r06.pdf - Section 6.26 - Table 170
typedef struct
{
    uint8_t bOperationCode;    //!< 0x03 : SBC_REQUEST_SENSE
    uint8_t isDesc    :1,      //!< Type of information expected
            bReserved1:7;      //!< Reserved bits
    uint8_t pReserved2[2];     //!< Reserved bytes
    uint8_t bAllocationLength; //!< Size of host buffer
    uint8_t bControl;          //!< 0x00
} S_sbc_request_sense;


//! \brief  Fixed format sense data returned after a REQUEST SENSE command has
//!         been received with a DESC bit cleared.
//! \see    spc4r06.pdf - Section 4.5.3 - Table 26
typedef struct
{
    uint8_t bResponseCode:7,                //!< Sense data format
            isValid      :1;                //!< Information field is standard
    uint8_t bObsolete1;                     //!< Obsolete byte
    uint8_t bSenseKey :4,                   //!< Generic error information
            bReserved1:1,                   //!< Reserved bit
            isILI     :1,                   //!< SSC
            isEOM     :1,                   //!< SSC
            isFilemark:1;                   //!< SSC
    uint8_t pInformation[4];                //!< Command-specific
    uint8_t bAdditionalSenseLength;         //!< sizeof(S_sbc_request_sense_data)-8
    uint8_t pCommandSpecificInformation[4]; //!< Command-specific
    uint8_t bAdditionalSenseCode;           //!< Additional error information
    uint8_t bAdditionalSenseCodeQualifier;  //!< Further error information
    uint8_t bFieldReplaceableUnitCode;      //!< Specific component code
    uint8_t bSenseKeySpecific:7,            //!< Additional exception info
            isSKSV           :1;            //!< Is sense key specific valid?
    uint8_t pSenseKeySpecific[2];           //!< Additional exception info
} S_sbc_request_sense_data;


//! \brief  Data structure for the TEST UNIT READY command
//! \see    spc4r06.pdf - Section 6.34 - Table 192
typedef struct 
{
    uint8_t bOperationCode; //!< 0x00 : SBC_TEST_UNIT_READY
    uint8_t pReserved1[4];  //!< Reserved bits
    uint8_t bControl;       //!< 0x00
} __packed__ S_sbc_test_unit_ready;


//! \brief  Structure for the WRITE (10) command
//! \see    sbc3r07.pdf - Section 5.26 - Table 70
typedef struct
{
    uint8_t bOperationCode;          //!< 0x2A : SBC_WRITE_10
    uint8_t bObsolete1:1,            //!< Obsolete bit
            isFUA_NV:1,              //!< Cache control bit
            bReserved1:1,            //!< Reserved bit
            isFUA:1,                 //!< Cache control bit
            isDPO:1,                 //!< Cache control bit
            bWrProtect:3;            //!< Protection information to send
    uint8_t pLogicalBlockAddress[4]; //!< First block to write
    uint8_t bGroupNumber:5,          //!< Information grouping
            bReserved2:3;            //!< Reserved bits
    uint8_t pTransferLength[2];      //!< Number of blocks to write
    uint8_t bControl;                //!< 0x00
} __packed__ S_sbc_write_10;


//! \brief  Structure for the PREVENT/ALLOW MEDIUM REMOVAL command
//! \see    sbc3r07.pdf - Section 5.5 - Table 30
typedef struct {

    uint8_t bOperationCode; //!< 0x1E : SBC_PREVENT_ALLOW_MEDIUM_REMOVAL
    uint8_t pReserved1[3];  //!< Reserved bytes
    uint8_t bPrevent:2,     //!< Accept/prohibit removal
            bReserved2:6;   //!< Reserved bits
    uint8_t bControl;       //!< 0x00
} __packed__ S_sbc_medium_removal;


//! \brief  Structure for the MODE SENSE (6) command
//! \see    spc4r06 - Section 6.9.1 - Table 98
typedef struct 
{
    uint8_t bOperationCode;    //!< 0x1A : SBC_MODE_SENSE_6
    uint8_t bReserved1:3,      //!< Reserved bits
            isDBD:1,           //!< Disable block descriptors bit
            bReserved2:4;      //!< Reserved bits
    uint8_t bPageCode:6,       //!< Mode page to return
            bPC:2;             //!< Type of parameter values to return
    uint8_t bSubpageCode;      //!< Mode subpage to return
    uint8_t bAllocationLength; //!< Host buffer allocated size
    uint8_t bControl;          //!< 0x00
} __packed__ S_sbc_mode_sense_6;


//! \brief  Header for the data returned after a MODE SENSE (6) command
//! \see    spc4r06.pdf - Section 7.4.3 - Table 268
typedef struct
{
    uint8_t bModeDataLength;          //!< Length of mode data to follow
    uint8_t bMediumType;              //!< Type of medium (SBC_MEDIUM_TYPE_DIRECT_ACCESS_BLOCK_DEVICE)
    uint8_t bReserved1:4,             //!< Reserved bits
            isDPOFUA:1,               //!< DPO/FUA bits supported ?
            bReserved2:2,             //!< Reserved bits
            isWP:1;                   //!< Is medium write-protected ?
    uint8_t bBlockDescriptorLength;   //!< Length of all block descriptors
} __packed__ S_sbc_mode_parameter_header_6;


//! \brief  Informational exceptions control mode page
//! \see    spc4r06.pdf - Section 7.4.11 - Table 285
typedef struct
{
    uint8_t bPageCode:6,       //!< 0x1C : SBC_PAGE_INFORMATIONAL_EXCEPTIONS_CONTROL
            isSPF:1,           //!< Page or subpage data format
            isPS:1;            //!< Parameters saveable ?
    uint8_t bPageLength;       //!< Length of page data (0x0A)
    uint8_t isLogErr:1,        //!< Should informational exceptions be logged ?
            isEBackErr:1,      //!< Enable background error bit
            isTest:1,          //!< Create a device test failure ?
            isDExcpt:1,        //!< Disable exception control bit
            isEWasc:1,         //!< Report warnings ?
            isEBF:1,           //!< Enable background function bit
            bReserved1:1,      //!< Reserved bit
            isPerf:1;          //!< Delay acceptable when treating exceptions ?
    uint8_t bMRIE:4,           //!< Method of reporting informational exceptions
            bReserved2:4;      //!< Reserved bits
    uint8_t pIntervalTimer[4]; //!< Error reporting period
    uint8_t pReportCount[4];   //!< Maximum number of time a report can be issued
} __packed__ S_sbc_informational_exceptions_control;


//! \brief  Read/write error recovery mode page
//! \see    sbc3r07.pdf - Section 6.3.5 - Table 122
typedef struct
{
    uint8_t bPageCode:6,           //!< 0x01 : SBC_PAGE_READ_WRITE_ERROR_RECOVERY
            isSPF:1,               //!< Page or subpage data format
            isPS:1;                //!< Parameters saveable ?
    uint8_t bPageLength;           //!< Length of page data (0x0A)
    uint8_t isDCR:1,               //!< Disable correction bit
            isDTE:1,               //!< Data terminate on error bit
            isPER:1,               //!< Post error bit
            isEER:1,               //!< Enable early recovery bit
            isRC:1,                //!< Read continuous bit
            isTB:1,                //!< Transfer block bit
            isARRE:1,              //!< Automatic read reallocation enabled bit
            isAWRE:1;              //!< Automatic write reallocation enabled bit
    uint8_t bReadRetryCount;       //!< Number of retries when reading
    uint8_t pObsolete1[3];         //!< Obsolete bytes
    uint8_t bReserved1;            //!< Reserved byte
    uint8_t bWriteRetryCount;      //!< Number of retries when writing
    uint8_t bReserved2;            //!< Reserved byte
    uint8_t pRecoveryTimeLimit[2]; //!< Maximum time duration for error recovery
} __packed__ S_sbc_read_write_error_recovery;


//! \brief  Generic structure for holding information about SBC commands
//! \see    S_sbc_inquiry
//! \see    S_sbc_read_10
//! \see    S_sbc_read_capacity_10
//! \see    S_sbc_request_sense
//! \see    S_sbc_test_unit_ready
//! \see    S_sbc_write_10
typedef union
{
    uint8_t                bOperationCode;  //!< Operation code of the command
    S_sbc_inquiry          sInquiry;        //!< INQUIRY command
    S_sbc_read_10          sRead10;         //!< READ (10) command
    S_sbc_read_capacity_10 sReadCapacity10; //!< READ CAPACITY (10) command
    S_sbc_request_sense    sRequestSense;   //!< REQUEST SENSE command
    S_sbc_test_unit_ready  sTestUnitReady;  //!< TEST UNIT READY command
    S_sbc_write_10         sWrite10;        //!< WRITE (10) command
    S_sbc_medium_removal   sMediumRemoval;  //!< PREVENT/ALLOW MEDIUM REMOVAL command
    S_sbc_mode_sense_6     sModeSense6;     //!< MODE SENSE (6) command
} __packed__ S_sbc_command;



#ifdef __cplusplus
}
#endif    
#endif

