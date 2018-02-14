/** @file   menu.h
    @author M. P. Hayes, UCECE
    @date   19 February 2009
    @brief  Menu support.
*/

#ifndef MENU_H
#define MENU_H

#ifdef __cplusplus
extern "C" {
#endif
    

#include "config.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof (ARRAY[0]))
#endif

typedef enum {MENU_STYLE_ROTATE, MENU_STYLE_SCROLL} menu_style_t;

#define MENU_ITEM(NAME, ACTION) {NAME, ACTION}

#define MENU(NAME, MENU_ITEMS) {NAME, MENU_ITEMS, ARRAY_SIZE (MENU_ITEMS), 0, 0, 0}

typedef struct menu_item_struct
{
    const char *name;
    bool (*action)(void);
} menu_item_t;


typedef struct menu_struct
{
    const char *title;
    menu_item_t *items;
    uint8_t size;
    uint8_t index;
    uint8_t top;
    struct menu_struct *parent;
} menu_t;


/* Move to previous menu item.  */
void
menu_prev (void);


/* Move to next menu item.  */
void
menu_next (void);


/* Go to selected menu item.  */
void 
menu_goto (int index);


/* Select current menu item.  */
void
menu_select (void);


/* Quit current menu and return to parent menu.  */
void
menu_quit (void);


/* Display a new menu with prompt at last position.  */
bool
menu_display (menu_t *menu);


/* Display a new menu with prompt at top.  */
bool
menu_display_top (menu_t *menu);


/* Display current menu.  */
void
menu_show (void);


/* Get current index for menu; useful for saving menu options.  */
static inline uint8_t
menu_index_get (menu_t *menu)
{
    return menu->index;
}


static inline const char *
menu_title_get (menu_t *menu)
{
    return menu->title;
}


static inline const char *
menu_item_name_get (menu_t *menu, int item)
{
    return menu->items[item].name;
}


uint8_t
menu_current_index_get (void);


/* Set current index for menu and execute action; useful for restoring
   menu options.  */
void
menu_index_set (menu_t *menu, uint8_t index);


menu_t *
menu_current_get (void);


void 
menu_init (int rows,
           void (*display)(const char *title, int row,
                           const char *item_name, bool highlight));


#ifdef __cplusplus
}
#endif    
#endif

