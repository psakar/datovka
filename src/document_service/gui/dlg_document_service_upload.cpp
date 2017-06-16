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

#include <cinttypes>
#include <QMessageBox>
#include <QString>

#include "src/document_service/gui/dlg_document_service_upload.h"
#include "src/document_service/json/upload_file.h"
#include "src/document_service/json/upload_hierarchy.h"
#include "src/io/document_service_db.h"
#include "src/models/sort_filter_proxy_model.h"
#include "src/log/log.h"
#include "ui_dlg_document_service_upload.h"

#define IGNORE_SSL_ERRORS true

DlgDocumentServiceUpload::DlgDocumentServiceUpload(const QString &urlStr,
    const QString &tokenStr, qint64 dmId, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentServiceUpload),
    m_url(urlStr),
    m_token(tokenStr),
    m_dsc(IGNORE_SSL_ERRORS, this),
    m_uploadModel(),
    m_uploadProxyModel(),
    m_selectedUploadIds()
{
	m_ui->setupUi(this);
	setWindowTitle(tr("Upload Message into Document Service"));

	m_ui->appealLabel->setText(
	    tr("Select the location where you want to upload the message '%1' into.")
	        .arg(dmId));

	connect(m_ui->reloadButton, SIGNAL(clicked(bool)),
	    this, SLOT(callUploadHierarchy()));

	m_ui->filterLine->setClearButtonEnabled(true);
	connect(m_ui->filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterHierarchy(QString)));

	m_ui->uploadView->setNarrowedLineHeight();

	m_ui->uploadView->setModel(&m_uploadProxyModel);
	m_uploadProxyModel.setSourceModel(&m_uploadModel);
	connect(m_ui->uploadView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this, SLOT(uploadHierarchySelectionChanged()));

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(&m_dsc, SIGNAL(connectionError(QString)),
	    this, SLOT(notifyCommunicationError(QString)));

	/* Dialogue window is not shown now so using parent instead. */
	callUploadHierarchy();
}

DlgDocumentServiceUpload::~DlgDocumentServiceUpload(void)
{
	delete m_ui;
}

bool DlgDocumentServiceUpload::uploadMessage(
    const DocumentServiceSettings &docSrvcSettings, qint64 dmId,
    const QString &msgFileName, const QByteArray &msgData, QWidget *parent)
{
	if (!docSrvcSettings.isSet()) {
		Q_ASSERT(0);
		return false;
	}

	if (msgFileName.isEmpty() || msgData.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	DlgDocumentServiceUpload dlg(docSrvcSettings.url, docSrvcSettings.token,
	    dmId, parent);
	if (QDialog::Accepted != dlg.exec()) {
		return false;
	}

	if (dlg.m_selectedUploadIds.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	/* The connection should be still working. */
	return uploadFile(dlg.m_dsc, dmId, dlg.m_selectedUploadIds, msgFileName,
	    msgData, parent);
}

void DlgDocumentServiceUpload::callUploadHierarchy(void)
{
	QByteArray response;

	/* Clear model. */
	m_uploadModel.setHierarchy(UploadHierarchyResp());

	m_dsc.setConnection(m_url, m_token);

	if (m_dsc.communicate(DocumentServiceConnection::SRVC_UPLOAD_HIERARCHY,
	        QByteArray(), response)) {
		if (!response.isEmpty()) {
			bool ok = false;
			UploadHierarchyResp uhRes(
			    UploadHierarchyResp::fromJson(response, &ok));
			if (!ok || !uhRes.isValid()) {
				QMessageBox::critical(this,
				    tr("Communication Error"),
				    tr("Received invalid response."));
				return;
			}

			m_uploadModel.setHierarchy(uhRes);
			m_ui->uploadView->expandAll();
		} else {
			QMessageBox::critical(this, tr("Communication Error"),
			    tr("Received empty response."));
			return;
		}
	} else {
		return;
	}
}

void DlgDocumentServiceUpload::filterHierarchy(const QString &text)
{
	m_uploadProxyModel.setFilterRole(UploadHierarchyModel::ROLE_FILTER);

	m_uploadProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));

	m_uploadProxyModel.setFilterKeyColumn(0);

	m_ui->uploadView->expandAll();

	if (text.isEmpty()) {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::blankFilterEditStyle);
	} else if (m_uploadProxyModel.rowCount() != 0) {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::foundFilterEditStyle);
	} else {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::notFoundFilterEditStyle);
	}
}

