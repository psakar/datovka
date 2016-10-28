/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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


#include <cstdarg>
#include <cstddef> /* NULL */
#include <cstdio>
#include <QCoreApplication>
#include <QDateTime>
#include <QHostInfo>
#include <QMutex>
#include <QProcess>
#include <QString>
#include <QtGlobal>

#include "log.h"


#define LOG_TIME_FMT "MMM dd hh:mm:ss"


GlobLog globLog;


/* ========================================================================= */
/*
 * Message output function.
 */
void globalLogOutput(QtMsgType type, const QMessageLogContext &context,
    const QString &msg)
/* ========================================================================= */
{
	QByteArray localMsg = msg.toLocal8Bit();
	QString dateTime = QDateTime::currentDateTime().toString(LOG_TIME_FMT);
	uint8_t level = GlobLog::levelFromType(type);

	switch (type) {
	case QtDebugMsg:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QtInfoMsg:
#endif /* >= Qt-5.5 */
	case QtWarningMsg:
	case QtCriticalMsg:
		if (globLog.logVerbosity() > 0) {
			globLog.log(LOGSRC_DEF, level, "%s (%s:%u, %s)\n",
			    localMsg.constData(), context.file, context.line,
			    context.function);
		} else {
			globLog.log(LOGSRC_DEF, level, "%s\n",
			    localMsg.constData());
		}
		break;
	case QtFatalMsg:
		if (globLog.logVerbosity() > 0) {
			globLog.log(LOGSRC_DEF, level, "%s (%s:%u, %s)\n",
			    localMsg.constData(), context.file, context.line,
			    context.function);
		} else {
			globLog.log(LOGSRC_DEF, level, "%s\n",
			    localMsg.constData());
		}
		abort();
		break;
	}
}


/* ========================================================================= */
/*
 * Debugging using Qt-defined output.
 */
void qDebugCall(const char *fmt, ...)
/* ========================================================================= */
{
	va_list argp;

	va_start(argp, fmt);

	QString outStr;
	outStr.vsprintf(fmt, argp);

	qDebug("%s", outStr.toUtf8().constData());

	va_end(argp);
}


/* ========================================================================= */
/*
 * Debugging using Qt-defined output.
 */
void qDebugCallV(const char *fmt, va_list ap)
/* ========================================================================= */
{
	QString outStr;
	outStr.vsprintf(fmt, ap);

	qDebug("%s", outStr.toUtf8().constData());
}


/*!
 * @brief A convenience macro for getting levels for a given facility
 * and source.
 */
#define facilityLevels(facility, source) \
	facDescVect[(facility)].levels[(source)]


/* ========================================================================= */
/*
 * Constructor.
 */
GlobLog::GlobLog(void)
/* ========================================================================= */
    : m_mutex(),
    m_hostName(QHostInfo::localHostName()),
    m_logVerbosity(0),
    m_debugVerbosity(0)
{
	usedSources = 1;
	openedFiles = 0;
	for (int i = 0; i < MAX_LOG_FILES; ++i) {
		for (int j = 0; j < MAX_SOURCES; ++j) {
			facDescVect[i].levels[j] = 0;
		}
		facDescVect[i].fout = NULL;
	}

	/* TODO -- Initialise syslog. */
}


/* ========================================================================= */
/*
 * Destructor.
 */
GlobLog::~GlobLog(void)
/* ========================================================================= */
{
	/* TODO -- De-initialise syslog. */

	for (int i = 0; i < openedFiles; ++i) {
		Q_ASSERT(NULL != facDescVect[i + LF_FILE].fout);
		fflush(facDescVect[i + LF_FILE].fout);
		fclose(facDescVect[i + LF_FILE].fout);
	}
}


/* ========================================================================= */
/*
 * Get log verbosity.
 */
int GlobLog::logVerbosity(void)
/* ========================================================================= */
{
	int ret;

	m_mutex.lock();

	ret = m_logVerbosity;

	m_mutex.unlock();

	return ret;
}


/* ========================================================================= */
/*
 * Set log verbosity.
 */
void GlobLog::setLogVerbosity(int verb)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Get debug verbosity.
 */
int GlobLog::debugVerbosity(void)
/* ========================================================================= */
{
	int ret;

	m_mutex.lock();

	ret = m_debugVerbosity;

	m_mutex.unlock();

	return ret;
}


/* ========================================================================= */
/*
 * Set debug verbosity.
 */
