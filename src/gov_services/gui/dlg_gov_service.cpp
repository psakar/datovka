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

#include <QCalendarWidget>
#include <QLineEdit>

#include "src/datovka_shared/gov_services/service/gov_service_form_field.h"
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

DlgGovService::DlgGovService(const QString &userName, Gov::Service *gs,
    MessageDbSet *dbSet, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgGovService),
    m_userName(userName),
    m_gs(gs),
    m_dbSet(dbSet),
    m_transactIds()
{
	m_ui->setupUi(this);

	initDialog();
}

DlgGovService::~DlgGovService(void)
{
	delete m_gs; m_gs = Q_NULLPTR;
	delete m_ui;
}

void DlgGovService::openGovServiceForm(const QString &userName,
    Gov::Service *gs, MessageDbSet *dbSet, QWidget *parent)
{
	DlgGovService govServiceDialog(userName, gs, dbSet, parent);
	govServiceDialog.exec();
}

void DlgGovService::haveAllMandatoryFields(void)
{
	m_ui->invalidValueLabel->clear();
	m_ui->invalidValueLabel->setEnabled(false);
	m_ui->sendServiceButton->setEnabled(m_gs->haveAllMandatoryFields());
}

void DlgGovService::onLineEditTextChanged(QString text)
{
	/* Get pointer to sender. */
	QObject *senderObj = sender();
	if (senderObj == Q_NULLPTR || senderObj->objectName().isNull()) {
		return;
	}

	/* Set text value to the service field. */
	foreach (const Gov::FormField &field, m_gs->fields()) {
		if (field.key() == senderObj->objectName()) {
			m_gs->setFieldVal(field.key(), text);
		}
	}

	/* Check if service has all mandatory fields. */
	haveAllMandatoryFields();
}

void DlgGovService::onDateChanged(QDate date)
{
	/* Get pointer to sender. */
	QObject *senderObj = sender();
	if (senderObj == Q_NULLPTR || senderObj->objectName().isNull()) {
		return;
	}

	/* Set text value to the service field. */
	foreach (const Gov::FormField &field, m_gs->fields()) {
		if (field.key() == senderObj->objectName()) {
			m_gs->setFieldVal(field.key(), date.toString("yyyy-MM-dd"));
		}
	}

	/* Check if service has all mandatory fields. */
	haveAllMandatoryFields();
}

void DlgGovService::onCreateAndSendMsg(void)
{
	debugSlotCall();

	QString errDescr;

	/* Check if service has all mandatory fields. */
	if (!m_gs->haveAllMandatoryFields()) {
		logErrorNL("The e-gov service '%s' is missing some mandatory data.",
		    m_gs->internalId().toUtf8().constData());
		return;
	}

	/* Check if form has all fields valid. */
	if (!m_gs->haveAllValidFields(&errDescr)) {
		logWarningNL("The e-gov service '%s' has some mandatory data invalid.",
		    m_gs->internalId().toUtf8().constData());
		showValidityNotification(errDescr);
		return;
	}

	/* Create Gov message. */
	const Isds::Message msg(m_gs->dataMessage());
	if (Q_UNLIKELY(msg.isNull())) {
		Q_ASSERT(0);
		return;
	}

	/* Send Gov message. */
	sendGovMessage(msg);
}

void DlgGovService::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId, int processFlags)
{
	debugSlotCall();

	Q_UNUSED(isPDZ);
	Q_UNUSED(processFlags);

	/* Nothing found. */
	if (m_transactIds.end() == m_transactIds.find(transactId)) {
		return;
	}

	/* Remove transaction. */
	if (!m_transactIds.remove(transactId)) {
		logErrorNL("%s",
		    "Was not able to remove a transaction identifier from list of unfinished transactions.");
	}

	/* Show sent result dialog. */
	if (TaskSendMessage::SM_SUCCESS == result) {
		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Message sent: %1").arg(userName),
		    "<b>" + tr("Gov request was successfully sent to ISDS.") + "</b>",
		    tr("Message was sent to <i>%1 (%2)</i> as message number <i>%3</i>.").
		    arg(recipientName).arg(dbIDRecipient).arg(dmId) + "<br/>",
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
		this->close();
	} else {
		DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Message sent: %1").arg(userName),
		    "<b>" + tr("Gov request was NOT successfully sent to ISDS.") + "</b>",
		    tr("ISDS returns:") + " " + resultDesc,
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
	}
}

