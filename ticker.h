/** @file   ticker.h
    @author M. P. Hayes, UCECE
    @date   2 April 2007
    @brief 
*/
#ifndef TICKER_H
#define TICKER_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

typedef struct
{
    uint16_t period;
    uint16_t clock;
} ticker_t;


typedef struct
{
    uint16_t period;
    uint16_t clock;
} ticker16_t;


typedef struct
{
    uint8_t period;
    uint8_t clock;
} ticker8_t;


#define TICKER_INIT(DEV, PERIOD)        \
    (DEV)->period = (PERIOD);           \
    (DEV)->clock = (DEV)->period;


/* Return non-zero when the ticker rolls over otherwise return 0.  */
#define TICKER_UPDATE(DEV)              \
    (--(DEV)->clock ? 0 : ((DEV)->clock = (DEV)->period))


#define TICKER_START(DEV)               \
    (DEV)->clock = (DEV)->period;


#ifdef __cplusplus
}
#endif    
#endif

