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


#include "src/gui/dlg_import_zfo_result.h"
#include "src/io/import_zfo.h"
#include "src/log/log.h"
#include "src/worker/pool.h"
#include "src/worker/task_import_zfo.h"
#include "src/worker/message_emitter.h"

ImportZfo::ImportZfo(QObject *parent)
    : QObject(parent),
    m_zfoFilesToImport(QSet<QString>()),
    m_numFilesToImport(0),
    m_importSucceeded(QList<QPair<QString, QString>>()),
    m_importExisted(QList<QPair<QString, QString>>()),
    m_importFailed(QList<QPair<QString, QString>>())
{
}

void ImportZfo::importZfoIntoDatabase(const QStringList &files,
    const QList<Task::AccountDescr> &accountList,
    enum ImportZFODialog::ZFOtype zfoType, bool authenticate, QString &errTxt)
{
	debugFuncCall();

	connect(&globMsgProcEmitter,
	    SIGNAL(importZfoFinished(QString, int, QString)), this,
	    SLOT(collectImportZfoStatus(QString, int, QString)));

	QPair<QString,QString> impZFOInfo;
	QSet<QString> messageZfoFiles;
	QSet<QString> deliveryZfoFiles;

	m_zfoFilesToImport.clear();
	m_importSucceeded.clear();
	m_importExisted.clear();
	m_importFailed.clear();

	/* Sort ZFOs by format types. */
	foreach (const QString &file, files) {
		switch (TaskImportZfo::determineFileType(file)) {
		case TaskImportZfo::ZT_UKNOWN:
			impZFOInfo.first = file;
			impZFOInfo.second = tr("Wrong ZFO format. This "
			    "file does not contain correct data for import.");
			m_importFailed.append(impZFOInfo);
			break;
		case TaskImportZfo::ZT_MESSAGE:
			if ((ImportZFODialog::IMPORT_ALL_ZFO == zfoType) ||
			    (ImportZFODialog::IMPORT_MESSAGE_ZFO == zfoType)) {
				messageZfoFiles.insert(file);
				m_zfoFilesToImport.insert(file);
			}
			break;
		case TaskImportZfo::ZT_DELIVERY_INFO:
			if ((ImportZFODialog::IMPORT_ALL_ZFO == zfoType) ||
			    (ImportZFODialog::IMPORT_DELIVERY_ZFO == zfoType)) {
				deliveryZfoFiles.insert(file);
				m_zfoFilesToImport.insert(file);
			}
			break;
		default:
			break;
		}
	}

	if (messageZfoFiles.isEmpty() && deliveryZfoFiles.isEmpty()) {
		errTxt = tr("The selection does not contain any valid ZFO file.");
		return;
	}

	m_numFilesToImport = m_zfoFilesToImport.size();

	/* First, import messages. */
	foreach (const QString &fileName, messageZfoFiles) {

		TaskImportZfo *task;
		task = new (std::nothrow) TaskImportZfo(accountList, fileName,
		    TaskImportZfo::ZT_MESSAGE, authenticate);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task);
	}
	/* Second, import delivery information. */
	foreach (const QString &fileName, deliveryZfoFiles) {

		TaskImportZfo *task;
		task = new (std::nothrow) TaskImportZfo(accountList, fileName,
		    TaskImportZfo::ZT_DELIVERY_INFO, authenticate);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task);
	}
}

void ImportZfo::collectImportZfoStatus(const QString &fileName, int result,
    const QString &resultDesc)
{
	debugSlotCall();

	logDebugLv0NL("Received import ZFO finished for file '%s' %d: '%s'",
	    fileName.toUtf8().constData(), result,
	    resultDesc.toUtf8().constData());

	switch (result) {
	case TaskImportZfo::IMP_SUCCESS:
		m_importSucceeded.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	case TaskImportZfo::IMP_DB_EXISTS:
		m_importExisted.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	default:
		m_importFailed.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	}

	if (!m_zfoFilesToImport.remove(fileName)) {
		logErrorNL("Processed ZFO file that '%s' the application "
		    "has not been aware of.", fileName.toUtf8().constData());
	}

	if (m_zfoFilesToImport.isEmpty()) {
		showImportZfoResultDialogue(m_numFilesToImport,
		    m_importSucceeded, m_importExisted, m_importFailed);

		m_numFilesToImport = 0;
		m_importSucceeded.clear();
		m_importExisted.clear();
		m_importFailed.clear();
	}
}

void ImportZfo::showImportZfoResultDialogue(int filesCnt,
    const QList<QPair<QString,QString>> &successFilesList,
    const QList<QPair<QString,QString>> &existFilesList,
    const QList<QPair<QString,QString>> &errorFilesList)
{
	debugFuncCall();

	QDialog *importZfoResult = new ImportZFOResultDialog(filesCnt,
	    errorFilesList, successFilesList, existFilesList, 0);
	importZfoResult->exec();
	importZfoResult->deleteLater();
}

ImportZfo globImportZfo;
