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

#include <algorithm> /* std::sort */
#include <QDateTime>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QMimeDatabase>

#include "src/cli/cmd_compose.h"
#include "src/datovka_shared/isds/box_interface.h"
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/datovka_shared/isds/types.h"
#include "src/datovka_shared/localisation/localisation.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/settings/records_management.h"
#include "src/datovka_shared/utility/strings.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/gui/datovka.h"
#include "src/gui/dlg_contacts.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_msg_box_informative.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_yes_no_checkbox.h"
#include "src/gui/helper.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"
#include "src/views/attachment_table_view.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_tab_ignore_filter.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_credit_info.h"
#include "src/worker/task_send_message.h"
#include "src/worker/task_search_owner.h"
#include "src/worker/task_search_owner_fulltext.h"
#include "src/worker/task_search_owner.h"
#include "ui_dlg_send_message.h"

/*
 * Message types as defined in ISDS and used in libisds.
 * For details see libisds.h .
 */
#define DMTYPE_INIT "I"
#define DMTYPE_COMM "K"
#define DMTYPE_RESP "O"

const QString &dzPrefix(MessageDb *messageDb, qint64 dmId)
{
	const static QString unspecified(QLatin1String("DZ"));
	const static QString received(QLatin1String("DDZ"));
	const static QString sent(QLatin1String("ODZ"));

	if (Q_UNLIKELY(Q_NULLPTR == messageDb || dmId < 0)) {
		return unspecified;
	}

	switch (messageDb->getMessageType(dmId)) {
	case MessageDb::TYPE_RECEIVED:
		return received;
		break;
	case MessageDb::TYPE_SENT:
		return sent;
		break;
	default:
		return unspecified;
		break;
	}
}

DlgSendMessage::DlgSendMessage(
    const QList<Task::AccountDescr> &messageDbSetList,
    enum Action action, const QList<MessageDb::MsgId> &msgIds,
    const QString &userName, const QString &composeSerialised,
    class MainWindow *mw, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgSendMessage),
    m_keepAliveTimer(),
    m_messageDbSetList(messageDbSetList),
    m_userName(userName),
    m_boxId(),
    m_senderName(),
    m_dbType(),
    m_dbEffectiveOVM(false),
    m_dbOpenAddressing(false),
    m_isLoggedIn(false),
    m_lastAttAddPath(),
    m_pdzCredit("0"),
    m_dmType(),
    m_dmSenderRefNumber(),
    m_dbSet(Q_NULLPTR),
    m_recipTableModel(this),
    m_attachModel(this),
    m_transactIds(),
    m_sentMsgResultList(),
    m_mw(mw)
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	setIcons();

	/* Set default line height for table views/widgets. */
	m_ui->recipTableView->setNarrowedLineHeight();
	m_ui->recipTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	m_ui->attachTableView->setNarrowedLineHeight();
	m_ui->attachTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	initContent(action, msgIds);
	if ((action == ACT_NEW) && !composeSerialised.isEmpty()) {
		fillContentCompose(composeSerialised);
	}

	Q_ASSERT(!m_boxId.isEmpty());
	Q_ASSERT(Q_NULLPTR != m_dbSet);
}

DlgSendMessage::~DlgSendMessage(void)
{
	delete m_ui;
}

void DlgSendMessage::immDownloadStateChanged(int state)
{
	if (state == Qt::Unchecked) {
		m_ui->immRecMgmtUploadCheckBox->setCheckState(Qt::Unchecked);
	}
}

void DlgSendMessage::immRecMgmtUploadStateChanged(int state)
{
	if (state == Qt::Checked) {
		m_ui->immDownloadCheckBox->setCheckState(Qt::Checked);
	}
}

void DlgSendMessage::checkInputFields(void)
{
	bool enable = calculateAndShowTotalAttachSize() &&
	    !m_ui->subjectLine->text().isEmpty() &&
	    (m_recipTableModel.rowCount() > 0) &&
	    (m_attachModel.rowCount() > 0);

	if (m_ui->payReplyCheckBox->isChecked()) {
		if (m_ui->dmSenderRefNumber->text().isEmpty()) {
			/*
			 * Transfer charges for reply can only be set when
			 * sender reference number is provided.
			 */
			enable = false;
		}
	}

	if (m_isLoggedIn) {
		m_ui->sendButton->setEnabled(enable);
	} else {
		/* cannot send message when not logged in. */
		m_ui->sendButton->setEnabled(false);
	}

	/* Enable only when sending a message to a single recipient. */
	m_ui->immRecMgmtUploadCheckBox->setEnabled(m_recipTableModel.rowCount() <= 1);
}

void DlgSendMessage::addRecipientFromLocalContact(void)
{
	QStringList dbIDs;
	DlgContacts::selectContacts(*m_dbSet, &dbIDs, this);
	addRecipientBoxes(dbIDs);
}

void DlgSendMessage::addRecipientFromISDSSearch(void)
{
	QStringList dbIDs(DlgDsSearch::search(m_userName, m_dbType,
	    m_dbEffectiveOVM, m_dbOpenAddressing, this));
	addRecipientBoxes(dbIDs);
}

void DlgSendMessage::addRecipientManually(void)
{
	bool ok = false;

	QString dbID = QInputDialog::getText(this, tr("Data box ID"),
	    tr("Enter data box ID (7 characters):"), QLineEdit::Normal,
	    QString(), &ok, Qt::WindowStaysOnTopHint);

	if (!ok) {
		return;
	}

	addRecipientBox(dbID);
}

void DlgSendMessage::recipientSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	/*
	 * DMTYPE_INIT means contractual PDZ that initiates a reply (prepaid?)
	 * PDZ. It should not be possible to delete recipients for those
	 * messages.
	 */
	m_ui->removeRecipButton->setEnabled(
	    (m_dmType != QStringLiteral(DMTYPE_INIT)) &&
	    (m_ui->recipTableView->selectionModel()->selectedRows(0).size() > 0));
}

/*!
 * @brief Used for sorting lists of integers.
 */
class RowLess {
public:
	bool operator()(int a, int b) const
	{
		return a < b;
	}
};

/*!
 * @brief Remove selected lines from model.
 *
 * @param[in]     view Table view to use for determining the selected lines.
 * @param[in,out] model Model to delete the rows from.
 */
