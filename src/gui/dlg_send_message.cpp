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

#include <algorithm> /* std::sort */
#include <QDateTime>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QMimeDatabase>

#include "src/gui/datovka.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_contacts.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_msg_box_informative.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_yes_no_checkbox.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/isds/isds_conversion.h"
#include "src/localisation/localisation.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"
#include "src/views/attachment_table_view.h"
#include "src/views/table_home_end_filter.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h"
#include "src/worker/task_download_credit_info.h"
#include "src/worker/task_keep_alive.h"
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

const QString &dzPrefix(const MessageDb *messageDb, qint64 dmId)
{
	const static QString unspecified(QLatin1String("DZ"));
	const static QString received(QLatin1String("DDZ"));
	const static QString sent(QLatin1String("ODZ"));

	if (Q_UNLIKELY(Q_NULLPTR == messageDb || dmId < 0)) {
		return unspecified;
	}

	switch (messageDb->msgMessageType(dmId)) {
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
    const QString &userName, class MainWindow *mw, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgSendMessage),
    m_keepAliveTimer(),
    m_messageDbSetList(messageDbSetList),
    m_userName(userName),
    m_dbId(),
    m_senderName(),
    m_dbType(),
    m_dbEffectiveOVM(false),
    m_dbOpenAddressing(false),
    m_isLogged(false),
    m_lastAttAddPath(),
    m_pdzCredit("0"),
    m_dmType(),
    m_dmSenderRefNumber(),
    m_dbSet(Q_NULLPTR),
    m_recipientTableModel(this),
    m_attachmentModel(this),
    m_transactIds(),
    m_sentMsgResultList(),
    m_mw(mw)
{
	m_ui->setupUi(this);

	/* Set default line height for table views/widgets. */
	m_ui->recipientTableView->setNarrowedLineHeight();
	m_ui->recipientTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	m_ui->attachmentTableView->setNarrowedLineHeight();
	m_ui->attachmentTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	initContent(action, msgIds);

	Q_ASSERT(!m_dbId.isEmpty());

	Q_ASSERT(Q_NULLPTR != m_dbSet);
}

DlgSendMessage::~DlgSendMessage(void)
{
	delete m_ui;
}

void DlgSendMessage::checkInputFields(void)
{
	bool enable = calculateAndShowTotalAttachSize() &&
	    !m_ui->subjectLine->text().isEmpty() &&
	    (m_recipientTableModel.rowCount() > 0) &&
	    (m_attachmentModel.rowCount() > 0);

	if (m_ui->payReplyCheckBox->isChecked()) {
		if (m_ui->dmSenderRefNumber->text().isEmpty()) {
			/*
			 * Transfer charges for reply can only be set when
			 * sender reference number is provided.
			 */
			enable = false;
		}
	}

	if (m_isLogged) {
		m_ui->sendButton->setEnabled(enable);
	} else {
		/* cannot send message when not logged in. */
		m_ui->sendButton->setEnabled(false);
	}
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

	if (dbID.isEmpty() || dbID.length() != 7) {
		QMessageBox::critical(this, tr("Wrong data box ID"),
		    tr("Wrong data box ID '%1'!").arg(dbID),
		    QMessageBox::Ok, QMessageBox::Ok);
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
	    (m_ui->recipientTableView->selectionModel()->selectedRows(0).size() > 0));
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
	removeSelectedEntries(m_ui->recipientTableView, &m_recipientTableModel);
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
		if (!globPref.use_global_paths) {
			m_lastAttAddPath = dialog.directory().absolutePath();
			emit usedAttachmentPath(m_userName, m_lastAttAddPath);
		}
	}

	foreach (const QString &fileName, fileNames) {
		int fileSize = m_attachmentModel.insertAttachmentFile(fileName,
		    m_attachmentModel.rowCount());
		if (fileSize <= 0) {
			/* TODO -- Generate some warning message. */
			continue;
		}
	}
}

void DlgSendMessage::attachmentSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	int selectionSize =
	    m_ui->attachmentTableView->selectionModel()->selectedRows(0).size();

	m_ui->removeAttachButton->setEnabled(selectionSize > 0);
	m_ui->openAttachButton->setEnabled(selectionSize == 1);
}

