/*
 * Copyright (C) 2014-2015 CZ.NIC
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
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimeZone>
#include <QUrl>

#include "src/common.h"
#include "src/crypto/crypto_funcs.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/views/table_home_end_filter.h"


/* ========================================================================= */
/*
 * Constructor.
 */
DlgViewZfo::DlgViewZfo(const QString &zfoFileName, QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_message(NULL),
    m_attachmentModel(0)
{
	setupUi(this);

	/* TODO -- Load message ZFO. */
	parseZfoFile(zfoFileName);

	if(NULL == m_message) {
		/* Just show error message. */
		this->attachmentTable->hide();
		envelopeTextEdit->setHtml(
		    "<h3>" + tr("Error parsing content") + "</h3><br/>" +
		    tr("Cannot parse the content of file '%1'.")
		        .arg(zfoFileName));
		envelopeTextEdit->setReadOnly(true);
		signaturePushButton->setEnabled(false);
		return;
	}

	/* TODO -- Adjust splitter sizes. */

	if (ImportZFODialog::IMPORT_DELIVERY_ZFO == m_zfoType) {
		this->attachmentTable->hide();
		envelopeTextEdit->setHtml(
		    deliveryDescriptionHtml(
		        m_message->raw, m_message->raw_length,
		        m_message->envelope->timestamp,
		        m_message->envelope->timestamp_length));
		envelopeTextEdit->setReadOnly(true);

	} else {
		this->attachmentTable->setEnabled(true);
		this->attachmentTable->show();
		m_attachmentModel.setModelData(m_message);
		envelopeTextEdit->setHtml(
		    messageDescriptionHtml(m_attachmentModel.rowCount(),
		        m_message->raw, m_message->raw_length,
		        m_message->envelope->timestamp,
		        m_message->envelope->timestamp_length));
		envelopeTextEdit->setReadOnly(true);

		/* Attachment list. */
		attachmentTable->setModel(&m_attachmentModel);
		/* First three columns contain hidden data. */
		attachmentTable->setColumnHidden(AttachmentModel::ATTACHID_COL,
		    true);
		attachmentTable->setColumnHidden(AttachmentModel::MSGID_COL,
		    true);
		attachmentTable->setColumnHidden(AttachmentModel::CONTENT_COL,
		    true);
		attachmentTable->resizeColumnToContents(
		    AttachmentModel::FNAME_COL);

		attachmentTable->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(attachmentTable, SIGNAL(customContextMenuRequested(QPoint)),
		    this, SLOT(attachmentItemRightClicked(QPoint)));
		connect(attachmentTable, SIGNAL(doubleClicked(QModelIndex)),
		    this, SLOT(attachmentItemDoubleClicked(QModelIndex)));

		attachmentTable->installEventFilter(new TableHomeEndFilter(this));
	}

	/* Signature details. */
	connect(signaturePushButton, SIGNAL(clicked()), this,
	    SLOT(showSignatureDetails()));
}


/* ========================================================================= */
/*
 * Destructor.
 */
DlgViewZfo::~DlgViewZfo(void)
/* ========================================================================= */
{
	if (NULL != m_message) {
		isds_message_free(&m_message);
	}
}


/* ========================================================================= */
/*
 * Generates menu to selected message item.
 */
void DlgViewZfo::attachmentItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QModelIndex index = attachmentTable->indexAt(point);
	QMenu *menu = new QMenu;

	/* Detects selection of multiple attachments. */
	QModelIndexList indexes = selectedAttachmentIndexes();

	if (index.isValid()) {
		//attachmentItemSelectionChanged(index);

		menu->addAction(QIcon(ICON_3PARTY_PATH "folder_16.png"),
		    tr("Open attachment"), this,
		    SLOT(openSelectedAttachment()))->
		        setEnabled(indexes.size() == 1);
		menu->addAction(QIcon(ICON_3PARTY_PATH "save_16.png"),
		    tr("Save attachment"), this,
		    SLOT(saveSelectedAttachmentToFile()))->
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


/* ========================================================================= */
/*
 * Handle attachment double click.
 */
void DlgViewZfo::attachmentItemDoubleClicked(const QModelIndex &index)
/* ========================================================================= */
{
	(void) index;

	//qDebug() << "Attachment double clicked.";

	openSelectedAttachment();
}


/* ========================================================================= */
/*
 * Loads ZFO file.
 */
void DlgViewZfo::parseZfoFile(const QString &zfoFileName)
/* ========================================================================= */
{
	/* Logging purposes. */
	struct isds_ctx *dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		logError("%s\n", "Cannot create dummy ISDS session.");
		goto fail;
	}

	m_zfoType = ImportZFODialog::IMPORT_MESSAGE_ZFO;
	Q_ASSERT(NULL == m_message);
	m_message = loadZfoFile(dummy_session, zfoFileName, m_zfoType);
	if (NULL == m_message) {
		m_zfoType = ImportZFODialog::IMPORT_DELIVERY_ZFO;
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


/* ========================================================================= */
/*
 * Saves selected attachment to file.
 */
void DlgViewZfo::saveSelectedAttachmentToFile(void)
/* ========================================================================= */
{
	QModelIndex selectedIndex = selectedAttachmentIndex();

	if (!selectedIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	QString fileName = selectedIndex.data().toString();
	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}
	/* TODO -- Remember directory? */
	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save attachment"), fileName);

	if (fileName.isEmpty()) {
		return;
	}

	QByteArray data =
	    m_attachmentModel.attachmentData(selectedIndex.row());

	if (WF_SUCCESS != writeFile(fileName, data)) {
		QMessageBox::warning(this,
		    tr("Error saving attachment."),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Saves selected attachments to directory.
 */
void DlgViewZfo::saveSelectedAttachmentsIntoDirectory(void)
/* ========================================================================= */
{
	QModelIndexList selectedIndexes = selectedAttachmentIndexes();

	if (selectedIndexes.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Save attachments"), NULL,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (newDir.isNull() || newDir.isEmpty()) {
		return;
	}
	/* TODO -- Remember directory? */

	bool unspecifiedFailed = false;
	QList<QString> unsuccessfullFiles;

	foreach (QModelIndex idx, selectedIndexes) {
		if (!idx.isValid()) {
			Q_ASSERT(0);
			unspecifiedFailed = true;
			continue;
		}

		QString fileName = idx.data().toString();
		if (fileName.isEmpty()) {
			Q_ASSERT(0);
			unspecifiedFailed = true;
			continue;
		}

		fileName = newDir + QDir::separator() + fileName;

		QByteArray data = m_attachmentModel.attachmentData(idx.row());

		if (WF_SUCCESS != writeFile(fileName, data)) {
			unsuccessfullFiles.append(fileName);
			continue;
		}
	}

	if (unspecifiedFailed) {
		QMessageBox::warning(this,
		    tr("Error saving attachments."),
		    tr("Could not save all attachments."),
		    QMessageBox::Ok);
	} else if (!unsuccessfullFiles.isEmpty()) {
		QString warnMsg =
		    tr("In total %1 attachment files could not be written.")
		        .arg(unsuccessfullFiles.size());
		warnMsg += "\n" +
		    tr("These are:").arg(unsuccessfullFiles.size()) + "\n";
		int i;
		for (i = 0; i < (unsuccessfullFiles.size() - 1); ++i) {
			warnMsg += "    '" + unsuccessfullFiles.at(i) + "'\n";
		}
		warnMsg += "    '" + unsuccessfullFiles.at(i) + "'.";
		QMessageBox::warning(this, tr("Error saving attachments."),
		    warnMsg, QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Open attachment in default application.
 */
void DlgViewZfo::openSelectedAttachment(void)
/* ========================================================================= */
{
	QModelIndex selectedIndex = selectedAttachmentIndex();

	if (!selectedIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	QString attachName = selectedIndex.data().toString();
	if (attachName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}
	attachName.replace(QRegExp("\\s"), "_").replace(
	    QRegExp("[^a-zA-Z\\d\\.\\-_]"), "x");
	/* TODO -- Add message id into file name? */
	QString fileName = TMP_ATTACHMENT_PREFIX + attachName;

	QByteArray data =
	    m_attachmentModel.attachmentData(selectedIndex.row());

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		QDesktopServices::openUrl(QUrl("file:///" + fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		QMessageBox::warning(this,
		    tr("Error opening attachment."),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * View signature details.
 */
void DlgViewZfo::showSignatureDetails(void)
/* ========================================================================= */
{
	Q_ASSERT(NULL != m_message);
	Q_ASSERT(NULL != m_message->envelope);

	QDialog *signature_detail = new DlgSignatureDetail(
	    m_message->raw, m_message->raw_length,
	    m_message->envelope->timestamp,
	    m_message->envelope->timestamp_length, this);
	signature_detail->exec();
}


/* ========================================================================= */
/*
 * Generate description from supplied message.
 */
QString DlgViewZfo::messageDescriptionHtml(int attachmentCount,
    const void *msgDER, size_t msgSize, const void *tstDER,
    size_t tstSize) const
/* ========================================================================= */
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



/* ========================================================================= */
/*
 * Generate description from supplied delivery info.
 */
QString DlgViewZfo::deliveryDescriptionHtml(const void *msgDER,
    size_t msgSize, const void *tstDER, size_t tstSize) const
/* ========================================================================= */
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

	html += strongAccountInfoLine(tr("Events"),"");

	html += indentDivStart;
	const struct isds_list *event = envelope->events;
	while (NULL != event) {
		isds_event *item = (isds_event *) event->data;
		html += strongAccountInfoLine(
		        dateTimeStrFromDbFormat(timevalToDbFormat(item->time),
		        dateTimeDisplayFormat),
		        item->description);
		event = event->next;
	}
	html += divEnd;

	signatureFooterDescription(html, msgDER, msgSize, tstDER, tstSize);

	html += divEnd;

	return html;
}

/* ========================================================================= */
bool DlgViewZfo::envelopeHeaderDescriptionHtml(QString &html,
    const isds_envelope *envelope)
/* ========================================================================= */
{
	if (NULL == envelope) {
		Q_ASSERT(0);
		return false;
	}

	html += "<h3>" + tr("Identification") + "</h3>";

	html += strongAccountInfoLine(tr("ID"), envelope->dmID);
	html += strongAccountInfoLine(tr("Subject"), envelope->dmAnnotation);
	html += strongAccountInfoLine(tr("Message type"), envelope->dmType);

	html += "<br/>";

	/* Information about message author. */
	html += strongAccountInfoLine(tr("Sender"), envelope->dmSender);
	html += strongAccountInfoLine(tr("Sender Databox ID"), envelope->dbIDSender);
	html += strongAccountInfoLine(tr("Sender Address"),
	    envelope->dmSenderAddress);

	html += "<br/>";

	html += strongAccountInfoLine(tr("Recipient"), envelope->dmRecipient);
	html += strongAccountInfoLine(tr("Recipient Databox ID"), envelope->dbIDRecipient);
	html += strongAccountInfoLine(tr("Recipient Address"),
	    envelope->dmRecipientAddress);

	html += "<h3>" + tr("Status") + "</h3>";

	html += strongAccountInfoLine(tr("Delivery time"),
	    dateTimeStrFromDbFormat(
	        timevalToDbFormat(envelope->dmDeliveryTime),
	        dateTimeDisplayFormat));
	html += strongAccountInfoLine(tr("Acceptance time"),
	    dateTimeStrFromDbFormat(
	        timevalToDbFormat(envelope->dmAcceptanceTime),
	        dateTimeDisplayFormat));

	QString statusString;
	if (NULL != envelope->dmMessageStatus) {
		statusString =
		    QString::number(convertHexToDecIndex(*(envelope->dmMessageStatus))) +
		    " -- " +
		    msgStatusToText(convertHexToDecIndex(*(envelope->dmMessageStatus)));
	}
	html += strongAccountInfoLine(tr("Status"), statusString);

	return true;
}

/* ========================================================================= */
bool DlgViewZfo::signatureFooterDescription(QString &html,
    const void *msgDER, size_t msgSize, const void *tstDER, size_t tstSize)
/* ========================================================================= */
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

/* ========================================================================= */
/*
 * Returns selected attachment index.
 */
QModelIndex DlgViewZfo::selectedAttachmentIndex(void) const
/* ========================================================================= */
{
	if (0 == attachmentTable->selectionModel()) {
		Q_ASSERT(0);
		return QModelIndex();
	}

	QModelIndex selectedIndex =
	    attachmentTable->selectionModel()->currentIndex();

	if (!selectedIndex.isValid()) {
		Q_ASSERT(0);
		return QModelIndex();
	}

	return selectedIndex.sibling(selectedIndex.row(),
	    AttachmentModel::FNAME_COL);
}


/* ========================================================================= */
/*
 * Returns all selected indexes.
 */
QModelIndexList DlgViewZfo::selectedAttachmentIndexes(void) const
/* ========================================================================= */
{
	if (0 == attachmentTable->selectionModel()) {
		Q_ASSERT(0);
		return QModelIndexList();
	}

	return attachmentTable->selectionModel()->selectedRows(0);
}
