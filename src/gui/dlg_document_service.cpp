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

#include "src/document_service/json/service_info.h"
#include "src/gui/dlg_document_service.h"
#include "src/io/document_service_db.h"
#include "ui_dlg_document_service.h"

/*!
 * @brief Return disabled palette.
 */
static
const QPalette &disableEditPalette(void)
{
        static QPalette palette;
        static bool prepared = false;
        if (!prepared) {
                palette.setColor(QPalette::Base, Qt::lightGray);
                palette.setColor(QPalette::Text, Qt::darkGray);
                prepared = true;
        }
        return palette;
}

DlgDocumentService::DlgDocumentService(const QString &urlStr,
    const QString &tokenStr, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentService),
    m_dsc(false, this)
{
	m_ui->setupUi(this);

	m_ui->urlLine->setText(urlStr);
	m_ui->tokenLine->setText(tokenStr);

	m_ui->infoButton->setEnabled(false);
	m_ui->eraseButton->setEnabled(!m_ui->urlLine->text().isEmpty() ||
	    !m_ui->tokenLine->text().isEmpty());

	m_ui->nameLine->setReadOnly(true);
	m_ui->nameLine->setPalette(disableEditPalette());
	m_ui->tokenNameLine->setReadOnly(true);
	m_ui->tokenNameLine->setPalette(disableEditPalette());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(m_ui->urlLine, SIGNAL(textChanged(QString)),
	    this, SLOT(activateServiceButtons()));
	connect(m_ui->tokenLine, SIGNAL(textChanged(QString)),
	    this, SLOT(activateServiceButtons()));

	connect(m_ui->infoButton, SIGNAL(clicked(bool)),
	    this, SLOT(callServiceInfo()));
	connect(m_ui->eraseButton, SIGNAL(clicked(bool)),
	    this, SLOT(eraseContent()));

	connect(&m_dsc, SIGNAL(connectionError(QString)),
	    this, SLOT(notifyCommunicationError(QString)));

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
	QByteArray response;

	m_dsc.setConnection(m_ui->urlLine->text(), m_ui->tokenLine->text());

	if (m_dsc.communicate(DocumentServiceConnection::SRVC_GET_ALL_CLIENTS,
	        QByteArray(), response)) {
		if (!response.isEmpty()) {
			bool ok = false;
			ServiceInfoResp siResp(
			    ServiceInfoResp::fromJson(response, &ok));
			if (!ok || !siResp.isValid()) {
				return;
			}

			setResponseContent(siResp.logoSvg(), siResp.name(),
			    siResp.tokenName());
		} else {
			return;
		}
	} else {
		return;
	}

	m_ui->infoButton->setEnabled(false);
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgDocumentService::eraseContent(void)
{
	m_ui->urlLine->setText(QString());
	m_ui->tokenLine->setText(QString());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgDocumentService::notifyCommunicationError(const QString &errMsg)
{
	QMessageBox::critical(this, tr("Communication Error"), errMsg);
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

	setResponseContent(entry.logoSvg, entry.name, entry.tokenName);
}

void DlgDocumentService::setResponseContent(const QByteArray &logoSvg,
    const QString &name, const QString &tokenName)
{
	/* TODO -- Set SVG. */

	m_ui->nameLine->setText(name);
	m_ui->tokenNameLine->setText(tokenName);
}
