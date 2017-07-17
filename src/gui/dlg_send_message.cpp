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
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QMimeDatabase>

#include "src/gui/datovka.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_contacts.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_yes_no_checkbox.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/models/accounts_model.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/wd_sessions.h"
#include "src/io/message_db.h"
#include "src/isds/isds_conversion.h"
#include "src/localisation/localisation.h"
#include "src/log/log.h"
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
#include "src/worker/task_send_message_mojeid.h"
#include "src/worker/task_search_owner.h"
#include "ui_dlg_send_message.h"

/*
 * Message types as defined in ISDS alnd used in libisds.
 * For details see linisds.h .
 */
#define DMTYPE_INIT "I"
#define DMTYPE_COMM "K"
#define DMTYPE_RESP "O"

const QString &dzPrefix(MessageDb *messageDb, qint64 dmId)
{
	const static QString nothing;
	const static QString received(QLatin1String("D"));
	const static QString sent(QLatin1String("O"));

	if (Q_NULLPTR == messageDb || dmId < 0) {
		return nothing;
	}

	switch (messageDb->msgMessageType(dmId)) {
	case MessageDb::TYPE_RECEIVED:
		return received;
		break;
	case MessageDb::TYPE_SENT:
		return sent;
		break;
	default:
		return nothing;
		break;
	}
}

DlgSendMessage::DlgSendMessage(
    const QList<Task::AccountDescr> &messageDbSetList,
    enum Action action, const QList<MessageDb::MsgId> &msgIds,
    const QString &userName, class MainWindow *mw, QWidget *parent)
    : QDialog(parent),
    m_keepAliveTimer(),
    m_messageDbSetList(messageDbSetList),
    m_userName(userName),
    m_dbId(),
    m_senderName(),
    m_dbType(),
    m_dbEffectiveOVM(false),
    m_dbOpenAddressing(false),
    m_isLogged(false),
    m_isWebDatovkaAccount(false),
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
	setupUi(this);

	/* Set default line height for table views/widgets. */
	recipientTableView->setNarrowedLineHeight();
	recipientTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	attachmentTableView->setNarrowedLineHeight();
	attachmentTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	initContent(action, msgIds);

	Q_ASSERT(!m_dbId.isEmpty());

	Q_ASSERT(Q_NULLPTR != m_dbSet);
}

void DlgSendMessage::checkInputFields(void)
{
	bool buttonEnabled = calculateAndShowTotalAttachSize() &&
	    !this->subjectText->text().isEmpty() &&
	    (m_recipientTableModel.rowCount() > 0) &&
	    (m_attachmentModel.rowCount() > 0);

	if (this->payReplyCheckBox->isChecked()) {
		if (this->dmSenderRefNumber->text().isEmpty()) {
			buttonEnabled = false;
		}
	}

	if (m_isLogged) {
		this->sendButton->setEnabled(buttonEnabled);
	} else {
		this->sendButton->setEnabled(false);
	}
}

void DlgSendMessage::addRecipientFromLocalContact(void)
{
	QStringList dbIDs;
	QDialog *dlgCont = new DlgContacts(*m_dbSet, m_dbId, &dbIDs, this);
	dlgCont->exec();
	dlgCont->deleteLater();
	addRecipientBoxes(dbIDs);
}

void DlgSendMessage::addRecipientFromISDSSearch(void)
{
	QStringList dbIDs;
	QDialog *dsSearch = new DlgDsSearch(m_userName, m_dbType,
		    m_dbEffectiveOVM, m_dbOpenAddressing, &dbIDs, this);
	dsSearch->exec();
	dsSearch->deleteLater();
	addRecipientBoxes(dbIDs);
}

