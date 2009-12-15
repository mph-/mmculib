#include "dialog.h"
#include <string.h>

/* This supports only a single dialog instance to reduce programming clutter.  
   It is designed for simple LCD displays.  */



typedef struct
{
    uint8_t rows;
    uint8_t cols;
    void (*display)(uint8_t row, const char *str);
    dialog_t *current;
} dialog_data_t;

static dialog_data_t dialog_data;


/* Display a new dialog, saving previous dialog to return to later.  */
void
dialog_display (dialog_t *dialog, const char *string)
{
    int i;
    int rows;
    int cols;
    char buffer[64];
    char *str;

    rows = 1;
    cols = 0;
    for (i = 0; string[i]; i++)
    {
        if (string[i] == '\n')
        {
            rows++;
            cols = 0;
        }
        else
        {
            cols++;
            if (cols > dialog_data.cols)
            {
                rows++;
                cols = 0;
            }
        }
    }
    dialog_data.display (0, string);

    for (; rows < dialog_data.rows; rows++)
        dialog_data.display (rows - 1, "\n");        

    cols = dialog_data.cols 
        - (strlen (dialog->left_name) + strlen (dialog->right_name));
    
    str = buffer;
    for (i = 0; dialog->left_name[i]; i++)
        *str++ = dialog->left_name[i];
    for (i = 0; i < cols; i++)
        *str++ = ' ';
    for (i = 0; dialog->right_name[i]; i++)
        *str++ = dialog->right_name[i];    
    *str = '\0';
    dialog_data.display (rows - 1, buffer);        

    dialog_data.current = dialog;


}


void 
dialog_right (void)
{
    if (dialog_data.current->right_action)
        dialog_data.current->right_action ();
}


void 
dialog_left (void)
{
    if (dialog_data.current->left_action)
        dialog_data.current->left_action ();
}


void 
dialog_init (uint8_t rows, uint8_t cols,
             void (*display)(uint8_t row, const char *message))
{
    dialog_data.rows = rows;
    dialog_data.cols = cols;
    dialog_data.display = display;
}
