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
#include "src/datovka_shared/gov_services/service/gov_service.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/log/memory_log.h"
#include "src/datovka_shared/utility/strings.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/gov_services/gui/dlg_gov_service.h"
#include "src/gui/dlg_msg_box_informative.h"
#include "src/io/account_db.h"
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

	initDialogue();
}

DlgGovService::~DlgGovService(void)
{
	delete m_gs; m_gs = Q_NULLPTR;
	delete m_ui;
}

void DlgGovService::openForm(const QString &userName,
    const Gov::Service *cgs, MessageDbSet *dbSet, QWidget *parent)
{
	if (Q_UNLIKELY(userName.isEmpty() || (cgs == Q_NULLPTR) ||
	        (dbSet == Q_NULLPTR))) {
		return;
	}

	const Isds::DbOwnerInfo dbOwnerInfo(GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(userName)));
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return;
	}

	Gov::Service *gs = cgs->createNew();
	if (Q_UNLIKELY(gs == Q_NULLPTR)) {
		return;
	}
	/* Fill form data from account info. */
	gs->setOwnerInfoFields(dbOwnerInfo);

	DlgGovService dlg(userName, gs, dbSet, parent);
	dlg.exec();
}

void DlgGovService::lineEditTextChanged(const QString &text)
{
	/* Get pointer to sender. */
	const QObject *senderObj = QObject::sender();
	if (Q_UNLIKELY(senderObj == Q_NULLPTR || senderObj->objectName().isNull())) {
		return;
	}

	/* Set text value to the service field. */
	foreach (const Gov::FormField &field, m_gs->fields()) {
		if (field.key() == senderObj->objectName()) {
			m_gs->setFieldVal(field.key(), text);
		}
	}

	/* Check if request has all mandatory fields. */
	updateSendReadiness();
}

void DlgGovService::calendarDateChanged(void)
{
	/* Get pointer to sender. */
	const QObject *senderObj = QObject::sender();
	if (Q_UNLIKELY(senderObj == Q_NULLPTR || senderObj->objectName().isNull())) {
		return;
	}

	/* Set text value to the service field. */
	foreach (const Gov::FormField &field, m_gs->fields()) {
		if (field.key() == senderObj->objectName()) {
			const QCalendarWidget *calendar =
			    qobject_cast<const QCalendarWidget *>(senderObj);
			if (Q_UNLIKELY(calendar == Q_NULLPTR)) {
				Q_ASSERT(0);
				continue;
			}

			m_gs->setFieldVal(field.key(),
			    calendar->selectedDate().toString(GOV_DATE_FORMAT));
		}
	}

	/* Check if request has all mandatory fields. */
	updateSendReadiness();
}

void DlgGovService::createAndSendMsg(void)
{
	debugSlotCall();

	QString errDescr;

	/* Check if service has all mandatory fields. */
	if (!m_gs->haveAllMandatoryFields()) {
		logErrorNL(
		    "The e-gov request '%s' is missing some mandatory data.",
		    m_gs->internalId().toUtf8().constData());
		return;
	}

	/* Check if form has all fields valid. */
	if (!m_gs->haveAllValidFields(&errDescr)) {
		logWarningNL(
		    "The e-gov request '%s' contains some invalid mandatory data.",
		    m_gs->internalId().toUtf8().constData());
		showValidityNotification(errDescr);
		return;
	}

	/* Create data message containing e-gov request. */
	const Isds::Message msg(m_gs->dataMessage());
	if (Q_UNLIKELY(msg.isNull())) {
		Q_ASSERT(0);
		return;
	}

	/* Send e-gov request. */
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
	if (Q_UNLIKELY(m_transactIds.end() == m_transactIds.find(transactId))) {
		return;
	}

	/* Remove transaction. */
	if (!m_transactIds.remove(transactId)) {
		logErrorNL("%s",
		    "Unable to remove a transaction identifier from list of unfinished transactions.");
	}

	/* Show sent result dialogue. */
	if (TaskSendMessage::SM_SUCCESS == result) {
		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Message sent: %1").arg(userName),
		    "<b>" + tr("E-gov request was successfully sent.") + "</b>",
		    tr("Message was sent to <i>%1 (%2)</i> as message number <i>%3</i>.").
		    arg(recipientName).arg(dbIDRecipient).arg(dmId) + "<br/>",
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
		this->close();
	} else {
		DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Message sent: %1").arg(userName),
		    "<b>" + tr("E-gov request could NOT be sent.") + "</b>",
		    tr("ISDS returns:") + " " + resultDesc,
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
	}
}

