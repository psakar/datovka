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

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "src/io/filesystem.h"

QString fileNameFromFormat(QString format, qint64 dmId, const QString &dbId,
    const QString &userName, const QString &attachName,
    const QDateTime &dmDeliveryTime, QDateTime dmAcceptanceTime,
    QString dmAnnotation, QString dmSender)
{
	/* Unused. */
	(void) dmDeliveryTime;

	if (format.isEmpty()) {
		format = DEFAULT_TMP_FORMAT;
	}

	if (!dmAcceptanceTime.isValid()) {
		dmAcceptanceTime = QDateTime::currentDateTime();
	}

	QPair<QString, QString> pair;
	QList< QPair<QString, QString> > knowAtrrList;

	pair.first = "%Y";
	pair.second = dmAcceptanceTime.date().toString("yyyy");
	knowAtrrList.append(pair);
	pair.first = "%M";
	pair.second = dmAcceptanceTime.date().toString("MM");
	knowAtrrList.append(pair);
	pair.first = "%D";
	pair.second = dmAcceptanceTime.date().toString("dd");
	knowAtrrList.append(pair);
	pair.first = "%h";
	pair.second = dmAcceptanceTime.time().toString("hh");
	knowAtrrList.append(pair);
	pair.first = "%m";
	pair.second = dmAcceptanceTime.time().toString("mm");
	knowAtrrList.append(pair);
	pair.first = "%i";
	pair.second = QString::number(dmId);
	knowAtrrList.append(pair);
	pair.first = "%s";
	pair.second = dmAnnotation.replace(" ", "-").replace("\t", "-");
	knowAtrrList.append(pair);
	pair.first = "%S";
	pair.second = dmSender.replace(" ", "-").replace("\t", "-");
	knowAtrrList.append(pair);
	pair.first = "%d";
	pair.second = dbId;
	knowAtrrList.append(pair);
	pair.first = "%u";
	pair.second = userName;
	knowAtrrList.append(pair);
	pair.first = "%f";
	pair.second = attachName;
	knowAtrrList.append(pair);

	for (int i = 0; i < knowAtrrList.length(); ++i) {
		format.replace(knowAtrrList[i].first, knowAtrrList[i].second);
	}

	/*
	 * Eliminate illegal characters "\/:*?"<>|" in the file name.
	 * All these characters are replaced by "_".
	 */
	format.replace(QRegExp("[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]"),
	    "_");

	return format;
}

QString nonconflictingFileName(QString filePath)
{
	if (filePath.isEmpty()) {
		return QString();
	}

	if (QFile::exists(filePath)) {

		int fileCnt = 0;
		QFileInfo fi(filePath);

		const QString baseName(fi.baseName());
		const QString path(fi.path());
		const QString suffix(fi.completeSuffix());

		do {
			++fileCnt;
			QString newName(
			    baseName + "_" + QString::number(fileCnt));
			filePath =
			    path + QDir::separator() + newName + "." + suffix;
		} while (QFile::exists(filePath));
	}

	return filePath;
}
