/*
 * Copyright (C) 2014-2017 CZ.NIC
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

/*!
 * @brief Replace some problematic characters when constructing a file name.
 *
 * @param[in,out] str String to be modified.
 */
static inline
void replaceNameChars(QString &str)
{
	/*
	 * Intentionally also replace characters that could be interpreted
	 * as directory separators.
	 */
	str.replace(QChar(' '), QChar('-')).replace(QChar('\t'), QChar('-'))
	    .replace(QChar('/'), QChar('-')).replace(QChar('\\'), QChar('-'));
}

/*!
 * @brief Illegal characters in file names without directory separators.
 */
#define ILL_FNAME_CH_NO_SEP ":*?\"<>|"
/*!
 * @brief Illegal characters in file name with directory separators.
 */
#define ILL_FNAME_CH "\\/" ILL_FNAME_CH_NO_SEP
/*!
 * @brief Replacement character for illegal characters.
 */
#define ILL_FNAME_REP "_"

/*!
 * @brief Replace illegal characters when constructing a file name.
 *
 * @param[in,out] str String to be modified.
 * @param[in]     replaceSeparators Whether to replace directory separators.
 */
static inline
void replaceIllegalChars(QString &str, bool replaceSeparators)
{
	static const QRegExp regExpNoSep(
	    "[" + QRegExp::escape(ILL_FNAME_CH_NO_SEP) + "]");
	static const QRegExp regExp("[" + QRegExp::escape(ILL_FNAME_CH) + "]");

	str.replace(replaceSeparators ? regExp : regExpNoSep, ILL_FNAME_REP);
}

#undef ILL_FNAME_CH_NO_SEP
#undef ILL_FNAME_CH
#undef ILL_FNAME_REP

QString fileSubpathFromFormat(QString format, bool prohibitDirSep, qint64 dmId,
    const QString &dbId, const QString &userName, const QString &attachName,
    const QDateTime &dmDeliveryTime, QDateTime dmAcceptanceTime,
    QString dmAnnotation, QString dmSender)
{
	Q_UNUSED(dmDeliveryTime);

	if (format.isEmpty()) {
		format = DEFAULT_TMP_FORMAT;
	}

	if (!dmAcceptanceTime.isValid()) {
		dmAcceptanceTime = QDateTime::currentDateTime();
	}

	/* Replace problematic characters. */
	replaceNameChars(dmAnnotation);
	replaceNameChars(dmSender);

	/* Construct list of format attributes that can be replaced. */
	typedef QPair<QString, QString> StringPairType;
	QList<StringPairType> knowAtrrList;

	knowAtrrList.append(StringPairType(QStringLiteral("%Y"),
	    dmAcceptanceTime.date().toString("yyyy")));
	knowAtrrList.append(StringPairType(QStringLiteral("%M"),
	    dmAcceptanceTime.date().toString("MM")));
	knowAtrrList.append(StringPairType(QStringLiteral("%D"),
	    dmAcceptanceTime.date().toString("dd")));
	knowAtrrList.append(StringPairType(QStringLiteral("%h"),
	    dmAcceptanceTime.time().toString("hh")));
	knowAtrrList.append(StringPairType(QStringLiteral("%m"),
	    dmAcceptanceTime.time().toString("mm")));
	knowAtrrList.append(StringPairType(QStringLiteral("%i"),
	    QString::number(dmId)));
	knowAtrrList.append(StringPairType(QStringLiteral("%s"), dmAnnotation));
	knowAtrrList.append(StringPairType(QStringLiteral("%S"), dmSender));
	knowAtrrList.append(StringPairType(QStringLiteral("%d"), dbId));
	knowAtrrList.append(StringPairType(QStringLiteral("%u"), userName));
	knowAtrrList.append(StringPairType(QStringLiteral("%f"), attachName));

	for (int i = 0; i < knowAtrrList.length(); ++i) {
		format.replace(knowAtrrList[i].first, knowAtrrList[i].second);
	}

	replaceIllegalChars(format, prohibitDirSep);

	return QDir::toNativeSeparators(format);
}

bool createDirStructureRecursive(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}

	QDir fileDir(QFileInfo(filePath).absoluteDir());
	if (fileDir.exists()) {
		return true;
	}

	return fileDir.mkpath(".");
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
	replaceIllegalChars(nameCopy, true);

	/* StandardLocation::writableLocation(QStandardPaths::TempLocation) ? */
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
	/* StandardLocation::writableLocation(QStandardPaths::HomeLocation) ? */
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
 * @brief Return text file name.
 *
 * @param[in] textFile Text file identifier.
 * @return Pointer to C string or NULL on error.
 */
static inline
const char *textFileName(enum TextFile textFile)
{
#define LICENCE_FILE "COPYING"
	const char *fileName = NULL;

	switch (textFile) {
	case TEXT_FILE_LICENCE:
		fileName = LICENCE_FILE;
		break;
	default:
		break;
	}

	return fileName;
#undef LICENCE_FILE
}

/*!
 * @brief Expands to defined text files installation location.
 */
#define instLocationUnix() \
	QString(TEXT_FILES_INST_DIR)

/*!
 * @brief Adjust text file location for OS X/macOS package.
 *
 * @param[in] filePath Location as defined by executable.
 * @return Adjusted path.
 */
static inline
QString adjustForMacPackage(const QString &filePath)
{
	QDir directory(filePath);
	if (QLatin1String("MacOS") == directory.dirName()) {
		directory.cdUp();
	}
	return directory.absolutePath() + QDir::separator() +
	    QLatin1String("Resources");
}

QString expectedTextFilePath(enum TextFile textFile)
{
	QString filePath;

#if defined TEXT_FILES_INST_DIR
	filePath = instLocationUnix();
#elif defined Q_OS_OSX
	filePath = QCoreApplication::applicationDirPath();
	filePath = adjustForMacPackage(filePath);
#else /* !TEXT_FILES_INST_DIR && !Q_OS_OSX */
	filePath = QCoreApplication::applicationDirPath();
#endif /* TEXT_FILES_INST_DIR || Q_OS_OSX */

	return filePath + QDir::separator() + textFileName(textFile);
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

	filePath = instLocationUnix() + QDir::separator() + fName;

	if (QFile::exists(filePath)) {
		return filePath;
	} else {
		filePath.clear();
	}
#endif /* TEXT_FILES_INST_DIR */

	/* Search in application location. */
	filePath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	filePath = adjustForMacPackage(filePath);
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
	const char *fileName = textFileName(textFile);
	if (Q_UNLIKELY(NULL == fileName)) {
		Q_ASSERT(0);
		return QString();
	}

	QString fileLocation = suppliedTextFileLocation(fileName);
	if (fileLocation.isEmpty()) {
		return QString();
	}

	QString content;

	QFile file(fileLocation);
	QTextStream textStream(&file);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		content = textStream.readAll();
	}

	file.close();

	return content;
}

#undef LOCALE_SRC_PATH
