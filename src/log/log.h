
#ifndef _LOG_H_
#define _LOG_H_


#include <QMutex>
#include <QString>
#include <QtGlobal>


/*!
 * @brief Message output function.
 */
void globalLogOutput(QtMsgType type, const QMessageLogContext &context,
    const QString &msg);


/*!
 * @brief Maximal number of simultaneously opened files.
 */
#define MAX_LOG_FILES 64


/*!
 * @brief Maximal number of sources to write to log facility.
 */
#define MAX_SOURCES 64


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

	int m_debugVerbosity; /*!< Level verbosity of debugging output. */

	/*!
	 * @brief Converts log level to urgency prefix.
	 */
	static
	const char * urgencyPrefix(uint8_t level);

	/*!
	 * @brief Converts message type to printable string.
	 */
	static
	const char * msgTypeCstr(QtMsgType type);

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
};


extern GlobLog globLog; /*!< Global log facility. */


#endif /* _LOG_H_ */
