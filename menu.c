#include "menu.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif


typedef struct
{
    menu_t *current;
    uint8_t rows;
    menu_style_t style;
    void (*display)(const char *title, int row, 
                    const char *item_name, bool highlight);
} menu_data_t;

static menu_data_t menu_data;


void 
menu_show (void)
{
    /* Probably need a callback here.  */
    int i;
    uint8_t top;
    menu_t *menu = menu_data.current;

    if (menu_data.style == MENU_STYLE_ROTATE)
    {
        /* Rotate mode with pointer not moving  */
        menu->pointer = 0;
    }

    top = menu->index - menu->pointer;
    for (i = 0; i < menu_data.rows; i++)
    {
        uint8_t item;

        if (i >= menu->size)
            break;

        item = (top + i) % menu->size;

        menu_data.display (menu->title, i, menu->items[item].name,
                           i == menu->pointer);
    }
}


/* Display a new menu, saving previous menu to return to later.  */
bool
menu_display (menu_t *menu, int index)
{
    menu->parent = menu_data.current;
    menu_data.current = menu;
    menu->index = index;
    menu->pointer = 0;
    menu_show ();
    return 0;
}


/* Quit the current menu and return to parent menu.  */
void 
menu_quit (void)
{
    /* Can't exit main menu.  */
    if (!menu_data.current->parent)
        return;

    menu_data.current = menu_data.current->parent;
    menu_show ();
}


/* Select an item in a menu.  */
void 
menu_select (int index)
{
    if (index > menu_data.current->size)
        index = menu_data.current->size - 1;
    else if (index < 0)
        index = 0;

    menu_data.current->index = index;
    menu_show ();
}


/* Perform action for current menu item.  */
void 
menu_do (void)
{
    if (!menu_data.current->items[menu_data.current->index].action
        || menu_data.current->items[menu_data.current->index].action ())
        menu_quit ();
}


/* Select next menu item.  */
void 
menu_next (void)
{
    if (menu_data.current->index >= menu_data.current->size - 1)
    {
        menu_data.current->index = 0;
        menu_data.current->pointer = 0;
    }
    else
    {
        menu_data.current->index++;
        menu_data.current->pointer++;
        if (menu_data.current->pointer >= min (menu_data.rows, menu_data.current->size))
            menu_data.current->pointer = min (menu_data.rows, menu_data.current->size) - 1;
    }

    menu_show ();
}


/* Select previous menu item.  */
void 
menu_prev (void)
{
    if (menu_data.current->index == 0)
    {
        menu_data.current->index = menu_data.current->size - 1;
        menu_data.current->pointer = min (menu_data.rows, menu_data.current->size) - 1;
    }
    else
    {
        menu_data.current->index--;
        if (menu_data.current->pointer)
            menu_data.current->pointer--;
    }

    menu_show ();
}


void menu_style_set (menu_style_t style)
{
    menu_data.style = style;
}


void 
menu_init (menu_t *menu, int index, int rows,
           void (*display)(const char *title, int row,
                           const char *item_name, bool highlight))
{
    menu_data.current = menu;
    menu_data.rows = rows;
    menu_data.style = MENU_STYLE_SCROLL;
    menu_data.display = display;
    menu->index = index;
    menu_show ();
}
