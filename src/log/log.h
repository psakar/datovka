
#ifndef _LOG_H_
#define _LOG_H_


#include <cstdarg>
#include <QMutex>
#include <QString>

#include "src/log/log_common.h"


/*!
 * @brief Message output function.
 */
void globalLogOutput(QtMsgType type, const QMessageLogContext &context,
    const QString &msg);


/*!
 * @brief Debugging using Qt-defined output.
 *
 * @param[in] fmt Format string.
 */
void qDebugCall(const char *fmt, ...);


/*!
 * @brief Debugging using Qt-defined output.
 *
 * @param[in] fmt Format string.
 */
void qDebugCallV(const char *fmt, va_list ap);


/*!
 * @brief Generates location debug information.
 */
#ifdef DEBUG
#  define debugSlotCall() \
	do { \
		/* qDebugCall("<SLOT> %s() '%s'", __func__, __FILE__); */ \
		logDebugLv1NL("<SLOT> %s() '%s'", __func__, __FILE__); \
	} while(0)
#else
   /* Forces the semicolon after the macro. */
#  define debugSlotCall() do {} while(0)
#endif


/*!
 * @brief Generates location debug information.
 */
#ifdef DEBUG
#  define debugFuncCall() \
	do { \
		/* qDebugCall("<FUNC> %s() '%s'", __func__, __FILE__); */ \
		logDebugLv2NL("<FUNC> %s() '%s'", __func__, __FILE__); \
	} while(0)
#else
   /* Forces the semicolon after the macro. */
#  define debugFuncCall() do {} while(0)
#endif


/*!
 * @brief Maximal number of simultaneously opened files.
 */
#define MAX_LOG_FILES 64


/*!
 * @brief Maximal number of sources to write to log facility.
 */
#define MAX_SOURCES 64


/*!
 * @brief Global logger class.
 */
class GlobLog {
public:
	enum LogFac {
		LF_SYSLOG = 0, /*!< @brief Syslog facility. */
		LF_STDOUT = 1, /*!< @brief Stdout log target. */
		LF_STDERR = 2, /*!< @brief Stderr log target. */
		LF_FILE = 3 /*!< @brief File log target. */
	};

	enum LogMode {
		LM_WRONLY, /*!< File will be overwritten. */
		LM_APPEND, /*!< New content will be appended. */
	};

	struct FacDesc {
		uint8_t levels[MAX_SOURCES]; /*!< Up to eight syslog levels. */
		FILE *fout; /*!< Output file. */
	};

	GlobLog(void);
	~GlobLog(void);

	/*!
	 * @brief Get log verbosity.
	 *
	 * @return Log verbosity.
	 */
	int logVerbosity(void);

	/*!
	 * @brief Set log verbosity.
	 *
	 * @param[in] verb Verbosity level.
	 */
	void setLogVerbosity(int verb);

	/*!
	 * @brief Get debug verbosity.
	 *
	 * @return Debug verbosity.
	 */
	int debugVerbosity(void);

	/*!
	 * @brief Set debug verbosity.
	 *
	 * @param[in] verb Verbosity to be set.
	 */
	void setDebugVerbosity(int verb);

	/*!
	 * @brief Opens a log file as a logging facility.
	 *
	 * @param[in] fName File name.
	 * @param[in] mode  Open mode.
	 * @return -1 on error, facility id else.
	 */
	int openFile(const QString &fName, LogMode mode);

	/*!
	 * @brief Returns the log levels for the given facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source   Source identifier.
	 * @return Levels for the selected facility and identifier.
	 */
	uint8_t logLevels(int facility, int source);

	/*!
	 * @brief Sets the log levels for the selected facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source   Source identifier.
	 * @param[in] levels   Levels to be set.
	 */
	void setLogLevels(int facility, int source, uint8_t levels);

	/*!
	 * @brief Add log levels to the selected facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source   Source identifier.
	 * @param[in] levels   Log levels to be added.
	 */
	void addLogLevels(int facility, int source, uint8_t levels);

	/*!
	 * @brief Returns the id of a new unique source that can be used.
	 *
	 * @return -1 when error, id of a new unique source identifier else.
	 *
	 * @note After this function returns, the returned source id is
	 * considered to be used. It cannot be returned as unused.
	 */
	int acquireUniqueLogSource(void);

	/*!
	 * @brief Log message.
	 *
	 * @param[in] source Source identifier.
	 * @param[in] level  Message urgency level.
	 * @param[in] fmt    Format of the log message -- follows printf(3)
	 *     format.
	 * @return -1 if error, 0 else.
	 */
	int log(int source, uint8_t level, const char *fmt, ...);

	/*!
	 * @brief Log message.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level  Message urgency level.
	 * @param[in]     fmt    Format of the log message -- follows printf(3)
	 *     format.
	 * @param[in,out] ap     Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	int logVlog(int source, uint8_t level, const char *fmt, va_list ap);

	/*!
	 * @brief Log multi-line message.
	 *
	 * Every new line is merged with the same prefix.
	 *
	 * @param[in] source Source identifier.
	 * @param[in] level  Message urgency level.
	 * @param[in] fmt    Format of the log message -- follows printf(3)
	 *     format.
	 * @return -1 if error, 0 else.
	 */
	int logMl(int source, uint8_t level, const char *fmt, ...);

