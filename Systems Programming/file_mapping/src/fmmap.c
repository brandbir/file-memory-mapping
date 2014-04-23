/*
 * fmmap.c
 *
 *  Created on: 23 Apr 2014
 *      Author: brandon
 */

#include "fmmap.h"

void *rmmap(fileloc_t location, off_t offset)
{
	return 0;
}

int rmunmap(void *addr)
{
	return -1;
}

ssize_t mread(void *addr, off_t offset, void *buff, size_t count)
{
	return 0;
}
ssize_t mwrite(void *addr, off_t offset, void *buff, size_t count)
{
	return 0;
}
