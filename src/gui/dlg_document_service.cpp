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

#include "src/gui/dlg_document_service.h"
#include "src/io/document_service_db.h"
#include "ui_dlg_document_service.h"

DlgDocumentService::DlgDocumentService(const QString &urlStr,
    const QString &tokenStr, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentService)
{
	m_ui->setupUi(this);

	m_ui->urlLine->setText(urlStr);
	m_ui->tokenLine->setText(tokenStr);

	m_ui->infoButton->setEnabled(false);
	m_ui->eraseButton->setEnabled(!m_ui->urlLine->text().isEmpty() ||
	    !m_ui->tokenLine->text().isEmpty());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(m_ui->urlLine, SIGNAL(textChanged(QString)),
	    this, SLOT(activateServiceButtons()));
	connect(m_ui->tokenLine, SIGNAL(textChanged(QString)),
	    this, SLOT(activateServiceButtons()));

	connect(m_ui->infoButton, SIGNAL(clicked(bool)),
	    this, SLOT(callServiceInfo()));
	connect(m_ui->eraseButton, SIGNAL(clicked(bool)),
	    this, SLOT(eraseContent()));

	loadStoredServiceInfo();
}

DlgDocumentService::~DlgDocumentService(void)
{
	delete m_ui;
}

bool DlgDocumentService::updateSettings(
    DocumentServiceSettings &docSrvcSettings, QWidget *parent)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return false;
	}

	DocumentServiceDb::ServiceInfoEntry entry(
	    globDocumentServiceDbPtr->serviceInfo());

	DlgDocumentService dlg(docSrvcSettings.url, docSrvcSettings.token,
	    parent);
	if (QDialog::Accepted != dlg.exec()) {
		return false;
	}

	if ((docSrvcSettings.url == dlg.m_ui->urlLine->text()) &&
	    (docSrvcSettings.token == dlg.m_ui->tokenLine->text())) {
		/* No change. */
		return false;
	}

	/* Save changes. */
	docSrvcSettings.url = dlg.m_ui->urlLine->text();
	docSrvcSettings.token = dlg.m_ui->tokenLine->text();

	return true;
}

void DlgDocumentService::activateServiceButtons(void)
{
	m_ui->infoButton->setEnabled(!m_ui->urlLine->text().isEmpty() &&
	    !m_ui->tokenLine->text().isEmpty());
	m_ui->eraseButton->setEnabled(!m_ui->urlLine->text().isEmpty() ||
	    !m_ui->tokenLine->text().isEmpty());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void DlgDocumentService::callServiceInfo(void)
{
	/* TODO -- Check connection. */

	m_ui->infoButton->setEnabled(false);
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgDocumentService::eraseContent(void)
{
	m_ui->urlLine->setText(QString());
	m_ui->tokenLine->setText(QString());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgDocumentService::loadStoredServiceInfo(void)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return;
	}

	DocumentServiceDb::ServiceInfoEntry entry(
	    globDocumentServiceDbPtr->serviceInfo());
	if (!entry.isValid()) {
		return;
	}
}
