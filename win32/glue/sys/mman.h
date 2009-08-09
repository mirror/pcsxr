//
// Copyright (c) 2008, Wei Mingzhi. All rights reserved.
//
// Use, redistribution and modification of this code is unrestricted
// as long as this notice is preserved.
//
// This code is provided with ABSOLUTELY NO WARRANTY.
//

#ifndef MMAN_H
#define MMAN_H

#include <stdlib.h>

#define mmap(start, length, prot, flags, fd, offset) \
	((unsigned char *)malloc(length))

#define munmap(start, length) do { free(start); } while (0)

#endif
