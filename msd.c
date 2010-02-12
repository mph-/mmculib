#include "msd.h"


msd_size_t 
msd_read (msd_t *msd, msd_addr_t addr, void *buffer, msd_size_t size)
{
    return msd->read (msd->handle, addr, buffer, size);
}


msd_size_t
msd_write (msd_t *msd, msd_addr_t addr, const void *buffer, msd_size_t size)
{
    return msd->write (msd->handle, addr, buffer, size);

}


msd_status_t
msd_status_get (msd_t *msd)
{
    return msd->status_get (msd->handle);
}


void
msd_shutdown (msd_t *msd)
{
    if (msd->shutdown)
        msd->shutdown (msd->handle);
}
