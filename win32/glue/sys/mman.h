//
// Copyright (c) 2008, Wei Mingzhi. All rights reserved.
//
// Use, redistribution and modification of this code is unrestricted
// as long as this notice is preserved.
//

#ifndef MMAN_H
#define MMAN_H

#define PROT_WRITE      0
#define PROT_READ       0
#define PROT_EXEC       0
#define MAP_PRIVATE     0
#define MAP_ANONYMOUS   0

#include <stdlib.h>

static inline void *mmap(void *start, size_t length, int prot, int flags,
                  int fd, off_t offset)
{
	return (unsigned char *)malloc(length);
}

static inline int munmap(void *start, size_t length)
{
	free(start);
	return 0;
}

#endif
