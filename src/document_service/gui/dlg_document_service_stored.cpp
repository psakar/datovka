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
#include <QTimer>

#include "src/document_service/gui/dlg_document_service_stored.h"
#include "src/io/document_service_db.h"
#include "src/log/log.h"
#include "src/worker/pool.h"
#include "src/worker/task_document_service_stored_messages.h"
#include "ui_dlg_document_service_stored.h"

DlgDocumentServiceStored::DlgDocumentServiceStored(const QString &urlStr,
    const QString &tokenStr, const QList<AcntData> &accounts, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentServiceStored),
    m_url(urlStr),
    m_token(tokenStr),
    m_accounts(accounts)
{
	Q_ASSERT(!m_url.isEmpty());
	Q_ASSERT(!m_token.isEmpty());

	m_ui->setupUi(this);
	setWindowTitle(tr("Document Service Stored Messages"));

	m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

	QTimer::singleShot(500, this, SLOT(downloadAndStore()));
}

DlgDocumentServiceStored::~DlgDocumentServiceStored(void)
{
	delete m_ui;
}

bool DlgDocumentServiceStored::updateStoredInformation(
    DocumentServiceSettings &docSrvcSettings, const QList<AcntData> &accounts,
    QWidget *parent)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return false;
	}

	if (!docSrvcSettings.isSet()) {
		return false;
	}

	DlgDocumentServiceStored dlg(docSrvcSettings.url, docSrvcSettings.token,
	    accounts, parent);
	dlg.exec();

	return true;
}

void DlgDocumentServiceStored::downloadAndStore(void)
{
	QCoreApplication::processEvents();

	/* Update already held information. */
	{
		TaskDocumentServiceStoredMessages *task =
		    new (::std::nothrow) TaskDocumentServiceStoredMessages(
		        m_url, m_token,
		        TaskDocumentServiceStoredMessages::DS_UPDATE_STORED,
		        Q_NULLPTR);
		if (Q_NULLPTR == task) {
			logErrorNL("%s",
			    "Cannot create stored_files update task.");
			return;
		}
		task->setAutoDelete(false);
		/* This will block the GUI and all workers. */
		globWorkPool.runSingle(task); /* TODO -- Run in background. */

		delete task; task = Q_NULLPTR;
	}

	foreach (const AcntData &account, m_accounts) {
		if (account.dbSet == Q_NULLPTR) {
			Q_ASSERT(0);
			continue;
		}

		QCoreApplication::processEvents();

		TaskDocumentServiceStoredMessages *task =
		    new (::std::nothrow) TaskDocumentServiceStoredMessages(
		        globDocumentServiceSet.url,
		        globDocumentServiceSet.token,
		        TaskDocumentServiceStoredMessages::DS_DOWNLOAD_ALL,
		        account.dbSet);
		if (Q_NULLPTR == task) {
			logErrorNL("Cannot create stored_files task for '%s'.",
			    account.userName.toUtf8().constData());
			continue;
		}
		task->setAutoDelete(false);
		/* This will block the GUI and all workers. */
		globWorkPool.runSingle(task); /* TODO -- Run in background. */

		delete task; task = Q_NULLPTR;
	}

	QCoreApplication::processEvents();

	this->close();
}
