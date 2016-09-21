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
#include <QSqlDatabase>

#include "message_db_set.h"
#include "src/log/log.h"

MessageDbSet::MessageDbSet(const QString &locDir, const QString &primaryKey,
    bool testing, enum Organisation organisation,
    const QString &connectionPrefix)
    : QMap<QString, MessageDb *>(),
    m_connectionPrefix(connectionPrefix),
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

bool MessageDbSet::openLocation(const QString &newLocDir,
   enum Organisation organisation, enum CreationManner manner)
{
	QStringList matchingFiles;

	/* New location must exist. */
	if (!QDir(newLocDir).exists()) {
		logErrorNL("Directory '%s' does not exist.\n",
		    newLocDir.toUtf8().constData());
		return false;
	}

	/* Try to determine the database organisation structure. */
	enum Organisation existingOrganisation = dbOrganisation(newLocDir,
	    m_primaryKey, m_testing);
	matchingFiles = existingDbFileNamesInLocation(newLocDir,
	    m_primaryKey, m_testing, DO_UNKNOWN, true);
	if ((existingOrganisation == DO_UNKNOWN) &&
	    (!matchingFiles.isEmpty())) {
		logErrorNL("Ambiguous organisation of database '%s' in %s'.",
		    m_primaryKey.toUtf8().constData(),
		    newLocDir.toUtf8().constData());
		return false;
	}

	if (organisation == DO_UNKNOWN) {
		if (existingOrganisation != DO_UNKNOWN) {
			organisation = existingOrganisation;
		} else {
			logErrorNL("Unspecified organisation of database '%s' in '%s'.",
			    m_primaryKey.toUtf8().constData(),
			    newLocDir.toUtf8().constData());
			return false;
		}
	} else if (organisation != existingOrganisation) {
		logErrorNL("Differently organised database '%s' in '%s' already exists.",
		    m_primaryKey.toUtf8().constData(),
		    newLocDir.toUtf8().constData());
		return false;
	}

	matchingFiles = existingDbFileNamesInLocation(newLocDir,
	    m_primaryKey, m_testing, organisation, true);

	if ((manner == CM_MUST_EXIST) && matchingFiles.isEmpty()) {
		logErrorNL("No matching database file for '%s' in '%s'.",
		    m_primaryKey.toUtf8().constData(),
		    newLocDir.toUtf8().constData());
		return false;
	}

	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();

		/* Close database. */
		delete db;
	}

	/* Remove all elements from this map. */
	this->clear();

	/* There may be no matching files. */

	/* Check primary keys. */
	foreach (const QString &fileName, matchingFiles) {
		QString secondaryKey = secondaryKeyFromFileName(
		    fileName, organisation);

		if (secondaryKey.isNull()) {
			logErrorNL("Failed obtaining secondary key from file name '%s'.",
			    fileName.toUtf8().constData());
			return false;
		}
	}

	m_locDir = newLocDir;
	m_organisation = organisation;

	MessageDb *db = NULL;
	if (matchingFiles.size()) {
		/* Load files that have been found. */
		foreach (const QString &fileName, matchingFiles) {
			QString secondaryKey = secondaryKeyFromFileName(
			    fileName, organisation);
			Q_ASSERT(!secondaryKey.isNull());

			db = _accessMessageDb(secondaryKey, false);
			if (db == NULL) {
				logErrorNL("Failed opening database file '%s'.",
				    fileName.toUtf8().constData());
				/* TODO -- How can be this operation aborted? */
			}
		}
	} else if (manner == CM_CREATE_EMPTY_CURRENT) {
		/* Create empty file matching current date. */
		QString secKey = secondaryKey(QDateTime::currentDateTime());
		Q_ASSERT(!secKey.isNull());

		db = _accessMessageDb(secKey, true);
		if (db == NULL) {
			QString fileName(constructDbFileName(m_locDir,
			    m_primaryKey, secKey, m_testing, m_organisation));
			logErrorNL("Failed opening database file '%s'.",
			    fileName.toUtf8().constData());
			return false;
		}
	}

	return true;
}

