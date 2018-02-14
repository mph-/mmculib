#ifndef USB_STD_H_
#define USB_STD_H_

#ifdef __cplusplus
extern "C" {
#endif
    

/**
 * \name USB standard request identifier
 * 
 */
enum 
{
    USB_GET_STATUS        = 0x00,
    USB_CLEAR_FEATURE     = 0x01,
    USB_SET_FEATURE       = 0x03,
    USB_SET_ADDRESS       = 0x05,
    USB_GET_DESCRIPTOR    = 0x06,
    USB_SET_DESCRIPTOR    = 0x07,
    USB_GET_CONFIGURATION = 0x08,
    USB_SET_CONFIGURATION = 0x09,
    USB_GET_INTERFACE     = 0x0A,
    USB_SET_INTERFACE     = 0x0B,
    USB_SYNCH_FRAME       = 0x0C
};

/**
 * \name USB standard feature selectors
 * 
 */
//@{
#define USB_ENDPOINT_HALT              0x00
#define USB_DEVICE_REMOTE_WAKEUP       0x01
#define USB_TEST_MODE                  0x02
//@}

//! \brief Recipient is the whole device
#define USB_RECIPIENT_DEVICE                0x00

//! \brief Recipient is an interface
#define USB_RECIPIENT_INTERFACE             0x01

//! \brief Recipient is an endpoint
#define USB_RECIPIENT_ENDPOINT              0x02

//! \brief Defines a standard request
#define USB_STANDARD_REQUEST                0x00

//! \brief Defines a class request
#define USB_CLASS_REQUEST                   0x01

//! \brief Defines a vendor request
#define USB_VENDOR_REQUEST                  0x02



#ifdef __cplusplus
}
#endif    
#endif /*USB_STD_H_*/

