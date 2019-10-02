/******************************************************************************
 * File: dave_base_linux.c
 *
 * Created: 2014-04
 * Author: CTh
 *
 * Description:
 * 	Linux implementation of the DAVE2D Level 1 interface, relying on the
 * 	DAVE2D kernel module.
 *
 *****************************************************************************/
#include "dave_base_linux.h"
#include "dave_base.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>

#include <signal.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "tes_dave.h"
#include "memmgr.h"

#define REG_WRITE_DRIVER(b,x,y) ioctl(b, DAVE2D_IOCTL_WREG(x), y)
#define REG_READ_DRIVER(b,x,y) ioctl(b, DAVE2D_IOCTL_RREG(x), y)

/* copied status register IRQ defines to drop d2_ dependency */
#define D2C_IRQ_DLIST (1 << 5)
#define D2C_IRQ_ENUM (1 << 4)


/**
 * Function: getclock_us
 *
 * Returns the current system clock time in microseconds.
 *
 * Parameters:
 * 	None.
 *
 * Returns:
 * 	The current system clock time in microseconds.
 */
static unsigned long getclock_us()
{
	struct timespec time1;
	unsigned long t_us;

	clock_gettime(CLOCK_MONOTONIC_RAW, &time1);
	t_us = (time1.tv_nsec/1000) + (time1.tv_sec*1000000);

	return t_us;
}


/**
 * Function: d1_getsettings
 *
 * Gets DAVE's memory settings, in particular the physical memory addresses of
 * the Video RAM and its span.
 *
 * Parameters:
 * 	handle  - device handle (see: <d1_opendevice>)
 * 	dset	- Ptr to a dave2d_settings structure, that will be filled with the
 * 			  DAVE2D parameters.
 */
static void d1_getsettings( d1_device *handle, dave2d_settings *dset)
{
	ioctl(DAVE2D_H(handle)->fd, DAVE2D_IOCTL_GET_SETTINGS, (unsigned long) dset);
}


/**
 * Function: d1_setupmemory
 *
 * Sets up the Video RAM manager.
 *
 * Parameters:
 * 	handle  - device handle (see: <d1_opendevice>)
 */
static void d1_setupmemory( d1_device *handle )
{
	dave2d_settings dset;

	d1_getsettings(handle, &dset);

	DAVE2D_H(handle)->mem = mmap(NULL, dset.mem_span, PROT_READ|PROT_WRITE, MAP_SHARED, DAVE2D_H(handle)->fd, 0);
	DAVE2D_H(handle)->mem_mapping = ((unsigned long)DAVE2D_H(handle)->mem) - dset.mem_base_phys;

	memmgr_initheapmem(&DAVE2D_H(handle)->mmgr, DAVE2D_H(handle)->mem, dset.mem_span + 1);
}


d1_device * d1_opendevice( long flags )
{
	int fd;
	_d1_device *d;

	fd = open(DAVE2D_DEV_NODE_NAME, O_RDWR);
	if(!fd)
		return 0;

	d = malloc(sizeof(_d1_device));
	if(!d)
		return 0;

	d->fd = fd;

	d1_setupmemory(d);

	return d;
}


int d1_closedevice( d1_device *handle )
{
	if(!handle)
		return 0;

	if(close(DAVE2D_H(handle)->fd))
		return 0;

	return 1;
}


void d1_setregister( d1_device *handle, int deviceid, int index, long value )
{
	REG_WRITE_DRIVER(DAVE2D_H(handle)->fd, index, value);
}


long d1_getregister( d1_device *handle, int deviceid, int index )
{
	long temp;

	REG_READ_DRIVER(DAVE2D_H(handle)->fd, index, &temp);

	return temp;
}


int d1_devicesupported( d1_device *handle, int deviceid )
{
	/* We need only support for DAVE2D nowadays */
	switch (deviceid) {
		case D1_DAVE2D:
			return 1;
			break;
		default:
			break;
	}
	return 0;
}


unsigned int d1_memsize(void * ptr)
{
	return malloc_usable_size(ptr);
}


void * d1_allocmem(unsigned int size )
{
	void *ptr = malloc(size);
	return ptr;
}


void d1_freemem(void *ptr )
{
	free(ptr);
}


void * d1_allocvidmem( d1_device *handle, int memtype, unsigned int size )
{
	void *ptr = d1_maptovidmem(handle, memmgr_alloc(&DAVE2D_H(handle)->mmgr, size));
	return ptr;
}


void d1_freevidmem( d1_device *handle, int memtype, void *ptr )
{
	memmgr_free(&DAVE2D_H(handle)->mmgr, d1_mapfromvidmem(handle, ptr));
}


int d1_copytovidmem( d1_device *handle, void *dst, const void *src, unsigned int size, int flags )
{
	void *p = memcpy(d1_mapfromvidmem(handle, dst), src, size);

	return (int) p;
}


int d1_copyfromvidmem( d1_device *handle, void *dst, const void *src, unsigned int size, int flags )
{
	void *p = memcpy(dst, d1_mapfromvidmem(handle, src), size);

	return (int) p;
}


void * d1_maptovidmem( d1_device *handle, void *ptr )
{
	return (void*)(((unsigned long)ptr) - DAVE2D_H(handle)->mem_mapping);
}


void * d1_mapfromvidmem( d1_device *handle, void *ptr )
{
	return (void*)(((unsigned long)ptr) + DAVE2D_H(handle)->mem_mapping);
}


int d1_queryarchitecture( d1_device *handle )
{
	/**
	 * Caching was disabled in the kernel module. Furthermore, DAVE has its
	 * own Video RAM using physical addresses, which aren't directly accessible
	 * by a process in user space. Hence, the memory has to be mapped first.
	 */
	return d1_ma_uncached|d1_ma_separated;
}


int d1_cacheblockflush( d1_device *handle, int memtype, const void *ptr, unsigned int size )
{
	return 1;
}


int d1_queryirq( d1_device *handle, int irqmask, int timeout )
{
	int irq=0;

	while(1)
	{
		read(DAVE2D_H(handle)->fd, &irq, sizeof(irq));

		if(irqmask&d1_irq_dlist)
		{
			if(irq&D2C_IRQ_DLIST)
			{
				irqmask = d1_irq_dlist;
				break;
			}
		}

		if(irqmask&d1_irq_enum)
		{
			if(irq&D2C_IRQ_ENUM)
			{
				irqmask = d1_irq_enum;
				break;
			}
		}
	}

	return irqmask;
}



void d1_timerreset( d1_device *handle )
{
	DAVE2D_H(handle)->time = getclock_us();
}


unsigned long d1_timervalue( d1_device *handle )
{
	unsigned long val;

	val = getclock_us() - DAVE2D_H(handle)->time;

	return val;
}



unsigned long d1_deviceclkfreq( d1_device *handle, int deviceid )
{
	/* TODO: Use kernel clock management or at least device tree for this */
	return 66666666;
}