bool MessageDbSet::copyToLocation(const QString &newLocDir)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	if (!QDir(newLocDir).exists()) {
		logErrorNL("Directory '%s' does not exist.\n",
		    newLocDir.toUtf8().constData());
		return false;
	}

	bool sucessfullyCopied = true;
	QList< QPair<QString, MessageDb *> > oldLocations;
	QList<QString> newLocations;

	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i) {
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

	if (!QDir(newLocDir).exists()) {
		logErrorNL("Directory '%s' does not exist.\n",
		    newLocDir.toUtf8().constData());
		return false;
	}

	bool sucessfullyCopied = true;
	QList< QPair<QString, MessageDb *> > oldLocations;
	QList<QString> newLocations;

	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i) {
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
}

bool MessageDbSet::reopenLocation(const QString &newLocDir,
    enum Organisation organisation, enum CreationManner manner)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	if (!QDir(newLocDir).exists()) {
		logErrorNL("Directory '%s' does not exist.\n",
		    newLocDir.toUtf8().constData());
		return false;
	}

	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();

		/* Close database. */
		delete db;
	}

	/* Remove all elements from this map. */
	this->clear();

	/* Remove all possible database files from new location. */
	QStringList impedingFiles = existingDbFileNamesInLocation(newLocDir,
	    m_primaryKey, m_testing, DO_UNKNOWN, false);
	foreach (const QString &fileName, impedingFiles) {
		QFile::remove(newLocDir + QDir::separator() + fileName);
	}

	m_locDir = newLocDir;
	m_organisation = organisation;

	MessageDb *db = NULL;
	if (manner == CM_CREATE_EMPTY_CURRENT) {
		/* Create empty file matching current date. */
		QString secKey = secondaryKey(QDateTime::currentDateTime());
		Q_ASSERT(!secKey.isNull());

		db = _accessMessageDb(secKey, true);
		if (db == NULL) {
			QString fileName(constructDbFileName(m_locDir,
			    m_primaryKey, secKey, m_testing, m_organisation));
			logErrorNL("Failed opening database file '%s'.",
			    fileName.toUtf8().constData());
			return false;
		}
	}

	return true;
}

bool MessageDbSet::deleteLocation(void)
{
	if (m_organisation == DO_UNKNOWN) {
		return false;
	}

	bool sucessfullyDeleted = true;
	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i) {
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

/*!
 * @brief Creates secondary key from given time.
 *
 * @param[in] time         Time.
 * @return Secondary key or null string on error.
 */
static
QString secondaryKeySingleFile(const QDateTime &time)
{
	Q_UNUSED(time);

	return QString(SINGLE_FILE_SEC_KEY);
}

QString MessageDbSet::secondaryKey(const QDateTime &time) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return secondaryKeySingleFile(time);
		break;
	case DO_YEARLY:
		return yearFromDateTime(time);
		break;
	case DO_UNKNOWN:
	default:
		Q_ASSERT(0);
		return QString();
	}
}

MessageDb *MessageDbSet::constAccessMessageDb(
    const QDateTime &deliveryTime) const
{
	QString secondary = secondaryKey(deliveryTime);

	if (secondary.isNull()) {
		return 0;
	}

	/* Already opened. */
	if (this->constFind(secondary) != this->end()) {
		return (*this)[secondary];
	}

	return 0;
}

MessageDb *MessageDbSet::accessMessageDb(const QDateTime &deliveryTime,
    bool writeNew)
{
	QString secondary = secondaryKey(deliveryTime);

	if (secondary.isNull()) {
		return 0;
	}

	return _accessMessageDb(secondary, writeNew);
}

MessageDbSet::Organisation MessageDbSet::organisation(void) const
{
	return m_organisation;
}

qint64 MessageDbSet::underlyingFileSize(
    enum MessageDbSet::SizeComputation sc) const
{
	qint64 dbSize = 0;

	QMap<QString, MessageDb *>::const_iterator i;
	for (i = this->begin(); i != this->end(); ++i) {
		const QString fileName(i.value()->fileName());
		if (fileName == MessageDb::memoryLocation) {
			continue;
		}
		const QFileInfo fileInfo(fileName);
		if (fileInfo.exists()) {
			if (sc == SC_SUM) {
				dbSize += fileInfo.size();
			} else if (sc == SC_LARGEST) {
				if (dbSize < fileInfo.size()) {
					dbSize = fileInfo.size();
				}
			} else {
				Q_ASSERT(0);
			}
		}
	}