static
void removeSelectedEntries(const QTableView *view, QAbstractItemModel *model)
{
	if (Q_UNLIKELY((view == Q_NULLPTR) || (model == Q_NULLPTR))) {
		Q_ASSERT(0);
		return;
	}

	QList<int> rows;
	{
		foreach (const QModelIndex &idx,
		         view->selectionModel()->selectedRows(0)) {
			rows.append(idx.row());
		}

		::std::sort(rows.begin(), rows.end(), RowLess());
	}

	/* In reverse order. */
	for (int i = rows.size() - 1; i >= 0; --i) {
		model->removeRow(rows.at(i));
	}
}

void DlgSendMessage::deleteRecipientEntries(void)
{
	removeSelectedEntries(m_ui->recipTableView, &m_recipTableModel);
}

void DlgSendMessage::showOptionalFormElements(void)
{
	m_ui->optionalForm->setHidden(
	    (m_ui->optionalFormCheckBox->checkState() == Qt::Unchecked) &&
	    (m_ui->payReplyCheckBox->checkState() == Qt::Unchecked));

	checkInputFields();

	if (m_ui->payReplyCheckBox->checkState() == Qt::Unchecked) {
		m_ui->dmSenderRefNumberLabel->setStyleSheet(
		    "QLabel { color: black }");
		m_ui->dmSenderRefNumberLabel->setText(
		    tr("Our reference number:"));
		disconnect(m_ui->dmSenderRefNumber, SIGNAL(textChanged(QString)),
		    this, SLOT(checkInputFields()));
	} else {
		m_ui->dmSenderRefNumberLabel->setStyleSheet(
		    "QLabel { color: red }");
		m_ui->dmSenderRefNumberLabel->setText(
		    tr("Enter reference number:"));
		m_ui->dmSenderRefNumber->setFocus();
		connect(m_ui->dmSenderRefNumber, SIGNAL(textChanged(QString)),
		    this, SLOT(checkInputFields()));
	}
}

void DlgSendMessage::addAttachmentFile(void)
{
	QFileDialog dialog(this);
	dialog.setDirectory(m_lastAttAddPath);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	QStringList fileNames;

	if (dialog.exec()) {
		fileNames = dialog.selectedFiles();
		if (!GlobInstcs::prefsPtr->useGlobalPaths) {
			m_lastAttAddPath = dialog.directory().absolutePath();
			emit usedAttachmentPath(m_userName, m_lastAttAddPath);
		}
	}

	insertAttachmentFiles(fileNames);
}

void DlgSendMessage::attachmentSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	int selectionSize =
	    m_ui->attachTableView->selectionModel()->selectedRows(0).size();

	m_ui->removeAttachButton->setEnabled(selectionSize > 0);
	m_ui->openAttachButton->setEnabled(selectionSize == 1);
}

void DlgSendMessage::deleteSelectedAttachmentFiles(void)
{
	removeSelectedEntries(m_ui->attachTableView, &m_attachModel);
}

void DlgSendMessage::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	AttachmentInteraction::openAttachment(this, *m_ui->attachTableView,
	    index);
}

void DlgSendMessage::pingIsdsServer(void) const
{
	GuiHelper::pingIsdsServer(m_userName);
}

void DlgSendMessage::setAccountInfo(int fromComboIdx)
{
	debugSlotCall();

	/* Get user name for selected account. */
	const QString userName(
	    m_ui->fromComboBox->itemData(fromComboIdx).toString());
	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	/* Remove all recipients if account was changed. */
	if (m_userName != userName) {
		m_recipTableModel.removeRows(0, m_recipTableModel.rowCount());
		m_userName = userName;
	}

	m_isLoggedIn = GuiHelper::isLoggedIn(m_keepAliveTimer, m_mw,
	    m_userName);

	m_dbSet = GuiHelper::getDbSet(m_messageDbSetList, m_userName);
	if (Q_UNLIKELY(Q_NULLPTR == m_dbSet)) {
		Q_ASSERT(0);
		return;
	}

	const AcntSettings &accountInfo((*GlobInstcs::acntMapPtr)[m_userName]);
	const QString acntDbKey(AccountDb::keyFromLogin(m_userName));
	m_boxId = GlobInstcs::accntDbPtr->dbId(acntDbKey);
	Q_ASSERT(!m_boxId.isEmpty());
	m_senderName = GlobInstcs::accntDbPtr->senderNameGuess(acntDbKey);
	const Isds::DbOwnerInfo dbOwnerInfo(
	    GlobInstcs::accntDbPtr->getOwnerInfo(acntDbKey));
	if (!dbOwnerInfo.isNull()) {
		m_dbType = Isds::dbType2Str(dbOwnerInfo.dbType());
		m_dbEffectiveOVM = (dbOwnerInfo.dbEffectiveOVM() == Isds::Type::BOOL_TRUE);
		m_dbOpenAddressing = (dbOwnerInfo.dbOpenAddressing() == Isds::Type::BOOL_TRUE);
	}
	if (GlobInstcs::prefsPtr->useGlobalPaths) {
		m_lastAttAddPath =
		    GlobInstcs::prefsPtr->addFileToAttachmentsPath;
	} else {
		m_lastAttAddPath = accountInfo.lastAttachAddPath();
	}

	if (m_dbOpenAddressing) {
		m_pdzCredit = getPDZCreditFromISDS(m_userName, m_boxId);
	}

	QString dbOpenAddressingText;
	if (!m_dbEffectiveOVM) {
		if (m_dbOpenAddressing) {
			dbOpenAddressingText =
			    " - " + tr("sending of PDZ: enabled") + "; " +
			    tr("remaining credit: ") + m_pdzCredit + " Kč";
		} else {
			dbOpenAddressingText =
			    " - " + tr("sending of PDZ: disabled");
		}
	}

	m_ui->fromUser->setText("<strong>" +
	    (*GlobInstcs::acntMapPtr)[m_userName].accountName() + "</strong>" +
	    " (" + m_userName + ") - " + m_dbType + dbOpenAddressingText);
}

void DlgSendMessage::sendMessage(void)
{
	debugSlotCall();

	const QList<BoxContactsModel::PartialEntry> recipEntries(
	    m_recipTableModel.partialBoxEntries(BoxContactsModel::ANY));

	sendMessageISDS(recipEntries);
}