void DlgSendMessage::deleteSelectedAttachmentFiles(void)
{
	removeSelectedEntries(m_ui->attachmentTableView, &m_attachmentModel);
}

void DlgSendMessage::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	AttachmentInteraction::openAttachment(this, *m_ui->attachmentTableView,
	    index);
}

void DlgSendMessage::pingIsdsServer(void) const
{
	TaskKeepAlive *task = new (std::nothrow) TaskKeepAlive(m_userName);
	task->setAutoDelete(true);
	globWorkPool.assignHi(task);
}

void DlgSendMessage::setAccountInfo(int fromComboIdx)
{
	debugSlotCall();

	/* Get user name for selected account. */
	const QString userName(
	    m_ui->fromComboBox->itemData(fromComboIdx).toString());
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	/* Remove all recipients if account was changed. */
	if (m_userName != userName) {
		m_recipientTableModel.removeRows(0,
		    m_recipientTableModel.rowCount());
		m_userName = userName;
	}

	m_isLogged = true;
	m_keepAliveTimer.stop();
	{
		TaskKeepAlive *task =
		    new (std::nothrow) TaskKeepAlive(m_userName);
		task->setAutoDelete(false);
		globWorkPool.runSingle(task);

		m_isLogged = task->m_isAlive;

		delete task;
	}
	if (!m_isLogged) {
		if (Q_NULLPTR != m_mw) {
			m_isLogged = m_mw->connectToIsds(m_userName);
		}
	}
	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	/* Check for presence of struct isds_ctx . */
	if (NULL == globIsdsSessions.session(m_userName)) {
		logErrorNL("%s", "Missing ISDS session.");
		m_isLogged = false;
	}

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		if (acnt.userName == m_userName) {
			m_dbSet = acnt.messageDbSet;
			break;
		}
	}

	const AcntSettings &accountInfo(globAccounts[m_userName]);
	const QString acntDbKey(AccountDb::keyFromLogin(m_userName));
	m_dbId = globAccountDbPtr->dbId(acntDbKey);
	Q_ASSERT(!m_dbId.isEmpty());
	m_senderName = globAccountDbPtr->senderNameGuess(acntDbKey);
	const QList<QString> accountData(
	    globAccountDbPtr->getUserDataboxInfo(acntDbKey));
	if (!accountData.isEmpty()) {
		m_dbType = accountData.at(0);
		m_dbEffectiveOVM = (accountData.at(1) == "1");
		m_dbOpenAddressing = (accountData.at(2) == "1");
	}
	if (globPref.use_global_paths) {
		m_lastAttAddPath = globPref.add_file_to_attachments_path;
	} else {
		m_lastAttAddPath = accountInfo.lastAttachAddPath();
	}

	if (m_dbOpenAddressing) {
		m_pdzCredit = getPDZCreditFromISDS(m_userName, m_dbId);
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
	    globAccounts[m_userName].accountName() + "</strong>" +
	    " (" + m_userName + ") - " + m_dbType + dbOpenAddressingText);
}

void DlgSendMessage::sendMessage(void)
{
	debugSlotCall();

	const QList<BoxContactsModel::PartialEntry> recipEntries(
	    m_recipientTableModel.partialBoxEntries(BoxContactsModel::ANY));


	sendMessageISDS(recipEntries);
}

