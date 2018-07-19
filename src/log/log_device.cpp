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

#include <cstddef> /* NULL */
#include <QCoreApplication>
#include <QDateTime>
#include <QHostInfo>

#include "src/log/log_common.h"
#include "src/log/log_device.h"

const QString LogDevice::logTimeFmt("MMM dd hh:mm:ss");

LogDevice::LogDevice(void)
    : m_usedSources(1),
    m_openedFiles(0),
    m_mutex(),
    m_hostName(QHostInfo::localHostName()),
    m_logVerbosity(0),
    m_debugVerbosity(0)
{
	for (int i = 0; i < MAX_LOG_FILES; ++i) {
		for (int j = 0; j < MAX_SOURCES; ++j) {
			m_facDescVect[i].levels[j] = 0;
		}
		m_facDescVect[i].fout = NULL;
	}

	/* TODO -- Initialise syslog. */
}

LogDevice::~LogDevice(void)
{
	/* TODO -- De-initialise syslog. */

	for (int i = 0; i < m_openedFiles; ++i) {
		Q_ASSERT(NULL != m_facDescVect[i + LF_FILE].fout);
		fflush(m_facDescVect[i + LF_FILE].fout);
		fclose(m_facDescVect[i + LF_FILE].fout);
	}
}

int LogDevice::logVerbosity(void)
{
	int ret;

	m_mutex.lock();

	ret = m_logVerbosity;

	m_mutex.unlock();

	return ret;
}

void LogDevice::setLogVerbosity(int verb)
{
	m_mutex.lock();

	if (verb < 0) {
		verb = 0;
	} else if (verb > 3) {
		verb = 3;
	}

	m_logVerbosity = verb;

	m_mutex.unlock();
}

int LogDevice::debugVerbosity(void)
{
	int ret;

	m_mutex.lock();

	ret = m_debugVerbosity;

	m_mutex.unlock();

	return ret;
}

void LogDevice::setDebugVerbosity(int verb)
{
	m_mutex.lock();

	m_debugVerbosity = verb;

	m_mutex.unlock();
}

int LogDevice::openFile(const QString &fName, enum LogMode mode)
{
	FILE *of;
	const char *openMode;
	int fidx = -1;

	m_mutex.lock();

	if ((m_openedFiles + LF_FILE) >= MAX_LOG_FILES) {
		/* Maximal number of facilities reached. */
		goto fail;
	}

	switch (mode) {
	case LM_WRONLY:
		openMode = "w";
		break;
	case LM_APPEND:
		openMode = "a";
		break;
	default:
		Q_ASSERT(0);
		goto fail;
		break;
	}

	of = fopen(fName.toUtf8().constData(), openMode);
	if (of == NULL) {
		goto fail;
	}

	++m_openedFiles;

	fidx = LF_FILE + m_openedFiles - 1;

	for (int i = 0; i < MAX_SOURCES; ++i) {
		m_facDescVect[fidx].levels[i] = 0;
	}
	m_facDescVect[fidx].fout = of;

	m_mutex.unlock();

	return fidx;
fail:
	m_mutex.unlock();
	return -1;
}

/*!
 * @brief A convenience macro for getting levels for a given facility
 * and source.
 */
#define facilityLevels(facility, source) \
	m_facDescVect[(facility)].levels[(source)]

quint8 LogDevice::logLevels(int facility, enum LogSource source)
{
	quint8 ret;

	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));

	m_mutex.lock();

	ret = facilityLevels(facility, source);

	m_mutex.unlock();

	return ret;
}

void LogDevice::setLogLevels(int facility, enum LogSource source, quint8 levels)
{
	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= -1) && (source < MAX_SOURCES));

	m_mutex.lock();

	if (source != LOGSRC_ANY) {
		facilityLevels(facility, source) = levels;
	} else {
		for (int i = 0; i < MAX_SOURCES; ++i) {
			facilityLevels(facility, i) = levels;
		}
	}

	m_mutex.unlock();
}

void LogDevice::addLogLevels(int facility, enum LogSource source, quint8 levels)
{
	int i;

	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= -1) && (source < MAX_SOURCES));

	m_mutex.lock();

	if (source != LOGSRC_ANY) {
		facilityLevels(facility, source) |= levels;
	} else {
		for (i = 0; i < MAX_SOURCES; ++i) {
			facilityLevels(facility, i) |= levels;
		}
	}

	m_mutex.unlock();
}

