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

#include <cinttypes> /* PRId64 */
#include <cstdlib> /* exit(3) */
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QPrinter>
#include <QSet>
#include <QSettings>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QUrl>

#include "datovka.h"
#include "src/about.h"
#include "src/crypto/crypto_funcs.h"
#include "src/datovka_shared/isds/message_interface.h"
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/settings/pin.h"
#include "src/datovka_shared/settings/records_management.h"
#include "src/datovka_shared/utility/strings.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/delegates/tags_delegate.h"
#include "src/dimensions/dimensions.h"
#include "src/global.h"
#include "src/gov_services/gui/dlg_gov_services.h"
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_account_from_db.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_change_directory.h"
#include "src/gui/dlg_correspondence_overview.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_msg_box_informative.h"
#include "src/gui/dlg_msg_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_view_log.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/gui/dlg_import_zfo_result.h"
#include "src/gui/dlg_timestamp_expir.h"
#include "src/gui/dlg_yes_no_checkbox.h"
#include "src/gui/dlg_tags.h"
#include "src/gui/icon_container.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/filesystem.h"
#include "src/io/isds_helper.h"
#include "src/io/isds_login.h"
#include "src/io/isds_sessions.h"
#include "src/io/imports.h"
#include "src/io/message_db_single.h"
#include "src/io/message_db_set_container.h"
#include "src/io/tag_db.h"
#include "src/isds/type_description.h"
#include "src/model_interaction/account_interaction.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/records_management/gui/dlg_records_management.h"
#include "src/records_management/gui/dlg_records_management_stored.h"
#include "src/records_management/gui/dlg_records_management_upload.h"
#include "src/settings/preferences.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_key_press_filter.h"
#include "src/views/table_tab_ignore_filter.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_authenticate_message.h"
#include "src/worker/task_download_message.h"
#include "src/worker/task_download_message_list.h"
#include "src/worker/task_erase_message.h"
#include "src/worker/task_download_owner_info.h"
#include "src/worker/task_download_password_info.h"
#include "src/worker/task_download_user_info.h"
#include "src/worker/task_import_zfo.h"
#include "src/worker/task_vacuum_db_set.h"
#include "src/worker/task_verify_message.h"
#include "src/worker/task_split_db.h"
#include "ui_datovka.h"

#define WIN_POSITION_HEADER "window_position"
#define WIN_POSITION_X "x"
#define WIN_POSITION_Y "y"
#define WIN_POSITION_W "w"
#define WIN_POSITION_H "h"
#define WIN_POSITION_MAX "max"

QNetworkAccessManager *nam;

/*
 * If defined then no message table is going to be generated when clicking
 * on all sent or received messages.
 */
#define DISABLE_ALL_TABLE 1

/*!
 * @brief Returns QModelIndex of the currently selected account model node.
 */
#define currentAccountModelIndex() \
	(ui->accountList->selectionModel()->currentIndex())

/*!
 * @brief Returns QModelIndex of the currently selected message model node.
 */
#define currentSingleMessageIndex() \
	(ui->messageList->selectionModel()->currentIndex())

/*!
 * @brief Returns QModelIndexList containing first column indexes of selected
 *     message model rows.
 */
#define currentFrstColMessageIndexes() \
	(ui->messageList->selectionModel()->selectedRows(0))

/*!
 * @brief Returns QModelIndexList containing first column indexes of selected
 *     attachment model rows.
 */
#define currentFrstColAttachmentIndexes() \
	(ui->messageAttachmentList->selectionModel()->selectedRows(0))

/*!
 * @brief Return index for message with given properties.
 *
 * @param[in] view Message table view.
 * @param[in] dmId Message identifier.
 * @return Index of given message.
 */
static inline
QModelIndex messageIndex(const QTableView *view, qint64 dmId)
{
	if (Q_UNLIKELY((Q_NULLPTR == view) || (dmId < 0))) {
		Q_ASSERT(0);
		return QModelIndex();
	}

	const QAbstractItemModel *model = view->model();
	if (Q_UNLIKELY(Q_NULLPTR == model)) {
		Q_ASSERT(0);
		return QModelIndex();
	}

	/* Find and select the message with the ID. */
	for (int row = 0; row < model->rowCount(); ++row) {
		/*
		 * TODO -- Search in a more resource-saving way.
		 * Eliminate index copying, use smarter search.
		 */
		if (model->index(row, 0).data().toLongLong() == dmId) {
			return model->index(row, 0);
		}
	}

	return QModelIndex();
}

/*!
 * @brief Message identifier from index.
 *
 * @param[in] msgIdx Index into the message table view.
 * @return Message identifier.
 */
static inline
qint64 msgIdentifier(const QModelIndex &msgIdx)
{
	Q_ASSERT(msgIdx.isValid());
	if (msgIdx.column() == DbMsgsTblModel::DMID_COL) {
		return msgIdx.data().toLongLong();
	} else {
		return msgIdx.sibling(msgIdx.row(), DbMsgsTblModel::DMID_COL)
		    .data().toLongLong();
	}
}

/*!
 * @brief Message time from index.
 *
 * @param[in] msgIdx Index into the message table view.
 * @return Delivery time.
 */
static inline
QDateTime msgDeliveryTime(const QModelIndex &msgIdx)
{
	QModelIndex deliveryIdx(
	    msgIdx.sibling(msgIdx.row(), DbMsgsTblModel::DELIVERY_COL));
	Q_ASSERT(deliveryIdx.isValid());

	return dateTimeFromDbFormat(
	    deliveryIdx.data(ROLE_PLAIN_DISPLAY).toString());
}

/*!
 * @brief Returns a full message identifier from index.
 *
 * @param[in] msgIdx Index into the message table view.
 * @return Full message identifier.
 */
static inline
MessageDb::MsgId msgMsgId(const QModelIndex &msgIdx)
{
	Q_ASSERT(msgIdx.isValid());
	return MessageDb::MsgId(msgIdentifier(msgIdx), msgDeliveryTime(msgIdx));
}

/*!
 * @brief Returns list of full message identifiers.
 *
 * @param[in] msgIdxs List of indexes.
 * @return List if message identifiers.
 */
static inline
QList<MessageDb::MsgId> msgMsgIds(const QModelIndexList &msgIdxs)
{
	QList<MessageDb::MsgId> msgIds;
	foreach (const QModelIndex &idx, msgIdxs) {
		if (!idx.isValid()) {
			Q_ASSERT(0);
			return QList<MessageDb::MsgId>();
		}
		MessageDb::MsgId msgId(msgMsgId(idx));
		Q_ASSERT(msgId.dmId >= 0);
		msgIds.append(msgId);
	}
	return msgIds;
}

/*!
 * @brief Returns whether we are working with sent or received messages.
 *
 * @param[in] acntIdx Account model index.
 * @param[in] dfltDirect Default direction to be returned.
 * @return Message direction identifier.
 */
static
enum MessageDirection messageDirection(const QModelIndex &acntIdx,
    enum MessageDirection dfltDirect)
{
	enum MessageDirection ret = dfltDirect;

	switch (AccountModel::nodeType(acntIdx)) {
	case AccountModel::nodeRecentReceived:
	case AccountModel::nodeReceived:
	case AccountModel::nodeReceivedYear:
		ret = MSG_RECEIVED;
		break;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		ret = MSG_SENT;
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return ret;
}

MainWindow::~MainWindow(void)
{
	saveSettings();
	delete ui;
}

#define DFLT_COL_WIDTH 200
#define COL_WIDTH_COUNT 3
#define DFLT_COL_SORT 0
#define ICON_COL_WIDTH 24

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_accountModel(this),
    m_messageTableModel(DbMsgsTblModel::RCVD_MODEL, this),
    m_messageListProxyModel(this),
    m_attachmentModel(false, this),
    m_messageMarker(this),
    m_lastSelectedMessageId(-1),
    m_lastStoredMessageId(-1),
    m_lastSelectedAccountNodeType(AccountModel::nodeUnknown),
    m_lastStoredAccountNodeType(AccountModel::nodeUnknown),
    m_viewLogDlg(Q_NULLPTR),
    m_searchDlgActive(false),
    m_colWidthRcvd(COL_WIDTH_COUNT),
    m_colWidthSnt(COL_WIDTH_COUNT),
    m_sortCol(DFLT_COL_SORT),
    m_sort_order(),
    m_save_attach_dir(QDir::homePath()),
    m_add_attach_dir(QDir::homePath()),
    m_export_correspond_dir(QDir::homePath()),
    m_on_export_zfo_activate(QDir::homePath()),
    m_on_import_database_dir_activate(QDir::homePath()),
    m_import_zfo_path(QDir::homePath()),
    m_geometry(),
    m_msgTblAppendedCols(),
    ui(new Ui::MainWindow),
    mui_filterLine(0),
    mui_statusBar(0),
    mui_statusDbMode(0),
    mui_statusOnlineLabel(0),
    mui_statusProgressBar(0),
    mui_dockMenu()
{
	m_colWidthRcvd[1] = m_colWidthRcvd[2] = DFLT_COL_WIDTH;
	m_colWidthSnt[1] = m_colWidthSnt[2] = DFLT_COL_WIDTH;

	setUpUi();

	m_msgTblAppendedCols.append(DbMsgsTblModel::AppendedCol(QString(),
	    IconContainer::construcIcon(IconContainer::ICON_BRIEFCASE_GREY),
	    tr("Uploaded to records management service")));
	m_msgTblAppendedCols.append(DbMsgsTblModel::AppendedCol(
	    tr("Tags"), QIcon(), tr("User-assigned tags")));

	connect(&mui_dockMenu, SIGNAL(aboutToShow()),
	    this, SLOT(dockMenuPopulate()));
	connect(&mui_dockMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(dockMenuActionTriggerred(QAction *)));
#ifdef Q_OS_OSX
	mui_dockMenu.setAsDockMenu();
#endif /* Q_OS_OSX */

	/* Single instance emitter. */
	connect(GlobInstcs::snglInstEmitterPtr,
	    SIGNAL(messageReceived(int, QString)), this,
	    SLOT(processSingleInstanceMessages(int, QString)));

	/* Worker-related processing signals. */
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(downloadMessageFinished(QString, qint64, QDateTime, int,
	        QString, bool, int)), this,
	    SLOT(collectDownloadMessageStatus(QString, qint64, QDateTime, int,
	        QString, bool, int)));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(downloadMessageListFinished(QString, int, int, QString,
	        bool, int, int, int, int)), this,
	    SLOT(collectDownloadMessageListStatus(QString, int, int, QString,
	        bool, int, int, int, int)));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(importZfoFinished(QString, int, QString)), this,
	    SLOT(collectImportZfoStatus(QString, int, QString)));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(importMessageFinished(QString, QStringList, int, int)), this,
	    SLOT(showImportMessageResults(QString, QStringList, int, int)));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(progressChange(QString, int)), this,
	    SLOT(updateProgressBar(QString, int)));
	connect(GlobInstcs::msgProcEmitterPtr, SIGNAL(statusBarChange(QString)),
	    this, SLOT(updateStatusBarText(QString)));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64, int)));
	connect(GlobInstcs::msgProcEmitterPtr,
	    SIGNAL(refreshAccountList(QString)), this,
	    SLOT(refreshAccountList(QString)));

	connect(GlobInstcs::workPoolPtr, SIGNAL(assignedFinished()),
	    this, SLOT(backgroundWorkersFinished()));

	/* Account list. */
	ui->accountList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->accountList, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(accountItemRightClicked(QPoint)));
	ui->accountList->setSelectionMode(QAbstractItemView::SingleSelection);

	/* Message list. */
	ui->messageList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->messageList, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(messageItemRightClicked(QPoint)));
	ui->messageList->setSelectionMode(QAbstractItemView::ExtendedSelection);
	ui->messageList->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->messageList->setFocusPolicy(Qt::StrongFocus);
	connect(ui->messageList, SIGNAL(doubleClicked(QModelIndex)), this,
	    SLOT(viewSelectedMessage()));
	ui->messageList->installEventFilter(
	    new TableHomeEndFilter(ui->messageList));
	{
		TableKeyPressFilter *filter =
		    new TableKeyPressFilter(ui->messageList);
		filter->registerAction(Qt::Key_Return,
		    &viewSelectedMessageViaFilter, this, true);
		filter->registerAction(Qt::Key_Enter, /* On keypad. */
		    &viewSelectedMessageViaFilter, this, true);
		ui->messageList->installEventFilter(filter);
	}
	ui->messageList->installEventFilter(
	    new TableTabIgnoreFilter(ui->messageList));
	ui->messageList->setItemDelegate(new TagsDelegate(this));

#if 1
	/*
	 * If the 'opened' signal is connected before loadSettings() is called
	 * then an unnecessary series of model updates is called into action.
	 * However, the code must still be able to handle such order of action.
	 * So don't remove this comment or the code inside the following block
	 * which has been disabled using the preprocessor conditional because
	 * it may be used for testing.
	 */

	/* Load configuration file. */
	loadSettings();

	/* Handle opening of database files on demand. */
	connect(GlobInstcs::msgDbsPtr, SIGNAL(opened(QString)),
	    this, SLOT(refreshAccountList(QString)));
#else
	connect(GlobInstcs::msgDbsPtr, SIGNAL(opened(QString)),
	    this, SLOT(refreshAccountList(QString)));

	loadSettings();
#endif

	/* Set toolbar buttons style from settings */
	ui->toolBar->setToolButtonStyle(
	    (Qt::ToolButtonStyle)Preferences::qtToolButtonStyle(
	        GlobInstcs::prefsPtr->toolbarButtonStyle));

	/* Account list must already be set in order to connect this signal. */
	connect(ui->accountList->selectionModel(),
	    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
	    SLOT(accountItemCurrentChanged(QModelIndex, QModelIndex)));

	/* Enable drag and drop on account list. */
	ui->accountList->setAcceptDrops(true);
	ui->accountList->setDragEnabled(true);
	ui->accountList->setDragDropOverwriteMode(false);
	ui->accountList->setDropIndicatorShown(true);
	ui->accountList->setDragDropMode(QAbstractItemView::InternalMove);
	ui->accountList->setDefaultDropAction(Qt::MoveAction);

	/* Enable sorting of message table items. */
	ui->messageList->setSortingEnabled(true);

	/* Set default column size. */
	/* TODO -- Check whether received or sent messages are shown? */
	setReceivedColumnWidths();

	/* Attachment list. */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* Selection model may not be set. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(attachmentItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
	}
	ui->messageAttachmentList->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->messageAttachmentList->setSelectionMode(
	    QAbstractItemView::ExtendedSelection);
	ui->messageAttachmentList->setSelectionBehavior(
	    QAbstractItemView::SelectRows);
	connect(ui->messageAttachmentList,
	    SIGNAL(customContextMenuRequested(QPoint)), this,
	    SLOT(attachmentItemRightClicked(QPoint)));
	connect(ui->messageAttachmentList,
	    SIGNAL(doubleClicked(QModelIndex)), this,
	    SLOT(openSelectedAttachment(QModelIndex)));
	ui->messageAttachmentList->installEventFilter(
	    new TableHomeEndFilter(ui->messageAttachmentList));
	ui->messageAttachmentList->installEventFilter(
	    new TableTabIgnoreFilter(ui->messageAttachmentList));

	/* It fires when any column was resized. */
	connect(ui->messageList->horizontalHeader(),
	    SIGNAL(sectionResized(int, int, int)),
	    this, SLOT(onTableColumnResized(int, int, int)));

	/* It fires when any column was clicked. */
	connect(ui->messageList->horizontalHeader(),
	    SIGNAL(sectionClicked(int)),
	    this, SLOT(onTableColumnHeaderSectionClicked(int)));

	/* Connect non-automatic menu actions. */
	connectTopMenuBarSlots();
	connectMessageActionBarSlots();

	/* Message marking timer. */
	connect(&m_messageMarker, SIGNAL(timeout()),
	    this, SLOT(messageItemsSelectedMarkRead()));

	/* Initialisation of message download timer. */
	connect(&m_timerSyncAccounts, SIGNAL(timeout()), this,
	    SLOT(synchroniseAllAccounts()));
	if (GlobInstcs::prefsPtr->downloadOnBackground) {
		if (GlobInstcs::prefsPtr->timerValue > 4) {
			m_timeoutSyncAccounts =
			    GlobInstcs::prefsPtr->timerValue * 60000;
		} else {
			m_timeoutSyncAccounts = TIMER_DEFAULT_TIMEOUT_MS;
		}
		m_timerSyncAccounts.start(m_timeoutSyncAccounts);

		logInfo("Timer set to %d minutes.\n",
		    m_timeoutSyncAccounts / 60000);
	}

	QTimer::singleShot(RUN_FIRST_ACTION_MS, this,
	    SLOT(setWindowsAfterInit()));

	QString msgStrg(tr("disk"));
	QString acntStrg(tr("disk"));

	if (!GlobInstcs::prefsPtr->storeMessagesOnDisk) {
		msgStrg = tr("memory");
	}

	if (!GlobInstcs::prefsPtr->storeAdditionalDataOnDisk) {
		acntStrg = tr("memory");
	}

	mui_statusDbMode->setText(tr("Storage:") + " " + msgStrg + " | "
	    + acntStrg + "   ");

	/* TODO -- This is only a temporary solution. */
	ui->actionUpdate_records_management_information->setEnabled(
	    GlobInstcs::recMgmtSetPtr->isValid());
}

void MainWindow::setWindowsAfterInit(void)
{
	debugSlotCall();

	/* Always send the request for new version check. */
	checkNewDatovkaVersion();

	if (ui->accountList->model()->rowCount() <= 0) {
		showAddNewAccountDialog();
	} else {
		if (GlobInstcs::prefsPtr->downloadAtStart &&
		    ui->actionSync_all_accounts->isEnabled()) {
			/*
			 * Calling account synchronisation as slot bound to
			 * different events is repeatedly causing problems.
			 * Here we try to minimise the probability that the
			 * synchronisation  action is triggered for the second
			 * time after the application start-up.
			 *
			 * TODO -- The checking for live ISDS connection must
			 * be guarded for simultaneous access.
			 */
			synchroniseAllAccounts();
		}
	}
}

void MainWindow::checkNewDatovkaVersion(void)
{
	debugSlotCall();

	if (GlobInstcs::prefsPtr->sendStatsWithVersionChecks()) {
		/* TODO - sent info about datovka, libs and OS to our server */
		nam = new QNetworkAccessManager(this);
		connect(nam, SIGNAL(finished(QNetworkReply *)),
		    this, SLOT(datovkaVersionResponce(QNetworkReply *)));
		QUrl url(DATOVKA_CHECK_NEW_VERSION_URL);
		nam->get(QNetworkRequest(url));
	} else {
		nam = new QNetworkAccessManager(this);
		connect(nam, SIGNAL(finished(QNetworkReply *)),
		    this, SLOT(datovkaVersionResponce(QNetworkReply *)));
		QUrl url(DATOVKA_CHECK_NEW_VERSION_URL);
		nam->get(QNetworkRequest(url));
	}
}

void MainWindow::datovkaVersionResponce(QNetworkReply *reply)
{
	debugFuncCall();

	if (!GlobInstcs::prefsPtr->checkNewVersions()) {
		/* Just discard the response. */
		delete reply;
		return;
	}

	if (reply->error() == QNetworkReply::NoError) {
		QByteArray bytes = reply->readAll();
		QString vstr = QString::fromUtf8(bytes.data(), bytes.size());
		vstr.remove(QRegExp("[\n\t\r]"));
		if (1 == compareVersionStrings(vstr,
		             QCoreApplication::applicationVersion())) {
			showStatusTextWithTimeout(
			    tr("New version of Datovka is available:") +
			    " " + vstr);
#ifdef WIN32
			int res = QMessageBox::information(this,
			    tr("New version of Datovka"),
			    tr("New version of Datovka is available.") +"\n\n"+
			    tr("Current version is %1").
			        arg(QCoreApplication::applicationVersion())
			    + "\n" +
			    tr("New version is %1").arg(vstr) +
			    + "\n\n" +
			    tr("Do you want to download new version?"),
			    QMessageBox::Yes | QMessageBox::No);
			if (QMessageBox::Yes == res) {
				QDesktopServices::openUrl(
				QUrl(DATOVKA_DOWNLOAD_URL,
				QUrl::TolerantMode));
			}
#else
			QMessageBox::information(this,
			    tr("New version of Datovka"),
			    tr("New version of Datovka is available.") +"\n\n"+
			    tr("Current version is \"%1\"").
			        arg(QCoreApplication::applicationVersion())
			    + "\n" +
			    tr("New version is \"%1\"").arg(vstr)
			    + "\n\n" +
			    tr("Update your application..."),
			    QMessageBox::Ok);
#endif
		}
	}

	delete reply;
}

void MainWindow::showStatusTextWithTimeout(const QString &qStr)
{
	clearStatusBar();
	mui_statusBar->showMessage((qStr), TIMER_STATUS_TIMEOUT_MS);
}

void MainWindow::showStatusTextPermanently(const QString &qStr)
{
	clearStatusBar();
	mui_statusBar->showMessage((qStr), 0);
}

void MainWindow::clearProgressBar(void)
{
	mui_statusProgressBar->setFormat(PL_IDLE);
	mui_statusProgressBar->setValue(0);
}

void MainWindow::clearStatusBar(void)
{
	mui_statusBar->clearMessage();
}

void MainWindow::updateProgressBar(const QString &label, int value)
{
	if (value == -1) {
		mui_statusProgressBar->setMaximum(0);
		mui_statusProgressBar->setMinimum(0);
	} else {
		mui_statusProgressBar->setMaximum(100);
		mui_statusProgressBar->setMinimum(0);
		mui_statusProgressBar->setValue(value);
	}
	mui_statusProgressBar->setFormat(label);
	mui_statusProgressBar->repaint();
}

void MainWindow::updateStatusBarText(const QString &text)
{
	showStatusTextPermanently(text);
}

void MainWindow::showPreferencesDialog(void)
{
	debugSlotCall();

	if (!DlgPreferences::modify(*GlobInstcs::prefsPtr,
	        *GlobInstcs::pinSetPtr, this)) {
		return;
	}

	// set actual download timer value from settings if is enable
	if (GlobInstcs::prefsPtr->downloadOnBackground) {
		if (GlobInstcs::prefsPtr->timerValue > 4) {
			m_timeoutSyncAccounts =
			    GlobInstcs::prefsPtr->timerValue * 60000;
		} else {
			m_timeoutSyncAccounts = TIMER_DEFAULT_TIMEOUT_MS;
		}
		m_timerSyncAccounts.start(m_timeoutSyncAccounts);
		qDebug() << "Timer set on" << m_timeoutSyncAccounts / 60000
		    << "minutes";
	} else {
		m_timerSyncAccounts.stop();
	}
}

void MainWindow::showProxySettingsDialog(void)
{
	debugSlotCall();

	if (DlgProxysets::modify(*GlobInstcs::proxSetPtr, this)) {
		/* Dialog accepted, store all settings. */
		saveSettings();
	}
}

/*!
 * @brief Shows all columns except the supplied ones.
 *
 * @param[in] view Table view.
 * @param[in] hideCols Indexes of column to be hidden, may also be negative.
 */
static
void showAllColumnsExcept(QTableView *view, const QList<int> &hideCols)
{
	if (Q_NULLPTR == view) {
		Q_ASSERT(0);
		return;
	}

	QAbstractItemModel *model = view->model();
	if (Q_NULLPTR == model) {
		return;
	}

	/* Set all columns visible. */
	for (int col = 0; col < model->columnCount(); ++col) {
		view->setColumnHidden(col, false);
	}

	foreach (int col, hideCols) {
		if (col < 0) {
			/* Negative are taken from the end. */
			col = model->columnCount() + col;
		}
		if ((col < 0) || (col >= model->columnCount())) {
			Q_ASSERT(0);
			continue;
		}
		view->setColumnHidden(col, true);
	}
}

/*!
 * @brief Shows/hides message table columns according to functionality.
 *
 * @param[in,out] view Table view.
 * @param[in]     type Whether received or sent messages are displayed.
 */
static
void showMessageColumnsAccordingToFunctionality(QTableView *view,
    enum DbMsgsTblModel::Type type)
{
	QList<int> hideCols;

	if (type == DbMsgsTblModel::RCVD_MODEL) {
		hideCols.append(DbMsgsTblModel::RECIP_COL);
		hideCols.append(DbMsgsTblModel::MSGSTAT_COL);
	} else if (type == DbMsgsTblModel::SNT_MODEL) {
		hideCols.append(DbMsgsTblModel::PERSDELIV_COL);
		hideCols.append(DbMsgsTblModel::SENDER_COL);
		hideCols.append(DbMsgsTblModel::READLOC_COL);
		hideCols.append(DbMsgsTblModel::PROCSNG_COL);
	}

	if (!GlobInstcs::recMgmtSetPtr->isValid()) {
		hideCols.append(DbMsgsTblModel::RECMGMT_NEG_COL);
	}

	showAllColumnsExcept(view, hideCols);
}

void MainWindow::showRecordsManagementDialogue(void)
{
	debugSlotCall();

	if (DlgRecordsManagement::updateSettings(*GlobInstcs::recMgmtSetPtr,
	        this)) {
		saveSettings();
	}

	ui->actionUpdate_records_management_information->setEnabled(
	    GlobInstcs::recMgmtSetPtr->isValid());

	m_messageTableModel.setRecordsManagementIcon();
	m_messageTableModel.fillRecordsManagementColumn(
	    DbMsgsTblModel::RECMGMT_NEG_COL);

	showMessageColumnsAccordingToFunctionality(ui->messageList,
	    m_messageTableModel.type());
	AccountModel::nodeTypeIsReceived(currentAccountModelIndex()) ?
	    setReceivedColumnWidths() : setSentColumnWidths();
}

void MainWindow::accountItemCurrentChanged(const QModelIndex &current,
    const QModelIndex &previous)
{
	debugSlotCall();

	Q_UNUSED(previous);

	QString html;

	if (!current.isValid()) {
		/* May occur on deleting last account. */
		setMessageActionVisibility(0);

		ui->messageList->selectionModel()->disconnect(
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(messageItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutAboutToBeChanged()), this,
		    SLOT(messageItemStoreSelectionOnModelChange()));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutChanged()), this,
		    SLOT(messageItemRestoreSelectionAfterLayoutChange()));

		/* Decouple model and show banner page. */
		ui->messageList->setModel(Q_NULLPTR);
		ui->messageStackedWidget->setCurrentIndex(0);
		ui->accountTextInfo->setHtml(createDatovkaBanner(
		    QCoreApplication::applicationVersion()));
		ui->accountTextInfo->setReadOnly(true);
		return;
	}

	const QString userName(m_accountModel.userName(current));
	if (userName.isEmpty()) {
		logErrorNL("%s", "Have empty user name.");
//		Q_ASSERT(0);
		return;
	}

	enableSendEGovRequestAction(true, userName);

	const MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		/* May occur on deleting last account. */
		setMessageActionVisibility(0);

		ui->messageList->selectionModel()->disconnect(
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(messageItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutAboutToBeChanged()), this,
		    SLOT(messageItemStoreSelectionOnModelChange()));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutChanged()), this,
		    SLOT(messageItemRestoreSelectionAfterLayoutChange()));

		/* Get user name and db location. */
		const AcntSettings &itemSettings(
		    (*GlobInstcs::acntMapPtr)[userName]);

		QString dbDir = itemSettings.dbDir();
		if (dbDir.isEmpty()) {
			/* Set default directory name. */
			dbDir = GlobInstcs::prefsPtr->confDir();
		}

		/* Decouple model and show banner page. */
		ui->messageList->setModel(Q_NULLPTR);
		ui->messageStackedWidget->setCurrentIndex(0);
		QString htmlMessage = "<div style=\"margin-left: 12px;\">"
		    "<h3>" + tr("Database access error") + "</h3>" "<br/>";
		htmlMessage += "<div>";
		htmlMessage += tr("Database files for account '%1' cannot be accessed in location '%2'."
		    ).arg(userName.toHtmlEscaped()).arg(dbDir.toHtmlEscaped());
		htmlMessage += "<br/>";
		htmlMessage += tr("The file cannot be accessed or is "
		    "corrupted. Please fix the access privileges or "
		    "remove or rename the file so that the application can "
		    "create a new empty file.");
		htmlMessage += "<br/><br/>";
		htmlMessage += tr("Create a backup copy of the affected file. "
		    "This will help when trying to perform data recovery.");
		htmlMessage += "<br/><br/>";
		htmlMessage += tr("In general, it is recommended to create "
		    "backup copies of the database files to prevent data "
		    "loss.");
		htmlMessage += "</div>";
		htmlMessage += "</div>";
		ui->accountTextInfo->setHtml(htmlMessage);
		ui->accountTextInfo->setReadOnly(true);
		return;
	}

	/*
	 * Disconnect message clicked. This slot will be enabled only for
	 * received messages.
	 */
	ui->messageList->disconnect(SIGNAL(clicked(QModelIndex)),
	    this, SLOT(messageItemClicked(QModelIndex)));

	/* Clicked account item. */
	const enum AccountModel::NodeType accntNodeType =
	    AccountModel::nodeType(current);
	/* Depending on which account item was clicked show/hide elements. */
	enum AccountModel::NodeType msgViewType = AccountModel::nodeUnknown;

	/* Reading database data may take some time. */
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	m_messageListProxyModel.setSourceModel(&m_messageTableModel); /* TODO */
	m_messageListProxyModel.setSortRole(ROLE_MSGS_DB_PROXYSORT); /* TODO */
	ui->messageList->setModel(&m_messageListProxyModel); /* TODO */

	switch (accntNodeType) {
	case AccountModel::nodeAccountTop:
		setMessageActionVisibility(0);
		html = createAccountInfo(userName);
		ui->actionDelete_message_from_db->setEnabled(false);
		break;
	case AccountModel::nodeRecentReceived:
		m_messageTableModel.assignData(
		    dbSet->msgsRcvdEntriesWithin90Days(),
		    m_msgTblAppendedCols.size());
		m_messageTableModel.setHeader(m_msgTblAppendedCols);
		//ui->messageList->horizontalHeader()->moveSection(5,3);
		ui->actionDelete_message_from_db->setEnabled(false);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeRecentSent:
		m_messageTableModel.assignData(
		    dbSet->msgsSntEntriesWithin90Days(),
		    m_msgTblAppendedCols.size());
		m_messageTableModel.setHeader(m_msgTblAppendedCols);
		ui->actionDelete_message_from_db->setEnabled(false);
		break;
	case AccountModel::nodeAll:
		setMessageActionVisibility(0);
		html = createAccountInfoAllField(tr("All messages"),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_RECEIVED,
		        DESCENDING),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_SENT, DESCENDING));
		ui->actionDelete_message_from_db->setEnabled(false);
		break;
	case AccountModel::nodeReceived:
