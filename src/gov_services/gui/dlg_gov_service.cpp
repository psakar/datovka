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

#include <QLineEdit>
#include <QMessageBox>

#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/log/memory_log.h"
#include "src/datovka_shared/utility/strings.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/gov_services/gui/dlg_gov_service.h"
#include "src/gui/dlg_msg_box_informative.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task.h"
#include "src/worker/task_send_message.h"
#include "ui_dlg_gov_service.h"

DlgGovService::DlgGovService(const QString &userName,
    GovFormListModel *govFormModel, MessageDbSet *dbSet, QWidget *parent)
    : QDialog(parent),
    m_userName(userName),
    m_govFormModel(govFormModel),
    m_dbSet(dbSet),
    m_ui(new (std::nothrow) Ui::DlgGovService),
    m_transactIds()
{
	m_ui->setupUi(this);

	initDialog();
}

DlgGovService::~DlgGovService(void)
{
	delete m_ui;
}

void DlgGovService::haveAllMandatoryFields(void)
{
	m_ui->sendServiceButton->setEnabled(
	    m_govFormModel->service()->haveAllMandatoryFields());
}

void DlgGovService::onLineEditTextChanged(QString text)
{
	QObject* senderObj = sender();

	if (senderObj == Q_NULLPTR) {
		return;
	}

	if (senderObj->objectName().isNull()) {
		return;
	}

	foreach (const Gov::FormField &field, m_govFormModel->service()->fields()) {
		if (field.key() == senderObj->objectName()) {
			m_govFormModel->setKeyValue(field.key(), text);
		}
	}

	haveAllMandatoryFields();
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

	/* Create Gov message. */
	const Isds::Message msg(m_govFormModel->service()->dataMessage());
	if (Q_UNLIKELY(msg.isNull())) {
		Q_ASSERT(0);
		return;
	}

	/* Genterate unique identifier. */
	QStringList taskIdentifiers;
	const QDateTime currentTime(QDateTime::currentDateTimeUtc());
	taskIdentifiers.append(m_userName + "_" + currentTime.toString() + "_" +
	    Utility::generateRandomString(6));
	m_transactIds = taskIdentifiers.toSet();

	/* Send Gov message. */
	TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
	    m_userName, m_dbSet, taskIdentifiers.at(0), msg,
	    m_govFormModel->service()->instituteName(),
	    m_govFormModel->service()->boxId(), false, Task::PROC_NOTHING);
	if (task != Q_NULLPTR) {
		task->setAutoDelete(true);
		GlobInstcs::workPoolPtr->assignHi(task);
	}
}

void DlgGovService::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId, int processFlags)
{
	debugSlotCall();

	Q_UNUSED(userName);
	Q_UNUSED(isPDZ);
	Q_UNUSED(processFlags);

	if (m_transactIds.end() == m_transactIds.find(transactId)) {
		/* Nothing found. */
		return;
	}

	if (!m_transactIds.remove(transactId)) {
		logErrorNL("%s",
		    "Was not able to remove a transaction identifier from list of unfinished transactions.");
	}

	if (TaskSendMessage::SM_SUCCESS == result) {

		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Message sent"),
		    "<b>" + tr("Gov request was successfully sent to ISDS.") + "</b>",
		    tr("Message was sent to <i>%1 (%2)</i> as message number <i>%3</i>.").
		    arg(recipientName).arg(dbIDRecipient).arg(dmId) + "<br/>",
		    QString(), QMessageBox::Ok, QMessageBox::Ok);

		this->close(); /* Close dialog. */

	} else {

		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Message sent"),
		    "<b>" + tr("Gov request was NOT successfully sent to ISDS.") + "</b>",
		    tr("ISDS returns:") + " " + resultDesc,
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
	}
}

void DlgGovService::initDialog(void)
{
	this->setWindowTitle(m_govFormModel->service()->internalId());
	m_ui->serviceName->setText(m_govFormModel->service()->fullName());
	m_ui->serviceDbId->setText(QString("%1 -- %2").arg(m_govFormModel->
	    service()->boxId()).arg(m_govFormModel->service()->instituteName()));
	m_ui->userName->setText(m_userName);

	if (m_govFormModel->service()->fields().count() == 0) {
		m_ui->filedsLabel->setText(tr("Service does not require another data."));
	} else {
		foreach (const Gov::FormField &field,
		    m_govFormModel->service()->fields()) {
			QLineEdit *le = new QLineEdit();
			le->setObjectName(field.key());
			le->setPlaceholderText(field.placeholder());
			le->setText(field.val());
			le->setEnabled(field.val().isEmpty());
			m_ui->formGovLayout->addRow(field.descr(), le);
			if (le->isEnabled()) {
				connect(le, SIGNAL(textChanged(QString)),
				    this, SLOT(onLineEditTextChanged(QString)));
			}
		}
	}

	connect(m_ui->sendServiceButton, SIGNAL(clicked()),
	    this, SLOT(sendGovRequest()));
	connect(m_ui->cancelButton, SIGNAL(clicked()),
	    this, SLOT(close()));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)));

	haveAllMandatoryFields();
}
