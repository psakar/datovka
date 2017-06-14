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

#include <QString>

#include "src/document_service/gui/dlg_document_service_upload.h"
#include "src/models/sort_filter_proxy_model.h"
#include "ui_dlg_document_service_upload.h"

#define IGNORE_SSL_ERRORS true

DlgDocumentServiceUpload::DlgDocumentServiceUpload(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentServiceUpload),
    m_dsc(IGNORE_SSL_ERRORS, this),
    m_uploadModel(),
    m_uploadProxyModel()
{
	m_ui->setupUi(this);
	setWindowTitle(tr("Upload Message into Document Service"));

	m_ui->filterLine->setClearButtonEnabled(true);
	connect(m_ui->filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterHierarchy(QString)));

	m_ui->uploadView->setNarrowedLineHeight();

	m_ui->uploadView->setModel(&m_uploadProxyModel);
	m_uploadProxyModel.setSourceModel(&m_uploadModel);
}

DlgDocumentServiceUpload::~DlgDocumentServiceUpload(void)
{
	delete m_ui;
}

bool DlgDocumentServiceUpload::uploadMessage(
    const DocumentServiceSettings &docSrvcSettings, QWidget *parent)
{
	DlgDocumentServiceUpload dlg(parent);
	dlg.exec();

	return true;
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