int LogDevice::acquireUniqueLogSource(void)
{
	int ret;

	m_mutex.lock();

	if (m_usedSources < MAX_SOURCES) {
		ret = m_usedSources;
		++m_usedSources;
	} else {
		ret = -1;
	}

	m_mutex.unlock();

	return ret;
}

int LogDevice::log(enum LogSource source, quint8 level, const char *fmt, ...)
{
	const char *prefix;
	va_list argp;

	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	prefix = urgencyPrefix(level);
	if (prefix == NULL) {
		return -1;
	}

	va_start(argp, fmt);

	m_mutex.lock();

	logPrefixVlog(source, level, prefix, fmt, argp);

	m_mutex.unlock();

	va_end(argp);

	return 0;
}

int LogDevice::logVlog(enum LogSource source, quint8 level, const char *fmt,
    va_list ap)
{
	const char *prefix;

	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	prefix = urgencyPrefix(level);
	if (prefix == NULL) {
		return -1;
	}

	m_mutex.lock();

	logPrefixVlog(source, level, prefix, fmt, ap);

	m_mutex.unlock();

	return 0;
}

int LogDevice::logMl(enum LogSource source, quint8 level, const char *fmt, ...)
{
	const char *prefix;
	va_list argp;

	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	prefix = urgencyPrefix(level);
	if (prefix == NULL) {
		return -1;
	}

	va_start(argp, fmt);

	m_mutex.lock();

	logPrefixVlogMl(source, level, prefix, fmt, argp);

	m_mutex.unlock();

	va_end(argp);

	return 0;
}

int LogDevice::logVlogMl(enum LogSource source, quint8 level, const char *fmt,
    va_list ap)
{
	const char *prefix;

	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	prefix = urgencyPrefix(level);
	if (prefix == NULL) {
		return -1;
	}

	m_mutex.lock();

	logPrefixVlogMl(source, level, prefix, fmt, ap);

	m_mutex.unlock();

	return 0;
}

void LogDevice::logQtMessage(enum QtMsgType type,
    const QMessageLogContext &context, const QString &msg)
{
	const QByteArray localMsg(msg.toLocal8Bit());
	quint8 level = levelFromType(type);

	switch (type) {
	case QtDebugMsg:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QtInfoMsg:
#endif /* >= Qt-5.5 */
	case QtWarningMsg:
	case QtCriticalMsg:
		if (logVerbosity() > 0) {
			log(LOGSRC_DFLT, level, "%s (%s:%u, %s)\n",
			    localMsg.constData(), context.file, context.line,
			    context.function);
		} else {
			log(LOGSRC_DFLT, level, "%s\n", localMsg.constData());
		}
		break;
	case QtFatalMsg:
		if (logVerbosity() > 0) {
			log(LOGSRC_DFLT, level, "%s (%s:%u, %s)\n",
			    localMsg.constData(), context.file, context.line,
			    context.function);
		} else {
			log(LOGSRC_DFLT, level, "%s\n", localMsg.constData());
		}
		abort();
		break;
	}
}

const char *LogDevice::urgencyPrefix(quint8 level)
{
	const char *prefix;

	switch (level) {
	case LOG_EMERG :   prefix = "emergency: "; break;
	case LOG_ALERT :   prefix = "alert: ";     break;
	case LOG_CRIT :    prefix = "critical: ";  break;
	case LOG_ERR :     prefix = "error: ";     break;
	case LOG_WARNING : prefix = "warning: ";   break;
	case LOG_NOTICE :  prefix = "notice: ";    break;
	case LOG_INFO :    prefix = "info: ";      break;
	case LOG_DEBUG :   prefix = "debug: ";     break;
	default :          prefix = NULL;          break;
	}

	return prefix;
}

quint8 LogDevice::levelFromType(enum QtMsgType type)
{
	switch (type) {
	case QtDebugMsg:
		return LOG_DEBUG;
		break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QtInfoMsg:
		return LOG_INFO;
		break;
#endif /* >= Qt-5.5 */
	case QtWarningMsg:
		return LOG_WARNING;
		break;
	case QtCriticalMsg:
		return LOG_CRIT;
		break;
	case QtFatalMsg:
		return LOG_EMERG; /* System is unusable. */
		break;
	default:
		Q_ASSERT(0);
		return -1;
		break;
	}
}

