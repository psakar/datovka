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

#include <QMessageBox>

#include "src/common.h"
#include "src/io/imports.h"
#include "src/io/message_db_single.h"
#include "src/log/log.h"
#include "src/worker/pool.h"
#include "src/worker/task_import_zfo.h"

void Imports::importDbMsgsIntoDatabase(QWidget *parent,
    MessageDbSet &dbSet, const QStringList &dbFileList,
    const QString &userName, const QString &dbId, QString &errTxt)
{
	debugFuncCall();

	int sMsgCnt;
	bool dbTestingFlag;
	QString baseText;
	QString informativeText;
	QString dbDir;
	QString dbFileName;
	QString dbUserName;
	QString dbYearFlag;
	QString detailText;
	QStringList errImportList;
	QString title = QObject::tr("Database import: %1").arg(userName);

	foreach (const QString &dbFile, dbFileList) {

		errImportList.clear();
		sMsgCnt = 0;
		baseText = QObject::tr("File") + ": " + dbFile;

		/* get db filename from path */
		QFileInfo file(dbFile);
		dbDir = file.path();
		dbFileName = file.fileName();

		/* parse and check the import database file name */
		if (!isValidDatabaseFileName(dbFileName, dbUserName,
		    dbYearFlag, dbTestingFlag, errTxt)) {
			showMessageBox(parent, title, baseText,
			    errTxt, NULL);
			continue;
		}

		/* check if username of db file is relevant to account */
		if (userName != dbUserName) {
			errTxt = QObject::tr("This database file cannot import into "
			    "selected account because username of account "
			    "and username of database file do not correspond.");
			showMessageBox(parent, title, baseText,
			    errTxt, NULL);
			continue;
		}

		/* open selected database file as temporary single db */
		MessageDbSingle *srcDbSingle =
		     MessageDbSingle::createNew(dbFile, "TEMPORARYDBS");
		if (0 == srcDbSingle) {
			errTxt = QObject::tr("Failed to open import database file.");
			showMessageBox(parent, title, baseText,
			    errTxt, NULL);
			continue;
		}

		/* get all messages from source single database */
		QList<MessageDb::MsgId> msgIdList(
		    srcDbSingle->getAllMessageIDsFromDB());

		/* over all messages in source database do import */
		foreach (const MessageDb::MsgId &mId, msgIdList) {

			/* select target database via delivery time for account */
			MessageDb *dstDb = dbSet.accessMessageDb(mId.deliveryTime, true);
			if (Q_NULLPTR == dstDb) {
				errTxt = QObject::tr("Failed to open database file of "
				    "target account '%1'").arg(userName);
				showMessageBox(parent, title, baseText,
				    errTxt, NULL);
				continue;
			}

			/* check if msg exists in target database */
			if (-1 != dstDb->msgsStatusIfExists(mId.dmId)) {
				errTxt = QObject::tr("Message '%1' already exists in "
				    "database for this account.").arg(mId.dmId);
				errImportList.append(errTxt);
				continue;
			}

			/* check if msg is relevant for account databox ID  */
			if (!srcDbSingle->isRelevantMsgForImport(mId.dmId, dbId)) {
				errTxt = QObject::tr("Message '%1' cannot be imported "
				    "into this account. Message does not "
				    "contain any valid ID of databox "
				    "corresponding with this account.").
				    arg(mId.dmId);
				errImportList.append(errTxt);
				continue;
			}

			/* copy all msg data to target account database */
			if (!dstDb->copyCompleteMsgDataToAccountDb(dbFile, mId.dmId)) {
				errTxt = QObject::tr("Message '%1' cannot be inserted "
				    "into database of this account. An error "
				    "occurred during insertion procedure.").
				    arg(mId.dmId);
				errImportList.append(errTxt);
				continue;
			}
			sMsgCnt++;
		}

		delete srcDbSingle; srcDbSingle = NULL;

		title = QObject::tr("Messages import result");
		baseText = QObject::tr("Import of messages into account '%1' "
		    "finished with result:").arg(userName) +
		    "<br/><br/>" +
		    QObject::tr("Source database file: '%1'").arg(dbFile);
		informativeText =
		    QObject::tr("Total of messages in database: %1").arg(msgIdList.count())
		    + "<br/><b>" +
		    QObject::tr("Imported messages: %1").arg(sMsgCnt)
		    + "<br/>" +
		    QObject::tr("Non-imported messages: %1").arg(errImportList.count()) + "</b><br/>";
		if (errImportList.count() > 0) {
			detailText = "";
			for (int m = 0; m < errImportList.count(); ++ m) {
				detailText += errImportList.at(m) + "\n";
			}
		}

		showMessageBox(parent, title, baseText,
		    informativeText, detailText);
	}
}

