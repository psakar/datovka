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
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QPair>
#include <QRegExp>

#include "message_db_set.h"
#include "src/log/log.h"

MessageDbSet::MessageDbSet(const QString &primaryKey, const QString &locDir,
    bool testing, bool create, Organisation organisation)
    : QMap<QString, MessageDb *>(),
    m_primaryKey(primaryKey),
    m_testing(testing),
    m_locDir(locDir),
    m_organisation(organisation)
{

}

MessageDbSet::~MessageDbSet(void)
{
	QMap<QString, MessageDb *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		delete i.value();
	}
}

bool MessageDbSet::copyToLocation(const QString &newLocDir)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	bool sucessfullyCopied = true;
	QList< QPair<QString, MessageDb *> > oldLocations;
	QList<QString> newLocations;

	QMap<QString, MessageDb *>::iterator i = this->begin();
	while (i != this->end()) {
		MessageDb *db = i.value();
		QString oldFileName = db->fileName();
		QFileInfo fileInfo(oldFileName);
		QString newFileName =
		    newLocDir + QDir::separator() + fileInfo.fileName();

		sucessfullyCopied = db->copyDb(newFileName);
		if (sucessfullyCopied) {
			/* Store origins of successful copies. */
			oldLocations.append(QPair<QString, MessageDb *>(oldFileName, db));
			/* Store new copies. */
			newLocations.append(newFileName);
		} else {
			break;
		}
	}

	if (!sucessfullyCopied) {
		/* Restore origins. */
		QList< QPair<QString, MessageDb *> >::iterator oi;
		for (oi = oldLocations.begin(); oi != oldLocations.end(); ++oi) {
			oi->second->openDb(oi->first);
		}
		/* Delete new copies. */
		QList<QString>::iterator ni;
		for (ni = newLocations.begin(); ni != newLocations.end(); ++ni) {
			QFile::remove(*ni);
		}
		return false;
	} else {
		m_locDir = newLocDir;
	}
	return true;
}

bool MessageDbSet::moveToLocation(const QString &newLocDir)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	bool sucessfullyCopied = true;
	QList< QPair<QString, MessageDb *> > oldLocations;
	QList<QString> newLocations;

	QMap<QString, MessageDb *>::iterator i = this->begin();
	while (i != this->end()) {
		MessageDb *db = i.value();
		QString oldFileName = db->fileName();
		QFileInfo fileInfo(oldFileName);
		QString newFileName =
		    newLocDir + QDir::separator() + fileInfo.fileName();

		sucessfullyCopied = db->copyDb(newFileName);
		if (sucessfullyCopied) {
			/* Store origins of successful copies. */
			oldLocations.append(QPair<QString, MessageDb *>(oldFileName, db));
			/* Store new copies. */
			newLocations.append(newFileName);
		} else {
			break;
		}
	}

	if (!sucessfullyCopied) {
		/* Restore origins. */
		QList< QPair<QString, MessageDb *> >::iterator oi;
		for (oi = oldLocations.begin(); oi != oldLocations.end(); ++oi) {
			oi->second->openDb(oi->first);
		}
		/* Delete new copies. */
		QList<QString>::iterator ni;
		for (ni = newLocations.begin(); ni != newLocations.end(); ++ni) {
			QFile::remove(*ni);
		}
		return false;
	} else {
		/* Delete origins. */
		QList< QPair<QString, MessageDb *> >::iterator oi;
		for (oi = oldLocations.begin(); oi != oldLocations.end(); ++oi) {
			QFile::remove(oi->first);
		}
		m_locDir = newLocDir;
	}
	return true;

	return true;
}

bool MessageDbSet::reopenLocation(const QString &newLocDir,
    Organisation organisation)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	QMap<QString, MessageDb *>::iterator i = this->begin();
	while (i != this->end()) {
		MessageDb *db = i.value();

		/* Close database. */
		delete db;
	}

	/* Remove all elements from this map. */
	this->clear();

	/* Remove all possible database files from new location. */
	QStringList impedingFiles = existingDbFilesInLocation(newLocDir,
	    m_primaryKey, m_testing, DO_UNKNOWN);
	foreach (const QString &fileName, impedingFiles) {
		QFile::remove(newLocDir + QDir::separator() + fileName);
	}

	m_locDir = newLocDir;
	m_organisation = organisation;

	return true;
}

