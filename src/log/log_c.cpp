

#include <cstdarg>
#include <cstddef> /* NULL */
#include <cstdint>

#include "src/log/log.h"
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

	return qDebugCallV(fmt, argp);

	va_end(argp);
}


/* ========================================================================= */
/*
 * Get debug verbosity.
 */
int glob_debug_verbosity(void)
/* ========================================================================= */
{
	return globLog.debugVerbosity();
}


/* ========================================================================= */
/*
 * Log message.
 */
int glob_log(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
{
	va_list argp;

	va_start(argp, fmt);

	globLog.logVlog(source, level, fmt, argp);

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
	va_list argp;

	va_start(argp, fmt);

	globLog.logVlogMl(source, level, fmt, argp);

	va_end(argp);

	return 0;
}