void DlgDocumentServiceUpload::uploadHierarchySelectionChanged(void)
{
	m_selectedUploadIds.clear();

	const QModelIndexList indexes(
	    m_ui->uploadView->selectionModel()->selectedIndexes());

	/* Entries with empty identifiers should not be able to be selected. */
	foreach (const QModelIndex &index, indexes) {
		const QString uploadId(
		    index.data(UploadHierarchyModel::ROLE_ID).toString());
		if (!uploadId.isEmpty()) {
			m_selectedUploadIds.append(uploadId);
		} else {
			Q_ASSERT(0);
		}
	}

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    !indexes.isEmpty());
}

void DlgDocumentServiceUpload::notifyCommunicationError(const QString &errMsg)
{
	QMessageBox::critical(this, tr("Communication Error"), errMsg);
}

/*!
 * @brief Process upload file service response.
 *
 * @note Stores location into database.
 *
 * @param[in] ifRes Response structure.
 * @param[in] dmId Message identifier.
 * @patam[in] parent Parent window for potential error dialogues.
 * @return True if response could be processed and location has been saved.
 */
static
bool processUploadFileResponse(const UploadFileResp &ufRes, qint64 dmId,
    QWidget *parent = Q_NULLPTR)
{
	if (ufRes.id().isEmpty()) {
		QString errorMessage(
		    QObject::tr("Message '%1' could not be uploaded.")
		        .arg(dmId));
		errorMessage += QLatin1String("\n");
		errorMessage += QObject::tr("Received error") +
		    QLatin1String(": ") + ufRes.error().trVerbose();
		errorMessage += QLatin1String("\n");
		errorMessage += ufRes.error().description();

		QMessageBox::critical(parent, QObject::tr("File Upload Error"),
		    errorMessage);
		return false;
	}

	if (!ufRes.locations().isEmpty()) {
		logInfoNL(
		    "Message '%" PRId64 "'has been stored into document service.",
		    dmId);
		if (Q_NULLPTR != globDocumentServiceDbPtr) {
			return globDocumentServiceDbPtr->updateStoredMsg(dmId,
			    ufRes.locations());
		} else {
			Q_ASSERT(0);
			return true;
		}
	} else {
		logErrorNL(
		    "Received empty location list when uploading message '%" PRId64 "'.",
		    dmId);
	}

	return false;
}

bool DlgDocumentServiceUpload::uploadFile(DocumentServiceConnection &dsc,
    qint64 dmId, const QStringList &uploadIds, const QString &msgFileName,
    const QByteArray &msgData, QWidget *parent)
{
	UploadFileReq ufReq(uploadIds, msgFileName, msgData);
	if (!ufReq.isValid()) {
		Q_ASSERT(0);
		return false;
	}

	QByteArray response;

	if (dsc.communicate(DocumentServiceConnection::SRVC_UPLOAD_FILE,
	        ufReq.toJson(), response)) {
		if (!response.isEmpty()) {
			bool ok = false;
			UploadFileResp ufRes(
			    UploadFileResp::fromJson(response, &ok));
			if (!ok || !ufRes.isValid()) {
				QMessageBox::critical(parent,
				    tr("Communication Error"),
				    tr("Received invalid response."));
				logErrorNL("Received invalid response '%s'.",
				    QString(response).toUtf8().constData());
				return false;
			}

			return processUploadFileResponse(ufRes, dmId, parent);
		} else {
			QMessageBox::critical(parent, tr("Communication Error"),
			    tr("Received empty response."));
			return false;
		}
	} else {
		return false;
	}
}
