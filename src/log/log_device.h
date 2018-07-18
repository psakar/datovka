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
	 * @brief Specifies target logging facility type.
	 */
	enum LogFac {
		LF_SYSLOG = 0, /*!< @brief Syslog facility. */
		LF_STDOUT = 1, /*!< @brief Stdout log target. */
		LF_STDERR = 2, /*!< @brief Stderr log target. */
		LF_FILE = 3 /*!< @brief File log target. */
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
		FILE *fout; /*!< Output file. */
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
	 * @param[in] source   Source identifier.
	 * @return Levels for the selected facility and identifier.
	 */
	quint8 logLevels(int facility, int source);

	/*!
	 * @brief Sets the log levels for the selected facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source   Source identifier.
	 * @param[in] levels   Levels to be set.
	 */
	void setLogLevels(int facility, int source, quint8 levels);

	/*!
	 * @brief Add log levels to the selected facility and source.
	 *
	 * @param[in] facility Facility identifier.
	 * @param[in] source   Source identifier.
	 * @param[in] levels   Log levels to be added.
	 */
	void addLogLevels(int facility, int source, quint8 levels);

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
	int log(int source, quint8 level, const char *fmt, ...);

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
	int logVlog(int source, quint8 level, const char *fmt, va_list ap);

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
	int logMl(int source, quint8 level, const char *fmt, ...);

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
	int logVlogMl(int source, quint8 level, const char *fmt, va_list ap);

	friend void globalLogOutput(enum QtMsgType type,
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
	const char *urgencyPrefix(quint8 level);

	/*!
	 * @brief converts message type to urgency level.
	 *
	 * @param[in] type Message type.
	 * @return Urgency level.
	 */
	static
	quint8 levelFromType(enum QtMsgType type);

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
	void logPrefixVlog(int source, quint8 level,
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
	void logPrefixVlogMl(int source, quint8 level,
	    const char *prefix, const char *format, va_list ap);
};