#ifdef DISABLE_ALL_TABLE
		setMessageActionVisibility(0);
		html = createAccountInfoMessagesCount(
		    tr("All received messages"),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_RECEIVED,
		        DESCENDING),
		    MessageDb::TYPE_RECEIVED);
		ui->actionDelete_message_from_db->setEnabled(false);
#else /* !DISABLE_ALL_TABLE */
		m_messageTableModel.assignData(dbSet->msgsEntriesRcvd(),
		    m_msgTblAppendedCols.size());
		m_messageTableModel.setHeader(m_msgTblAppendedCols);
		ui->actionDelete_message_from_db->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
#endif /* DISABLE_ALL_TABLE */
		break;
	case AccountModel::nodeSent:
#ifdef DISABLE_ALL_TABLE
		setMessageActionVisibility(0);
		html = createAccountInfoMessagesCount(
		    tr("All sent messages"),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_SENT, DESCENDING),
		    MessageDb::TYPE_SENT);
		ui->actionDelete_message_from_db->setEnabled(false);
#else /* !DISABLE_ALL_TABLE */
		m_messageTableModel.assignData(dbSet->msgsSntEntries(),
		    m_msgTblAppendedCols.size());
		m_messageTableModel.setHeader(m_msgTblAppendedCols);
		ui->actionDelete_message_from_db->setEnabled(true);
#endif /* DISABLE_ALL_TABLE */
		break;
	case AccountModel::nodeReceivedYear:
		m_messageTableModel.assignData(
		    dbSet->msgsRcvdEntriesInYear(
		        current.data(ROLE_PLAIN_DISPLAY).toString()),
		    m_msgTblAppendedCols.size());
		m_messageTableModel.setHeader(m_msgTblAppendedCols);
		/* TODO -- Parameter check. */
		ui->actionDelete_message_from_db->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeSentYear:
		m_messageTableModel.assignData(
		    dbSet->msgsSntEntriesInYear(
		        current.data(ROLE_PLAIN_DISPLAY).toString()),
		    m_msgTblAppendedCols.size());
		m_messageTableModel.setHeader(m_msgTblAppendedCols);
		/* TODO -- Parameter check. */
		ui->actionDelete_message_from_db->setEnabled(true);
		break;
	default:
		logErrorNL("%s", "Cannot determine account node type.");
//		Q_ASSERT(0);
		goto end;
		break;
	}

	{
		m_messageTableModel.setRecordsManagementIcon();
		m_messageTableModel.fillRecordsManagementColumn(
		    DbMsgsTblModel::RECMGMT_NEG_COL);
		m_messageTableModel.fillTagsColumn(userName,
		    DbMsgsTblModel::TAGS_NEG_COL);
		/* TODO -- Add some labels. */
	}

	/* Enable/disable split database by year submenu */
	if (MessageDbSet::DO_YEARLY == dbSet->organisation()) {
		ui->actionSplit_database_by_years->setEnabled(false);
	} else {
		ui->actionSplit_database_by_years->setEnabled(true);
	}

	/*
	 * Disconnect slot from model as we want to prevent a signal to be
	 * handled multiple times.
	 */
	if (0 != ui->messageList->selectionModel()) {
		/* New model hasn't been set yet. */
		ui->messageList->selectionModel()->disconnect(
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(messageItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutAboutToBeChanged()), this,
		    SLOT(messageItemStoreSelectionOnModelChange()));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutChanged()), this,
		    SLOT(messageItemRestoreSelectionAfterLayoutChange()));
	}

	switch (accntNodeType) {
	case AccountModel::nodeAccountTop:
	case AccountModel::nodeAll:
#ifdef DISABLE_ALL_TABLE
	case AccountModel::nodeReceived:
	case AccountModel::nodeSent:
#endif /* DISABLE_ALL_TABLE */
		ui->messageStackedWidget->setCurrentIndex(0);
		ui->accountTextInfo->setHtml(html);
		ui->accountTextInfo->setReadOnly(true);
		break;
	case AccountModel::nodeRecentReceived:
#ifndef DISABLE_ALL_TABLE
	case AccountModel::nodeReceived:
#endif /* !DISABLE_ALL_TABLE */
	case AccountModel::nodeReceivedYear:
		/* Set model. */
		showMessageColumnsAccordingToFunctionality(ui->messageList,
		    m_messageTableModel.type());
		/* Set specific column width. */
		setReceivedColumnWidths();
		msgViewType = AccountModel::nodeReceived;
		break;
	case AccountModel::nodeRecentSent:
#ifndef DISABLE_ALL_TABLE
	case AccountModel::nodeSent:
#endif /* !DISABLE_ALL_TABLE */
	case AccountModel::nodeSentYear:
		/* Set model. */
		showMessageColumnsAccordingToFunctionality(ui->messageList,
		    m_messageTableModel.type());
		/* Set specific column width. */
		setSentColumnWidths();
		msgViewType = AccountModel::nodeSent;
		break;
	default:
		logErrorNL("%s", "Cannot determine account node type.");
//		Q_ASSERT(0);
		goto end;
		break;
	}

	/* Set model. */
	if (msgViewType != AccountModel::nodeUnknown) {
		ui->messageStackedWidget->setCurrentIndex(1);
		/* Apply message filter. */
		filterMessages(mui_filterLine->text());
		/* Connect new slot. */
		connect(ui->messageList->selectionModel(),
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(messageItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
		connect(ui->messageList->model(),
		    SIGNAL(layoutAboutToBeChanged()), this,
		    SLOT(messageItemStoreSelectionOnModelChange()));
		connect(ui->messageList->model(),
		    SIGNAL(layoutChanged()), this,
		    SLOT(messageItemRestoreSelectionAfterLayoutChange()));
		/* Clear message info. */
		ui->messageInfo->clear();
		/* Clear attachment list. */
		messageItemsSelectionChanged(QItemSelection());
		{
			/* Select last message in list if there are some messages. */
			QAbstractItemModel *itemModel = ui->messageList->model();
			/* enable/disable buttons */
			if ((Q_NULLPTR != itemModel) && (0 < itemModel->rowCount())) {
				messageItemRestoreSelectionOnModelChange();
				ui->actionAuthenticate_message_file->setEnabled(true);
				ui->actionExport_correspondence_overview->
				    setEnabled(true);
				ui->actionCheck_message_timestamp_expiration->
				    setEnabled(true);
			} else {
				ui->actionAuthenticate_message_file->setEnabled(false);
			}
		}
	}

	/* Set specific column width. */
	switch (msgViewType) {
	case AccountModel::nodeReceived:
		setReceivedColumnWidths();
		break;
	case AccountModel::nodeSent:
		setSentColumnWidths();
		break;
	default:
		break;
	}

end:
	QApplication::restoreOverrideCursor();
}

/* ========================================================================= */
/*
 * Generates menu to selected account item.
 *     (And redraw widgets.)
 */
void MainWindow::accountItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex acntIdx = ui->accountList->indexAt(point);
	QMenu *menu = new QMenu;

	if (acntIdx.isValid()) {
		bool received = AccountModel::nodeTypeIsReceived(acntIdx);

		menu->addAction(ui->actionGet_messages);
		menu->addAction(ui->actionSend_message);
		menu->addAction(ui->actionSend_egov_request);
		menu->addSeparator();
		if (received) {
			QMenu *submenu = menu->addMenu(tr("Mark"));
			menu->addSeparator();

			switch (AccountModel::nodeType(acntIdx)) {
			case AccountModel::nodeRecentReceived:
				submenu->addAction(tr("As Read"), this,
				    SLOT(accountMarkRecentReceivedRead()));
				submenu->addAction(tr("As Unread"), this,
				    SLOT(accountMarkRecentReceivedUnread()));

				submenu->addSeparator();
				submenu->addAction(tr("As Unsettled"), this,
				    SLOT(accountMarkRecentReceivedUnsettled()));
				submenu->addAction(tr("As in Progress"), this,
				    SLOT(accountMarkRecentReceivedInProgress()));
				submenu->addAction(tr("As Settled"), this,
				    SLOT(accountMarkRecentReceivedSettled()));
				break;
			case AccountModel::nodeReceived:
				submenu->addAction(tr("As Read"),
				    this, SLOT(accountMarkReceivedRead()));
				submenu->addAction(tr("As Unread"),
				    this, SLOT(accountMarkReceivedUnread()));

				submenu->addSeparator();
				submenu->addAction(tr("As Unsettled"), this,
				    SLOT(accountMarkReceivedUnsettled()));
				submenu->addAction(tr("As in Progress"), this,
				    SLOT(accountMarkReceivedInProgress()));
				submenu->addAction(tr("As Settled"), this,
				    SLOT(accountMarkReceivedSettled()));
				break;
			case AccountModel::nodeReceivedYear:
				submenu->addAction(tr("As Read"), this,
				    SLOT(accountMarkReceivedYearRead()));
				submenu->addAction(tr("As Unread"), this,
				    SLOT(accountMarkReceivedYearUnread()));

				submenu->addSeparator();
				submenu->addAction(tr("As Unsettled"), this,
				    SLOT(accountMarkReceivedYearUnsettled()));
				submenu->addAction(tr("As in Progress"), this,
				    SLOT(accountMarkReceivedYearInProgress()));
				submenu->addAction(tr("As Settled"), this,
				    SLOT(accountMarkReceivedYearSettled()));
				break;
			default:
				Q_ASSERT(0);
				break;
			}
		}
		menu->addAction(ui->actionChange_password);
		menu->addSeparator();
		menu->addAction(ui->actionAccount_properties);
		menu->addAction(ui->actionDelete_account);
		menu->addSeparator();
		menu->addAction(ui->actionMove_account_up);
		menu->addAction(ui->actionMove_account_down);
		menu->addSeparator();
		menu->addAction(ui->actionChange_data_directory);
#ifdef PORTABLE_APPLICATION
		ui->actionChange_data_directory->setEnabled(false);
#endif /* PORTABLE_APPLICATION */

		menu->addSeparator();
		menu->addAction(ui->actionImport_messages_from_database);
		menu->addAction(ui->actionImport_ZFO_file_into_database);
	} else {
		menu->addAction(ui->actionAdd_account);
	}

	menu->exec(QCursor::pos());
	menu->deleteLater();
}

/* ========================================================================= */
/*
 * Sets contents of widgets according to selected messages.
 */
void MainWindow::messageItemsSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
/* ========================================================================= */
{
	debugSlotCall();

	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	/*
	 * Disconnect slot from model as we want to prevent a signal to be
	 * handled multiple times.
	 */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* New model hasn't been set yet. */
		ui->messageAttachmentList->selectionModel()->disconnect(
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(attachmentItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
	}

	/* Disable message/attachment related buttons. */
	setMessageActionVisibility(0);

	ui->messageStateCombo->setEnabled(false);

	/* Disable model for attachment list. */
	ui->messageAttachmentList->setModel(0);
	/* Clear message information. */
	ui->messageInfo->setHtml("");
	ui->messageInfo->setReadOnly(true);

	QModelIndexList firstColumnIdxs(currentFrstColMessageIndexes());

	/* Stop the timer. */
	m_messageMarker.stop();

	if (firstColumnIdxs.isEmpty()) {
		/* Invalid message selected. */
		messageItemStoreSelection(-1);
		/* End if invalid item is selected. */
		return;
	}

	/* Enable message/attachment actions depending on message selection. */
	setMessageActionVisibility(firstColumnIdxs.size());

	/* Reply only to received messages. */
	bool received =
	    AccountModel::nodeTypeIsReceived(currentAccountModelIndex());
	ui->actionReply->setEnabled(received && (firstColumnIdxs.size() == 1));

	ui->messageStateCombo->setEnabled(received);

	if (1 != firstColumnIdxs.size()) {
		/* Multiple messages selected - stop here. */
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	MessageDb::MsgId msgId(msgMsgId(firstColumnIdxs.first()));

	/* Remember last selected message. */
	messageItemStoreSelection(msgId.dmId);

	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return;
	}

	/* Mark message locally read. */
	if (!messageDb->messageLocallyRead(msgId.dmId)) {
		if (GlobInstcs::prefsPtr->messageMarkAsReadTimeout >= 0) {
			qDebug() << "Starting timer to mark as read for message"
			    << msgId.dmId;
			m_messageMarker.setSingleShot(true);
			m_messageMarker.start(
			    GlobInstcs::prefsPtr->messageMarkAsReadTimeout);
		}
	} else {
		m_messageMarker.stop();
	}

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId.dmId));
	ui->messageInfo->setReadOnly(true);

	if (received) {
		int msgState = messageDb->getMessageProcessState(msgId.dmId);

		/* msgState is -1 if message is not in database */
		if (msgState >= 0) {
			ui->messageStateCombo->setCurrentIndex(msgState);
		} else {
			/* insert message state into database */
			messageDb->setMessageProcessState(msgId.dmId, UNSETTLED);
			ui->messageStateCombo->setCurrentIndex(UNSETTLED);
		}
	} else {
		ui->messageStateCombo->setCurrentIndex(UNSETTLED);
	}

	/* Show files related to message message. */
	m_attachmentModel.removeRows(0, m_attachmentModel.rowCount());
	m_attachmentModel.setHeader();
	m_attachmentModel.appendData(messageDb->attachEntries(msgId.dmId));
	ui->messageAttachmentList->setModel(&m_attachmentModel);
	/* First three columns contain hidden data. */
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::ATTACHID_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::MSGID_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::BINARY_CONTENT_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::MIME_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::FPATH_COL, true);

	if (ui->messageAttachmentList->model()->rowCount() > 0) {
		ui->actionSave_all_attachments->setEnabled(true);
	}

	ui->messageAttachmentList->resizeColumnToContents(
	    AttachmentTblModel::FNAME_COL);

	/* Connect new slot. */
	connect(ui->messageAttachmentList->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this,
	    SLOT(attachmentItemsSelectionChanged(QItemSelection,
	        QItemSelection)));
}


/* ========================================================================= */
/*
 * Used for toggling the message read state.
 */
void MainWindow::messageItemClicked(const QModelIndex &index)
/* ========================================================================= */
{
	debugSlotCall();

	if (DbMsgsTblModel::READLOC_COL != index.column()) {
		logDebugLv1NL("%s", "Not clicked read locally.");
		return;
	}

	/* Stop the timer. */
	m_messageMarker.stop();

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const MessageDb::MsgId msgId(msgMsgId(index));

	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return;
	}

	/* Get message state from database and toggle the value. */
	bool isRead = messageDb->messageLocallyRead(msgId.dmId);
	messageDb->setMessageLocallyRead(msgId.dmId, !isRead);

	/*
	 * Mark message as read without reloading
	 * the whole model.
	 */
	m_messageTableModel.overrideRead(msgId.dmId, !isRead);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(currentAccountModelIndex());
}


/* ========================================================================= */
/*
 * Generates menu to selected message item.
 *     (And redraw widgets.)
 */
void MainWindow::messageItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex index = ui->messageList->indexAt(point);

	/* Only when clicked on some message. */
	if (!index.isValid()) {
		return;
	}

	bool singleSelected = true;
	bool received =
	    AccountModel::nodeTypeIsReceived(currentAccountModelIndex());

	QMenu *menu = new QMenu;
	QMenu *submenu = 0;

	/*
	 * Remember last selected message. Pick the first from
	 * the current selection.
	 *
	 * TODO -- Save whole selection?
	 */
	{
		QModelIndexList firstMsgColumnIdxs(
		    currentFrstColMessageIndexes());

		singleSelected = (1 == firstMsgColumnIdxs.size());

		if (!firstMsgColumnIdxs.isEmpty()) {
			messageItemStoreSelection(firstMsgColumnIdxs
			    .first().data().toLongLong());
		}
	}

	/* TODO use QAction::iconText() instead of direct strings here. */

	menu->addAction(ui->actionDownload_message_signed);
	if (singleSelected) {
		menu->addAction(ui->actionReply);
	}
	menu->addAction(ui->actionForward_message);
	if (singleSelected) {
		menu->addAction(ui->actionCreate_message_from_template);
	}
	menu->addSeparator();
	if (singleSelected) {
		menu->addAction(ui->actionSignature_detail);
		menu->addAction(ui->actionAuthenticate_message);
		menu->addSeparator();
		menu->addAction(ui->actionOpen_message_externally);
		menu->addAction(ui->actionOpen_delivery_info_externally);
		menu->addSeparator();
		menu->addAction(ui->actionSend_to_records_management);
		menu->addSeparator();
	}
	menu->addAction(ui->actionExport_as_ZFO);
	menu->addAction(ui->actionExport_delivery_info_as_ZFO);
	menu->addAction(ui->actionExport_delivery_info_as_PDF);
	menu->addAction(ui->actionExport_message_envelope_as_PDF);
	menu->addAction(ui->actionExport_envelope_PDF_and_attachments);
	menu->addSeparator();
	menu->addAction(ui->actionEmail_ZFOs);
	menu->addAction(ui->actionEmail_all_attachments);
	menu->addSeparator();

	if (received) {
		submenu = menu->addMenu(tr("Mark"));
		menu->addSeparator();

		submenu->addAction(tr("As Read"), this,
		    SLOT(messageItemsSelectedMarkRead()));
		submenu->addAction(tr("As Unread"), this,
		    SLOT(messageItemsSelectedMarkUnread()));

		submenu->addSeparator();
		submenu->addAction(tr("As Unsettled"), this,
		    SLOT(messageItemsSelectedMarkUnsettled()));
		submenu->addAction(tr("As in Progress"), this,
		    SLOT(messageItemsSelectedMarkInProgress()));
		submenu->addAction(tr("As Settled"), this,
		    SLOT(messageItemsSelectedMarkSettled()));
	}
	menu->addAction(IconContainer::construcIcon(IconContainer::ICON_LABEL),
	    ui->actionTag_settings->text(), this, SLOT(addOrDeleteMsgTags()));

	menu->addSeparator();

	menu->addAction(ui->actionDelete_message_from_db);

	menu->exec(QCursor::pos());
	menu->deleteLater();
}

void MainWindow::viewSelectedMessage(void)
{
	debugSlotCall();

	QModelIndex msgIndex;
	{
		QModelIndexList msgIndexes(currentFrstColMessageIndexes());

		if (msgIndexes.size() != 1) {
			/* Do nothing when multiple messages selected. */
			return;
		}

		msgIndex = msgIndexes.first();
	}

	if (!msgIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDb::MsgId msgId(msgMsgId(msgIndex));
	Q_ASSERT(msgId.dmId >= 0);

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}
	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return;
	}

	QByteArray msgRaw(messageDb->getCompleteMessageRaw(msgId.dmId));
	if (msgRaw.isEmpty()) {

		if (!messageMissingOfferDownload(msgId,
		        tr("Message export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(msgId.deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			logErrorNL(
			    "Could not access database of freshly downloaded message '%" PRId64 "'.",
			    UGLY_QINT64_CAST msgId.dmId);
			return;
		}

		msgRaw = messageDb->getCompleteMessageRaw(msgId.dmId);
		if (msgRaw.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	/* Generate dialog showing message content. */
	DlgViewZfo::view(msgRaw, this);
}

/* ========================================================================= */
/*
 * Saves message selection.
 */
void MainWindow::messageItemStoreSelection(qint64 msgId)
/* ========================================================================= */
{
	debugSlotCall();

	m_lastSelectedMessageId = msgId;
	qDebug() << "Last selected" << m_lastSelectedMessageId;
	if (-1 == msgId) {
		m_lastSelectedAccountNodeType = AccountModel::nodeUnknown;
		return;
	}

	/*
	 * If we selected a message from last received then store the
	 * selection to the model.
	 */
	QModelIndex acntIdx(currentAccountModelIndex());
	m_lastSelectedAccountNodeType = AccountModel::nodeType(acntIdx);
	if (AccountModel::nodeRecentReceived == m_lastSelectedAccountNodeType) {

		qDebug() << "Storing recent received selection into the model"
		    << msgId;

		const QString userName(m_accountModel.userName(acntIdx));
		Q_ASSERT(!userName.isEmpty());
		(*GlobInstcs::acntMapPtr)[userName].setLastMsg(msgId);
	}
}

void MainWindow::storeExportPath(const QString &userName)
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());

	AcntSettings &accountInfo((*GlobInstcs::acntMapPtr)[userName]);
	accountInfo.setLastAttachSavePath(m_save_attach_dir);
	accountInfo.setLastAttachAddPath(m_add_attach_dir);
	accountInfo.setLastCorrespPath(m_export_correspond_dir);
	accountInfo.setLastZFOExportPath(m_on_export_zfo_activate);
	saveSettings();
}

/* ========================================================================= */
/*
 * Saves message selection when model changes.
 */
void MainWindow::messageItemStoreSelectionOnModelChange(void)
/* ========================================================================= */
{
	debugSlotCall();

	m_lastStoredMessageId = m_lastSelectedMessageId;
	m_lastStoredAccountNodeType = m_lastSelectedAccountNodeType;
	qDebug() << "Last stored position" << m_lastStoredMessageId;
}


/* ========================================================================= */
/*
 * Restores message selection.
 */
void MainWindow::messageItemRestoreSelectionOnModelChange(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex msgIndex;

	const QAbstractItemModel *model = ui->messageList->model();
	Q_ASSERT(0 != model);

	int rowCount = model->rowCount();
	int row = 0;

	QModelIndex acntIdx(currentAccountModelIndex());
	enum AccountModel::NodeType acntNodeType =
	    AccountModel::nodeType(acntIdx);

	if (0 == rowCount) {
		/* Do nothing on empty model. */
		return;
	}

	/* Search for existing ID if account position did not change. */
	if ((-1 != m_lastStoredMessageId) &&
	    (m_lastStoredAccountNodeType == acntNodeType)) {

		/* Find and select the message with the ID. */
		for (row = 0; row < rowCount; ++row) {
			/*
			 * TODO -- Search in a more resource-saving way.
			 * Eliminate index copying, use smarter search.
			 */
			if (model->index(row, 0).data().toLongLong() ==
			    m_lastStoredMessageId) {
				msgIndex = model->index(row, 0);
				break;
			}
		}

		if (msgIndex.isValid()) { /*(row < rowCount)*/
			/* Message found. */
			ui->messageList->setCurrentIndex(msgIndex);
			ui->messageList->scrollTo(msgIndex);
			return;
		}
	}

	/*
	 * If we selected a message from last received then restore the
	 * selection according to the model.
	 */
	switch (GlobInstcs::prefsPtr->afterStartSelect) {
	case Preferences::SELECT_NEWEST:
		/* Search for the message with the largest id. */
		{
			msgIndex = model->index(0, 0);
			qint64 largestSoFar = msgIndex.data().toLongLong();
			for (row = 1; row < rowCount; ++row) {
				/*
				 * TODO -- Search in a more resource-saving
				 * way. Eliminate index copying, use smarter
				 * search.
				 */
				if (largestSoFar < model->index(row, 0).data()
				        .toLongLong()) {
					msgIndex = model->index(row, 0);
					largestSoFar =
					    msgIndex.data().toLongLong();
				}
			}
			if (msgIndex.isValid()) {
				ui->messageList->setCurrentIndex(msgIndex);
				ui->messageList->scrollTo(msgIndex);
			}
		}
		break;
	case Preferences::SELECT_LAST_VISITED:
		{
			qint64 msgLastId = -1;
			if (AccountModel::nodeRecentReceived == acntNodeType) {
				const QString userName(
				    m_accountModel.userName(acntIdx));
				Q_ASSERT(!userName.isEmpty());

				msgLastId =
				    (*GlobInstcs::acntMapPtr)[userName].lastMsg();
			} else {
				msgLastId = m_lastStoredMessageId;
			}
			if (0 <= msgLastId) {
				/* Find and select the message with the ID. */
				for (row = 0; row < rowCount; ++row) {
					/*
					 * TODO -- Search in a more
					 * resource-saving way.
					 * Eliminate index copying, use
					 * smarter search.
					 */
					if (model->index(row, 0)
					        .data().toLongLong()
					    == msgLastId) {
						msgIndex =
						    model->index(row, 0);
						break;
					}
				}
				if (msgIndex.isValid()) { /*(row < rowCount)*/
					ui->messageList->setCurrentIndex(
					    msgIndex);
					ui->messageList->scrollTo(msgIndex);
				}
			}
		}
		break;
	case Preferences::SELECT_NOTHING:
		/* Don't select anything. */
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}


/* ========================================================================= */
/*
 * Restores message selection.
 */
void MainWindow::messageItemRestoreSelectionAfterLayoutChange(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex msgIndex;

	const QAbstractItemModel *model = ui->messageList->model();
	Q_ASSERT(0 != model);

	int rowCount = model->rowCount();
	int row = 0;

	if (0 == rowCount) {
		/* Do nothing on empty model. */
		return;
	}

	/* If the ID does not exist then don't search for it. */
	if (-1 == m_lastSelectedMessageId) {
		row = rowCount;
	}

	/* Find and select the message with the ID. */
	for (; row < rowCount; ++row) {
		/*
		 * TODO -- Search in a more resource-saving way.
		 * Eliminate index copying, use smarter search.
		 */
		if (model->index(row, 0).data().toLongLong() ==
		    m_lastSelectedMessageId) {
			msgIndex = model->index(row, 0);
			break;
		}
	}

	if (msgIndex.isValid()) { /*(row < rowCount)*/
		/* Message found. */
		ui->messageList->setCurrentIndex(msgIndex);
		ui->messageList->scrollTo(msgIndex);
	}
}


/* ========================================================================= */
/*
 * Return index for yearly entry with given properties.
 */
QModelIndex MainWindow::accountYearlyIndex(const QString &userName,
    const QString &year, int msgType)
/* ========================================================================= */
{
	debugFuncCall();

	/* first step: search correspond account index from username */
	QModelIndex acntIdxTop = m_accountModel.topAcntIndex(userName);

	if (!acntIdxTop.isValid()) {
		return QModelIndex();
	}

	/* second step: obtain index of received or sent messages */
	QModelIndex typeIdx;
	switch (msgType) {
	case MessageDb::TYPE_RECEIVED:
		typeIdx = acntIdxTop.child(2, 0).child(0, 0); /* All received. */
		break;
	case MessageDb::TYPE_SENT:
		typeIdx = acntIdxTop.child(2, 0).child(1, 0); /* All sent. */
		break;
	default:
		Q_ASSERT(0);
		return QModelIndex();
		break;
	}

	/* third step: obtain index with given year */
	int childRow = 0;
	QModelIndex yearIdx = typeIdx.child(childRow, 0);
	while (yearIdx.isValid() &&
	   (yearIdx.data(ROLE_PLAIN_DISPLAY).toString() != year)) {
		yearIdx = yearIdx.sibling(++childRow, 0);
	}

	if (!yearIdx.isValid()) {
		return QModelIndex();
	}

	return yearIdx;
}

/* ========================================================================= */
/*
 * Select account via userName and focus on message ID from search selection.
 */
void MainWindow::messageItemFromSearchSelection(const QString &userName,
    qint64 msgId, const QString &deliveryYear, int msgType)
/* ========================================================================= */
{
	debugSlotCall();

	/* If the ID does not exist then don't search for it. */
	if (-1 == msgId) {
		return;
	}

	/* first step: navigate year entry in account list */
	QModelIndex yearIdx(accountYearlyIndex(userName, deliveryYear,
	    msgType));
	if (!yearIdx.isValid()) {
		return;
	}

	ui->accountList->setCurrentIndex(yearIdx);
	accountItemCurrentChanged(yearIdx);

	/* second step: find and select message according to msgId */
	QModelIndex msgIdx(messageIndex(ui->messageList, msgId));
	if (msgIdx.isValid()) {
		ui->messageList->setCurrentIndex(msgIdx);
		ui->messageList->scrollTo(msgIdx);
	}
}


/* ========================================================================= */
/*
 * Redraws widgets according to attachment item selection.
 */
void MainWindow::attachmentItemsSelectionChanged(
    const QItemSelection &selected, const QItemSelection &deselected)
/* ========================================================================= */
{
	debugSlotCall();

	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	setAttachmentActionVisibility(
	    currentFrstColAttachmentIndexes().size());
}


/* ========================================================================= */
/*
 * Generates menu to selected message item.
 *     (And redraws widgets.)
 */
void MainWindow::attachmentItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	debugSlotCall();

	{
		QModelIndex index = ui->messageAttachmentList->indexAt(point);
		if (!index.isValid()) {
			/* Do nothing. */
			return;
		}
	}

	QModelIndexList attachmentIndexes(currentFrstColAttachmentIndexes());
	Q_ASSERT(attachmentIndexes.size() > 0);

	QMenu *menu = new QMenu;

	if (attachmentIndexes.size() == 1) {
		menu->addAction(ui->actionOpen_attachment);
	}
	menu->addAction(ui->actionSave_selected_attachments);
	menu->addSeparator();
	menu->addAction(ui->actionEmail_selected_attachments);

	menu->exec(QCursor::pos());
	menu->deleteLater();
}

