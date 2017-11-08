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

#include <QTimer>

#include "src/graphics/graphics.h"
#include "src/io/records_management_db.h"
#include "src/log/log.h"
#include "src/records_management/gui/dlg_records_management_stored.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h"
#include "src/worker/task_records_management_stored_messages.h"
#include "ui_dlg_records_management_stored.h"

#define LOGO_EDGE 64

#define PROGRESS_MIN 0
#define PROGRESS_MAX 100

#define RUN_DELAY_MS 500

DlgRecordsManagementStored::DlgRecordsManagementStored(const QString &urlStr,
    const QString &tokenStr, const QList<AcntData> &accounts, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgRecordsManagementStored),
    m_url(urlStr),
    m_token(tokenStr),
    m_accounts(accounts),
    m_accIdx(0),
    m_taskIncr(PROGRESS_MAX / (1.0 + accounts.size())),
    m_cancel(false)
{
	Q_ASSERT(!m_url.isEmpty());
	Q_ASSERT(!m_token.isEmpty());

	m_ui->setupUi(this);

	/* Just to make the progress bar stationary. */
	m_ui->taskLabel->setText(QStringLiteral("\n"));

	loadRecordsManagementPixmap(LOGO_EDGE);

	m_ui->taskProgress->setRange(PROGRESS_MIN, PROGRESS_MAX);
	m_ui->taskProgress->setValue(PROGRESS_MIN);

	m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(cancelLoop()));
	connect(&globMsgProcEmitter,
	    SIGNAL(recordsManagementStoredMessagesFinished(QString)),
	    this, SLOT(downloadAndStoreContinue()));

	/* Currently there is no means how to detect whether dialogue is shown. */
	QTimer::singleShot(RUN_DELAY_MS, this, SLOT(downloadAndStoreStart()));
}

DlgRecordsManagementStored::~DlgRecordsManagementStored(void)
{
	delete m_ui;
}

bool DlgRecordsManagementStored::updateStoredInformation(
    const RecordsManagementSettings &recMgmtSettings,
    const QList<AcntData> &accounts, QWidget *parent)
{
	if (Q_NULLPTR == globRecordsManagementDbPtr) {
		return false;
	}

	if (!recMgmtSettings.isSet()) {
		return false;
	}

	DlgRecordsManagementStored dlg(recMgmtSettings.url(),
	    recMgmtSettings.token(), accounts, parent);
	dlg.exec();

	return true;
}

void DlgRecordsManagementStored::downloadAndStoreStart(void)
{
	QProgressBar *pBar = m_ui->taskProgress;

	/* Update already held information. */
	{
		if (m_cancel) {
			goto cancel;
		}

		m_ui->taskLabel->setText(
		    tr("Updating stored information about messages.") +
		    QStringLiteral("\n"));

		TaskRecordsManagementStoredMessages *task =
		    new (::std::nothrow) TaskRecordsManagementStoredMessages(
		        m_url, m_token,
		        TaskRecordsManagementStoredMessages::RM_UPDATE_STORED,
		        Q_NULLPTR);
		if (Q_NULLPTR == task) {
			logErrorNL("%s",
			    "Cannot create stored_files update task.");
			return;
		}
		task->setAutoDelete(true);
		/* Run in background. */
		globWorkPool.assignHi(task);

		return;
	}

cancel:
	pBar->setValue(PROGRESS_MAX);

	this->close();
}

void DlgRecordsManagementStored::downloadAndStoreContinue(void)
{
	QProgressBar *pBar = m_ui->taskProgress;

	pBar->setValue(pBar->value() + m_taskIncr);

	while (m_accIdx < m_accounts.size()) {
		if (m_cancel) {
			goto cancel;
		}

		const AcntData &account(m_accounts.at(m_accIdx));

		m_ui->taskLabel->setText(
		    tr("Downloading information about messages from account:\n%1 (%2).")
		        .arg(account.accountName).arg(account.userName));

		if (account.dbSet == Q_NULLPTR) {
			Q_ASSERT(0);
			pBar->setValue(pBar->value() + m_taskIncr);
			++m_accIdx;
			continue;
		}

		TaskRecordsManagementStoredMessages *task =
		    new (::std::nothrow) TaskRecordsManagementStoredMessages(
		        m_url, m_token,
		        TaskRecordsManagementStoredMessages::RM_DOWNLOAD_ALL,
		        account.dbSet);
		if (Q_NULLPTR == task) {
			logErrorNL("Cannot create stored_files task for '%s'.",
			    account.userName.toUtf8().constData());
			pBar->setValue(pBar->value() + m_taskIncr);
			++m_accIdx;
			continue;
		}
		task->setAutoDelete(true);
		/* Run in background. */
		globWorkPool.assignHi(task);

		++m_accIdx;
		return;
	}

cancel:
	pBar->setValue(PROGRESS_MAX);

	this->close();
}

void DlgRecordsManagementStored::cancelLoop(void)
{
	m_cancel = true;
}

void DlgRecordsManagementStored::loadRecordsManagementPixmap(int width)
{
	if (Q_NULLPTR == globRecordsManagementDbPtr) {
		return;
	}

	RecordsManagementDb::ServiceInfoEntry entry(
	    globRecordsManagementDbPtr->serviceInfo());
	if (!entry.isValid() || entry.logoSvg.isEmpty()) {
		return;
	}
	QPixmap pixmap(Graphics::pixmapFromSvg(entry.logoSvg, width));
	if (!pixmap.isNull()) {
		m_ui->pixmapLabel->setPixmap(pixmap);
	}
}