void DlgGovService::initDialog(void)
{
	/* Set window title and labels. */
	this->setWindowTitle(m_gs->internalId());
	m_ui->serviceName->setText(m_gs->fullName());
	m_ui->serviceDbId->setText(QString("%1 -- %2").arg(m_gs->boxId()).arg(m_gs->instituteName()));
	m_ui->userName->setText(m_userName);

	/* Set properties for error label. */
	m_ui->invalidValueLabel->setStyleSheet("QLabel { color: red }");
	m_ui->invalidValueLabel->setEnabled(false);

	/* Generate form layout from service fileds. */
	if (m_gs->fields().count() == 0) {
		m_ui->filedsLabel->setText(tr("Service does not require another data."));
	} else {
		generateFormLayoutUi();
	}

	/* Connect signal section. */
	connect(m_ui->sendServiceButton, SIGNAL(clicked()),
	    this, SLOT(onCreateAndSendMsg()));
	connect(m_ui->cancelButton, SIGNAL(clicked()),
	    this, SLOT(close()));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)));

	/* Check if service has all mandatory fields. */
	haveAllMandatoryFields();
}

void DlgGovService::generateFormLayoutUi(void)
{
	int formLine = 0;

	foreach (const Gov::FormField &field, m_gs->fields()) {
		QLabel *label = new QLabel(this);
		label->setObjectName(field.key() + QStringLiteral("Label"));
		m_ui->formGovLayout->setWidget(formLine, QFormLayout::LabelRole, label);

		if ((field.properties() & Gov::FormFieldType::PROP_TYPE_DATE) &&
		    field.val().isEmpty()) {
			/* Date input required. Show calendar. */
			QCalendarWidget *calendar = new QCalendarWidget(this);
			calendar->setObjectName(field.key());
			m_ui->formGovLayout->setWidget(formLine, QFormLayout::FieldRole, calendar);

			label->setText(field.placeholder());
			calendar->setMaximumDate(QDate::currentDate());
			calendar->setGridVisible(true);
			calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
			calendar->setHorizontalHeaderFormat(QCalendarWidget::NoHorizontalHeader);

			connect(calendar, SIGNAL(activated(QDate)),
			    this, SLOT(onDateChanged(QDate)));
		} else {
			/* Text input required. Show line edit. */
			QLineEdit *lineEdit = new QLineEdit(this);
			lineEdit->setObjectName(field.key());
			m_ui->formGovLayout->setWidget(formLine, QFormLayout::FieldRole, lineEdit);

			label->setText(field.descr());
			lineEdit->setPlaceholderText(field.placeholder());
			lineEdit->setText(field.val());
			lineEdit->setEnabled(field.val().isEmpty());

			connect(lineEdit, SIGNAL(textChanged(QString)),
			    this, SLOT(onLineEditTextChanged(QString)));
		}

		++formLine;
	}
}

void DlgGovService::showValidityNotification(const QString &errText)
{
	m_ui->invalidValueLabel->setEnabled(true);
	m_ui->invalidValueLabel->setText(errText);
}

void DlgGovService::sendGovMessage(const Isds::Message &msg)
{
	debugFuncCall();

	QString service(tr("Request: %1").arg(m_gs->fullName()));
	service.append("\n");
	service.append(tr("Recipient: %1").arg(m_gs->instituteName()));

	/* Show send message box. */
	int ret = DlgMsgBox::message(this, QMessageBox::Question,
	    tr("Send e-gov request"),
	    tr("Do you want to send the e-gov request to data box '%1'?").arg(m_gs->boxId()),
	    service, QString(),
	    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (ret == QMessageBox::No) {
		return;
	}

	/* Generate unique identifier. */
	const QDateTime currentTime(QDateTime::currentDateTimeUtc());
	QString taskIdentifier(m_userName + "_" + currentTime.toString() + "_" +
	    Utility::generateRandomString(6));
	m_transactIds.insert(taskIdentifier);

	/* Send Gov message to isds. */
	TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
	    m_userName, m_dbSet, taskIdentifier, msg,
	    m_gs->instituteName(),
	    m_gs->boxId(), false, Task::PROC_NOTHING);
	if (task != Q_NULLPTR) {
		task->setAutoDelete(true);
		GlobInstcs::workPoolPtr->assignHi(task);
	}
}
