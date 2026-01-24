/*
 * Stubs for functions called by shared code but not needed in unit tests
 */

#include <stdarg.h>

/* Com_Printf stub - shared code (mathlib.c, string.c) may call this */
void Com_Printf(int flags, const char *fmt, ...)
{
	(void)flags;
	(void)fmt;
	/* Silent stub for unit tests */
}
