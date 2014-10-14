

#ifndef _COMPAT_WIN_H_
#define _COMPAT_WIN_H_

#ifdef WIN32

#ifdef __cplusplus
#  include <ctime>
#else
#  include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

time_t timegm_win(struct tm *tm);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIN32 */

#endif /* !_COMPAT_WIN_H_ */