void LogDevice::logPrefixVlog(enum LogSource source, quint8 level,
    const char *prefix, const char *format, va_list ap)
{
	quint8 logMask;
	int i;
	FILE *of;
	va_list aq;
	QString dateTime = QDateTime::currentDateTime().toString(logTimeFmt);
	QString msgPrefix = "";
	QString msgFormatted = "";
	QString msg;
	/*!
	 * @todo Handle case in which no additional memory can be
	 * allocated.
	 */

	if (m_logVerbosity > 1) {
		msgPrefix = dateTime;
	}
	if (m_logVerbosity > 2) {
		msgPrefix += " " + m_hostName + " " +
		    QCoreApplication::applicationName() + "[" +
		    QString::number(QCoreApplication::applicationPid()) +
		    "]";
	}
	if (m_logVerbosity > 1) {
		msgPrefix += ": ";
	}

	if (NULL != prefix) {
		msgPrefix += prefix;
	}

	va_copy(aq, ap);
	msgFormatted.vsprintf(format, aq);
	va_end(aq);

	msg = msgPrefix + msgFormatted;

	/* Convert lo log mask. */
	logMask = LOG_MASK(level);

	/* Syslog. */
	if (facilityLevels(LF_SYSLOG, source) & logMask) {
		/* TODO -- Write to syslog. */
//		syslog(level, "%s", msg.toUtf8().constData());
	}

	/* Log files. */
	for (i = LF_STDOUT; i < (LF_FILE + m_openedFiles); ++i) {
		if (facilityLevels(i, source) & logMask) {
			switch (i) {
			case LF_STDOUT:
				of = stdout;
				break;
			case LF_STDERR:
				of = stderr;
				break;
			default:
				of = m_facDescVect[i].fout;
			}

			Q_ASSERT(of != NULL);
			fputs(msg.toUtf8().constData(), of);
			/*
			 * Windows buffers stderr, explicit flush is needed.
			 */
			if (stderr == of) {
				fflush(of);
			}
		}
	}
}

void LogDevice::logPrefixVlogMl(enum LogSource source, quint8 level,
    const char *prefix, const char *format, va_list ap)
{
	quint8 logMask;
	int i;
	FILE *of;
	va_list aq;
	QString dateTime = QDateTime::currentDateTime().toString(logTimeFmt);
	QString msgPrefix = "";
	QString msgFormatted = "";
	QStringList msgLines;
	QString msg;

	if (m_logVerbosity > 1) {
		msgPrefix = dateTime;
	}
	if (m_logVerbosity > 2) {
		msgPrefix += " " + m_hostName + " " +
		    QCoreApplication::applicationName() + "[" +
		    QString::number(QCoreApplication::applicationPid()) +
		    "]";
	}
	if (m_logVerbosity > 1) {
		msgPrefix += ": ";
	}

	if (NULL != prefix) {
		msgPrefix += prefix;
	}

	/* Convert lo log mask. */
	logMask = LOG_MASK(level);

	va_copy(aq, ap);
	msgFormatted.vsprintf(format, aq);
	va_end(aq);

	msgLines = msgFormatted.split(QRegExp("(\r\n|\r|\n)"),
	    QString::SkipEmptyParts);

	for (QList<QString>::iterator it = msgLines.begin();
	     it != msgLines.end(); ++it) {
		msg = msgPrefix + *it;

		/* Syslog. */
		if (facilityLevels(LF_SYSLOG, source) & logMask) {
			/* TODO -- Write to syslog. */
//			syslog(level, "%s", msg.toUtf8().constData());
		}

		/* Log files. */
		for (i = LF_STDOUT; i < (LF_FILE + m_openedFiles); ++i) {
			if (facilityLevels(i, source) & logMask) {
				switch (i) {
				case LF_STDOUT:
					of = stdout;
					break;
				case LF_STDERR:
					of = stderr;
					break;
				default:
					of = m_facDescVect[i].fout;
				}

				Q_ASSERT(of != NULL);
				fputs(msg.toUtf8().constData(), of);
				fputc('\n', of);
				/*
				 * Windows buffers stderr, explicit flush is
				 * needed.
				 *
				 * Also flush everything to files so there is
				 * nothing lost.
				 */
				if (stdout != of) {
					fflush(of);
				}
			}
		}
	}
}
