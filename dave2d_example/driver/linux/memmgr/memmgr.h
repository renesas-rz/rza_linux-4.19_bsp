//--------------------------------------------------------------------------
// Project: D/AVE
// File:    dave_d0_mm_fixed_range.h (%version: 1 %)
//
// Description:
//  Memory management in a fixed range of memory
//  %date_modified: Wed Jan 31 13:56:41 2007 %  (%derived_by:  hh74036 %)
//
// Changes:
//   2006-11-21 MGe start
//	 2014-05-15 CTh adapted to Linux port and memory alignment requirements
//

#ifndef __DAVE_D0_MM_FIXED_RANGE_H
#define __DAVE_D0_MM_FIXED_RANGE_H
#ifdef __cplusplus
extern "C" {	
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#include <stdlib.h>

#define MEMMGR_SIZE_ALIGN 16
#define MEMMGR_PADDING(x) (MEMMGR_SIZE_ALIGN - ((x-1)%MEMMGR_SIZE_ALIGN + 1))

typedef struct {
  void *base;
  void *end;  /* last valid address in heap*/
  char _padding[MEMMGR_PADDING(2*sizeof(void*))];
} d0_heap;

typedef struct _memblk {
  struct _memblk *lastblk;
  unsigned int size;
  char _padding[MEMMGR_PADDING(sizeof(void*) + sizeof(unsigned int))];
} memblk;

typedef struct {
  d0_heap *ctrlblk;
} memmgr;

//---------------------------------------------------------------------------
void * memmgr_alloc(memmgr *mmgr, size_t size);
unsigned int memmgr_free(memmgr *mmgr, void *ptr);
unsigned int memmgr_msize(memmgr *mmgr, void *ptr);
void memmgr_initheapmem(memmgr *mmgr, void *base, size_t size );
void memmgr_exitheapmem(memmgr *mmgr);
//---------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
#endif
