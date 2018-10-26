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

#include <QMessageBox>

#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/log/memory_log.h"
#include "src/global.h"
#include "src/gov_services/gui/dlg_gov_service.h"
#include "ui_dlg_gov_service.h"

DlgGovService::DlgGovService(const QString &userName,
    GovFormListModel *govFormModel, QWidget *parent)
    : QDialog(parent),
    m_userName(userName),
    m_govFormModel(govFormModel),
    m_ui(new (std::nothrow) Ui::DlgGovService)
{
	m_ui->setupUi(this);

	initDialog();
}

DlgGovService::~DlgGovService(void)
{
	delete m_ui;
}

void DlgGovService::initDialog(void)
{
	this->setWindowTitle(m_govFormModel->service()->internalId());
	m_ui->serviceName->setText(m_govFormModel->service()->fullName());
	m_ui->serviceDbId->setText(QString("%1 -- %2").arg(m_govFormModel->
	    service()->boxId()).arg(m_govFormModel->service()->instituteName()));
	m_ui->userName->setText(m_userName);

	m_ui->govFormTableView->setModel(m_govFormModel);

	if (m_ui->govFormTableView->model()->rowCount() == 0) {
		m_ui->filedsLabel->setText(tr("Service does not require another data."));
		m_ui->govFormTableView->setEnabled(false);
	}

	connect(m_ui->sendServiceButton, SIGNAL(clicked()),
	    this, SLOT(sendGovRequest()));
	connect(m_ui->cancelButton, SIGNAL(clicked()),
	    this, SLOT(close()));
	connect(m_govFormModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	    this, SLOT(haveAllMandatoryFields()));

	haveAllMandatoryFields();
}

void DlgGovService::haveAllMandatoryFields(void)
{
	m_ui->sendServiceButton->setEnabled(
	    m_govFormModel->service()->haveAllMandatoryFields());
}

void DlgGovService::sendGovRequest(void)
{
	debugSlotCall();

	QString service = "\n\n";
	service.append(tr("Request: %1").arg(m_govFormModel->service()->fullName()));
	service.append("\n");
	service.append(tr("Recipient: %1").arg(m_govFormModel->service()->instituteName()));

	QMessageBox::StandardButton reply = QMessageBox::question(this,
	    tr("Send e-gov request"),
	    tr("Do you want to send the e-gov request to data box '%1'?").arg(m_govFormModel->service()->boxId()) +
	    service,
	    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (reply == QMessageBox::No) {
		return;
	}

	/* Set message content according to model data. */
	if (!m_govFormModel->service()->haveAllMandatoryFields()) {
		logErrorNL("The e-gov service '%s' is missing some mandatory data.",
		    m_govFormModel->service()->internalId().toUtf8().constData());
		return;
	}

	const Isds::Message msg(m_govFormModel->service()->dataMessage());
	if (Q_UNLIKELY(msg.isNull())) {
		Q_ASSERT(0);
		return;
	}

	/* TODO - send request to ISDS */
}