void DlgSendMessage::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId, int processFlags)
{
	debugSlotCall();

	Q_UNUSED(userName);
	Q_UNUSED(processFlags);

	if (m_transactIds.end() == m_transactIds.find(transactId)) {
		/* Nothing found. */
		return;
	}

	/* Gather data. */
	m_sentMsgResultList.append(TaskSendMessage::ResultData(
	    (enum TaskSendMessage::Result) result, resultDesc,
	    dbIDRecipient, recipientName, isPDZ, dmId));

	if (!m_transactIds.remove(transactId)) {
		logErrorNL("%s",
		    "Was not able to remove a transaction identifier from list of unfinished transactions.");
	}

	if (!m_transactIds.isEmpty()) {
		/* Still has some pending transactions. */
		return;
	}

	/* All transactions finished. */

	this->setCursor(Qt::ArrowCursor);
	this->setEnabled(true);

	int successfullySentCnt = 0;
	QString infoText;

	foreach (const TaskSendMessage::ResultData &resultData,
	         m_sentMsgResultList) {
		if (TaskSendMessage::SM_SUCCESS == resultData.result) {
			++successfullySentCnt;

			if (resultData.isPDZ) {
				infoText += tr(
				    "Message was successfully sent to "
				    "<i>%1 (%2)</i> as PDZ with number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			} else {
				infoText += tr(
				    "Message was successfully sent to "
				    "<i>%1 (%2)</i> as message number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			}
		} else {
			infoText += tr("Message was NOT sent to "
			    "<i>%1 (%2)</i>. Server says: %3").
			    arg(resultData.recipientName).
			    arg(resultData.dbIDRecipient).
			    arg(resultData.errInfo) + "<br/>";
		}
	}
	m_sentMsgResultList.clear();

	if (m_recipTableModel.rowCount() == successfullySentCnt) {
		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Message sent"),
		    "<b>" + tr("Message was successfully sent to all recipients.") + "</b>",
		    infoText, QString(), QMessageBox::Ok, QMessageBox::Ok);
		this->accept(); /* Set return code to accepted. */
	} else {
		infoText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") + "</b>";
		int ret = DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Message sending error"),
		    "<b>" + tr("Message was NOT sent to all recipients.") + "</b>",
		    infoText, QString(), QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::No);
		if (ret == QMessageBox::Yes) {
			this->close(); /* Set return code to closed. */
		}
	}

	emit usedAttachmentPath(m_userName, m_lastAttAddPath);
}

void DlgSendMessage::initContent(enum Action action,
    const QList<MessageDb::MsgId> &msgIds)
{
	m_recipTableModel.setHeader();
	m_ui->recipTableView->setModel(&m_recipTableModel);

	m_ui->recipTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 60);
	m_ui->recipTableView->setColumnWidth(BoxContactsModel::BOX_TYPE_COL, 70);
	m_ui->recipTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 120);
	m_ui->recipTableView->setColumnWidth(BoxContactsModel::ADDRESS_COL, 100);

	m_ui->recipTableView->setColumnHidden(BoxContactsModel::CHECKBOX_COL, true);
	m_ui->recipTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL, true);

	m_attachModel.setHeader();
	m_ui->attachTableView->setModel(&m_attachModel);

	m_ui->attachTableView->setColumnWidth(AttachmentTblModel::FNAME_COL, 150);
	m_ui->attachTableView->setColumnWidth(AttachmentTblModel::MIME_COL, 120);

	m_ui->attachTableView->setColumnHidden(AttachmentTblModel::ATTACHID_COL, true);
	m_ui->attachTableView->setColumnHidden(AttachmentTblModel::MSGID_COL, true);
	m_ui->attachTableView->setColumnHidden(AttachmentTblModel::BINARY_CONTENT_COL, true);

	/* Enable drag and drop on attachment table. */
	m_ui->attachTableView->setAcceptDrops(true);
	m_ui->attachTableView->setDragEnabled(true);
	m_ui->attachTableView->setDragDropOverwriteMode(false);
	m_ui->attachTableView->setDropIndicatorShown(true);
	m_ui->attachTableView->setDragDropMode(QAbstractItemView::DragDrop);
	m_ui->attachTableView->setDefaultDropAction(Qt::CopyAction);

	m_ui->prepaidReplyLabel->setEnabled(false);
	m_ui->prepaidReplyLabel->hide();

	m_ui->immDownloadCheckBox->setCheckState(Qt::Unchecked);
	m_ui->immRecMgmtUploadCheckBox->setCheckState(Qt::Unchecked);
	m_ui->immRecMgmtUploadCheckBox->setVisible(GlobInstcs::recMgmtSetPtr->isValid());

	Q_ASSERT(!m_userName.isEmpty());

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		const QString accountName =
		    (*GlobInstcs::acntMapPtr)[acnt.userName].accountName() +
		    " (" + acnt.userName + ")";
		m_ui->fromComboBox->addItem(accountName, QVariant(acnt.userName));
		if (m_userName == acnt.userName) {
			int i = m_ui->fromComboBox->count() - 1;
			Q_ASSERT(0 <= i);
			m_ui->fromComboBox->setCurrentIndex(i);
			setAccountInfo(i);
		}
	}

	connect(m_ui->fromComboBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(setAccountInfo(int)));

	connect(&m_recipTableModel, SIGNAL(rowsInserted(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_recipTableModel, SIGNAL(rowsRemoved(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));

	m_ui->optionalForm->setHidden(true);

	connect(m_ui->optionalFormCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(showOptionalFormElements()));
	connect(m_ui->payReplyCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(showOptionalFormElements()));

	connect(m_ui->addRecipButton, SIGNAL(clicked()),
	    this, SLOT(addRecipientFromLocalContact()));
	connect(m_ui->removeRecipButton, SIGNAL(clicked()),
	    this, SLOT(deleteRecipientEntries()));
	connect(m_ui->findRecipButton, SIGNAL(clicked()),
	    this, SLOT(addRecipientFromISDSSearch()));
	connect(m_ui->enterBoxIdButton, SIGNAL(clicked()),
	    this, SLOT(addRecipientManually()));

	connect(m_ui->addAttachButton, SIGNAL(clicked()), this,
	    SLOT(addAttachmentFile()));
	connect(m_ui->removeAttachButton, SIGNAL(clicked()), this,
	    SLOT(deleteSelectedAttachmentFiles()));
	connect(m_ui->openAttachButton, SIGNAL(clicked()), this,
	    SLOT(openSelectedAttachment()));

	connect(m_ui->recipTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(recipientSelectionChanged(QItemSelection, QItemSelection)));

	connect(m_ui->attachTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(openSelectedAttachment(QModelIndex)));

	connect(m_ui->subjectLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(&m_attachModel, SIGNAL(rowsInserted(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_attachModel, SIGNAL(rowsRemoved(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_attachModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->attachTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(attachmentSelectionChanged(QItemSelection, QItemSelection)));

	m_ui->recipTableView->setEditTriggers(
	    QAbstractItemView::NoEditTriggers);
	m_ui->attachTableView->setEditTriggers(
	    QAbstractItemView::NoEditTriggers);

	m_ui->recipTableView->installEventFilter(
	    new TableHomeEndFilter(m_ui->recipTableView));
	m_ui->recipTableView->installEventFilter(
	    new TableTabIgnoreFilter(m_ui->recipTableView));
	m_ui->attachTableView->installEventFilter(
	    new TableHomeEndFilter(m_ui->attachTableView));
	m_ui->attachTableView->installEventFilter(
	    new TableTabIgnoreFilter(m_ui->attachTableView));

	connect(m_ui->immDownloadCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(immDownloadStateChanged(int)));
	connect(m_ui->immRecMgmtUploadCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(immRecMgmtUploadStateChanged(int)));

	connect(m_ui->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)));

	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	connect(&m_keepAliveTimer, SIGNAL(timeout()), this, SLOT(pingIsdsServer()));

	calculateAndShowTotalAttachSize();

	if (Isds::str2DbType(m_dbType) > Isds::Type::BT_OVM_REQ) {
		m_ui->dmAllowSubstDelivery->setEnabled(false);
		m_ui->dmAllowSubstDelivery->hide();
	}

	if (ACT_REPLY == action) {
		fillContentAsReply(msgIds);
	} else {
		if (m_dbOpenAddressing) {
			m_ui->payReplyCheckBox->setEnabled(true);
			m_ui->payReplyCheckBox->show();
		} else {
			m_ui->payReplyCheckBox->setEnabled(false);
			m_ui->payReplyCheckBox->hide();
		}

		m_ui->payRecipient->setEnabled(false);
		m_ui->payRecipient->hide();
		if (ACT_NEW_FROM_TMP == action) {
			fillContentFromTemplate(msgIds);
		} else if (ACT_FORWARD == action) {
			fillContentAsForward(msgIds);
		}
	}

	this->adjustSize();
}

void DlgSendMessage::fillContentAsForward(const QList<MessageDb::MsgId> &msgIds)
{
	debugFuncCall();

	if (msgIds.size() == 0) {
		logWarningNL("%s",
		    "Expected at least one message to generate message from.");
		return;
	}

	/* Fill attachments with messages. */
	foreach (const MessageDb::MsgId &msgId, msgIds) {
		MessageDb *messageDb =
		    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
		if (Q_NULLPTR == messageDb) {
			Q_ASSERT(0);
			continue;
		}

		/* If only a single message if forwarded. */
		if (msgIds.size() == 1) {
			Isds::Envelope envData(
			    messageDb->getMessageReplyData(msgId.dmId));

			m_ui->subjectLine->setText("Fwd: " + envData.dmAnnotation());
		}

		QByteArray msgBinary(messageDb->getCompleteMessageRaw(msgId.dmId));
		if (msgBinary.isEmpty()) {
			continue;
		}

		m_attachModel.appendBinaryAttachment(msgBinary,
		    QString("%1_%2.zfo").arg(dzPrefix(messageDb, msgId.dmId)).arg(msgId.dmId));
	}
}

void DlgSendMessage::fillContentAsReply(const QList<MessageDb::MsgId> &msgIds)
{
	debugFuncCall();

	if (msgIds.size() != 1) {
		logWarningNL("%s",
		    "Expected one message to generate reply from.");
		return;
	}
	const MessageDb::MsgId &msgId(msgIds.first());

	bool hideOptionalForm = true;

	m_ui->fromComboBox->setEnabled(false);

	MessageDb *messageDb =
	    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
	Q_ASSERT(Q_NULLPTR != messageDb);

	const Isds::Envelope envData =
	    messageDb->getMessageReplyData(msgId.dmId);
	m_dmType = envData.dmType();
	m_dmSenderRefNumber = envData.dmRecipientRefNumber();

	m_ui->subjectLine->setText("Re: " + envData.dmAnnotation());

	if (!envData.dmSenderRefNumber().isEmpty()) {
		m_ui->dmRecipientRefNumber->setText(envData.dmSenderRefNumber());
		hideOptionalForm = false;
	}
	if (!envData.dmSenderIdent().isEmpty()) {
		m_ui->dmRecipientIdent->setText(envData.dmSenderIdent());
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientRefNumber().isEmpty()) {
		m_ui->dmSenderRefNumber->setText(envData.dmRecipientRefNumber());
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientIdent().isEmpty()) {
		m_ui->dmSenderIdent->setText(envData.dmRecipientIdent());
		hideOptionalForm = false;
	}

	m_ui->optionalForm->setHidden(hideOptionalForm);
	m_ui->optionalFormCheckBox->setChecked(!hideOptionalForm);
	m_ui->payRecipient->setEnabled(false);
	m_ui->payRecipient->hide();
	m_ui->payRecipient->setChecked(false);

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDSender());
		m_ui->payReplyCheckBox->setEnabled(true);
		m_ui->payReplyCheckBox->show();
	} else {
		m_ui->payReplyCheckBox->setEnabled(false);
		m_ui->payReplyCheckBox->hide();
		pdz = false;
	}

	if (m_dmType == QStringLiteral(DMTYPE_INIT)) {
		m_ui->addRecipButton->setEnabled(false);
		m_ui->removeRecipButton->setEnabled(false);
		m_ui->findRecipButton->setEnabled(false);
		m_ui->prepaidReplyLabel->setEnabled(true);
		m_ui->prepaidReplyLabel->show();
		m_ui->payReplyCheckBox->setEnabled(false);
		m_ui->payReplyCheckBox->hide();
		m_ui->payRecipient->setEnabled(true);
		m_ui->payRecipient->show();
		m_ui->payRecipient->setChecked(true);
		pdz = true;
	}

	m_recipTableModel.appendData(envData.dbIDSender(), Isds::Type::BT_NULL,
	    envData.dmSender(), envData.dmSenderAddress(), QString(), pdz);
}

void DlgSendMessage::fillContentFromTemplate(
    const QList<MessageDb::MsgId> &msgIds)
{
	debugFuncCall();

	if (msgIds.size() != 1) {
		logWarningNL("%s",
		    "Expected one message to generate message from.");
		return;
	}
	const MessageDb::MsgId &msgId(msgIds.first());

	bool hideOptionalForm = true;

	MessageDb *messageDb =
	    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
	Q_ASSERT(Q_NULLPTR != messageDb);

	const Isds::Envelope envData =
	    messageDb->getMessageReplyData(msgId.dmId);
	m_dmType = envData.dmType();
	m_dmSenderRefNumber = envData.dmRecipientRefNumber();

	m_ui->subjectLine->setText(envData.dmAnnotation());

	/* Fill in optional fields.  */
	if (!envData.dmSenderRefNumber().isEmpty()) {
		m_ui->dmSenderRefNumber->setText(envData.dmSenderRefNumber());
		hideOptionalForm = false;
	}
	if (!envData.dmSenderIdent().isEmpty()) {
		m_ui->dmSenderIdent->setText(envData.dmSenderIdent());
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientRefNumber().isEmpty()) {
		m_ui->dmRecipientRefNumber->setText(envData.dmRecipientRefNumber());
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientIdent().isEmpty()) {
		m_ui->dmRecipientIdent->setText(envData.dmRecipientIdent());
		hideOptionalForm = false;
	}
	if (!envData.dmToHands().isEmpty()) {
		m_ui->dmToHands->setText(envData.dmToHands());
		hideOptionalForm = false;
	}
	/* set check boxes */
	m_ui->dmPersonalDelivery->setChecked(envData.dmPersonalDelivery());
	m_ui->dmAllowSubstDelivery->setChecked(envData.dmAllowSubstDelivery());
	/* fill optional LegalTitle - Law, year, ... */
	if (!envData.dmLegalTitleLawStr().isEmpty()) {
		m_ui->dmLegalTitleLaw->setText(envData.dmLegalTitleLawStr());
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitleYearStr().isEmpty()) {
		m_ui->dmLegalTitleYear->setText(envData.dmLegalTitleYearStr());
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitleSect().isEmpty()) {
		m_ui->dmLegalTitleSect->setText(envData.dmLegalTitleSect());
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitlePar().isEmpty()) {
		m_ui->dmLegalTitlePar->setText(envData.dmLegalTitlePar());
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitlePoint().isEmpty()) {
		m_ui->dmLegalTitlePoint->setText(envData.dmLegalTitlePoint());
		hideOptionalForm = false;
	}

	m_ui->optionalForm->setHidden(hideOptionalForm);
	m_ui->optionalFormCheckBox->setChecked(!hideOptionalForm);

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDRecipient());
		m_ui->payReplyCheckBox->setEnabled(true);
		m_ui->payReplyCheckBox->show();
	} else {
		m_ui->payReplyCheckBox->setEnabled(false);
		m_ui->payReplyCheckBox->hide();
		pdz = false;
	}

	/* message is received -> recipient == sender */
	if (m_boxId != envData.dbIDRecipient()) {
		m_recipTableModel.appendData(envData.dbIDRecipient(),
		    Isds::Type::BT_NULL, envData.dmRecipient(),
		    envData.dmRecipientAddress(), QString(), pdz);
	}

	/* fill attachments from template message */
	QList<Isds::Document> msgFileList =
	    messageDb->getMessageAttachments(msgId.dmId);

	foreach (const Isds::Document &file, msgFileList) {
		m_attachModel.appendBinaryAttachment(file.binaryContent(),
		    file.fileDescr());
	}
}

void DlgSendMessage::fillContentCompose(const QString &composeSerialised)
{
	debugFuncCall();

	const CLI::CmdCompose composeCmd(
	    CLI::CmdCompose::deserialise(composeSerialised));
	if (composeCmd.isNull()) {
		logErrorNL("%s", "Could not deserialise compose data.");
		return;
	}

	if (!composeCmd.dmToHands().isEmpty() ||
	    !composeCmd.dmRecipientRefNumber().isEmpty() ||
	    !composeCmd.dmSenderRefNumber().isEmpty() ||
	    !composeCmd.dmRecipientIdent().isEmpty() ||
	    !composeCmd.dmSenderIdent().isEmpty() ||
	    !composeCmd.dmLegalTitleLawStr().isEmpty() ||
	    !composeCmd.dmLegalTitleYearStr().isEmpty() ||
	    !composeCmd.dmLegalTitleSect().isEmpty() ||
	    !composeCmd.dmLegalTitlePar().isEmpty() ||
	    !composeCmd.dmLegalTitlePoint().isEmpty() ||
	    (composeCmd.dmPersonalDelivery() == Isds::Type::BOOL_TRUE)) {
		m_ui->optionalFormCheckBox->setCheckState(Qt::Checked);
	}

	if (!composeCmd.dbIDRecipient().isEmpty()) {
		addRecipientBoxes(composeCmd.dbIDRecipient());
	}
	if (!composeCmd.dmAnnotation().isEmpty()) {
		m_ui->subjectLine->setText(composeCmd.dmAnnotation());
	}
	if (!composeCmd.dmToHands().isEmpty()) {
		m_ui->dmToHands->setText(composeCmd.dmToHands());
	}
	if (!composeCmd.dmRecipientRefNumber().isEmpty()) {
		m_ui->dmRecipientRefNumber->setText(composeCmd.dmRecipientRefNumber());
	}
	if (!composeCmd.dmSenderRefNumber().isEmpty()) {
		m_ui->dmSenderRefNumber->setText(composeCmd.dmSenderRefNumber());
	}
	if (!composeCmd.dmRecipientIdent().isEmpty()) {
		m_ui->dmRecipientIdent->setText(composeCmd.dmRecipientIdent());
	}
	if (!composeCmd.dmSenderIdent().isEmpty()) {
		m_ui->dmSenderIdent->setText(composeCmd.dmSenderIdent());
	}
	if (!composeCmd.dmLegalTitleLawStr().isEmpty()) {
		m_ui->dmLegalTitleLaw->setText(composeCmd.dmLegalTitleLawStr());
	}
	if (!composeCmd.dmLegalTitleYearStr().isEmpty()) {
		m_ui->dmLegalTitleYear->setText(composeCmd.dmLegalTitleYearStr());
	}
	if (!composeCmd.dmLegalTitleSect().isEmpty()) {
		m_ui->dmLegalTitleSect->setText(composeCmd.dmLegalTitleSect());
	}
	if (!composeCmd.dmLegalTitlePar().isEmpty()) {
		m_ui->dmLegalTitlePar->setText(composeCmd.dmLegalTitlePar());
	}
	if (!composeCmd.dmLegalTitlePoint().isEmpty()) {
		m_ui->dmLegalTitlePoint->setText(composeCmd.dmLegalTitlePoint());
	}
	if (composeCmd.dmPersonalDelivery() != Isds::Type::BOOL_NULL) {
		m_ui->dmPersonalDelivery->setCheckState(
		    (composeCmd.dmPersonalDelivery() == Isds::Type::BOOL_TRUE) ?
		        Qt::Checked : Qt::Unchecked);
	}
	if (composeCmd.dmAllowSubstDelivery() != Isds::Type::BOOL_NULL) {
		m_ui->dmAllowSubstDelivery->setCheckState(
		    (composeCmd.dmAllowSubstDelivery() == Isds::Type::BOOL_TRUE) ?
		        Qt::Checked : Qt::Unchecked);
	}
	if (composeCmd.dmPublishOwnID() != Isds::Type::BOOL_NULL) {
		m_ui->dmPublishOwnID->setCheckState(
		    (composeCmd.dmPublishOwnID() == Isds::Type::BOOL_TRUE) ?
		        Qt::Checked : Qt::Unchecked);
	}
	if (!composeCmd.dmAttachment().isEmpty()) {
		insertAttachmentFiles(composeCmd.dmAttachment());
	}
}

void DlgSendMessage::insertAttachmentFiles(const QStringList &filePaths)
{
	foreach (const QString &filePath, filePaths) {
		int fileSize = m_attachModel.insertAttachmentFile(filePath,
		    m_attachModel.rowCount());
		switch (fileSize) {
		case AttachmentTblModel::FILE_NOT_EXISTENT:
			QMessageBox::warning(this, tr("Non-existent file"),
			    tr("Cannot add non-existent file '%1' to attachments.").arg(filePath),
			    QMessageBox::Ok, QMessageBox::Ok);
			break;
		case AttachmentTblModel::FILE_NOT_READABLE:
			QMessageBox::warning(this, tr("File not readable"),
			    tr("Cannot add file '%1' without readable permissions to attachments.").arg(filePath),
			    QMessageBox::Ok, QMessageBox::Ok);
			break;
		case AttachmentTblModel::FILE_ALREADY_PRESENT:
			QMessageBox::warning(this, tr("File already present"),
			    tr("Cannot add file '%1' because the file is already present in the attachments.").arg(filePath),
			    QMessageBox::Ok, QMessageBox::Ok);
			break;
		case AttachmentTblModel::FILE_ZERO_SIZE:
			QMessageBox::warning(this, tr("Empty file"),
			    tr("Cannot add empty file '%1' to attachments.").arg(filePath),
			    QMessageBox::Ok, QMessageBox::Ok);
			break;
		default:
			break;
		}
	}
}

int DlgSendMessage::notifyOfPDZ(int pdzCnt)
{
	QString title;
	QString info;

	if (pdzCnt > 1) {
		title = tr("Message contains non-OVM recipients.");
		info = tr("Your message contains %1 non-OVM recipients "
		    "therefore this message will be sent as a "
		    "commercial messages (PDZ) for these recipients.").
		    arg(pdzCnt);
		info += "\n\n";
		info += tr("Do you want to send all messages?");
	} else {
		title = tr("Message contains non-OVM recipient.");
		info = tr("Your message contains non-OVM recipient "
		    "therefore this message will be sent as a "
		    "commercial message (PDZ) for this recipient.");
		info += "\n\n";
		info += tr("Do you want to send message?");
	}

	info += "\n\n" + tr("Your remaining credit is ") + m_pdzCredit + " Kč";

	return DlgMsgBox::message(this, QMessageBox::Information, title, title,
	    info, QString(), QMessageBox::Yes | QMessageBox::No,
	    QMessageBox::Yes);
}

bool DlgSendMessage::calculateAndShowTotalAttachSize(void)
{
	qint64 aSize = m_attachModel.totalAttachmentSize();

	if (m_attachModel.rowCount() == 0) {
		Q_ASSERT(aSize == 0);
		m_ui->attachmentSizeInfo->setEnabled(false);
		m_ui->attachmentSizeInfo->setStyleSheet(QString());
		m_ui->attachmentSizeInfo->setText(
		    tr("Total size of attachments is %1 B").arg(0));
		return false;
	} else {
		m_ui->attachmentSizeInfo->setEnabled(true);
	}

	m_ui->attachmentSizeInfo->setStyleSheet("QLabel { color: red }");

	if (m_attachModel.rowCount() > MAX_ATTACHMENT_FILES) {
		m_ui->attachmentSizeInfo->setText(
		    tr("The permitted amount (%1) of attachments has been exceeded.")
		        .arg(MAX_ATTACHMENT_FILES));
		return false;
	}

	if (aSize >= MAX_OVM_ATTACHMENT_SIZE_BYTES) {
		m_ui->attachmentSizeInfo->setText(
		    tr("Total size of attachments is larger than %1 MB.")
		        .arg(MAX_OVM_ATTACHMENT_SIZE_MB));
		return false;
	}

	if (aSize >= MAX_ATTACHMENT_SIZE_BYTES) {
		m_ui->attachmentSizeInfo->setText(
		    tr("Total size of attachments is larger than %1 MB. Most of the data boxes cannot receive messages larger than %1 MB. However some OVM data boxes can receive message up to %2 MB.")
		        .arg(MAX_ATTACHMENT_SIZE_MB).arg(MAX_OVM_ATTACHMENT_SIZE_MB));
		return true;
	}

	m_ui->attachmentSizeInfo->setStyleSheet("QLabel { color: black }");
	if (aSize >= 1024) {
		m_ui->attachmentSizeInfo->setText(tr("Total size of attachments is ~%1 KB").arg(aSize/1024));
	} else {
		m_ui->attachmentSizeInfo->setText(tr("Total size of attachments is ~%1 B").arg(aSize));
	}
	return true;
}

void DlgSendMessage::addRecipientBox(const QString &boxId)
{
	if (boxId.isEmpty() || boxId.length() != 7) {
		QMessageBox::critical(this, tr("Wrong data box ID"),
		    tr("Wrong data box ID '%1'!").arg(boxId),
		    QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	/* Ignore existent entry. */
	if (m_recipTableModel.containsBoxId(boxId)) {
		return;
	}

	/*
	 * If we are manually adding the recipient then we may not be able to
	 * download information about the data box.
	 *
	 * TODO -- If we have been searching for the data box then we are
	 * downloading the information about the data box for the second time.
	 * This is not optimal.
	 */

	/* Search for box information according to supplied identifier. */
	enum TaskSearchOwnerFulltext::Result result =
	    TaskSearchOwnerFulltext::SOF_ERROR;
	QString errMsg, longErrMsg;
	QList<Isds::FulltextResult> foundBoxes;
	TaskSearchOwnerFulltext *task =
	    new (std::nothrow) TaskSearchOwnerFulltext(m_userName, boxId,
	        Isds::Type::FST_BOX_ID, TaskSearchOwnerFulltext::BT_ALL);
	if (task != Q_NULLPTR) {
		task->setAutoDelete(false);
		GlobInstcs::workPoolPtr->runSingle(task);

		result = task->m_result;
		errMsg = task->m_isdsError;
		longErrMsg = task->m_isdsLongError;
		foundBoxes = task->m_foundBoxes;
		delete task; task = Q_NULLPTR;
	}

	QString name = tr("Unknown");
	QString address = tr("Unknown");
	QVariant pdz;
	enum Isds::Type::DbType boxType = Isds::Type::BT_NULL; /* Unknown type. */

	if (foundBoxes.size() == 1) {
		const Isds::FulltextResult &entry(foundBoxes.first());

		name = entry.dbName();
		address = entry.dbAddress();
		boxType = entry.dbType();
		if (!entry.active()) {
			DlgMsgBox::message(this, QMessageBox::Warning,
			    tr("Data box is not active"),
			    tr("Recipient with data box ID '%1' does not have active data box.")
			        .arg(boxId),
			    tr("The message cannot be delivered."), QString(),
			    QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		if (entry.publicSending()) {
			pdz = false;
		} else if (entry.commercialSending()) {
			pdz = true;
		} else if (entry.dbEffectiveOVM()) {
			pdz = false;
		} else {
			DlgMsgBox::message(this, QMessageBox::Critical,
			    tr("Cannot send to data box"),
			    tr("Cannot send message to recipient with data box ID '%1'.")
			        .arg(boxId),
			    tr("You won't be able as user '%1' to send messages into data box '%2'.")
			        .arg(m_userName).arg(boxId),
			    QString(), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
	} else if (foundBoxes.isEmpty()) {
		if (result == TaskSearchOwnerFulltext::SOF_SUCCESS) {
			/* No data box found. */
			QMessageBox::critical(this, tr("Wrong Recipient"),
			    tr("Recipient with data box ID '%1' does not exist.")
			        .arg(boxId),
			    QMessageBox::Ok, QMessageBox::Ok);
			return;
		} else {
			/* Search error. */
			DlgYesNoCheckbox questionDlg(tr("Recipient Search Failed"),
			    tr("Information about recipient data box could not be obtained.") +
			    QStringLiteral("\n") +
			    tr("Do you still want to add the box '%1' into the recipient list?").arg(boxId),
			    tr("Enable commercial messages (PDZ)."),
			    !longErrMsg.isEmpty() ?
			        tr("Obtained ISDS error") + QStringLiteral(": ") + longErrMsg :
			        QString());
			int retVal = questionDlg.exec();
			switch (retVal) {
			case DlgYesNoCheckbox::YesChecked:
				pdz = true;
				break;
			case DlgYesNoCheckbox::YesUnchecked:
				pdz = false;
				break;
			default:
				return;
				break;
			}
		}
	} else {
		Q_ASSERT(0);
		return;
	}

	m_recipTableModel.appendData(boxId, boxType, name, address, QString(),
	    pdz);
}

void DlgSendMessage::addRecipientBoxes(const QStringList &boxIds)
{
	foreach (const QString &boxId, boxIds) {
		addRecipientBox(boxId);
	}
}

QString DlgSendMessage::getPDZCreditFromISDS(const QString &userName,
    const QString &dbId)
{
	debugFuncCall();

	TaskDownloadCreditInfo *task =
	    new (std::nothrow) TaskDownloadCreditInfo(userName, dbId);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return QString();
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	qint64 credit = task->m_heller;
	delete task; task = Q_NULLPTR;

	if (credit <= 0) {
		return "0";
	}

	return Localisation::programLocale.toString((float)credit / 100, 'f', 2);
}

bool DlgSendMessage::queryISDSBoxEOVM(const QString &userName,
    const QString &boxId)
{
	bool ret = false;

	Isds::DbOwnerInfo dbOwnerInfo;
	dbOwnerInfo.setDbID(boxId);
	dbOwnerInfo.setDbType(Isds::Type::BT_FO);

	TaskSearchOwner *task =
	    new (std::nothrow) TaskSearchOwner(userName, dbOwnerInfo);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return false;
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	const QList<Isds::DbOwnerInfo> foundBoxes(task->m_foundBoxes);

	delete task; task = Q_NULLPTR;

	if (foundBoxes.count() == 1) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
		const Isds::DbOwnerInfo &box(foundBoxes.constFirst());
#else /* < Qt-5.6 */
		const Isds::DbOwnerInfo &box(foundBoxes.first());
#endif /* >= Qt-5.6 */
		ret = (box.dbEffectiveOVM() == Isds::Type::BOOL_TRUE);
	}

	return ret;
}

bool DlgSendMessage::buildEnvelope(Isds::Envelope &envelope) const
{
	Isds::Type::DmType dmType = Isds::Type::MT_UNKNOWN;

	/* Set mandatory fields of envelope. */
	envelope.setDmAnnotation(m_ui->subjectLine->text());

	/* Set optional fields. */
	envelope.setDmSenderIdent(m_ui->dmSenderIdent->text());
	envelope.setDmRecipientIdent(m_ui->dmRecipientIdent->text());
	envelope.setDmSenderRefNumber(m_ui->dmSenderRefNumber->text());
	envelope.setDmRecipientRefNumber(m_ui->dmRecipientRefNumber->text());
	if (!m_ui->dmLegalTitleLaw->text().isEmpty()) {
		envelope.setDmLegalTitleLaw(m_ui->dmLegalTitleLaw->text().toLongLong());
	}
	if (!m_ui->dmLegalTitleYear->text().isEmpty()) {
		envelope.setDmLegalTitleYear(m_ui->dmLegalTitleYear->text().toLongLong());
	}
	envelope.setDmLegalTitleSect(m_ui->dmLegalTitleSect->text());
	envelope.setDmLegalTitlePar(m_ui->dmLegalTitlePar->text());
	envelope.setDmLegalTitlePoint(m_ui->dmLegalTitlePoint->text());
	envelope.setDmPersonalDelivery(m_ui->dmPersonalDelivery->isChecked() ?
	    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);

	/* Only OVM can change. */
	if (Isds::str2DbType(m_dbType) > Isds::Type::BT_OVM_REQ) {
		envelope.setDmAllowSubstDelivery(Isds::Type::BOOL_TRUE);
	} else {
		envelope.setDmAllowSubstDelivery(
		   m_ui->dmAllowSubstDelivery->isChecked() ?
		   Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);
	}

	if (m_dmType == QStringLiteral(DMTYPE_INIT)) {
		if (m_ui->payRecipient->isChecked()) {
			dmType = Isds::Type::MT_O;
		} else {
			dmType = Isds::Type::MT_K;
		}
		if (!m_dmSenderRefNumber.isEmpty()) {
			envelope.setDmRecipientRefNumber(m_dmSenderRefNumber);
		}
	} else {
		if (m_ui->payReplyCheckBox->isChecked()) {
			dmType = Isds::Type::MT_I;
		}
	}

	envelope.setDmType(Isds::Envelope::dmType2Char(dmType));
	envelope.setDmOVM(m_dbEffectiveOVM ?
	    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);
	envelope.setDmPublishOwnID((m_ui->dmPublishOwnID->isChecked()) ?
	    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);

	return true;
}

bool DlgSendMessage::buildDocuments(QList<Isds::Document> &documents) const
{
	/* Load attachments. */
	for (int row = 0; row < m_attachModel.rowCount(); ++row) {
		Isds::Document document;
		QModelIndex index;

		index = m_attachModel.index(row, AttachmentTblModel::FNAME_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}
		document.setFileDescr(index.data().toString());

		/*
		 * First document must have dmFileMetaType set to
		 * FILEMETATYPE_MAIN. Remaining documents have
		 * FILEMETATYPE_ENCLOSURE.
		 */
		document.setFileMetaType((row == 0) ?
		    Isds::Type::FMT_MAIN : Isds::Type::FMT_ENCLOSURE);

		/*
		 * Since 2011 Mime Type can be empty and MIME type will
		 * be filled up on the ISDS server. It allows sending files
		 * with special mime types without recognition by application.
		 */
		index = m_attachModel.index(row, AttachmentTblModel::MIME_COL);
		{
			const QString mimeName(index.data().toString());
			if ((mimeName == QStringLiteral("application/xml")) ||
			    (mimeName == QStringLiteral("text/xml"))) {
				/*
				 * When sending XML requests to the eGov portals
				 * then the robots handling the requests are
				 * likely to discard them if the MIME type for
				 * the XML documents is not set.
				 */
				logDebugLv1NL("Setting '%s' mime type for document '%s'.",
				    mimeName.toUtf8().constData(),
				    document.fileDescr().toUtf8().constData());
				document.setMimeType(mimeName);
			} else {
				/* Must be empty non-null string. */
				document.setMimeType(QStringLiteral(""));
			}
		}

		index = m_attachModel.index(row,
		    AttachmentTblModel::BINARY_CONTENT_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}
		document.setBinaryContent(
		    index.data(Qt::DisplayRole).toByteArray());

		documents.append(document);
	}

	return true;
}

void DlgSendMessage::sendMessageISDS(
    const QList<BoxContactsModel::PartialEntry> &recipEntries)
{
	QString infoText;

	/* List of unique identifiers. */
	QList<QString> taskIdentifiers;
	const QDateTime currentTime(QDateTime::currentDateTimeUtc());

	int pdzCnt = 0; /* Number of paid messages. */

	/* Compute number of messages which the sender has to pay for. */
	foreach (const BoxContactsModel::PartialEntry &e, recipEntries) {
		if (e.pdz) {
			++pdzCnt;
		}
	}

	if (pdzCnt > 0) {
		if (m_dmType == QStringLiteral(DMTYPE_INIT)) {
			if (!m_ui->payRecipient->isChecked()) {
				if (QMessageBox::No == notifyOfPDZ(pdzCnt)) {
					return;
				}
			}
		} else {
			if (QMessageBox::No == notifyOfPDZ(pdzCnt)) {
				return;
			}
		}
	}

	QList<Isds::Document> documents;
	Isds::Envelope envelope;
	Isds::Message message;

	/* Attach envelope and attachment files to message structure. */
	if (!buildDocuments(documents)) {
		infoText = tr("An error occurred while loading attachments into message.");
		goto finish;
	}
	message.setDocuments(documents);

	if (!buildEnvelope(envelope)) {
		infoText = tr("An error occurred during message envelope creation.");
		goto finish;
	}

	this->setCursor(Qt::WaitCursor);
	this->setEnabled(false);

	/*
	 * Generate unique identifiers.
	 * These must be complete before creating first task.
	 */
	foreach (const BoxContactsModel::PartialEntry &e, recipEntries) {
		taskIdentifiers.append(m_userName + "_" + e.id + "_" +
		    currentTime.toString() + "_" +
		    Utility::generateRandomString(6));
	}
	m_transactIds = taskIdentifiers.toSet();
	m_sentMsgResultList.clear();

	/* Send message to all recipients. */
	for (int i = 0; i < recipEntries.size(); ++i) {
		const BoxContactsModel::PartialEntry &e(recipEntries.at(i));

		/* Set new recipient. */
		envelope.setDbIDRecipient(e.id);
		envelope.setDmToHands(m_ui->dmToHands->text());
		message.setEnvelope(envelope);

		int processFlags = Task::PROC_NOTHING;
		if (m_ui->immDownloadCheckBox->checkState() == Qt::Checked) {
			processFlags |= Task::PROC_IMM_DOWNLOAD;
		}
		if (m_ui->immRecMgmtUploadCheckBox->checkState() == Qt::Checked) {
			processFlags |= Task::PROC_IMM_RM_UPLOAD;
		}
		TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
		    m_userName, m_dbSet, taskIdentifiers.at(i), message,
		    e.name, e.address, e.pdz, processFlags);
		if (task != Q_NULLPTR) {
			task->setAutoDelete(true);
			GlobInstcs::workPoolPtr->assignHi(task);
		}
	}

	return;

finish:
	infoText += "\n\n" + tr("The message will be discarded.");
	DlgMsgBox::message(this, QMessageBox::Critical, tr("Send message error"),
	    tr("It has not been possible to send a message to the ISDS server."),
	    infoText, QString(), QMessageBox::Ok);
	this->close();
}

void DlgSendMessage::setIcons(void)
{
	{
		QIcon ico;
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "plus_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "plus_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		m_ui->addRecipButton->setIcon(ico);
		m_ui->addAttachButton->setIcon(ico);
	}

	{
		QIcon ico;
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "delete_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "delete_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		m_ui->removeRecipButton->setIcon(ico);
		m_ui->removeAttachButton->setIcon(ico);
	}

	{
		QIcon ico;
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "search_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "search_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		m_ui->findRecipButton->setIcon(ico);
	}

	{
		QIcon ico;
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "pencil_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "pencil_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		m_ui->enterBoxIdButton->setIcon(ico);
	}

	{
		QIcon ico;
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "folder_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "folder_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		m_ui->openAttachButton->setIcon(ico);
	}
}