bool MessageDbSet::deleteLocation(void)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	bool sucessfullyDeleted = true;
	QMap<QString, MessageDb *>::iterator i = this->begin();
	while (i != this->end()) {
		MessageDb *db = i.value();

		/* Get file name. */
		QString fileName = db->fileName();

		/* Close database. */
		delete db;

		/* Delete file. */
		logInfo("Deleting database file '%s'.\n",
		    fileName.toUtf8().constData());

		if (!QFile::remove(fileName)) {
			logErrorNL("Failed deleting database file '%s'.",
			    fileName.toUtf8().constData());
			sucessfullyDeleted = false;
		}
	}

	/* Remove all elements from this map. */
	this->clear();

	m_organisation = DO_UNKNOWN;

	return sucessfullyDeleted;
}

#define DB_SUFFIX ".db"

/*!
 * @brief Returns true if file name matches single file naming conventions.
 *
 * @param[in] fileName   File name.
 * @param[in] primaryKey Usually user name.
 * @param[in] testing    True if testing account.
 * @return True if file name matches the naming convention.
 */
static
bool fileNameMatchesSingleFile(const QString &fileName,
    const QString &primaryKey, bool testing)
{
	QString constructed =
	    primaryKey + "___" + (testing ? "1" : "0") + DB_SUFFIX;

	return fileName == constructed;
}

/*!
 * @brief Returns true if file name matches yearly naming conventions.
 *
 * @param[in] fileName   File name.
 * @param[in] primaryKey Usually user name.
 * @param[in] testing    True if testing account.
 * @return True if file name matches the naming convention.
 */
static
bool fileNameMatchesYearly(const QString &fileName, const QString &primaryKey,
    bool testing)
{
	QRegExp re("^" + primaryKey + "_[0-9][0-9][0-9][0-9]"
	    "___" + (testing ? "1" : "0") + DB_SUFFIX + "$");

	return re.indexIn(fileName) > -1;
}

MessageDbSet::Organisation MessageDbSet::dbOrganisation(const QString &locDir,
    const QString &primaryKey, bool testing)
{
	Organisation org = DO_UNKNOWN;

	QString singleFile;
	QStringList yearlyFiles;

	QDirIterator dirIt(locDir, QDirIterator::NoIteratorFlags);

	while (dirIt.hasNext()) {
		dirIt.next();
		if (!QFileInfo(dirIt.filePath()).isFile()) {
			continue;
		}

		QString fileName = dirIt.fileName();

		if (fileNameMatchesSingleFile(fileName, primaryKey, testing)) {
			singleFile = fileName;
		}

		if (fileNameMatchesYearly(fileName, primaryKey, testing)) {
			yearlyFiles.append(fileName);
		}
	}

/* TODO -- Currently single file is preferred. */
/*
	if (!singleFile.isEmpty() && yearlyFiles.isEmpty()) {
		org = DO_SINGLE_FILE;
	} else if (singleFile.isEmpty() && !yearlyFiles.isEmpty()) {
		org = DO_YEARLY;
	}
*/

	if (!singleFile.isEmpty()) {
		org = DO_SINGLE_FILE;
	}

	return org;
}

QStringList MessageDbSet::existingDbFilesInLocation(const QString &locDir,
    const QString &primaryKey, bool testing, Organisation organisation)
{
	QStringList matchingFiles;

	bool matches;
	QDirIterator dirIt(locDir, QDirIterator::NoIteratorFlags);

	while (dirIt.hasNext()) {
		dirIt.next();
		if (!QFileInfo(dirIt.filePath()).isFile()) {
			continue;
		}

		QString fileName = dirIt.fileName();

		matches = false;
		switch (organisation) {
		case DO_UNKNOWN:
			matches = matches || fileNameMatchesSingleFile(
			    fileName, primaryKey, testing);
			matches = matches || fileNameMatchesYearly(fileName,
			    primaryKey, testing);
			break;
		case DO_SINGLE_FILE:
			matches = fileNameMatchesSingleFile(fileName,
			    primaryKey, testing);
			break;
		case DO_YEARLY:
			matches = fileNameMatchesYearly(fileName, primaryKey,
			    testing);
			break;
		default:
			break;
		}

		if (matches) {
			matchingFiles.append(fileName);
		}
	}

	return matchingFiles;
}