void DlgGovService::initDialogue(void)
{
	/* Set window title and labels. */
	this->setWindowTitle(m_gs->internalId());
	m_ui->requestNameLabel->setText(m_gs->fullName());
	m_ui->boxIdLabel->setText(
	    QString("%1 -- %2").arg(m_gs->boxId()).arg(m_gs->instituteName()));
	m_ui->userNameLabel->setText(m_userName);

	/* Set properties for error label. */
	m_ui->invalidValueLabel->setStyleSheet("QLabel { color: red }");
	m_ui->invalidValueLabel->setEnabled(false);

	/* Generate form layout from service fields. */
	if (m_gs->fields().count() == 0) {
		m_ui->promptLabel->setText(tr("No user data needed."));
	} else {
		generateFormLayoutUi();
	}

	/* Connect signal section. */
	connect(m_ui->sendButton, SIGNAL(clicked()),
	    this, SLOT(createAndSendMsg()));
	connect(m_ui->cancelButton, SIGNAL(clicked()),
	    this, SLOT(close()));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)));

	/* Check if request has all mandatory fields. */
	updateSendReadiness();
}

void DlgGovService::updateSendReadiness(void)
{
	m_ui->invalidValueLabel->clear();
	m_ui->invalidValueLabel->setEnabled(false);
	m_ui->sendButton->setEnabled(m_gs->haveAllMandatoryFields());
}

void DlgGovService::generateFormLayoutUi(void)
{
	int formLine = 0;

	foreach (const Gov::FormField &ff, m_gs->fields()) {
		QLabel *label = new QLabel(this);
		label->setObjectName(ff.key() + QStringLiteral("Label"));
		m_ui->formGovLayout->setWidget(formLine, QFormLayout::LabelRole, label);

		if ((ff.properties() & Gov::FormFieldType::PROP_TYPE_DATE) &&
		    (ff.properties() & Gov::FormFieldType::PROP_USER_INPUT)) {
			/* Date input required. Show calendar. */
			QCalendarWidget *calendar = new QCalendarWidget(this);
			calendar->setObjectName(ff.key());
			m_ui->formGovLayout->setWidget(formLine, QFormLayout::FieldRole, calendar);

			label->setText(ff.placeholder());
			calendar->setMaximumDate(QDate::currentDate());
			calendar->setGridVisible(true);
			calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
			calendar->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
			calendar->setSelectionMode(QCalendarWidget::SingleSelection);

			if (ff.val().isEmpty()) {
				/*
				 * It is not possible to create the calendar without an
				 * already selected date. Therefore we directly set
				 * the selection into the underlying e-gov request
				 * container.
				 */
				m_gs->setFieldVal(ff.key(),
				    calendar->selectedDate().toString(GOV_DATE_FORMAT));
			} else {
				/* Set calendar. */
				calendar->setSelectedDate(
				    QDate::fromString(ff.val(), GOV_DATE_FORMAT));
			}

			connect(calendar, SIGNAL(selectionChanged()),
			    this, SLOT(calendarDateChanged()));
		} else {
			/* Text input required. Show line edit. */
			QLineEdit *lineEdit = new QLineEdit(this);
			lineEdit->setObjectName(ff.key());
			m_ui->formGovLayout->setWidget(formLine, QFormLayout::FieldRole, lineEdit);

			label->setText(ff.descr());
			lineEdit->setPlaceholderText(ff.placeholder());
			lineEdit->setText(ff.val());
			lineEdit->setEnabled(
			    ff.properties() & Gov::FormFieldType::PROP_USER_INPUT);

			connect(lineEdit, SIGNAL(textChanged(QString)),
			    this, SLOT(lineEditTextChanged(QString)));
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

	/* Send data message with e-gov request to ISDS. */
	TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
	    m_userName, m_dbSet, taskIdentifier, msg,
	    m_gs->instituteName(),
	    m_gs->boxId(), false, Task::PROC_NOTHING);
	if (task != Q_NULLPTR) {
		task->setAutoDelete(true);
		GlobInstcs::workPoolPtr->assignHi(task);
	}
}
