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

#include <QDateTime>
#include <QDir>
#include <QMenu>
#include <QTimeZone>

#include "src/crypto/crypto_funcs.h"
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/io/dbs.h"
#include "src/io/filesystem.h"
#include "src/isds/type_description.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/settings/preferences.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_tab_ignore_filter.h"
#include "ui_dlg_view_zfo.h"

DlgViewZfo::DlgViewZfo(const Isds::Message &message, enum Isds::LoadType zfoType,
    const QString &errMsg, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgViewZfo),
    m_message(message),
    m_zfoType(zfoType),
    m_attachmentModel(this)
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	/* Set default line height for table views/widgets. */
	m_ui->attachmentTable->setNarrowedLineHeight();

	if (m_message.isNull()) {
		/* Just show error message. */
		m_ui->attachmentTable->hide();
		m_ui->envelopeTextEdit->setHtml(
		    "<h3>" + tr("Error parsing content") + "</h3><br/>" +
		    errMsg);
		m_ui->envelopeTextEdit->setReadOnly(true);
		m_ui->signatureDetailsButton->setEnabled(false);
		return;
	}

	setUpDialogue();
}

DlgViewZfo::~DlgViewZfo(void)
{
	delete m_ui;
}

void DlgViewZfo::view(const QString &zfoFileName, QWidget *parent)
{
	if (Q_UNLIKELY(zfoFileName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	Isds::Message message;
	enum Isds::LoadType zfoType;

	/* Load message ZFO. */
	parseZfoFile(zfoFileName, message, zfoType);

	DlgViewZfo dlg(message, zfoType,
	    tr("Cannot parse the content of file '%1'.")
	        .arg(QDir::toNativeSeparators(zfoFileName)),
	    parent);
	dlg.exec();
}

void DlgViewZfo::view(const QByteArray &zfoData, QWidget *parent)
{
	Isds::Message message;
	enum Isds::LoadType zfoType;

	/* Load raw message. */
	parseZfoData(zfoData, message, zfoType);

	DlgViewZfo dlg(message, zfoType,
	    tr("Cannot parse the content of message."), parent);
	dlg.exec();
}

void DlgViewZfo::attachmentItemRightClicked(const QPoint &point)
{
	QModelIndex index(m_ui->attachmentTable->indexAt(point));
	QMenu *menu = new (std::nothrow) QMenu(this);
	if (Q_UNLIKELY(Q_NULLPTR == menu)) {
		Q_ASSERT(0);
		return;
	}

	/* Detects selection of multiple attachments. */
	QModelIndexList indexes(
	    AttachmentInteraction::selectedColumnIndexes(*m_ui->attachmentTable,
	        AttachmentTblModel::FNAME_COL));

	if (index.isValid()) {
		{
			QIcon ico;
			ico.addFile(QStringLiteral(ICON_3PARTY_PATH "folder_16.png"), QSize(), QIcon::Normal, QIcon::Off);
			ico.addFile(QStringLiteral(ICON_3PARTY_PATH "folder_32.png"), QSize(), QIcon::Normal, QIcon::Off);
			menu->addAction(ico, tr("Open attachment"), this,
			    SLOT(openSelectedAttachment()))->
			        setEnabled(indexes.size() == 1);
		}
		{
			QIcon ico;
			ico.addFile(QStringLiteral(ICON_3PARTY_PATH "save_16.png"), QSize(), QIcon::Normal, QIcon::Off);
			ico.addFile(QStringLiteral(ICON_3PARTY_PATH "save_32.png"), QSize(), QIcon::Normal, QIcon::Off);
			menu->addAction(ico, tr("Save attachment"), this,
			    SLOT(saveSelectedAttachmentsToFile()))->
			        setEnabled(indexes.size() == 1);
			menu->addAction(ico, tr("Save attachments"), this,
			    SLOT(saveSelectedAttachmentsIntoDirectory()))->
			        setEnabled(indexes.size() > 1);
		}
	} else {
		/* Do nothing. */
	}
	menu->exec(QCursor::pos());
	menu->deleteLater();
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
	        AttachmentTblModel::FNAME_COL));
}

void DlgViewZfo::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	AttachmentInteraction::openAttachment(this, *m_ui->attachmentTable,
	    index);
}

void DlgViewZfo::showSignatureDetailsDlg(void)
{
	Q_ASSERT(!m_message.isNull());
	Q_ASSERT(!m_message.envelope().isNull());

	DlgSignatureDetail::detail(m_message.raw(),
	    m_message.envelope().dmQTimestamp(), this);
}