	return dbSize;
}

QStringList MessageDbSet::fileNames(void) const
{
	QStringList fileList;

	bool inFiles = false;
	bool inMemory = false;

	QMap<QString, MessageDb *>::const_iterator i;
	for (i = this->begin(); i != this->end(); ++i) {
		QString fileName = i.value()->fileName();
		if (fileName == MessageDb::memoryLocation) {
			inMemory = true;
		} else {
			inFiles = true;
		}
		fileList.append(fileName);
	}

	if (inMemory && inFiles) {
		Q_ASSERT(0);
	}

	if (inFiles) {
		fileList.sort();
	} else if (inMemory) {
		fileList = QStringList(MessageDb::memoryLocation);
	}

	return fileList;
}

QString MessageDbSet::yearFromDateTime(const QDateTime &time)
{
	if (time.isValid()) {
		return time.toString("yyyy");
	} else {
		return YEARLY_SEC_KEY_INVALID;
	}
}

MessageDbSet *MessageDbSet::createNew(const QString &locDir,
    const QString &primaryKey, bool testing, enum Organisation organisation,
    const QString &connectionPrefix, enum CreationManner manner)
{
	MessageDbSet *dbSet = NULL;
	QStringList matchingFiles;

	if (manner == CM_MUST_EXIST) {
		if (organisation == DO_UNKNOWN) {
			/*
			 * Try to determine the database organisation
			 * structure.
			 */
			organisation = dbOrganisation(locDir, primaryKey,
			    testing);
		}

		matchingFiles = existingDbFileNamesInLocation(locDir,
		    primaryKey, testing, organisation, true);

		if (matchingFiles.isEmpty()) {
			return NULL;
		}

		/* Check primary keys. */
		foreach (const QString &fileName, matchingFiles) {
			QString secondaryKey = secondaryKeyFromFileName(
			    fileName, organisation);

			if (secondaryKey.isNull()) {
				logErrorNL("Failed obtaining secondary key from file name '%s'.",
				    fileName.toUtf8().constData());
				return NULL;
			}
		}
	} else {
		/* Create missing directory. */
		QDir dir(locDir);
		if (!dir.exists()) {
			/* Empty file will be created automatically. */
			if (!dir.mkpath(dir.absolutePath())) {
				/* Cannot create directory. */
				return NULL;
			}
		}
	}

	if (organisation == DO_UNKNOWN) {
		logErrorNL("Ambiguous organisation of database '%s' in %s'.",
		    primaryKey.toUtf8().constData(),
		    locDir.toUtf8().constData());
		return NULL;
	}

	/* Create database set. */
	dbSet = new(std::nothrow) MessageDbSet(locDir, primaryKey, testing,
	    organisation, connectionPrefix);
	if (dbSet == NULL) {
		Q_ASSERT(0);
		return NULL;
	}

	MessageDb *db = NULL;
	if (matchingFiles.size()) {
		/* Load files that have been found. */
		foreach (const QString &fileName, matchingFiles) {
			QString secondaryKey = secondaryKeyFromFileName(
			    fileName, organisation);
			Q_ASSERT(!secondaryKey.isNull());

			db = dbSet->_accessMessageDb(secondaryKey, false);
			if (db == NULL) {
				logErrorNL("Failed opening database file '%s'.",
				    fileName.toUtf8().constData());
				delete dbSet;
				return NULL;
			}
		}
	} else if (manner == CM_CREATE_EMPTY_CURRENT) {
		/* Create empty file matching current date. */
		QString secKey = dbSet->secondaryKey(
		    QDateTime::currentDateTime());
		Q_ASSERT(!secKey.isNull());

		db = dbSet->_accessMessageDb(secKey, true);
		if (db == NULL) {
			QString fileName(constructDbFileName(dbSet->m_locDir,
			    dbSet->m_primaryKey, secKey, dbSet->m_testing,
			    dbSet->m_organisation));
			logErrorNL("Failed opening database file '%s'.",
			    fileName.toUtf8().constData());
			delete dbSet;
			return NULL;
		}
	}

	return dbSet;
}

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
	QRegExp re("^" + primaryKey + "_" YEARLY_SEC_KEY_RE
	    "___" + (testing ? "1" : "0") + DB_SUFFIX "$");

	QString invFileName(primaryKey + "_" YEARLY_SEC_KEY_INVALID
	    "___" + (testing ? "1" : "0") + DB_SUFFIX);

	return (re.indexIn(fileName) > -1) || (fileName == invFileName);
}