void MainWindow::saveSelectedAttachmentsToFile(void)
{
	debugSlotCall();

	QModelIndexList attachmentIndexes(currentFrstColAttachmentIndexes());

	QModelIndex messageIndex(currentSingleMessageIndex());
	if (!messageIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	const MessageDb::MsgId msgId(msgMsgId(messageIndex));

	foreach (const QModelIndex &attachmentIndex, attachmentIndexes) {
		saveAttachmentToFile(userName, msgId, attachmentIndex);
	}
}

void MainWindow::saveAttachmentToFile(const QString &userName,
    const MessageDb::MsgId &msgId, const QModelIndex &attIdx)
{
	if (!attIdx.isValid()) {
		Q_ASSERT(0);
		return;
	}

	QModelIndex fileNameIndex(attIdx.sibling(attIdx.row(),
	    AttachmentTblModel::FNAME_COL));
	Q_ASSERT(fileNameIndex.isValid());
	if(!fileNameIndex.isValid()) {
		showStatusTextWithTimeout(
		    tr("Saving attachment of message '%1' to files "
		    "was not successful!").arg(msgId.dmId));
		return;
	}
	QString filename(fileNameIndex.data().toString());

	setAccountStoragePaths(userName);

	QString saveAttachPath;
	if (GlobInstcs::prefsPtr->useGlobalPaths) {
		saveAttachPath = GlobInstcs::prefsPtr->saveAttachmentsPath;
	} else {
		saveAttachPath = m_save_attach_dir;
	}

	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	QString fileName(Exports::attachmentSavePathWithFileName(*dbSet,
	    saveAttachPath, filename, dbId, userName, accountName, msgId, true));

	QString savedFileName(AttachmentInteraction::saveAttachmentToFile(this,
	    attIdx, fileName));
	if (!savedFileName.isEmpty()) {
		showStatusTextWithTimeout(tr(
		    "Saving attachment of message '%1' to file was successful.")
		    .arg(msgId.dmId));
		if (!GlobInstcs::prefsPtr->useGlobalPaths) {
			m_save_attach_dir =
			    QFileInfo(savedFileName).absoluteDir()
			        .absolutePath();
			storeExportPath(userName);
		}
	} else {
		showStatusTextWithTimeout(tr(
		    "Saving attachment of message '%1' to file "
		    "was not successful!").arg(msgId.dmId));
	}
}

void MainWindow::saveAllAttachmentsToDir(void)
{
	debugSlotCall();

	QString attSaveDir;
	QString errStr;

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDb::MsgId msgId(msgMsgId(currentFrstColMessageIndexes().first()));

	// is complete message downloaded? if not, download now
	AttachmentTblModel *attachModel = qobject_cast<AttachmentTblModel *>(
	    ui->messageAttachmentList->model());
	if ((!msgId.deliveryTime.isValid()) ||
	    (attachModel == Q_NULLPTR) || (attachModel->rowCount() == 0)) {
		if (!messageMissingOfferDownload(msgId, errStr)) {
			return;
		}
	}

	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	setAccountStoragePaths(userName);

	// how does default path used for?
	if (GlobInstcs::prefsPtr->useGlobalPaths) {
		attSaveDir = GlobInstcs::prefsPtr->saveAttachmentsPath;
	} else {
		attSaveDir = m_save_attach_dir;
	}

	// change target path
	attSaveDir = QFileDialog::getExistingDirectory(this,
	    tr("Select target folder to save"), attSaveDir,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (attSaveDir.isEmpty()) {
		return;
	}

	// store selected path
	if ((!GlobInstcs::prefsPtr->useGlobalPaths) && (!attSaveDir.isEmpty())) {
		m_save_attach_dir = attSaveDir;
		storeExportPath(userName);
	}

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	// save attachments and export zfo/pdf files
	if (Exports::EXP_SUCCESS ==
	    Exports::saveAttachmentsWithExports(*dbSet, attSaveDir,
	    userName, accountName, dbId, msgId, errStr)) {
		showStatusTextWithTimeout(errStr);
	} else {
		showStatusTextWithTimeout(errStr);
		QMessageBox::warning(this, tr("Error saving of attachments"),
		    tr("Some attachments of message '%1' were not saved "
		    "to target folder!").arg(msgId.dmId), QMessageBox::Ok);
	}
}

void MainWindow::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	QString attachName;
	QString tmpPath;

	if (AttachmentInteraction::openAttachment(this,
	        *ui->messageAttachmentList, index,
	        &attachName, &tmpPath)) {
		showStatusTextWithTimeout(tr(
		    "Attachment '%1' stored into temporary file '%2'.")
		    .arg(attachName).arg(tmpPath));
	} else {
		showStatusTextWithTimeout(tr(
		    "Attachment '%1' couldn't be stored into temporary file.")
		    .arg(attachName));
	}
}

void MainWindow::processSingleInstanceMessages(int msgType,
    const QString &msgVal)
{
	debugSlotCall();

	logDebugLv0NL("Received message '%d>%s'.", msgType,
	    msgVal.toUtf8().constData());

	switch (msgType) {
	case SingleInstance::MTYPE_RAISE_MAIN_WIN:
		this->show();
		this->raise();
		this->activateWindow();
		break;
	case SingleInstance::MTYPE_COMPOSE:
		if (m_accountModel.rowCount() > 0) {
			showSendMessageDialog(DlgSendMessage::ACT_NEW, msgVal);
		} else {
			QMessageBox::information(this,
			    tr("Cannot create message"),
			    tr("Create an user account first before trying to create and send a message."),
			    QMessageBox::Ok, QMessageBox::Ok);
		}
		break;
	default:
		break;
	}
}

void MainWindow::backgroundWorkersFinished(void)
{
	debugSlotCall();

	int accountCount = ui->accountList->model()->rowCount();
	if (accountCount > 0) {
		ui->actionSync_all_accounts->setEnabled(true);
		ui->actionGet_messages->setEnabled(true);

		/* Activate import buttons. */
		ui->actionImport_messages_from_database->setEnabled(true);
		ui->actionImport_ZFO_file_into_database->setEnabled(true);
	}
	/*
	 * Prepare counters for next action.
	 * TODO -- This must be moved to separate slot that waits for downloads.
	 */
	dataFromWorkerToStatusBarInfo(false, 0, 0, 0, 0);

	if (GlobInstcs::prefsPtr->downloadOnBackground) {
		m_timerSyncAccounts.start(m_timeoutSyncAccounts);
	}
}

void MainWindow::collectDownloadMessageStatus(const QString &usrName,
    qint64 msgId, const QDateTime &deliveryTime, int result,
    const QString &errDesc, bool listScheduled, int processFlags)
{
	debugSlotCall();

	Q_UNUSED(deliveryTime);

	if (TaskDownloadMessage::DM_SUCCESS == result) {
		/* Refresh account and attachment list. */
		refreshAccountList(usrName);

		if (0 <= msgId) {
			postDownloadSelectedMessageAttachments(usrName, msgId);
		}
	} else {
		/* Notify the user. */
		if (!listScheduled) {
			showStatusTextWithTimeout(tr("It was not possible download "
			    "complete message \"%1\" from ISDS server.").arg(msgId));

			QString infoText(!errDesc.isEmpty() ?
			    tr("ISDS") + QLatin1String(": ") + errDesc :
			    tr("A connection error occurred or the message has already been deleted from the server."));
			DlgMsgBox::message(this, QMessageBox::Warning,
			    tr("Download message error"),
			    tr("It was not possible to download a complete message \"%1\" from ISDS server.")
			        .arg(msgId),
			    infoText, QString(), QMessageBox::Ok,
			    QMessageBox::Ok);
		} else {
			showStatusTextWithTimeout(
			    tr("Couldn't download message '%1'.").arg(msgId));
		}
	}

	if ((TaskDownloadMessage::DM_SUCCESS == result) &&
	    (processFlags & Task::PROC_IMM_RM_UPLOAD)) {
		sendMessageToRecordsManagement(usrName, MessageDb::MsgId(msgId, deliveryTime));
	}
}

void MainWindow::collectDownloadMessageListStatus(const QString &usrName,
    int direction, int result, const QString &errDesc,
    bool add, int rt, int rn, int st, int sn)
{
	debugSlotCall();

	/* Refresh account list. */
	refreshAccountList(usrName);

	dataFromWorkerToStatusBarInfo(add, rt, rn, st, sn);

	if (TaskDownloadMessageList::DL_SUCCESS != result) {
		/* Notify the user. */
		QString errorMessage = (MSG_RECEIVED == direction) ?
		    tr("It was not possible download received message list from"
		        " server.") :
		    tr("It was not possible download sent message list from"
		        " server.");

		showStatusTextWithTimeout(errorMessage);

		QString infoText(!errDesc.isEmpty() ?
		    tr("Server") + QLatin1String(": ") + errDesc :
		    tr("A connection error occurred."));
		DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Download message list error"), errorMessage, infoText,
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
	}
}

/*!
 * @brief ZFO import context singleton.
 */
class ZFOImportCtx {
private:
	/*!
	 * @brief Constructor.
	 */
	ZFOImportCtx(void)
	    : zfoFilesToImport(), numFilesToImport(0),
	    importSucceeded(), importExisted(), importFailed()
	{
	}

public:
	/*!
	 * @brief Return reference to singleton instance.
	 */
	static
	ZFOImportCtx &getInstance(void)
	{
		static
		ZFOImportCtx ctx;

		return ctx;
	}

	/*!
	 * @brief Erase content.
	 */
	void clear(void)
	{
		zfoFilesToImport.clear();
		numFilesToImport = 0;
		importSucceeded.clear();
		importExisted.clear();
		importFailed.clear();
	}

	QSet<QString> zfoFilesToImport; /*!< Set of names of files to be imported. */
	int numFilesToImport; /*!< Input ZFO count. */
	/*
	 * QPair in following lists means:
	 * first string - ZFO file name,
	 * second - import result text
	 */
	QList< QPair<QString, QString> > importSucceeded; /*!< Successful import result lists. */
	QList< QPair<QString, QString> > importExisted; /*!< Import existed result lists. */
	QList< QPair<QString, QString> > importFailed; /*!< Import error result lists. */
};

void MainWindow::collectImportZfoStatus(const QString &fileName, int result,
    const QString &resultDesc)
{
	debugSlotCall();

	logDebugLv0NL("Received import ZFO finished for file '%s' %d: '%s'",
	    fileName.toUtf8().constData(), result,
	    resultDesc.toUtf8().constData());

	ZFOImportCtx &importCtx(ZFOImportCtx::getInstance());

	switch (result) {
	case TaskImportZfo::IMP_SUCCESS:
		importCtx.importSucceeded.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	case TaskImportZfo::IMP_DB_EXISTS:
		importCtx.importExisted.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	default:
		importCtx.importFailed.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	}

	if (!importCtx.zfoFilesToImport.remove(fileName)) {
		logErrorNL(
		    "Processed ZFO file which '%s' the application has not been aware of.",
		    fileName.toUtf8().constData());
	}

	if (importCtx.zfoFilesToImport.isEmpty()) {
		DlgImportZFOResult::view(importCtx.numFilesToImport,
		    importCtx.importSucceeded, importCtx.importExisted,
		    importCtx.importFailed, this);

		importCtx.clear();

		/* Activate import buttons. */
		ui->actionImport_messages_from_database->setEnabled(true);
		ui->actionImport_ZFO_file_into_database->setEnabled(true);
	}
}

void MainWindow::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId, int processFlags)
{
	debugSlotCall();

	Q_UNUSED(userName);
	Q_UNUSED(transactId);
	Q_UNUSED(resultDesc);
	Q_UNUSED(isPDZ);
	Q_UNUSED(dmId);

	if (TaskSendMessage::SM_SUCCESS == result) {
		showStatusTextWithTimeout(tr(
		    "Message from '%1' (%2) has been successfully sent to '%3' (%4).").
		    arg((*GlobInstcs::acntMapPtr)[userName].accountName()).
		    arg(userName).arg(recipientName).arg(dbIDRecipient));

		/* Refresh account list. */
		refreshAccountList(userName);
	} else {
		showStatusTextWithTimeout(tr(
		    "Error while sending message from '%1' (%2) to '%3' (%4).").
		    arg((*GlobInstcs::acntMapPtr)[userName].accountName()).
		    arg(userName).arg(recipientName).arg(dbIDRecipient));
	}

	clearProgressBar();

	if ((TaskSendMessage::SM_SUCCESS == result) &&
	    (processFlags & Task::PROC_IMM_DOWNLOAD)) {
		/* Schedule message download. */
		TaskDownloadMessage *task =
		    new (std::nothrow) TaskDownloadMessage(
		        userName, accountDbSet(userName), MSG_SENT,
		        MessageDb::MsgId(dmId, QDateTime()), false,
		        processFlags & ~Task::PROC_IMM_DOWNLOAD);
		if (Q_UNLIKELY(task == Q_NULLPTR)) {
			Q_ASSERT(0);
			return;
		}
		task->setAutoDelete(true);
		GlobInstcs::workPoolPtr->assignHi(task, WorkerPool::PREPEND);
	}
}

void MainWindow::postDownloadSelectedMessageAttachments(
    const QString &userName, qint64 dmId)
{
	debugFuncCall();

	showStatusTextWithTimeout(
	    tr("Message '%1' was downloaded from server.").arg(dmId));

	const QString currentUserName(
	    m_accountModel.userName(currentAccountModelIndex()));
	const QModelIndex currentMsgIdx(ui->messageList->currentIndex());

	if (Q_UNLIKELY(currentUserName.isEmpty() || !currentMsgIdx.isValid())) {
		/*
		 * Current index may be empty when the account is changed and
		 * the message list looses focus.
		 */
		return;
	}

	/* Get message ID from model index. */
	qint64 currentDmId = currentMsgIdx.data().toLongLong();

	/* Do nothing if account or message was changed. */
	if ((userName != currentUserName) || (dmId != currentDmId)) {
		return;
	}

	/*
	 * Mark message as having attachment downloaded without reloading
	 * the whole model.
	 */
	m_messageTableModel.overrideDownloaded(dmId, true);
	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();
	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	if (1 != currentFrstColMessageIndexes().size()) {
		return;
	}

	/*
	 * TODO -- Create a separate function for reloading attachment
	 * contents. Similar code is used for handling message selection
	 * changes.
	 */

	/* Disconnect model from slot if model already set. */
	if (Q_NULLPTR != ui->messageAttachmentList->selectionModel()) {
		/* New model hasn't been set yet. */
		ui->messageAttachmentList->selectionModel()->disconnect(
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(attachmentItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_UNLIKELY(Q_NULLPTR == dbSet)) {
		Q_ASSERT(0);
		return;
	}
	QDateTime deliveryTime = msgDeliveryTime(currentMsgIdx);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	if (Q_UNLIKELY(Q_NULLPTR == messageDb)) {
		Q_ASSERT(0);
		return;
	}

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(dmId));
	ui->messageInfo->setReadOnly(true);

	m_attachmentModel.removeRows(0, m_attachmentModel.rowCount());
	m_attachmentModel.setHeader();
	m_attachmentModel.appendData(messageDb->attachEntries(dmId));
	ui->messageAttachmentList->setModel(&m_attachmentModel);
	/* First three columns contain hidden data. */
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::ATTACHID_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::MSGID_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::BINARY_CONTENT_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::MIME_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    AttachmentTblModel::FPATH_COL, true);

	if (ui->messageAttachmentList->model()->rowCount() > 0) {
		ui->actionSave_all_attachments->setEnabled(true);
	} else {
		ui->actionSave_all_attachments->setEnabled(false);
	}

	ui->messageAttachmentList->resizeColumnToContents(
	    AttachmentTblModel::FNAME_COL);

	/* Connect new slot. */
	connect(ui->messageAttachmentList->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(attachmentItemsSelectionChanged(QItemSelection,
	        QItemSelection)));
}

/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedLocallyRead(bool read)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	dbSet->smsgdtSetAllReceivedLocallyRead(read);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(acntIdx);

	/* Regenerate the model. */
	accountItemCurrentChanged(acntIdx);
}


/* ========================================================================= */
/*
 * Mark all messages in the current working account.
 */
void MainWindow::accountMarkReceivedRead(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedLocallyRead(true);
}



/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedUnread(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedLocallyRead(false);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearLocallyRead(bool read)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}
	/*
	 * Data cannot be read directly from index because to the overloaded
	 * model functions.
	 * TODO -- Parameter check.
	 */
	dbSet->smsgdtSetReceivedYearLocallyRead(
	    acntIdx.data(ROLE_PLAIN_DISPLAY).toString(), read);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(acntIdx);

	/* Regenerate the model. */
	accountItemCurrentChanged(acntIdx);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearRead(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedYearLocallyRead(true);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearUnread(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedYearLocallyRead(false);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedLocallyRead(bool read)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	dbSet->smsgdtSetWithin90DaysReceivedLocallyRead(read);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(acntIdx);

	/* Regenerate the model. */
	accountItemCurrentChanged(acntIdx);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedRead(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkRecentReceivedLocallyRead(true);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedUnread(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkRecentReceivedLocallyRead(false);
}


/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedProcessState(
    enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	dbSet->setReceivedMessagesProcessState(state);

	/*
	 * No need to reload account model.
	 */

	/* Regenerate the model. */
	accountItemCurrentChanged(acntIdx);
}


/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedUnsettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedProcessState(UNSETTLED);
}


/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedInProgress(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedProcessState(IN_PROGRESS);
}


/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedSettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedProcessState(SETTLED);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearProcessState(
    enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}
	/*
	 * Data cannot be read directly from index because to the overloaded
	 * model functions.
	 * TODO -- Parameter check.
	 */
	dbSet->smsgdtSetReceivedYearProcessState(
	    acntIdx.data(ROLE_PLAIN_DISPLAY).toString(), state);

	/*
	 * No need to reload account model.
	 */

	/* Regenerate the model. */
	accountItemCurrentChanged(acntIdx);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearUnsettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedYearProcessState(UNSETTLED);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearInProgress(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedYearProcessState(IN_PROGRESS);
}


/* ========================================================================= */
/*
 * Mark all received messages in given year in the current
 *     working account.
 */
void MainWindow::accountMarkReceivedYearSettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkReceivedYearProcessState(SETTLED);
}


/* ========================================================================= */
/*
 * Mark recently received messages in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedProcessState(
    enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	dbSet->smsgdtSetWithin90DaysReceivedProcessState(state);

	/*
	 * No need to reload account model.
	 */

	/* Regenerate the model. */
	accountItemCurrentChanged(acntIdx);
}


/* ========================================================================= */
/*
 * Mark recently received messages in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedUnsettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkRecentReceivedProcessState(UNSETTLED);
}


/* ========================================================================= */
/*
 * Mark recently received messages in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedInProgress(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkRecentReceivedProcessState(IN_PROGRESS);
}


/* ========================================================================= */
/*
 * Mark recently received messages in the current
 *     working account.
 */
void MainWindow::accountMarkRecentReceivedSettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	accountMarkRecentReceivedProcessState(SETTLED);
}


/* ========================================================================= */
/*
 * Mark selected messages as read.
 */
void MainWindow::messageItemsSelectedMarkRead(void)
/* ========================================================================= */
{
	debugSlotCall();

	messageItemsSetReadStatus(currentFrstColMessageIndexes(), true);
}


/* ========================================================================= */
/*
 * Mark selected messages as unread.
 */
void MainWindow::messageItemsSelectedMarkUnread(void)
/* ========================================================================= */
{
	debugSlotCall();

	messageItemsSetReadStatus(currentFrstColMessageIndexes(), false);
}


/* ========================================================================= */
/*
 * Mark selected messages as unsettled.
 */
void MainWindow::messageItemsSelectedMarkUnsettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	messageItemsSetProcessStatus(currentFrstColMessageIndexes(), UNSETTLED);
}


/* ========================================================================= */
/*
 * Mark selected messages as in progress.
 */
void MainWindow::messageItemsSelectedMarkInProgress(void)
/* ========================================================================= */
{
	debugSlotCall();

	messageItemsSetProcessStatus(currentFrstColMessageIndexes(), IN_PROGRESS);
}


/* ========================================================================= */
/*
 * Mark selected messages as settled.
 */
void MainWindow::messageItemsSelectedMarkSettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	messageItemsSetProcessStatus(currentFrstColMessageIndexes(), SETTLED);
}


/* ========================================================================= */
/*
 * Delete selected message from local database and ISDS.
 */
void MainWindow::deleteMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	QString dlgTitleText, questionText, checkBoxText, detailText;

	int msgIdxCnt = firstMsgColumnIdxs.size();
	if (1 == msgIdxCnt) {
		qint64 dmId = firstMsgColumnIdxs.first().data().toLongLong();
		dlgTitleText = tr("Delete message %1").arg(dmId);
		questionText = tr("Do you want to delete "
		    "message '%1'?").arg(dmId);
		checkBoxText = tr("Delete this message also from server ISDS");
		detailText = tr("Warning: If you delete the message "
		    "from ISDS then this message will be lost forever.");
	} else {
		dlgTitleText = tr("Delete messages");
		questionText = tr("Do you want to delete selected messages?");
		checkBoxText =
		    tr("Delete these messages also from server ISDS");
		detailText = tr("Warning: If you delete selected messages "
		    "from ISDS then these messages will be lost forever.");
	}

	QDialog *yesNoCheckDlg = new DlgYesNoCheckbox(dlgTitleText,
	    questionText, checkBoxText, detailText, this);
	int retVal = yesNoCheckDlg->exec();
	yesNoCheckDlg->deleteLater();
	bool delMsgIsds = false;

	if (retVal == DlgYesNoCheckbox::YesChecked) {
		/* Delete message(s) in the local db and ISDS */
		delMsgIsds = true;
	} else if (retVal == DlgYesNoCheckbox::YesUnchecked) {
		/* Delete message(s) only local */
		delMsgIsds = false;
	} else {
		/* Cancel delete action */
		return;
	}

	/* Save current account index */
	QModelIndex selectedAcntIndex(currentAccountModelIndex());

	foreach (const MessageDb::MsgId &id, msgMsgIds(firstMsgColumnIdxs)) {
		if (eraseMessage(userName, id, delMsgIsds)) {
			/*
			 * Hiding selected line in the message model actually
			 * does not help. The model contains all the old data
			 * and causes problems. Therefore the model must be
			 * regenerated.
			 */
			if (selectedAcntIndex.isValid()) {
				accountItemCurrentChanged(selectedAcntIndex);
			}
			/*
			 * TODO -- Remove the year on account list if last
			 * message was removed.
			 */

			/* Delete all tags from message_tags table.
			 * Tag in the tag table are kept.
			 */
			GlobInstcs::tagDbPtr->removeAllTagsFromMsg(userName,
			    id.dmId);
		}
	}

	/* Refresh account list. */
	refreshAccountList(userName);
}

/* ========================================================================= */
/*
 * Delete message from long term storage in ISDS and
 * local database - based on action parameter.
*/
bool MainWindow::eraseMessage(const QString &userName,
    const MessageDb::MsgId &msgId, bool delFromIsds)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return false;
	}

	enum MessageDirection msgDirect =
	    messageDirection(currentAccountModelIndex(), MSG_RECEIVED);

	if (delFromIsds &&
	    !GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		logErrorNL(
		    "Couldn't connect to ISDS when erasing message '%" PRId64 "'.",
		    UGLY_QINT64_CAST msgId.dmId);
		return false;
	}

	QString errorStr, longErrorStr;
	TaskEraseMessage *task;

	task = new (std::nothrow) TaskEraseMessage(userName, dbSet, msgId,
	    msgDirect, delFromIsds);
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	TaskEraseMessage::Result result = task->m_result;
	errorStr = task->m_isdsError;
	longErrorStr = task->m_isdsLongError;
	delete task;

	switch (result) {
	case TaskEraseMessage::NOT_DELETED:
		logErrorNL("Message '%" PRId64 "' couldn't be deleted.",
		    UGLY_QINT64_CAST msgId.dmId);
		showStatusTextWithTimeout(tr("Message \"%1\" was not deleted.")
		    .arg(msgId.dmId));
		return false;
		break;
	case TaskEraseMessage::DELETED_ISDS:
		logWarning("Message '%" PRId64 "' deleted only from ISDS.\n",
		    UGLY_QINT64_CAST msgId.dmId);
		showStatusTextWithTimeout(tr(
		    "Message \"%1\" was deleted only from ISDS.")
		    .arg(msgId.dmId));
		return false;
		break;
	case TaskEraseMessage::DELETED_LOCAL:
		if (delFromIsds) {
			logWarning(
			    "Message '%" PRId64 "' deleted only from local database.\n",
			    UGLY_QINT64_CAST msgId.dmId);
			showStatusTextWithTimeout(tr(
			    "Message \"%1\" was deleted only from local database.")
			    .arg(msgId.dmId));
		} else {
			logInfo(
			    "Message '%" PRId64 "' deleted from local database.\n",
			    UGLY_QINT64_CAST msgId.dmId);
			showStatusTextWithTimeout(tr(
			    "Message \"%1\" was deleted from local database.")
			    .arg(msgId.dmId));
		}
		return true;
		break;
	case TaskEraseMessage::DELETED_ISDS_LOCAL:
		logInfo(
		    "Message '%" PRId64 "' deleted from ISDS and local database.\n",
		    UGLY_QINT64_CAST msgId.dmId);
		showStatusTextWithTimeout(tr(
		    "Message \"%1\" was deleted from ISDS and local database.")
		    .arg(msgId.dmId));
		return true;
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}


/* ========================================================================= */
/*
* Set info status bar from worker.
*/
void MainWindow::dataFromWorkerToStatusBarInfo(bool add,
    int rt, int rn, int st, int sn)
/* ========================================================================= */
{
	debugFuncCall();

	static int s_rt = 0, s_rn = 0, s_st = 0, s_sn = 0;

	if (add) {
		s_rt += rt;
		s_rn += rn;
		s_st += st;
		s_sn += sn;
		showStatusTextWithTimeout(tr("Messages on the server") + ": " +
		    QString::number(s_rt) + " " + tr("received") +
		    " (" + QString::number(s_rn) + " " + tr("new") + "); " +
		    QString::number(s_st) + " " + tr("sent") +
		    " (" + QString::number(s_sn) + " " + tr("new") + ")");
	} else {
		s_rt = rt;
		s_rn = rn;
		s_st = st;
		s_sn = sn;
	}
}


/* ========================================================================= */
/*
 * Downloads new messages from server for all accounts.
 */
void MainWindow::synchroniseAllAccounts(void)
/* ========================================================================= */
{
	debugSlotCall();

	showStatusTextPermanently(
	    tr("Synchronise all accounts with ISDS server."));

	if (GlobInstcs::prefsPtr->downloadOnBackground) {
		m_timerSyncAccounts.stop();
	}

	int accountCount = ui->accountList->model()->rowCount();
	bool appended = false;

	for (int i = 0; i < accountCount; ++i) {

		QModelIndex index = m_accountModel.index(i, 0);

		const QString userName(m_accountModel.userName(index));
		Q_ASSERT(!userName.isEmpty());

		/* Skip those that should omitted. */
		if (!(*GlobInstcs::acntMapPtr)[userName].syncWithAll()) {
			continue;
		}

		/* Try connecting to ISDS, just to generate log-in dialogue. */
		if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
		    !connectToIsds(userName)) {
			continue;
		}

		if (synchroniseSelectedAccount(userName)) {
			appended = true;
		}
	}

	if (!appended) {
		showStatusTextWithTimeout(tr("No account synchronised."));
		if (GlobInstcs::prefsPtr->downloadOnBackground) {
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
		}
		return;
	}
}


/* ========================================================================= */
/*
* Download sent/received message list for current (selected) account
*/
bool MainWindow::synchroniseSelectedAccount(QString userName)
/* ========================================================================= */
{
	debugSlotCall();

	/*
	 * TODO -- Save/restore the position of selected account and message.
	 */

	if (userName.isEmpty()) {
		userName = m_accountModel.userName(currentAccountModelIndex());
		Q_ASSERT(!userName.isEmpty());
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		return false;
	}

	bool wasEnabled = ui->actionSync_all_accounts->isEnabled();

	{
		/*
		 * Disabling buttons is not as easy as it is handled by
		 * the event loop. The event loop is forced to process all
		 * pending events to minimise the probability that the actions
		 * remain enabled accidentally.
		 *
		 * TODO -- The checking for live ISDS connection must be
		 * guarded for simultaneous access.
		 */
		QCoreApplication::processEvents();
		ui->actionSync_all_accounts->setEnabled(false);
		ui->actionGet_messages->setEnabled(false);
		QCoreApplication::processEvents();
	}

	/* Try connecting to ISDS, just to generate log-in dialogue. */
	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		ui->actionSync_all_accounts->setEnabled(wasEnabled);
		ui->actionGet_messages->setEnabled(wasEnabled);
		return false;
	}

	/* Method connectToIsds() acquires account information. */

	bool downloadReceivedMessages =
	    GlobInstcs::prefsPtr->autoDownloadWholeMessages;
	if (downloadReceivedMessages) {
		/* Method connectToIsds() acquires account information. */
		const QString acntDbKey(AccountDb::keyFromLogin(userName));
		DbEntry userEntry = GlobInstcs::accntDbPtr->userEntry(acntDbKey);
		const QString key("userPrivils");
		if (userEntry.hasValue(key)) {
			int privils = userEntry.value(key).toInt();
			if (!(privils & (Isds::Type::PRIVIL_READ_NON_PERSONAL | Isds::Type::PRIVIL_READ_ALL))) {
				logInfo(
				    "User '%s' has no privileges to download received messages. Won't try downloading messages.\n",
				    userName.toUtf8().constData());
				downloadReceivedMessages = false;
			}
		}
	}

	TaskDownloadMessageList *task;

	task = new (std::nothrow) TaskDownloadMessageList(userName, dbSet,
	    MSG_RECEIVED, downloadReceivedMessages);
	task->setAutoDelete(true);
	GlobInstcs::workPoolPtr->assignLo(task);

	task = new (std::nothrow) TaskDownloadMessageList(userName, dbSet,
	    MSG_SENT, GlobInstcs::prefsPtr->autoDownloadWholeMessages);
	task->setAutoDelete(true);
	GlobInstcs::workPoolPtr->assignLo(task);

	return true;
}

void MainWindow::downloadSelectedMessageAttachments(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	const QModelIndex acntIdx(currentAccountModelIndex());
	enum MessageDirection msgDirect =
	    messageDirection(acntIdx, MSG_RECEIVED);
	const QString userName(m_accountModel.userName(acntIdx));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		return;
	}

	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);

	foreach (const MessageDb::MsgId &id, msgMsgIds(firstMsgColumnIdxs)) {
		/* Using prepend() just to outrun other jobs. */
		TaskDownloadMessage *task =
		    new (std::nothrow) TaskDownloadMessage(
		        userName, dbSet, msgDirect, id, false);
		if (Q_UNLIKELY(task == Q_NULLPTR)) {
			Q_ASSERT(0);
			continue;
		}
		task->setAutoDelete(true);
		GlobInstcs::workPoolPtr->assignLo(task, WorkerPool::PREPEND);
	}
}

