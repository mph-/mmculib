#ifndef TTY_H
#define TTY_H

#include "config.h"
#include "sys.h"

#ifndef TTY_INPUT_BUFFER_SIZE
#define TTY_INPUT_BUFFER_SIZE 80
#endif

#ifndef TTY_OUTPUT_BUFFER_SIZE
#define TTY_OUTPUT_BUFFER_SIZE 1024
#endif

struct tty_cfg_struct
{
    bool (*update)(void);
    void (*shutdown)(void);
    sys_read_t read;
    sys_write_t write;
};


typedef struct tty_struct tty_t;

typedef struct tty_cfg_struct tty_cfg_t;


int
tty_getc (tty_t *tty);


char *
tty_gets (tty_t *tty, char *buffer, int size);


int
tty_printf (tty_t *tty, const char *fmt, ...);


bool 
tty_poll (tty_t *tty);


tty_t *
tty_init (tty_cfg_t *cfg, void *dev);


void
tty_shutdown (tty_t *tty);

#endif
