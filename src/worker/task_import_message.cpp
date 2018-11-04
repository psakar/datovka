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

#include <QThread>

#include "src/datovka_shared/isds/types.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/message_db_set.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_import_message.h"

TaskImportMessage::TaskImportMessage(const QString &userName,
    MessageDbSet *dbSet, const QStringList &dbFileList, const QString &dbId)
    : m_result(IMP_ERR),
    m_resultDescList(),
    m_msgCntTotal(0),
    m_importedMsg(0),
    m_userName(userName),
    m_dbSet(dbSet),
    m_dbFileList(dbFileList),
    m_dbId(dbId),
    m_resultDesc()
{
}

void TaskImportMessage::run(void)
{
	if (m_dbFileList.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_dbId.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (Q_NULLPTR == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting import message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = importMessages(m_userName, m_dbSet, m_dbFileList,
	    m_dbId, m_resultDescList, m_msgCntTotal, m_importedMsg);

	emit GlobInstcs::msgProcEmitterPtr->importMessageFinished(m_userName,
	    m_resultDescList, m_msgCntTotal, m_importedMsg);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Import message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskImportMessage::Result TaskImportMessage::importSingleMessage(
    const QString &userName, MessageDbSet *dbSet,
    const MessageDbSingle *srcDbSingle, const QString &dbFile,
    const MessageDb::MsgId &mId, const QString &dbId, QString &resultDesc)
{
	/* select target database via delivery time for account */
	MessageDb *dstDb = dbSet->accessMessageDb(mId.deliveryTime, true);
	if (Q_NULLPTR == dstDb) {
		resultDesc = QObject::tr("Failed to open database file of "
		    "target account '%1'").arg(userName);
		return TaskImportMessage::IMP_DB_ERROR;
	}

	/* check if msg exists in target database */
	if (Isds::Type::MS_NULL != dstDb->getMessageStatus(mId.dmId)) {
		resultDesc = QObject::tr("Message '%1' already exists in "
		    "database for this account.").arg(mId.dmId);
		return TaskImportMessage::IMP_DB_EXISTS;
	}

	/* check if msg is relevant for account databox ID  */
	if (!srcDbSingle->isRelevantMsgForImport(mId.dmId, dbId)) {
		resultDesc = QObject::tr("Message '%1' cannot be imported "
		    "into this account. Message does not "
		    "contain any valid ID of databox "
		    "corresponding with this account.").
		    arg(mId.dmId);
		return TaskImportMessage::IMP_MSG_ID_ERR;
	}

	/* copy all msg data to target account database */
	if (!dstDb->copyCompleteMsgDataToAccountDb(dbFile, mId.dmId)) {
		resultDesc = QObject::tr("Message '%1' cannot be inserted "
		    "into database of this account. An error "
		    "occurred during insertion procedure.").
		    arg(mId.dmId);
		return TaskImportMessage::IMP_DB_EXISTS;
	}
	return TaskImportMessage::IMP_SUCCESS;
}

enum TaskImportMessage::Result TaskImportMessage::importMessages(
    const QString &userName, MessageDbSet *dbSet,
    const QStringList &dbFileList, const QString &dbId,
    QStringList &resultDescList, int &msgCntTotal, int &importedMsg)
{
	bool dbTestingFlag;
	QString dbDir;
	QString dbFileName;
	QString dbUserName;
	QString dbYearFlag;
	QString resultDesc;

	float delta = 0.0;
	float diff = 0.0;

	/* via all files */
	foreach (const QString &dbFile, dbFileList) {

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    PL_IMPORT_MSG, 0);

		/* get db filename from path */
		QFileInfo file(dbFile);
		dbDir = file.path();
		dbFileName = file.fileName();

		/* parse and check the import database file name */
		if (!MessageDbSet::isValidDbFileName(dbFileName, dbUserName,
		    dbYearFlag, dbTestingFlag, resultDesc)) {
			resultDescList.append(resultDesc);
			continue;
		}

		/* check if username of db file is relevant to account */
		if (userName != dbUserName) {
			resultDesc = QObject::tr("Database file '%1' cannot "
			    "import into selected account because username "
			    "of account and username of database file do "
			    "not correspond.").arg(dbFileName);
			resultDescList.append(resultDesc);
			continue;
		}

		/* open selected database file as temporary single db */
		MessageDbSingle *srcDbSingle =
		     MessageDbSingle::createNew(dbFile, "TEMPORARYDBS");
		if (0 == srcDbSingle) {
			resultDesc = QObject::tr("Failed to open import database file %1'.").arg(dbFileName);
			resultDescList.append(resultDesc);
			continue;
		}

		/* get all messages from source single database */
		QList<MessageDb::MsgId> msgIdList(
		    srcDbSingle->getAllMessageIDsFromDB());

		msgCntTotal += msgIdList.count();

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    PL_IMPORT_MSG, 20);

		msgCntTotal += msgIdList.count();
		delta = 80.0 / msgCntTotal;

		/* over all messages in source database do import */
		foreach (const MessageDb::MsgId &mId, msgIdList) {

			if (msgCntTotal == 0) {
				emit GlobInstcs::msgProcEmitterPtr->progressChange(
				    PL_IMPORT_MSG, 50);
			} else {
				diff += delta;
				emit GlobInstcs::msgProcEmitterPtr->progressChange(
				    PL_IMPORT_MSG, (20 + diff));
			}
			if (TaskImportMessage::IMP_SUCCESS !=
			    importSingleMessage(userName, dbSet, srcDbSingle,
			        dbFile, mId, dbId, resultDesc)) {
				resultDescList.append(resultDesc);
			} else {
				importedMsg++;
			}
		}

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    PL_IMPORT_MSG, 100);

		delete srcDbSingle; srcDbSingle = NULL;
	}

	return TaskImportMessage::IMP_SUCCESS;
}
