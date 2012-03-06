#include <stdarg.h>
#include <stdio.h>

static FILE *tracelog_file;

void
tracelog_printf (const char *fmt, ...)
{
    va_list ap;

    if (!tracelog_file)
        return;
    
    va_start (ap, fmt);
    vfprintf (tracelog_file, fmt, ap);
    va_end (ap);
}


void 
tracelog_init (const char *filename)
{
    tracelog_file = fopen (filename, "a");
    tracelog_printf ("\n>>>\n");
}


void
tracelog_flush (void)
{
    if (!tracelog_file)
        return;
    fflush (tracelog_file);
}


void 
tracelog_close (void)
{
    if (tracelog_file)
    {
        fclose (tracelog_file);
        tracelog_file = 0;
    }
}
