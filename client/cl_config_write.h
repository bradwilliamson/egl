#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CL_ConfigWriteFn)(FILE *f);

// Writes an EGL-style config file to an explicit path.
// Returns non-zero on success.
int CL_WriteConfigFile(const char *path, CL_ConfigWriteFn writeBindings, CL_ConfigWriteFn writeCvars);

#ifdef __cplusplus
}
#endif
