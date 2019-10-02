/******************************************************************************
 * File:
 * 	dave_base_linux.c
 *
 * Created:
 * 	2014-04-10
 *
 * Author:
 * 	CTh
 *
 * Description:
 * 	Library specific defines.
 *
 *****************************************************************************/
#ifndef DAVE_BASE_LINUX_H_
#define DAVE_BASE_LINUX_H_

#include <time.h>
#include "memmgr.h"

#define DAVE2D_DEV_NODE_NAME "/dev/dave2d"

#define PAGE_SIZE (sysconf(_SC_PAGE_SIZE))

// The DAVE2D hardware handle for the linux driver implementation
typedef struct {
	int fd;
	void *mem;
	unsigned long mem_mapping;
	memmgr mmgr;
	time_t time;
} _d1_device;

#define DAVE2D_H(x) ((_d1_device*) x)

#endif /* DAVE_BASE_LINUX_H_ */