	/*!
	 * @brief Log multi-line message.
	 *
	 * Every new line is merged with the same prefix.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level  Message urgency level.
	 * @param[in]     fmt    Format of the log message -- follows printf(3)
	 *     format.
	 * @param[in,out] ap     Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	int logVlogMl(int source, uint8_t level, const char *fmt, va_list ap);

	friend void globalLogOutput(QtMsgType type,
	    const QMessageLogContext &context, const QString &msg);

private:
	FacDesc facDescVect[MAX_LOG_FILES]; /*!< Facility vector. */
	int usedSources; /*!<
	                   * Number of used sources.
	                   * 0 is the default source id.
	                   */
	int openedFiles; /*!< Number of opened files. */
	QMutex m_mutex; /*!< @brief Mutual exclusion. */
	const QString m_hostName; /*!< @brief Host name. */

	int m_logVerbosity; /*!< Amount of information in single message. */
	int m_debugVerbosity; /*!< Verbosity of debugging output. */

	/*!
	 * @brief Converts log level to urgency prefix.
	 */
	static
	const char * urgencyPrefix(uint8_t level);

	/*!
	 * @brief converts message type to urgency level.
	 *
	 * @param[in] type Message type.
	 * @return Urgency level.
	 */
	static
	uint8_t levelFromType(QtMsgType type);

	/*!
	 * @brief Log message.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level  Message urgency level.
	 * @param[in]     prefix Message prefix.
	 * @param[in]     format Content of the log message in printf(3)
	 *     format.
	 * @param[in,out] ap     Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	void logPrefixVlog(int source, uint8_t level,
	    const char *prefix, const char *format, va_list ap);

	/*!
	 * @brief Log multi-line message.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level  Message urgency level.
	 * @param[in]     prefix Message prefix.
	 * @param[in]     format Content of the log message -- in printf(3)
	 *     format.
	 * @param[in,out] ap     Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	void logPrefixVlogMl(int source, uint8_t level,
	    const char *prefix, const char *format, va_list ap);
};


extern GlobLog globLog; /*!< Global log facility. */


/*!
 * @brief A macro which logs a function name followed with given debugging
 * information. The data are logged only when the global verbosity variable
 * exceeds the given threshold. Automatic newline is added.
 *
 * @param[in] verbThresh Verbosity threshold.
 * @param[in] format     Format of the messages, follows printf syntax.
 * @param[in] ...        Variadic arguments.
 *
 * @note This macro works only if the DEBUG macro is defined. If no DEBUG macro
 * is defined then it won't generate any code.
 */
#if DEBUG
#define logDebugNL(verbThresh, format, ...) \
	if (globLog.debugVerbosity() > verbThresh) { \
		if (globLog.logVerbosity() > 0) { \
			globLog.log(LOGSRC_DEF, LOG_DEBUG, \
			    format " (%s:%d, %s())\n", \
			    __VA_ARGS__, __FILE__, __LINE__, __func__); \
		} else { \
			globLog.log(LOGSRC_DEF, LOG_DEBUG, \
			    format "\n", __VA_ARGS__); \
		} \
	}
#else /* !DEBUG */
#define logDebugNL(verbThresh, format, ...) \
	(void) 0
#endif /* DEBUG */


/*!
 * @brief Logs the debugging information even if the threshold was not set.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv0NL(format, ...) \
	logDebugNL(-1, format, __VA_ARGS__)


/*!
 * @brief Logs the debugging information only if the verbosity exceeds 0.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv1NL(format, ...) \
	logDebugNL(0, format, __VA_ARGS__)


/*!
 * @brief Logs the debugging information only if the verbosity exceeds 1.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv2NL(format, ...) \
	logDebugNL(1, format, __VA_ARGS__)


/*!
 * @brief Logs the debugging information only if the verbosity exceeds 2.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv3NL(format, ...) \
	logDebugNL(2, format, __VA_ARGS__)


/*!
 * @brief Logs information message.
 *
 * @param[in] format Format of the message.
 * @param[in] ...    Varidic arguments.
 */
#define logInfo(format, ...) \
	do { \
		globLog.log(LOGSRC_DEF, LOG_INFO, format, __VA_ARGS__); \
	} while (0)


/*!
 * @brief Logs warning message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logWarning(format, ...) \
	do { \
		globLog.log(LOGSRC_DEF, LOG_WARNING, format, __VA_ARGS__); \
	} while (0)


/*!
 * @brief Logs error message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logError(format, ...) \
	do { \
		globLog.log(LOGSRC_DEF, LOG_ERR, format, __VA_ARGS__); \
	} while (0)


/*!
 * @brief Logs multi-line error message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logErrorMl(format, ...) \
	do { \
		globLog.logMl(LOGSRC_DEF, LOG_ERR, format, __VA_ARGS__); \
	} while (0)


#endif /* _LOG_H_ */
