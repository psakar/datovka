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

#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimeZone>

#include "src/crypto/crypto_funcs.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/io/dbs.h"
#include "src/io/filesystem.h"
#include "src/io/isds_sessions.h"
#include "src/isds/isds_conversion.h"
#include "src/log/log.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/settings/preferences.h"
#include "src/views/table_home_end_filter.h"
#include "ui_dlg_view_zfo.h"

DlgViewZfo::DlgViewZfo(const QString &zfoFileName, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgViewZfo),
    m_message(NULL),
    m_attachmentModel(this)
{
	m_ui->setupUi(this);

	/* Set default line height for table views/widgets. */
	m_ui->attachmentTable->setNarrowedLineHeight();

	/* Load message ZFO. */
	parseZfoFile(zfoFileName);

	if (NULL == m_message) {
		/* Just show error message. */
		m_ui->attachmentTable->hide();
		m_ui->envelopeTextEdit->setHtml(
		    "<h3>" + tr("Error parsing content") + "</h3><br/>" +
		    tr("Cannot parse the content of file '%1'.")
		        .arg(zfoFileName));
		m_ui->envelopeTextEdit->setReadOnly(true);
		m_ui->signaturePushButton->setEnabled(false);
		return;
	}

	setUpDialogue();
}

DlgViewZfo::DlgViewZfo(const QByteArray &zfoData, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgViewZfo),
    m_message(NULL),
    m_attachmentModel(this)
{
	m_ui->setupUi(this);

	/* Set default line height for table views/widgets. */
	m_ui->attachmentTable->setNarrowedLineHeight();

	/* Load raw message. */
	parseZfoData(zfoData);

	if (NULL == m_message) {
		/* Just show error message. */
		m_ui->attachmentTable->hide();
		m_ui->envelopeTextEdit->setHtml(
		    "<h3>" + tr("Error parsing content") + "</h3><br/>" +
		    tr("Cannot parse the content of message."));
		m_ui->envelopeTextEdit->setReadOnly(true);
		m_ui->signaturePushButton->setEnabled(false);
		return;
	}

	setUpDialogue();
}

DlgViewZfo::~DlgViewZfo(void)
{
	if (NULL != m_message) {
		isds_message_free(&m_message);
	}
	delete m_ui;
}

void DlgViewZfo::attachmentItemRightClicked(const QPoint &point)
{
	QModelIndex index(m_ui->attachmentTable->indexAt(point));
	QMenu *menu = new QMenu(this);

	/* Detects selection of multiple attachments. */
	QModelIndexList indexes(
	    AttachmentInteraction::selectedColumnIndexes(*m_ui->attachmentTable,
	        DbFlsTblModel::FNAME_COL));

	if (index.isValid()) {
		menu->addAction(QIcon(ICON_3PARTY_PATH "folder_16.png"),
		    tr("Open attachment"), this,
		    SLOT(openSelectedAttachment()))->
		        setEnabled(indexes.size() == 1);
		menu->addAction(QIcon(ICON_3PARTY_PATH "save_16.png"),
		    tr("Save attachment"), this,
		    SLOT(saveSelectedAttachmentsToFile()))->
		        setEnabled(indexes.size() == 1);
		menu->addAction(QIcon(ICON_3PARTY_PATH "save_16.png"),
		    tr("Save attachments"), this,
		    SLOT(saveSelectedAttachmentsIntoDirectory()))->
		        setEnabled(indexes.size() > 1);
	} else {
		/* Do nothing. */
	}
	menu->exec(QCursor::pos());
}

void DlgViewZfo::saveSelectedAttachmentsToFile(void)
{
	debugSlotCall();

	AttachmentInteraction::saveAttachmentsToFile(this,
	    *m_ui->attachmentTable);
}

void DlgViewZfo::saveSelectedAttachmentsIntoDirectory(void)
{
	debugSlotCall();

	AttachmentInteraction::saveAttachmentsToDirectory(this,
	    *m_ui->attachmentTable,
	    AttachmentInteraction::selectedColumnIndexes(*m_ui->attachmentTable,
	        DbFlsTblModel::FNAME_COL));
}

void DlgViewZfo::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	AttachmentInteraction::openAttachment(this, *m_ui->attachmentTable,
	    index);
}

void DlgViewZfo::showSignatureDetailsDialog(void)
{
	Q_ASSERT(NULL != m_message);
	Q_ASSERT(NULL != m_message->envelope);

	DlgSignatureDetail::detail(m_message->raw, m_message->raw_length,
	    m_message->envelope->timestamp,
	    m_message->envelope->timestamp_length, this);
}