void GlobLog::setDebugVerbosity(int verb)
/* ========================================================================= */
{
	m_mutex.lock();

	m_debugVerbosity = verb;

	m_mutex.unlock();
}


/* ========================================================================= */
/*
 * Opens a log file as a logging facility.
 */
int GlobLog::openFile(const QString &fName, LogMode mode)
/* ========================================================================= */
{
	FILE *of;
	const char *openMode;
	int fidx = -1;

	m_mutex.lock();

	if ((openedFiles + LF_FILE) >= MAX_LOG_FILES) {
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

	++openedFiles;

	fidx = LF_FILE + openedFiles - 1;

	for (int i = 0; i < MAX_SOURCES; ++i) {
		facDescVect[fidx].levels[i] = 0;
	}
	facDescVect[fidx].fout = of;

	m_mutex.unlock();

	return fidx;
fail:
	m_mutex.unlock();
	return -1;
}


/* ========================================================================= */
/*
 * Returns the log levels for the given facility and source.
 */
uint8_t GlobLog::logLevels(int facility, int source)
/* ========================================================================= */
{
	uint8_t ret;

	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));

	m_mutex.lock();

	ret = facilityLevels(facility, source);

	m_mutex.unlock();

	return ret;
}


/* ========================================================================= */
/*
 * Sets the log levels for the selected facility and source.
 */
void GlobLog::setLogLevels(int facility, int source, uint8_t levels)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Add log levels to the selected facility and source.
 */
void GlobLog::addLogLevels(int facility, int source, uint8_t levels)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Returns the id of a new unique source that can be used.
 */
int GlobLog::acquireUniqueLogSource(void)
/* ========================================================================= */
{
	int ret;

	m_mutex.lock();

	if (usedSources < MAX_SOURCES) {
		ret = usedSources;
		++usedSources;
	} else {
		ret = -1;
	}

	m_mutex.unlock();

	return ret;
}


/* ========================================================================= */
/*
 * Log message.
 */
int GlobLog::log(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Log message.
 */
int GlobLog::logVlog(int source, uint8_t level, const char *fmt, va_list ap)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Log multi-line message.
 */
int GlobLog::logMl(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Log multi-line message.
 */
int GlobLog::logVlogMl(int source, uint8_t level, const char *fmt, va_list ap)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Returns a string containing log urgency prefix.
 */
const char * GlobLog::urgencyPrefix(uint8_t level)
/* ========================================================================= */
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
	default :          prefix = NULL;
	}

	return prefix;
}


/* ========================================================================= */
/*
 * converts message type to urgency level.
 */
uint8_t GlobLog::levelFromType(QtMsgType type)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Log message.
 */
void GlobLog::logPrefixVlog(int source, uint8_t level,
    const char *prefix, const char *format, va_list ap)
/* ========================================================================= */
{
	uint8_t logMask;
	int i;
	FILE *of;
	va_list aq;
	QString dateTime = QDateTime::currentDateTime().toString(LOG_TIME_FMT);
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
		msgPrefix += " " + globLog.m_hostName + " " +
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
	for (i = LF_STDOUT; i < (LF_FILE + openedFiles); ++i) {
		if (facilityLevels(i, source) & logMask) {
			switch (i) {
			case LF_STDOUT:
				of = stdout;
				break;
			case LF_STDERR:
				of = stderr;
				break;
			default:
				of = facDescVect[i].fout;
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


/* ========================================================================= */
/*
 * Log multi-line message.
 */
void GlobLog::logPrefixVlogMl(int source, uint8_t level,
    const char *prefix, const char *format, va_list ap)
/* ========================================================================= */
{
	uint8_t logMask;
	int i;
	FILE *of;
	va_list aq;
	QString dateTime = QDateTime::currentDateTime().toString(LOG_TIME_FMT);
	QString msgPrefix = "";
	QString msgFormatted = "";
	QStringList msgLines;
	QString msg;

	if (m_logVerbosity > 1) {
		msgPrefix = dateTime;
	}
	if (m_logVerbosity > 2) {
		msgPrefix += " " + globLog.m_hostName + " " +
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
		for (i = LF_STDOUT; i < (LF_FILE + openedFiles); ++i) {
			if (facilityLevels(i, source) & logMask) {
				switch (i) {
				case LF_STDOUT:
					of = stdout;
					break;
				case LF_STDERR:
					of = stderr;
					break;
				default:
					of = facDescVect[i].fout;
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