void DlgSendMessage::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId)
{
	debugSlotCall();

	Q_UNUSED(userName);

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

	if (m_recipientTableModel.rowCount() == successfullySentCnt) {
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
	m_recipientTableModel.setHeader();
	m_ui->recipientTableView->setModel(&m_recipientTableModel);

	m_ui->recipientTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 60);
	m_ui->recipientTableView->setColumnWidth(BoxContactsModel::BOX_TYPE_COL, 70);
	m_ui->recipientTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 120);
	m_ui->recipientTableView->setColumnWidth(BoxContactsModel::ADDRESS_COL, 100);

	m_ui->recipientTableView->setColumnHidden(BoxContactsModel::CHECKBOX_COL, true);
	m_ui->recipientTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL, true);

	m_attachmentModel.setHeader();
	m_ui->attachmentTableView->setModel(&m_attachmentModel);

	m_ui->attachmentTableView->setColumnWidth(DbFlsTblModel::FNAME_COL, 150);
	m_ui->attachmentTableView->setColumnWidth(DbFlsTblModel::MIME_COL, 120);

	m_ui->attachmentTableView->setColumnHidden(DbFlsTblModel::ATTACHID_COL, true);
	m_ui->attachmentTableView->setColumnHidden(DbFlsTblModel::MSGID_COL, true);
	m_ui->attachmentTableView->setColumnHidden(DbFlsTblModel::CONTENT_COL, true);

	/* Enable drag and drop on attachment table. */
	m_ui->attachmentTableView->setAcceptDrops(true);
	m_ui->attachmentTableView->setDragEnabled(true);
	m_ui->attachmentTableView->setDragDropOverwriteMode(false);
	m_ui->attachmentTableView->setDropIndicatorShown(true);
	m_ui->attachmentTableView->setDragDropMode(QAbstractItemView::DragDrop);
	m_ui->attachmentTableView->setDefaultDropAction(Qt::CopyAction);

	m_ui->prepaidReplyLabel->setEnabled(false);
	m_ui->prepaidReplyLabel->hide();

	Q_ASSERT(!m_userName.isEmpty());

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		const QString accountName =
		    globAccounts[acnt.userName].accountName() +
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

	connect(&m_recipientTableModel,
	    SIGNAL(rowsInserted(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_recipientTableModel,
	    SIGNAL(rowsRemoved(QModelIndex, int, int)),
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

	connect(m_ui->recipientTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(recipientSelectionChanged(QItemSelection, QItemSelection)));

	connect(m_ui->attachmentTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(openSelectedAttachment(QModelIndex)));

	connect(m_ui->subjectLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(&m_attachmentModel,
	    SIGNAL(rowsInserted(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_attachmentModel,
	    SIGNAL(rowsRemoved(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_attachmentModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->attachmentTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(attachmentSelectionChanged(QItemSelection, QItemSelection)));

	m_ui->recipientTableView->setEditTriggers(
	    QAbstractItemView::NoEditTriggers);
	m_ui->attachmentTableView->setEditTriggers(
	    QAbstractItemView::NoEditTriggers);

	m_ui->recipientTableView->installEventFilter(
	    new TableHomeEndFilter(this));
	m_ui->attachmentTableView->installEventFilter(
	    new TableHomeEndFilter(this));

	connect(m_ui->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64)));

	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	connect(&m_keepAliveTimer, SIGNAL(timeout()), this, SLOT(pingIsdsServer()));

	m_ui->attachmentSizeInfo->setText(
	    tr("Total size of attachments is %1 B").arg(0));

	if (IsdsConversion::boxTypeStrToInt(m_dbType) > DBTYPE_OVM_REQ) {
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
			MessageDb::PartialEnvelopeData envData(
			    messageDb->msgsReplyData(msgId.dmId));

			m_ui->subjectLine->setText("Fwd: " + envData.dmAnnotation);
		}

		QByteArray msgBase64(messageDb->msgsMessageBase64(msgId.dmId));
		if (msgBase64.isEmpty()) {
			continue;
		}

		m_attachmentModel.appendAttachmentEntry(msgBase64,
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

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(msgId.dmId);
	m_dmType = envData.dmType;
	m_dmSenderRefNumber = envData.dmRecipientRefNumber;

	m_ui->subjectLine->setText("Re: " + envData.dmAnnotation);

	if (!envData.dmSenderRefNumber.isEmpty()) {
		m_ui->dmRecipientRefNumber->setText(envData.dmSenderRefNumber);
		hideOptionalForm = false;
	}
	if (!envData.dmSenderIdent.isEmpty()) {
		m_ui->dmRecipientIdent->setText(envData.dmSenderIdent);
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientRefNumber.isEmpty()) {
		m_ui->dmSenderRefNumber->setText(envData.dmRecipientRefNumber);
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientIdent.isEmpty()) {
		m_ui->dmSenderIdent->setText(envData.dmRecipientIdent);
		hideOptionalForm = false;
	}

	m_ui->optionalForm->setHidden(hideOptionalForm);
	m_ui->optionalFormCheckBox->setChecked(!hideOptionalForm);
	m_ui->payRecipient->setEnabled(false);
	m_ui->payRecipient->hide();
	m_ui->payRecipient->setChecked(false);

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDSender);
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

	m_recipientTableModel.appendData(envData.dbIDSender, -1,
	    envData.dmSender, envData.dmSenderAddress, QString(), pdz);
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

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(msgId.dmId);
	m_dmType = envData.dmType;
	m_dmSenderRefNumber = envData.dmRecipientRefNumber;

	m_ui->subjectLine->setText(envData.dmAnnotation);

	/* Fill in optional fields.  */
	if (!envData.dmSenderRefNumber.isEmpty()) {
		m_ui->dmSenderRefNumber->setText(envData.dmSenderRefNumber);
		hideOptionalForm = false;
	}
	if (!envData.dmSenderIdent.isEmpty()) {
		m_ui->dmSenderIdent->setText(envData.dmSenderIdent);
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientRefNumber.isEmpty()) {
		m_ui->dmRecipientRefNumber->setText(envData.dmRecipientRefNumber);
		hideOptionalForm = false;
	}
	if (!envData.dmRecipientIdent.isEmpty()) {
		m_ui->dmRecipientIdent->setText(envData.dmRecipientIdent);
		hideOptionalForm = false;
	}
	if (!envData.dmToHands.isEmpty()) {
		m_ui->dmToHands->setText(envData.dmToHands);
		hideOptionalForm = false;
	}
	/* set check boxes */
	m_ui->dmPersonalDelivery->setChecked(envData.dmPersonalDelivery);
	m_ui->dmAllowSubstDelivery->setChecked(envData.dmAllowSubstDelivery);
	/* fill optional LegalTitle - Law, year, ... */
	if (!envData.dmLegalTitleLaw.isEmpty()) {
		m_ui->dmLegalTitleLaw->setText(envData.dmLegalTitleLaw);
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitleYear.isEmpty()) {
		m_ui->dmLegalTitleYear->setText(envData.dmLegalTitleYear);
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitleSect.isEmpty()) {
		m_ui->dmLegalTitleSect->setText(envData.dmLegalTitleSect);
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitlePar.isEmpty()) {
		m_ui->dmLegalTitlePar->setText(envData.dmLegalTitlePar);
		hideOptionalForm = false;
	}
	if (!envData.dmLegalTitlePoint.isEmpty()) {
		m_ui->dmLegalTitlePoint->setText(envData.dmLegalTitlePoint);
		hideOptionalForm = false;
	}

	m_ui->optionalForm->setHidden(hideOptionalForm);
	m_ui->optionalFormCheckBox->setChecked(!hideOptionalForm);

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDRecipient);
		m_ui->payReplyCheckBox->setEnabled(true);
		m_ui->payReplyCheckBox->show();
	} else {
		m_ui->payReplyCheckBox->setEnabled(false);
		m_ui->payReplyCheckBox->hide();
		pdz = false;
	}

	/* message is received -> recipient == sender */
	if (m_dbId != envData.dbIDRecipient) {
		m_recipientTableModel.appendData(envData.dbIDRecipient, -1,
		    envData.dmRecipient, envData.dmRecipientAddress, QString(),
		    pdz);
	}

	/* fill attachments from template message */
	QList<MessageDb::FileData> msgFileList =
	    messageDb->getFilesFromMessage(msgId.dmId);

	foreach (const MessageDb::FileData &fileData, msgFileList) {
		m_attachmentModel.appendAttachmentEntry(
		    fileData.dmEncodedContent, fileData.dmFileDescr);
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
	int aSize = m_attachmentModel.totalAttachmentSize();

	m_ui->attachmentSizeInfo->setStyleSheet("QLabel { color: black }");

	if (m_attachmentModel.rowCount() > MAX_ATTACHMENT_FILES) {
		m_ui->attachmentSizeInfo->
		     setStyleSheet("QLabel { color: red }");
		m_ui->attachmentSizeInfo->setText(tr(
		    "Warning: The permitted amount (%1) of attachments has been exceeded.")
		        .arg(QString::number(MAX_ATTACHMENT_FILES)));
		return false;
	}


	if (aSize > 0) {
		if (aSize >= 1024) {
			m_ui->attachmentSizeInfo->setText(
			    tr("Total size of attachments is ~%1 KB").
			    arg(aSize/1024));
			if (aSize >= MAX_ATTACHMENT_SIZE_BYTES) {
				m_ui->attachmentSizeInfo->
				     setStyleSheet("QLabel { color: red }");
				m_ui->attachmentSizeInfo->setText(
				    tr("Warning: Total size of attachments is larger than %1 MB!")
				    .arg(QString::number(
				        MAX_ATTACHMENT_SIZE_MB)));
				return false;
			}
		} else {
			m_ui->attachmentSizeInfo->setText(tr(
			    "Total size of attachments is ~%1 B").arg(aSize));
		}
	} else {
		m_ui->attachmentSizeInfo->setText(
		    tr("Total size of attachments is %1 B").arg(aSize));
	}

	return true;
}

void DlgSendMessage::addRecipientBox(const QString &boxId)
{
	/* Ignore existent entry. */
	if (boxId.isEmpty() || m_recipientTableModel.containsBoxId(boxId)) {
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
	TaskSearchOwnerFulltext *task =
	    new (std::nothrow) TaskSearchOwnerFulltext(m_userName, boxId,
	        TaskSearchOwnerFulltext::FT_BOX_ID,
	        TaskSearchOwnerFulltext::BT_ALL);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	enum TaskSearchOwnerFulltext::Result result = task->m_result;
	QString errMsg = task->m_isdsError;
	QString longErrMsg = task->m_isdsLongError;
	QList<TaskSearchOwnerFulltext::BoxEntry> foundBoxes(task->m_foundBoxes);
	delete task; task = Q_NULLPTR;

	QString name = tr("Unknown");
	QString address = tr("Unknown");
	QVariant pdz;
	int boxType = -1; /* Unknown type. */

	if (foundBoxes.size() == 1) {
		const TaskSearchOwnerFulltext::BoxEntry &entry(
		    foundBoxes.first());

		name = entry.name;
		address = entry.address;
		boxType = entry.type;
		if (!entry.active) {
			DlgMsgBox::message(this, QMessageBox::Warning,
			    tr("Data box is not active"),
			    tr("Recipient with data box ID '%1' does not have active data box.")
			        .arg(boxId),
			    tr("The message cannot be delivered."), QString(),
			    QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		if (entry.publicSending) {
			pdz = false;
		} else if (entry.commercialSending) {
			pdz = true;
		} else if (entry.effectiveOVM) {
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

	m_recipientTableModel.appendData(boxId, boxType, name, address,
	    QString(), pdz);
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
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

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

	TaskSearchOwner::SoughtOwnerInfo soughtInfo(
	    boxId, TaskSearchOwner::BT_FO, QString(), QString(), QString(),
	    QString(), QString());

	TaskSearchOwner *task =
	    new (std::nothrow) TaskSearchOwner(userName, soughtInfo);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	QList<TaskSearchOwner::BoxEntry> foundBoxes(task->m_foundBoxes);

	delete task; task = Q_NULLPTR;

	if (foundBoxes.size() == 1) {
		const TaskSearchOwner::BoxEntry &entry(foundBoxes.first());

		ret = entry.effectiveOVM;
	}

	return ret;
}

bool DlgSendMessage::buildEnvelope(IsdsEnvelope &envelope) const
{
	QString dmType;

	/* Set mandatory fields of envelope. */
	envelope.dmID.clear();
	envelope.dmAnnotation = m_ui->subjectLine->text();

	/* Set optional fields. */
	envelope.dmSenderIdent = m_ui->dmSenderIdent->text();
	envelope.dmRecipientIdent = m_ui->dmRecipientIdent->text();
	envelope.dmSenderRefNumber = m_ui->dmSenderRefNumber->text();
	envelope.dmRecipientRefNumber = m_ui->dmRecipientRefNumber->text();
	envelope._using_dmLegalTitleLaw =
	    !m_ui->dmLegalTitleLaw->text().isEmpty();
	if (envelope._using_dmLegalTitleLaw) {
		envelope.dmLegalTitleLaw =
		    m_ui->dmLegalTitleLaw->text().toLong();
	}
	envelope._using_dmLegalTitleYear =
	    !m_ui->dmLegalTitleYear->text().isEmpty();
	if (envelope._using_dmLegalTitleYear) {
		envelope.dmLegalTitleYear =
		    m_ui->dmLegalTitleYear->text().toLong();
	}
	envelope.dmLegalTitleSect = m_ui->dmLegalTitleSect->text();
	envelope.dmLegalTitlePar = m_ui->dmLegalTitlePar->text();
	envelope.dmLegalTitlePoint = m_ui->dmLegalTitlePoint->text();
	envelope.dmPersonalDelivery = m_ui->dmPersonalDelivery->isChecked();

	/* Only OVM can change. */
	if (IsdsConversion::boxTypeStrToInt(m_dbType) > DBTYPE_OVM_REQ) {
		envelope.dmAllowSubstDelivery = true;
	} else {
		envelope.dmAllowSubstDelivery =
		    m_ui->dmAllowSubstDelivery->isChecked();
	}

	if (m_dmType == QStringLiteral(DMTYPE_INIT)) {
		if (m_ui->payRecipient->isChecked()) {
			dmType = QStringLiteral(DMTYPE_RESP);
		} else {
			dmType = QStringLiteral(DMTYPE_COMM);
		}
		if (!m_dmSenderRefNumber.isEmpty()) {
			envelope.dmRecipientRefNumber = m_dmSenderRefNumber;
		}
	} else {
		if (m_ui->payReplyCheckBox->isChecked()) {
			dmType = QStringLiteral(DMTYPE_INIT);
		}
	}

	envelope.dmType = dmType;

	envelope.dmOVM = m_dbEffectiveOVM;

	envelope.dmPublishOwnID = m_ui->dmPublishOwnID->isChecked();

	return true;
}

bool DlgSendMessage::buildDocuments(QList<IsdsDocument> &documents) const
{
	/* Load attachments. */
	for (int row = 0; row < m_attachmentModel.rowCount(); ++row) {
		IsdsDocument document;
		QModelIndex index;

		document.isXml = false;

		index = m_attachmentModel.index(row, DbFlsTblModel::FNAME_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}
		document.dmFileDescr = index.data().toString();

		/*
		 * First document must have dmFileMetaType set to
		 * FILEMETATYPE_MAIN. Remaining documents have
		 * FILEMETATYPE_ENCLOSURE.
		 */

		/*
		 * Since 2011 Mime Type can be empty and MIME type will
		 * be filled up on the ISDS server. It allows sending files
		 * with special mime types without recognition by application.
		 */
		document.dmMimeType = QStringLiteral("");

		index =
		    m_attachmentModel.index(row, DbFlsTblModel::CONTENT_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}
		document.data = QByteArray::fromBase64(
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

	IsdsMessage message;

	/* Attach envelope and attachment files to message structure. */
	if (!buildDocuments(message.documents)) {
		infoText = tr("An error occurred while loading attachments into message.");
		goto finish;
	}
	if (!buildEnvelope(message.envelope)) {
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
		    DlgChangePwd::generateRandomString(6));
	}
	m_transactIds = taskIdentifiers.toSet();
	m_sentMsgResultList.clear();

	/* Send message to all recipients. */
	for (int i = 0; i < recipEntries.size(); ++i) {
		const BoxContactsModel::PartialEntry &e(recipEntries.at(i));

		/* Clear fields. */
		message.envelope.dmID.clear();

		/* Set new recipient. */
		message.envelope.dbIDRecipient = e.id;
		message.envelope.dmToHands = m_ui->dmToHands->text();

		TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
		    m_userName, m_dbSet, taskIdentifiers.at(i), message,
		    e.name, e.address, e.pdz);
		task->setAutoDelete(true);
		globWorkPool.assignHi(task);
	}

	return;

finish:
	infoText += "\n\n" + tr("The message will be discarded.");
	DlgMsgBox::message(this, QMessageBox::Critical, tr("Send message error"),
	    tr("It has not been possible to send a message to the ISDS server."),
	    infoText, QString(), QMessageBox::Ok);
	this->close();
}