enum MessageDbSet::Organisation MessageDbSet::dbOrganisation(
    const QString &locDir, const QString &primaryKey, bool testing)
{
	enum Organisation org = DO_UNKNOWN;

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

	if (!singleFile.isEmpty() && yearlyFiles.isEmpty()) {
		org = DO_SINGLE_FILE;
	} else if (singleFile.isEmpty() && !yearlyFiles.isEmpty()) {
		org = DO_YEARLY;
	}

	return org;
}

/*!
 * @brief Secondary key if file name matches single file naming conventions.
 *
 * @param[in] fileName   File name.
 * @return Key if name matches the naming convention, null string on error.
 */
static
QString fileNameSecondaryKeySingleFile(const QString &fileName)
{
	static const QRegExp re(QString("^") + PRIMARY_KEY_RE
	    "___" "[01]" DB_SUFFIX "$");

	return (re.indexIn(fileName) > -1) ? SINGLE_FILE_SEC_KEY : QString();
}

/*!
 * @brief Secondary key if file name matches single file naming conventions.
 *
 * @param[in] fileName   File name.
 * @return Key if name matches the naming convention, null string on error.
 */
static
QString fileNameSecondaryKeyYearly(const QString &fileName)
{
	static const QRegExp reInv(QString("^") + PRIMARY_KEY_RE
	    "_" YEARLY_SEC_KEY_INVALID "___" "[01]" DB_SUFFIX "$");

	if (reInv.indexIn(fileName) > -1) {
		return YEARLY_SEC_KEY_INVALID;
	}

	static const QRegExp reValid(QString("^") + PRIMARY_KEY_RE
	    "_" YEARLY_SEC_KEY_RE "___" "[01]" DB_SUFFIX "$");

	if (reValid.indexIn(fileName) > -1) {
		return fileName.section('_', 1, 1);
	}

	return QString();
}

QString MessageDbSet::secondaryKeyFromFileName(const QString &fileName,
    enum Organisation organisation)
{
	if (fileName.isEmpty() || (organisation == DO_UNKNOWN)) {
		return QString();
	}

