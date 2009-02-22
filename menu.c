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
menu_display (menu_t *menu)
{
    menu->parent = menu_data.current;
    menu_data.current = menu;
    menu_show ();
    return 0;
}


/* Quit the current menu and return to parent menu.  */
void 
menu_quit (void)
{
    /* Can't exit main menu.  */
    if (!menu_data.current->parent)
    {
        /* Redraw.  */
        menu_show ();
        return;
    }

    menu_data.current = menu_data.current->parent;
    menu_show ();
}


/* Go to an item in a menu.  */
void 
menu_goto (int index)
{
    if (index > menu_data.current->size)
        index = menu_data.current->size - 1;
    else if (index < 0)
        index = 0;

    menu_data.current->index = index;
    menu_data.current->pointer = min (index, menu_data.rows);
    menu_show ();
}


void
menu_index_set (menu_t *menu, uint8_t index)
{
    /* Handle bogus values.  */
    if (index >= menu->size)
        index = 0;

    menu->index = index;
    menu->pointer = min (index, menu_data.rows);

    if (menu->items[menu->index].action)
        menu->items[menu->index].action ();
}


/* Perform action for current menu item.  */
void 
menu_select (void)
{
    if (!menu_data.current->items[menu_data.current->index].action
        || menu_data.current->items[menu_data.current->index].action ())
        menu_quit ();
}


/* Go to next menu item.  */
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


/* Go to previous menu item.  */
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


menu_t *
menu_current_get (void)
{
    return menu_data.current;
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
}
