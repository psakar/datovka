

#include <QAbstractTableModel>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QTemporaryFile>
#include <QUrl>

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
	return m_docs.size();
}


/* ========================================================================= */
/*
 * Returns column count.
 */
int AttachmentModel::columnCount(const QModelIndex &parent) const
/* ========================================================================= */
{
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
	    descriptionHtml(m_attachmentModel.rowCount()));
	envelopeTextEdit->setReadOnly(true);

	/* Attachment list. */
	attachmentTable->setModel(&m_attachmentModel);
	attachmentTable->resizeColumnToContents(0);

	attachmentTable->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(attachmentTable, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(attachmentItemRightClicked(QPoint)));
	connect(attachmentTable, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(attachmentItemDoubleClicked(QModelIndex)));
}


/* ========================================================================= */
/*
 * Generates menu to selected message item.
 */
void DlgViewZfo::attachmentItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	qDebug() << __func__;

	/* TODO */
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
	/* TODO */
}


/* ========================================================================= */
/*
 * Saves selected attachments to directory.
 */
void DlgViewZfo::saveSelectedAttachmentsIntoDirectory(void)
/* ========================================================================= */
{
	/* TODO */
}


/* ========================================================================= */
/*
 * Open attachment in default application.
 */
void DlgViewZfo::openSelectedAttachment(void)
/* ========================================================================= */
{
	Q_ASSERT(0 != attachmentTable->selectionModel());

	QModelIndex selectedIndex =
	    attachmentTable->selectionModel()->currentIndex();

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return;
	}

	selectedIndex = selectedIndex.sibling(selectedIndex.row(), FNAME_COL);

	QString fileName = selectedIndex.data().toString();
	Q_ASSERT(!fileName.isEmpty());
	/* TODO -- Add message id into file name? */
	fileName = "qdatovka_attachment_" + fileName;

	//qDebug() << "Selected file: " << fileName;

	if (fileName.isEmpty()) {
		return;
	}

	QTemporaryFile fout(QDir::tempPath() + "/" + fileName);
	if (!fout.open()) {
		return; /* TODO -- Error message. */
	}
	fout.setAutoRemove(false);

	/* Get whole path. */
	fileName = fout.fileName();

	QByteArray data = m_attachmentModel.attachmentData(selectedIndex.row());

	int written = fout.write(data);
	if (written != data.size()) {
		/* TODO -- Error message? */
	}

	fout.close();

	//qDebug() << "file://" + fileName;
	QDesktopServices::openUrl(QUrl("file://" + fileName));
	/* TODO -- Handle openUrl() return value. */
}


/* ========================================================================= */
/*
 * Generate description from supplied message.
 */
QString DlgViewZfo::descriptionHtml(int attachmentCount)
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

	html += strongAccountInfoLine(tr("Message signature"), "TODO");
	html += strongAccountInfoLine(tr("Signing certificate"), "TODO");
	html += strongAccountInfoLine(tr("Time-stamp"), "TODO");

	return html;
}
