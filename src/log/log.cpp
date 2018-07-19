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

#include "src/log/log.h"

void globalLogOutput(enum QtMsgType type, const QMessageLogContext &context,
    const QString &msg)
{
	if (GlobInstcs::logPtr != Q_NULLPTR) {
		GlobInstcs::logPtr->logQtMessage(type, context, msg);
	}
}

void qDebugCall(const char *fmt, ...)
{
	std::va_list argp;

	va_start(argp, fmt);

	QString outStr;
	outStr.vsprintf(fmt, argp);

	qDebug("%s", outStr.toUtf8().constData());

	va_end(argp);
}

void qDebugCallV(const char *fmt, std::va_list ap)
{
	QString outStr;
	outStr.vsprintf(fmt, ap);

	qDebug("%s", outStr.toUtf8().constData());
}
