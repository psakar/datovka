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
#include "src/graphics/graphics.h"
#include "src/io/document_service_db.h"
#include "src/log/log.h"
#include "src/worker/pool.h"
#include "src/worker/task_document_service_stored_messages.h"
#include "ui_dlg_document_service_stored.h"

#define LOGO_EDGE 64

#define PROGRESS_MIN 0
#define PROGRESS_MAX 100

#define RUN_DELAY_MS 500

DlgDocumentServiceStored::DlgDocumentServiceStored(const QString &urlStr,
    const QString &tokenStr, const QList<AcntData> &accounts, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentServiceStored),
    m_url(urlStr),
    m_token(tokenStr),
    m_accounts(accounts),
    m_taskIncr(PROGRESS_MAX / (1.0 + accounts.size())),
    m_cancel(false)
{
	Q_ASSERT(!m_url.isEmpty());
	Q_ASSERT(!m_token.isEmpty());

	m_ui->setupUi(this);
	setWindowTitle(tr("Document Service Stored Messages"));

	loadDocumentServicePixmap(LOGO_EDGE);

	m_ui->taskProgress->setRange(PROGRESS_MIN, PROGRESS_MAX);
	m_ui->taskProgress->setValue(PROGRESS_MIN);

	m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(cancelLoop()));

	QTimer::singleShot(RUN_DELAY_MS, this, SLOT(downloadAndStore()));
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
	QProgressBar *pBar = m_ui->taskProgress;

	QCoreApplication::processEvents();

	/* Update already held information. */
	{
		if (m_cancel) {
			goto cancel;
		}

		m_ui->taskLabel->setText(
		    tr("Updating stored information about messages."));

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
	pBar->setValue(pBar->value() + m_taskIncr);

	foreach (const AcntData &account, m_accounts) {
		if (m_cancel) {
			goto cancel;
		}

		m_ui->taskLabel->setText(
		    tr("Downloading information about messages from account %1 (%2).")
		        .arg(account.accountName).arg(account.userName));

		if (account.dbSet == Q_NULLPTR) {
			Q_ASSERT(0);
			pBar->setValue(pBar->value() + m_taskIncr);
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
			pBar->setValue(pBar->value() + m_taskIncr);
			continue;
		}
		task->setAutoDelete(false);
		/* This will block the GUI and all workers. */
		globWorkPool.runSingle(task); /* TODO -- Run in background. */

		delete task; task = Q_NULLPTR;

		pBar->setValue(pBar->value() + m_taskIncr);
	}

cancel:
	pBar->setValue(PROGRESS_MAX);

	QCoreApplication::processEvents();

	this->close();
}

void DlgDocumentServiceStored::cancelLoop(void)
{
	m_cancel = true;
}

void DlgDocumentServiceStored::loadDocumentServicePixmap(int width)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return;
	}

	DocumentServiceDb::ServiceInfoEntry entry(
	    globDocumentServiceDbPtr->serviceInfo());
	if (!entry.isValid() || entry.logoSvg.isEmpty()) {
		return;
	}
	QPixmap pixmap(Graphics::pixmapFromSvg(entry.logoSvg, width));
	if (!pixmap.isNull()) {
		m_ui->pixmapLabel->setPixmap(pixmap);
	}
}
