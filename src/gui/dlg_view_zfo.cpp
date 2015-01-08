

#include <QAbstractTableModel>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QTimeZone>
#include <QUrl>

#include "src/common.h"
#include "src/crypto/crypto_threadsafe.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/io/dbs.h"


#define COL_NUM 2
#define FNAME_COL 0
#define FSIZE_COL 1


const QVector<QString> AttachmentModel::m_headerLabels = {
QObject::tr("File name"),
QObject::tr("Size")
};


/* ========================================================================= */
/*
 * Constructor.
 */
AttachmentModel::AttachmentModel(QObject *parent)
/* ========================================================================= */
    : QAbstractTableModel(parent),
    m_docs()
{
}


/* ========================================================================= */
/*
 * Destructor.
 */
AttachmentModel::~AttachmentModel(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Returns row count.
 */
int AttachmentModel::rowCount(const QModelIndex &parent) const
/* ========================================================================= */
{
	/* unused */
	(void) parent;

	return m_docs.size();
}


/* ========================================================================= */
/*
 * Returns column count.
 */
int AttachmentModel::columnCount(const QModelIndex &parent) const
/* ========================================================================= */
{
	/* unused */
	(void) parent;

	return COL_NUM;
}


/* ========================================================================= */
/*
 * Returns data.
 */
QVariant AttachmentModel::data(const QModelIndex &index, int role) const
/* ========================================================================= */
{
	int row, col;

	switch (role) {
	case Qt::DisplayRole:
		row = index.row();
		col = index.column();
		Q_ASSERT(row < m_docs.size());
		Q_ASSERT(col < COL_NUM);

		if (FNAME_COL == col) {
			/* File name. */
			return QString(m_docs[row]->dmFileDescr);
		} else if (FSIZE_COL == col) {
			/* File size. */
			return QString::number(m_docs[row]->data_length);
		}

		return QVariant();
		break;
	case Qt::TextAlignmentRole:
		return Qt::AlignLeft;
		break;
	default:
		return QVariant();
		break;
	}
}


/* ========================================================================= */
/*
 * Returns header data.
 */
QVariant AttachmentModel::headerData(int section,
    Qt::Orientation orientation, int role) const
/* ========================================================================= */
{
	(void) orientation; /* Unused. */

	if (role != Qt::DisplayRole) {
		return QVariant();
	}
	return m_headerLabels[section];
}


/* ========================================================================= */
/*
 * Set attachment model according to message content.
 */
bool AttachmentModel::setModelData(const isds_message *message)
/* ========================================================================= */
{
	const struct isds_list *docListItem;

	Q_ASSERT(NULL != message);
	if (NULL == message) {
		return false;
	}

	docListItem = message->documents;
	Q_ASSERT(NULL != docListItem);
	if (NULL == docListItem) {
		return false;
	}

	this->beginResetModel();

	m_docs.clear();

	while (NULL != docListItem) {
		Q_ASSERT(NULL != docListItem->data);
		m_docs.append((struct isds_document *) docListItem->data);
		docListItem = docListItem->next;
	}

	this->endResetModel();

	return true;
}


/* ========================================================================= */
/*
 * Get attachment content.
 */
QByteArray AttachmentModel::attachmentData(int indexRow) const
/* ========================================================================= */
{
	Q_ASSERT(m_docs.size() > 0);

	QByteArray data((char *) m_docs[indexRow]->data,
	    (int) m_docs[indexRow]->data_length);

	return data;
}


/* ========================================================================= */
/*
 * Constructor.
 */
DlgViewZfo::DlgViewZfo(const isds_message *isdsMsg, QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_message(isdsMsg),
    m_attachmentModel(0)
{
	setupUi(this);

	/* TODO -- Adjust splitter sizes. */

	Q_ASSERT(NULL != m_message);

	m_attachmentModel.setModelData(isdsMsg);

	envelopeTextEdit->setHtml(
	    descriptionHtml(m_attachmentModel.rowCount(),
	        m_message->raw, m_message->raw_length,
	        m_message->envelope->timestamp,
	        m_message->envelope->timestamp_length));
	envelopeTextEdit->setReadOnly(true);

	/* Attachment list. */
	attachmentTable->setModel(&m_attachmentModel);
	attachmentTable->resizeColumnToContents(0);

	attachmentTable->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(attachmentTable, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(attachmentItemRightClicked(QPoint)));
	connect(attachmentTable, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(attachmentItemDoubleClicked(QModelIndex)));

	/* Signature details. */
	connect(signaturePushButton, SIGNAL(clicked()), this,
	    SLOT(showSignatureDetails()));
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
 * Saves selected attachment to file.
 */
void DlgViewZfo::saveSelectedAttachmentToFile(void)
/* ========================================================================= */
{
	QModelIndex selectedIndex = selectedAttachmentIndex();

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return;
	}

	QString fileName = selectedIndex.data().toString();
	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
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

	Q_ASSERT(!selectedIndexes.isEmpty());
	if (selectedIndexes.isEmpty()) {
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
		Q_ASSERT(idx.isValid());
		if (!idx.isValid()) {
			unspecifiedFailed = true;
			continue;
		}

		QString fileName = idx.data().toString();
		Q_ASSERT(!fileName.isEmpty());
		if (fileName.isEmpty()) {
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

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return;
	}

	QString attachName = selectedIndex.data().toString();
	Q_ASSERT(!attachName.isEmpty());
	if (attachName.isEmpty()) {
		return;
	}
	/* TODO -- Add message id into file name? */
	QString fileName = TMP_ATTACHMENT_PREFIX + fileName;

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
QString DlgViewZfo::descriptionHtml(int attachmentCount,
    const void *msgDER, size_t msgSize, const void *tstDER, size_t tstSize)
/* ========================================================================= */
{
	const isds_envelope *envelope;
	isds_message_status *state;

	envelope = m_message->envelope;
	Q_ASSERT(NULL != envelope);

	state = envelope->dmMessageStatus;
	Q_ASSERT(NULL != state);

	QString html;

	html += indentDivStart;
	html += "<h3>" + tr("Identification") + "</h3>";

	html += strongAccountInfoLine(tr("ID"), envelope->dmID);
	html += strongAccountInfoLine(tr("Subject"), envelope->dmAnnotation);
	html += strongAccountInfoLine(tr("Message type"), envelope->dmType);

	html += "<br/>";

	/* Information about message author. */
	html += strongAccountInfoLine("From", envelope->dmSender);
	html += strongAccountInfoLine("Sender Address",
	    envelope->dmSenderAddress);

	html += "<br/>";

	html += strongAccountInfoLine(tr("To"), envelope->dmRecipient);
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
	html += strongAccountInfoLine(tr("Status"),
	    QString::number(convertHexToDecIndex(
	        *(envelope->dmMessageStatus))) + " -- " +
	    msgStatusToText(convertHexToDecIndex(
	        *(envelope->dmMessageStatus))));
	html += strongAccountInfoLine(tr("Attachments"),
	    QString::number(attachmentCount));

	html += "<h3>" + tr("Signature") + "</h3>";

	QString resultStr;
	if (1 == rawMsgVerifySignature(msgDER, msgSize, 0, 0)) {
		resultStr = QObject::tr("Valid");
	} else {
		resultStr = QObject::tr("Invalid")  + " -- " +
		    QObject::tr("Message signature and content do not "
		        "correspond!");
	}
	html += strongAccountInfoLine(tr("Message signature"), resultStr);
	if (1 == rawMsgVerifySignatureDate(msgDER, msgSize,
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
	int ret = rawTstVerify(tstDER, tstSize, &utc_time);
	if (-1 != ret) {
		tst = QDateTime::fromTime_t(utc_time);
	}
	resultStr = (1 == ret) ? QObject::tr("Valid") : QObject::tr("Invalid");
	if (-1 != ret) {
		resultStr += " (" + tst.toString("dd.MM.yyyy hh:mm:ss") + " " +
		    tst.timeZone().abbreviation(tst) + ")";
	}
	html += strongAccountInfoLine(tr("Time stamp"), resultStr);

	return html;
}


/* ========================================================================= */
/*
 * Returns selected attachment index.
 */
QModelIndex DlgViewZfo::selectedAttachmentIndex(void) const
/* ========================================================================= */
{
	Q_ASSERT(0 != attachmentTable->selectionModel());
	if (0 == attachmentTable->selectionModel()) {
		return QModelIndex();
	}

	QModelIndex selectedIndex =
	    attachmentTable->selectionModel()->currentIndex();

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return QModelIndex();
	}

	return selectedIndex.sibling(selectedIndex.row(), FNAME_COL);
}


/* ========================================================================= */
/*
 * Returns all selected indexes.
 */
QModelIndexList DlgViewZfo::selectedAttachmentIndexes(void) const
/* ========================================================================= */
{
	Q_ASSERT(0 != attachmentTable->selectionModel());
	if (0 == attachmentTable->selectionModel()) {
		return QModelIndexList();
	}

	return attachmentTable->selectionModel()->selectedRows(0);
}