/* ========================================================================= */
/*
 * Generate account info HTML message.
 */
QString MainWindow::createAccountInfo(const QString &userName)
/* ========================================================================= */
{
	Q_ASSERT(!userName.isEmpty());

	QString html;
	DbEntry userEntry;
	DbEntry accountEntry;

	html.append(indentDivStart);
	html.append("<h3>");


	if ((*GlobInstcs::acntMapPtr)[userName].isTestAccount()) {
		html.append(tr("Test account"));
	} else {
		html.append(tr("Standard account"));
	}
	html.append("<br/><img src=\":/dschranka.png\" alt=\"ISDS\">");
	html.append("</h3>");

	html.append(strongAccountInfoLine(tr("Account name"),
	    (*GlobInstcs::acntMapPtr)[userName].accountName()));

	const QString acntDbKey(AccountDb::keyFromLogin(userName));
	if (GlobInstcs::accntDbPtr->dbId(acntDbKey).isEmpty()) {
		/*
		 * Generate this message if no account information can
		 * be obtained.
		 */
		html.append(QString("<div><strong>") +
		    tr("Account and user information could not be acquired.") +
		    QString("</strong></div>"));
		goto lastPart;
	}

	html.append("<table cellpadding=\"5\"><tr><td>");
	html.append(QString("<div><strong>") + tr("User information") +
	    QString("</strong></div>"));
	html.append("</td><td width=\"100\"></td><td>");
	html.append(QString("<div><strong>") + tr("Databox information") +
	    QString("</strong></div>"));
	html.append("</td></tr><tr><td>");

	userEntry = GlobInstcs::accntDbPtr->userEntry(acntDbKey);
	html.append(strongAccountInfoLine(tr("User name"), userName));
	/* Print non-empty entries. */
	for (int i = 0; i < userinfTbl.knownAttrs.size(); ++i) {
		const QString &key = userinfTbl.knownAttrs[i].first;
		if (userEntry.hasValue(key) &&
		    !userinfTbl.attrProps[key].desc.isEmpty()) {
			switch (userinfTbl.knownAttrs[i].second) {
			case DB_INTEGER:
				if (key == "ic") {
					if (userEntry.value(key).toInt() > 0) {
						html.append(strongAccountInfoLine(
						    userinfTbl.attrProps[key].desc,
						    QString::number(userEntry.
						    value(key).toInt())));
					}
				} else if (key == "userPrivils") {
					html.append(strongAccountInfoLineNoEscape(
					    userinfTbl.attrProps[key].desc,
					    Isds::Description::htmlDescrPrivileges(
					        Isds::variant2Privileges(userEntry.value(key)))));
				} else {
					html.append(strongAccountInfoLine(
					    userinfTbl.attrProps[key].desc,
					    QString::number(userEntry.
					        value(key).toInt())));
				}
				break;
			case DB_TEXT:
				if (key == "userType") {
					html.append(strongAccountInfoLine(
					    userinfTbl.attrProps[key].desc,
					    Isds::Description::descrSenderType(
					        Isds::variant2SenderType(userEntry.value(key)))));
				} else {
					html.append(strongAccountInfoLine(
					    userinfTbl.attrProps[key].desc,
					    userEntry.value(key).toString()));
				}
				break;
			case DB_DATE:
				html.append(strongAccountInfoLine(
				    userinfTbl.attrProps[key].desc,
				    dateStrFromDbFormat(
				        userEntry.value(key).toString(),
				        dateDisplayFormat)));
				break;
			default:
				Q_ASSERT(0);
				break;
			}
		}
	}

	html.append("</td><td></td><td>");

	accountEntry = GlobInstcs::accntDbPtr->accountEntry(acntDbKey);
	/* Print non-empty entries. */
	for (int i = 0; i < accntinfTbl.knownAttrs.size(); ++i) {
		const QString &key = accntinfTbl.knownAttrs[i].first;
		if (accountEntry.hasValue(key) &&
		    !accntinfTbl.attrProps[key].desc.isEmpty()) {
			switch (accntinfTbl.knownAttrs[i].second) {
			case DB_INTEGER:
				if (key == "dbState") {
					html.append(strongAccountInfoLine(
					    accntinfTbl.attrProps[key].desc,
					    Isds::Description::descrDbState(
					        Isds::variant2DbState(accountEntry.value(key)))));
				} else if (key == "ic") {
					if (accountEntry.value(key).toInt() > 0) {
						html.append(strongAccountInfoLine(
						    accntinfTbl.attrProps[key].desc,
						   QString::number(accountEntry.
						   value(key).toInt())));
					}
				} else {
					html.append(strongAccountInfoLine(
					    accntinfTbl.attrProps[key].desc,
					    QString::number(accountEntry.
					        value(key).toInt())));
				}
				break;
			case DB_TEXT:
				html.append(strongAccountInfoLine(
				    accntinfTbl.attrProps[key].desc,
				    accountEntry.value(key).toString()));
				break;
			case DB_BOOLEAN:
				html.append(strongAccountInfoLine(
				    accntinfTbl.attrProps[key].desc,
				    accountEntry.value(key).toBool() ?
				        tr("Yes") : tr("No")));
				break;
			case DB_DATETIME:
				html.append(strongAccountInfoLine(
				    accntinfTbl.attrProps[key].desc,
				    dateTimeStrFromDbFormat(
				        accountEntry.value(key).toString(),
				        dateTimeDisplayFormat)));
				break;
			case DB_DATE:
				html.append(strongAccountInfoLine(
				    accntinfTbl.attrProps[key].desc,
				    dateStrFromDbFormat(
				        accountEntry.value(key).toString(),
				        dateDisplayFormat)));
				break;
			default:
				Q_ASSERT(0);
				break;
			}
		}
	}

	html.append("</td></tr></table>");

lastPart:

	QString info(qDateTimeToDbFormat(GlobInstcs::accntDbPtr->getPwdExpirFromDb(acntDbKey)));
	if (info.isEmpty()) {
		info = tr("unknown or without expiration");
	} else {
		info = info.split(".")[0];
	}

	html.append(strongAccountInfoLine(tr("Password expiration date"),
	    info));

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return QString();
	}
	QStringList dbFilePaths = dbSet->fileNames();
	if ((dbFilePaths.size() == 1) &&
	    (MessageDb::memoryLocation == dbFilePaths.first())) {
		QString dbFilePath = tr("Database is stored in memory. "
		    "Data will be lost on application exit.");
		html.append(strongAccountInfoLine(
		    tr("Local database file location"), dbFilePath));
	} else {
		foreach (const QString &path, dbFilePaths) {
			html.append(strongAccountInfoLine(
			    tr("Local database file location"), path));
		}
	}

	html.append(divEnd);

	return html;
}


/* ========================================================================= */
/*
 * Generate overall account information.
 */
QString MainWindow::createAccountInfoAllField(const QString &accountName,
    const QList< QPair<QString, int> > &receivedCounts,
    const QList< QPair<QString, int> > &sentCounts) const
/* ========================================================================= */
{
	QString html = indentDivStart;
	html.append ("<h3>" + accountName.toHtmlEscaped() + "</h3>");

	html.append(strongAccountInfoLine(tr("Received messages"), QString()));
	html.append(indentDivStart);
	if (0 == receivedCounts.size()) {
		html.append(tr("none"));
	} else {
		for (int i = 0; i < receivedCounts.size(); ++i) {
			html.append(accountInfoLine(receivedCounts[i].first,
			    QString::number(receivedCounts[i].second)));
		}
	}
	html.append(divEnd);

	html.append("<br/>");

	html.append(strongAccountInfoLine(tr("Sent messages"), QString()));
	html.append(indentDivStart);
	if (0 == sentCounts.size()) {
		html.append(tr("none"));
	} else {
		for (int i = 0; i < sentCounts.size(); ++i) {
			html.append(accountInfoLine(sentCounts[i].first,
			    QString::number(sentCounts[i].second)));
		}
	}
	html.append(divEnd);

	html.append(divEnd);
	return html;
}

/* ========================================================================= */
/*
 * Generate overall account information only for sent or received messages.
 */
QString MainWindow::createAccountInfoMessagesCount(const QString &accountName,
    const QList< QPair<QString, int> > &counts,
    enum MessageDb::MessageType type) const
/* ========================================================================= */
{
	QString html = indentDivStart;
	html.append ("<h3>" + accountName.toHtmlEscaped() + "</h3>");

	if (type == MessageDb::TYPE_RECEIVED) {
		html.append(strongAccountInfoLine(tr("Received messages"), QString()));
	} else {
		html.append(strongAccountInfoLine(tr("Sent messages"), QString()));
	}
	html.append(indentDivStart);
	if (0 == counts.size()) {
		html.append(tr("none"));
	} else {
		for (int i = 0; i < counts.size(); ++i) {
			html.append(accountInfoLine(counts[i].first,
			    QString::number(counts[i].second)));
		}
	}
	html.append(divEnd);

	html.append(divEnd);
	return html;
}

/* ========================================================================= */
/*
 * Generate banner.
 */
QString MainWindow::createDatovkaBanner(const QString &version) const
/* ========================================================================= */
{
	QString html = "<br><center>";
	html += "<h2>" +
	    tr("Datovka - Free client for Datové schránky") + "</h2>";
#ifdef PORTABLE_APPLICATION
	html += "<h3>" + tr("Portable version") + "</h3>";
#endif /* PORTABLE_APPLICATION */
	html += strongAccountInfoLine(tr("Version"), version);
	html += QString("<br><img src=") + ICON_128x128_PATH +
	    "logo.png />";
	html += "<h3>" + tr("Powered by") + "</h3>";
	html += QString("<br><img src=") + ICON_128x128_PATH + "cznic.png />";
	html += "</center>";
	return html;
}

MessageDbSet *MainWindow::accountDbSet(const QString &userName)
{
	enum AccountInteraction::AccessStatus status =
	    AccountInteraction::AS_ERR;
	QString dbDir, namesStr;

	MessageDbSet *dbSet = AccountInteraction::accessDbSet(userName, status,
	    dbDir, namesStr);

	dbDir = QDir::toNativeSeparators(dbDir);

	switch (status) {
	case AccountInteraction::AS_DB_ALREADY_PRESENT:
		QMessageBox::information(this,
		    tr("Datovka: Database file present"),
		    tr("Database file for account '%1' already exists.").arg(userName) +
		    "\n\n" +
		    tr("The existing database files %1 in '%2' are going to be used.").arg(namesStr).arg(dbDir) +
		    "\n\n" +
		    tr("If you want to use a new blank file then delete, rename or move the existing file so that the application can create a new empty file."),
		    QMessageBox::Ok);
		break;
	case AccountInteraction::AS_DB_NOT_PRESENT:
		QMessageBox::warning(this,
		    tr("Datovka: Problem loading database"),
		    tr("Could not load data from the database for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Database files are missing in '%1'.").arg(dbDir) +
		    "\n\n" +
		    tr("I'll try to create an empty one."),
		    QMessageBox::Ok);
		break;
	case AccountInteraction::AS_DB_NOT_FILES:
		QMessageBox::warning(this,
		    tr("Datovka: Problem loading database"),
		    tr("Could not load data from the database for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Some databases of %1 in '%2' are not a file.").arg(namesStr).arg(dbDir),
		    QMessageBox::Ok);
		break;
	case AccountInteraction::AS_DB_FILES_INACCESSIBLE:
		QMessageBox::warning(this,
		    tr("Datovka: Problem loading database"),
		    tr("Could not load data from the database for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Some databases of '%1' in '%2' cannot be accessed.").arg(namesStr).arg(dbDir) +
		    "\n\n" +
		    tr("You don't have enough access rights to use the file."),
		    QMessageBox::Ok);
		break;
	case AccountInteraction::AS_DB_FILES_CORRUPT:
		QMessageBox::warning(this,
		    tr("Datovka: Problem loading database"),
		    tr("Could not load data from the database for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Some databases of %1 in '%2' cannot be used.").arg(namesStr).arg(dbDir) +
		    "\n\n" +
		    tr("The file either does not contain an sqlite database or the file is corrupted."),
		    QMessageBox::Ok);
		break;
	case AccountInteraction::AS_DB_CONFUSING_ORGANISATION:
		QMessageBox::warning(this,
		    tr("Datovka: Problem loading database"),
		    tr("Could not load data from the database for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Conflicting databases %1 in '%2' cannot be used.").arg(namesStr).arg(dbDir) +
		    "\n\n" +
		    tr("Please remove the conflicting files."),
		    QMessageBox::Ok);
		break;
	default:
		break;
	}

	/*
	 * TODO -- Give the user some recovery options such as
	 * move/rename/remove the corrupted file or remove/ignore the affected
	 * account.
	 */

	if (Q_NULLPTR == dbSet) {
		/*
		 * TODO -- generate notification dialogue and give the user
		 * a choice between aborting program and skipping account?
		 */
		QMessageBox::critical(this,
		    tr("Datovka: Database opening error"),
		    tr("Could not load data from the database for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Database files in '%1' cannot be created or are corrupted.").arg(dbDir),
		    QMessageBox::Ok);
		/*
		 * The program has to be aborted right now. The method
		 * QCoreApplication::exit(EXIT_FAILURE) uses the event loop
		 * whereas some event may be already planned and will crash
		 * because of the returnning NULL pointer.
		 * Therefore exit() should be used.
		 */
	}

	return dbSet;
}

/* ========================================================================= */
/*
 * Get storage paths to selected account item.
 */
void MainWindow::setAccountStoragePaths(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());

	const AcntSettings &itemSettings((*GlobInstcs::acntMapPtr)[userName]);

	if (!itemSettings.lastAttachSavePath().isEmpty()) {
		m_save_attach_dir = itemSettings.lastAttachSavePath();
	}
	if (!itemSettings.lastAttachAddPath().isEmpty()) {
		m_add_attach_dir = itemSettings.lastAttachAddPath();
	}
	if (!itemSettings.lastCorrespPath().isEmpty()) {
		m_export_correspond_dir = itemSettings.lastCorrespPath();
	}
	if (!itemSettings.lastZFOExportPath().isEmpty()) {
		m_on_export_zfo_activate = itemSettings.lastZFOExportPath();
	}
}

/* ========================================================================= */
/*
 * Sets geometry from settings.
 */
void MainWindow::loadWindowGeometry(const QSettings &settings)
/* ========================================================================= */
{
	/* Default geometry. */
	QRect defaultDimensions(Dimensions::windowDimensions(this, 76.0, 48.0));

	/* Stored geometry. */
	int x = settings.value(WIN_POSITION_HEADER "/" WIN_POSITION_X,
	    defaultDimensions.x()).toInt();
	int y = settings.value(WIN_POSITION_HEADER "/" WIN_POSITION_Y,
	    defaultDimensions.y()).toInt();
	int w = settings.value(WIN_POSITION_HEADER "/" WIN_POSITION_W,
	    defaultDimensions.width()).toInt();
	int h = settings.value(WIN_POSITION_HEADER "/" WIN_POSITION_H,
	    defaultDimensions.height()).toInt();

	bool max = settings.value(WIN_POSITION_HEADER "/" WIN_POSITION_MAX,
	    false).toBool();

	this->setGeometry(x, y, w, h);
	if (!max) {
		/*
		 * adjustSize() causes problems in Cinnamon WM in maximised
		 * mode. That's why its called only when not maximised.
		 */
		this->adjustSize();
		/*
		 * adjustSize() shrinks the window in Cinnamon WM. Therefore,
		 * set window geometry again.
		 */
		this->setGeometry(x, y, w, h);
	}
	/* Update the window and force repaint. */
	this->update();
	this->repaint();

	/* Adjust for screen size. */
	this->setGeometry(Dimensions::windowOnScreenDimensions(this));

	if (max) {
		m_geometry = QRect(x, y, w, h);
		this->showMaximized();
		this->update();
		this->repaint();
	}

	/*
	 * It appears that repaint does not immediately call the event.
	 * Therefore the event loop is enforced to process pending events.
	 */
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents |
	    QEventLoop::ExcludeSocketNotifiers);

	/* Set minimal size of splitter content and disable collapsing. */
	ui->accountList->setMinimumSize(QSize(100, 100));
	ui->hSplitterAccount->setChildrenCollapsible(false);
	ui->messageList->setMinimumSize(QSize(100, 100));
	ui->vSplitterMessage->setChildrenCollapsible(false);
	ui->messageInfo->setMinimumSize(QSize(100, 100));
	ui->messageAttachmentList->setMinimumSize(QSize(100, 100));
	ui->hSplitterMessageInfo->setChildrenCollapsible(false);

	/* Splitter geometry. */

	// set mainspliter - hSplitterAccount
	w = ui->centralWidgetWindows->width();
	QList<int> sizes = ui->hSplitterAccount->sizes();
	int tmp = settings.value("panes/hpaned1", 226).toInt();
	sizes[0] = tmp;
	sizes[1] = w
	    - ui->hSplitterAccount->handleWidth()
	    - sizes[0];
	ui->hSplitterAccount->setSizes(sizes);
	ui->hSplitterAccount->adjustSize();

	// set messagelistspliter - vSplitterMessage
	h = ui->centralWidgetWindows->height();
	sizes = ui->vSplitterMessage->sizes();
	sizes[0] = settings.value("panes/message_pane", 265).toInt();
	sizes[1] = h -
	    ui->vSplitterMessage->handleWidth()
	    - sizes[0];
	ui->vSplitterMessage->setSizes(sizes);

	// set message/mesageinfospliter - hSplitterMessageInfo
	sizes = ui->hSplitterMessageInfo->sizes();
	sizes[0] = settings.value("panes/message_display_pane", 505).toInt();
	sizes[1] = w
	    - tmp
	    - ui->hSplitterAccount->handleWidth()
	    - ui->hSplitterMessageInfo->handleWidth()
	    - sizes[0];
	ui->hSplitterMessageInfo->setSizes(sizes);
}

void MainWindow::setDefaultAccount(const QSettings &settings)
{
	debugFuncCall();

	QString userName = settings.value("default_account/username", "")
	   .toString();
	if (!userName.isEmpty()) {
		QModelIndex acntTopIdx = m_accountModel.topAcntIndex(userName);
		if (acntTopIdx.isValid()) {
			ui->accountList->setCurrentIndex(
			    acntTopIdx.child(0, 0));
			accountItemCurrentChanged(acntTopIdx.child(0, 0));
			activateAccountMenuActions(true, userName);
		}
	} else {
		defaultUiMainWindowSettings();
	}
}

/* ========================================================================= */
/*
 * Connects top menu-bar buttons to appropriate actions.
 */
void MainWindow::connectTopMenuBarSlots(void)
/* ========================================================================= */
{
	debugFuncCall();

	/*
	 * Actions that cannot be automatically connected
	 * via QMetaObject::connectSlotsByName because of mismatching names.
	 */

	/* File menu. */
	connect(ui->actionSync_all_accounts, SIGNAL(triggered()),
	    this, SLOT(synchroniseAllAccounts()));
	    /* Separator. */
	connect(ui->actionAdd_account, SIGNAL(triggered()),
	    this, SLOT(showAddNewAccountDialog()));
	connect(ui->actionDelete_account, SIGNAL(triggered()),
	    this, SLOT(deleteSelectedAccount()));
	    /* Separator. */
	connect(ui->actionImport_database_directory, SIGNAL(triggered()),
	    this, SLOT(showImportDatabaseDialog()));
	    /* Separator. */
	connect(ui->actionProxy_settings, SIGNAL(triggered()),
	    this, SLOT(showProxySettingsDialog()));
	    /* Separator */
	connect(ui->actionRecords_management_settings, SIGNAL(triggered()),
	    this, SLOT(showRecordsManagementDialogue()));
	connect(ui->actionUpdate_records_management_information, SIGNAL(triggered()),
	    this, SLOT(getStoredMsgInfoFromRecordsManagement()));
	    /* Separator. */
	connect(ui->actionPreferences, SIGNAL(triggered()),
	    this, SLOT(showPreferencesDialog()));
	/* actionQuit -- connected in ui file. */

	/* Data box menu. */
	connect(ui->actionGet_messages, SIGNAL(triggered()),
	    this, SLOT(synchroniseSelectedAccount()));
	connect(ui->actionSend_message, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessage()));
	    /* Separator. */
	connect(ui->actionSend_egov_request, SIGNAL(triggered()),
	    this, SLOT(createGovRequest()));
	    /* Separator. */
	connect(ui->actionMark_all_as_read, SIGNAL(triggered()),
	    this, SLOT(accountMarkReceivedRead()));
	    /* Separator. */
	connect(ui->actionChange_password, SIGNAL(triggered()),
	    this, SLOT(changeAccountPassword()));
	    /* Separator. */
	connect(ui->actionAccount_properties, SIGNAL(triggered()),
	    this, SLOT(manageAccountProperties()));
	    /* Separator. */
	connect(ui->actionMove_account_up, SIGNAL(triggered()),
	    this, SLOT(moveSelectedAccountUp()));
	connect(ui->actionMove_account_down, SIGNAL(triggered()),
	    this, SLOT(moveSelectedAccountDown()));
	    /* Separator. */
	connect(ui->actionChange_data_directory, SIGNAL(triggered()),
	    this, SLOT(changeDataDirectory()));
#ifdef PORTABLE_APPLICATION
	ui->actionChange_data_directory->setEnabled(false);
#endif /* PORTABLE_APPLICATION */
	    /* Separator. */
	connect(ui->actionImport_messages_from_database, SIGNAL(triggered()),
	    this, SLOT(prepareMsgsImportFromDatabase()));
	connect(ui->actionImport_ZFO_file_into_database, SIGNAL(triggered()),
	    this, SLOT(showImportZFOActionDialog()));
	    /* Separator. */
	connect(ui->actionVacuum_message_database, SIGNAL(triggered()),
	    this, SLOT(vacuumMsgDbSlot()));
	connect(ui->actionSplit_database_by_years, SIGNAL(triggered()),
	    this, SLOT(splitMsgDbByYearsSlot()));

	/* Message menu. */
	connect(ui->actionDownload_message_signed, SIGNAL(triggered()),
	    this, SLOT(downloadSelectedMessageAttachments()));
	connect(ui->actionReply, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessageReply()));
	connect(ui->actionForward_message, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessageWithZfos()));
	connect(ui->actionCreate_message_from_template, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessageFromTmpl()));
	    /* Separator. */
	connect(ui->actionSignature_detail, SIGNAL(triggered()),
	    this, SLOT(showSignatureDetailsDialog()));
	connect(ui->actionAuthenticate_message, SIGNAL(triggered()),
	    this, SLOT(verifySelectedMessage()));
	    /* Separator. */
	connect(ui->actionOpen_message_externally, SIGNAL(triggered()),
	    this, SLOT(openSelectedMessageExternally()));
	connect(ui->actionOpen_delivery_info_externally, SIGNAL(triggered()),
	    this, SLOT(openDeliveryInfoExternally()));
	    /* Separator. */
	connect(ui->actionSend_to_records_management, SIGNAL(triggered()),
	    this, SLOT(sendSelectedMessageToRecordsManagement()));
	    /* Separator. */
	connect(ui->actionExport_as_ZFO, SIGNAL(triggered()),
	    this, SLOT(exportSelectedMessagesAsZFO()));
	connect(ui->actionExport_delivery_info_as_ZFO, SIGNAL(triggered()),
	    this, SLOT(exportSelectedDeliveryInfosAsZFO()));
	connect(ui->actionExport_delivery_info_as_PDF, SIGNAL(triggered()),
	    this, SLOT(exportSelectedDeliveryInfosAsPDF()));
	connect(ui->actionExport_message_envelope_as_PDF, SIGNAL(triggered()),
	    this, SLOT(exportSelectedMessageEnvelopesAsPDF()));
	connect(ui->actionExport_envelope_PDF_and_attachments, SIGNAL(triggered()),
	    this, SLOT(exportSelectedMessageEnvelopeAttachments()));
	    /* Separator. */
	connect(ui->actionEmail_ZFOs, SIGNAL(triggered()),
	    this, SLOT(sendMessagesZfoEmail()));
	connect(ui->actionEmail_all_attachments, SIGNAL(triggered()),
	    this, SLOT(sendAllAttachmentsEmail()));
	    /* Separator. */
	connect(ui->actionSave_all_attachments, SIGNAL(triggered()),
	    this, SLOT(saveAllAttachmentsToDir()));
	connect(ui->actionSave_selected_attachments, SIGNAL(triggered()),
	    this, SLOT(saveSelectedAttachmentsToFile()));
	connect(ui->actionOpen_attachment, SIGNAL(triggered()),
	    this, SLOT(openSelectedAttachment()));
	    /* Separator. */
	connect(ui->actionDelete_message_from_db, SIGNAL(triggered()),
	    this, SLOT(deleteMessage()));

	/* Tools menu. */
	connect(ui->actionFind_databox, SIGNAL(triggered()),
	    this, SLOT(findDatabox()));
	    /* Separator. */
	connect(ui->actionAuthenticate_message_file, SIGNAL(triggered()),
	    this, SLOT(authenticateMessageFile()));
	connect(ui->actionView_message_file, SIGNAL(triggered()),
	    this, SLOT(showViewMessageFromZFODialog()));
	connect(ui->actionExport_correspondence_overview, SIGNAL(triggered()),
	    this, SLOT(showExportCorrespondenceOverviewDialog()));
	connect(ui->actionCheck_message_timestamp_expiration, SIGNAL(triggered()),
	    this, SLOT(showMsgTmstmpExpirDialog()));
	    /* Separator. */
	connect(ui->actionMsgAdvancedSearch, SIGNAL(triggered()),
	    this, SLOT(showMsgAdvancedSearchDialog()));
	    /* Separator. */
	connect(ui->actionTag_settings, SIGNAL(triggered()),
	    this, SLOT(showTagDialog()));
	    /* Separator. */
	connect(ui->actionViewLog, SIGNAL(triggered()),
	    this, SLOT(viewLogDlg()));

	/* Help. */
	connect(ui->actionAbout_Datovka, SIGNAL(triggered()),
	    this, SLOT(showAboutApplicationDialog()));
	connect(ui->actionHomepage, SIGNAL(triggered()),
	    this, SLOT(goHome()));
	connect(ui->actionHelp, SIGNAL(triggered()),
	    this, SLOT(showAppHelpInTheBrowser()));

	/* Actions that are not shown in the top menu. */
	connect(ui->actionEmail_selected_attachments, SIGNAL(triggered()),
	    this, SLOT(sendAttachmentsEmail()));
}


/* ========================================================================= */
/*
 * Connect message-action-bar buttons to appropriate actions.
 */
void MainWindow::connectMessageActionBarSlots(void)
/* ========================================================================= */
{
	debugFuncCall();

	/*
	 * Actions that cannot be automatically connected
	 * via QMetaObject::connectSlotsByName because of mismatching names.
	 */

	/* Downloading attachments also triggers signature verification. */
	ui->signatureDetails->setDefaultAction(ui->actionSignature_detail);

	/* Sets message processing state. */
	connect(ui->messageStateCombo, SIGNAL(activated(int)),
	    this, SLOT(msgSetSelectedMessageProcessState(int)));

	/* Message/attachment related buttons. */
	ui->downloadComplete->setDefaultAction(
	    ui->actionDownload_message_signed);
	ui->saveAttachments->setDefaultAction(ui->actionSave_all_attachments);
	ui->saveAttachment->setDefaultAction(
	    ui->actionSave_selected_attachments);
	ui->openAttachment->setDefaultAction(ui->actionOpen_attachment);
}


/* ========================================================================= */
/*
 *  Set default settings of main window.
 */
void MainWindow::defaultUiMainWindowSettings(void) const
/* ========================================================================= */
{
	// TopMenu
	ui->menuDatabox->setEnabled(false);
	ui->menuMessage->setEnabled(false);
	// Menu: File
	ui->actionDelete_account->setEnabled(false);
	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionUpdate_records_management_information->setEnabled(
	    GlobInstcs::recMgmtSetPtr->isValid());
	// Menu: Tools
	ui->actionFind_databox->setEnabled(false);
	ui->actionImport_ZFO_file_into_database->setEnabled(false);
	ui->actionMsgAdvancedSearch->setEnabled(false);
	ui->actionAuthenticate_message_file->setEnabled(false);
	ui->actionExport_correspondence_overview->setEnabled(false);
	ui->actionCheck_message_timestamp_expiration->setEnabled(false);

	// Top tool bar
	//ui->actionSync_all_accounts->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);
	ui->actionSend_message->setEnabled(false);
	ui->actionSend_egov_request->setEnabled(false);
	ui->actionReply->setEnabled(false);
	ui->actionAuthenticate_message->setEnabled(false);
	//ui->actionMsgAdvancedSearch->setEnabled(false);
	ui->actionAccount_properties->setEnabled(false);
}


/* ========================================================================= */
/*
 * Enables menu actions according to message selection.
 */
void MainWindow::setMessageActionVisibility(int numSelected) const
/* ========================================================================= */
{
	/* Top menu + menu items. */
	ui->menuMessage->setEnabled(numSelected > 0);

	ui->actionDownload_message_signed->setEnabled(numSelected > 0);
	ui->actionReply->setEnabled(numSelected == 1);
	ui->actionForward_message->setEnabled(numSelected > 0);
	ui->actionCreate_message_from_template->setEnabled(numSelected == 1);
	    /* Separator. */
	ui->actionSignature_detail->setEnabled(numSelected == 1);
	ui->actionAuthenticate_message->setEnabled(numSelected == 1);
	    /* Separator. */
	ui->actionOpen_message_externally->setEnabled(numSelected == 1);
	ui->actionOpen_delivery_info_externally->setEnabled(numSelected == 1);
	    /* Separator. */
	ui->actionSend_to_records_management->setEnabled(
	    (numSelected == 1) && GlobInstcs::recMgmtSetPtr->isValid());
	    /* Separator. */
	ui->actionExport_as_ZFO->setEnabled(numSelected > 0);
	ui->actionExport_delivery_info_as_ZFO->setEnabled(numSelected > 0);
	ui->actionExport_delivery_info_as_PDF->setEnabled(numSelected > 0);
	ui->actionExport_message_envelope_as_PDF->setEnabled(numSelected > 0);
	ui->actionExport_envelope_PDF_and_attachments->setEnabled(numSelected > 0);
	    /* Separator. */
	ui->actionEmail_ZFOs->setEnabled(numSelected > 0);
	ui->actionEmail_all_attachments->setEnabled(numSelected > 0);
	    /* Separator. */
	/* These must be also handled with relation to attachment selection. */
	ui->actionSave_all_attachments->setEnabled(numSelected == 1);
	ui->actionSave_selected_attachments->setEnabled(false);
	ui->actionOpen_attachment->setEnabled(false);
	    /* Separator. */
	/* Delete action is controlled elsewhere. */
	//ui->actionDelete_message_from_db->setEnabled(numSelected == 1);
}

