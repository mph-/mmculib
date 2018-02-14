/** @file   dialog.h
    @author M. P. Hayes, UCECE
    @date   19 February 2009
    @brief  Dialog support.
*/

#ifndef DIALOG_H
#define DIALOG_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof (ARRAY[0]))
#endif


#define DIALOG(LEFT_NAME, LEFT_ACTION, RIGHT_NAME, RIGHT_ACTION) \
    {(LEFT_NAME), (LEFT_ACTION), (RIGHT_NAME), (RIGHT_ACTION)}


typedef struct dialog_struct
{
    const char *left_name;
    bool (*left_action)(void);
    const char *right_name;
    bool (*right_action)(void);
} dialog_t;


bool
dialog_left (void);


bool
dialog_right (void);


/* Display dialog options.  */
void
dialog_display_options (dialog_t *dialog, int rows);


/* Display a new dialog.  */
void
dialog_display (dialog_t *dialog, const char *message);


void 
dialog_init (uint8_t rows, uint8_t cols,
             void (*display)(uint8_t row, const char *str));


#ifdef __cplusplus
}
#endif    
#endif

