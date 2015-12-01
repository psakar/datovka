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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QRegExp>
#include <QTemporaryFile>
#include <QTextStream>

#include "src/io/filesystem.h"
#include "src/log/log.h"

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
	format.replace(QRegExp("[" + QRegExp::escape("\\/:*?\"<>|") + "]"),
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

enum WriteFileState writeFile(const QString &filePath, const QByteArray &data,
    bool deleteOnError)
{
	if (filePath.isEmpty()) {
		Q_ASSERT(0);
		return WF_ERROR;
	}

	QFile fout(filePath);
	if (!fout.open(QIODevice::WriteOnly)) {
		return WF_CANNOT_CREATE;
	}

	int written = fout.write(data);
	bool flushed = fout.flush();
	fout.close();

	if ((written != data.size()) || !flushed) {
		if (deleteOnError) {
			QFile::remove(filePath);
		}
		return WF_CANNOT_WRITE_WHOLE;
	}

	return WF_SUCCESS;
}

QString writeTemporaryFile(const QString &fileName, const QByteArray &data,
    bool deleteOnError)
{
	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return QString();
	}

	QString nameCopy(fileName);
	nameCopy.replace(QRegExp("[" + QRegExp::escape("\\/:*?\"<>|") + "]"),
	    QString( "_" ));

	QTemporaryFile fout(QDir::tempPath() + QDir::separator() + nameCopy);
	if (!fout.open()) {
		return QString();
	}
	fout.setAutoRemove(false);

	/* Get whole path. */
	QString fullName = fout.fileName();

	int written = fout.write(data);
	bool flushed = fout.flush();
	fout.close();

	if ((written != data.size()) || !flushed) {
		if (deleteOnError) {
			QFile::remove(fullName);
		}
		return QString();
	}

	return fullName;
}

QString confDirPath(const QString &confSubdir)
{
#define WIN_PREFIX "AppData/Roaming"

#ifdef PORTABLE_APPLICATION
	QString dirPath;

	/* Search in application location. */
	dirPath = QCoreApplication::applicationDirPath();
#  ifdef Q_OS_OSX
	{
		QDir directory(dirPath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		dirPath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#  endif /* Q_OS_OSX */

	dirPath += QDir::separator() + confSubdir;
	return dirPath;

#else /* !PORTABLE_APPLICATION */
	QDir homeDir(QDir::homePath());

	if (homeDir.exists(WIN_PREFIX) && !homeDir.exists(confSubdir)) {
		/* Set windows directory. */
		homeDir.cd(WIN_PREFIX);
	}

	return homeDir.path() + QDir::separator() + confSubdir;
#endif /* PORTABLE_APPLICATION */

#undef WIN_PREFIX
}

enum WriteFileState confFileFixBackSlashes(const QString &filePath)
{
	QString fileContent;
	QFile file(filePath);

	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		fileContent = in.readAll();
		file.close();
	} else {
		logErrorNL("Cannot open file '%s' for reading.",
		    filePath.toUtf8().constData());
		return WF_CANNOT_READ;
	}

	fileContent.replace(QString("\\"), QString("/"));

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream in(&file);
		file.reset();
		in << fileContent;
		file.close();
	} else {
		logErrorNL("Cannot write file '%s'.",
		    filePath.toUtf8().constData());
		return WF_CANNOT_WRITE_WHOLE;
	}

	return WF_SUCCESS;
}

enum WriteFileState confFileRemovePwdQuotes(const QString &filePath)
{
	QString fileContent;
	QFile file(filePath);

	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&file);
		fileContent = in.readAll();
		file.close();
	} else {
		logErrorNL("Cannot open file '%s' for reading.",
		    filePath.toUtf8().constData());
		return WF_CANNOT_READ;
	}

	fileContent.replace(QString("\""), QString(""));

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream in(&file);
		file.reset();
		in << fileContent;
		file.close();
	} else {
		logErrorNL("Cannot write file '%s'.",
		    filePath.toUtf8().constData());
		return WF_CANNOT_WRITE_WHOLE;
	}

	return WF_SUCCESS;
}

#define LOCALE_SRC_PATH "locale"

QString appLocalisationDir(void)
{
	QString dirPath;

#ifdef LOCALE_INST_DIR
	/*
	 * Search in installation location if supplied.
	 */

	dirPath = QString(LOCALE_INST_DIR);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}
#endif /* LOCALE_INST_DIR */

	/* Search in application location. */
	dirPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	{
		QDir directory(dirPath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		dirPath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#endif /* Q_OS_OSX */
	dirPath += QDir::separator() + QString(LOCALE_SRC_PATH);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}

	return dirPath;
}

QString qtLocalisationDir(void)
{
	QString dirPath;

#ifdef LOCALE_INST_DIR
	/*
	 * This variable is set in UNIX-like systems. Use default Qt location
	 * directory.
	 */

	dirPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}
#endif /* LOCALE_INST_DIR */

	/* Search in application location. */
	dirPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	{
		QDir directory(dirPath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		dirPath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#endif /* Q_OS_OSX */
	dirPath += QDir::separator() + QString(LOCALE_SRC_PATH);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}

	return dirPath;
}

/*!
 * @brief Searches for the location of the supplied text file.
 *
 * @param[in] fName File name.
 * @return Full file path to sought file.
 */
static
QString suppliedTextFileLocation(const QString &fName)
{
	QString filePath;

#ifdef TEXT_FILES_INST_DIR
	/*
	 * Search in installation location if supplied.
	 */

	filePath = QString(TEXT_FILES_INST_DIR) + QDir::separator() + fName;

	if (QFile::exists(filePath)) {
		return filePath;
	} else {
		filePath.clear();
	}
#endif /* TEXT_FILES_INST_DIR */

	/* Search in application location. */
	filePath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	{
		QDir directory(filePath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		filePath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#endif /* Q_OS_OSX */
	filePath += QDir::separator() + fName;

	if (QFile::exists(filePath)) {
		return filePath;
	} else {
		filePath.clear();
	}

	return filePath;
}

QString suppliedTextFileContent(enum TextFile textFile)
{
#define CREDITS_FILE "AUTHORS"
#define LICENCE_FILE "COPYING"

	const char *fileName = NULL;
	QString content;

	switch (textFile) {
	case TEXT_FILE_CREDITS:
		fileName = CREDITS_FILE;
		break;
	case TEXT_FILE_LICENCE:
		fileName = LICENCE_FILE;
		break;
	default:
		fileName = NULL;
	}

	if (NULL == fileName) {
		Q_ASSERT(0);
		return QString();
	}

	QString fileLocation = suppliedTextFileLocation(fileName);
	if (fileLocation.isEmpty()) {
		return QString();
	}

	QFile file(fileLocation);
	QTextStream textStream(&file);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		content = textStream.readAll();
	}

	file.close();

	return content;
#undef CREDITS_FILE
#undef LICENCE_FILE
}

#undef LOCALE_SRC_PATH