void MainWindow::setAttachmentActionVisibility(int numSelected) const
{
	/* Save all attachments is handles elsewhere. */
	//ui->actionSave_all_attachments->setEnabled(numSelected == 1);
	ui->actionSave_selected_attachments->setEnabled(numSelected > 0);
	ui->actionOpen_attachment->setEnabled(numSelected == 1);
}

void MainWindow::activateAccountMenuActions(bool enable,
    const QString &username)
{
	ui->menuDatabox->setEnabled(enable);
	ui->actionAccount_properties->setEnabled(enable);
	ui->actionChange_password->setEnabled(enable);
	ui->actionSync_all_accounts->setEnabled(enable);
	ui->actionGet_messages->setEnabled(enable);
	ui->actionSend_message->setEnabled(enable);
	enableSendEGovRequestAction(enable, username);
	ui->actionDelete_account->setEnabled(enable);
	ui->actionFind_databox->setEnabled(enable);
	ui->actionMsgAdvancedSearch->setEnabled(enable);
	ui->actionImport_ZFO_file_into_database->setEnabled(enable);
}

/* ========================================================================= */
/*
 * Store geometry to settings.
 */
void MainWindow::saveWindowGeometry(QSettings &settings) const
/* ========================================================================= */
{
	int value;

	/* Window geometry. */
	QRect geom(this->geometry());

	if (this->isMaximized() && m_geometry.isValid()) {
		geom = m_geometry;
	}

	settings.beginGroup(WIN_POSITION_HEADER);

	value = geom.x();
	value = (value < 0) ? 0 : value;
	settings.setValue(WIN_POSITION_X, value);

	value = geom.y();
	value = (value < 0) ? 0 : value;
	settings.setValue(WIN_POSITION_Y, value);

	settings.setValue(WIN_POSITION_W, geom.width());
	settings.setValue(WIN_POSITION_H, geom.height());

	if (this->isMaximized()) {
		settings.setValue(WIN_POSITION_MAX, true);
	}

	settings.endGroup();

	/* Splitter geometry. */

	settings.beginGroup("panes");

	settings.setValue("hpaned1", ui->hSplitterAccount->sizes()[0]);
	settings.setValue("message_pane", ui->vSplitterMessage->sizes()[0]);
	settings.setValue("message_display_pane",
	    ui->hSplitterMessageInfo->sizes()[0]);

	settings.endGroup();
}

/* ========================================================================= */
/*
 * Load and apply settings from configuration file.
 */
void MainWindow::loadSettings(void)
/* ========================================================================= */
{
	QSettings settings(GlobInstcs::prefsPtr->loadConfPath(),
	    QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	/* Load last directory paths */
	loadLastDirectoryPaths(settings);

	/* Received Sent messages Column widths */
	loadSentReceivedMessagesColumnWidth(settings);

	/* Window geometry. */
	loadWindowGeometry(settings);

	/* Global preferences. */
	GlobInstcs::prefsPtr->loadFromSettings(settings);

	/* Proxy settings. */
	GlobInstcs::proxSetPtr->loadFromSettings(settings);

	/* PIN should already be set because main window is running. */

	/* Records management settings. */
	GlobInstcs::recMgmtSetPtr->loadFromSettings(settings);
	GlobInstcs::recMgmtSetPtr->decryptToken(GlobInstcs::pinSetPtr->_pinVal);

	/* Accounts. */
	m_accountModel.loadFromSettings(GlobInstcs::prefsPtr->confDir(),
	    settings);
	ui->accountList->setModel(&m_accountModel);
	GlobInstcs::acntMapPtr->decryptAllPwds(GlobInstcs::pinSetPtr->_pinVal);

	/* Select last-used account. */
	setDefaultAccount(settings);

	/* Scan databases. */
	regenerateAllAccountModelYears();

	/* Load collapse info of account items from settings */
	loadAccountCollapseInfo(settings);
}

void MainWindow::loadSentReceivedMessagesColumnWidth(const QSettings &settings)
{
	m_colWidthRcvd[1] = settings.value("column_widths/received_1", DFLT_COL_WIDTH).toInt();
	m_colWidthRcvd[2] = settings.value("column_widths/received_2", DFLT_COL_WIDTH).toInt();
	m_colWidthSnt[1] = settings.value("column_widths/sent_1", DFLT_COL_WIDTH).toInt();
	m_colWidthSnt[2] = settings.value("column_widths/sent_2", DFLT_COL_WIDTH).toInt();
	m_sortCol = settings.value("message_ordering/sort_column", DFLT_COL_SORT).toInt();
	/* Sort column saturation from old datovka */
	if (m_sortCol > 5) {
		m_sortCol = DFLT_COL_SORT;
	}
	m_sort_order = settings.value("message_ordering/sort_order",
	    QString()).toString();
}

void MainWindow::saveSentReceivedColumnWidth(QSettings &settings) const
{
	settings.beginGroup("column_widths");
	settings.setValue("received_1", m_colWidthRcvd[1]);
	settings.setValue("received_2", m_colWidthRcvd[2]);
	settings.setValue("sent_1", m_colWidthSnt[1]);
	settings.setValue("sent_2", m_colWidthSnt[2]);
	settings.endGroup();

	settings.beginGroup("message_ordering");
	settings.setValue("sort_column", m_sortCol);
	settings.setValue("sort_order", m_sort_order);
	settings.endGroup();
}

/* ========================================================================= */
/*
 * Store current account user name to settings.
 */
void MainWindow::saveAccountIndex(QSettings &settings) const
/* ========================================================================= */
{
	QModelIndex acntIndex(currentAccountModelIndex());
	if (acntIndex.isValid()) {
		const QString userName(m_accountModel.userName(acntIndex));
		Q_ASSERT(!userName.isEmpty());

		settings.beginGroup("default_account");
		settings.setValue("username", userName);
		settings.endGroup();
	}
}


/* ========================================================================= */
/*
 * Update numbers of unread messages in account model.
 */
bool MainWindow::updateExistingAccountModelUnread(const QModelIndex &index)
/* ========================================================================= */
{
	/*
	 * Several nodes may be updated at once, because some messages may be
	 * referred from multiple nodes.
	 */

	QList<QString> yearList;
	int unreadMsgs;

	Q_ASSERT(index.isValid());

	/* Get database id. */
	const QString userName(m_accountModel.userName(index));
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return false;
	}

	/* Received. */
	unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_RECEIVED);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_RECEIVED,
		    yearList.value(j));
		AccountModel::YearCounter yCounter(unreadMsgs != -2,
		    (unreadMsgs > 0) ? unreadMsgs : 0);
		m_accountModel.updateYear(userName,
		    AccountModel::nodeReceivedYear, yearList.value(j),
		    yCounter);
	}
	/* Sent. */
	//unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_SENT);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentSent, 0);
	yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_SENT,
		    yearList.value(j));
		AccountModel::YearCounter yCounter(unreadMsgs != -2, 0);
		m_accountModel.updateYear(userName, AccountModel::nodeSentYear,
		    yearList.value(j), yCounter);
	}
	return true;
}

bool MainWindow::regenerateAccountModelYears(const QModelIndex &index,
    bool prohibitYearRemoval)
{
	debugFuncCall();

	if (Q_UNLIKELY(!index.isValid())) {
		Q_ASSERT(0);
		return false;
	}

	/* Get database id. */
	const QString userName(m_accountModel.userName(index));
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_UNLIKELY(Q_NULLPTR == dbSet)) {
		Q_ASSERT(0);
		return false;
	}

	QStringList yearList;
	int unreadMsgs;
	typedef QPair<QString, AccountModel::YearCounter> CounterPair;
	QList<CounterPair> yearlyUnreadList;

	/* Received. */
	unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_RECEIVED);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED, DESCENDING);
	foreach (const QString &year, yearList) {
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_RECEIVED,
		    year);
		AccountModel::YearCounter yCounter(unreadMsgs != -2,
		    (unreadMsgs > 0) ? unreadMsgs : 0);
		yearlyUnreadList.append(CounterPair(year, yCounter));
	}
	m_accountModel.updateYearNodes(userName, AccountModel::nodeReceivedYear,
	    yearlyUnreadList, AccountModel::DESCENDING, prohibitYearRemoval);

	/* Sent. */
	//unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_SENT);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentSent, 0);
	yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
	yearlyUnreadList.clear();
	foreach (const QString &year, yearList) {
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_SENT,
		    year);
		AccountModel::YearCounter yCounter(unreadMsgs != -2, 0);
		yearlyUnreadList.append(CounterPair(year, yCounter));
	}
	m_accountModel.updateYearNodes(userName, AccountModel::nodeSentYear,
	    yearlyUnreadList, AccountModel::DESCENDING, prohibitYearRemoval);

	return true;
}

bool MainWindow::regenerateAllAccountModelYears(void)
{
	debugFuncCall();

	m_accountModel.removeAllYearNodes();

	for (int i = 0; i < m_accountModel.rowCount(); ++i) {
		regenerateAccountModelYears(m_accountModel.index(i, 0));
	}

	return true;
}

/* ========================================================================= */
/*!
 * @brief Store current settings to configuration file.
 */
void MainWindow::saveSettings(void) const
/* ========================================================================= */
{
	debugFuncCall();

	/*
	 * TODO -- Target file name differs from source for testing purposes.
	 */
	QSettings settings(GlobInstcs::prefsPtr->saveConfPath(),
	    QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	settings.clear();

	/* Store application ID and config format */
	saveAppIdConfigFormat(settings);

	/* PIN settings. */
	GlobInstcs::pinSetPtr->saveToSettings(settings);

	/* Accounts. */
	m_accountModel.saveToSettings(GlobInstcs::pinSetPtr->_pinVal,
	    GlobInstcs::prefsPtr->confDir(), settings);

	/* Store last-used account. */
	saveAccountIndex(settings);

	/* TODO */
	saveSentReceivedColumnWidth(settings);

	/* Window geometry. */
	saveWindowGeometry(settings);

	/* Store account collapses */
	saveAccountCollapseInfo(settings);

	/* Proxy settings. */
	GlobInstcs::proxSetPtr->saveToSettings(settings);

	/* Records management settings. */
	GlobInstcs::recMgmtSetPtr->saveToSettings(
	    GlobInstcs::pinSetPtr->_pinVal, settings);

	/* Global preferences. */
	GlobInstcs::prefsPtr->saveToSettings(settings);

	settings.sync();

	/* Remove " symbols from passwords in dsgui.conf */
	confFileRemovePwdQuotes(GlobInstcs::prefsPtr->saveConfPath());
}

void MainWindow::createAndSendMessage(void)
{
	debugSlotCall();
	showSendMessageDialog(DlgSendMessage::ACT_NEW);
}

/*!
 * @brief Return list of available usernames and associated database sets.
 *
 * @param[in,out] mainWindow Pointer to main window instance.
 * @param[in]     accountModel Account model.
 * @param[in]     accountList Account list view.
 * @param[in]     onlyConnected If true then only connected accounts are
 *                              inserted into the list.
 * @return Account descriptor list. Empty list on any error.
 */
static
QList<Task::AccountDescr> messageDbListForAllAccounts(MainWindow *mainWindow,
    const AccountModel &accountModel, const QTreeView *accountList,
    bool onlyConnected)
{
	if (Q_UNLIKELY((mainWindow == Q_NULLPTR) || (accountList == Q_NULLPTR))) {
		Q_ASSERT(0);
		return QList<Task::AccountDescr>();
	}

	QList<Task::AccountDescr> messageDbList;

	for (int i = 0; i < accountList->model()->rowCount(); ++i) {
		const QModelIndex index(accountModel.index(i, 0));
		const QString uName(accountModel.userName(index));
		MessageDbSet *dbSet = mainWindow->accountDbSet(uName);
		if (Q_UNLIKELY(uName.isEmpty() || (Q_NULLPTR == dbSet))) {
			Q_ASSERT(0);
			QList<Task::AccountDescr>();
		}
		if (onlyConnected) {
			/* Don't push into list if the user is unable to log in. */
			if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(uName) &&
			    !mainWindow->connectToIsds(uName)) {
				continue;
			}
		}
		messageDbList.append(Task::AccountDescr(uName, dbSet));
	}

	return messageDbList;
}

void MainWindow::createGovRequest(void)
{
	debugSlotCall();

	/* Get username of selected account */
	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	/* The dialogue window checks whether the account is connected. */
	DlgGovServices::sendRequest(
	    messageDbListForAllAccounts(this, m_accountModel, ui->accountList, false),
	    userName, this, this);
}

void MainWindow::createAndSendMessageReply(void)
{
	debugSlotCall();
	showSendMessageDialog(DlgSendMessage::ACT_REPLY);
}

void MainWindow::createAndSendMessageWithZfos(void)
{
	debugSlotCall();
	showSendMessageDialog(DlgSendMessage::ACT_FORWARD);
}

void MainWindow::createAndSendMessageFromTmpl(void)
{
	debugSlotCall();
	showSendMessageDialog(DlgSendMessage::ACT_NEW_FROM_TMP);
}

void MainWindow::showSendMessageDialog(int action,
    const QString &composeSerialised)
{
	debugFuncCall();

	QList<Task::AccountDescr> messageDbList;
	QList<MessageDb::MsgId> msgIds;

	/* get username of selected account */
	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	/* if not reply, get pointers to database for other accounts */
	if (DlgSendMessage::ACT_REPLY != action) {
		messageDbList = messageDbListForAllAccounts(this,
		    m_accountModel, ui->accountList, false);
	} else {
		MessageDbSet *dbSet = accountDbSet(userName);
		if (Q_UNLIKELY(Q_NULLPTR == dbSet)) {
			Q_ASSERT(0);
			return;
		}
		messageDbList.append(Task::AccountDescr(userName, dbSet));
	}

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

	switch (action) {
	case DlgSendMessage::ACT_NEW:
		break;
	case DlgSendMessage::ACT_REPLY:
	case DlgSendMessage::ACT_NEW_FROM_TMP:
		Q_ASSERT(firstMsgColumnIdxs.size() == 1);
		/* No break here. */
	case DlgSendMessage::ACT_FORWARD:
		Q_ASSERT(firstMsgColumnIdxs.size() > 0);
		foreach (const QModelIndex &msgIdx, firstMsgColumnIdxs) {
			MessageDb::MsgId msgId(msgMsgId(msgIdx));

			/* Check whether full messages are present. */
			MessageDbSet *dbSet = accountDbSet(userName);
			Q_ASSERT(Q_NULLPTR != dbSet);

			MessageDb *messageDb =
			    dbSet->accessMessageDb(msgId.deliveryTime, false);
			if (0 == messageDb) {
				Q_ASSERT(0);
				return;
			}

			if (!messageDb->isCompleteMessageInDb(msgId.dmId)) {
				if (!messageMissingOfferDownload(msgId,
				    tr("Full message not present!"))) {
					return;
				}
			}

			msgIds.append(msgId);
		}
		break;
	default:
		Q_ASSERT(0);
		return;
		break;
	}

	QDialog *sendMsgDialog = new DlgSendMessage(messageDbList,
	    (DlgSendMessage::Action) action, msgIds, userName,
	    composeSerialised, this);

	showStatusTextWithTimeout(tr("Create and send a message."));

	connect(sendMsgDialog,
	    SIGNAL(usedAttachmentPath(const QString, const QString)),
	    this, SLOT(storeAttachmentPath(const QString, const QString)));

	sendMsgDialog->setAttribute(Qt::WA_DeleteOnClose, true);
	sendMsgDialog->show();
}

void MainWindow::storeAttachmentPath(const QString &userName,
    const QString &lastDir)
{
	debugSlotCall();

	Q_UNUSED(userName);

	if (!GlobInstcs::prefsPtr->useGlobalPaths) {
		m_add_attach_dir = lastDir;
		storeExportPath(userName);
	}
}

void MainWindow::showAddNewAccountDialog(void)
{
	debugSlotCall();

	showStatusTextWithTimeout(tr("Create a new account."));

	AcntSettings newAcntSettings;
	if (!DlgCreateAccount::modify(newAcntSettings,
	        DlgCreateAccount::ACT_ADDNEW,
	        ui->actionSync_all_accounts->text(), this)) {
		return;
	}

	getAccountUserDataboxInfo(newAcntSettings);

	if (ui->accountList->model()->rowCount() > 0) {
		activateAccountMenuActions(true,
		    m_accountModel.userName(currentAccountModelIndex()));
		saveSettings();
	}
}

/* ========================================================================= */
/*
 * Slot: Delete selected account
 */
void MainWindow::deleteSelectedAccount(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));

	deleteAccount(userName);
}


/* ========================================================================= */
/*
 * Slot: Delete account.
 */
void MainWindow::deleteAccount(const QString &userName)
/* ========================================================================= */
{
	debugSlotCall();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	QString dlgTitleText = tr("Remove account ") + accountName;
	QString questionText = tr("Do you want to remove account") + " '" +
	    accountName + "' (" + userName + ")?";
	QString checkBoxText = tr("Delete also message database from storage");
	QString detailText = tr(
	    "Warning: If you delete the message database then all locally "
	    "accessible messages that are not stored on the ISDS server "
	    "will be lost.");

	QDialog *yesNoCheckDlg = new DlgYesNoCheckbox(dlgTitleText,
	    questionText, checkBoxText, detailText, this);
	int retVal = yesNoCheckDlg->exec();
	yesNoCheckDlg->deleteLater();

	if ((DlgYesNoCheckbox::YesChecked == retVal) ||
	    (DlgYesNoCheckbox::YesUnchecked == retVal)) {
		/* Delete account from model. */
		m_accountModel.deleteAccount(userName);
		GlobInstcs::accntDbPtr->deleteAccountInfo(
		    AccountDb::keyFromLogin(userName));
	}

	if (DlgYesNoCheckbox::YesChecked == retVal) {
		if (GlobInstcs::msgDbsPtr->deleteDbSet(dbSet)) {
			showStatusTextWithTimeout(tr("Account '%1' was deleted "
			    "together with message database file.")
			    .arg(accountName));
		} else {
			showStatusTextWithTimeout(tr("Account '%1' was deleted "
			    "but its message database was not deleted.")
			    .arg(accountName));
		}
	} else if (DlgYesNoCheckbox::YesUnchecked == retVal) {
		showStatusTextWithTimeout(tr("Account '%1' was deleted.")
		    .arg(accountName));
	}

	GlobInstcs::tagDbPtr->removeAllMsgTagsFromAccount(userName);

	if ((DlgYesNoCheckbox::YesChecked == retVal) ||
	    (DlgYesNoCheckbox::YesUnchecked == retVal)) {
		saveSettings();
	}

	if (ui->accountList->model()->rowCount() < 1) {
		accountItemCurrentChanged(QModelIndex());
		defaultUiMainWindowSettings();
	}
}


/* ========================================================================= */
/*
 * Shows change password dialog.
 */
void MainWindow::changeAccountPassword(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		return;
	}

	/* Method connectToIsds() acquires account information. */
	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));
	Q_ASSERT(!dbId.isEmpty());

	const AcntSettings &accountInfo((*GlobInstcs::acntMapPtr)[userName]);

	showStatusTextWithTimeout(tr("Change password of account \"%1\".")
	    .arg(accountInfo.accountName()));

	DlgChangePwd::changePassword(dbId, userName, this);
}

void MainWindow::manageAccountProperties(void)
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	showStatusTextWithTimeout(tr("Change properties of account \"%1\".")
	    .arg((*GlobInstcs::acntMapPtr)[userName].accountName()));

	if (!DlgCreateAccount::modify((*GlobInstcs::acntMapPtr)[userName],
	        DlgCreateAccount::ACT_EDIT,
	        ui->actionSync_all_accounts->text(), this)) {
		return;
	}

	/* Save changes. */
	emit GlobInstcs::acntMapPtr->accountDataChanged(userName);

	showStatusTextWithTimeout(
	    tr("Account '%1' was updated.").arg(userName));
	saveSettings();
}

/* ========================================================================= */
/*
 * Move selected account up.
 */
void MainWindow::moveSelectedAccountUp(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	if (m_accountModel.changePosition(userName, -1)) {
		showStatusTextWithTimeout(tr("Account was moved up."));
	}
}


/* ========================================================================= */
/*
 * Move selected account down.
 */
void MainWindow::moveSelectedAccountDown(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	if (m_accountModel.changePosition(userName, 1)) {
		showStatusTextWithTimeout(tr("Account was moved down."));
	}
}


/* ========================================================================= */
/*
 * Change data directory dialog.
 */
void MainWindow::changeDataDirectory(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const AcntSettings &itemSettings((*GlobInstcs::acntMapPtr)[userName]);
	showStatusTextWithTimeout(tr("Change data dierctory of account \"%1\".")
	    .arg(itemSettings.accountName()));

	QModelIndex accountIdx(currentAccountModelIndex());

	if (DlgChangeDirectory::changeDataDirectory(userName, dbSet, this)) {
		saveSettings();
		accountItemCurrentChanged(accountIdx);
	}
}

/* ========================================================================= */
/*
 * Search data box dialog.
 */
void MainWindow::findDatabox(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		return;
	}

	/* Method connectToIsds() acquires account information. */
	const Isds::DbOwnerInfo dbOwnerInfo(GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(userName)));
	if (dbOwnerInfo.isNull()) {
		return;
	}

	QString dbType(Isds::dbType2Str(dbOwnerInfo.dbType()));
	bool dbEffectiveOVM = (dbOwnerInfo.dbEffectiveOVM() == Isds::Type::BOOL_TRUE);
	bool dbOpenAddressing = (dbOwnerInfo.dbOpenAddressing() == Isds::Type::BOOL_TRUE);

	showStatusTextWithTimeout(tr("Find databoxes from account \"%1\".")
	    .arg((*GlobInstcs::acntMapPtr)[userName].accountName()));

	DlgDsSearch::search(userName, dbType, dbEffectiveOVM, dbOpenAddressing,
	    this);
}

/* ========================================================================= */
/*
 * Message filter
 */
void MainWindow::filterMessages(const QString &text)
/* ========================================================================= */
{
	debugSlotCall();

	/* The model is always associated to the proxy model. */

	m_messageListProxyModel.setSortRole(ROLE_MSGS_DB_PROXYSORT);

	m_messageListProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));
	/* Filter according to second and third column. */
	QList<int> columnList;
	columnList.append(DbMsgsTblModel::DMID_COL);
	columnList.append(DbMsgsTblModel::ANNOT_COL);
	columnList.append(DbMsgsTblModel::SENDER_COL);
	columnList.append(DbMsgsTblModel::RECIP_COL);
	if (Q_NULLPTR != GlobInstcs::tagDbPtr) {
		columnList.append(
		    m_messageListProxyModel.sourceModel()->columnCount() +
		    DbMsgsTblModel::TAGS_NEG_COL); /* Tags. */
	}
	m_messageListProxyModel.setFilterKeyColumns(columnList);

	/* Set filter field background colour. */
	if (text.isEmpty()) {
		mui_filterLine->setStyleSheet(
		    SortFilterProxyModel::blankFilterEditStyle);
	} else if (m_messageListProxyModel.rowCount() != 0) {
		mui_filterLine->setStyleSheet(
		    SortFilterProxyModel::foundFilterEditStyle);
	} else {
		mui_filterLine->setStyleSheet(
		    SortFilterProxyModel::notFoundFilterEditStyle);
	}
}

void MainWindow::setReceivedColumnWidths(void)
{
	debugFuncCall();

	ui->messageList->resizeColumnToContents(DbMsgsTblModel::DMID_COL);

	/* Sender and recipient column must be set both here. */
	ui->messageList->setColumnWidth(DbMsgsTblModel::ANNOT_COL, m_colWidthRcvd[1]);
	ui->messageList->setColumnWidth(DbMsgsTblModel::SENDER_COL, m_colWidthRcvd[2]);
	ui->messageList->setColumnWidth(DbMsgsTblModel::RECIP_COL, m_colWidthSnt[2]);

	ui->messageList->resizeColumnToContents(DbMsgsTblModel::DELIVERY_COL);
	ui->messageList->resizeColumnToContents(DbMsgsTblModel::ACCEPT_COL);

	/* These columns display an icon. */
	ui->messageList->setColumnWidth(DbMsgsTblModel::PERSDELIV_COL, ICON_COL_WIDTH);
	ui->messageList->setColumnWidth(DbMsgsTblModel::READLOC_COL, ICON_COL_WIDTH);
	ui->messageList->setColumnWidth(DbMsgsTblModel::ATTDOWN_COL, ICON_COL_WIDTH);
	ui->messageList->setColumnWidth(DbMsgsTblModel::PROCSNG_COL, ICON_COL_WIDTH);
	if (GlobInstcs::recMgmtSetPtr->isValid()) {
		ui->messageList->setColumnWidth(
		    DbMsgsTblModel::MAX_COLNUM + DbMsgsTblModel::RECMGMT_NEG_COL,
		    ICON_COL_WIDTH);
	}

	if (m_sort_order == "SORT_ASCENDING") {
		ui->messageList->sortByColumn(m_sortCol, Qt::AscendingOrder);
	} else if (m_sort_order == "SORT_DESCENDING") {
		ui->messageList->sortByColumn(m_sortCol, Qt::DescendingOrder);
	}
}

void MainWindow::setSentColumnWidths(void)
{
	debugFuncCall();

	ui->messageList->resizeColumnToContents(DbMsgsTblModel::DMID_COL);

	/* Sender and recipient column must be set both here. */
	ui->messageList->setColumnWidth(DbMsgsTblModel::ANNOT_COL, m_colWidthSnt[1]);
	ui->messageList->setColumnWidth(DbMsgsTblModel::SENDER_COL, m_colWidthRcvd[2]);
	ui->messageList->setColumnWidth(DbMsgsTblModel::RECIP_COL, m_colWidthSnt[2]);

	ui->messageList->resizeColumnToContents(DbMsgsTblModel::DELIVERY_COL);
	ui->messageList->resizeColumnToContents(DbMsgsTblModel::ACCEPT_COL);
	ui->messageList->resizeColumnToContents(DbMsgsTblModel::MSGSTAT_COL);

	/* These columns display an icon. */
	ui->messageList->setColumnWidth(DbMsgsTblModel::ATTDOWN_COL, ICON_COL_WIDTH);
	if (GlobInstcs::recMgmtSetPtr->isValid()) {
		ui->messageList->setColumnWidth(
		    DbMsgsTblModel::MAX_COLNUM + DbMsgsTblModel::RECMGMT_NEG_COL,
		    ICON_COL_WIDTH);
	}

	if (m_sort_order == "SORT_ASCENDING") {
		ui->messageList->sortByColumn(m_sortCol, Qt::AscendingOrder);
	} else if (m_sort_order == "SORT_DESCENDING") {
		ui->messageList->sortByColumn(m_sortCol, Qt::DescendingOrder);
	}
}

void MainWindow::onTableColumnResized(int index, int oldSize, int newSize)
{
	//debugSlotCall();

	Q_UNUSED(oldSize);
	QModelIndex current(currentAccountModelIndex());

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeRecentReceived:
	case AccountModel::nodeReceived:
	case AccountModel::nodeReceivedYear:
		if (index == DbMsgsTblModel::ANNOT_COL) {
			m_colWidthRcvd[1] = newSize;
		} else if (index == DbMsgsTblModel::SENDER_COL) {
			m_colWidthRcvd[2] = newSize;
		}
		break;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		if (index == DbMsgsTblModel::ANNOT_COL) {
			m_colWidthSnt[1] = newSize;
		} else if (index == DbMsgsTblModel::RECIP_COL) {
			m_colWidthSnt[2] = newSize;
		}
		break;
	default:
		break;
	}
}

void MainWindow::onTableColumnHeaderSectionClicked(int column)
{
	debugSlotCall();

	m_sortCol = column;
	if (ui->messageList->horizontalHeader()->sortIndicatorOrder() ==
	    Qt::AscendingOrder) {
		m_sort_order = "SORT_ASCENDING";
	} else if (ui->messageList->horizontalHeader()->sortIndicatorOrder() ==
	           Qt::DescendingOrder) {
		m_sort_order = "SORT_DESCENDING";
	} else {
		m_sort_order = QString();
	}
}

/* ========================================================================= */
/*
 * Load directory paths
 */
void MainWindow::loadLastDirectoryPaths(const QSettings &settings)
/* ========================================================================= */
{
	m_export_correspond_dir =
	    settings.value("last_directories/export_correspondence_overview",
	    "").toString();
	m_on_export_zfo_activate =
	    settings.value("last_directories/on_export_zfo_activate",
	    "").toString();
	m_on_import_database_dir_activate =
	    settings.value("last_directories/on_import_database_dir_activate",
	    "").toString();
}


/* ========================================================================= */
/*
* Load collapse info of account items from settings
*/
void MainWindow::loadAccountCollapseInfo(QSettings &settings)
/* ========================================================================= */
{
	debugFuncCall();

	settings.beginGroup("account_tree");
	QStringList key = settings.childKeys();
	QModelIndex index;
	for (int i = 0; i < key.size(); i++) {

		QStringList keyindex = key[i].split("_");
		/* Expanded toplevel item */
		if (keyindex.size() == 3) {
			index = ui->accountList->model()->
			    index(keyindex[2].toInt(),0);
			ui->accountList->setExpanded(index,
			    !settings.value(key[i], true).toBool());
		}
		/* Expanded all item */
		if (keyindex.size() == 4) {
			index = ui->accountList->model()->
			    index(keyindex[2].toInt(),0);
			index = index.child(keyindex[3].toInt(),0);
			ui->accountList->setExpanded(index,
			    !settings.value(key[i], true).toBool());
		}
		/* Expanded items chilg of all*/
		if (keyindex.size() == 5) {
			index = ui->accountList->model()->
			    index(keyindex[2].toInt(),0);
			index = index.child(keyindex[3].toInt(),0);
			index = index.child(keyindex[4].toInt(),0);
			ui->accountList->setExpanded(index,
			    !settings.value(key[i], true).toBool());
		}
	}
}

