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

#include "src/datovka_shared/log/log_common.h"
#include "src/datovka_shared/log/log_device.h"
#include "src/datovka_shared/log/memory_log.h"

static
const QString logTimeFmt("MMM dd hh:mm:ss"); /*!< Format of time to be used in log output. */

static
const QRegExp newlineRegExp("(\r\n|\r|\n)"); /*!< Newline regular expression. */
static
const QRegExp trailingNewlineRegExp("(\r\n|\r|\n)$"); /*!< Trailing newline regular expression. */

LogDevice::LogDevice(void)
    : m_handler(Q_NULLPTR),
    m_memLog(Q_NULLPTR),
    m_usedSources(1),
    m_openedFiles(0),
    m_mutex(),
    m_hostName(QHostInfo::localHostName()),
    m_logVerbosity(0),
    m_debugVerbosity(0)
{
	for (int i = 0; i < MAX_LOG_FILES; ++i) {
		for (int j = 0; j < MAX_SOURCES; ++j) {
			m_facDescVect[i].levelBits[j] = 0;
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
		std::fflush(m_facDescVect[i + LF_FILE].fout);
		std::fclose(m_facDescVect[i + LF_FILE].fout);
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
	std::FILE *of;
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

	of = std::fopen(fName.toUtf8().constData(), openMode);
	if (of == NULL) {
		goto fail;
	}

	++m_openedFiles;

	fidx = LF_FILE + m_openedFiles - 1;

	for (int i = 0; i < MAX_SOURCES; ++i) {
		m_facDescVect[fidx].levelBits[i] = 0;
	}
	m_facDescVect[fidx].fout = of;

	m_mutex.unlock();

	return fidx;
fail:
	m_mutex.unlock();
	return -1;
}

/*!
 * @brief A convenience macro for getting level bits for a given facility and
 *     source.
 */
#define facilityLevelBits(facility, source) \
	m_facDescVect[(facility)].levelBits[(source)]

quint8 LogDevice::logLevelBits(int facility, enum LogSource source)
{
	quint8 ret;

	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));

	m_mutex.lock();

	ret = facilityLevelBits(facility, source);

	m_mutex.unlock();

	return ret;
}

void LogDevice::setLogLevelBits(int facility, enum LogSource source,
    quint8 levelBits)
{
	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= -1) && (source < MAX_SOURCES));

	m_mutex.lock();

	if (source != LOGSRC_ANY) {
		facilityLevelBits(facility, source) = levelBits;
	} else {
		for (int i = 0; i < MAX_SOURCES; ++i) {
			facilityLevelBits(facility, i) = levelBits;
		}
	}

	m_mutex.unlock();
}

