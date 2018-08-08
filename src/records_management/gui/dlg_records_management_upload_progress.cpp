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

#include "src/datovka_shared/graphics/graphics.h"
#include "src/datovka_shared/io/records_management_db.h"
#include "src/global.h"
#include "src/records_management/gui/dlg_records_management_upload_progress.h"
#include "ui_dlg_records_management_progress.h"

#define LOGO_EDGE 64

#define PROGRESS_MIN 0
#define PROGRESS_MAX 100

/*
 * Upload time should be greater than the time needed for obtaining the response.
 */
#define UPLOAD_SHARE 90
#define DOWNLOAD_SHARE (PROGRESS_MAX - UPLOAD_SHARE)

DlgRecordsManagementUploadProgress::DlgRecordsManagementUploadProgress(
    qint64 dmId, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgRecordsManagementProgress)
{
	m_ui->setupUi(this);
	setWindowTitle(tr("Records Management Upload Progress"));

	m_ui->taskLabel->setText(
	    tr("Sending message '%1' into records management service.").arg(dmId));

	loadRecordsManagementPixmap(LOGO_EDGE);

	m_ui->taskProgress->setRange(PROGRESS_MIN, PROGRESS_MAX);
	m_ui->taskProgress->setValue(PROGRESS_MIN);

	m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

	/* Abort when dialogue closed or cancelled. */
	connect(this, SIGNAL(rejected()), this, SLOT(emitAbort()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(emitAbort()));
}

DlgRecordsManagementUploadProgress::~DlgRecordsManagementUploadProgress(void)
{
	delete m_ui;
}

void DlgRecordsManagementUploadProgress::onDownloadProgress(
    qint64 bytesReceived, qint64 bytesTotal)
{
	if (bytesTotal <= 0) {
		/* Undetermined progress bar. */
		m_ui->taskProgress->setRange(0, 0);
	} else {
		m_ui->taskProgress->setRange(PROGRESS_MIN, PROGRESS_MAX);
		m_ui->taskProgress->setValue(UPLOAD_SHARE + ((DOWNLOAD_SHARE * bytesReceived) / bytesTotal));
	}
}

void DlgRecordsManagementUploadProgress::onUploadProgress(
    qint64 bytesSent, qint64 bytesTotal)
{
	if (bytesTotal <= 0) {
		/* Undetermined progress bar. */
		m_ui->taskProgress->setRange(0, 0);
	} else {
		m_ui->taskProgress->setRange(PROGRESS_MIN, PROGRESS_MAX);
		m_ui->taskProgress->setValue(PROGRESS_MIN + ((UPLOAD_SHARE * bytesSent) / bytesTotal));
	}
}

void DlgRecordsManagementUploadProgress::emitAbort(void)
{
	emit callAbort();
}

void DlgRecordsManagementUploadProgress::loadRecordsManagementPixmap(int width)
{
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		return;
	}

	RecordsManagementDb::ServiceInfoEntry entry(
	    GlobInstcs::recMgmtDbPtr->serviceInfo());
	if (!entry.isValid() || entry.logoSvg.isEmpty()) {
		return;
	}
	QPixmap pixmap(Graphics::pixmapFromSvg(entry.logoSvg, width));
	if (!pixmap.isNull()) {
		m_ui->pixmapLabel->setPixmap(pixmap);
	}
}