void DlgSendMessage::addRecipientManually(void)
{
	bool ok = false;

	QString dbID = QInputDialog::getText(this, tr("Databox ID"),
	    tr("Enter Databox ID (7 characters):"), QLineEdit::Normal,
	    NULL, &ok, Qt::WindowStaysOnTopHint);

	if (!ok) {
		return;
	}

	if (dbID.isEmpty() || dbID.length() != 7) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle(tr("Wrong data box ID"));
		msgBox.setText(tr("Wrong data box ID '%1'!").arg(dbID));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
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
	removeRecipient->setEnabled((m_dmType != QStringLiteral(DMTYPE_INIT)) &&
	    recipientTableView->selectionModel()->selectedRows(0).size() > 0);
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
	if ((view == Q_NULLPTR) || (model == Q_NULLPTR)) {
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
	removeSelectedEntries(recipientTableView, &m_recipientTableModel);
}

void DlgSendMessage::showOptionalForm(void)
{
	this->optionalWidget->setHidden(
	    (this->optionalFieldCheckBox->checkState() == Qt::Unchecked) &&
	    (this->payReplyCheckBox->checkState() == Qt::Unchecked));

	checkInputFields();

	if (this->payReplyCheckBox->checkState() == Qt::Unchecked) {
		this->labeldmSenderRefNumber->setStyleSheet(
		    "QLabel { color: black }");
		this->labeldmSenderRefNumber->setText(
		    tr("Our reference number:"));
		disconnect(this->dmSenderRefNumber,SIGNAL(textChanged(QString)),
		    this, SLOT(checkInputFields()));
	} else {
		this->labeldmSenderRefNumber->setStyleSheet(
		    "QLabel { color: red }");
		this->labeldmSenderRefNumber->setText(
		    tr("Enter reference number:"));
		this->dmSenderRefNumber->setFocus();
		connect(this->dmSenderRefNumber, SIGNAL(textChanged(QString)),
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
	    attachmentTableView->selectionModel()->selectedRows(0).size();

	removeAttachment->setEnabled(selectionSize > 0);
	openAttachment->setEnabled(selectionSize == 1);
}

void DlgSendMessage::deleteSelectedAttachmentFiles(void)
{
	removeSelectedEntries(attachmentTableView, &m_attachmentModel);
}

void DlgSendMessage::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	AttachmentInteraction::openAttachment(this, *this->attachmentTableView,
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
	    this->fromComboBox->itemData(fromComboIdx).toString());
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

	if (isWebDatovkaAccount(m_userName)) {
		m_isWebDatovkaAccount = true;
	}

	if (!m_isWebDatovkaAccount) {

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
	} else {
		if (!wdSessions.isConnectedToWebdatovka(m_userName)) {
			if (Q_NULLPTR != m_mw) {
				m_mw->loginToMojeId(m_userName);
			}
		}
		m_isLogged = true;
	}

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		if (acnt.userName == m_userName) {
			m_dbSet = acnt.messageDbSet;
			break;
		}
	}

	const AcntSettings &accountInfo(AccountModel::globAccounts[m_userName]);
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

	if (!m_isWebDatovkaAccount) {
		if (m_dbOpenAddressing) {
			m_pdzCredit = getPDZCreditFromISDS(m_userName, m_dbId);
		}
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

	this->fromUser->setText("<strong>" +
	    AccountModel::globAccounts[m_userName].accountName() +
	    "</strong>" + " (" + m_userName + ") - " + m_dbType +
	    dbOpenAddressingText);
}

void DlgSendMessage::sendMessage(void)
{
	debugSlotCall();

	const QList<BoxContactsModel::PartialEntry> recipEntries(
	    m_recipientTableModel.partialBoxEntries(BoxContactsModel::ANY));

	if (!m_isWebDatovkaAccount) {
		sendMessageISDS(recipEntries);
	} else {
		sendMessageWebDatovka(recipEntries);
	}
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
	QString detailText;

	foreach (const TaskSendMessage::ResultData &resultData,
	         m_sentMsgResultList) {
		if (TaskSendMessage::SM_SUCCESS == resultData.result) {
			++successfullySentCnt;

			if (resultData.isPDZ) {
				detailText += tr(
				    "Message was successfully sent to "
				    "<i>%1 (%2)</i> as PDZ with number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			} else {
				detailText += tr(
				    "Message was successfully sent to "
				    "<i>%1 (%2)</i> as message number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			}
		} else {
			detailText += tr("Message was NOT sent to "
			    "<i>%1 (%2)</i>. Server says: %3").
			    arg(resultData.recipientName).
			    arg(resultData.dbIDRecipient).
			    arg(resultData.errInfo) + "<br/>";
		}
	}
	m_sentMsgResultList.clear();

	if (m_recipientTableModel.rowCount() == successfullySentCnt) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Message sent"));
		msgBox.setText("<b>" +
		    tr("Message was successfully sent to all recipients.") +
		    "</b>");
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		this->accept(); /* Set return code to accepted. */
	} else {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Message sending error"));
		msgBox.setText("<b>" +
		    tr("Message was NOT sent to all recipients.") + "</b>");
		detailText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") + "</b>";
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if (msgBox.exec() == QMessageBox::Yes) {
			this->close(); /* Set return code to closed. */
		}
	}

	emit usedAttachmentPath(m_userName, m_lastAttAddPath);
}

void DlgSendMessage::collectSendMessageStatusWebDatovka(const QString &userName,
    const QStringList &results, const QString &error)
{
	debugSlotCall();

	Q_UNUSED(error);

	QString detailText;

	if (results.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Message sent"));
		msgBox.setText("<b>" +
		    tr("Message was successfully sent to all recipients.") +
		    "</b>");
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		this->accept(); /* Set return code to accepted. */
		emit usedAttachmentPath(userName, m_lastAttAddPath);
	} else {
		for (int i = 0; i < results.count(); ++i) {
			QString msg = results.at(i);
			detailText += msg.replace("§", ": ");
		}
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Message sending error"));
		msgBox.setText("<b>" +
		    tr("Message was NOT sent to all recipients.") + "</b>");
		detailText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") + "</b>";
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if (msgBox.exec() == QMessageBox::Yes) {
			this->close(); /* Set return code to closed. */
			emit usedAttachmentPath(userName, m_lastAttAddPath);
		}
	}
}

void DlgSendMessage::initContent(enum Action action,
    const QList<MessageDb::MsgId> &msgIds)
{
	if (isWebDatovkaAccount(m_userName)) {
		m_isWebDatovkaAccount = true;
	}

	m_recipientTableModel.setHeader();
	this->recipientTableView->setModel(&m_recipientTableModel);

	this->recipientTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 60);
	this->recipientTableView->setColumnWidth(BoxContactsModel::BOX_TYPE_COL, 70);
	this->recipientTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 120);
	this->recipientTableView->setColumnWidth(BoxContactsModel::ADDRESS_COL, 100);

	this->recipientTableView->setColumnHidden(BoxContactsModel::CHECKBOX_COL, true);
	this->recipientTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL, true);

	m_attachmentModel.setHeader();
	this->attachmentTableView->setModel(&m_attachmentModel);

	this->attachmentTableView->setColumnWidth(DbFlsTblModel::FNAME_COL, 150);
	this->attachmentTableView->setColumnWidth(DbFlsTblModel::MIME_COL, 120);

	this->attachmentTableView->setColumnHidden(DbFlsTblModel::ATTACHID_COL, true);
	this->attachmentTableView->setColumnHidden(DbFlsTblModel::MSGID_COL, true);
	this->attachmentTableView->setColumnHidden(DbFlsTblModel::CONTENT_COL, true);

	/* Enable drag and drop on attachment table. */
	this->attachmentTableView->setAcceptDrops(true);
	this->attachmentTableView->setDragEnabled(true);
	this->attachmentTableView->setDragDropOverwriteMode(false);
	this->attachmentTableView->setDropIndicatorShown(true);
	this->attachmentTableView->setDragDropMode(QAbstractItemView::DragDrop);
	this->attachmentTableView->setDefaultDropAction(Qt::CopyAction);

	this->replyLabel->hide();
	this->replyLabel->setEnabled(false);

	Q_ASSERT(!m_userName.isEmpty());

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		const QString accountName =
		    AccountModel::globAccounts[acnt.userName].accountName() +
		    " (" + acnt.userName + ")";
		this->fromComboBox->addItem(accountName, QVariant(acnt.userName));
		if (m_userName == acnt.userName) {
			int i = this->fromComboBox->count() - 1;
			Q_ASSERT(0 <= i);
			this->fromComboBox->setCurrentIndex(i);
			setAccountInfo(i);
		}
	}

	connect(this->fromComboBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(setAccountInfo(int)));

	connect(&m_recipientTableModel,
	    SIGNAL(rowsInserted(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_recipientTableModel,
	    SIGNAL(rowsRemoved(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));

	this->optionalWidget->setHidden(true);

	connect(this->optionalFieldCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(showOptionalForm()));
	connect(this->payReplyCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(showOptionalForm()));

	connect(this->addRecipient, SIGNAL(clicked()),
	    this, SLOT(addRecipientFromLocalContact()));
	connect(this->removeRecipient, SIGNAL(clicked()),
	    this, SLOT(deleteRecipientEntries()));
	connect(this->findRecipient, SIGNAL(clicked()),
	    this, SLOT(addRecipientFromISDSSearch()));
	connect(this->enterDbIdpushButton, SIGNAL(clicked()),
	    this, SLOT(addRecipientManually()));

	connect(this->addAttachment, SIGNAL(clicked()), this,
	    SLOT(addAttachmentFile()));
	connect(this->removeAttachment, SIGNAL(clicked()), this,
	    SLOT(deleteSelectedAttachmentFiles()));
	connect(this->openAttachment, SIGNAL(clicked()), this,
	    SLOT(openSelectedAttachment()));

	connect(this->recipientTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(recipientSelectionChanged(QItemSelection, QItemSelection)));

	connect(this->attachmentTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(openSelectedAttachment(QModelIndex)));

	connect(this->subjectText, SIGNAL(textChanged(QString)),
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
	connect(this->attachmentTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(attachmentSelectionChanged(QItemSelection, QItemSelection)));

	this->recipientTableView->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->attachmentTableView->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->recipientTableView->installEventFilter(
	    new TableHomeEndFilter(this));
	this->attachmentTableView->installEventFilter(
	    new TableHomeEndFilter(this));

	connect(this->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	connect(this->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64)));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageMojeIdFinished(QString, QStringList, QString)),
	    this,
	    SLOT(collectSendMessageStatusWebDatovka(QString, QStringList,
	        QString)));

	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	if (!m_isWebDatovkaAccount) {
		connect(&m_keepAliveTimer, SIGNAL(timeout()), this,
		    SLOT(pingIsdsServer()));
	}

	this->attachmentSizeInfo->setText(
	    tr("Total size of attachments is %1 B").arg(0));

	if (IsdsConversion::boxTypeStrToInt(m_dbType) > DBTYPE_OVM_REQ) {
		this->dmAllowSubstDelivery->setEnabled(false);
		this->dmAllowSubstDelivery->hide();
	}

	if (ACT_REPLY == action) {
		fillContentAsReply(msgIds);
	} else {
		if (m_dbOpenAddressing) {
			this->payReplyCheckBox->setEnabled(true);
			this->payReplyCheckBox->show();
		} else {
			this->payReplyCheckBox->setEnabled(false);
			this->payReplyCheckBox->hide();
		}

		this->payRecipient->setEnabled(false);
		this->payRecipient->hide();
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

			this->subjectText->setText("Fwd: " + envData.dmAnnotation);
		}

		QByteArray msgBase64(messageDb->msgsMessageBase64(msgId.dmId));
		if (msgBase64.isEmpty()) {
			continue;
		}

		m_attachmentModel.appendAttachmentEntry(msgBase64,
		    dzPrefix(messageDb, msgId.dmId) + QString("DZ_%1.zfo").arg(msgId.dmId));
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

	bool hideOptionalWidget = true;

	this->fromComboBox->setEnabled(false);

	MessageDb *messageDb =
	    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
	Q_ASSERT(Q_NULLPTR != messageDb);

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(msgId.dmId);
	m_dmType = envData.dmType;
	m_dmSenderRefNumber = envData.dmRecipientRefNumber;

	this->subjectText->setText("Re: " + envData.dmAnnotation);

	if (!envData.dmSenderRefNumber.isEmpty()) {
		this->dmRecipientRefNumber->setText(envData.dmSenderRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmSenderIdent.isEmpty()) {
		this->dmRecipientIdent->setText(envData.dmSenderIdent);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientRefNumber.isEmpty()) {
		this->dmSenderRefNumber->setText(envData.dmRecipientRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientIdent.isEmpty()) {
		this->dmSenderIdent->setText(envData.dmRecipientIdent);
		hideOptionalWidget = false;
	}

	this->optionalWidget->setHidden(hideOptionalWidget);
	this->optionalFieldCheckBox->setChecked(!hideOptionalWidget);
	this->payRecipient->setEnabled(false);
	this->payRecipient->setChecked(false);
	this->payRecipient->hide();

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDSender);
		this->payReplyCheckBox->show();
		this->payReplyCheckBox->setEnabled(true);
	} else {
		this->payReplyCheckBox->setEnabled(false);
		this->payReplyCheckBox->hide();
		pdz = false;
	}

	if (m_dmType == QStringLiteral(DMTYPE_INIT)) {
		this->addRecipient->setEnabled(false);
		this->removeRecipient->setEnabled(false);
		this->findRecipient->setEnabled(false);
		this->replyLabel->show();
		this->replyLabel->setEnabled(true);
		this->payReplyCheckBox->hide();
		this->payReplyCheckBox->setEnabled(false);
		this->payRecipient->setEnabled(true);
		this->payRecipient->setChecked(true);
		this->payRecipient->show();
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

	bool hideOptionalWidget = true;

	MessageDb *messageDb =
	    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
	Q_ASSERT(Q_NULLPTR != messageDb);

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(msgId.dmId);
	m_dmType = envData.dmType;
	m_dmSenderRefNumber = envData.dmRecipientRefNumber;

	this->subjectText->setText(envData.dmAnnotation);

	/* Fill in optional fields.  */
	if (!envData.dmSenderRefNumber.isEmpty()) {
		this->dmSenderRefNumber->setText(envData.dmSenderRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmSenderIdent.isEmpty()) {
		this->dmSenderIdent->setText(envData.dmSenderIdent);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientRefNumber.isEmpty()) {
		this->dmRecipientRefNumber->setText(envData.dmRecipientRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientIdent.isEmpty()) {
		this->dmRecipientIdent->setText(envData.dmRecipientIdent);
		hideOptionalWidget = false;
	}
	if (!envData.dmToHands.isEmpty()) {
		this->dmToHands->setText(envData.dmToHands);
		hideOptionalWidget = false;
	}
	/* set check boxes */
	this->dmPersonalDelivery->setChecked(envData.dmPersonalDelivery);
	this->dmAllowSubstDelivery->setChecked(envData.dmAllowSubstDelivery);
	/* fill optional LegalTitle - Law, year, ... */
	if (!envData.dmLegalTitleLaw.isEmpty()) {
		this->dmLegalTitleLaw->setText(envData.dmLegalTitleLaw);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitleYear.isEmpty()) {
		this->dmLegalTitleYear->setText(envData.dmLegalTitleYear);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitleSect.isEmpty()) {
		this->dmLegalTitleSect->setText(envData.dmLegalTitleSect);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitlePar.isEmpty()) {
		this->dmLegalTitlePar->setText(envData.dmLegalTitlePar);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitlePoint.isEmpty()) {
		this->dmLegalTitlePoint->setText(envData.dmLegalTitlePoint);
		hideOptionalWidget = false;
	}

	this->optionalWidget->setHidden(hideOptionalWidget);
	this->optionalFieldCheckBox->setChecked(!hideOptionalWidget);

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDRecipient);
		this->payReplyCheckBox->show();
		this->payReplyCheckBox->setEnabled(true);
	} else {
		this->payReplyCheckBox->setEnabled(false);
		this->payReplyCheckBox->hide();
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

int DlgSendMessage::notifyOfPDZ(int pdzCnt) const
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

	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(title);
	msgBox.setInformativeText(info);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	return msgBox.exec();
}

bool DlgSendMessage::calculateAndShowTotalAttachSize(void)
{
	int aSize = m_attachmentModel.totalAttachmentSize();

	this->attachmentSizeInfo->setStyleSheet("QLabel { color: black }");

	if (m_attachmentModel.rowCount() > MAX_ATTACHMENT_FILES) {
		this->attachmentSizeInfo->
		     setStyleSheet("QLabel { color: red }");
		this->attachmentSizeInfo->setText(tr(
		    "Warning: The permitted amount (%1) of attachments has been exceeded.")
		        .arg(QString::number(MAX_ATTACHMENT_FILES)));
		return false;
	}


	if (aSize > 0) {
		if (aSize >= 1024) {
			this->attachmentSizeInfo->setText(
			    tr("Total size of attachments is ~%1 KB").
			    arg(aSize/1024));
			if (aSize >= MAX_ATTACHMENT_SIZE_BYTES) {
				this->attachmentSizeInfo->
				     setStyleSheet("QLabel { color: red }");
				this->attachmentSizeInfo->setText(
				    tr("Warning: Total size of attachments is larger than %1 MB!")
				    .arg(QString::number(
				        MAX_ATTACHMENT_SIZE_MB)));
				return false;
			}
		} else {
			this->attachmentSizeInfo->setText(tr(
			    "Total size of attachments is ~%1 B").arg(aSize));
		}
	} else {
		this->attachmentSizeInfo->setText(
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
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setWindowTitle(tr("Data box is not active"));
			msgBox.setText(tr(
			    "Recipient with data box ID '%1' does not have active data box.")
			    .arg(boxId));
			msgBox.setInformativeText(
			    tr("The message cannot be delivered."));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
			return;
		}
		if (entry.publicSending) {
			pdz = false;
		} else if (entry.commercialSending) {
			pdz = true;
		} else if (entry.effectiveOVM) {
			pdz = false;
		} else {
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setWindowTitle(tr("Cannot send to data box"));
			msgBox.setText(tr(
			    "Cannot send message to recipient with data box ID '%1'.")
			    .arg(boxId));
			msgBox.setInformativeText(tr(
			    "You won't be able as user '%1' to send messages into data box '%2'.")
			    .arg(m_userName).arg(boxId));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
			return;
		}
	} else if (foundBoxes.isEmpty()) {
		if (result == TaskSearchOwnerFulltext::SOF_SUCCESS) {
			/* No data box found. */
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setWindowTitle(tr("Wrong Recipient"));
			msgBox.setText(tr(
			    "Recipient with data box ID '%1' does not exist.")
			    .arg(boxId));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
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
	envelope.dmAnnotation = this->subjectText->text();

	/* Set optional fields. */
	envelope.dmSenderIdent = this->dmSenderIdent->text();
	envelope.dmRecipientIdent = this->dmRecipientIdent->text();
	envelope.dmSenderRefNumber = this->dmSenderRefNumber->text();
	envelope.dmRecipientRefNumber = this->dmRecipientRefNumber->text();
	envelope._using_dmLegalTitleLaw =
	    !this->dmLegalTitleLaw->text().isEmpty();
	if (envelope._using_dmLegalTitleLaw) {
		envelope.dmLegalTitleLaw =
		    this->dmLegalTitleLaw->text().toLong();
	}
	envelope._using_dmLegalTitleYear =
	    !this->dmLegalTitleYear->text().isEmpty();
	if (envelope._using_dmLegalTitleYear) {
		envelope.dmLegalTitleYear =
		    this->dmLegalTitleYear->text().toLong();
	}
	envelope.dmLegalTitleSect = this->dmLegalTitleSect->text();
	envelope.dmLegalTitlePar = this->dmLegalTitlePar->text();
	envelope.dmLegalTitlePoint = this->dmLegalTitlePoint->text();
	envelope.dmPersonalDelivery = this->dmPersonalDelivery->isChecked();

	/* Only OVM can change. */
	if (IsdsConversion::boxTypeStrToInt(m_dbType) > DBTYPE_OVM_REQ) {
		envelope.dmAllowSubstDelivery = true;
	} else {
		envelope.dmAllowSubstDelivery =
		    this->dmAllowSubstDelivery->isChecked();
	}

	if (m_dmType == QStringLiteral(DMTYPE_INIT)) {
		if (this->payRecipient->isChecked()) {
			dmType = QStringLiteral(DMTYPE_RESP);
		} else {
			dmType = QStringLiteral(DMTYPE_COMM);
		}
		if (!m_dmSenderRefNumber.isEmpty()) {
			envelope.dmRecipientRefNumber = m_dmSenderRefNumber;
		}
	} else {
		if (this->payReplyCheckBox->isChecked()) {
			dmType = QStringLiteral(DMTYPE_INIT);
		}
	}

	envelope.dmType = dmType;

	envelope.dmOVM = m_dbEffectiveOVM;

	envelope.dmPublishOwnID = this->dmPublishOwnID->isChecked();

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
	QString detailText;

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
			if (!this->payRecipient->isChecked()) {
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
		detailText = tr("An error occurred while loading attachments into message.");
		goto finish;
	}
	if (!buildEnvelope(message.envelope)) {
		detailText = tr("An error occurred during message envelope creation.");
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
		message.envelope.dmToHands = this->dmToHands->text();

		TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
		    m_userName, m_dbSet, taskIdentifiers.at(i), message,
		    e.name, e.address, e.pdz);
		task->setAutoDelete(true);
		globWorkPool.assignHi(task);
	}

	return;

finish:
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Send message error"));
	msgBox.setText(tr("It has not been possible to send a message to the ISDS server."));
	detailText += "\n\n" + tr("The message will be discarded.");
	msgBox.setInformativeText(detailText);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
	this->close();
}

void DlgSendMessage::buildEnvelopeWebDatovka(
    JsonLayer::Envelope &envelope) const
{
	/* Set mandatory fields of envelope. */
	envelope.dmAnnotation = this->subjectText->text();

	/* Set optional fields. */
	envelope.dmSenderIdent = this->dmSenderIdent->text();
	envelope.dmRecipientIdent = this->dmRecipientIdent->text();
	envelope.dmSenderRefNumber = this->dmSenderRefNumber->text();
	envelope.dmRecipientRefNumber = this->dmRecipientRefNumber->text();
	envelope.dmLegalTitleLaw = this->dmLegalTitleLaw->text();
	envelope.dmLegalTitleYear = this->dmLegalTitleYear->text();
	envelope.dmLegalTitleSect = this->dmLegalTitleSect->text();
	envelope.dmLegalTitlePar = this->dmLegalTitlePar->text();
	envelope.dmLegalTitlePoint = this->dmLegalTitlePoint->text();
	envelope.dmPersonalDelivery = this->dmPersonalDelivery->isChecked();
	envelope.dmPublishOwnID = this->dmPublishOwnID->isChecked();
	envelope.dmOVM = m_dbEffectiveOVM;

	/* Only OVM can change. */
	if (IsdsConversion::boxTypeStrToInt(m_dbType) > DBTYPE_OVM_REQ) {
		envelope.dmAllowSubstDelivery = true;
	} else {
		envelope.dmAllowSubstDelivery =
		    this->dmAllowSubstDelivery->isChecked();
	}
}

void DlgSendMessage::buildFileListWebDatovka(
    QList<JsonLayer::File> &fileList) const
{
	QModelIndex index;

	for (int row = 0; row < m_attachmentModel.rowCount(); ++row) {

		index = m_attachmentModel.index(row, DbFlsTblModel::FNAME_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}

		JsonLayer::File file;
		file.fName = index.data(Qt::DisplayRole).toString();

		index =
		    m_attachmentModel.index(row, DbFlsTblModel::CONTENT_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}

		file.fContent = index.data(Qt::DisplayRole).toByteArray();
		fileList.append(file);
	}
}

void DlgSendMessage::sendMessageWebDatovka(
    const QList<BoxContactsModel::PartialEntry> &recipEntries)
{
	/* Get account ID */
	int accountID = getWebDatovkaAccountId(m_userName);

	/* Create recipient list. */
	JsonLayer::Recipient recipient;
	QList<JsonLayer::Recipient> recipientList;
	foreach (const BoxContactsModel::PartialEntry &e, recipEntries) {
		recipient.recipientDbId = e.id;
		recipient.toHands = this->dmToHands->text();
		recipient.recipientName = e.name;
		recipient.recipientAddress = e.address;
		recipientList.append(recipient);
	}

	JsonLayer::Envelope envelope;
	buildEnvelopeWebDatovka(envelope);
	QList<JsonLayer::File> fileList;
	buildFileListWebDatovka(fileList);

	TaskSendMessageMojeId *task = new (std::nothrow) TaskSendMessageMojeId(
	    m_userName, accountID, recipientList, envelope, fileList);
	task->setAutoDelete(true);
	globWorkPool.assignHi(task);
}
