

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "src/log/log_c.h"


/* ========================================================================= */
/*
 * Debugging using Qt-defined output.
 */
void q_debug_call(const char *fmt, ...)
/* ========================================================================= */
{
	va_list argp;

	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);

	va_end(argp);
}


/* ========================================================================= */
/*
 * Get debug verbosity.
 */
int glob_debug_verbosity(void)
/* ========================================================================= */
{
	return 0;
}


/* ========================================================================= */
/*
 * Log message.
 */
int glob_log(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
{
	(void) source;
	(void) level;

	va_list argp;

	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);

	va_end(argp);

	return 0;
}


/* ========================================================================= */
/*
 * Log multi-line message.
 */
int glob_log_ml(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
{
	(void) source;
	(void) level;

	va_list argp;

	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);

	va_end(argp);

	return 0;
}
