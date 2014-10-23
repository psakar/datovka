

#ifndef _LOG_COMMON_H_
#define _LOG_COMMON_H_


/*!
 * @brief Identifies the default source.
 */
#define LOGSRC_DEF 0


/*!
 * @brief Identifies all sources.
 */
#define LOGSRC_ANY -1


/* Taken from syslog.h */
#define LOG_EMERG   0 /* system is unusable */
#define LOG_ALERT   1 /* action must be taken immediately */
#define LOG_CRIT    2 /* critical conditions */
#define LOG_ERR     3 /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE  5 /* normal but significant condition */
#define LOG_INFO    6 /* informational */
#define LOG_DEBUG   7 /* debug-level messages */

#define LOG_MASK(pri) (1 << (pri)) /* mask for one priority */
#define LOG_UPTO(pri) ((1 << ((pri)+1)) - 1) /* all priorities through pri */


#endif /* _LOG_COMMON_H_ */
