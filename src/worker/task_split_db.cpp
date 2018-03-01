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

#include <QThread>

#include "src/global.h"
#include "src/io/message_db_set_container.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_split_db.h"

TaskSplitDb::TaskSplitDb(MessageDbSet *dbSet, const QString &userName,
    const QString &dbDir, const QString &newDbDir, bool isTestAccount)
    : m_success(false),
    m_error(),
    m_dbSet(dbSet),
    m_userName(userName),
    m_dbDir(dbDir),
    m_newDbDir(newDbDir),
    m_isTestAccount(isTestAccount)
{
}

void TaskSplitDb::run(void)
{
	if (Q_NULLPTR == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_dbDir.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_newDbDir.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting database split task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = splitMsgDbByYears(m_dbSet, m_userName, m_dbDir, m_newDbDir,
	    m_isTestAccount, m_error);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Database split task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskSplitDb::setBackOriginDb(MessageDbSet *dbset, const QString &dbDir)
{
	if (Q_NULLPTR == dbset) {
		return false;
	}

	/* set back and open origin database */
	if (!dbset->openLocation(dbDir, dbset->organisation(),
	    MessageDbSet::CM_MUST_EXIST)) {
		return false;
	}
	return true;
}

bool TaskSplitDb::splitMsgDbByYears(MessageDbSet *dbSet,
    const QString &userName, const QString &dbDir, const QString &newDbDir,
    bool isTestAccount, QString &errStr)
{
	QString testAcnt = "0";
	float delta = 0.0;
	float diff = 0.0;
	int flags = 0;

	errStr = QObject::tr("Action was canceled and original database file was returned back.");

	/* is testing account db */
	if (isTestAccount) {
		testAcnt = "1";
		flags |= MDS_FLG_TESTING;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 5);

	/*
	 * test if current account already use database files
	 * split according to years
	 */
	if (MessageDbSet::DO_YEARLY == dbSet->organisation()) {
		errStr = QObject::tr("Database file cannot split by years "
		    "because this account already use database files "
		    "split according to years.");
		return false;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 10);
	emit GlobInstcs::msgProcEmitterPtr->statusBarChange(
	    QObject::tr("Copying origin database file to selected location"));

	/* copy current account dbset to new location and open here */
	if (!dbSet->copyToLocation(newDbDir)) {
		errStr = QObject::tr("Cannot copy database file for account '%1' to '%2'. "
		    "Probably not enough disk space.").arg(userName).arg(newDbDir);
		return false;
	}

	/* get message db for splitting */
	MessageDb *messageDb = dbSet->accessMessageDb(QDateTime(), false);
	if (Q_NULLPTR == messageDb) {
		errStr = QObject::tr("Database file for account '%1' does not exist.").arg(userName);
		/* set back and open origin database */
		setBackOriginDb(dbSet, dbDir);
		return false;
	}

	/* get all unique years from database */
	QStringList yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED,
	    DESCENDING);
	yearList.append(dbSet->msgsYears(MessageDb::TYPE_SENT,
	    DESCENDING));
	yearList.removeDuplicates();

	/* create new db set for splitting of database files */
	MessageDbSet *dstDbSet = Q_NULLPTR;
	DbContainer temporaryDbCont("TEMPORARYDBS");
	/* open destination database file */
	dstDbSet = temporaryDbCont.accessDbSet(newDbDir, userName,
	    flags, MessageDbSet::DO_YEARLY, MessageDbSet::CM_CREATE_ON_DEMAND);
	if (0 == dstDbSet) {
		errStr = QObject::tr("Set of new database files for account '%1' "
		    "could not be created.").arg(userName);
		setBackOriginDb(dbSet, dbDir);
		return false;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 20);

	int years = yearList.count();
	if (years > 0) {
		delta = 60.0 / years;
	}

	for (int i = 0; i < years; ++i) {

		diff += delta;

		emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB,
		    (20 + diff));
		emit GlobInstcs::msgProcEmitterPtr->statusBarChange(
		    QObject::tr("Creating a new database file for year %1").arg(yearList.at(i)));

		QString newDbName = userName + "_" + yearList.at(i);

		QString dateStr = QString("%1-06-06 06:06:06.000")
		    .arg(yearList.at(i));
		QDateTime fakeTime = QDateTime::fromString(dateStr,
		    "yyyy-MM-dd HH:mm:ss.zzz");

		/* Delete the database file if it already exists. */
		QString fullNewDbFileName(newDbDir + "/" +
		    newDbName + "___" + testAcnt + ".db");
		if (QFileInfo::exists(fullNewDbFileName)) {
			if (QFile::remove(fullNewDbFileName)) {
				logInfo("Deleted existing file '%s'.",
				    fullNewDbFileName.toUtf8().constData());
			} else {
				logErrorNL("Cannot delete file '%s'.",
				    fullNewDbFileName.toUtf8().constData());
				errStr = QObject::tr("Existing file '%1' could not be deleted.").arg(fullNewDbFileName);
				return false;
			}
		}

		/* select destination database via fake delivery time */
		MessageDb *dstDb =
		    dstDbSet->accessMessageDb(fakeTime, true);
		if (0 == dstDb) {
			errStr = QObject::tr("New database file for account '%1' "
			    "corresponds with year '%2' could not be created. "
			    "Messages were not copied.").arg(userName).arg(yearList.at(i));
			setBackOriginDb(dbSet, dbDir);
			return false;
		}

		/* copy all message data to new database */
		if (!messageDb->copyRelevantMsgsToNewDb(fullNewDbFileName,
		        yearList.at(i))) {
			errStr = QObject::tr("Messages correspond with year '%1' "
			    "for account '%2' were not copied.").arg(yearList.at(i)).arg(userName);
			setBackOriginDb(dbSet, dbDir);
			return false;
		}
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 85);

	/* set back original database path and removed previous connection */
	if (!dbSet->openLocation(dbDir, dbSet->organisation(),
	    MessageDbSet::CM_MUST_EXIST)) {
		errStr = QObject::tr("Error to set and open original database for account '%1'.").arg(userName);
		errStr += " ";
		errStr += QObject::tr("Action was canceled and the origin database "
		    "is now used from location:\n'%1'").arg(newDbDir);
		return false;
	}

	emit GlobInstcs::msgProcEmitterPtr->statusBarChange(
	    QObject::tr("Replacing of new database files to origin database location"));
	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 90);

	/* move new database set to origin database path */
	if (!dstDbSet->moveToLocation(dbDir)) {
		errStr = QObject::tr("Error when move new databases for account '%1'").arg(userName);
		errStr += " ";
		errStr += QObject::tr("Action was canceled because new databases "
		    "cannot move from\n'%1'\nto origin path\n'%2'").arg(newDbDir).arg(dbDir);
		errStr += "\n\n";
		errStr += QObject::tr("Probably not enough disk space. The origin database is still used.");
		return false;
	}

	emit GlobInstcs::msgProcEmitterPtr->statusBarChange(
	    QObject::tr("Deleting of old database from origin location"));

	/* delete origin database file */
	if (!dbSet->deleteLocation()) {
		errStr = QObject::tr("Error when removed origin database for account '%1'").arg(userName);
		errStr += " ";
		errStr += QObject::tr("Action was canceled.");
		errStr += " ";
		errStr += QObject::tr("Please, remove the origin database file manually "
		    "from origin location:\n'%1'").arg(dbDir);
		return false;
	}

	emit GlobInstcs::msgProcEmitterPtr->statusBarChange(
	    QObject::tr("Opening of new database files"));
	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 95);

	/* open new database set in the origin location */
	if (!dbSet->openLocation(dbDir, dbSet->organisation(),
	    MessageDbSet::CM_MUST_EXIST)) {
		errStr = QObject::tr("A problem when opening new databases for account '%1'").arg(userName);
		errStr += " ";
		errStr += QObject::tr("Action was done but it cannot open new database files.");
		errStr += " ";
		errStr += QObject::tr("Please, restart the application.");
		return false;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_SPLIT_DB, 100);

	return true;
}
