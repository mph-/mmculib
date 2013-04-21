/** @file   tcm8230.h
    @author M. P. Hayes, UCECE
    @date   21 Apr 2013
    @brief  Simple TCM8230 driver.
*/
#ifndef TCM8230_H
#define TCM8230_H

void tcm8230_init (void);

uin32_t tcm8230_capture (uint16_t *image, uint32_t bytes);

#endif