/* ========================================================================= */
/*
* Save collapse info of account items from settings
*/
void MainWindow::saveAccountCollapseInfo(QSettings &settings) const
/* ========================================================================= */
{
	debugFuncCall();

	QString keypref= "acc_collapsed_";
	settings.beginGroup("account_tree");
	int row = ui->accountList->model()->rowCount();
	QModelIndex index;
	for (int i = 0; i < row; i++) {
		index = ui->accountList->model()->index(i,0);
		bool isExpTopLevel = ui->accountList->isExpanded(index);
		settings.setValue(keypref+QString::number(i), !isExpTopLevel);
		/* TopLevel item is expanded */
		if (isExpTopLevel) {
			index = ui->accountList->model()->index(i,0).child(2,0);
			bool isExpAll = ui->accountList->isExpanded(index);
			settings.setValue(keypref+QString::number(i)+
			    QString("_2"), !isExpAll);
			/* All item is expanded */
			if (isExpAll) {
				index = ui->accountList->model()->
				    index(i,0).child(2,0).child(0,0);
				settings.setValue(keypref+QString::number(i)+
				    QString("_2_0"),
				    !ui->accountList->isExpanded(index));
				index = ui->accountList->model()->
				    index(i,0).child(2,0).child(1,0);
				settings.setValue(keypref+QString::number(i)+
				    QString("_2_1"),
				    !ui->accountList->isExpanded(index));
			}
		}
	}
	settings.endGroup();
}


/* ========================================================================= */
/*
* Store application ID, config format and directory paths
*/
void MainWindow::saveAppIdConfigFormat(QSettings &settings) const
/* ========================================================================= */
{
	settings.beginGroup("version");
	settings.setValue("config_format", 1);
	settings.setValue("app_id", "f2252df807471479fc4ea71682fa3e53");
	settings.endGroup();

	settings.beginGroup("last_directories");
	settings.setValue("export_correspondence_overview",
	    m_export_correspond_dir);
	settings.setValue("on_export_zfo_activate",
	    m_on_export_zfo_activate);
	settings.setValue("on_import_database_dir_activate",
	    m_on_import_database_dir_activate);
	settings.endGroup();
}

void MainWindow::refreshAccountList(const QString &userName)
{
	debugSlotCall();

	if (userName.isEmpty()) {
		logWarningNL("%s",
		    "Cannot refresh account list on empty user name.");
		return;
	}

	QModelIndex selectedIdx(currentAccountModelIndex());
	const QString selectedUserName(m_accountModel.userName(selectedIdx));
	/* There may be no account selected. */

	enum AccountModel::NodeType nodeType = AccountModel::nodeUnknown;
	MessageDb::MessageType msgType = MessageDb::TYPE_RECEIVED;
	QString year;
	qint64 dmId = -1;

	if (selectedUserName == userName) {
		/* Currently selected is the one being processed. */
		nodeType = AccountModel::nodeType(selectedIdx);
		switch (nodeType) {
		case AccountModel::nodeRecentReceived:
		case AccountModel::nodeRecentSent:
			break;
		case AccountModel::nodeReceivedYear:
			year = selectedIdx.data(ROLE_PLAIN_DISPLAY).toString();
			msgType = MessageDb::TYPE_RECEIVED;
			break;
		case AccountModel::nodeSentYear:
			year = selectedIdx.data(ROLE_PLAIN_DISPLAY).toString();
			msgType = MessageDb::TYPE_SENT;
			break;
		default:
			nodeType = AccountModel::nodeUnknown;
			break;
		}

		if (nodeType != AccountModel::nodeUnknown) {
			QModelIndexList firstMsgColumnIdxs(
			    currentFrstColMessageIndexes());
			if (firstMsgColumnIdxs.size() == 1) {
				QModelIndex msgIdx(firstMsgColumnIdxs.first());
				if (msgIdx.isValid()) {
					dmId = msgIdx.sibling(msgIdx.row(), 0)
					    .data().toLongLong();
				}
			}
		}
	}

	/* Redraw views' content. */
	const QModelIndex topAcntIdx(m_accountModel.topAcntIndex(userName));
	if (topAcntIdx.isValid()) {
		bool prohibitYearRemoval =
		    (nodeType == AccountModel::nodeReceivedYear) ||
		    (nodeType == AccountModel::nodeSentYear);
		regenerateAccountModelYears(topAcntIdx, prohibitYearRemoval);
	}

	/*
	 * Force repaint.
	 * TODO -- A better solution?
	 */
	ui->accountList->repaint();
	if ((nodeType != AccountModel::nodeUnknown) && (dmId != -1)) {
		if (!year.isEmpty()) {
			QModelIndex yearIdx(accountYearlyIndex(userName, year,
			    msgType));
			if (yearIdx.isValid()) {
				ui->accountList->setCurrentIndex(yearIdx);
			}
		}

		accountItemCurrentChanged(selectedIdx);

		if (dmId != -1) {
			QModelIndex msgIdx(messageIndex(ui->messageList, dmId));
			if (msgIdx.isValid()) {
				ui->messageList->setCurrentIndex(msgIdx);
				ui->messageList->scrollTo(msgIdx);
			}
		}
	} else {
		/* Update message model. */
		accountItemCurrentChanged(selectedIdx);
	}
}

void MainWindow::showAboutApplicationDialog(void)
{
	DlgAbout::about(this);
}

void MainWindow::showImportDatabaseDialog(void)
{
	QStringList createdUserNames(
	    DlgCreateAccountFromDb::createAccount(m_accountModel,
	        m_on_import_database_dir_activate, this));

	if (!createdUserNames.isEmpty()) {
		refreshAccountList(createdUserNames.last());
		saveSettings();
	}

	activateAccountMenuActions(true,
	    m_accountModel.userName(currentAccountModelIndex()));
	ui->accountList->expandAll();
}

void MainWindow::sendMessageToRecordsManagement(const QString &userName,
    MessageDb::MsgId msgId)
{
	if (Q_UNLIKELY(userName.isEmpty() || (msgId.dmId < 0))) {
		Q_ASSERT(0);
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}
	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		return;
	}

	QByteArray msgRaw(messageDb->getCompleteMessageRaw(msgId.dmId));
	if (msgRaw.isEmpty()) {

		if (!messageMissingOfferDownload(msgId,
		        tr("Message export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(msgId.deliveryTime, false);
		if (Q_NULLPTR == messageDb) {
			Q_ASSERT(0);
			logErrorNL(
			    "Could not access database of freshly downloaded message '%" PRId64 "'.",
			    UGLY_QINT64_CAST msgId.dmId);
			return;
		}

		msgRaw = messageDb->getCompleteMessageRaw(msgId.dmId);
		if (msgRaw.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	/* Show send to records management dialogue. */
	DlgRecordsManagementUpload::uploadMessage(*GlobInstcs::recMgmtSetPtr,
	    msgId.dmId,
	    QString("%1_%2.zfo").arg(dzPrefix(messageDb, msgId.dmId)).arg(msgId.dmId),
	    msgRaw, this);

	QList<qint64> msgIdList;
	msgIdList.append(msgId.dmId);

	m_messageTableModel.refillRecordsManagementColumn(msgIdList,
	    DbMsgsTblModel::RECMGMT_NEG_COL);
}

/* ========================================================================= */
/*
 * Authenticate message from ZFO file.
 */
int MainWindow::authenticateMessageFromZFO(void)
/* ========================================================================= */
{
	debugFuncCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	QString fileName = QFileDialog::getOpenFileName(this,
	    tr("Add ZFO file"), "", tr("ZFO file (*.zfo)"));

	if (fileName.isNull()) {
		return TaskAuthenticateMessage::AUTH_CANCELLED;
	}

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		return TaskAuthenticateMessage::AUTH_ISDS_ERROR;
	}

	showStatusTextPermanently(tr("Verifying the ZFO file \"%1\"")
	    .arg(fileName));

	TaskAuthenticateMessage *task;

	task = new (std::nothrow) TaskAuthenticateMessage(userName, fileName);
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	TaskAuthenticateMessage::Result result = task->m_result;
	delete task;

	return result;
}


/* ========================================================================= */
/*
 * Authenticate message file dialog.
 */
void MainWindow::authenticateMessageFile(void)
/* ========================================================================= */
{
	debugSlotCall();

	switch (authenticateMessageFromZFO()) {
	case TaskAuthenticateMessage::AUTH_SUCCESS:
		showStatusTextWithTimeout(tr("Server Datové schránky confirms "
		    "that the message is authentic."));
		QMessageBox::information(this, tr("Message is authentic"),
		    tr("Message was <b>successfully verified</b> "
		    "against data on the server Datové schránky.") +
		    "<br/><br/>" +
		    tr("This message has passed through the system of "
		    "Datové schránky and has not been tampered with since."),
		    QMessageBox::Ok);
		break;
	case TaskAuthenticateMessage::AUTH_NOT_EQUAL:
		showStatusTextWithTimeout(tr("Server Datové schránky confirms "
		    "that the message is not authentic."));
		QMessageBox::critical(this, tr("Message is not authentic"),
		    tr("Message was <b>not</b> authenticated as processed "
		    "by the system Datové schránky.") + "<br/><br/>" +
		    tr("It is either not a valid ZFO file or it was modified "
		    "since it was downloaded from Datové schránky."),
		    QMessageBox::Ok);
		break;
	case TaskAuthenticateMessage::AUTH_ISDS_ERROR:
		showStatusTextWithTimeout(tr("Message authentication failed."));
		QMessageBox::warning(this, tr("Message authentication failed"),
		    tr("Authentication of message has been stopped because "
		    "the connection to server Datové schránky failed!\n"
		    "Check your internet connection."),
		    QMessageBox::Ok);
		break;
	case TaskAuthenticateMessage::AUTH_DATA_ERROR:
		showStatusTextWithTimeout(tr("Message authentication failed."));
		QMessageBox::warning(this, tr("Message authentication failed"),
		    tr("Authentication of message has been stopped because "
		    "the message file has wrong format!"),
		    QMessageBox::Ok);
		break;
	case TaskAuthenticateMessage::AUTH_CANCELLED:
		break;
	default:
		showStatusTextWithTimeout(tr("Message authentication failed."));
		QMessageBox::warning(this, tr("Message authentication failed"),
		    tr("An undefined error occurred!\nTry again."),
		    QMessageBox::Ok);
		break;
	}
}


/* ========================================================================= */
/*
 * Verifies selected message and creates response dialog.
 */
void MainWindow::verifySelectedMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* First column. */
	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));

	const MessageDb::MsgId msgId(msgMsgId(firstMsgColumnIdxs.first()));
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(msgId.dmId >= 0);
	if (!msgId.deliveryTime.isValid()) {
		Q_ASSERT(0);
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		return;
	}

	const Isds::Hash hashDb(messageDb->getMessageHash(msgId.dmId));
	if (hashDb.isNull()) {
		logErrorNL(
		    "Error obtaining hash of message '%" PRId64 "' from local database.",
		    UGLY_QINT64_CAST msgId.dmId);
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::warning(this, tr("Verification error"),
		    tr("The message hash is not in local database.\n"
		        "Please download complete message from ISDS and try again."),
		    QMessageBox::Ok);
		return;
	}

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::critical(this, tr("Verification error"),
		    tr("An undefined error occurred!\nTry again."),
		    QMessageBox::Ok);
		return;
	}

	TaskVerifyMessage *task = new (std::nothrow) TaskVerifyMessage(userName,
	    msgId.dmId, hashDb);
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	TaskVerifyMessage::Result result = task->m_result;
	delete task; task = Q_NULLPTR;

	switch (result) {
	case TaskVerifyMessage::VERIFY_SUCCESS:
		showStatusTextWithTimeout(
		    tr("Server Datové schránky confirms that the message is valid."));
		QMessageBox::information(this, tr("Message is valid"),
		    tr("Message was <b>successfully verified</b> against data on the server Datové schránky.") +
		    "<br/><br/>" +
		    tr("This message has passed through the system of Datové schránky and has not been tampered with since."),
		    QMessageBox::Ok);
		break;
	case TaskVerifyMessage::VERIFY_NOT_EQUAL:
		showStatusTextWithTimeout(
		    tr("Server Datové schránky confirms that the message is not valid."));
		QMessageBox::critical(this, tr("Message is not valid"),
		    tr("Message was <b>not</b> authenticated as processed by the system Datové schránky.") +
		    "<br/><br/>" +
		    tr("It is either not a valid ZFO file or it was modified since it was downloaded from Datové schránky."),
		     QMessageBox::Ok);
		break;
	case TaskVerifyMessage::VERIFY_ISDS_ERR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::warning(this, tr("Verification failed"),
		    tr("Authentication of message has been stopped because the connection to server Datové schránky failed!\n"
		        "Check your internet connection."),
		    QMessageBox::Ok);
		break;
	case TaskVerifyMessage::VERIFY_ERR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::critical(this, tr("Verification error"),
		    tr("The message hash cannot be verified because an internal error occurred!\n"
		        "Try again."),
		    QMessageBox::Ok);
		break;
	default:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::critical(this, tr("Verification error"),
		    tr("An undefined error occurred!\nTry again."),
		    QMessageBox::Ok);
		break;
	}
}

void MainWindow::showViewMessageFromZFODialog(void)
{
	debugSlotCall();

	QString fileName = QFileDialog::getOpenFileName(this,
	    tr("Add ZFO file"), m_on_export_zfo_activate,
	    tr("ZFO file (*.zfo)"));

	if (fileName.isEmpty()) {
		return;
	}

	m_on_export_zfo_activate =
	    QFileInfo(fileName).absoluteDir().absolutePath();

	/* Generate dialog showing message content. */
	DlgViewZfo::view(fileName, this);
}

void MainWindow::showExportCorrespondenceOverviewDialog(void)
{
	debugSlotCall();

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));

	setAccountStoragePaths(userName);

	DlgCorrespondenceOverview::exportData(*dbSet, dbId, userName,
	    *GlobInstcs::tagDbPtr, m_export_correspond_dir, this);
	storeExportPath(userName);
}

void MainWindow::showImportZFOActionDialog(void)
{
	debugSlotCall();

	enum Imports::Type zfoType = Imports::IMPORT_MESSAGE;
	enum DlgImportZFO::ZFOlocation locationType =
	    DlgImportZFO::IMPORT_FROM_DIR;
	bool checkZfoOnServer = false;

	/* Import setting dialogue. */
	if (!DlgImportZFO::getImportConfiguration(zfoType, locationType,
	        checkZfoOnServer, this)) {
		return;
	}

	// get userName and pointer to database for all accounts from settings
	const QList<Task::AccountDescr> accountList(messageDbListForAllAccounts(
	    this, m_accountModel, ui->accountList, checkZfoOnServer));

	bool includeSubdir = false;
	QString importDir;
	QStringList fileList, filePathList;
	QStringList nameFilter("*.zfo");
	QDir directory(QDir::home());

	// dialog select zfo files or directory
	switch (locationType) {
	case DlgImportZFO::IMPORT_FROM_SUBDIR:
		includeSubdir = true;
	case DlgImportZFO::IMPORT_FROM_DIR:
		importDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory"), m_import_zfo_path,
		    QFileDialog::ShowDirsOnly |
		    QFileDialog::DontResolveSymlinks);

		if (importDir.isEmpty()) {
			return;
		}

		m_import_zfo_path = importDir;

		if (includeSubdir) {
			QDirIterator it(importDir, nameFilter, QDir::Files,
			    QDirIterator::Subdirectories);
			while (it.hasNext()) {
				filePathList.append(it.next());
			}
		} else {
			directory.setPath(importDir);
			fileList = directory.entryList(nameFilter);
			for (int i = 0; i < fileList.size(); ++i) {
				filePathList.append(
				    importDir + "/" + fileList.at(i));
			}
		}

		if (filePathList.isEmpty()) {
			logWarning(
			    "No *.zfo files in selected directory '%s'.\n",
			    importDir.toUtf8().constData());
			showStatusTextWithTimeout(tr("ZFO file(s) not found in "
			    "selected directory."));
			QMessageBox::warning(this,
			    tr("No ZFO file(s)"),
			    tr("ZFO file(s) not found in selected directory."),
			    QMessageBox::Ok);
			return;
		}

		break;

	case DlgImportZFO::IMPORT_SEL_FILES:
		filePathList = QFileDialog::getOpenFileNames(this,
		    tr("Select ZFO file(s)"), m_import_zfo_path,
		    tr("ZFO file (*.zfo)"));

		if (filePathList.isEmpty()) {
			logWarning("%s\n", "No selected *.zfo files.");
			showStatusTextWithTimeout(
			    tr("ZFO file(s) not selected."));
			return;
		}

		m_import_zfo_path =
		    QFileInfo(filePathList.at(0)).absoluteDir().absolutePath();
		break;

	default:
		return;
		break;
	}

	logInfo("Trying to import %d ZFO files.\n", filePathList.count());

	if (filePathList.count() == 0) {
		logInfo("%s\n", "No *.zfo files in received file list.");
		showStatusTextWithTimeout(tr("No ZFO files to import."));
		return;
	}

	if (accountList.isEmpty()) {
		logInfo("%s\n", "No accounts to import into.");
		showStatusTextWithTimeout(tr("There is no account to "
		    "import of ZFO files into."));
		return;
	}

	/* Block import GUI buttons. */
	ui->actionImport_messages_from_database->setEnabled(false);
	ui->actionImport_ZFO_file_into_database->setEnabled(false);

	QString errTxt;

	ZFOImportCtx &importCtx(ZFOImportCtx::getInstance());
	importCtx.clear();

	Imports::importZfoIntoDatabase(filePathList, accountList,
	    zfoType, checkZfoOnServer, importCtx.zfoFilesToImport,
	    importCtx.importFailed, importCtx.numFilesToImport, errTxt);

	if (!errTxt.isEmpty()) {
		logInfo("%s\n", errTxt.toUtf8().constData());
		showStatusTextWithTimeout(errTxt);
	}

	clearProgressBar();
}

void MainWindow::showAppHelpInTheBrowser(void)
{
	QDesktopServices::openUrl(QUrl(DATOVKA_ONLINE_HELP_URL,
	    QUrl::TolerantMode));
}

void MainWindow::goHome(void)
{
	QDesktopServices::openUrl(QUrl(DATOVKA_HOMEPAGE_URL,
	    QUrl::TolerantMode));
}

void MainWindow::viewSelectedMessageViaFilter(QObject *mwPtr)
{
	if (0 == mwPtr) {
		return;
	}

	MainWindow *mw = dynamic_cast<MainWindow *>(mwPtr);
	if (0 == mw) {
		Q_ASSERT(0);
		return;
	}

	mw->viewSelectedMessage();
}


/* ========================================================================= */
/*
 * Download complete message synchronously without worker and thread
 */
bool MainWindow::downloadCompleteMessage(MessageDb::MsgId &msgId)
/* ========================================================================= */
{
	debugFuncCall();

	/* selection().indexes() ? */

	const QModelIndex acntIdx(currentAccountModelIndex());
	enum MessageDirection msgDirect =
	    messageDirection(acntIdx, MSG_RECEIVED);
	const QString userName(m_accountModel.userName(acntIdx));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return false;
	}

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName) &&
	    !connectToIsds(userName)) {
		return false;
	}
	TaskDownloadMessage *task;

	task = new (std::nothrow) TaskDownloadMessage(
	    userName, dbSet, msgDirect, msgId, false);
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);
	bool ret = TaskDownloadMessage::DM_SUCCESS == task->m_result;
	if (ret) {
		msgId.deliveryTime = task->m_mId.deliveryTime;
	}

	delete task;

	/* Process all pending events. */
	QCoreApplication::processEvents();

	return ret;
}

bool MainWindow::messageMissingOfferDownload(MessageDb::MsgId &msgId,
    const QString &title)
{
	debugFuncCall();

	int dlgRet = DlgMsgBox::message(this, QMessageBox::Warning, title,
	    tr("Complete message '%1' is missing.").arg(msgId.dmId),
	    tr("First you must download the complete message to continue with the action.") +
	    "\n\n" +
	    tr("Do you want to download the complete message now?"),
	    QString(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

	if ((QMessageBox::Yes == dlgRet) && downloadCompleteMessage(msgId)) {
		showStatusTextWithTimeout(
		    tr("Complete message '%1' has been downloaded.").
		    arg(msgId.dmId));
		return true;
	} else {
		showStatusTextWithTimeout(
		    tr("Complete message '%1' has not been downloaded.").
		    arg(msgId.dmId));
		return false;
	}
}

void MainWindow::exportSelectedMessagesAsZFO(void)
{
	debugSlotCall();

	doExportOfSelectedFiles(Exports::ZFO_MESSAGE);
}

void MainWindow::exportSelectedDeliveryInfosAsZFO(void)
{
	debugSlotCall();

	doExportOfSelectedFiles(Exports::ZFO_DELIVERY);
}

void MainWindow::exportSelectedDeliveryInfosAsPDF(void)
{
	debugSlotCall();

	doExportOfSelectedFiles(Exports::PDF_DELIVERY);
}

void MainWindow::exportSelectedMessageEnvelopesAsPDF(void)
{
	debugSlotCall();

	doExportOfSelectedFiles(Exports::PDF_ENVELOPE);
}

void MainWindow::exportSelectedMessageEnvelopeAttachments(void)
{
	debugSlotCall();

	const QList<MessageDb::MsgId> msgIds(
	    msgMsgIds(currentFrstColMessageIndexes()));
	if (0 == msgIds.size()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	setAccountStoragePaths(userName);

	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Select target folder to save"), m_on_export_zfo_activate,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (newDir.isEmpty()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));
	QString errStr;

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	foreach (MessageDb::MsgId msgId, msgIds) {
		Q_ASSERT(msgId.dmId >= 0);
		if (Exports::EXP_NOT_MSG_DATA ==
		    Exports::exportEnvAndAttachments(*dbSet, newDir,
		    userName, accountName, dbId, msgId, errStr)) {
			if (messageMissingOfferDownload(msgId, errStr)) {
				Exports::exportEnvAndAttachments(*dbSet,
				    newDir, userName, accountName, dbId, msgId,
				    errStr);
			}
		}
	}

	m_on_export_zfo_activate = newDir;
	storeExportPath(userName);
}

void MainWindow::sendMessagesZfoEmail(void)
{
	debugSlotCall();

	const QList<MessageDb::MsgId> msgIds(
	    msgMsgIds(currentFrstColMessageIndexes()));
	if (0 == msgIds.size()) {
		return;
	}

	QString emailMessage;
	const QString boundary("-----" +
	    Utility::generateRandomString(16) + "_" +
	    QDateTime::currentDateTimeUtc().toString(
	        "dd.MM.yyyy-HH:mm:ss.zzz"));

	QString subject((1 == msgIds.size()) ?
	    tr("Data message") : tr("Data messages"));

	subject += " " + QString::number(msgIds.first().dmId);
	if (msgIds.size() > 1) {
		for (int i = 1; i < msgIds.size(); ++i) {
			subject += ", " + QString::number(msgIds.at(i).dmId);
		}
	}

	createEmailMessage(emailMessage, subject, boundary);

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	foreach (MessageDb::MsgId msgId, msgIds) {
		Q_ASSERT(msgId.dmId >= 0);

		MessageDb *messageDb = dbSet->accessMessageDb(
		    msgId.deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			return;
		}

		QByteArray base64 = messageDb->getCompleteMessageBase64(msgId.dmId);
		if (base64.isEmpty()) {

			if (!messageMissingOfferDownload(msgId,
			        tr("Message export error!"))) {
				return;
			}

			messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
			    false);
			if (0 == messageDb) {
				Q_ASSERT(0);
				logErrorNL(
				    "Could not access database of freshly downloaded message '%" PRId64 "'.",
				    UGLY_QINT64_CAST msgId.dmId);
				return;
			}

			base64 = messageDb->getCompleteMessageBase64(msgId.dmId);
			if (base64.isEmpty()) {
				Q_ASSERT(0);
				return;
			}
		}

		QString attachName(
		    QString("%1_%2.zfo").arg(dzPrefix(messageDb, msgId.dmId)).arg(msgId.dmId));
		if (attachName.isEmpty()) {
			Q_ASSERT(0);
			return;
		}

		addAttachmentToEmailMessage(emailMessage, attachName, base64,
		    boundary);
	}

	finishEmailMessage(emailMessage, boundary);

	/* Email is encoded using UTF-8. */
	QString tmpEmailFile = writeTemporaryFile(
	    TMP_ATTACHMENT_PREFIX "mail.eml", emailMessage.toUtf8());

	if (!tmpEmailFile.isEmpty()) {
		QDesktopServices::openUrl(QUrl::fromLocalFile(tmpEmailFile));
	}
}

void MainWindow::sendAllAttachmentsEmail(void)
{
	debugSlotCall();

	const QList<MessageDb::MsgId> msgIds(
	    msgMsgIds(currentFrstColMessageIndexes()));
	if (0 == msgIds.size()) {
		return;
	}

	QString emailMessage;
	const QString boundary("-----" +
	    Utility::generateRandomString(16) + "_" +
	    QDateTime::currentDateTimeUtc().toString(
	        "dd.MM.yyyy-HH:mm:ss.zzz"));

	QString subject((1 == msgIds.size()) ?
	    tr("Attachments of message") : tr("Attachments of messages"));

	subject += " " + QString::number(msgIds.first().dmId);
	if (msgIds.size() > 1) {
		for (int i = 1; i < msgIds.size(); ++i) {
			subject += ", " + QString::number(msgIds.at(i).dmId);
		}
	}

	createEmailMessage(emailMessage, subject, boundary);

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	foreach (MessageDb::MsgId msgId, msgIds) {
		Q_ASSERT(msgId.dmId >= 0);

		MessageDb *messageDb = dbSet->accessMessageDb(
		    msgId.deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			return;
		}

		QList<Isds::Document> attachList =
		    messageDb->getMessageAttachments(msgId.dmId);
		if (attachList.isEmpty()) {

			if (!messageMissingOfferDownload(msgId,
			        tr("Message export error!"))) {
				return;
			}

			messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
			    false);
			if (0 == messageDb) {
				Q_ASSERT(0);
				logErrorNL(
				    "Could not access database of freshly downloaded message '%" PRId64 "'.",
				    UGLY_QINT64_CAST msgId.dmId);
				return;
			}

			attachList = messageDb->getMessageAttachments(msgId.dmId);
			if (attachList.isEmpty()) {
				Q_ASSERT(0);
				return;
			}
		}

		foreach (const Isds::Document &attach, attachList) {
			Q_ASSERT(!attach.fileDescr().isEmpty());
			Q_ASSERT(!attach.binaryContent().isEmpty());

			addAttachmentToEmailMessage(emailMessage,
			    attach.fileDescr(), attach.base64Content().toUtf8(),
			    boundary);
		}
	}

	finishEmailMessage(emailMessage, boundary);

	QString tmpEmailFile = writeTemporaryFile(
	    TMP_ATTACHMENT_PREFIX "mail.eml", emailMessage.toUtf8());

	if (!tmpEmailFile.isEmpty()) {
		QDesktopServices::openUrl(QUrl::fromLocalFile(tmpEmailFile));
	}
}

void MainWindow::sendAttachmentsEmail(void)
{
	debugSlotCall();

	QModelIndexList attachmentIndexes(currentFrstColAttachmentIndexes());

	qint64 dmId = -1;
	{
		QModelIndex messageIndex(currentSingleMessageIndex());
		if (!messageIndex.isValid()) {
			Q_ASSERT(0);
			return;
		}
		dmId = messageIndex.sibling(messageIndex.row(), 0).data().
		    toLongLong();
	}

	QString emailMessage;
	const QString boundary("-----" +
	    Utility::generateRandomString(16) + "_" +
	    QDateTime::currentDateTimeUtc().toString(
	        "dd.MM.yyyy-HH:mm:ss.zzz"));

	QString subject = ((1 == attachmentIndexes.size()) ?
	    tr("Attachment of message %1") : tr("Attachments of message %1")).
	    arg(dmId);

	createEmailMessage(emailMessage, subject, boundary);

	foreach (const QModelIndex &attachIdx, attachmentIndexes) {
		QString attachmentName(attachIdx.sibling(attachIdx.row(),
		    AttachmentTblModel::FNAME_COL).data().toString());
		const QByteArray binaryData(attachIdx.sibling(attachIdx.row(),
		    AttachmentTblModel::BINARY_CONTENT_COL).data().toByteArray());

		addAttachmentToEmailMessage(emailMessage, attachmentName,
		    binaryData.toBase64(), boundary);
	}

	finishEmailMessage(emailMessage, boundary);

	QString tmpEmailFile = writeTemporaryFile(
	    TMP_ATTACHMENT_PREFIX "mail.eml", emailMessage.toUtf8());

	if (!tmpEmailFile.isEmpty()) {
		QDesktopServices::openUrl(QUrl::fromLocalFile(tmpEmailFile));
	}
}

/* ========================================================================= */
/*
 * Open selected message in external application.
 */
void MainWindow::openSelectedMessageExternally(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* First column. */
	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const MessageDb::MsgId msgId(msgMsgId(firstMsgColumnIdxs.first()));
	if (msgId.dmId < 0) {
		Q_ASSERT(0);
		return;
	}
	if (!msgId.deliveryTime.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}
	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return;
	}

	QByteArray base64 = messageDb->getCompleteMessageBase64(msgId.dmId);
	if (base64.isEmpty()) {
		DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Datovka - Export error!"),
		    tr("Cannot export the message '%1'.").arg(msgId.dmId),
		    tr("First you must download the message before its export..."),
		    QString());
		return;
	}

	QString fileName(TMP_ATTACHMENT_PREFIX +
	    QString("%1_%2.zfo").arg(dzPrefix(messageDb, msgId.dmId)).arg(msgId.dmId));
	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	QByteArray data = QByteArray::fromBase64(base64);

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		showStatusTextWithTimeout(
		    tr("Message '%1' stored to temporary file '%2'.")
		        .arg(msgId.dmId).arg(fileName));
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		showStatusTextWithTimeout(
		    tr("Message '%1' couldn't be stored to temporary file.")
		        .arg(msgId.dmId));
		QMessageBox::warning(this,
		    tr("Error opening message '%1'.").arg(msgId.dmId),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Open delivery information externally.
 */
void MainWindow::openDeliveryInfoExternally(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QString userName =
	    m_accountModel.userName(currentAccountModelIndex());

	/* First column. */
	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const MessageDb::MsgId msgId(msgMsgId(firstMsgColumnIdxs.first()));
	if (msgId.dmId < 0) {
		Q_ASSERT(0);
		return;
	}
	if (!msgId.deliveryTime.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}
	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return;
	}

	QByteArray base64 = messageDb->getDeliveryInfoBase64(msgId.dmId);
	if (base64.isEmpty()) {
		DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Datovka - Export error!"),
		    tr("Cannot export the message '%1'.").arg(msgId.dmId),
		    tr("First you must download the message before its export..."),
		    QString());
		return;
	}

	QString fileName(TMP_ATTACHMENT_PREFIX +
	    QString("%1_%2_info.zfo").arg(dzPrefix(messageDb, msgId.dmId)).arg(msgId.dmId));
	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	QByteArray data = QByteArray::fromBase64(base64);

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		showStatusTextWithTimeout(
		    tr("Message acceptance information '%1' stored to temporary file '%2'.")
		        .arg(msgId.dmId).arg(fileName));
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		showStatusTextWithTimeout(
		    tr("Message acceptance information '%1' couldn't be stored to temporary file.")
		        .arg(msgId.dmId));
		QMessageBox::warning(this,
		    tr("Error opening message '%1'.").arg(msgId.dmId),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}

