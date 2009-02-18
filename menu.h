#include "config.h"
#include <stdio.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof (ARRAY[0]))
#endif

typedef enum {MENU_STYLE_ROTATE, MENU_STYLE_SCROLL} menu_style_t;

#define MENU_ITEM(NAME, ACTION) {(NAME), (ACTION)}

#define MENU(NAME, MENU_ITEMS) {(NAME), (MENU_ITEMS), ARRAY_SIZE (MENU_ITEMS), 0, 0, 0}

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
    uint8_t pointer;
    struct menu_struct *parent;
} menu_t;


typedef int menu_iter_t;


extern void
menu_prev (void);


extern void
menu_next (void);


extern void
menu_do (void);


extern void
menu_quit (void);


extern void 
menu_select (int index);


extern bool
menu_display (menu_t *menu);


extern void 
menu_init (menu_t *menu, int index, int rows,
           void (*display)(const char *title, int row,
                           const char *item_name, bool highlight));

