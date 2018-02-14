#ifndef USB_TRACE_H
#define USB_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"
#include "trace.h"


/* These default macros gobble their arguments.  For tracing use
   #define TRACE_USB_BOT_ERROR TRACE_PRINTF in config.h
*/

#ifndef TRACE_USB_MSD_ERROR
#define TRACE_USB_MSD_ERROR(...)
#endif


#ifndef TRACE_USB_MSD_INFO
#define TRACE_USB_MSD_INFO(...)
#endif


#ifndef TRACE_USB_MSD_DEBUG
#define TRACE_USB_MSD_DEBUG(...)
#endif


#ifndef TRACE_USB_BOT_ERROR
#define TRACE_USB_BOT_ERROR(...)
#endif


#ifndef TRACE_USB_BOT_INFO
#define TRACE_USB_BOT_INFO(...)
#endif


#ifndef TRACE_USB_BOT_DEBUG
#define TRACE_USB_BOT_DEBUG(...)
#endif



#ifndef TRACE_USB_MSD_SBC_ERROR
#define TRACE_USB_MSD_SBC_ERROR(...)
#endif


#ifndef TRACE_USB_MSD_SBC_INFO
#define TRACE_USB_MSD_SBC_INFO(...)
#endif


#ifndef TRACE_USB_MSD_SBC_DEBUG
#define TRACE_USB_MSD_SBC_DEBUG(...)
#endif


#ifndef TRACE_USB_MSD_LUN_ERROR
#define TRACE_USB_MSD_LUN_ERROR(...)
#endif


#ifndef TRACE_USB_MSD_LUN_INFO
#define TRACE_USB_MSD_LUN_INFO(...)
#endif


#ifndef TRACE_USB_MSD_LUN_DEBUG
#define TRACE_USB_MSD_LUN_DEBUG(...)
#endif



#ifdef __cplusplus
}
#endif    
#endif