	switch (organisation) {
	case DO_SINGLE_FILE:
		return fileNameSecondaryKeySingleFile(fileName);
		break;
	case DO_YEARLY:
		return fileNameSecondaryKeyYearly(fileName);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QString();
}

QStringList MessageDbSet::existingDbFileNamesInLocation(const QString &locDir,
    const QString &primaryKey, bool testing, enum Organisation organisation,
    bool filesOnly)
{
	QStringList matchingFiles;

	bool matches;
	QDirIterator dirIt(locDir, QDirIterator::NoIteratorFlags);

	while (dirIt.hasNext()) {
		dirIt.next();
		if (filesOnly && !QFileInfo(dirIt.filePath()).isFile()) {
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

QString MessageDbSet::constructKey(const QString &primaryKey,
    const QString &secondaryKey, enum Organisation organisation)
{
	if (organisation == DO_UNKNOWN) {
		return QString();
	}

	QString key = primaryKey;
	if (organisation == DO_YEARLY) {
		key += "_" + secondaryKey;
	}
	return key;
}

int MessageDbSet::checkExistingDbFile(const QString &locDir,
    const QString &primaryKey, int flags)
{
	bool testing = flags & MDS_FLG_TESTING;

	QStringList fileNames(existingDbFileNamesInLocation(locDir,
	    primaryKey, testing, DO_UNKNOWN, false));
	enum Organisation organisation(dbOrganisation(locDir, primaryKey,
	    testing));

	if (!fileNames.isEmpty() && (organisation == DO_UNKNOWN)) {
		return MDS_ERR_MULTIPLE;
	}

	if (fileNames.isEmpty()) {
		return MDS_ERR_MISSFILE;
	}

	foreach (const QString &fileName, fileNames) {
		QString filePath(locDir + QDir::separator() + fileName);

		int ret = checkGivenDbFile(filePath, flags);
		if (ret != MDS_ERR_OK) {
			return ret;
		}
	}

	return MDS_ERR_OK;
}

QString MessageDbSet::constructDbFileName(const QString &locDir,
    const QString &primaryKey, const QString &secondaryKey,
    bool testing, enum Organisation organisation)
{
	QString key = constructKey(primaryKey, secondaryKey, organisation);
	if (key.isNull()) {
		return QString();
	}

	return locDir + QDir::separator() +
	    key + QString("___") + (testing ? "1" : "0") + DB_SUFFIX;
}

MessageDb *MessageDbSet::_accessMessageDb(const QString &secondaryKey,
    bool create)
{
	MessageDb *db = NULL;
	bool openRet;

	/* Already opened. */
	if (this->find(secondaryKey) != this->end()) {
		return (*this)[secondaryKey];
	}

	if (create && (m_organisation == DO_UNKNOWN)) {
		/* Organisation structure must be set. */
		return NULL;
	}

	if (!create && (m_organisation == DO_UNKNOWN)) {
		/* Try to determine the structure of the present database. */
		enum Organisation org = dbOrganisation(m_locDir, m_primaryKey,
		    m_testing);
		if (org == DO_UNKNOWN) {
			logErrorNL("The organisation structure of the database '%s' in '%s' could not be determined.",
			    m_primaryKey.toUtf8().constData(),
			    m_locDir.toUtf8().constData());
			return NULL;
		}
		m_organisation = org;
	}

	QString connectionName(constructKey(m_primaryKey, secondaryKey,
	    m_organisation));
	if (!m_connectionPrefix.isEmpty()) {
		connectionName = m_connectionPrefix + "_" + connectionName;
	}

	db = new(std::nothrow) MessageDb(connectionName);
	if (NULL == db) {
		Q_ASSERT(0);
		return NULL;
	}

	/* TODO -- Handle file name deviations! */
	/*
	 * Test accounts have ___1 in their names, ___0 relates to standard
	 * accounts.
	 */
	QString dbFileName = constructDbFileName(m_locDir, m_primaryKey,
	    secondaryKey, m_testing, m_organisation);
	QFileInfo fileInfo(dbFileName);

	if (!create && !fileInfo.isFile()) {
		delete db;
		return NULL;
	} else if (!fileInfo.isFile()) {
		/* Create missing directory. */
		QDir dir = fileInfo.absoluteDir().absolutePath();
		if (!dir.exists()) {
			/* Empty file will be created automatically. */
			if (!dir.mkpath(dir.absolutePath())) {
				/* Cannot create directory. */
				delete db;
				return NULL;
			}
		}
	}

	openRet = db->openDb(dbFileName);
	if (!openRet) {
		delete db;
		return NULL;
	}

	this->insert(secondaryKey, db);
	return db;
}

int MessageDbSet::checkGivenDbFile(const QString &filePath, int flags)
{
	bool checkQuick = flags & MDS_FLG_CHECK_QUICK;
	bool checkIntegity = flags & MDS_FLG_CHECK_INTEGRITY;

	if (checkIntegity) {
		checkQuick = false;
	}

	QFileInfo dbFileInfo(filePath);
	QDir dbDir(dbFileInfo.absoluteDir().absolutePath());
	QFileInfo dbDirInfo(dbDir.absolutePath());

	if (dbFileInfo.exists() && !dbFileInfo.isFile()) {
		return MDS_ERR_NOTAFILE;
	}

	if (!dbFileInfo.exists()) {
		if (!dbDirInfo.isReadable() || !dbDirInfo.isWritable()) {
			return MDS_ERR_ACCESS;
		} else {
			return MDS_ERR_MISSFILE;
		}
	} else {
		if (!dbFileInfo.isReadable() || !dbFileInfo.isWritable()) {
			return MDS_ERR_ACCESS;
		}
	}

	if (checkIntegity || checkQuick) {
		MessageDb db("someKey");
		if (!db.openDb(filePath, false)) {
			return MDS_ERR_DATA;
		}

		if (!db.checkDb(checkQuick)) {
			return MDS_ERR_DATA;
		}
	}

	return MDS_ERR_OK;
}