void MainWindow::getStoredMsgInfoFromRecordsManagement(void)
{
	debugSlotCall();

	QStringList userNames;
	for (int row = 0; row < m_accountModel.rowCount(); ++row) {
		userNames.append(
		    m_accountModel.userName(m_accountModel.index(row, 0)));
	}

	QList<DlgRecordsManagementStored::AcntData> accounts;
	foreach (const QString &userName, userNames) {
		MessageDbSet *dbSet = accountDbSet(userName);
		if (Q_NULLPTR == dbSet) {
			Q_ASSERT(0);
			return;
		}
		accounts.append(DlgRecordsManagementStored::AcntData(
		    (*GlobInstcs::acntMapPtr)[userName].accountName(),
		    userName, dbSet));
	}

	DlgRecordsManagementStored::updateStoredInformation(
	    *GlobInstcs::recMgmtSetPtr, accounts, this);

	m_messageTableModel.fillRecordsManagementColumn(
	    DbMsgsTblModel::RECMGMT_NEG_COL);
}

void MainWindow::sendSelectedMessageToRecordsManagement(void)
{
	debugSlotCall();

	QModelIndex msgIndex;
	{
		QModelIndexList msgIndexes(currentFrstColMessageIndexes());

		if (msgIndexes.size() != 1) {
			/* Do nothing when multiple messages selected. */
			return;
		}

		msgIndex = msgIndexes.first();
	}

	if (!msgIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDb::MsgId msgId(msgMsgId(msgIndex));
	Q_ASSERT(msgId.dmId >= 0);

	sendMessageToRecordsManagement(userName, msgId);
}

void MainWindow::showSignatureDetailsDialog(void)
{
	debugSlotCall();

	const MessageDb::MsgId msgId(msgMsgId(currentSingleMessageIndex()));
	if (msgId.dmId < 0) {
		Q_ASSERT(0);
		return;
	}
	if (!msgId.deliveryTime.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	DlgSignatureDetail::detail(*dbSet, msgId, this);
}

bool MainWindow::logInGUI(IsdsSessions &isdsSessions,
    AcntSettings &acntSettings)
{
	if (!acntSettings.isValid()) {
		return false;
	}

	const QString userName(acntSettings.userName());
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}
	/* Create clean session if session doesn't exist. */
	if (!isdsSessions.holdsSession(userName)) {
		isdsSessions.createCleanSession(userName,
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	enum IsdsLogin::ErrorCode errCode;
	IsdsLogin loginCtx(isdsSessions, acntSettings);

	do {
		errCode = loginCtx.logIn();

		acntSettings._setOtp(QString()); /* Erase OTP. */

		switch (errCode) {
		case IsdsLogin::IsdsLogin::EC_OK:
			mui_statusOnlineLabel->setText(tr("Mode: online"));
			break;
		case IsdsLogin::EC_NO_CRT_AGAIN:
			{
				QMessageBox::critical(this,
				    tr("Invalid certificate data"),
				    tr("The certificate or the supplied pass-phrase are invalid.") +
				    "<br/><br/>" +
				    tr("Please enter a path to a valid certificate and/or provide a correct key to unlock the certificate."),
				    QMessageBox::Ok);
			}
			showStatusTextWithTimeout(tr(
			    "Bad certificate data for account \"%1\".")
			    .arg(acntSettings.accountName()));
			break;
		case IsdsLogin::EC_NOT_LOGGED_IN:
		case IsdsLogin::EC_PARTIAL_SUCCESS_AGAIN:
		case IsdsLogin::EC_ISDS_ERR:
			{
				const QPair<QString, QString> pair(
				    loginCtx.dialogueErrMsg());
				QMessageBox::critical(this, pair.first,
				    pair.second, QMessageBox::Ok);
			}
			showStatusTextWithTimeout(
			    tr("It was not possible to connect to the data box from account '%1'.")
			        .arg(acntSettings.accountName()));
			break;
		case IsdsLogin::EC_NOT_IMPL:
			showStatusTextWithTimeout(tr(
			    "The log-in method used in account \"%1\" is not implemented.")
			    .arg(acntSettings.accountName()));
			return false;
			break;
		default:
			break;
		}

		switch (errCode) {
		case IsdsLogin::EC_OK:
			/* Do nothing. */
			break;
		case IsdsLogin::EC_NO_PWD:
			if (!DlgCreateAccount::modify(acntSettings,
			        DlgCreateAccount::ACT_PWD,
			        ui->actionSync_all_accounts->text(), this)) {
				acntSettings.setRememberPwd(false);
				acntSettings.setPassword(QString());
				showStatusTextWithTimeout(
				    tr("It was not possible to connect to the data box from account '%1'.")
				        .arg(acntSettings.accountName()));
				return false;
			}
			break;
		case IsdsLogin::EC_NO_CRT:
		case IsdsLogin::EC_NO_CRT_AGAIN:
			/* Erase pass-phrase. */
			acntSettings._setPassphrase(QString());

			if (!DlgCreateAccount::modify(acntSettings,
			        DlgCreateAccount::ACT_CERT,
			        ui->actionSync_all_accounts->text(), this)) {
				showStatusTextWithTimeout(
				    tr("It was not possible to connect to the data box from account '%1'.")
				        .arg(acntSettings.accountName()));
				return false;
			}
			break;
		case IsdsLogin::EC_NO_CRT_PWD:
			/* Erase pass-phrase. */
			acntSettings._setPassphrase(QString());

			if (!DlgCreateAccount::modify(acntSettings,
			        DlgCreateAccount::ACT_CERTPWD,
			        ui->actionSync_all_accounts->text(), this)) {
				showStatusTextWithTimeout(
				    tr("It was not possible to connect to the data box from account '%1'.")
				        .arg(acntSettings.accountName()));
				return false;
			}
			break;
		case IsdsLogin::EC_NO_CRT_PPHR:
			{
				/* Ask the user for password. */
				bool ok;
				QString enteredText = QInputDialog::getText(
				    this, tr("Password required"),
				    tr("Account: %1\n"
				        "User name: %2\n"
				        "Certificate file: %3\n"
				        "Enter password to unlock certificate file:")
				        .arg(acntSettings.accountName())
				        .arg(userName)
				        .arg(acntSettings.p12File()),
				    QLineEdit::Password, QString(), &ok);
				if (ok) {
					/*
					 * We cannot pass null string into the
					 * login operation a it will return
					 * immediately. But the dialogue can
					 * return null string.
					 */
					if (enteredText.isNull()) {
						enteredText = "";
					}
					acntSettings._setPassphrase(enteredText);
				} else {
					/* Aborted. */
					return false;
				}
			}
			break;
		case IsdsLogin::EC_NO_OTP:
		case IsdsLogin::EC_PARTIAL_SUCCESS:
		case IsdsLogin::EC_PARTIAL_SUCCESS_AGAIN:
			{
				QString msgTitle(tr("Enter OTP security code"));
				QString msgBody(
				    tr("Account \"%1\" requires authentication via OTP<br/>security code for connection to data box.")
				        .arg(acntSettings.accountName()) +
				    "<br/><br/>" +
				    tr("Enter OTP security code for account") +
				    "<br/><b>" +
				    acntSettings.accountName() +
				    " </b>(" + userName + ").");
				QString otpCode;
				do {
					bool ok;
					otpCode = QInputDialog::getText(this,
					    msgTitle, msgBody,
					    QLineEdit::Normal, otpCode, &ok,
					    Qt::WindowStaysOnTopHint);
					if (!ok) {
						showStatusTextWithTimeout(
						    tr("It was not possible to connect to the data box from account '%1'.")
						        .arg(acntSettings.accountName()));
						return false;
					}
				} while (otpCode.isEmpty());

				acntSettings._setOtp(otpCode);
			}
			break;
		case IsdsLogin::EC_NEED_TOTP_ACK:
			{
				QMessageBox::StandardButton reply =
				    QMessageBox::question(this,
				        tr("SMS code for account ") + acntSettings.accountName(),
				        tr("Account \"%1\" requires authentication via security code for connection to data box.")
				            .arg(acntSettings.accountName()) +
				        "<br/>" +
				        tr("Security code will be sent to you via a Premium SMS.") +
				        "<br/><br/>" +
				        tr("Do you want to send a Premium SMS with a security code into your mobile phone?"),
				        QMessageBox::Yes | QMessageBox::No,
				        QMessageBox::Yes);

				if (reply == QMessageBox::No) {
					showStatusTextWithTimeout(tr(
					    "It was not possible to connect to the data box from account '%1'.")
					    .arg(acntSettings.accountName()));
					return false;
				}

				acntSettings._setOtp(""); /* Must be empty. */
			}
			break;
		case IsdsLogin::EC_NOT_LOGGED_IN:
			if (!DlgCreateAccount::modify(acntSettings,
			        DlgCreateAccount::ACT_EDIT,
			        ui->actionSync_all_accounts->text(), this)) {
				/* Don't clear password here. */
				showStatusTextWithTimeout(
				    tr("It was not possible to connect to the data box from account '%1'.")
				        .arg(acntSettings.accountName()));
				return false;
			}
			break;
		default:
			logErrorNL(
			    "Received log-in error code %d for account '%s'.",
			    errCode,
			    acntSettings.accountName().toUtf8().constData());
			return false;
			break;
		}
	} while (errCode != IsdsLogin::EC_OK);

	if (errCode != IsdsLogin::EC_OK) {
		Q_ASSERT(0);
		return false;
	}

	return true;
}

bool MainWindow::connectToIsds(const QString &userName)
{
	AcntSettings settingsCopy((*GlobInstcs::acntMapPtr)[userName]);

	if (!logInGUI(*GlobInstcs::isdsSessionsPtr, settingsCopy)) {
		return false;
	}

	/* Logged in. */
	(*GlobInstcs::acntMapPtr)[userName] = settingsCopy;
	/*
	 * Catching the following signal is required only when account has
	 * changed.
	 *
	 * The account model catches the signal.
	 */
	emit GlobInstcs::acntMapPtr->accountDataChanged(userName);
	saveSettings();

	/* Get account information if possible. */
	if (!IsdsHelper::getOwnerInfoFromLogin(userName)) {
		logWarningNL("Owner information for account '%s' (login %s) could not be acquired.",
		    settingsCopy.accountName().toUtf8().constData(),
		    userName.toUtf8().constData());
	}
	if (!IsdsHelper::getUserInfoFromLogin(userName)) {
		logWarningNL("User information for account '%s' (login %s) could not be acquired.",
		    settingsCopy.accountName().toUtf8().constData(),
		    userName.toUtf8().constData());
	}
	if (!IsdsHelper::getPasswordInfoFromLogin(userName)) {
		logWarningNL("Password information for account '%s' (login %s) could not be acquired.",
		    settingsCopy.accountName().toUtf8().constData(),
		    userName.toUtf8().constData());
	}

	/* Check password expiration. */
	if (!settingsCopy._pwdExpirDlgShown()) {
		/* Notify only once. */
		settingsCopy._setPwdExpirDlgShown(true);

		const QString acntDbKey(AccountDb::keyFromLogin(userName));

		int daysTo = GlobInstcs::accntDbPtr->pwdExpiresInDays(acntDbKey,
		    PWD_EXPIRATION_NOTIFICATION_DAYS);

		if (daysTo >= 0) {
			logWarningNL(
			    "Password for user '%s' of account '%s' expires in %d days.",
			    userName.toUtf8().constData(),
			    settingsCopy.accountName().toUtf8().constData(),
			    daysTo);

			/* Password change dialogue. */
			const QDateTime dbDateTime(
			    GlobInstcs::accntDbPtr->getPwdExpirFromDb(acntDbKey));
			if (QMessageBox::Yes == showDialogueAboutPwdExpir(
			        settingsCopy.accountName(), userName, daysTo,
			        dbDateTime)) {
				showStatusTextWithTimeout(tr(
				    "Change password of account \"%1\".")
				    .arg(settingsCopy.accountName()));
				const QString dbId(
				    GlobInstcs::accntDbPtr->dbId(acntDbKey));
				DlgChangePwd::changePassword(dbId, userName,
				    this);
			}
		}
	}

	/* Set longer time-out. */
	GlobInstcs::isdsSessionsPtr->setSessionTimeout(userName,
	    GlobInstcs::prefsPtr->isdsDownloadTimeoutMs);

	return true;
}

/* ========================================================================= */
/*
 * First connect to databox from new account
 */
bool MainWindow::firstConnectToIsds(AcntSettings &accountInfo)
/* ========================================================================= */
{
	debugFuncCall();

	if (!logInGUI(*GlobInstcs::isdsSessionsPtr, accountInfo)) {
		return false;
	}

	const QString userName(accountInfo.userName());
	Q_ASSERT(!userName.isEmpty());

	if (!IsdsHelper::getOwnerInfoFromLogin(userName)) {
		//TODO: return false;
	}
	if (!IsdsHelper::getUserInfoFromLogin(userName)) {
		//TODO: return false;
	}
	if (!IsdsHelper::getPasswordInfoFromLogin(userName)) {
		//TODO: return false;
	}

	return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	debugFuncCall();

	/*
	 * Check whether currently some tasks are being processed or are
	 * pending. If nothing works finish immediately, else show question.
	 */
	if (GlobInstcs::workPoolPtr->working()) {
		int dlgRet = DlgMsgBox::message(this, QMessageBox::Question,
		    tr("Datovka"),
		    tr("Datovka is currently processing some tasks."),
		    tr("Do you want to abort pending actions and close Datovka?"),
		    QString(), QMessageBox::No | QMessageBox::Yes,
		    QMessageBox::No);

		if (QMessageBox::Yes == dlgRet) {
			GlobInstcs::workPoolPtr->stop();
			GlobInstcs::workPoolPtr->clear();
		} else {
			event->ignore();
		}
	}
}

void MainWindow::moveEvent(QMoveEvent *event)
{
	/* Event precedes actual maximisation. */
	if (!isMaximized()) {
		QPoint oldPos(event->oldPos());
		if (oldPos != event->pos()) {
			m_geometry.setTopLeft(oldPos);
		}
	}
	QMainWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	/* Event precedes actual maximisation. */
	if (!isMaximized()) {
		QSize oldSize(event->oldSize());
		if (oldSize != event->size()) {
			m_geometry.setSize(oldSize);
		}
	}
	QMainWindow::resizeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
	debugFuncCall();

	QMainWindow::showEvent(event);

	QRect availRect(Dimensions::availableScreenSize());
	QRect frameRect(frameGeometry());

	if (isMaximized() ||
	    ((frameRect.width() <= availRect.width()) &&
	    (frameRect.height() <= availRect.height()))) {
		/* Window fits to screen. */
		return;
	}

	this->setGeometry(Dimensions::windowDimensions(this, -1.0, -1.0));
}

/* ========================================================================= */
/*
 * Verify if is a connection to ISDS and databox exists for a new account
 */
void MainWindow::getAccountUserDataboxInfo(AcntSettings accountInfo)
/* ========================================================================= */
{
	debugSlotCall();

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(
	        accountInfo.userName())) {
		if (!firstConnectToIsds(accountInfo)) {
			QString msgBoxTitle = tr("New account error") +
			    ": " + accountInfo.accountName();
			QString msgBoxContent =
			    tr("It was not possible to get user info and "
			    "databox info from ISDS server for account")
			    + " \"" + accountInfo.accountName() + "\"."
			    + "<br><br><b>" +
			    tr("Connection to ISDS or user authentication failed!")
			    + "</b><br><br>" +
			    tr("Please check your internet connection and "
			    "try again or it is possible that your password "
			    "(certificate) has expired - in this case, you "
			    "need to use the official web interface of Datové "
			    "schránky to change it.")
			    + "<br><br><b>" +
			    tr("Account") + "<i>" + " \"" +
			    accountInfo.accountName() + "\" " + "("
			    + accountInfo.userName() + ") </i> " +
			    tr("was not created!") + "</b>";

			QMessageBox::critical(this,
			    msgBoxTitle,
			    msgBoxContent,
			    QMessageBox::Ok);

			return;
		}
	}

	/* Store account information. */
	QModelIndex index;
	int ret = m_accountModel.addAccount(accountInfo, &index);
	if (ret == 0) {
		refreshAccountList(accountInfo.userName());

		/* get current account model */
		if (index.isValid()) {
			logDebugLv0NL("Changing selection to index %d.", index.row());
			ui->accountList->selectionModel()->setCurrentIndex(index,
			    QItemSelectionModel::ClearAndSelect);
			/* Expand the tree. */
			ui->accountList->expand(index);
		}
	} else if (ret == -1) {
		QMessageBox::warning(this, tr("Adding new account failed"),
		    tr("Account could not be added because an error occurred."),
		    QMessageBox::Ok);
	} else if (ret == -2) {
		QMessageBox::warning(this, tr("Adding new account failed"),
		    tr("Account could not be added because account already exists."),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Set message process state into db
 */
void MainWindow::msgSetSelectedMessageProcessState(int stateIndex)
/* ========================================================================= */
{
	debugSlotCall();

	enum MessageProcessState procSt;
	switch (stateIndex) {
	case UNSETTLED:
		procSt = UNSETTLED;
		break;
	case IN_PROGRESS:
		procSt = IN_PROGRESS;
		break;
	case SETTLED:
		procSt = SETTLED;
		break;
	default:
		Q_ASSERT(0);
		return;
		break;
	}

	messageItemsSetProcessStatus(currentFrstColMessageIndexes(), procSt);
}


/* ========================================================================= */
/*
 * Set read status to messages with given indexes.
 */
void MainWindow::messageItemsSetReadStatus(
    const QModelIndexList &firstMsgColumnIdxs, bool read)
/* ========================================================================= */
{
	debugFuncCall();

	/* Works only for received messages. */
	if (!AccountModel::nodeTypeIsReceived(currentAccountModelIndex())) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();

	for (QModelIndexList::const_iterator it = firstMsgColumnIdxs.begin();
	     it != firstMsgColumnIdxs.end(); ++it) {
		const MessageDb::MsgId msgId(msgMsgId(*it));
		Q_ASSERT(msgId.deliveryTime.isValid());

		MessageDb *messageDb = dbSet->accessMessageDb(
		    msgId.deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			continue;
		}

		messageDb->setMessageLocallyRead(msgId.dmId, read);

		/*
		 * Mark message as read without reloading
		 * the whole model.
		 */
		m_messageTableModel.overrideRead(msgId.dmId, read);
	}

	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(currentAccountModelIndex());
}


/* ========================================================================= */
/*
 * Set process status to messages with given indexes.
 */
void MainWindow::messageItemsSetProcessStatus(
    const QModelIndexList &firstMsgColumnIdxs, enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	/* Works only for received messages. */
	if (!AccountModel::nodeTypeIsReceived(currentAccountModelIndex())) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	   m_accountModel.userName(currentAccountModelIndex()));
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();

	for (QModelIndexList::const_iterator it = firstMsgColumnIdxs.begin();
	     it != firstMsgColumnIdxs.end(); ++it) {
		const MessageDb::MsgId msgId(msgMsgId(*it));
		Q_ASSERT(msgId.deliveryTime.isValid());

		MessageDb *messageDb = dbSet->accessMessageDb(
		    msgId.deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			continue;
		}

		messageDb->setMessageProcessState(msgId.dmId, state);

		/*
		 * Mark message as read without reloading
		 * the whole model.
		 */
		m_messageTableModel.overrideProcessing(msgId.dmId, state);
	}

	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(currentAccountModelIndex());
}

void MainWindow::viewLogDlg(void)
{
	debugSlotCall();

	if (m_viewLogDlg != Q_NULLPTR) {
		if (!m_viewLogDlg->isMinimized()) {
			m_viewLogDlg->show();
		} else {
			m_viewLogDlg->showNormal();
		}
		m_viewLogDlg->raise();
		m_viewLogDlg->activateWindow();
		return;
	}

	m_viewLogDlg = new (std::nothrow) DlgViewLog(this, Qt::Window);
	if (Q_UNLIKELY(m_viewLogDlg == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	connect(m_viewLogDlg, SIGNAL(finished(int)),
	    this, SLOT(viewLogDlgClosed(int)));
	m_viewLogDlg->show();
}

void MainWindow::viewLogDlgClosed(int result)
{
	Q_UNUSED(result);
	debugSlotCall();

	m_viewLogDlg->setAttribute(Qt::WA_DeleteOnClose, true);
	m_viewLogDlg->deleteLater();
	m_viewLogDlg = Q_NULLPTR;
}

void MainWindow::showMsgAdvancedSearchDialog(void)
{
	debugSlotCall();

	static QDialog *dlgMsgSearch = 0;

	if (m_searchDlgActive) {
		if (!dlgMsgSearch->isMinimized()) {
			dlgMsgSearch->show();
		} else {
			dlgMsgSearch->showNormal();
		}
		dlgMsgSearch->raise();
		dlgMsgSearch->activateWindow();
		return;
	} else {
		dlgMsgSearch = 0;
	}

	if (ui->accountList->model()->rowCount() == 0) {
		return;
	}

	/* Get pointers to database sets of all accounts. */
	const QList<Task::AccountDescr> messageDbList(
	    messageDbListForAllAccounts(this, m_accountModel, ui->accountList,
	        false));

	/* get current account username */
	const QString currentUserName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!currentUserName.isEmpty());

	dlgMsgSearch = new DlgMsgSearch(messageDbList, currentUserName, this,
	    Qt::Window);
	connect(dlgMsgSearch, SIGNAL(focusSelectedMsg(QString, qint64, QString, int)),
	    this, SLOT(messageItemFromSearchSelection(QString, qint64, QString, int)));
	connect(dlgMsgSearch, SIGNAL(finished(int)),
	    this, SLOT(msgAdvancedDlgFinished(int)));
	dlgMsgSearch->setAttribute(Qt::WA_DeleteOnClose, true);
	dlgMsgSearch->show();
	m_searchDlgActive = true;
}

void MainWindow::msgAdvancedDlgFinished(int result)
{
	Q_UNUSED(result);
	m_searchDlgActive = false;
}

/* ========================================================================= */
/*
 * Show dialogue that notifies the user about expiring password.
 */
int MainWindow::showDialogueAboutPwdExpir(const QString &accountName,
    const QString &userName, qint64 days, const QDateTime &dateTime)
/* ========================================================================= */
{
	debugFuncCall();

	int dlgRet = QMessageBox::No;

	if (days < 0) {
		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Password expiration"),
		    tr("According to the last available information, "
		        "your password for account '%1' (login '%2') expired %3 days ago (%4).")
		        .arg(accountName).arg(userName).arg(days*(-1))
		        .arg(dateTime.toString("dd.MM.yyyy hh:mm:ss")),
		    tr("You have to change your password from the ISDS web interface. "
		        "Your new password will be valid for 90 days."),
		    QString(), QMessageBox::Ok, QMessageBox::Ok);
	} else {
		dlgRet = DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Password expiration"),
		    tr("According to the last available information, "
		        "your password for account '%1' (login '%2') will expire in %3 days (%4).")
		        .arg(accountName).arg(userName).arg(days)
		        .arg(dateTime.toString("dd.MM.yyyy hh:mm:ss")),
		    tr("You can change your password now or later using the '%1' command. "
		        "Your new password will be valid for 90 days.\n\n"
		        "Change password now?").arg(ui->actionChange_password->text()),
		    QString(), QMessageBox::No | QMessageBox::Yes,
		    QMessageBox::No);
	}

	return dlgRet;
}


/* ========================================================================= */
/*
 * Show message time stamp expiration dialogue.
 */
void MainWindow::showMsgTmstmpExpirDialog(void)
/* ========================================================================= */
{
	debugSlotCall();

	enum DlgTimestampExpir::Action action =
	    DlgTimestampExpir::askAction(this);
	if (action != DlgTimestampExpir::CHECK_NOTHING) {
		prepareMsgTmstmpExpir(action);
	}
}


/* ========================================================================= */
/*
 * Prepare message timestamp expiration based on action.
 */
void MainWindow::prepareMsgTmstmpExpir(enum DlgTimestampExpir::Action action)
/* ========================================================================= */
{
	debugFuncCall();

	QString userName;
	bool includeSubdir = false;
	QString importDir;
	QStringList fileList, filePathList;
	QStringList nameFilter("*.zfo");
	QDir directory(QDir::home());
	fileList.clear();
	filePathList.clear();

	switch (action) {
	case DlgTimestampExpir::CHECK_SELECTED_ACNT:
		/* Process the selected account. */
		userName = m_accountModel.userName(currentAccountModelIndex());
		Q_ASSERT(!userName.isEmpty());
		showStatusTextPermanently(tr("Checking time stamps in account '%1'...")
		    .arg((*GlobInstcs::acntMapPtr)[userName].accountName()));
		checkMsgsTmstmpExpiration(userName, QStringList());
		break;

	case DlgTimestampExpir::CHECK_ALL_ACNTS:
		for (int i = 0; i < ui->accountList->model()->rowCount(); ++i) {
			QModelIndex index = m_accountModel.index(i, 0);
			userName = m_accountModel.userName(index);
			Q_ASSERT(!userName.isEmpty());
			showStatusTextPermanently(
			    tr("Checking time stamps in account '%1'...").arg(
			        (*GlobInstcs::acntMapPtr)[userName].accountName()));
			checkMsgsTmstmpExpiration(userName, QStringList());
		}
		break;

	case DlgTimestampExpir::CHECK_DIR_SUB:
		includeSubdir = true;
	case DlgTimestampExpir::CHECK_DIR:
		importDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory"), m_import_zfo_path,
		    QFileDialog::ShowDirsOnly |
		    QFileDialog::DontResolveSymlinks);

		if (importDir.isEmpty()) {
			return;
		}

		m_import_zfo_path = importDir;

		if (includeSubdir) {
			QDirIterator it(importDir, nameFilter, QDir::Files,
			    QDirIterator::Subdirectories);
			while (it.hasNext()) {
				filePathList.append(it.next());
			}
		} else {
			directory.setPath(importDir);
			fileList = directory.entryList(nameFilter);
			for (int i = 0; i < fileList.size(); ++i) {
				filePathList.append(
				    importDir + "/" + fileList.at(i));
			}
		}

		if (filePathList.isEmpty()) {
			qDebug() << "ZFO-IMPORT:" << "No *.zfo file(s) in the "
			    "selected directory";
			showStatusTextWithTimeout(tr("ZFO file(s) not found in "
			    "selected directory."));
			QMessageBox::warning(this,
			    tr("No ZFO file(s)"),
			    tr("ZFO file(s) not found in selected directory."),
			    QMessageBox::Ok);
			return;
		}

		checkMsgsTmstmpExpiration(QString(), filePathList);

		break;
	default:
		break;
	}

	clearStatusBar();
}


/* ========================================================================= */
/*
 * Check messages time stamp expiration for account.
 */
void MainWindow::checkMsgsTmstmpExpiration(const QString &userName,
    const QStringList &filePathList)
/* ========================================================================= */
{
	debugFuncCall();

	QList<MessageDb::MsgId> expirMsgIds;
	QStringList expirMsgFileNames;
	QList<MessageDb::MsgId> errorMsgIds;
	QStringList errorMsgFileNames;

	QByteArray tstData;
	int msgCnt = 0;
	QString dlgText;
	bool showExportOption = true;

	if (userName.isEmpty()) {

		msgCnt = filePathList.count();

		for (int i = 0; i < msgCnt; ++i) {

			Isds::Message message(
			    Isds::messageFromFile(filePathList.at(i),
			        Isds::LT_MESSAGE));
			if (message.isNull() || message.envelope().isNull()) {
				errorMsgFileNames.append(filePathList.at(i));
				continue;
			}

			const QByteArray &tstData(message.envelope().dmQTimestamp());
			if (tstData.isEmpty()) {
				errorMsgFileNames.append(filePathList.at(i));
				continue;
			}

			if (DlgSignatureDetail::signingCertExpiresBefore(tstData,
			        GlobInstcs::prefsPtr->timestampExpirBeforeDays)) {
				expirMsgFileNames.append(filePathList.at(i));
			}
		}

		dlgText = tr("Time stamp expiration check of ZFO files finished with result:")
		    + "<br/><br/>" +
		    tr("Total of ZFO files: %1").arg(msgCnt)
		    + "<br/><b>" +
		    tr("ZFO files with time stamp expiring within %1 days: %2")
			.arg(GlobInstcs::prefsPtr->timestampExpirBeforeDays)
			.arg(expirMsgFileNames.count())
		    + "</b><br/>" +
		    tr("Unchecked ZFO files: %1").arg(errorMsgFileNames.count());

		showExportOption = false;

	} else {
		MessageDbSet *dbSet = accountDbSet(userName);
		if (Q_NULLPTR == dbSet) {
			Q_ASSERT(0);
			return;
		}

		QList<MessageDb::MsgId> msgIdList(dbSet->getAllMessageIDsFromDB());
		msgCnt = msgIdList.count();

		foreach (const MessageDb::MsgId &mId, msgIdList) {
			MessageDb *messageDb = dbSet->accessMessageDb(mId.deliveryTime, false);
			if (0 == messageDb) {
				Q_ASSERT(0);
				continue;
			}

			tstData = messageDb->getMessageTimestampRaw(mId.dmId);
			if (tstData.isEmpty()) {
				errorMsgIds.append(mId);
				continue;
			}
			if (DlgSignatureDetail::signingCertExpiresBefore(tstData,
			        GlobInstcs::prefsPtr->timestampExpirBeforeDays)) {
				expirMsgIds.append(mId);
			}
		}

		dlgText = tr("Time stamp expiration check "
		    "in account '%1' finished with result:").arg(
		        (*GlobInstcs::acntMapPtr)[userName].accountName())
		    + "<br/><br/>" +
		    tr("Total of messages in database: %1").arg(msgCnt)
		    + "<br/><b>" +
		    tr("Messages with time stamp expiring within %1 days: %2")
			.arg(GlobInstcs::prefsPtr->timestampExpirBeforeDays)
			.arg(expirMsgIds.count())
		    + "</b><br/>" +
		    tr("Unchecked messages: %1").arg(errorMsgIds.count());
	}

	QString infoText;
	QString detailText;
	QMessageBox::StandardButtons buttons = QMessageBox::Ok;
	enum QMessageBox::StandardButton dfltDutton = QMessageBox::Ok;

	if (!expirMsgIds.isEmpty() || !expirMsgFileNames.isEmpty() ||
	    !errorMsgIds.isEmpty() || !errorMsgFileNames.isEmpty()) {
		infoText = tr("See details for more info...") + "<br/><br/>";
		if (!expirMsgIds.isEmpty() && showExportOption) {
			infoText += "<b>" +
			    tr("Do you want to export the expiring "
			    "messages to ZFO?") + "</b><br/><br/>";
		}

		if (!expirMsgIds.isEmpty() || !errorMsgIds.isEmpty()) {
			for (int i = 0; i < expirMsgIds.count(); ++i) {
				detailText += tr("Time stamp of message %1 expires "
				    "within specified interval.").arg(expirMsgIds.at(i).dmId);
				if (((expirMsgIds.count() - 1) != i) ||
				    errorMsgIds.count()) {
					detailText += "\n";
				}
			}
			for (int i = 0; i < errorMsgIds.count(); ++i) {
				detailText += tr("Time stamp of message %1 "
				    "is not present.").arg(errorMsgIds.at(i).dmId);
				if ((expirMsgIds.count() - 1) != i) {
					detailText += "\n";
				}
			}
		} else {
			for (int i = 0; i < expirMsgFileNames.count(); ++i) {
				detailText += tr("Time stamp of message %1 expires "
				    "within specified interval.").arg(expirMsgFileNames.at(i));
				if (((expirMsgFileNames.count() - 1) != i) ||
				    errorMsgFileNames.count()) {
					detailText += "\n";
				}
			}
			for (int i = 0; i < errorMsgFileNames.count(); ++i) {
				detailText += tr("Time stamp of message %1 "
				    "is not present.").arg(errorMsgFileNames.at(i));
				if ((expirMsgFileNames.count() - 1) != i) {
					detailText += "\n";
				}
			}
		}

		if (!expirMsgIds.isEmpty() && showExportOption) {
			buttons = QMessageBox::No | QMessageBox::Yes;
			dfltDutton = QMessageBox::No;
		}
	}

	int dlgRet = DlgMsgBox::message(this, QMessageBox::Information,
	    tr("Time stamp expiration check results"), dlgText, infoText,
	    detailText, buttons, dfltDutton);

	if (QMessageBox::Yes == dlgRet) {
		if (!userName.isEmpty()) {
			exportExpirMessagesToZFO(userName, expirMsgIds);
		}
	}
}

void MainWindow::exportExpirMessagesToZFO(const QString &userName,
    const QList<MessageDb::MsgId> &expirMsgIds)
{
	setAccountStoragePaths(userName);

	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Select target folder for export"), m_on_export_zfo_activate,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (newDir.isEmpty()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	QString lastPath = newDir;
	QString errStr;
	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));
	Exports::ExportError ret;

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	foreach (MessageDb::MsgId mId, expirMsgIds) {
		ret = Exports::exportAs(this, *dbSet, Exports::ZFO_MESSAGE,
		    newDir, QString(), userName, accountName, dbId, mId, false,
		    lastPath, errStr);
		if (Exports::EXP_CANCELED == ret) {
			break;
		} else if(Exports::EXP_NOT_MSG_DATA == ret) {
			if (messageMissingOfferDownload(mId, errStr)) {
			    Exports::exportAs(this, *dbSet, Exports::ZFO_MESSAGE,
			    newDir, QString(), userName, accountName, dbId, mId,
			    false, lastPath, errStr);
			}
		}
		if (!lastPath.isEmpty()) {
			m_on_export_zfo_activate = lastPath;
			storeExportPath(userName);
		}
	}
}

