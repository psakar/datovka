/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#pragma once

#include <cstdarg>
#include <cstdio>
#include <QMessageLogContext>
#include <QMutex>
#include <QString>

/*!
 * @brief Maximal number of simultaneously opened files.
 */
#define MAX_LOG_FILES 64

/*!
 * @brief Maximal number of sources to write to log facility.
 */
#define MAX_SOURCES 64

/*!
 * @brief Logging device class.
 */
class LogDevice {
public:
	/*!
	 * @brief Specifies target logging facility.
	 */
	enum LogFac {
		LF_SYSLOG = 0, /*!< @brief Syslog facility. */
		LF_STDOUT = 1, /*!< @brief Stdout log target. */
		LF_STDERR = 2, /*!< @brief Stderr log target. */
		LF_FILE = 3 /*!<
		             * @brief File log target.
		             * The first opened file is identified by facility
		             * number LF_FILE. The second opened file
		             * is identified by facility number (LF_FILE + 1).
		             */
	};

	/*!
	 * @brief Mode to open log files.
	 */
	enum LogMode {
		LM_WRONLY, /*!< File will be overwritten. */
		LM_APPEND, /*!< New content will be appended. */
	};

	/*!
	 * @brief Log facility descriptor.
	 */
	struct FacDesc {
		quint8 levels[MAX_SOURCES]; /*!< Up to eight syslog levels. */
		std::FILE *fout; /*!< Output file. */
	};

	/*!
	 * @brief Constructor.
	 */
	LogDevice(void);

	/*!
	 * @brief Destructor.
	 */
	~LogDevice(void);

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
	 * @param[in] mode Open mode.
	 * @return -1 on error, facility id else.
	 */
	int openFile(const QString &fName, enum LogMode mode);

	/*!
	 * @brief Returns the log levels for the given facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source Source identifier.
	 * @return Levels for the selected facility and identifier.
	 */
	quint8 logLevels(int facility, enum LogSource source);

	/*!
	 * @brief Sets the log levels for the selected facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source Source identifier.
	 * @param[in] levels Levels to be set.
	 */
	void setLogLevels(int facility, enum LogSource source, quint8 levels);

	/*!
	 * @brief Add log levels to the selected facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source Source identifier.
	 * @param[in] levels Log levels to be added.
	 */
	void addLogLevels(int facility, enum LogSource source, quint8 levels);

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
	 * @param[in] level Message urgency level.
	 * @param[in] fmt Format of the log message -- follows printf(3) format.
	 * @return -1 if error, 0 else.
	 */
	int log(enum LogSource source, enum LogLevel level, const char *fmt,
	    ...);

	/*!
	 * @brief Log message.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level Message urgency level.
	 * @param[in]     fmt Format of the log message -- follows printf(3)
	 *     format.
	 * @param[in,out] ap Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	int logVlog(enum LogSource source, enum LogLevel level, const char *fmt,
	    std::va_list ap);

	/*!
	 * @brief Log multi-line message.
	 *
	 * Every new line is merged with the same prefix.
	 *
	 * @param[in] source Source identifier.
	 * @param[in] level Message urgency level.
	 * @param[in] fmt Format of the log message -- follows printf(3) format.
	 * @return -1 if error, 0 else.
	 */
	int logMl(enum LogSource source, enum LogLevel level, const char *fmt,
	    ...);

	/*!
	 * @brief Log multi-line message.
	 *
	 * Every new line is merged with the same prefix.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level Message urgency level.
	 * @param[in]     fmt Format of the log message in printf(3) format.
	 * @param[in,out] ap Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	int logVlogMl(enum LogSource source, enum LogLevel level,
	    const char *fmt, std::va_list ap);

	/*!
	 * @brief Provides message handler interface for the Qt library.
	 *
	 * @param[in] type Severity level.
	 * @param[in] context Log message context.
	 * @param[in] msg Log message.
	 */
	void logQtMessage(enum QtMsgType type,
	    const QMessageLogContext &context, const QString &msg);

private:
	FacDesc m_facDescVect[MAX_LOG_FILES]; /*!< Facility vector. */
	int m_usedSources; /*!<
	                    * Number of used sources.
	                    * 0 is the default source id.
	                    */
	int m_openedFiles; /*!< Number of opened files. */
	QMutex m_mutex; /*!< @brief Mutual exclusion. */ /* TODO -- mutable */
	const QString m_hostName; /*!< @brief Host name. */

	int m_logVerbosity; /*!< Amount of information in single message. */
	int m_debugVerbosity; /*!< Verbosity of debugging output. */

	static
	const QString logTimeFmt; /*!< Format of time to be used in log output. */

	/*!
	 * @brief Converts log level to urgency prefix.
	 */
	static
	const char *urgencyPrefix(enum LogLevel level);

	/*!
	 * @brief converts message type to urgency level.
	 *
	 * @param[in] type Message type.
	 * @return Urgency level.
	 */
	static
	enum LogLevel levelFromType(enum QtMsgType type);

	/*!
	 * @brief Build log line prefix string. Its form depends on
	 *     the verbosity value.
	 *
	 * @param[in] urgPrefix Urgency prefix.
	 * @return Prefix string.
	 */
	QString buildPrefix(const char *urgPrefix) const;

	/*!
	 * @brief Log message.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level Message urgency level.
	 * @param[in]     urgPrefix Message urgency prefix.
	 * @param[in]     format Content of the log message in printf(3) format.
	 * @param[in,out] ap Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	void logPrefixVlog(enum LogSource source, enum LogLevel level,
	    const char *urgPrefix, const char *format, std::va_list ap);

	/*!
	 * @brief Log multi-line message.
	 *
	 * @param[in]     source Source identifier.
	 * @param[in]     level Message urgency level.
	 * @param[in]     urgPrefix Message urgency prefix.
	 * @param[in]     format Content of the log message in printf(3) format.
	 * @param[in,out] ap Variable argument list.
	 * @return -1 if error, 0 else.
	 */
	void logPrefixVlogMl(enum LogSource source, enum LogLevel level,
	    const char *urgPrefix, const char *format, std::va_list ap);
};
