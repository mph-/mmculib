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



void
dialog_display_options (dialog_t *dialog, int rows)
{
    int i;
    int cols;
    char *str;
    char buffer[64];

    for (; rows < dialog_data.rows - 1; rows++)
        dialog_data.display (rows, "\n");        

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


/* Display a new dialog, saving previous dialog to return to later.  */
void
dialog_display (dialog_t *dialog, const char *string)
{
    int i;
    int rows;
    int cols;

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
    if (string[i - 1] == '\n')
        rows--;

    dialog_data.display (0, string);

    dialog_display_options (dialog, rows);
}


/** Return non-zero if the dialog has finished.  */
bool
dialog_right (void)
{
    if (!dialog_data.current->right_action)
        return 1;
    return dialog_data.current->right_action ();
}


/** Return non-zero if the dialog has finished.  */
bool
dialog_left (void)
{
    if (!dialog_data.current->left_action)
        return 1;
    return dialog_data.current->left_action ();
}


void 
dialog_init (uint8_t rows, uint8_t cols,
             void (*display)(uint8_t row, const char *message))
{
    dialog_data.rows = rows;
    dialog_data.cols = cols;
    dialog_data.display = display;
}
