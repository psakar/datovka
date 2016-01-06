/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include "src/gui/dlg_import_zfo.h" /* TODO -- Remove this dependency. */
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_import_zfo.h"

TaskImportZfo::TaskImportZfo(const QString &userName, const QString &fileName)
    : m_userName(userName),
    m_fileName(fileName)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(!m_fileName.isEmpty());
}

void TaskImportZfo::run(void)
{
}

enum TaskImportZfo::ZfoType TaskImportZfo::determineFileType(
    const QString &fileName)
{
	debugFuncCall();

	ZfoType zfoType = ZT_UKNOWN;

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return zfoType;
	}

	struct isds_ctx *dummySession = isds_ctx_create();
	if (NULL == dummySession) {
		logErrorNL("%s", "Cannot create dummy ISDS session.");
		return zfoType;
	}

	struct isds_message *message = loadZfoFile(dummySession, fileName,
	    ImportZFODialog::IMPORT_MESSAGE_ZFO);
	if (NULL != message) {
		zfoType = ZT_MESSAGE;
	} else {
		message = loadZfoFile(dummySession, fileName,
		    ImportZFODialog::IMPORT_DELIVERY_ZFO);
		if(NULL != message) {
			zfoType = ZT_DELIVERY_INFO;
		} else {
			zfoType = ZT_UKNOWN;
		}
	}

	isds_message_free(&message);
	isds_ctx_free(&dummySession);

	return zfoType;
}