void DlgViewZfo::parseZfoData(const QByteArray &zfoData)
{
	/* Logging purposes. */
	struct isds_ctx *dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		logError("%s\n", "Cannot create dummy ISDS session.");
		goto fail;
	}

	m_zfoType = Imports::IMPORT_MESSAGE;
	Q_ASSERT(NULL == m_message);
	m_message = loadZfoData(dummy_session, zfoData, m_zfoType);
	if (NULL == m_message) {
		m_zfoType = Imports::IMPORT_DELIVERY;
		m_message = loadZfoData(dummy_session, zfoData, m_zfoType);
		if (NULL == m_message) {
			logError("%s\n", "Cannot parse message data.");
			goto fail;
		}
	}

fail:
	if (NULL != dummy_session) {
		isds_ctx_free(&dummy_session);
	}
}

void DlgViewZfo::parseZfoFile(const QString &zfoFileName)
{
	/* Logging purposes. */
	struct isds_ctx *dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		logError("%s\n", "Cannot create dummy ISDS session.");
		goto fail;
	}

	m_zfoType = Imports::IMPORT_MESSAGE;
	Q_ASSERT(NULL == m_message);
	m_message = loadZfoFile(dummy_session, zfoFileName, m_zfoType);
	if (NULL == m_message) {
		m_zfoType = Imports::IMPORT_DELIVERY;
		m_message = loadZfoFile(dummy_session, zfoFileName, m_zfoType);
		if (NULL == m_message) {
			logError("Cannot parse file '%s'.\n",
			    zfoFileName.toUtf8().constData());
			goto fail;
		}
	}

fail:
	if (NULL != dummy_session) {
		isds_ctx_free(&dummy_session);
	}
}

void DlgViewZfo::setUpDialogue(void)
{
	/* TODO -- Adjust splitter sizes. */

	if (Imports::IMPORT_DELIVERY == m_zfoType) {
		m_ui->attachmentTable->hide();
		m_ui->envelopeTextEdit->setHtml(
		    deliveryDescriptionHtml(
		        m_message->raw, m_message->raw_length,
		        m_message->envelope->timestamp,
		        m_message->envelope->timestamp_length));
		m_ui->envelopeTextEdit->setReadOnly(true);

	} else {
		m_ui->attachmentTable->setEnabled(true);
		m_ui->attachmentTable->show();
		m_attachmentModel.setMessage(m_message);
		m_attachmentModel.setHeader();
		m_ui->envelopeTextEdit->setHtml(
		    messageDescriptionHtml(m_attachmentModel.rowCount(),
		        m_message->raw, m_message->raw_length,
		        m_message->envelope->timestamp,
		        m_message->envelope->timestamp_length));
		m_ui->envelopeTextEdit->setReadOnly(true);

		/* Attachment list. */
		m_ui->attachmentTable->setModel(&m_attachmentModel);
		/* First three columns contain hidden data. */
		m_ui->attachmentTable->setColumnHidden(
		    DbFlsTblModel::ATTACHID_COL, true);
		m_ui->attachmentTable->setColumnHidden(DbFlsTblModel::MSGID_COL,
		    true);
		m_ui->attachmentTable->setColumnHidden(
		    DbFlsTblModel::CONTENT_COL, true);
		m_ui->attachmentTable->setColumnHidden(DbFlsTblModel::MIME_COL,
		    true);
		m_ui->attachmentTable->setColumnHidden(DbFlsTblModel::FPATH_COL,
		    true);
		m_ui->attachmentTable->resizeColumnToContents(
		    DbFlsTblModel::FNAME_COL);

		m_ui->attachmentTable->setContextMenuPolicy(
		    Qt::CustomContextMenu);
		connect(m_ui->attachmentTable,
		    SIGNAL(customContextMenuRequested(QPoint)),
		    this, SLOT(attachmentItemRightClicked(QPoint)));
		connect(m_ui->attachmentTable,
		    SIGNAL(doubleClicked(QModelIndex)),
		    this, SLOT(openSelectedAttachment(QModelIndex)));

		m_ui->attachmentTable->installEventFilter(
		    new TableHomeEndFilter(this));
	}

	/* Signature details. */
	connect(m_ui->signaturePushButton, SIGNAL(clicked()), this,
	    SLOT(showSignatureDetailsDialog()));
}

QString DlgViewZfo::messageDescriptionHtml(int attachmentCount,
    const void *msgDER, size_t msgSize, const void *tstDER,
    size_t tstSize) const
{
	if (NULL == m_message) {
		Q_ASSERT(0);
		return QString();
	}

	const isds_envelope *envelope = m_message->envelope;
	if (NULL == envelope) {
		Q_ASSERT(0);
		return QString();
	}

	QString html(indentDivStart);

	envelopeHeaderDescriptionHtml(html, envelope);

	html += strongAccountInfoLine(tr("Attachments"),
	    QString::number(attachmentCount));

	signatureFooterDescription(html, msgDER, msgSize, tstDER, tstSize);

	html += divEnd;

	return html;
}