bool DlgViewZfo::parseZfoData(const QByteArray &zfoData,
    Isds::Message &message, enum Isds::LoadType &zfoType)
{
	bool success = false;

	zfoType = Isds::LT_MESSAGE;
	message = Isds::messageFromData(zfoData, zfoType);
	if (message.isNull()) {
		zfoType = Isds::LT_DELIVERY;
		message = Isds::messageFromData(zfoData, zfoType);
	}
	if (!message.isNull()) {
		success = true;
	} else {
		logError("%s\n", "Cannot parse message data.");
	}

	return success;
}

bool DlgViewZfo::parseZfoFile(const QString &zfoFileName,
    Isds::Message &message, enum Isds::LoadType &zfoType)
{
	QFile file(zfoFileName);

	if (!file.open(QIODevice::ReadOnly)) {
		logErrorNL("Cannot open file '%s'.",
		    zfoFileName.toUtf8().constData());
		return false;
	}

	QByteArray zfoContent(file.readAll());
	file.close();

	return parseZfoData(zfoContent, message, zfoType);
}

void DlgViewZfo::setUpDialogue(void)
{
	/* TODO -- Adjust splitter sizes. */

	if (Isds::LT_DELIVERY == m_zfoType) {
		m_ui->attachmentTable->hide();
		m_ui->envelopeTextEdit->setHtml(
		    deliveryDescriptionHtml(m_message.raw(),
		        m_message.envelope().dmQTimestamp()));
		m_ui->envelopeTextEdit->setReadOnly(true);

	} else {
		m_ui->attachmentTable->setEnabled(true);
		m_ui->attachmentTable->show();
		m_attachmentModel.setMessage(m_message);
		m_attachmentModel.setHeader();
		m_ui->envelopeTextEdit->setHtml(
		    messageDescriptionHtml(m_attachmentModel.rowCount(),
		        m_message.raw(), m_message.envelope().dmQTimestamp()));
		m_ui->envelopeTextEdit->setReadOnly(true);

		/* Attachment list. */
		m_ui->attachmentTable->setModel(&m_attachmentModel);
		/* First three columns contain hidden data. */
		m_ui->attachmentTable->setColumnHidden(
		    AttachmentTblModel::ATTACHID_COL, true);
		m_ui->attachmentTable->setColumnHidden(
		    AttachmentTblModel::MSGID_COL, true);
		m_ui->attachmentTable->setColumnHidden(
		    AttachmentTblModel::BINARY_CONTENT_COL, true);
		m_ui->attachmentTable->setColumnHidden(
		    AttachmentTblModel::MIME_COL, true);
		m_ui->attachmentTable->setColumnHidden(
		    AttachmentTblModel::FPATH_COL, true);
		m_ui->attachmentTable->resizeColumnToContents(
		    AttachmentTblModel::FNAME_COL);

		m_ui->attachmentTable->setContextMenuPolicy(
		    Qt::CustomContextMenu);
		connect(m_ui->attachmentTable,
		    SIGNAL(customContextMenuRequested(QPoint)),
		    this, SLOT(attachmentItemRightClicked(QPoint)));
		connect(m_ui->attachmentTable,
		    SIGNAL(doubleClicked(QModelIndex)),
		    this, SLOT(openSelectedAttachment(QModelIndex)));

		m_ui->attachmentTable->installEventFilter(
		    new TableHomeEndFilter(m_ui->attachmentTable));
		m_ui->attachmentTable->installEventFilter(
		    new TableTabIgnoreFilter(m_ui->attachmentTable));
	}

	/* Signature details. */
	QIcon ico;
	ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
	ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
	ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
	ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
	ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
	m_ui->signatureDetailsButton->setIcon(ico);
	connect(m_ui->signatureDetailsButton, SIGNAL(clicked()), this,
	    SLOT(showSignatureDetailsDlg()));
}

QString DlgViewZfo::messageDescriptionHtml(int attachmentCount,
    const QByteArray &msgDER, const QByteArray &tstDER) const
{
	if (Q_UNLIKELY(m_message.isNull())) {
		Q_ASSERT(0);
		return QString();
	}

	const Isds::Envelope &envelope(m_message.envelope());
	if (Q_UNLIKELY(envelope.isNull())) {
		Q_ASSERT(0);
		return QString();
	}

	QString html(indentDivStart);

	envelopeHeaderDescriptionHtml(html, envelope);

	html += strongAccountInfoLine(tr("Attachments"),
	    QString::number(attachmentCount));

	signatureFooterDescription(html, msgDER, tstDER);

	html += divEnd;

	return html;
}