void Imports::importZfoIntoDatabase(const QStringList &fileList,
    const QList<Task::AccountDescr> &databaseList,
    enum ImportZFODialog::ZFOtype zfoType, bool authenticate,
    QSet<QString> &zfoFilesToImport,
    QList< QPair<QString, QString> > &zfoFilesInvalid,
    int &numFilesToImport, QString &errTxt)
{
	debugFuncCall();

	QPair<QString, QString> impZFOInfo;
	QSet<QString> messageZfoFiles;
	QSet<QString> deliveryZfoFiles;

	/* Sort ZFOs by format type. */
	foreach (const QString &file, fileList) {
		switch (TaskImportZfo::determineFileType(file)) {
		case TaskImportZfo::ZT_UKNOWN:
			impZFOInfo.first = file;
			impZFOInfo.second = QObject::tr("Wrong ZFO format. This "
			    "file does not contain correct data for import.");
			zfoFilesInvalid.append(impZFOInfo);
			break;
		case TaskImportZfo::ZT_MESSAGE:
			if ((ImportZFODialog::IMPORT_ALL_ZFO == zfoType) ||
			    (ImportZFODialog::IMPORT_MESSAGE_ZFO == zfoType)) {
				messageZfoFiles.insert(file);
				zfoFilesToImport.insert(file);
			}
			break;
		case TaskImportZfo::ZT_DELIVERY_INFO:
			if ((ImportZFODialog::IMPORT_ALL_ZFO == zfoType) ||
			    (ImportZFODialog::IMPORT_DELIVERY_ZFO == zfoType)) {
				deliveryZfoFiles.insert(file);
				zfoFilesToImport.insert(file);
			}
			break;
		default:
			break;
		}
	}

	if (messageZfoFiles.isEmpty() && deliveryZfoFiles.isEmpty()) {
		errTxt = QObject::tr("The selection does not contain any "
		    "valid ZFO file.");
		return;
	}

	numFilesToImport = zfoFilesToImport.size();

	/* First, import messages. */
	foreach (const QString &fileName, messageZfoFiles) {

		TaskImportZfo *task;
		task = new (std::nothrow) TaskImportZfo(databaseList, fileName,
		    TaskImportZfo::ZT_MESSAGE, authenticate);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task);
	}
	/* Second, import delivery information. */
	foreach (const QString &fileName, deliveryZfoFiles) {

		TaskImportZfo *task;
		task = new (std::nothrow) TaskImportZfo(databaseList, fileName,
		    TaskImportZfo::ZT_DELIVERY_INFO, authenticate);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task);
	}
}

void Imports::showMessageBox(QWidget *parent, const QString &title,
    const QString &baseText, const QString &informativeText,
    const QString &detailedText)
{
	QMessageBox msgBox(parent);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setWindowTitle(title);
	msgBox.setText(baseText);
	if (!informativeText.isEmpty()) {
		msgBox.setInformativeText(informativeText);
	}
	if (!detailedText.isEmpty()) {
		msgBox.setDetailedText(detailedText);
	}
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
}
