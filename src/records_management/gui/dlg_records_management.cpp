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

#include "src/datovka_shared/io/records_management_db.h"
#include "src/datovka_shared/records_management/json/service_info.h"
#include "src/global.h"
#include "src/records_management/gui/dlg_records_management.h"
#include "src/records_management/widgets/svg_view.h"
#include "ui_dlg_records_management.h"

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

DlgRecordsManagement::DlgRecordsManagement(const QString &urlStr,
    const QString &tokenStr, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgRecordsManagement),
    m_rmc(RecMgmt::Connection::ignoreSslErrorsDflt, this),
    m_logoSvg()
{
	m_ui->setupUi(this);

	m_ui->graphicsView->setSvgData(QByteArray());

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

	connect(&m_rmc, SIGNAL(connectionError(QString)),
	    this, SLOT(notifyCommunicationError(QString)));

	loadStoredServiceInfo();
}

DlgRecordsManagement::~DlgRecordsManagement(void)
{
	delete m_ui;
}

bool DlgRecordsManagement::updateSettings(
    RecordsManagementSettings &recMgmtSettings, QWidget *parent)
{
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		return false;
	}

	DlgRecordsManagement dlg(recMgmtSettings.url(), recMgmtSettings.token(),
	    parent);
	if (QDialog::Accepted != dlg.exec()) {
		return false;
	}

	if (!dlg.m_ui->urlLine->text().trimmed().isEmpty()) {
		Q_ASSERT(!dlg.m_ui->tokenLine->text().trimmed().isEmpty());

		/* Update entry. */
		RecordsManagementDb::ServiceInfoEntry entry;
		entry.url = dlg.m_ui->urlLine->text().trimmed();
		entry.name = dlg.m_ui->nameLine->text();
		entry.tokenName = dlg.m_ui->tokenNameLine->text();
		entry.logoSvg = dlg.m_logoSvg;
		GlobInstcs::recMgmtDbPtr->updateServiceInfo(entry);

		if (recMgmtSettings.url() != dlg.m_ui->urlLine->text()) {
			/* Erase all message-related data as URL has changed. */
			GlobInstcs::recMgmtDbPtr->deleteAllStoredMsg();
		}
	} else {
		GlobInstcs::recMgmtDbPtr->deleteAllEntries();
	}

	/* Save changes to settings. */
	recMgmtSettings.setUrl(dlg.m_ui->urlLine->text().trimmed());
	recMgmtSettings.setToken(dlg.m_ui->tokenLine->text().trimmed());

	return true;
}

void DlgRecordsManagement::activateServiceButtons(void)
{
	m_ui->infoButton->setEnabled(!m_ui->urlLine->text().isEmpty() &&
	    !m_ui->tokenLine->text().isEmpty());
	m_ui->eraseButton->setEnabled(!m_ui->urlLine->text().isEmpty() ||
	    !m_ui->tokenLine->text().isEmpty());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void DlgRecordsManagement::callServiceInfo(void)
{
	QByteArray response;

	m_rmc.setConnection(m_ui->urlLine->text().trimmed(),
	    m_ui->tokenLine->text().trimmed());

	if (m_rmc.communicate(RecMgmt::Connection::SRVC_SERVICE_INFO,
	        QByteArray(), response)) {
		if (!response.isEmpty()) {
			bool ok = false;
			RecMgmt::ServiceInfoResp siRes(
			    RecMgmt::ServiceInfoResp::fromJson(response, &ok));
			if (!ok || !siRes.isValid()) {
				QMessageBox::critical(this,
				    tr("Communication Error"),
				    tr("Received invalid response."));
				return;
			}

			setResponseContent(siRes.logoSvg(), siRes.name(),
			    siRes.tokenName());
		} else {
			QMessageBox::critical(this, tr("Communication Error"),
			    tr("Received empty response."));
			return;
		}
	} else {
		return;
	}

	m_ui->infoButton->setEnabled(false);
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgRecordsManagement::eraseContent(void)
{
	m_ui->urlLine->setText(QString());
	m_ui->tokenLine->setText(QString());

	m_ui->graphicsView->setSvgData(QByteArray());
	m_logoSvg = QByteArray();
	m_ui->nameLine->setText(QString());
	m_ui->tokenNameLine->setText(QString());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgRecordsManagement::notifyCommunicationError(const QString &errMsg)
{
	QMessageBox::critical(this, tr("Communication Error"), errMsg);
}

void DlgRecordsManagement::loadStoredServiceInfo(void)
{
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		return;
	}

	RecordsManagementDb::ServiceInfoEntry entry(
	    GlobInstcs::recMgmtDbPtr->serviceInfo());
	if (!entry.isValid()) {
		return;
	}

	setResponseContent(entry.logoSvg, entry.name, entry.tokenName);
}

void DlgRecordsManagement::setResponseContent(const QByteArray &logoSvg,
    const QString &name, const QString &tokenName)
{
	m_ui->graphicsView->setSvgData(logoSvg);
	m_logoSvg = logoSvg;
	m_ui->nameLine->setText(name);
	m_ui->tokenNameLine->setText(tokenName);
}