void MainWindow::prepareMsgsImportFromDatabase(void)
{
	debugSlotCall();

	const QString userName =
	    m_accountModel.userName(currentAccountModelIndex());

	{
		int dlgRet = DlgMsgBox::message(this, QMessageBox::Question,
		    tr("Import messages from database"),
		    tr("This action allow to import messages from selected database files into current account. "
		        "Keep in mind that this action may take a while based on number of messages in the imported database. "
		        "Import progress will be displayed in the status bar."),
		    tr("Do you want to continue?"), QString(),
		    QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
		if (QMessageBox::No == dlgRet) {
			return;
		}
	}

	/* get list of selected database files */
	QStringList dbFileList = QFileDialog::getOpenFileNames(this,
	    tr("Select database file(s)"),
	    m_on_import_database_dir_activate, tr("DB file (*.db)"));

	if (dbFileList.isEmpty()) {
		qDebug() << "No *.db selected file(s)";
		showStatusTextWithTimeout(tr("Database file(s) not selected."));
		return;
	}

	/* remember import path */
	m_on_import_database_dir_activate =
	    QFileInfo(dbFileList.at(0)).absoluteDir().absolutePath();

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));

	Imports::importDbMsgsIntoDatabase(*dbSet, dbFileList, userName, dbId);

	/* update account model */
	refreshAccountList(userName);
}

void MainWindow::splitMsgDbByYearsSlot(void)
{
	debugSlotCall();

	{
		int dlgRet = DlgMsgBox::message(this, QMessageBox::Question,
		    tr("Database split"),
		    tr("This action splits current account message database into several separate databases which will contain messages relevant to one year only. "
		        "It is recommended for large databases in order to improve the performance."),
		    tr("The original database file will be copied to selected directory and new database files will be created in the original location. "
		        "If action finishes with success then new databases will be used instead of the original. "
		        "Application restart is required.")
		    + "\n\n" +
		    tr("Note: Keep in mind that this action may take a while based on the number of messages in the database.")
		    + "\n\n" + tr("Do you want to continue?"), QString(),
		    QMessageBox::No | QMessageBox::Yes, QMessageBox::No);

		if (QMessageBox::No == dlgRet) {
			return;
		}
	}

	QString newDbDir;
	QString userName = m_accountModel.userName(currentAccountModelIndex());
	/* get current db file location */
	AcntSettings &itemSettings((*GlobInstcs::acntMapPtr)[userName]);
	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		dbDir = GlobInstcs::prefsPtr->confDir();
	}

	/* get origin message db set based on username */
	MessageDbSet *msgDbSet = accountDbSet(userName);
	if (Q_NULLPTR == msgDbSet) {
		return;
	}

	/* get directory for saving of split database files */
	do {
		newDbDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory for new databases"),
		    m_on_import_database_dir_activate,
		    QFileDialog::ShowDirsOnly |
		    QFileDialog::DontResolveSymlinks);

		if (newDbDir.isEmpty()) {
			return;
		}

		/* new db files cannot save into same location as original db */
		if (dbDir == newDbDir) {
			clearProgressBar();
			showStatusTextWithTimeout(tr("Split of message database "
			    "finished with error"));

			DlgMsgBox::message(this, QMessageBox::Critical,
			    tr("Database file error"),
			    tr("Database file cannot be split into original directory."),
			    tr("Please choose another directory."), QString(),
			    QMessageBox::Ok);

			clearStatusBar();
		}
	} while (dbDir == newDbDir);

	/* remember import path */
	m_on_import_database_dir_activate = newDbDir;

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	TaskSplitDb *task = new (::std::nothrow) TaskSplitDb(msgDbSet,
	    userName, dbDir, newDbDir, itemSettings.isTestAccount());
	task->setAutoDelete(false);
	/* This will block the GUI and all workers. */
	GlobInstcs::workPoolPtr->runSingle(task);

	QApplication::restoreOverrideCursor();

	/* Show final notification. */
	if (task->m_success) {
		showStatusTextWithTimeout(tr("Split of message database finished"));

		DlgMsgBox::message(this, QMessageBox::Information,
		    tr("Database split result"),
		    tr("Congratulation: message database for account '%1' was split successfully. "
		        "Please, restart the application for loading of new databases.")
		        .arg(userName),
		    tr("Note: Original database file was backed up to:") + "\n" + newDbDir,
		    QString(), QMessageBox::Ok);
	} else {
		DlgMsgBox::message(this, QMessageBox::Critical,
		    tr("Database split result"),
		    tr("Splitting of message database for account '%1' was not successful. "
		        "Please, restart the application in order to reload the  original database.")
		        .arg(userName),
		    task->m_error, QString(), QMessageBox::Ok);
	}

	delete task;
	clearProgressBar();

	clearStatusBar();

	/* refresh account model and account list */
	refreshAccountList(userName);
}

void MainWindow::setUpUi(void)
{
	ui->setupUi(this);
	setUpTabOrder();

	setWindowIcon(IconContainer::construcIcon(IconContainer::ICON_DATOVKA));

	/* Set default line height for table views/widgets. */
	ui->accountList->setNarrowedLineHeight();
	ui->messageList->setNarrowedLineHeight();
	ui->messageAttachmentList->setNarrowedLineHeight();

	/* Header content alignment. */
	ui->accountList->header()->setDefaultAlignment(Qt::AlignLeft);
	ui->messageList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	ui->messageAttachmentList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

	/* Window title. */
	setWindowTitle(
	    tr("Datovka - Free client for Datov\303\251 schr\303\241nky"));
#ifdef PORTABLE_APPLICATION
	setWindowTitle(windowTitle() + " - " + tr("Portable version"));
#endif /* PORTABLE_APPLICATION */

	setMenuActionIcons();

	topToolBarSetUp();

	/* Create info status bar */
	mui_statusBar = new QStatusBar(this);
	mui_statusBar->setSizeGripEnabled(false);
	ui->statusBar->addWidget(mui_statusBar, 1);
	showStatusTextWithTimeout(tr("Welcome..."));

	/* Create status bar label shows database mode memory/disk */
	mui_statusDbMode = new QLabel(this);
	mui_statusDbMode->setText(tr("Storage: disk | disk"));
	mui_statusDbMode->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	ui->statusBar->addWidget(mui_statusDbMode, 0);

	/* Create status bar online/offline label */
	mui_statusOnlineLabel = new QLabel(this);
	mui_statusOnlineLabel->setText(tr("Mode: offline"));
	mui_statusOnlineLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	ui->statusBar->addWidget(mui_statusOnlineLabel, 0);

	/* Create progress bar object and set default value */
	mui_statusProgressBar = new QProgressBar(this);
	mui_statusProgressBar->setAlignment(Qt::AlignRight);
	mui_statusProgressBar->setMinimumWidth(100);
	mui_statusProgressBar->setMaximumWidth(200);
	mui_statusProgressBar->setTextVisible(true);
	mui_statusProgressBar->setRange(0, 100);
	mui_statusProgressBar->setValue(0);
	clearProgressBar();
	ui->statusBar->addWidget(mui_statusProgressBar, 1);

	/* Message state combo box. */
	ui->messageStateCombo->setInsertPolicy(QComboBox::InsertAtBottom);
	ui->messageStateCombo->addItem(
	    IconContainer::construcIcon(IconContainer::ICON_RED_BALL),
	    tr("Unsettled"));
	ui->messageStateCombo->addItem(
	    IconContainer::construcIcon(IconContainer::ICON_YELLOW_BALL),
	    tr("In Progress"));
	ui->messageStateCombo->addItem(
	    IconContainer::construcIcon(IconContainer::ICON_GREY_BALL),
	    tr("Settled"));

	/* Override tool button sizes. */
	ui->signatureDetails->setVerticalSizeHintOrigin(ui->messageStateCombo);
	ui->downloadComplete->setVerticalSizeHintOrigin(ui->messageStateCombo);
	ui->saveAttachments->setVerticalSizeHintOrigin(ui->messageStateCombo);
	ui->saveAttachment->setVerticalSizeHintOrigin(ui->messageStateCombo);
	ui->openAttachment->setVerticalSizeHintOrigin(ui->messageStateCombo);

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(
	    QCoreApplication::applicationVersion()));
	ui->accountTextInfo->setReadOnly(true);
}

void MainWindow::setUpTabOrder(void)
{
	ui->toolBar->setFocusPolicy(Qt::StrongFocus);

	/* Tool bar elements need to have focus enabled. */
	QWidget::setTabOrder(ui->toolBar, ui->accountList);
	QWidget::setTabOrder(ui->accountList, ui->accountTextInfo);
	QWidget::setTabOrder(ui->accountTextInfo, ui->messageList);
	QWidget::setTabOrder(ui->messageList, ui->signatureDetails);
	QWidget::setTabOrder(ui->signatureDetails, ui->messageStateCombo);
	QWidget::setTabOrder(ui->messageStateCombo, ui->downloadComplete);
	QWidget::setTabOrder(ui->downloadComplete, ui->saveAttachments);
	QWidget::setTabOrder(ui->saveAttachments, ui->saveAttachment);
	QWidget::setTabOrder(ui->saveAttachment, ui->openAttachment);
	QWidget::setTabOrder(ui->openAttachment, ui->messageInfo);
	QWidget::setTabOrder(ui->messageInfo, ui->messageAttachmentList);
}

/*!
 * @brief Add action to a tool bar.
 *
 * @param[in,out] toolBar Tool bar.
 * @param[in]     action Action to be added.
 * @param[in]     focusPolicy Focus policy.
 * @return True if action has been added and policy set.
 */
static
bool addToolBarAction(QToolBar *toolBar, QAction *action,
    enum Qt::FocusPolicy focusPolicy = Qt::StrongFocus)
{
	if (Q_UNLIKELY((toolBar == Q_NULLPTR) || (action == Q_NULLPTR))) {
		Q_ASSERT(0);
		return false;
	}

	/* TODO -- Abort if action already held. */

	toolBar->addAction(action);

	if (focusPolicy == Qt::NoFocus) {
		return true;
	}

	QWidget *w = toolBar->widgetForAction(action);
	if (Q_UNLIKELY(w == Q_NULLPTR)) {
		return false;
	}
	w->setFocusPolicy(focusPolicy);
	return true;
}

void MainWindow::topToolBarSetUp(void)
{
	/* Add actions to the top tool bar. */
	addToolBarAction(ui->toolBar, ui->actionSync_all_accounts);
	addToolBarAction(ui->toolBar, ui->actionGet_messages);
	ui->toolBar->addSeparator();
	addToolBarAction(ui->toolBar, ui->actionSend_message);
	addToolBarAction(ui->toolBar, ui->actionReply);
	addToolBarAction(ui->toolBar, ui->actionAuthenticate_message);
	ui->toolBar->addSeparator();
	addToolBarAction(ui->toolBar, ui->actionMsgAdvancedSearch);
	ui->toolBar->addSeparator();
	addToolBarAction(ui->toolBar, ui->actionAccount_properties);
	addToolBarAction(ui->toolBar, ui->actionPreferences);

	{
		QWidget *spacer = new QWidget();
		spacer->setSizePolicy(QSizePolicy::Expanding,
		    QSizePolicy::Expanding);
		ui->toolBar->addWidget(spacer);
	}

	{
		QLabel *searchLabel = new QLabel;
		searchLabel->setText(tr("Search: "));
		ui->toolBar->addWidget(searchLabel);
	}

	/* Message filter field. */
	mui_filterLine = new QLineEdit(this);
	connect(mui_filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterMessages(QString)));
	mui_filterLine->setFixedWidth(200);
	mui_filterLine->setToolTip(tr("Enter sought expression"));
	mui_filterLine->setClearButtonEnabled(true);
	ui->toolBar->addWidget(mui_filterLine);
}

void MainWindow::setMenuActionIcons(void)
{
	/*
	 * Don't remove the isEnabled() calls as they server as placeholders
	 * for existing actions.
	 *
	 * You may replace them with a setIcon() if you wan to specify a icon.
	 */

	/* File menu. */
	ui->actionSync_all_accounts->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_ALL_ACCOUNTS_SYNC));
	    /* Separator. */
	ui->actionAdd_account->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_PLUS));
	ui->actionDelete_account->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DELETE));
	    /* Separator. */
	ui->actionImport_database_directory->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_ADDRESS));
	    /* Separator. */
	ui->actionProxy_settings->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_GLOBE));
	    /* Separator. */
	{
		const QIcon ico(
		    IconContainer::construcIcon(IconContainer::ICON_BRIEFCASE));
		ui->actionRecords_management_settings->setIcon(ico);
		ui->actionUpdate_records_management_information->setIcon(ico);
	}
	    /* Separator. */
	ui->actionPreferences->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_GEAR));
	/* actionQuit -- connected in ui file. */
	ui->actionQuit->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DELETE));

	/* Data box menu. */
	ui->actionGet_messages->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_ACCOUNT_SYNC));
	ui->actionSend_message->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE));
	    /* Separator. */
	ui->actionSend_egov_request->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE_UPLOAD));
	    /* Separator. */
	ui->actionMark_all_as_read->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_OK));
	    /* Separator. */
	ui->actionChange_password->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_USER));
	    /* Separator. */
	ui->actionAccount_properties->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_LETTER));
	    /* Separator. */
	ui->actionMove_account_up->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_UP));
	ui->actionMove_account_down->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DOWN));
	    /* Separator. */
	ui->actionChange_data_directory->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_FOLDER));
	    /* Separator. */
	{
		const QIcon ico(
		    IconContainer::construcIcon(IconContainer::ICON_CLIPBOARD));
		ui->actionImport_messages_from_database->setIcon(ico);
		ui->actionImport_ZFO_file_into_database->setIcon(ico);
	}
	    /* Separator. */
	ui->actionVacuum_message_database->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_TRASH));
	ui->actionSplit_database_by_years->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_STATISTICS));

	/* Message menu. */
	ui->actionDownload_message_signed->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE_DOWNLOAD));
	ui->actionReply->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE_REPLY));
	ui->actionForward_message->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE));
	ui->actionCreate_message_from_template->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE));
	    /* Separator. */
	ui->actionSignature_detail->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE_SIGNATURE));
	ui->actionAuthenticate_message->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE_VERIFY));
	    /* Separator. */
	ui->actionOpen_message_externally->isEnabled();
	ui->actionOpen_delivery_info_externally->isEnabled();
	    /* Separator. */
	ui->actionSend_to_records_management->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_BRIEFCASE));
	    /* Separator. */
	ui->actionExport_as_ZFO->isEnabled();
	ui->actionExport_delivery_info_as_ZFO->isEnabled();
	ui->actionExport_delivery_info_as_PDF->isEnabled();
	ui->actionExport_message_envelope_as_PDF->isEnabled();
	ui->actionExport_envelope_PDF_and_attachments->isEnabled();
	    /* Separator. */
	ui->actionEmail_ZFOs->isEnabled();
	ui->actionEmail_all_attachments->isEnabled();
	    /* Separator. */
	ui->actionSave_all_attachments->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_SAVE_ALL));
	ui->actionSave_selected_attachments->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_SAVE));
	ui->actionOpen_attachment->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_FOLDER));
	    /* Separator. */
	ui->actionDelete_message_from_db->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DELETE));

	/* Tools menu. */
	ui->actionFind_databox->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_SEARCH));
	    /* Separator. */
	ui->actionAuthenticate_message_file->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_DATOVKA_MESSAGE_VERIFY));
	ui->actionView_message_file->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_MONITOR));
	ui->actionExport_correspondence_overview->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_SAVE));
	ui->actionCheck_message_timestamp_expiration->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_SHIELD));
	    /* Separator. */
	ui->actionMsgAdvancedSearch->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_SEARCH));
	    /* Separator. */
	ui->actionTag_settings->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_LABEL));

	/* Help. */
	ui->actionAbout_Datovka->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_INFO));
	ui->actionHomepage->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_HOME));
	ui->actionHelp->setIcon(
	    IconContainer::construcIcon(IconContainer::ICON_HELP));

	/* Actions that are not shown in the top menu. */
	ui->actionEmail_selected_attachments->isEnabled();
}

void MainWindow::showTagDialog(void)
{
	debugSlotCall();
	modifyTags(m_accountModel.userName(currentAccountModelIndex()),
	    QList<qint64>());
}

/* ========================================================================= */
/*
 * Slot: Add/delete tags to/from selected messages.
 */
void MainWindow::addOrDeleteMsgTags(void)
/* ========================================================================= */
{
	debugSlotCall();

	QList<qint64> msgIdList;

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));

	foreach (const QModelIndex &idx, currentFrstColMessageIndexes()) {
		msgIdList.append(idx.data().toLongLong());
	}

	modifyTags(userName, msgIdList);
}

void MainWindow::vacuumMsgDbSlot(void)
{
	debugSlotCall();

	if (!GlobInstcs::prefsPtr->storeMessagesOnDisk) {
		showStatusTextWithTimeout(tr("Vacuum cannot be performed on databases in memory."));

		DlgMsgBox::message(this, QMessageBox::Warning,
		    tr("Database operation error"),
		    tr("Database clean-up cannot be performed on database in memory."),
		    tr("Cannot call VACUUM on database in memory."), QString(),
		    QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	const QString userName =
	    m_accountModel.userName(currentAccountModelIndex());

	MessageDbSet *msgDbSet = accountDbSet(userName);
	if (Q_NULLPTR == msgDbSet) {
		return;
	}

	qint64 dbSizeInBytes = msgDbSet->underlyingFileSize(MessageDbSet::SC_LARGEST);
	if (dbSizeInBytes == 0) {
		return;
	}

	QString size = QString::number(dbSizeInBytes) + " B";
	if (dbSizeInBytes >= 1000000000) {
		size = QString::number(dbSizeInBytes / 1000000000) + " GB";
	} else if (dbSizeInBytes >= 1000000) {
		size = QString::number(dbSizeInBytes / 1000000) + " MB";
	} else if (dbSizeInBytes >= 1000) {
		size = QString::number(dbSizeInBytes / 1000) + " KB";
	}

	{
		int dlgRet = DlgMsgBox::message(this, QMessageBox::Question,
		    tr("Clean message database"),
		    tr("Performs a message database clean-up for the selected account. "
		        "This action will block the entire application. "
		        "The action may take several minutes to be completed. "
		        "Furthermore, it requires more than %1 of free disk space to successfully proceed.")
		        .arg(size),
		    tr("Do you want to continue?"), QString(),
		    QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
		if (QMessageBox::Yes != dlgRet) {
			return;
		}
	}

	showStatusTextPermanently(tr("Performing database clean-up."));
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	TaskVacuumDbSet *task = new (::std::nothrow) TaskVacuumDbSet(msgDbSet);
	task->setAutoDelete(false);
	/* This will block the GUI and all workers. */
	GlobInstcs::workPoolPtr->runSingle(task);

	showStatusTextWithTimeout(tr("Database clean-up finished."));
	QApplication::restoreOverrideCursor();

	if (task->m_success) {
		QMessageBox::information(this,
		    tr("Database clean-up successful"),
		    tr("The database clean-up has finished successfully."),
		    QMessageBox::Ok);
	} else {
		QMessageBox::warning(this,
		    tr("Database clean-up failure"),
		    tr("The database clean-up failed with error message: %1").arg(task->m_error),
		    QMessageBox::Ok);
	}

	delete task;
}

void MainWindow::modifyTags(const QString &userName, QList<qint64> msgIdList)
{
	if (GlobInstcs::tagDbPtr == Q_NULLPTR) {
		Q_ASSERT(0);
		return;
	}

	int dlgRet = DlgTags::NO_ACTION;

	if (msgIdList.isEmpty()) {
		dlgRet = DlgTags::editAvailable(userName, GlobInstcs::tagDbPtr,
		    this);
	} else if ((!userName.isEmpty() && !msgIdList.isEmpty()) || (!userName.isEmpty())) {
		dlgRet = DlgTags::editAssignment(userName, GlobInstcs::tagDbPtr,
		    msgIdList, this);
	} else {
		Q_ASSERT(0);
		return;
	}

	if (userName.isEmpty() || (dlgRet == DlgTags::NO_ACTION)) {
		/* Nothing else to do. */
		return;
	}

	if (dlgRet == DlgTags::TAGS_CHANGED) {
		/* May affect all rows. */
		msgIdList.clear();
		for (int row = 0; row < m_messageTableModel.rowCount(); ++row) {
			msgIdList.append(m_messageTableModel.index(row,
			    DbMsgsTblModel::DMID_COL).data().toLongLong());
		}
	}

	if (msgIdList.isEmpty()) {
		return;
	}

	m_messageTableModel.refillTagsColumn(userName, msgIdList,
	    DbMsgsTblModel::TAGS_NEG_COL);
}

void MainWindow::doExportOfSelectedFiles(
    enum Exports::ExportFileType expFileType)
{
	debugFuncCall();

	QString lastPath;
	QString errStr;

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	const QList<MessageDb::MsgId> msgIds(
	    msgMsgIds(currentFrstColMessageIndexes()));
	if (0 == msgIds.size()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName);
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return;
	}

	const QString dbId(
	    GlobInstcs::accntDbPtr->dbId(AccountDb::keyFromLogin(userName)));

	setAccountStoragePaths(userName);
	lastPath = m_on_export_zfo_activate;
	Exports::ExportError ret;

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	foreach (MessageDb::MsgId msgId, msgIds) {
		Q_ASSERT(msgId.dmId >= 0);
		ret = Exports::exportAs(this, *dbSet, expFileType,
		    m_on_export_zfo_activate, QString(), userName, accountName,
		    dbId, msgId, true, lastPath, errStr);
		if (Exports::EXP_CANCELED == ret) {
			break;
		} else if (Exports::EXP_NOT_MSG_DATA == ret) {
			if (messageMissingOfferDownload(msgId, errStr)) {
				Exports::exportAs(this, *dbSet, expFileType,
				    m_on_export_zfo_activate, QString(), userName,
				    accountName, dbId, msgId, true, lastPath, errStr);
			}
		}
		if (!lastPath.isEmpty()) {
			m_on_export_zfo_activate = lastPath;
			storeExportPath(userName);
		}

	}
}

void MainWindow::showImportMessageResults(const QString &userName,
    const QStringList &errImportList, int totalMsgs, int importedMsgs)
{
	showStatusTextPermanently(
	    tr("Import of messages to account %1 finished").arg(userName));

	QString detailText;
	if (errImportList.count() > 0) {
		for (int m = 0; m < errImportList.count(); ++ m) {
			detailText += errImportList.at(m) + "\n";
		}
	}

	DlgMsgBox::message(this, QMessageBox::Information,
	    tr("Messages import result"),
	    tr("Import of messages into account '%1' finished with result:")
	        .arg(userName),
	    tr("Total of messages in database: %1").arg(totalMsgs) + "<br/><b>" +
	        tr("Imported messages: %1").arg(importedMsgs) + "<br/>" +
	        tr("Non-imported messages: %1").arg(errImportList.count()) + "</b><br/>",
	    detailText, QMessageBox::Ok);
}

void MainWindow::dockMenuPopulate(void)
{
	static const QIcon winIco(
	    IconContainer::construcIcon(IconContainer::ICON_MACOS_WINDOW));

	mui_dockMenu.clear();
	QAction *action;

	/* Main window. */
	action = mui_dockMenu.addAction(this->windowTitle());
	action->setData(QVariant::fromValue(this));
	action->setCheckable(true);
	action->setChecked(this->isActiveWindow());
#ifdef Q_OS_OSX
	action->setIconVisibleInMenu(true);
	action->setIcon(winIco);
#endif /* Q_OS_OSX */

	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		QDialog *dialogue = qobject_cast<QDialog *>(widget);
		if (dialogue == Q_NULLPTR) {
			continue;
		}
		/* Remaining dialogue windows. */
		action = mui_dockMenu.addAction(dialogue->windowTitle());
		action->setData(QVariant::fromValue(dialogue));
		action->setCheckable(true);
		action->setChecked(dialogue->isActiveWindow());
#ifdef Q_OS_OSX
		action->setIconVisibleInMenu(true);
		action->setIcon(winIco);
#endif /* Q_OS_OSX */
	}
}

void MainWindow::dockMenuActionTriggerred(QAction *action)
{
	if (Q_UNLIKELY(action == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}

	QObject *object = Q_NULLPTR;
	{
		QVariant data(action->data());
		if (data.canConvert<QObject *>()) {
			object = qvariant_cast<QObject *>(data);
		}
	}
	if (Q_UNLIKELY(object == Q_NULLPTR)) {
		return;
	}
	QWidget *widget = qobject_cast<QWidget *>(object);
	if (Q_UNLIKELY(widget == Q_NULLPTR)) {
		return;
	}
	if (!widget->isMinimized()) {
		widget->show();
	} else {
		widget->showNormal();
	}
	widget->raise();
	widget->activateWindow();
}

void MainWindow::enableSendEGovRequestAction(bool enable,
    const QString &userName)
{
	if (enable && !userName.isEmpty()) {
		const AcntSettings &itemSettings((*GlobInstcs::acntMapPtr)[userName]);
		ui->actionSend_egov_request->setEnabled(
		    !itemSettings.isTestAccount());
	} else {
		ui->actionSend_egov_request->setEnabled(false);
	}
}
