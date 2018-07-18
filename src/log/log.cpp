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

#include <cstdarg>
#include <cstddef> /* NULL */
#include <cstdio>
#include <QDateTime>

#include "src/global.h"
#include "src/log/log.h"

#define LOG_TIME_FMT "MMM dd hh:mm:ss"

void globalLogOutput(enum QtMsgType type, const QMessageLogContext &context,
    const QString &msg)
{
	QByteArray localMsg = msg.toLocal8Bit();
	QString dateTime = QDateTime::currentDateTime().toString(LOG_TIME_FMT);
	uint8_t level = LogDevice::levelFromType(type);

	switch (type) {
	case QtDebugMsg:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	case QtInfoMsg:
#endif /* >= Qt-5.5 */
	case QtWarningMsg:
	case QtCriticalMsg:
		if (GlobInstcs::logPtr->logVerbosity() > 0) {
			GlobInstcs::logPtr->log(LOGSRC_DFLT, level,
			    "%s (%s:%u, %s)\n", localMsg.constData(),
			    context.file, context.line, context.function);
		} else {
			GlobInstcs::logPtr->log(LOGSRC_DFLT, level, "%s\n",
			    localMsg.constData());
		}
		break;
	case QtFatalMsg:
		if (GlobInstcs::logPtr->logVerbosity() > 0) {
			GlobInstcs::logPtr->log(LOGSRC_DFLT, level,
			    "%s (%s:%u, %s)\n", localMsg.constData(),
			    context.file, context.line, context.function);
		} else {
			GlobInstcs::logPtr->log(LOGSRC_DFLT, level, "%s\n",
			    localMsg.constData());
		}
		abort();
		break;
	}
}

void qDebugCall(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);

	QString outStr;
	outStr.vsprintf(fmt, argp);

	qDebug("%s", outStr.toUtf8().constData());

	va_end(argp);
}

void qDebugCallV(const char *fmt, va_list ap)
{
	QString outStr;
	outStr.vsprintf(fmt, ap);

	qDebug("%s", outStr.toUtf8().constData());
}