QString DlgViewZfo::deliveryDescriptionHtml(const void *msgDER,
    size_t msgSize, const void *tstDER, size_t tstSize) const
{
	if (NULL == m_message) {
		Q_ASSERT(0);
		return QString();
	}

	const isds_envelope *envelope = m_message->envelope;
	if (NULL == envelope) {
		Q_ASSERT(0);
		return QString();
	}

	QString html(indentDivStart);

	envelopeHeaderDescriptionHtml(html, envelope);

	html += strongAccountInfoLine(tr("Events"), QString());

	html += indentDivStart;
	const struct isds_list *event = envelope->events;
	while (NULL != event) {
		isds_event *item = (isds_event *) event->data;
		html += strongAccountInfoLine(
		    dateTimeStrFromDbFormat(timevalToDbFormat(item->time),
		        dateTimeDisplayFormat),
		    QString(item->description));
		event = event->next;
	}
	html += divEnd;

	signatureFooterDescription(html, msgDER, msgSize, tstDER, tstSize);

	html += divEnd;

	return html;
}

bool DlgViewZfo::envelopeHeaderDescriptionHtml(QString &html,
    const struct isds_envelope *envelope)
{
	if (NULL == envelope) {
		Q_ASSERT(0);
		return false;
	}

	html += "<h3>" + tr("Identification") + "</h3>";

	html += strongAccountInfoLine(tr("ID"), QString(envelope->dmID));
	html += strongAccountInfoLine(tr("Subject"), QString(envelope->dmAnnotation));
	html += strongAccountInfoLine(tr("Message type"), QString(envelope->dmType));

	html += "<br/>";

	/* Information about message author. */
	html += strongAccountInfoLine(tr("Sender"), QString(envelope->dmSender));
	html += strongAccountInfoLine(tr("Sender Databox ID"), QString(envelope->dbIDSender));
	html += strongAccountInfoLine(tr("Sender Address"),
	    QString(envelope->dmSenderAddress));

	html += "<br/>";

	html += strongAccountInfoLine(tr("Recipient"), QString(envelope->dmRecipient));
	html += strongAccountInfoLine(tr("Recipient Databox ID"), QString(envelope->dbIDRecipient));
	html += strongAccountInfoLine(tr("Recipient Address"),
	    QString(envelope->dmRecipientAddress));

	html += "<h3>" + tr("Status") + "</h3>";

	html += strongAccountInfoLine(tr("Delivery time"),
	    (NULL != envelope->dmDeliveryTime) ?
	        dateTimeStrFromDbFormat(
	            timevalToDbFormat(envelope->dmDeliveryTime),
	            dateTimeDisplayFormat) : "");
	html += strongAccountInfoLine(tr("Acceptance time"),
	    (NULL != envelope->dmAcceptanceTime) ?
	        dateTimeStrFromDbFormat(
	            timevalToDbFormat(envelope->dmAcceptanceTime),
	            dateTimeDisplayFormat) : "");

	QString statusString;
	if (NULL != envelope->dmMessageStatus) {
		statusString =
		    QString::number(IsdsConversion::msgStatusIsdsToDbRepr(*(envelope->dmMessageStatus))) +
		    " -- " +
		    IsdsConversion::msgStatusDbToText(
		        IsdsConversion::msgStatusIsdsToDbRepr(*(envelope->dmMessageStatus)));
	}
	html += strongAccountInfoLine(tr("Status"), statusString);

	return true;
}

bool DlgViewZfo::signatureFooterDescription(QString &html,
    const void *msgDER, size_t msgSize, const void *tstDER, size_t tstSize)
{
	html += "<h3>" + tr("Signature") + "</h3>";

	QString resultStr;
	if (1 == raw_msg_verify_signature(msgDER, msgSize, 0, 0)) {
		resultStr = QObject::tr("Valid");
	} else {
		resultStr = QObject::tr("Invalid")  + " -- " +
		    QObject::tr("Message signature and content do not "
		        "correspond!");
	}
	html += strongAccountInfoLine(tr("Message signature"), resultStr);
	if (1 == raw_msg_verify_signature_date(msgDER, msgSize,
	        QDateTime::currentDateTime().toTime_t(), 0)) {
		resultStr = QObject::tr("Valid");
	} else {
		resultStr = QObject::tr("Invalid");
	}
	if (!globPref.check_crl) {
		resultStr += " (" +
		    QObject::tr("Certificate revocation check is turned off!") +
		    ")";
	}
	html += strongAccountInfoLine(tr("Signing certificate"), resultStr);
	time_t utc_time = 0;
	QDateTime tst;
	int ret = raw_tst_verify(tstDER, tstSize, &utc_time);
	if (-1 != ret) {
		tst = QDateTime::fromTime_t(utc_time);
	}
	resultStr = (1 == ret) ? QObject::tr("Valid") : QObject::tr("Invalid");
	if (-1 != ret) {
		resultStr += " (" + tst.toString("dd.MM.yyyy hh:mm:ss") + " " +
		    tst.timeZone().abbreviation(tst) + ")";
	}
	html += strongAccountInfoLine(tr("Time stamp"), resultStr);

	return true;
}