QString DlgViewZfo::deliveryDescriptionHtml(const QByteArray &msgDER,
    const QByteArray &tstDER) const
{
	if (Q_UNLIKELY(m_message.isNull())) {
		Q_ASSERT(0);
		return QString();
	}

	const Isds::Envelope &envelope(m_message.envelope());
	if (Q_UNLIKELY(envelope.isNull())) {
		Q_ASSERT(0);
		return QString();
	}

	QString html(indentDivStart);

	envelopeHeaderDescriptionHtml(html, envelope);

	html += strongAccountInfoLine(tr("Events"), QString());

	html += indentDivStart;
	foreach (const Isds::Event &event, envelope.dmEvents()) {
		html += strongAccountInfoLine(
		    dateTimeStrFromDbFormat(qDateTimeToDbFormat(event.time()),
		        dateTimeDisplayFormat),
		    event.descr());
	}
	html += divEnd;

	signatureFooterDescription(html, msgDER, tstDER);

	html += divEnd;

	return html;
}

bool DlgViewZfo::envelopeHeaderDescriptionHtml(QString &html,
    const Isds::Envelope &envelope)
{
	if (Q_UNLIKELY(envelope.isNull())) {
		Q_ASSERT(0);
		return false;
	}

	html += "<h3>" + tr("Identification") + "</h3>";

	html += strongAccountInfoLine(tr("ID"), envelope.dmID());
	html += strongAccountInfoLine(tr("Subject"), envelope.dmAnnotation());
	html += strongAccountInfoLine(tr("Message type"), QString(envelope.dmType()));

	html += "<br/>";

	/* Information about message author. */
	html += strongAccountInfoLine(tr("Sender"), envelope.dmSender());
	html += strongAccountInfoLine(tr("Sender Databox ID"), envelope.dbIDSender());
	html += strongAccountInfoLine(tr("Sender Address"), envelope.dmSenderAddress());

	html += "<br/>";

	html += strongAccountInfoLine(tr("Recipient"), envelope.dmRecipient());
	html += strongAccountInfoLine(tr("Recipient Databox ID"), envelope.dbIDRecipient());
	html += strongAccountInfoLine(tr("Recipient Address"), envelope.dmRecipientAddress());

	html += "<h3>" + tr("Status") + "</h3>";

	html += strongAccountInfoLine(tr("Delivery time"),
	    (!envelope.dmDeliveryTime().isNull()) ?
	        dateTimeStrFromDbFormat(
	            qDateTimeToDbFormat(envelope.dmDeliveryTime()),
	            dateTimeDisplayFormat) : "");
	html += strongAccountInfoLine(tr("Acceptance time"),
	    (!envelope.dmAcceptanceTime().isNull()) ?
	        dateTimeStrFromDbFormat(
	            qDateTimeToDbFormat(envelope.dmAcceptanceTime()),
	            dateTimeDisplayFormat) : "");

	QString statusString;
	if (Isds::Type::MS_NULL != envelope.dmMessageStatus()) {
		statusString =
		    QString::number(Isds::dmState2Variant(envelope.dmMessageStatus()).toInt()) +
		    " -- " +
		    Isds::Description::descrDmState(envelope.dmMessageStatus());
	}
	html += strongAccountInfoLine(tr("Status"), statusString);

	return true;
}

bool DlgViewZfo::signatureFooterDescription(QString &html,
    const QByteArray &msgDER, const QByteArray &tstDER)
{
	html += "<h3>" + tr("Signature") + "</h3>";

	QString resultStr;
	if (1 == raw_msg_verify_signature(msgDER.constData(), msgDER.size(), 0, 0)) {
		resultStr = tr("Valid");
	} else {
		resultStr = tr("Invalid")  + " -- " +
		    tr("Message signature and content do not correspond!");
	}
	html += strongAccountInfoLine(tr("Message signature"), resultStr);
	if (1 == raw_msg_verify_signature_date(msgDER.constData(), msgDER.size(),
	        QDateTime::currentDateTime().toTime_t(), 0)) {
		resultStr = tr("Valid");
	} else {
		resultStr = tr("Invalid");
	}
	if (!GlobInstcs::prefsPtr->checkCrl) {
		resultStr += " (" +
		    tr("Certificate revocation check is turned off!") +
		    ")";
	}
	html += strongAccountInfoLine(tr("Signing certificate"), resultStr);
	time_t utc_time = 0;
	QDateTime tst;
	int ret = raw_tst_verify(tstDER.constData(), tstDER.size(), &utc_time);
	if (-1 != ret) {
		tst = QDateTime::fromTime_t(utc_time);
	}
	resultStr = (1 == ret) ? tr("Valid") : tr("Invalid");
	if (-1 != ret) {
		resultStr += " (" + tst.toString("dd.MM.yyyy hh:mm:ss") + " " +
		    tst.timeZone().abbreviation(tst) + ")";
	}
	html += strongAccountInfoLine(tr("Time stamp"), resultStr);

	return true;
}