void LogDevice::addLogLevelBits(int facility, enum LogSource source,
    quint8 levelBits)
{
	int i;

	Q_ASSERT((facility >= 0) && (facility < MAX_LOG_FILES));
	Q_ASSERT((source >= -1) && (source < MAX_SOURCES));

	m_mutex.lock();

	if (source != LOGSRC_ANY) {
		facilityLevelBits(facility, source) |= levelBits;
	} else {
		for (i = 0; i < MAX_SOURCES; ++i) {
			facilityLevelBits(facility, i) |= levelBits;
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

int LogDevice::log(const QMessageLogContext &logCtx, enum LogSource source,
    enum LogLevel level, const char *fmt, ...)
{
	std::va_list argp;

	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	va_start(argp, fmt);

	m_mutex.lock();

	int ret = logPrefixVlog(logCtx, source, level, fmt, argp);

	m_mutex.unlock();

	va_end(argp);

	return ret;
}

int LogDevice::logVlog(const QMessageLogContext &logCtx, enum LogSource source,
    enum LogLevel level, const char *fmt, std::va_list ap)
{
	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	m_mutex.lock();

	int ret = logPrefixVlog(logCtx, source, level, fmt, ap);

	m_mutex.unlock();

	return ret;
}

int LogDevice::logMl(const QMessageLogContext &logCtx, enum LogSource source,
    enum LogLevel level, const char *fmt, ...)
{
	std::va_list argp;

	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	va_start(argp, fmt);

	m_mutex.lock();

	int ret = logPrefixVlogMl(logCtx, source, level, fmt, argp);

	m_mutex.unlock();

	va_end(argp);

	return ret;
}

int LogDevice::logVlogMl(const QMessageLogContext &logCtx,
    enum LogSource source, enum LogLevel level, const char *fmt,
    std::va_list ap)
{
	Q_ASSERT((source >= 0) && (source < MAX_SOURCES));
	Q_ASSERT(level < 8);

	m_mutex.lock();

	int ret = logPrefixVlogMl(logCtx, source, level, fmt, ap);

	m_mutex.unlock();

	return ret;
}

void LogDevice::logQtMessage(enum QtMsgType type,
    const QMessageLogContext &context, const QString &msg)
{
	const QByteArray localMsg(msg.toLocal8Bit());
	enum LogLevel level = levelFromType(type);

	switch (type) {
	case QtDebugMsg:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QtInfoMsg:
#endif /* >= Qt-5.5 */
	case QtWarningMsg:
	case QtCriticalMsg:
		log(context, LOGSRC_DFLT, level, "%s\n", localMsg.constData());
		break;
	case QtFatalMsg:
		log(context, LOGSRC_DFLT, level, "%s\n", localMsg.constData());
		abort();
		break;
	}
}

QtMessageHandler LogDevice::installMessageHandler(QtMessageHandler handler)
{
	QtMessageHandler oldHandler;

	m_mutex.lock();

	oldHandler = m_handler;
	m_handler = handler;

	m_mutex.unlock();

	return oldHandler;
}

MemoryLog *LogDevice::installMemoryLog(MemoryLog *memLog)
{
	MemoryLog *oldMemLog;

	m_mutex.lock();

	oldMemLog = m_memLog;
	m_memLog = memLog;

	m_mutex.unlock();

	return oldMemLog;
}

MemoryLog *LogDevice::memoryLog(void)
{
	MemoryLog *memLog;

	m_mutex.lock();

	memLog = m_memLog;

	m_mutex.unlock();

	return memLog;
}

const char *LogDevice::urgencyPrefix(enum LogLevel level)
{
	const char *prefix;

	switch (level) {
	case LOG_EMERG:   prefix = "emergency: "; break;
	case LOG_ALERT:   prefix = "alert: ";     break;
	case LOG_CRIT:    prefix = "critical: ";  break;
	case LOG_ERR:     prefix = "error: ";     break;
	case LOG_WARNING: prefix = "warning: ";   break;
	case LOG_NOTICE:  prefix = "notice: ";    break;
	case LOG_INFO:    prefix = "info: ";      break;
	case LOG_DEBUG:   prefix = "debug: ";     break;
	default:          prefix = NULL;          break;
	}

	return prefix;
}

enum LogLevel LogDevice::levelFromType(enum QtMsgType type)
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
		return LOG_EMERG;
		break;
	}
}

enum QtMsgType LogDevice::typeFromLevel(enum LogLevel level)
{
	switch (level) {
	case LOG_EMERG:
		return QtFatalMsg;
		break;
	case LOG_ALERT:
	case LOG_CRIT:
		return QtCriticalMsg;
		break;
	case LOG_ERR:
	case LOG_WARNING:
		return QtWarningMsg;
		break;
	case LOG_NOTICE:
	case LOG_INFO:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
		return QtInfoMsg;
		break;
#endif /* >= Qt-5.5 */
	case LOG_DEBUG:
		return QtDebugMsg;
		break;
	default:
		Q_ASSERT(0);
		return QtFatalMsg;
		break;
	}
}

QString LogDevice::buildPrefix(const char *urgPrefix) const
{
	QString msgPrefix;

	if (m_logVerbosity > 1) {
		msgPrefix = QDateTime::currentDateTime().toString(logTimeFmt);
	}
	if (m_logVerbosity > 2) {
		msgPrefix += QStringLiteral(" ") + m_hostName +
		    QStringLiteral(" ") + QCoreApplication::applicationName() +
		    QStringLiteral("[") +
		    QString::number(QCoreApplication::applicationPid()) +
		    QStringLiteral("]");
	}
	if (m_logVerbosity > 1) {
		msgPrefix += QStringLiteral(": ");
	}

	if (NULL != urgPrefix) {
		msgPrefix += urgPrefix;
	}

	return msgPrefix;
}

QString LogDevice::buildPostfix(const QMessageLogContext &logCtx)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	return QString::asprintf(" (%s:%d, %s())", logCtx.file, logCtx.line,
	    logCtx.function);
#else /* < Qt-5.5 */
	return QString().sprintf(" (%s:%d, %s())", logCtx.file, logCtx.line,
	    logCtx.function);
#endif /* >= Qt-5.5 */
}

