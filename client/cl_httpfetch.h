/*
Copyright (C) 2025-2026 EGL Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef CL_HTTPFETCH_H
#define CL_HTTPFETCH_H

#include "shared.h"

// Fetches a URL into memory.
// On success, *outData is allocated with Mem_Alloc and must be freed with Mem_Free.
qBool CL_HTTPFetch(const char *url, byte **outData, size_t *outLen, char *errBuf, size_t errBufSize);

#endif
