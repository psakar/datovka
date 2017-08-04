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
#include "src/log/log.h"
#include "src/worker/pool.h"
#include "src/worker/task_import_message.h"
#include "src/worker/task_import_zfo.h"

void Imports::importDbMsgsIntoDatabase(MessageDbSet &dbSet,
    const QStringList &dbFileList, const QString &userName, const QString &dbId)
{
	debugFuncCall();

	TaskImportMessage *task;
	task = new (std::nothrow) TaskImportMessage(userName, &dbSet,
	    dbFileList, dbId);
	task->setAutoDelete(true);
	globWorkPool.assignLo(task);
}

void Imports::importZfoIntoDatabase(const QStringList &fileList,
    const QList<Task::AccountDescr> &databaseList, enum Type zfoType,
    bool authenticate, QSet<QString> &zfoFilesToImport,
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
			impZFOInfo.second = QObject::tr("Wrong ZFO format. "
			    "This file does not contain correct data for import.");
			zfoFilesInvalid.append(impZFOInfo);
			break;
		case TaskImportZfo::ZT_MESSAGE:
			if ((IMPORT_ANY == zfoType) || (IMPORT_MESSAGE == zfoType)) {
				messageZfoFiles.insert(file);
				zfoFilesToImport.insert(file);
			}
			break;
		case TaskImportZfo::ZT_DELIVERY_INFO:
			if ((IMPORT_ANY == zfoType) || (IMPORT_DELIVERY == zfoType)) {
				deliveryZfoFiles.insert(file);
				zfoFilesToImport.insert(file);
			}
			break;
		default:
			break;
		}
	}

	if (messageZfoFiles.isEmpty() && deliveryZfoFiles.isEmpty()) {
		errTxt = QObject::tr(
		    "The selection does not contain any valid ZFO file.");
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