void LogDevice::logString(enum LogSource source, enum LogLevel level,
    const QString &msg)
{
	quint8 logMask = LOG_MASK(level);

	/* Syslog. */
	if (facilityLevelBits(LF_SYSLOG, source) & logMask) {
		/* TODO -- Write to syslog. */
//		syslog(level, "%s", msg.toUtf8().constData());
	}

	/* Log files. */
	std::FILE *of;
	for (int i = LF_STDOUT; i < (LF_FILE + m_openedFiles); ++i) {
		if (facilityLevelBits(i, source) & logMask) {
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
			std::fputs(msg.toUtf8().constData(), of);
			/*
			 * Windows buffers stderr, explicit flush is needed.
			 *
			 * Also flush everything to files so there is nothing
			 * lost.
			 */
			if (stdout != of) {
				std::fflush(of);
			}
		}
	}
}

/*!
 * @brief Remove trailing newline character sequence from the string.
 *
 * @param[in,out] str Modified string.
 */
static inline
bool removeTrailingNewline(QString &str)
{
	int pos = str.lastIndexOf(trailingNewlineRegExp);
	bool resize = (pos >= 0);
	if (resize) {
		str.resize(pos);
	}
	return resize;
}

int LogDevice::logPrefixVlog(const QMessageLogContext &logCtx,
    enum LogSource source, enum LogLevel level, const char *format,
    std::va_list ap)
{
	std::va_list aq;
	QString msgFormatted;
	QString msg;
	/*!
	 * @todo Handle case in which no additional memory can be
	 * allocated.
	 */

	const char *urgPrefix = urgencyPrefix(level);
	if (Q_UNLIKELY(urgPrefix == NULL)) {
		return -1;
	}

	va_copy(aq, ap);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	msgFormatted = QString::vasprintf(format, aq);
#else /* < Qt-5.5 */
	msgFormatted.vsprintf(format, aq);
#endif /* >= Qt-5.5 */
	va_end(aq);
	bool removed = removeTrailingNewline(msgFormatted);

	msg = buildPrefix(urgPrefix) + msgFormatted;
	if (m_logVerbosity > 0) {
		msg += buildPostfix(logCtx);
	}
	if (removed) {
		msg += QStringLiteral("\n");
	}

	/* Message handler. */
	if (m_handler != Q_NULLPTR) {
		m_handler(typeFromLevel(level), logCtx, msgFormatted);
	}

	/* Memory log. */
	if (m_memLog != Q_NULLPTR) {
		m_memLog->log(msg);
	}

	logString(source, level, msg);

	return 0;
}

int LogDevice::logPrefixVlogMl(const QMessageLogContext &logCtx,
    enum LogSource source, enum LogLevel level, const char *format,
    std::va_list ap)
{
	std::va_list aq;
	QString msgFormatted;
	QStringList msgLines;
	QString msg;

	const char *urgPrefix = urgencyPrefix(level);
	if (Q_UNLIKELY(urgPrefix == NULL)) {
		return -1;
	}

	const QString msgPrefix(buildPrefix(urgPrefix));

	va_copy(aq, ap);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	msgFormatted = QString::vasprintf(format, aq);
#else /* < Qt-5.5 */
	msgFormatted.vsprintf(format, aq);
#endif /* >= Qt-5.5 */
	va_end(aq);

	msgLines = msgFormatted.split(newlineRegExp, QString::SkipEmptyParts);

	/* Message handler. */
	if (m_handler != Q_NULLPTR) {
		removeTrailingNewline(msgFormatted);
		m_handler(typeFromLevel(level), logCtx, msgFormatted);
	}

	foreach (const QString &msgLine, msgLines) {
		QString msg(msgPrefix + msgLine + QStringLiteral("\n"));

		/* Memory log. */
		if (m_memLog != Q_NULLPTR) {
			m_memLog->log(msg);
		}

		logString(source, level, msg);
	}

	return 0;
}
