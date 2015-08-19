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


#include <cstdlib> /* exit(3) */
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
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
#include "src/common.h"
#include "src/crypto/crypto_funcs.h"
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_account_from_db.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_change_directory.h"
#include "src/gui/dlg_correspondence_overview.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_msg_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/gui/dlg_timestamp_expir.h"
#include "src/gui/dlg_import_zfo_result.h"
#include "src/gui/dlg_yes_no_checkbox.h"
#include "src/log/log.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db_set_container.h"
#include "ui_datovka.h"


#define WIN_POSITION_HEADER "window_position"
#define WIN_POSITION_X "x"
#define WIN_POSITION_Y "y"
#define WIN_POSITION_W "w"
#define WIN_POSITION_H "h"

QNetworkAccessManager* nam;

/*
 * If defined then no message table is going to be generated when clicking
 * on all sent or received messages.
 */
#define DISABLE_ALL_TABLE 1

/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
    m_statusProgressBar(NULL),
    m_syncAcntThread(0),
    m_syncAcntWorker(0),
    m_accountModel(this),
    m_filterLine(NULL),
    m_messageListProxyModel(this),
    m_messageMarker(this),
    m_lastSelectedMessageId(-1),
    m_lastStoredMessageId(-1),
    m_lastSelectedAccountNodeType(AccountModel::nodeUnknown),
    m_lastStoredAccountNodeType(AccountModel::nodeUnknown),
    m_searchDlgActive(false),
    m_received_1(200),
    m_received_2(200),
    m_sent_1(200),
    m_sent_2(200),
    m_sort_column(0),
    m_sort_order(""),
    m_save_attach_dir(QDir::homePath()),
    m_add_attach_dir(QDir::homePath()),
    m_export_correspond_dir(QDir::homePath()),
    m_on_export_zfo_activate(QDir::homePath()),
    m_on_import_database_dir_activate(QDir::homePath()),
    m_import_zfo_path(QDir::homePath()),
    isMainWindow(false),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	/* Window title. */
	setWindowTitle(tr(
	    "Datovka - Free client for Datov\303\251 schr\303\241nky"));
#ifdef PORTABLE_APPLICATION
	setWindowTitle(windowTitle() + " - " + tr("Portable version"));
#endif /* PORTABLE_APPLICATION */

	/* Generate messages search filter */
	QWidget *spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	// toolBar is a pointer to an existing toolbar
	ui->toolBar->addWidget(spacer);

	QLabel *searchLabel = new QLabel;
	searchLabel->setText(tr("Search: "));
	ui->toolBar->addWidget(searchLabel);

	/* Message filter field. */
	m_filterLine = new QLineEdit(this);
	connect(m_filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterMessages(QString)));
	m_filterLine->setFixedWidth(200);
	m_filterLine->setToolTip(tr("Enter sought expression"));
	ui->toolBar->addWidget(m_filterLine);
	/* Clear message filter button. */
	m_clearFilterLineButton = new QPushButton(this);
	m_clearFilterLineButton->setIcon(
	    QIcon(ICON_3PARTY_PATH "delete_16.png"));
	m_clearFilterLineButton->setToolTip(tr("Clear search field"));
	ui->toolBar->addWidget(m_clearFilterLineButton);
	connect(m_clearFilterLineButton, SIGNAL(clicked()), this,
	    SLOT(clearFilterField()));

	/* Create info status bar */
	statusBar = new QStatusBar(this);
	statusBar->setSizeGripEnabled(false);
	ui->statusBar->addWidget(statusBar,1);
	showStatusTextWithTimeout(tr("Welcome..."));

	/* Create status bar label shows database mode memory/disk */
	statusDbMode = new QLabel(this);
	statusDbMode->setText(tr("Storage: disk | disk"));
	statusDbMode->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	ui->statusBar->addWidget(statusDbMode,0);


	/* Create status bar online/offline label */
	statusOnlineLabel = new QLabel(this);
	statusOnlineLabel->setText(tr("Mode: offline"));
	statusOnlineLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
	ui->statusBar->addWidget(statusOnlineLabel,0);

	/* Create progress bar object and set default value */
	m_statusProgressBar = new QProgressBar(this);
	m_statusProgressBar->setAlignment(Qt::AlignRight);
	m_statusProgressBar->setMinimumWidth(100);
	m_statusProgressBar->setMaximumWidth(200);
	m_statusProgressBar->setTextVisible(true);
	m_statusProgressBar->setRange(0,100);
	m_statusProgressBar->setValue(0);
	setDefaultProgressStatus();
	ui->statusBar->addWidget(m_statusProgressBar,1);

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
//	ui->messageList->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->messageList->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->messageList->setFocusPolicy(Qt::StrongFocus);
	/* TODO -- Use a delegate? */
//	ui->messageList->setStyleSheet(
//	    "QTableView::item:focus { border-color:green; "
//	    "border-style:outset; border-width:2px; color:black; }");

	/* Message state combo box. */
	ui->messageStateCombo->setInsertPolicy(QComboBox::InsertAtBottom);
	ui->messageStateCombo->addItem(QIcon(ICON_16x16_PATH "red.png"),
	    tr("Unsettled"));
	ui->messageStateCombo->addItem(QIcon(ICON_16x16_PATH "yellow.png"),
	    tr("In Progress"));
	ui->messageStateCombo->addItem(QIcon(ICON_16x16_PATH "grey.png"),
	    tr("Settled"));

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(
	    QCoreApplication::applicationVersion()));
	ui->accountTextInfo->setReadOnly(true);

	/* Load configuration file. */
	loadSettings();

	/* Set toolbar buttons style from settings */
	if (globPref.toolbar_button_style >=0
	    && globPref.toolbar_button_style <= 3) {
		ui->toolBar->setToolButtonStyle(
		    (Qt::ToolButtonStyle)globPref.toolbar_button_style);
	} else {
		ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	}

	/* Account list must already be set in order to connect this signal. */
	connect(ui->accountList->selectionModel(),
	    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
	    SLOT(accountItemCurrentChanged(QModelIndex, QModelIndex)));

	/* Enable sorting of message table items. */
	ui->messageList->setSortingEnabled(true);

	/* Set default column size. */
	/* TODO -- Check whether received or sent messages are shown? */
	setReceivedColumnWidths();

	/* Attachment list. */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* Selection model may not be set. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemCurrentChanged(QModelIndex,
		        QModelIndex)));
	}
	ui->messageAttachmentList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->messageAttachmentList,
	    SIGNAL(customContextMenuRequested(QPoint)), this,
	    SLOT(attachmentItemRightClicked(QPoint)));
	connect(ui->messageAttachmentList,
	    SIGNAL(doubleClicked(QModelIndex)), this,
	    SLOT(attachmentItemDoubleClicked(QModelIndex)));

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
	connectTopToolBarSlots();
	connectMessageActionBarSlots();

	/* Message marking timer. */
	connect(&m_messageMarker, SIGNAL(timeout()),
	    this, SLOT(messageItemsSelectedMarkRead()));

	/* Initialisation of message download timer. */
	connect(&m_timerSyncAccounts, SIGNAL(timeout()), this,
	    SLOT(synchroniseAllAccounts()));
	if (globPref.download_on_background) {
		if (globPref.timer_value > 4) {
			m_timeoutSyncAccounts = globPref.timer_value * 60000;
		} else {
			m_timeoutSyncAccounts = TIMER_DEFAULT_TIMEOUT_MS;
		}
		m_timerSyncAccounts.start(m_timeoutSyncAccounts);
		qDebug() << "Timer set to" << m_timeoutSyncAccounts / 60000
		    << "minutes";
	}

	QTimer::singleShot(RUN_FIRST_ACTION_MS, this,
	    SLOT(setWindowsAfterInit()));

	QString msgStrg = tr("disk");
	QString acntStrg = tr("disk");

	if (!globPref.store_messages_on_disk) {
		msgStrg = tr("memory");
	}

	if (!globPref.store_additional_data_on_disk) {
		acntStrg = tr("memory");
	}

	statusDbMode->setText(tr("Storage:") + " " + msgStrg + " | "
	    + acntStrg + "   ");
}


/* ========================================================================= */
/*
 * Do actions after main window initialization.
 */
void MainWindow::setWindowsAfterInit(void)
/* ========================================================================= */
{
	debugSlotCall();

	isMainWindow = true;

	if (globPref.check_new_versions) {
		checkNewDatovkaVersion();
	}

	if (ui->accountList->model()->rowCount() <= 0) {
		addNewAccount();
	} else {
		if (globPref.download_at_start) {
			synchroniseAllAccounts();
		}
	}
}


/* ========================================================================= */
/*
 * Sent and check a new version of Datovka
 */
void MainWindow::checkNewDatovkaVersion(void)
/* ========================================================================= */
{
	debugSlotCall();

	if (globPref.send_stats_with_version_checks) {
		/* TODO - sent info about datovka, libs and OS to our server */
		nam = new QNetworkAccessManager(this);
		QObject::connect(nam, SIGNAL(finished(QNetworkReply*)),
		this, SLOT(datovkaVersionResponce(QNetworkReply*)));
		QUrl url(DATOVKA_CHECK_NEW_VERSION_URL);
		nam->get(QNetworkRequest(url));
	} else {
		nam = new QNetworkAccessManager(this);
		QObject::connect(nam, SIGNAL(finished(QNetworkReply*)),
		this, SLOT(datovkaVersionResponce(QNetworkReply*)));
		QUrl url(DATOVKA_CHECK_NEW_VERSION_URL);
		nam->get(QNetworkRequest(url));
	}
}


/* ========================================================================= */
/*
 * Version response slot
 */
void MainWindow::datovkaVersionResponce(QNetworkReply* reply)
/* ========================================================================= */
{
	debugFuncCall();

	 if (reply->error() == QNetworkReply::NoError) {
		QByteArray bytes = reply->readAll();
		QString vstr = QString::fromUtf8(bytes.data(), bytes.size());
		vstr.remove(QRegExp("[\n\t\r]"));
		if (vstr > QCoreApplication::applicationVersion()) {
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


/* ========================================================================= */
/*
 * Set default status of progress bar
 */
void MainWindow::setDefaultProgressStatus(void)
/* ========================================================================= */
{
	m_statusProgressBar->setFormat("Idle");
	m_statusProgressBar->setValue(0);
}


/* ========================================================================= */
MainWindow::~MainWindow(void)
/* ========================================================================= */
{
	/* Save settings on exit. */
	saveSettings();

	delete ui;
}


/* ========================================================================= */
/*
 * Shows the application preferences dialog.
 */
void MainWindow::applicationPreferences(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *dlgPrefs = new DlgPreferences(this);
	dlgPrefs->exec();

	// set actual download timer value from settings if is enable
	if (globPref.download_on_background) {
		if (globPref.timer_value > 4) {
			m_timeoutSyncAccounts = globPref.timer_value * 60000;
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


/* ========================================================================= */
/*
 * Proxy setting dialog.
 */
void MainWindow::proxySettings(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *dlgProxy = new DlgProxysets(this);
	if (QDialog::Accepted == dlgProxy->exec()) {
		/* Dialog accepted, store all settings. */
		saveSettings();
	}
}


/* ========================================================================= */
/*
 * Redraws widgets according to selected account item.
 */
void MainWindow::accountItemCurrentChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	debugSlotCall();

	(void) previous; /* Unused. */

	QString html;
	DbMsgsTblModel *msgTblMdl = 0;

	if (!current.isValid()) {
		/* May occur on deleting last account. */
		setMessageActionVisibility(false);

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
		ui->messageList->setModel(0);
		ui->messageStackedWidget->setCurrentIndex(0);
		ui->accountTextInfo->setHtml(createDatovkaBanner(
		    QCoreApplication::applicationVersion()));
		ui->accountTextInfo->setReadOnly(true);
		return;
	}

	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(current);
	const QStandardItem *accountItemTop =
	    AccountModel::itemTop(accountItem);
	const QString userName =
	    accountItemTop->data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	if (0 == dbSet) {
		/* May occur on deleting last account. */
		setMessageActionVisibility(false);

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
		const AccountModel::SettingsMap &itemSettings =
		    AccountModel::globAccounts[userName];

		QString dbDir = itemSettings.dbDir();
		if (dbDir.isEmpty()) {
			/* Set default directory name. */
			dbDir = globPref.confDir();
		}

		/* Decouple model and show banner page. */
		ui->messageList->setModel(0);
		ui->messageStackedWidget->setCurrentIndex(0);
		QString htmlMessage = "<div style=\"margin-left: 12px;\">"
		    "<h3>" + tr("Database access error") + "</h3>" "<br/>";
		htmlMessage += "<div>";
		htmlMessage += tr("Database files for account '%1' cannot be accessed in location '%2'."
		    ).arg(userName).arg(dbDir);
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

	setAccountStoragePaths(userName);

	/*
	 * Disconnect message clicked. This slot will be enabled only for
	 * received messages.
	 */
	ui->messageList->disconnect(SIGNAL(clicked(QModelIndex)),
	    this, SLOT(messageItemClicked(QModelIndex)));

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
		setMessageActionVisibility(false);
		html = createAccountInfo(userName);
		ui->actionDelete_message_from_db->setEnabled(false);
		break;
	case AccountModel::nodeRecentReceived:
		msgTblMdl = dbSet->msgsRcvdWithin90DaysModel();
		//ui->messageList->horizontalHeader()->moveSection(5,3);
		ui->actionDelete_message_from_db->setEnabled(false);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeRecentSent:
		msgTblMdl = dbSet->msgsSntWithin90DaysModel();
		ui->actionDelete_message_from_db->setEnabled(false);
		break;
	case AccountModel::nodeAll:
		setMessageActionVisibility(false);
		html = createAccountInfoAllField(tr("All messages"),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_RECEIVED,
		        DESCENDING),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_SENT, DESCENDING));
		ui->actionDelete_message_from_db->setEnabled(false);
		break;
	case AccountModel::nodeReceived:
#ifdef DISABLE_ALL_TABLE
		setMessageActionVisibility(false);
		html = createAccountInfoMessagesCount(
		    tr("All received messages"),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_RECEIVED,
		        DESCENDING),
		    MessageDb::TYPE_RECEIVED);
		ui->actionDelete_message_from_db->setEnabled(false);
#else /* !DISABLE_ALL_TABLE */
		msgTblMdl = dbSet->msgsRcvdModel();
		ui->actionDelete_message_from_db->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
#endif /* DISABLE_ALL_TABLE */
		break;
	case AccountModel::nodeSent:
#ifdef DISABLE_ALL_TABLE
		setMessageActionVisibility(false);
		html = createAccountInfoMessagesCount(
		    tr("All sent messages"),
		    dbSet->msgsYearlyCounts(MessageDb::TYPE_SENT, DESCENDING),
		    MessageDb::TYPE_SENT);
		ui->actionDelete_message_from_db->setEnabled(false);
#else /* !DISABLE_ALL_TABLE */
		msgTblMdl = dbSet->msgsSntModel();
		ui->actionDelete_message_from_db->setEnabled(true);
#endif /* DISABLE_ALL_TABLE */
		break;
	case AccountModel::nodeReceivedYear:
		/* TODO -- Parameter check. */
		msgTblMdl = dbSet->msgsRcvdInYearModel(accountItem->text());
		ui->actionDelete_message_from_db->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeSentYear:
		/* TODO -- Parameter check. */
		msgTblMdl = dbSet->msgsSntInYearModel(accountItem->text());
		ui->actionDelete_message_from_db->setEnabled(true);
		break;
	default:
		Q_ASSERT(0);
		break;
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

	/* Depending on which item was clicked show/hide elements. */
	QAbstractItemModel *itemModel;
	bool received = true;

	switch (AccountModel::nodeType(current)) {
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
		Q_ASSERT(0 != msgTblMdl);
		m_messageListProxyModel.setSortRole(ROLE_MSGS_DB_PROXYSORT);
		m_messageListProxyModel.setSourceModel(msgTblMdl);
		ui->messageList->setModel(&m_messageListProxyModel);
		/* Set specific column width. */
		setReceivedColumnWidths();
		received = true;
		goto setmodel;
	case AccountModel::nodeRecentSent:
#ifndef DISABLE_ALL_TABLE
	case AccountModel::nodeSent:
#endif /* !DISABLE_ALL_TABLE */
	case AccountModel::nodeSentYear:
		/* Set model. */
		Q_ASSERT(0 != msgTblMdl);
		m_messageListProxyModel.setSortRole(ROLE_MSGS_DB_PROXYSORT);
		m_messageListProxyModel.setSourceModel(msgTblMdl);
		ui->messageList->setModel(&m_messageListProxyModel);
		/* Set specific column width. */
		setSentColumnWidths();
		received = false;

setmodel:
		ui->messageStackedWidget->setCurrentIndex(1);
		/* Apply message filter. */
		filterMessages(m_filterLine->text());
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
		/* Select last message in list if there are some messages. */
		itemModel = ui->messageList->model();
		/* enable/disable buttons */
		if ((0 != itemModel) && (0 < itemModel->rowCount())) {
			messageItemRestoreSelectionOnModelChange();
			ui->menuMessage->setEnabled(true);
			//ui->actionReply_to_the_sender->setEnabled(true);
			ui->actionVerify_a_message->setEnabled(true);
			ui->actionAuthenticate_message_file->setEnabled(true);
			ui->actionExport_correspondence_overview->
			    setEnabled(true);
			ui->actionCheck_message_timestamp_expiration->
			    setEnabled(true);
		} else {
			ui->menuMessage->setEnabled(false);
			ui->actionReply->setEnabled(false);
			ui->actionCreate_message_from_template->setEnabled(false);
			ui->actionReply_to_the_sender->setEnabled(false);
			ui->actionVerify_a_message->setEnabled(false);
			ui->actionAuthenticate_message_file->setEnabled(false);
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	/* Set specific column width. */
	received ? setReceivedColumnWidths() : setSentColumnWidths();
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
	QMenu *submenu = 0;
#ifdef PORTABLE_APPLICATION
	QAction *action;
#endif /* PORTABLE_APPLICATION */

	if (acntIdx.isValid()) {
		bool received = AccountModel::nodeTypeIsReceived(acntIdx);

		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-account-sync.png"),
		    tr("Get messages"),
		    this, SLOT(synchroniseSelectedAccount()));
		menu->addAction(QIcon(ICON_16x16_PATH "datovka-message.png"),
		    tr("Create message"),
		    this, SLOT(createAndSendMessage()));
		menu->addSeparator();
		if (received) {
			submenu = menu->addMenu(tr("Mark"));
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
		menu->addAction(QIcon(ICON_3PARTY_PATH "user_16.png"),
		    tr("Change password"),
		    this, SLOT(changeAccountPassword()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "letter_16.png"),
		    tr("Account properties"),
		    this, SLOT(manageAccountProperties()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "delete_16.png"),
		    tr("Remove Account"),
		    this, SLOT(deleteSelectedAccount()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "up_16.png"),
		    tr("Move account up"),
		    this, SLOT(moveSelectedAccountUp()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "down_16.png"),
		    tr("Move account down"),
		    this, SLOT(moveSelectedAccountDown()));
		menu->addSeparator();
#ifdef PORTABLE_APPLICATION
		action =
#endif /* PORTABLE_APPLICATION */
		    menu->addAction(
		        QIcon(ICON_3PARTY_PATH "folder_16.png"),
		        tr("Change data directory"),
		        this, SLOT(changeDataDirectory()));
#ifdef PORTABLE_APPLICATION
		action->setEnabled(false);
#endif /* PORTABLE_APPLICATION */

		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "statistics_16.png"),
		    tr("Split database by years"),
		    this, SLOT(splitMsgDbByYearsSlot()));

	} else {
		menu->addAction(QIcon(ICON_3PARTY_PATH "plus_16.png"),
		    tr("Add new account"),
		    this, SLOT(addNewAccount()));
	}

	menu->exec(QCursor::pos());
}

/*!
 * @brief Gets message time from selected index.
 */
static inline
QDateTime msgDeliveryTime(const QModelIndex &msgIdx)
{
	QModelIndex deliveryIdx(msgIdx.sibling(msgIdx.row(), DbMsgsTblModel::DELIVERY_COL));
	Q_ASSERT(deliveryIdx.isValid());

	QDateTime deliveryTime = dateTimeFromDbFormat(deliveryIdx.data(ROLE_PLAIN_DISPLAY).toString());

	return deliveryTime;
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

	(void) selected; /* Unused. */
	(void) deselected; /* Unused. */

	/*
	 * Disconnect slot from model as we want to prevent a signal to be
	 * handled multiple times.
	 */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* New model hasn't been set yet. */
		ui->messageAttachmentList->selectionModel()->disconnect(
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemCurrentChanged(QModelIndex,
		        QModelIndex)));
	}

	/* Disable message/attachment related buttons. */
	setMessageActionVisibility(false);

	ui->downloadComplete->setEnabled(false);
	ui->saveAttachments->setEnabled(false);
	ui->saveAttachment->setEnabled(false);
	ui->openAttachment->setEnabled(false);
	ui->verifySignature->setEnabled(false);
	ui->signatureDetails->setEnabled(false);
	ui->actionSave_all_attachments->setEnabled(false);
	ui->actionOpen_attachment->setEnabled(false);
	ui->actionSave_attachment->setEnabled(false);
	ui->messageStateCombo->setEnabled(false);

	/* Disable model for attachment list. */
	ui->messageAttachmentList->setModel(0);
	/* Clear message information. */
	ui->messageInfo->setHtml("");
	ui->messageInfo->setReadOnly(true);

	QModelIndexList firstColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	/* Stop the timer. */
	m_messageMarker.stop();

	if (firstColumnIdxs.isEmpty()) {
		/* Invalid message selected. */
		messageItemStoreSelection(-1);
		/* End if invalid item is selected. */
		return;
	}

	/* Enable message/attachment related buttons. */
	setMessageActionVisibility(true);

	ui->downloadComplete->setEnabled(true);

	bool received = AccountModel::nodeTypeIsReceived(ui->accountList->
	    selectionModel()->currentIndex());
	ui->actionReply->setEnabled(received);
	ui->actionReply_to_the_sender->setEnabled(received);

	ui->messageStateCombo->setEnabled(received);

	if (1 == firstColumnIdxs.size()) {
		/*
		 * Enabled all actions that can only be performed when
		 * single message selected.
		 */
		//ui->actionReply->setEnabled(received);
		ui->actionCreate_message_from_template->setEnabled(true);
		ui->actionSignature_detail->setEnabled(true);
		ui->actionAuthenticate_message->setEnabled(true);
		ui->actionOpen_message_externally->setEnabled(true);
		ui->actionOpen_delivery_info_externally->setEnabled(true);
		ui->actionExport_as_ZFO->setEnabled(true);
		ui->actionExport_delivery_info_as_ZFO->setEnabled(true);
		ui->actionExport_delivery_info_as_PDF->setEnabled(true);
		ui->actionExport_message_envelope_as_PDF->setEnabled(true);

		const QModelIndex &index = firstColumnIdxs.first();

		MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
		Q_ASSERT(0 != dbSet);
		qint64 msgId = index.data().toLongLong();
		/* Remember last selected message. */
		messageItemStoreSelection(msgId);

		QDateTime deliveryTime = msgDeliveryTime(index);
		MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime,
		    false);
		Q_ASSERT(0 != messageDb);

		/* Mark message locally read. */
		if (!messageDb->smsgdtLocallyRead(msgId)) {
			qDebug()
			    << "Starting timer to mark as read for message"
			    << msgId;
			m_messageMarker.setSingleShot(true);
			m_messageMarker.start(
			    globPref.message_mark_as_read_timeout);
		} else {
			m_messageMarker.stop();
		}

		/* Generate and show message information. */
		ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId,
		    ui->verifySignature));
		ui->messageInfo->setReadOnly(true);

		if (received) {
			int msgState = messageDb->msgGetProcessState(msgId);

			/* msgState is -1 if message is not in database */
			if (msgState >= 0) {
				ui->messageStateCombo->setCurrentIndex(
				    msgState);
			} else {
				/* insert message state into database */
				messageDb->msgSetProcessState(msgId, UNSETTLED,
				    true);
				ui->messageStateCombo->setCurrentIndex(
				    UNSETTLED);
			}
		} else {
			ui->messageStateCombo->setCurrentIndex(UNSETTLED);
		}

		/* Enable buttons according to database content. */
		ui->verifySignature->setEnabled(
		    !messageDb->msgsVerificationAttempted(msgId));
		ui->signatureDetails->setEnabled(true);
		/* Show files related to message message. */
		QAbstractTableModel *fileTblMdl = messageDb->flsModel(msgId);
		Q_ASSERT(0 != fileTblMdl);
		//qDebug() << "Setting files";
		ui->messageAttachmentList->setModel(fileTblMdl);
		/* First three columns contain hidden data. */
		ui->messageAttachmentList->setColumnHidden(0, true);
		ui->messageAttachmentList->setColumnHidden(1, true);
		ui->messageAttachmentList->setColumnHidden(2, true);

		if (ui->messageAttachmentList->model()->rowCount() > 0) {
			ui->saveAttachments->setEnabled(true);
			ui->actionSave_all_attachments->setEnabled(true);
		}

		ui->messageAttachmentList->resizeColumnToContents(3);

		/* Connect new slot. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemCurrentChanged(QModelIndex,
		        QModelIndex)));
	} else {
		/*
		 * Disable all actions that cannot be performed when
		 * multiple messages selected.
		 */
		ui->actionReply->setEnabled(false);
		ui->actionCreate_message_from_template->setEnabled(false);
		ui->actionSignature_detail->setEnabled(false);
		ui->actionAuthenticate_message->setEnabled(false);
		ui->actionOpen_message_externally->setEnabled(false);
		ui->actionOpen_delivery_info_externally->setEnabled(false);
		ui->actionExport_as_ZFO->setEnabled(false);
		ui->actionExport_delivery_info_as_ZFO->setEnabled(false);
		ui->actionExport_delivery_info_as_PDF->setEnabled(false);
		ui->actionExport_message_envelope_as_PDF->setEnabled(false);

		ui->actionReply_to_the_sender->setEnabled(false);
		ui->actionVerify_a_message->setEnabled(false);
	}
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
		qDebug() << "Not clicked read locally.";
		return;
	}

	/* Stop the timer. */
	m_messageMarker.stop();

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	qint64 msgId = index.sibling(index.row(), 0).data().toLongLong();
	QDateTime deliveryTime = msgDeliveryTime(index);

	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	/* Get message state from database and toggle the value. */
	bool isRead = messageDb->smsgdtLocallyRead(msgId);
	messageDb->smsgdtSetLocallyRead(msgId, !isRead);

	/*
	 * Mark message as read without reloading
	 * the whole model.
	 */
	DbMsgsTblModel *messageModel = (DbMsgsTblModel *)
	    m_messageListProxyModel.sourceModel();
	Q_ASSERT(0 != messageModel);

	messageModel->overrideRead(msgId, !isRead);
	/* Inform the view that the model has changed. */
	emit messageModel->dataChanged(
	    index.sibling(index.row(), 0),
	    index.sibling(index.row(), messageModel->columnCount() - 1));

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(ui->accountList->
	    selectionModel()->currentIndex());
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
	bool received = AccountModel::nodeTypeIsReceived(ui->accountList->
	    selectionModel()->currentIndex());

	QMenu *menu = new QMenu;
	QMenu *submenu = 0;

	/*
	 * Remember last selected message. Pick the first from
	 * the current selection.
	 *
	 * TODO -- Save whole selection?
	 */
	{
		QModelIndexList firstMsgColumnIdxs =
		    ui->messageList->selectionModel()->selectedRows(0);

		singleSelected = (1 == firstMsgColumnIdxs.size());

		if (!firstMsgColumnIdxs.isEmpty()) {
			messageItemStoreSelection(firstMsgColumnIdxs
			    .first().data().toLongLong());
		}
	}

	menu->addAction(
	    QIcon(ICON_16x16_PATH "datovka-message-download.png"),
	    tr("Download message signed"), this,
	    SLOT(downloadSelectedMessageAttachments()));
	if (singleSelected) {
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-reply.png"),
		    tr("Reply to message"), this,
		    SLOT(createAndSendMessageReply()))->
		    setEnabled(ui->actionReply->isEnabled());
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message.png"),
		    tr("Use message as template"), this,
		    SLOT(createAndSendMessageFromTmpl()))->
		    setEnabled(
		        ui->actionCreate_message_from_template->isEnabled());
	}
	menu->addSeparator();
	if (singleSelected) {
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH "label_16.png"),
		    tr("Signature details"), this,
		    SLOT(showSignatureDetails()));
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-verify.png"),
		    tr("Authenticate message"), this,
		    SLOT(verifySelectedMessage()));
		menu->addSeparator();
		menu->addAction(
		    tr("Open message externally"), this,
		    SLOT(openSelectedMessageExternally()))->
		    setEnabled(ui->actionOpen_message_externally->isEnabled());
		menu->addAction(
		    tr("Open delivery info externally"), this,
		    SLOT(openDeliveryInfoExternally()))->
		    setEnabled(ui->actionOpen_delivery_info_externally->isEnabled());
		menu->addSeparator();
		menu->addAction(
		    tr("Export message as ZFO"), this,
		    SLOT(exportSelectedMessageAsZFO()))->
		    setEnabled(ui->actionExport_as_ZFO->isEnabled());
		menu->addAction(
		    tr("Export delivery info as ZFO"), this,
		    SLOT(exportDeliveryInfoAsZFO()))->
		    setEnabled(ui->actionExport_delivery_info_as_ZFO->isEnabled());
		menu->addAction(
		    tr("Export delivery info as PDF"), this,
		    SLOT(exportDeliveryInfoAsPDF()))->
		    setEnabled(ui->actionExport_delivery_info_as_PDF->isEnabled());
		menu->addAction(
		    tr("Export message envelope as PDF"), this,
		    SLOT(exportMessageEnvelopeAsPDF()))->
		    setEnabled(ui->actionExport_message_envelope_as_PDF->isEnabled());
		menu->addSeparator();
	}
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
	menu->addAction(
	    QIcon(ICON_3PARTY_PATH "delete_16.png"),
	    tr("Delete message"), this,
	    SLOT(deleteMessage()))->
	    setEnabled(ui->actionDelete_message_from_db->isEnabled());

	menu->exec(QCursor::pos());
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
	QModelIndex acntIdx = ui->accountList->currentIndex();
	m_lastSelectedAccountNodeType = AccountModel::nodeType(acntIdx);
	if (AccountModel::nodeRecentReceived ==
	    m_lastSelectedAccountNodeType) {

		qDebug() << "Storing recent received selection into the model"
		    << msgId;

		acntIdx = AccountModel::indexTop(acntIdx);
		const QString userName =
		    acntIdx.data(ROLE_ACNT_USER_NAME).toString();
		Q_ASSERT(!userName.isEmpty());
		AccountModel::globAccounts[userName].setLastMsg(msgId);
	}
}



/* ========================================================================= */
/*
 * Saves account export paths.
 */
void MainWindow::storeExportPath(void)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntIdx = ui->accountList->currentIndex();
	acntIdx = AccountModel::indexTop(acntIdx);
	const QString userName = acntIdx.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	AccountModel::SettingsMap &accountInfo =
	    AccountModel::globAccounts[userName];
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

	QModelIndex acntIdx = ui->accountList->currentIndex();
	AccountModel::NodeType acntNodeType = AccountModel::nodeType(acntIdx);

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
	switch (globPref.after_start_select) {
	case GlobPreferences::SELECT_NEWEST:
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
	case GlobPreferences::SELECT_LAST_VISITED:
		{
			qint64 msgLastId = -1;
			if (AccountModel::nodeRecentReceived == acntNodeType) {
				acntIdx = AccountModel::indexTop(acntIdx);
				const QString userName =
				    acntIdx.data(ROLE_ACNT_USER_NAME)
				        .toString();
				Q_ASSERT(!userName.isEmpty());

				msgLastId =
				    AccountModel::globAccounts[userName]
				        .lastMsg();
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
	case GlobPreferences::SELECT_NOTHING:
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

	/* first step: search correspond account index from username */
	QModelIndex acntIdxTop =
	    m_accountModel.indexFromItem(itemFromUserName(userName));

	if (!acntIdxTop.isValid()) {
		return;
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
		return;
		break;
	}

	/* third step: obtain index with given year */
	int childRow = 0;
	QModelIndex yearIdx = typeIdx.child(childRow, 0);
	while (yearIdx.isValid() &&
	       (yearIdx.data().toString() != deliveryYear)) {
		yearIdx = yearIdx.sibling(++childRow, 0);
	}

	if (!yearIdx.isValid()) {
		Q_ASSERT(0);
		return;
	}

	/* fourth step: find and select message according to msgId */
	QModelIndex msgIdx;
	ui->accountList->setCurrentIndex(yearIdx);
	accountItemCurrentChanged(yearIdx);

	const QAbstractItemModel *model = ui->messageList->model();
	Q_ASSERT(0 != model);

	int rowCount = model->rowCount();

	if (0 == rowCount) {
		/* Do nothing on empty model. */
		return;
	}

	/* Find and select the message with the ID. */
	for (int row = 0; row < rowCount; ++row) {
		/*
		 * TODO -- Search in a more resource-saving way.
		 * Eliminate index copying, use smarter search.
		 */
		if (model->index(row, 0).data().toLongLong() == msgId) {
			msgIdx = model->index(row, 0);
			break;
		}
	}

	if (msgIdx.isValid()) { /*(row < rowCount)*/
		/* Message found. */
		ui->messageList->setCurrentIndex(msgIdx);
		ui->messageList->scrollTo(msgIdx);
	}
}


/* ========================================================================= */
/*
 * Redraws widgets according to selected attachment item.
 */
void MainWindow::attachmentItemCurrentChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	debugSlotCall();

	/* If the row has not been changed then do nothing. */
	if (current.isValid() && previous.isValid() &&
	    (current.row() == previous.row())) {
		return;
	}

	Q_ASSERT(current.isValid());
	if (!current.isValid()) {
		return;
	}

	//qDebug() << "Attachment selection changed.";
	ui->saveAttachment->setEnabled(true);
	//ui->saveAttachments->setEnabled(true);
	ui->openAttachment->setEnabled(true);
	ui->actionSave_attachment->setEnabled(true);
	ui->actionOpen_attachment->setEnabled(true);
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

	QModelIndex index = ui->messageAttachmentList->indexAt(point);
	QMenu *menu = new QMenu;

	if (index.isValid()) {
		//attachmentItemCurrentChanged(index);

		/* TODO */
		menu->addAction(QIcon(ICON_3PARTY_PATH "folder_16.png"),
		    tr("Open attachment"), this,
		    SLOT(openSelectedAttachment()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "save_16.png"),
		    tr("Save attachment"), this,
		    SLOT(saveSelectedAttachmentToFile()));
	} else {
		/* Do nothing. */
	}
	menu->exec(QCursor::pos());
}


/* ========================================================================= */
/*
 * Handle attachment double click.
 */
void MainWindow::attachmentItemDoubleClicked(const QModelIndex &index)
/* ========================================================================= */
{
	debugSlotCall();

	(void) index;
	//qDebug() << "Attachment double clicked.";
	openSelectedAttachment();
}


/* ========================================================================= */
/*
 * Saves selected attachment to file.
 */
void MainWindow::saveSelectedAttachmentToFile(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex selectedIndex =
	    ui->messageAttachmentList->selectionModel()->currentIndex();
	    /* selection().indexes() ? */

	//qDebug() << "Save attachment to file." << selectedIndex;

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		showStatusTextWithTimeout(tr("Saving attachment of message to "
		    "files was not successful!"));
		return;
	}

	QModelIndex messageIndex =
	    ui->messageList->selectionModel()->currentIndex();
	qint64 dmId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toLongLong();

	QModelIndex fileNameIndex =
	    selectedIndex.sibling(selectedIndex.row(), 3);
	Q_ASSERT(fileNameIndex.isValid());
	if(!fileNameIndex.isValid()) {
		showStatusTextWithTimeout(tr("Saving attachment of message "
		"\"%1\" to files was not successful!").arg(dmId));
		return;
	}
	QString fileName = fileNameIndex.data().toString();
	Q_ASSERT(!fileName.isEmpty());
	/* TODO -- Remember directory? */

	QString saveAttachPath;
	if (globPref.use_global_paths) {
		saveAttachPath = globPref.save_attachments_path;
	} else {
		saveAttachPath = m_save_attach_dir;
	}

	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);
	const QString userName =
	    acntTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	QDateTime deliveryTime = msgDeliveryTime(messageIndex);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	fileName =
	    createFilenameFromFormatString(globPref.attachment_filename_format,
	    QString::number(dmId), dbId, userName, fileName,
	    entry.dmDeliveryTime, entry.dmAcceptanceTime, entry.dmAnnotation,
	    entry.dmSender);

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save attachment"),
	    saveAttachPath + QDir::separator() + fileName);

	if (fileName.isEmpty()) {
		return;
	}

	if (!globPref.use_global_paths) {
		m_save_attach_dir =
		    QFileInfo(fileName).absoluteDir().absolutePath();
		storeExportPath();
	}

	/* Get data from base64. */
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(), 2);
	Q_ASSERT(dataIndex.isValid());
	if (!dataIndex.isValid()) {
		showStatusTextWithTimeout(tr("Saving attachment of message "
		"\"%1\" to files was not successful!").arg(dmId));
		return;
	}

	QByteArray data =
	    QByteArray::fromBase64(dataIndex.data().toByteArray());

	enum WriteFileState ret = writeFile(fileName, data);
	if (WF_SUCCESS == ret) {
		showStatusTextWithTimeout(tr("Saving attachment of message "
		"\"%1\" to file was successful.").arg(dmId));
	} else {
		showStatusTextWithTimeout(tr("Saving attachment of message "
		"\"%1\" to file was not successful!").arg(dmId));
		QMessageBox::warning(this,
		    tr("Error saving attachment of message '%1'.").arg(dmId),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Save all attachments to dir.
 */
void MainWindow::saveAllAttachmentsToDir(void)
/* ========================================================================= */
{
	debugSlotCall();

	int attachments = ui->messageAttachmentList->model()->rowCount();

	QModelIndex messageIndex =
	    ui->messageList->selectionModel()->currentIndex();

	qint64 dmId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toLongLong();

	QString saveAttachPath;
	if (globPref.use_global_paths) {
		saveAttachPath = globPref.save_attachments_path;
	} else {
		saveAttachPath = m_save_attach_dir;
	}

	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Save attachments"), saveAttachPath,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (newDir.isNull() || newDir.isEmpty()) {
		return;
	}

	if (!globPref.use_global_paths) {
		m_save_attach_dir = newDir;
		storeExportPath();
	}

	bool unspecifiedFailed = false;
	QList<QString> unsuccessfullFiles;

	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);
	const QString userName =
	    acntTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	QDateTime deliveryTime = msgDeliveryTime(messageIndex);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	for (int i = 0; i < attachments; ++i) {

		QModelIndex index = ui->messageAttachmentList->model()
		    ->index(i,0);

		Q_ASSERT(index.isValid());
		if (!index.isValid()) {
			unspecifiedFailed = true;
			continue;
		}

		QModelIndex fileNameIndex = index.sibling(index.row(), 3);
		Q_ASSERT(fileNameIndex.isValid());
		if(!fileNameIndex.isValid()) {
			unspecifiedFailed = true;
			continue;
		}

		QString fileName = fileNameIndex.data().toString();
		QString attFileName = fileName;
		Q_ASSERT(!fileName.isEmpty());
		if (fileName.isEmpty()) {
			unspecifiedFailed = true;
			continue;
		}

		fileName = createFilenameFromFormatString(
		    globPref.attachment_filename_format,
		    QString::number(dmId), dbId, userName, fileName,
		    entry.dmDeliveryTime, entry.dmAcceptanceTime,
		    entry.dmAnnotation, entry.dmSender);

		fileName = newDir + QDir::separator() + fileName;

		QModelIndex dataIndex = index.sibling(index.row(), 2);
		Q_ASSERT(dataIndex.isValid());
		if (!dataIndex.isValid()) {
			unsuccessfullFiles.append(fileName);
			continue;
		}

		QByteArray data =
		    QByteArray::fromBase64(dataIndex.data().toByteArray());

		if (WF_SUCCESS != writeFile(fileName, data)) {
			unsuccessfullFiles.append(fileName);
			continue;
		}

		if (globPref.delivery_info_for_every_file) {
			if (globPref.all_attachments_save_zfo_delinfo) {
				exportDeliveryInfoAsZFO(newDir, attFileName,
				    globPref.delivery_filename_format_all_attach,
				    dmId, deliveryTime);
			}
			if (globPref.all_attachments_save_pdf_delinfo) {
				exportDeliveryInfoAsPDF(newDir, attFileName,
				  globPref.delivery_filename_format_all_attach,
				  dmId, deliveryTime);
			}
		}
	}

	if (globPref.all_attachments_save_zfo_msg) {
		exportSelectedMessageAsZFO(newDir, userName,
		    dmId, deliveryTime);
	}

	if (globPref.all_attachments_save_pdf_msgenvel) {
		exportMessageEnvelopeAsPDF(newDir, dmId, deliveryTime);
	}

	if (!globPref.delivery_info_for_every_file) {
		if (globPref.all_attachments_save_zfo_delinfo) {
			exportDeliveryInfoAsZFO(newDir, "",
			    globPref.delivery_filename_format,
			    dmId, deliveryTime);
		}
		if (globPref.all_attachments_save_pdf_delinfo) {
			exportDeliveryInfoAsPDF(newDir, "",
			    globPref.delivery_filename_format,
			    dmId, deliveryTime);
		}
	}

	if (unspecifiedFailed) {
		showStatusTextWithTimeout(tr("Some attachments of "
		    "message \"%1\" were not saved to disk!").arg(dmId));
		QMessageBox::warning(this,
		    tr("Error saving attachments of message '%1'.").arg(dmId),
		    tr("Could not save all attachments of message '%1'.")
		        .arg(dmId),
		    QMessageBox::Ok);
	} else if (!unsuccessfullFiles.isEmpty()) {
		showStatusTextWithTimeout(tr("Some attachments of "
		    "message \"%1\" were not saved to disk!").arg(dmId));
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
		QMessageBox::warning(this,
		    tr("Error saving attachments of message '%1'.").arg(dmId),
		    warnMsg, QMessageBox::Ok);
	} else {
		showStatusTextWithTimeout(tr("All attachments of "
		    "message \"%1\" were saved.").arg(dmId));
	}
}


/* ========================================================================= */
/*
 * Open attachment in default application.
 */
void MainWindow::openSelectedAttachment(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex selectedIndex =
	    ui->messageAttachmentList->selectionModel()->currentIndex();
	    /* selection().indexes() ? */

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return;
	}

	QModelIndex fileNameIndex =
	    selectedIndex.sibling(selectedIndex.row(), 3);
	Q_ASSERT(fileNameIndex.isValid());
	if(!fileNameIndex.isValid()) {
		return;
	}
	QString attachName = fileNameIndex.data().toString();
	Q_ASSERT(!attachName.isEmpty());
	if (attachName.isEmpty()) {
		return;
	}
	/* TODO -- Add message id into file name? */
	QString fileName = TMP_ATTACHMENT_PREFIX + attachName;

	/* Get data from base64. */
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(), 2);
	Q_ASSERT(dataIndex.isValid());
	if (!dataIndex.isValid()) {
		return;
	}

	QByteArray data =
	    QByteArray::fromBase64(dataIndex.data().toByteArray());

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Attachment '%1' stored to "
		    "temporary file '%2'.").arg(attachName).arg(fileName));
		QDesktopServices::openUrl(QUrl("file:///" + fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		showStatusTextWithTimeout(tr("Attachment '%1' couldn't be "
		    "stored to temporary file.").arg(attachName));
		QMessageBox::warning(this,
		    tr("Error opening attachment."),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Clear status bar if download of complete message fails.
 */
void MainWindow::clearInfoInStatusBarAndShowDialog(qint64 msgId,
    const QString &errMsg)
/* ========================================================================= */
{
	debugSlotCall();

	QMessageBox msgBox(this);

	if (msgId == -1) {
		showStatusTextWithTimeout(tr("It was not possible download "
		    "received message list from ISDS server."));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Download message list error"));
		msgBox.setText(tr("It was not possible download "
		    "received message list from ISDS server."));
		if (!errMsg.isEmpty()) {
			msgBox.setInformativeText(tr("ISDS: ") + errMsg);
		} else {
			msgBox.setInformativeText(tr("A connection error "
			    "occured."));
		}
	} else if (msgId == -2) {
		showStatusTextWithTimeout(tr("It was not possible download "
		    "sent message list from ISDS server."));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Download message list error"));
		msgBox.setText(tr("It was not possible download "
		    "sent message list from ISDS server."));
		if (!errMsg.isEmpty()) {
			msgBox.setInformativeText(tr("ISDS: ") + errMsg);
		} else {
			msgBox.setInformativeText(tr("A connection error "
			    "occured."));
		}
	} else {
		showStatusTextWithTimeout(tr("It was not possible download "
		    "complete message \"%1\" from ISDS server.").arg(msgId));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Download message error"));
		msgBox.setText(tr("It was not possible to download a complete "
		    "message \"%1\" from server Datové schránky.").arg(msgId));
		if (!errMsg.isEmpty()) {
			msgBox.setInformativeText(tr("ISDS: ") + errMsg);
		} else {
			msgBox.setInformativeText(tr("A connection error "
			    "occured or the message has already been deleted "
			    "from the server."));
		}
	}
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}


/* ========================================================================= */
/*
 * Set tablewidget when message download worker is done.
 */
void MainWindow::postDownloadSelectedMessageAttachments(
    const QString &userName, qint64 dmId)
/* ========================================================================= */
{
	debugSlotCall();

	showStatusTextWithTimeout(tr("Message \"%1\" "
	    " was downloaded from ISDS server.").arg(dmId));

	QModelIndex acntTopIdx =
	    m_accountModel.indexFromItem(itemFromUserName(userName));
	if (!acntTopIdx.isValid()) {
		Q_ASSERT(0);
		return;
	}

	QModelIndex accountTopIndex =
	    AccountModel::indexTop(ui->accountList->currentIndex());
	if (!accountTopIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	/* Do nothing if account index was changed. */
	if (accountTopIndex != acntTopIdx) {
		return;
	}

	DbMsgsTblModel *messageModel = (DbMsgsTblModel *)
	    m_messageListProxyModel.sourceModel();
	Q_ASSERT(0 != messageModel);
	QModelIndex msgIdIdx;
	/* Find corresponding message in model. */
	for (int row = 0; row < messageModel->rowCount(); ++row) {
		QModelIndex index = messageModel->index(row, 0);
		if (index.data().toLongLong() == dmId) {
			msgIdIdx = index;
			break;
		}
	}

	if (!msgIdIdx.isValid()) {
		return;
	}

	/*
	 * Mark message as having attachment downloaded without reloading
	 * the whole model.
	 */
	messageModel->overrideDownloaded(dmId, true);
	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();
	/* Inform the view that the model has changed. */
	emit messageModel->dataChanged(
	    msgIdIdx.sibling(msgIdIdx.row(), 0),
	    msgIdIdx.sibling(msgIdIdx.row(), messageModel->columnCount() - 1));
	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	/*
	 * TODO -- Create a separate function for reloading attachment
	 * contents. Similar code is used for handling message selection
	 * changes.
	 */

	/* Disconnect model from slot if model already set. */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* New model hasn't been set yet. */
		ui->messageAttachmentList->selectionModel()->disconnect(
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemCurrentChanged(QModelIndex,
		        QModelIndex)));
	}

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);
	QDateTime deliveryTime = msgDeliveryTime(msgIdIdx);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(dmId,
	    ui->verifySignature));
	ui->messageInfo->setReadOnly(true);

	QAbstractTableModel *fileTblMdl = messageDb->flsModel(dmId);
	Q_ASSERT(0 != fileTblMdl);
	ui->messageAttachmentList->setModel(fileTblMdl);
	/* First three columns contain hidden data. */
	ui->messageAttachmentList->setColumnHidden(0, true);
	ui->messageAttachmentList->setColumnHidden(1, true);
	ui->messageAttachmentList->setColumnHidden(2, true);

	if (ui->messageAttachmentList->model()->rowCount() > 0) {
		ui->saveAttachments->setEnabled(true);
		ui->actionSave_all_attachments->setEnabled(true);
	} else {
		ui->saveAttachments->setEnabled(false);
		ui->actionSave_all_attachments->setEnabled(false);
	}

	ui->messageAttachmentList->resizeColumnToContents(3);

	/* Connect new slot. */
	connect(ui->messageAttachmentList->selectionModel(),
	    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
	    SLOT(attachmentItemCurrentChanged(QModelIndex, QModelIndex)));
}


/* ========================================================================= */
/*
 * Mark all received messages in the current working account.
 */
void MainWindow::accountMarkReceivedLocallyRead(bool read)
/* ========================================================================= */
{
	debugFuncCall();

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QModelIndex acntIdx =
	    ui->accountList->selectionModel()->currentIndex();

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

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QModelIndex acntIdx =
	    ui->accountList->selectionModel()->currentIndex();
	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(acntIdx);
	/*
	 * Data cannot be read directly from index because to the overloaded
	 * model functions.
	 * TODO -- Parameter check.
	 */
	dbSet->smsgdtSetReceivedYearLocallyRead(accountItem->text(), read);

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

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QModelIndex acntIdx =
	    ui->accountList->selectionModel()->currentIndex();

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

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QModelIndex acntIdx =
	    ui->accountList->selectionModel()->currentIndex();

	dbSet->msgSetAllReceivedProcessState(state);

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

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QModelIndex acntIdx =
	    ui->accountList->selectionModel()->currentIndex();
	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(acntIdx);
	/*
	 * Data cannot be read directly from index because to the overloaded
	 * model functions.
	 * TODO -- Parameter check.
	 */
	dbSet->smsgdtSetReceivedYearProcessState(accountItem->text(), state);

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

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QModelIndex acntIdx =
	    ui->accountList->selectionModel()->currentIndex();

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

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	messageItemsSetReadStatus(firstMsgColumnIdxs, true);
}


/* ========================================================================= */
/*
 * Mark selected messages as unread.
 */
void MainWindow::messageItemsSelectedMarkUnread(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	messageItemsSetReadStatus(firstMsgColumnIdxs, false);
}


/* ========================================================================= */
/*
 * Mark selected messages as unsettled.
 */
void MainWindow::messageItemsSelectedMarkUnsettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	messageItemsSetProcessStatus(firstMsgColumnIdxs, UNSETTLED);
}


/* ========================================================================= */
/*
 * Mark selected messages as in progress.
 */
void MainWindow::messageItemsSelectedMarkInProgress(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	messageItemsSetProcessStatus(firstMsgColumnIdxs, IN_PROGRESS);
}


/* ========================================================================= */
/*
 * Mark selected messages as settled.
 */
void MainWindow::messageItemsSelectedMarkSettled(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	messageItemsSetProcessStatus(firstMsgColumnIdxs, SETTLED);
}


/* ========================================================================= */
/*
 * Delete selected message from local database and ISDS.
 */
void MainWindow::deleteMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	if (!acntTopIdx.isValid()) {
		return;
	}

	const QString userName =
	    acntTopIdx.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

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
		    "from ISDS then these message will be lost forever.");
	}

	QDialog *yesNoCheckDlg = new YesNoCheckboxDialog(dlgTitleText,
	    questionText, checkBoxText, detailText, this);
	int retVal = yesNoCheckDlg->exec();
	bool delMsgIsds = false;

	if (retVal == YesNoCheckboxDialog::YesChecked) {
		/* Delete message(s) in the local db and ISDS */
		delMsgIsds = true;
	} else if (retVal == YesNoCheckboxDialog::YesUnchecked) {
		/* Delete message(s) only local */
		delMsgIsds = false;
	} else {
		/* Cancel delete action */
		return;
	}

	QList<MessageDb::MsgId> msgIds;
	foreach (const QModelIndex &idx, firstMsgColumnIdxs) {
		msgIds.append(MessageDb::MsgId(idx.data().toLongLong(),
		    msgDeliveryTime(idx)));
	}

	/* Save current account index */
	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();

	foreach (const MessageDb::MsgId &id, msgIds) {
		switch (eraseMessage(userName, id.dmId, id.deliveryTime,
		            delMsgIsds)) {
		case Q_SUCCESS:
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
			break;
		default:
			break;
		}
	}
}


/* ========================================================================= */
/*
 * Delete message from long term storage in ISDS and
 * local database - based on action parameter.
*/
qdatovka_error MainWindow::eraseMessage(const QString &userName, qint64 dmId,
    const QDateTime &deliveryTime, bool delFromIsds)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	if (!delFromIsds) {
		if (messageDb->msgsDeleteMessageData(dmId)) {
			qDebug() << "Message" << dmId <<
			    "was deleted from local database";
			showStatusTextWithTimeout(tr("Message \"%1\" was "
			    "deleted from local database.").arg(dmId));
			return Q_SUCCESS;
		}
	} else {

		isds_error status;
		if (!isdsSessions.isConnectedToIsds(userName)) {
			if (!connectToIsds(userName, this)) {
				return Q_CONNECT_ERROR;
			}
		}

		bool incoming = true;
		QModelIndex acntIdx = ui->accountList->
		    selectionModel()->currentIndex();

		switch (AccountModel::nodeType(acntIdx)) {
		case AccountModel::nodeRecentReceived:
		case AccountModel::nodeReceived:
		case AccountModel::nodeReceivedYear:
			incoming = true;
			break;
		case AccountModel::nodeRecentSent:
		case AccountModel::nodeSent:
		case AccountModel::nodeSentYear:
			incoming = false;
			break;
		default:
			break;
		}
		/* first delete message on ISDS */
		status = isds_delete_message_from_storage(
		    isdsSessions.session(userName),
		    QString::number(dmId).toUtf8().constData(), incoming);

		if (IE_SUCCESS == status) {
			if (messageDb->msgsDeleteMessageData(dmId)) {
				qDebug() << "Message" << dmId <<
				    "was deleted from ISDS and local databse";
				showStatusTextWithTimeout(tr("Message \"%1\" "
				    "was deleted from ISDS and local database.")
				    .arg(dmId));
				return Q_SUCCESS;
			} else {
				qDebug() << "Message" << dmId <<
				    "was deleted only from ISDS.";
				showStatusTextWithTimeout(tr("Message \"%1\" "
				    "was deleted only from ISDS.").arg(dmId));
				return Q_SQL_ERROR;
			}
		} else if (IE_INVAL == status) {
			qDebug() << "Error: "<< status << isds_strerror(status);
			if (messageDb->msgsDeleteMessageData(dmId)) {
				qDebug() << "Message" << dmId <<
				    "was deleted only from local database.";
				showStatusTextWithTimeout(tr("Message \"%1\" "
				    "was deleted only from local database.")
				    .arg(dmId));
				return Q_ISDS_ERROR;
			}
		}
	}

	qDebug() << "Message" << dmId << "was not deleted.";
	showStatusTextWithTimeout(tr("Message \"%1\" was not deleted.")
	    .arg(dmId));
	return Q_ISDS_ERROR;
}


/* ========================================================================= */
/*
* Set info status bar from worker.
*/
void MainWindow::dataFromWorkerToStatusBarInfo(bool add,
    int rt, int rn, int st, int sn)
/* ========================================================================= */
{
	debugSlotCall();

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

	/*
	 * TODO -- The actual work (function) which the worker performs should
	 * be defined somewhere outside of the worker object.
	 */

	showStatusTextPermanently(
	    tr("Synchronise all accounts with ISDS server."));

	if (globPref.download_on_background) {
		m_timerSyncAccounts.stop();
	}

	int accountCount = ui->accountList->model()->rowCount();
	bool appended = false;

	for (int i = 0; i < accountCount; ++i) {

		QModelIndex index = m_accountModel.index(i, 0);
		bool isConnectActive = true;

		const QString userName =
		    index.data(ROLE_ACNT_USER_NAME).toString();
		Q_ASSERT(!userName.isEmpty());

		/* Skip those that should omitted. */
		if (!AccountModel::globAccounts[userName].syncWithAll()) {
			continue;
		}

		/* Try connecting to ISDS, just to generate log-in dialogue. */
		if (!isdsSessions.isConnectedToIsds(userName)) {
			isConnectActive = connectToIsds(userName, this);
		}

		if (isConnectActive) {
			MessageDbSet *dbSet = accountDbSet(userName, this);
			if (0 == dbSet) {
				continue;
			}

			Worker::jobList.append(Worker::Job(userName, dbSet,
			    MSG_RECEIVED));
			Worker::jobList.append(Worker::Job(userName, dbSet,
			    MSG_SENT));

			appended = true;
		}

	}

	if (!appended) {
		showStatusTextWithTimeout(tr("No account synchronised."));
		if (globPref.download_on_background) {
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
		}
		return;
	}

	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionReceived_all->setEnabled(false);
	ui->actionDownload_messages->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);

	processPendingWorkerJobs();
}


/* ========================================================================= */
/*
* Download sent/received message list for current (selected) account
*/
void MainWindow::synchroniseSelectedAccount(void)
/* ========================================================================= */
{
	debugSlotCall();

	/*
	 * TODO -- Save/restore the position of selected account and message.
	 */

	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);
	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	if (0 == dbSet) {
		return;
	}

	/* Try connecting to ISDS, just to generate log-in dialogue. */
	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return;
		}
	}

	Worker::jobList.append(Worker::Job(userName, dbSet, MSG_RECEIVED));
	Worker::jobList.append(Worker::Job(userName, dbSet, MSG_SENT));

	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionReceived_all->setEnabled(false);
	ui->actionDownload_messages->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);

	processPendingWorkerJobs();
}


/* ========================================================================= */
/*
 * Downloads the attachments for the selected message.
 */
void MainWindow::downloadSelectedMessageAttachments(void)
/* ========================================================================= */
{
	debugSlotCall();

	enum MessageDirection msgDirection = MSG_RECEIVED;
	QModelIndex accountTopIndex;

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	QList<MessageDb::MsgId> msgIds;
	foreach (const QModelIndex &idx, firstMsgColumnIdxs) {
		msgIds.append(MessageDb::MsgId(idx.data().toLongLong(),
		    msgDeliveryTime(idx)));
	}

	{
		QModelIndex accountIndex =
		    ui->accountList->selectionModel()->currentIndex();
		Q_ASSERT(accountIndex.isValid());

		switch (AccountModel::nodeType(accountIndex)) {
		case AccountModel::nodeRecentReceived:
		case AccountModel::nodeReceived:
		case AccountModel::nodeReceivedYear:
			msgDirection = MSG_RECEIVED;
			break;
		case AccountModel::nodeRecentSent:
		case AccountModel::nodeSent:
		case AccountModel::nodeSentYear:
			msgDirection = MSG_SENT;
			break;
		default:
			break;
		}

		accountTopIndex = AccountModel::indexTop(accountIndex);
	}

	/* Try connecting to ISDS, just to generate log-in dialogue. */
	const QString userName =
	    accountTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return;
		}
	}

	foreach (const MessageDb::MsgId &id, msgIds) {
		/* Using prepend() just to outrun other jobs. */
		Worker::jobList.append(
		    Worker::Job(userName, dbSet, msgDirection, id.dmId, id.deliveryTime));
	}

	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionReceived_all->setEnabled(false);
	ui->actionDownload_messages->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);

	processPendingWorkerJobs();
}


/* ========================================================================= */
/*
 * Process pending worker jobs.
 */
void MainWindow::processPendingWorkerJobs(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* Only if no other worker is present. */
	if ((0 != m_syncAcntThread) || (0 != m_syncAcntWorker)) {
		qDebug() << "Worker already doing something.";
		return;
	}

	/* Only if no other worker is present. */

	Worker::Job job = Worker::jobList.firstPop(false);
	if (!job.isValid()) {
		/* TODO -- Re-enable buttons? */
		return;
	}

	showStatusTextPermanently(
	    tr("Synchronise account \"%1\" with ISDS server.")
	        .arg(AccountModel::globAccounts[job.userName].accountName()));

	if (!isdsSessions.isConnectedToIsds(job.userName)) {
		if (!connectToIsds(job.userName, this)) {
			return;
		}
	}

	m_syncAcntThread = new QThread();
	m_syncAcntWorker = new Worker();
	m_syncAcntWorker->moveToThread(m_syncAcntThread);

	connect(m_syncAcntWorker, SIGNAL(valueChanged(QString, int)),
	    this, SLOT(setProgressBarFromWorker(QString, int)));
	{
		/* Downloading message list. */
		connect(m_syncAcntWorker,
		    SIGNAL(changeStatusBarInfo(bool,
		        int, int, int, int)),
		    this,
		    SLOT(dataFromWorkerToStatusBarInfo(bool,
		        int, int, int, int)));
		connect(m_syncAcntWorker,
		    SIGNAL(refreshAccountList(const QString)),
		    this,
		    SLOT(refreshAccountListFromWorker(const QString)));
	}
	{
		/* Downloading attachment. */
		connect(m_syncAcntWorker,
		    SIGNAL(refreshAttachmentList(const QString, qint64)),
		    this, SLOT(postDownloadSelectedMessageAttachments(
		        const QString, qint64)));
		connect(m_syncAcntWorker,
		    SIGNAL(clearStatusBarAndShowDialog(qint64, QString)),
		    this, SLOT(clearInfoInStatusBarAndShowDialog(qint64,
		    QString)));
	}
	connect(m_syncAcntWorker, SIGNAL(workRequested()),
	    m_syncAcntThread, SLOT(start()));
	connect(m_syncAcntThread, SIGNAL(started()),
	    m_syncAcntWorker, SLOT(doJob()));
	connect(m_syncAcntWorker, SIGNAL(finished()),
	    m_syncAcntThread, SLOT(quit()), Qt::DirectConnection);
	connect(m_syncAcntThread, SIGNAL(finished()),
	    this, SLOT(endCurrentWorkerJob()));

	m_syncAcntWorker->requestWork();
}


/* ========================================================================= */
/*
 * End current worker job.
 */
void MainWindow::endCurrentWorkerJob(void)
/* ========================================================================= */
{
	debugSlotCall();

	qDebug() << "Deleting Worker and Thread objects.";

	delete m_syncAcntThread; m_syncAcntThread = NULL;
	delete m_syncAcntWorker; m_syncAcntWorker = NULL;

	if (Worker::jobList.firstPop(false).isValid()) {
		/* Queue still contains pending jobs. */
		processPendingWorkerJobs();
	} else {
		int accountCount = ui->accountList->model()->rowCount();
		if (accountCount > 0) {
			ui->actionSync_all_accounts->setEnabled(true);
			ui->actionReceived_all->setEnabled(true);
			ui->actionDownload_messages->setEnabled(true);
			ui->actionGet_messages->setEnabled(true);
		}
		/* Prepare cunters for next action. */
		dataFromWorkerToStatusBarInfo(false, 0, 0, 0, 0);

		if (globPref.download_on_background) {
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
		}
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
	UserEntry userEntry;
	AccountEntry accountEntry;

	html.append(indentDivStart);
	html.append("<h3>");
	if (AccountModel::globAccounts[userName].isTestAccount()) {
		html.append(tr("Test account"));
	} else {
		html.append(tr("Standard account"));
	}
	html.append("</h3>");

	html.append(strongAccountInfoLine(tr("Account name"),
	    AccountModel::globAccounts[userName].accountName()));

	const QString acndDbKey = userName + "___True";
	if (globAccountDbPtr->dbId(acndDbKey).isEmpty()) {
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

	userEntry = globAccountDbPtr->userEntry(acndDbKey);
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
					html.append(strongAccountInfoLine(
					    userinfTbl.attrProps[key].desc,
					    convertUserPrivilsToString(userEntry.
					    value(key).toInt())));
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
					    authorTypeToText(
					    userEntry.value(key).toString())));
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

	accountEntry = globAccountDbPtr->accountEntry(acndDbKey);
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
					    getdbStateText(
					    accountEntry.value(key).toInt())));
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

	QString info = globAccountDbPtr->getPwdExpirFromDb(acndDbKey);
	if (info.isEmpty()) {
		info = tr("unknown or without expiration");
	} else {
		info = info.split(".")[0];
	}

	html.append(strongAccountInfoLine(tr("Password expiration date"),
	    info));

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
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
	html.append ("<h3>" + accountName + "</h3>");

	html.append(strongAccountInfoLine(tr("Received messages"), ""));
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

	html.append(strongAccountInfoLine(tr("Sent messages"), ""));
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
	html.append ("<h3>" + accountName + "</h3>");

	if (type == MessageDb::TYPE_RECEIVED) {
		html.append(strongAccountInfoLine(tr("Received messages"), ""));
	} else {
		html.append(strongAccountInfoLine(tr("Sent messages"), ""));
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


/* ========================================================================= */
/*
 * Returns user name related to given account item.
 */
QString MainWindow::userNameFromItem(const QStandardItem *accountItem) const
/* ========================================================================= */
{
	if (0 == accountItem) {
		accountItem = m_accountModel.itemFromIndex(
		    ui->accountList->selectionModel()->currentIndex());
	}

	accountItem = AccountModel::itemTop(accountItem);
	Q_ASSERT(0 != accountItem);

	const QString userName =
	    accountItem->data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	return userName;
}


/* ========================================================================= */
/*
 * Returns pointer to account item related to given user name.
 */
QStandardItem *MainWindow::itemFromUserName(const QString &userName) const
/* ========================================================================= */
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return 0;
	}

	QStandardItem *item;
	int topItemCount = m_accountModel.rowCount();
	for (int i = 0; i < topItemCount; ++i) {
		item = m_accountModel.item(i, 0);
		if (item->data(ROLE_ACNT_USER_NAME).toString() == userName) {
			return item;
		}
	}

	return 0;
}


/* ========================================================================= */
/*
 * Get message db set related to given account.
 */
MessageDbSet * MainWindow::accountDbSet(const QString &userName,
    MainWindow *mw)
/* ========================================================================= */
{
	MessageDbSet *dbSet = NULL;
	int flags, dbPresenceCode;

	Q_ASSERT(!userName.isEmpty());

	/* Get user name and db location. */
	AccountModel::SettingsMap &itemSettings =
	    AccountModel::globAccounts[userName];

	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}

	flags = 0;
	if (itemSettings.isTestAccount()) {
		flags |= MDS_FLG_TESTING;
	}
	if (itemSettings._createdFromScratch()) {
		/* Check database structure on account creation. */
		flags |= MDS_FLG_CHECK_QUICK;
	}
	dbPresenceCode =
	    MessageDbSet::checkExistingDbFile(dbDir, userName, flags);

	switch (dbPresenceCode) {
	case MDS_ERR_OK:
		{
			if (itemSettings._createdFromScratch()) {
				/* Notify the user on account creation. */
				QStringList dbFileNames(
				    MessageDbSet::existingDbFileNamesInLocation(
				        dbDir, userName,
				        itemSettings.isTestAccount(),
				        MessageDbSet::DO_UNKNOWN, true));
				Q_ASSERT(!dbFileNames.isEmpty());
				QString namesStr("'" + dbFileNames[0] + "'");
				for (int i = 1; i < dbFileNames.size(); ++i) {
					namesStr += ", '" + dbFileNames[i] + "'";
				}
				logInfo("Database files %s for user name "
				    "'%s' already present in '%s'.\n",
				    namesStr.toUtf8().constData(),
				    userName.toUtf8().constData(),
				    dbDir.toUtf8().constData());
				if (0 != mw) {
					QMessageBox::information(mw,
					    tr("Datovka: Database file present"),
					    tr("Database file for account '%1' "
					        "already exists.").arg(userName) +
					    "\n\n" +
					    tr("The existing database files %1 in '%2' are "
					        "going to be used.").arg(namesStr).arg(dbDir) +
					    "\n\n" +
					    tr("If you want to use a new blank file "
					        "then delete, rename or move the "
					        "existing file so that the "
					        "application can create a new empty "
					        "file."),
					    QMessageBox::Ok);
				}
			}
			dbSet = globMessageDbsPtr->accessDbSet(dbDir, userName,
			    itemSettings.isTestAccount(),
			    MessageDbSet::DO_UNKNOWN, false);
		}
		break;
	case MDS_ERR_MISSFILE:
		{
			if (!itemSettings._createdFromScratch()) {
				/* Not on account creation. */
				logWarning("Missing database files for "
				    "user name '%s' in '%s'.\n",
				    userName.toUtf8().constData(),
				    dbDir.toUtf8().constData());
				if (0 != mw) {
					QMessageBox::warning(mw,
					    tr("Datovka: Problem loading database"),
					    tr("Could not load data from the database "
					        "for account '%1'").arg(userName) +
					    "\n\n" +
					    tr("Database files are missing in '%1'.").arg(
					        dbDir) +
					    "\n\n" +
					    tr("I'll try to create an empty one."),
					    QMessageBox::Ok);
				}
			}
			dbSet = globMessageDbsPtr->accessDbSet(dbDir, userName,
			    itemSettings.isTestAccount(),
			    MessageDbSet::DO_SINGLE_FILE, true);
		}
		break;
	case MDS_ERR_NOTAFILE:
		{
			/* Notify the user that the location is not a file. */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			QString namesStr("'" + dbFileNames[0] + "'");
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarning("Some databases of %s in '%s' related to "
			    "user name '%s' are not a file.\n",
			    namesStr.toUtf8().constData(),
			    dbDir.toUtf8().constData(),
			    userName.toUtf8().constData());
			if (0 != mw) {
				QMessageBox::warning(mw,
				    tr("Datovka: Problem loading database"),
				    tr("Could not load data from the database "
				        "for account '%1'").arg(userName) +
				    "\n\n" +
				    tr("Some databases of %1 in '%2' are not a file."
				        ).arg(namesStr).arg(dbDir),
				    QMessageBox::Ok);
			}
		}
		break;
	case MDS_ERR_ACCESS:
		{
			/* Notify that the user does not have enough rights. */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			QString namesStr("'" + dbFileNames[0] + "'");
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarning("Some databases of '%s' in '%s' related to "
			    "user name '%s' cannot be accessed.\n",
			    namesStr.toUtf8().constData(),
			    dbDir.toUtf8().constData(),
			    userName.toUtf8().constData());
			if (0 != mw) {
				QMessageBox::warning(mw,
				    tr("Datovka: Problem loading database"),
				    tr("Could not load data from the database "
				        "for account '%1'").arg(userName) +
				    "\n\n" +
				    tr("Some databases of '%1' in '%2' cannot be accessed."
				        ).arg(namesStr).arg(dbDir) +
				    "\n\n" +
				    tr("You don't have enough access rights to use "
				        "the file."),
				    QMessageBox::Ok);
			}
		}
		break;
	case MDS_ERR_CREATE:
		{
			/* This error should not be returned. */
			Q_ASSERT(0);
		}
		break;
	case MDS_ERR_DATA:
		{
			/*
			 * Database file is not a database file or is
			 * corrupted.
			 */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			QString namesStr("'" + dbFileNames[0] + "'");
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarning("Some databases of %s in '%s' related to "
			    "user name '%s' is probably corrupted.\n",
			    namesStr.toUtf8().constData(),
			    dbDir.toUtf8().constData(),
			    userName.toUtf8().constData());
			if (0 != mw) {
				QMessageBox::warning(mw,
				    tr("Datovka: Problem loading database"),
				    tr("Could not load data from the database "
				        "for account '%1'").arg(userName) +
				    "\n\n" +
				    tr("Some databases of %1 in '%2' cannot be used."
				        ).arg(namesStr).arg(dbDir) +
				    "\n\n" +
				    tr("The file either does not contain an sqlite "
				        "database or the file is corrupted."),
				    QMessageBox::Ok);
			}
		}
		break;
	case MDS_ERR_MULTIPLE:
		{
			/*
			 * Multiple atabase organisation types resite in the
			 * same location.
			 */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			QString namesStr("'" + dbFileNames[0] + "'");
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarning("Multiple databases %s for '%s' have been "
			    "encountered in the location '%s'.\n",
			    namesStr.toUtf8().constData(),
			    userName.toUtf8().constData(),
			    dbDir.toUtf8().constData());
			if (0 != mw) {
				QMessageBox::warning(mw,
				    tr("Datovka: Problem loading database"),
				    tr("Could not load data from the database "
				        "for account '%1'").arg(userName) +
				    "\n\n" +
				    tr("Conflicting databases %1 in '%2' cannot be used."
				        ).arg(namesStr).arg(dbDir) +
				    "\n\n" +
				    tr("Please remove the conflicting files."),
				    QMessageBox::Ok);
			}
		}
		break;
	default:
		/* The code should not end here. */
		Q_ASSERT(0);
		break;
	}

	if (itemSettings._createdFromScratch()) {
		/* Notify only once. */
		itemSettings._setCreatedFromScratch(false);
	}

	/*
	 * TODO -- Give the user some recovery options such as
	 * move/rename/remove the corrupted file or remove/ignore the affected
	 * account.
	 */

	if (NULL == dbSet) {
		/*
		 * TODO -- generate notification dialogue and give the user
		 * a choice between aborting program and skipping account?
		 */
		logError("Database files for user name '%s' in '%s' cannot be "
		    "created or is probably corrupted.\n",
		    userName.toUtf8().constData(),
		    dbDir.toUtf8().constData());
		if (0 != mw) {
			QMessageBox::critical(mw,
			    tr("Datovka: Database opening error"),
			    tr("Could not load data from the database "
			        "for account '%1'").arg(userName) +
			    "\n\n" +
			    tr("Database files in '%1' cannot be created or are "
			        "corrupted.").arg(dbDir),
			    QMessageBox::Ok);
		}
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

	const AccountModel::SettingsMap &itemSettings =
	    AccountModel::globAccounts[userName];

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
 * Create configuration file if not present.
 */
bool MainWindow::ensureConfPresence(void)
/* ========================================================================= */
{
	if (!QDir(globPref.confDir()).exists()) {
		if (!QDir(globPref.confDir()).mkpath(".")) {
			return false;
		}
	}
	if (!QFile(globPref.loadConfPath()).exists()) {
		QFile file(globPref.loadConfPath());
		if (!file.open(QIODevice::ReadWrite)) {
			return false;
		}
		file.close();
	}

	return true;
}


#define W_OFFS 2
#define H_OFFS 22
#define SH_OFFS 50 /* Menu bar + top tool-bar. */
/* ========================================================================= */
/*
 * Sets geometry from settings.
 */
void MainWindow::loadWindowGeometry(const QSettings &settings)
/* ========================================================================= */
{
	/* Window geometry. */

	int x = settings.value("window_position/x", 0).toInt() + W_OFFS;
	int y = settings.value("window_position/y", 0).toInt() + H_OFFS;
	int w = settings.value("window_position/w", 800).toInt();
	int h = settings.value("window_position/h", 600).toInt();
	this->setGeometry(x, y, w, h);

	/* Splitter geometry. */

	// set mainspliter - hSplitterAccount
	QList<int> sizes = ui->hSplitterAccount->sizes();
	int tmp = settings.value("panes/hpaned1", 226).toInt();
	sizes[0] = tmp;
	sizes[1] = w - sizes[0];;
	ui->hSplitterAccount->setSizes(sizes);

	// set messagelistspliter - vSplitterMessage
	sizes = ui->vSplitterMessage->sizes();
	sizes[0] = settings.value("panes/message_pane", 265).toInt();
	sizes[1] = h - SH_OFFS - sizes[0];
	ui->vSplitterMessage->setSizes(sizes);

	// set message/mesageinfospliter - hSplitterMessageInfo
	sizes = ui->hSplitterMessageInfo->sizes();
	sizes[0] = settings.value("panes/message_display_pane", 505).toInt();
	sizes[1] = w - tmp - sizes[0];
	ui->hSplitterMessageInfo->setSizes(sizes);
}


/* ========================================================================= */
/*
 * Set default account from settings.
 */
void MainWindow::setDefaultAccount(const QSettings &settings)
/* ========================================================================= */
{
	debugFuncCall();

	QString username = settings.value("default_account/username", "")
	   .toString();
	if (!username.isEmpty()) {
		int topItemCount = m_accountModel.rowCount();
		for (int i = 0; i < topItemCount; i++) {
			const QStandardItem *item = m_accountModel.item(i,0);
			const QString user =
			    item->data(ROLE_ACNT_USER_NAME).toString();
			if (user == username) {
				QModelIndex index = m_accountModel.
				    indexFromItem(item);
				ui->accountList->
				    setCurrentIndex(index.child(0,0));
				accountItemCurrentChanged(index.child(0,0));
				ui->menuDatabox->setEnabled(true);
				ui->actionDelete_account->setEnabled(true);
				ui->actionSync_all_accounts->setEnabled(true);
				ui->actionAccount_props->setEnabled(true);
				ui->actionChange_pwd->setEnabled(true);
				ui->actionCreate_message->setEnabled(true);
				ui->actionFind_databox->setEnabled(true);
				ui->actionMsgAdvancedSearch->setEnabled(true);
				ui->actionImport_ZFO_file_into_database->
				    setEnabled(true);
				ui->actionDownload_messages->setEnabled(true);
				ui->actionReceived_all->setEnabled(true);
				break;
			}
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

	/* File. */
	connect(ui->actionSync_all_accounts, SIGNAL(triggered()), this,
	    SLOT(synchroniseAllAccounts()));
	connect(ui->actionAdd_account, SIGNAL(triggered()), this,
	    SLOT(addNewAccount()));
	connect(ui->actionDelete_account, SIGNAL(triggered()), this,
	    SLOT(deleteSelectedAccount()));
	connect(ui->actionImport_database_directory, SIGNAL(triggered()), this,
	    SLOT(showImportDatabaseDialog()));
	connect(ui->actionProxy_settings, SIGNAL(triggered()), this,
	    SLOT(proxySettings()));
	connect(ui->actionPreferences, SIGNAL(triggered()), this,
	    SLOT(applicationPreferences()));
	/* actionQuit -- connected in ui file. */

	/* Databox. */
	connect(ui->actionGet_messages, SIGNAL(triggered()), this,
	    SLOT(synchroniseSelectedAccount()));
	connect(ui->actionSend_message, SIGNAL(triggered()), this,
	    SLOT(createAndSendMessage()));
	connect(ui->actionMark_all_as_read, SIGNAL(triggered()), this,
	    SLOT(accountMarkReceivedRead()));
	connect(ui->actionChange_password, SIGNAL(triggered()), this,
	    SLOT(changeAccountPassword()));
	connect(ui->actionAccount_properties, SIGNAL(triggered()), this,
	    SLOT(manageAccountProperties()));
	connect(ui->actionMove_account_up, SIGNAL(triggered()), this,
	    SLOT(moveSelectedAccountUp()));
	connect(ui->actionMove_account_down, SIGNAL(triggered()), this,
	    SLOT(moveSelectedAccountDown()));
	connect(ui->actionChange_data_directory, SIGNAL(triggered()), this,
	    SLOT(changeDataDirectory()));
	connect(ui->actionImport_messages_from_database, SIGNAL(triggered()),
	    this, SLOT(prepareMsgsImportFromDatabase()));
#ifdef PORTABLE_APPLICATION
	ui->actionChange_data_directory->setEnabled(false);
#endif /* PORTABLE_APPLICATION */

	/* Message. */
	connect(ui->actionDownload_message_signed, SIGNAL(triggered()), this,
	    SLOT(downloadSelectedMessageAttachments()));
	connect(ui->actionReply, SIGNAL(triggered()), this,
	    SLOT(createAndSendMessageReply()));
	connect(ui->actionCreate_message_from_template, SIGNAL(triggered()), this,
	    SLOT(createAndSendMessageFromTmpl()));
	connect(ui->actionSignature_detail, SIGNAL(triggered()), this,
	    SLOT(showSignatureDetails()));
	connect(ui->actionAuthenticate_message, SIGNAL(triggered()), this,
	    SLOT(verifySelectedMessage()));
	connect(ui->actionExport_as_ZFO, SIGNAL(triggered()), this,
	    SLOT(exportSelectedMessageAsZFO()));
	connect(ui->actionOpen_message_externally, SIGNAL(triggered()), this,
	    SLOT(openSelectedMessageExternally()));
	connect(ui->actionOpen_delivery_info_externally, SIGNAL(triggered()), this,
	    SLOT(openDeliveryInfoExternally()));
	connect(ui->actionExport_delivery_info_as_ZFO, SIGNAL(triggered()), this,
	    SLOT(exportDeliveryInfoAsZFO()));
	connect(ui->actionExport_delivery_info_as_PDF, SIGNAL(triggered()), this,
	    SLOT(exportDeliveryInfoAsPDF()));
	connect(ui->actionExport_message_envelope_as_PDF, SIGNAL(triggered()), this,
	    SLOT(exportMessageEnvelopeAsPDF()));
	connect(ui->actionOpen_attachment, SIGNAL(triggered()), this,
	    SLOT(openSelectedAttachment()));
	connect(ui->actionSave_attachment, SIGNAL(triggered()), this,
	    SLOT(saveSelectedAttachmentToFile()));
	connect(ui->actionSave_all_attachments, SIGNAL(triggered()), this,
	    SLOT(saveAllAttachmentsToDir()));
	connect(ui->actionDelete_message_from_db, SIGNAL(triggered()), this,
	    SLOT(deleteMessage()));

	/* Tools. */
	connect(ui->actionFind_databox, SIGNAL(triggered()), this,
	    SLOT(findDatabox()));
	connect(ui->actionAuthenticate_message_file, SIGNAL(triggered()), this,
	    SLOT(authenticateMessageFile()));
	connect(ui->actionView_message_from_ZPO_file, SIGNAL(triggered()), this,
	    SLOT(viewMessageFromZFO()));
	connect(ui->actionExport_correspondence_overview, SIGNAL(triggered()), this,
	    SLOT(exportCorrespondenceOverview()));
	connect(ui->actionImport_ZFO_file_into_database, SIGNAL(triggered()), this,
	    SLOT(showImportZFOActionDialog()));
	connect(ui->actionCheck_message_timestamp_expiration, SIGNAL(triggered()), this,
	    SLOT(showMsgTmstmpExpirDialog()));

	/* Help. */
	connect(ui->actionAbout_Datovka, SIGNAL(triggered()), this,
	    SLOT(aboutApplication()));
	connect(ui->actionHomepage, SIGNAL(triggered()), this,
	    SLOT(goHome()));
	connect(ui->actionHelp, SIGNAL(triggered()), this,
	    SLOT(showHelp()));
}


/* ========================================================================= */
/*
 * Connect top tool-bar buttons to appropriate actions.
 */
void MainWindow::connectTopToolBarSlots(void)
/* ========================================================================= */
{
	debugFuncCall();

	/*
	 * Actions that cannot be automatically connected
	 * via QMetaObject::connectSlotsByName because of mismatching names.
	 */

	connect(ui->actionReceived_all, SIGNAL(triggered()), this,
	    SLOT(synchroniseAllAccounts()));
	connect(ui->actionDownload_messages, SIGNAL(triggered()), this,
	    SLOT(synchroniseSelectedAccount()));
	connect(ui->actionCreate_message, SIGNAL(triggered()), this,
	    SLOT(createAndSendMessage()));
	connect(ui->actionReply_to_the_sender, SIGNAL(triggered()), this,
	    SLOT(createAndSendMessageReply()));
	connect(ui->actionVerify_a_message, SIGNAL(triggered()), this,
	    SLOT(verifySelectedMessage()));
	connect(ui->actionMsgAdvancedSearch, SIGNAL(triggered()), this,
	    SLOT(showMsgAdvancedSearchDlg()));
	connect(ui->actionAccount_props, SIGNAL(triggered()), this,
	    SLOT(manageAccountProperties()));
	connect(ui->actionChange_pwd, SIGNAL(triggered()), this,
	    SLOT(changeAccountPassword()));
	connect(ui->actionPrefs, SIGNAL(triggered()), this,
	    SLOT(applicationPreferences()));
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

	/* Message/attachment related buttons. */
	connect(ui->downloadComplete, SIGNAL(clicked()), this,
	    SLOT(downloadSelectedMessageAttachments()));
	connect(ui->saveAttachment, SIGNAL(clicked()), this,
	    SLOT(saveSelectedAttachmentToFile()));
	connect(ui->saveAttachments, SIGNAL(clicked()), this,
	    SLOT(saveAllAttachmentsToDir()));
	connect(ui->openAttachment, SIGNAL(clicked()), this,
	    SLOT(openSelectedAttachment()));
	/* Downloading attachments also triggers signature verification. */
	connect(ui->verifySignature, SIGNAL(clicked()), this,
	    SLOT(downloadSelectedMessageAttachments()));
	connect(ui->signatureDetails, SIGNAL(clicked()), this,
	    SLOT(showSignatureDetails()));
	/* Sets message processing state. */
	connect(ui->messageStateCombo, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(msgSetSelectedMessageProcessState(int)));
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
	// ToolBar
	ui->actionReceived_all->setEnabled(false);
	ui->actionDownload_messages->setEnabled(false);
	ui->actionCreate_message->setEnabled(false);
	ui->actionReply_to_the_sender->setEnabled(false);
	ui->actionVerify_a_message->setEnabled(false);
	ui->actionAccount_props->setEnabled(false);
	ui->actionChange_pwd->setEnabled(false);
	// Menu: File
	ui->actionDelete_account->setEnabled(false);
	ui->actionSync_all_accounts->setEnabled(false);
	// Menu: Tools
	ui->actionFind_databox->setEnabled(false);
	ui->actionImport_ZFO_file_into_database->setEnabled(false);
	ui->actionMsgAdvancedSearch->setEnabled(false);
	ui->actionAuthenticate_message_file->setEnabled(false);
	ui->actionExport_correspondence_overview->setEnabled(false);
	ui->actionCheck_message_timestamp_expiration->setEnabled(false);
}


/* ========================================================================= */
/*
 *  Set default settings of mainwindow.
 */
void MainWindow::setMessageActionVisibility(bool action) const
/* ========================================================================= */
{
	/* Top menu + menu items. */
	ui->menuMessage->setEnabled(action);
	ui->actionReply->setEnabled(action); /* Has key short cut. */

	/* Top tool bar. */
	ui->actionReply_to_the_sender->setEnabled(action);
	ui->actionVerify_a_message->setEnabled(action);
}


/* ========================================================================= */
/*
 *  Active/Inactive account menu and buttons in the mainwindow.
 */
void MainWindow::activeAccountMenuAndButtons(bool action) const
/* ========================================================================= */
{
	ui->menuDatabox->setEnabled(action);
	ui->actionReceived_all->setEnabled(action);
	ui->actionCreate_message->setEnabled(action);
	ui->actionAccount_properties->setEnabled(action);
	ui->actionDownload_messages->setEnabled(action);
	ui->actionChange_password->setEnabled(action);
	ui->actionSync_all_accounts->setEnabled(action);
	ui->actionDelete_account->setEnabled(action);
	ui->actionFind_databox->setEnabled(action);
	ui->actionMsgAdvancedSearch->setEnabled(action);
	ui->actionImport_ZFO_file_into_database->setEnabled(action);
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

	settings.beginGroup("window_position");

	value = this->geometry().x() - W_OFFS;
	value = (value < 0) ? 0 : value;
	settings.setValue("x", value);

	value = this->geometry().y() - H_OFFS;
	value = (value < 0) ? 0 : value;
	settings.setValue("y", value);

	settings.setValue("w", this->geometry().width());
	settings.setValue("h", this->geometry().height());

	settings.endGroup();

	/* Splitter geometry. */

	settings.beginGroup("panes");

	settings.setValue("hpaned1", ui->hSplitterAccount->sizes()[0]);
	settings.setValue("message_pane", ui->vSplitterMessage->sizes()[0]);
	settings.setValue("message_display_pane",
	    ui->hSplitterMessageInfo->sizes()[0]);

	settings.endGroup();
}
#undef W_OFFS
#undef H_OFFS
#undef SH_OFFS


/* ========================================================================= */
/*
 * Load and apply setting from configuration file.
 */
void MainWindow::loadSettings(void)
/* ========================================================================= */
{
	QSettings settings(globPref.loadConfPath(), QSettings::IniFormat);
	settings.setIniCodec("UTF-8");


	/* Load last directory paths */
	loadLastDirectoryPaths(settings);

	/* Received Sent messages Column widths */
	loadSentReceivedMessagesColumnWidth(settings);

	/* Window geometry. */
	loadWindowGeometry(settings);

	/* Global preferences. */
	globPref.loadFromSettings(settings);

	/* Proxy settings. */
	globProxSet.loadFromSettings(settings);

	/* Accounts. */
	m_accountModel.loadFromSettings(settings);
	ui->accountList->setModel(&m_accountModel);

	/* Select last-used account. */
	setDefaultAccount(settings);

	/* Scan databases. */
	regenerateAllAccountModelYears();

	/* Load collapse info of account items from settings */
	loadAccountCollapseInfo(settings);
}


/* ========================================================================= */
/*
 * Load received/sent messages column widths and sort order from settings.
 */
void MainWindow::loadSentReceivedMessagesColumnWidth(const QSettings &settings)
/* ========================================================================= */
{
	m_received_1 = settings.value("column_widths/received_1", 200).toInt();
	m_received_2 = settings.value("column_widths/received_2", 200).toInt();
	m_sent_1 = settings.value("column_widths/sent_1", 200).toInt();
	m_sent_2 = settings.value("column_widths/sent_2", 200).toInt();
	m_sort_column = settings.value("message_ordering/sort_column",
	    0).toInt();
	/* Sort column saturation from old datovka */
	if (m_sort_column > 5) {
		m_sort_column = 1;
	}
	m_sort_order = settings.value("message_ordering/sort_order",
	    "").toString();
}

/* ========================================================================= */
/*
 * Save received/sent messages column widths and sort order into settings.
 */
void MainWindow::saveSentReceivedColumnWidth(QSettings &settings) const
/* ========================================================================= */
{
	settings.beginGroup("column_widths");
	settings.setValue("received_1", m_received_1);
	settings.setValue("received_2", m_received_2);
	settings.setValue("sent_1", m_sent_1);
	settings.setValue("sent_2", m_sent_2);
	settings.endGroup();

	settings.beginGroup("message_ordering");
	settings.setValue("sort_column", m_sort_column);
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
	QModelIndex index = ui->accountList->currentIndex();
	if (index.isValid()) {
		const QStandardItem *item =
		    m_accountModel.itemFromIndex(index);
		const QStandardItem *itemTop = AccountModel::itemTop(item);

		const QString userName =
		    itemTop->data(ROLE_ACNT_USER_NAME).toString();
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
bool MainWindow::updateExistingAccountModelUnread(QModelIndex index)
/* ========================================================================= */
{
	/*
	 * Several nodes may be updated at once, because some messages may be
	 * referred from multiple nodes.
	 */

	QStandardItem *topItem;
	QList<QString> yearList;
	int unreadMsgs;

	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	/* Get database id. */
	topItem = m_accountModel.itemFromIndex(index);
	Q_ASSERT(0 != topItem);
	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	/* Received. */
	unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_RECEIVED);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Received" << yearList.value(j);
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_RECEIVED,
		    yearList.value(j));
		m_accountModel.updateYear(topItem,
		    AccountModel::nodeReceivedYear, yearList.value(j),
		    unreadMsgs);
	}
	/* Sent. */
	//unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_SENT);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentSent, 0);
	yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Sent" << yearList.value(j);
		//unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_SENT,
		//    yearList.value(j));
		m_accountModel.updateYear(topItem, AccountModel::nodeSentYear,
		    yearList.value(j), 0);
	}
	return true;
}


/* ========================================================================= */
/*
 * Partially regenerates account model according to the database
 *     content.
 */
bool MainWindow::regenerateAccountModelYears(QModelIndex index)
/* ========================================================================= */
{
	debugFuncCall();

	QStandardItem *topItem;
	QList<QString> yearList;
	int unreadMsgs;

	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	m_accountModel.removeYearNodes(index);

	/* Get database id. */
	topItem = m_accountModel.itemFromIndex(index);
	Q_ASSERT(0 != topItem);
	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	/* Received. */
	unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_RECEIVED);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Received" << yearList.value(j);
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_RECEIVED,
		    yearList.value(j));
		m_accountModel.addYear(topItem, AccountModel::nodeReceivedYear,
		    yearList.value(j), unreadMsgs);
	}
	/* Sent. */
	//unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_SENT);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentSent, 0);
	yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Sent" << yearList.value(j);
		//unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_SENT,
		//    yearList.value(j));
		m_accountModel.addYear(topItem, AccountModel::nodeSentYear,
		    yearList.value(j), 0);
	}
	return true;
}


/* ========================================================================= */
/*
 * Regenerates account model according to the database content.
 */
bool MainWindow::regenerateAllAccountModelYears(void)
/* ========================================================================= */
{
	debugFuncCall();

	QStandardItem *itemTop;
	QList<QString> yearList;
	int unreadMsgs;

	m_accountModel.removeAllYearNodes();

	//qDebug() << "Generating years";

	for (int i = 0; i < m_accountModel.rowCount(); ++i) {
		/* Get database ID. */
		itemTop = m_accountModel.item(i, 0);
		Q_ASSERT(0 != itemTop);
		const QString userName =
		    itemTop->data(ROLE_ACNT_USER_NAME).toString();
		Q_ASSERT(!userName.isEmpty());
		MessageDbSet *dbSet = accountDbSet(userName, this);
		if (0 == dbSet) {
			/*
			 * Skip creation of leaves when no database is present.
			 */
			continue;
		}

		/* Received. */
		unreadMsgs = dbSet->msgsUnreadWithin90Days(
		    MessageDb::TYPE_RECEIVED);
		m_accountModel.updateRecentUnread(itemTop,
		    AccountModel::nodeRecentReceived, unreadMsgs);
		yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED,
		    DESCENDING);
		for (int j = 0; j < yearList.size(); ++j) {
			//qDebug() << yearList.value(j);
			unreadMsgs = dbSet->msgsUnreadInYear(
			    MessageDb::TYPE_RECEIVED, yearList.value(j));
			m_accountModel.addYear(itemTop,
			    AccountModel::nodeReceivedYear, yearList.value(j),
			    unreadMsgs);
		}
		/* Sent. */
		//unreadMsgs = dbSet->msgsUnreadWithin90Days(
		//    MessageDb::TYPE_SENT);
		m_accountModel.updateRecentUnread(itemTop,
		    AccountModel::nodeRecentSent, 0);
		yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
		for (int j = 0; j < yearList.size(); ++j) {
			//qDebug() << yearList.value(j);
			//unreadMsgs = dbSet->msgsUnreadInYear(
			//    MessageDb::TYPE_SENT, yearList.value(j));
			m_accountModel.addYear(itemTop,
			    AccountModel::nodeSentYear, yearList.value(j),
			    0);
		}
	}

	return true;
}


/* ========================================================================= */
/*!
 * @brief Store current setting to configuration file.
 */
void MainWindow::saveSettings(void) const
/* ========================================================================= */
{
	debugFuncCall();

	/*
	 * TODO -- Target file name differs from source for testing purposes.
	 */
	QSettings settings(globPref.saveConfPath(), QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	settings.clear();

	/* Store application ID and config format */
	saveAppIdConfigFormat(settings);

	/* Accounts. */
	m_accountModel.saveToSettings(settings);

	/* Store last-used account. */
	saveAccountIndex(settings);

	/* TODO */
	saveSentReceivedColumnWidth(settings);

	/* Window geometry. */
	saveWindowGeometry(settings);

	/* Store account collapses */
	saveAccountCollapseInfo(settings);

	/* Proxy settings. */
	globProxSet.saveToSettings(settings);

	/* Global preferences. */
	globPref.saveToSettings(settings);

	settings.sync();

	/* Remove " symbols from passwords in dsgui.conf */
	removeDoubleQuotesFromAccountPassword(globPref.saveConfPath());
}


/* ========================================================================= */
/*
 * Create new message from selected account.
 */
void MainWindow::createAndSendMessage(void)
/* ========================================================================= */
{
	debugSlotCall();
	openSendMessageDialog(DlgSendMessage::ACT_NEW);
}


/* ========================================================================= */
/*
 * Create reply from selected message.
 */
void MainWindow::createAndSendMessageReply(void)
/* ========================================================================= */
{
	debugSlotCall();
	openSendMessageDialog(DlgSendMessage::ACT_REPLY);
}


/* ========================================================================= */
/*
 * Create message from template (selected message).
 */
void MainWindow::createAndSendMessageFromTmpl(void)
/* ========================================================================= */
{
	debugSlotCall();
	openSendMessageDialog(DlgSendMessage::ACT_NEW_FROM_TMP);
}

/* ========================================================================= */
/*
 * Open send message dialog and send message.
 */
void MainWindow::openSendMessageDialog(int action)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);

	qint64 msgId = -1;
	QDateTime deliveryTime;

	/* if is reply or template, ID of selected message is required */
	if (DlgSendMessage::ACT_REPLY == action ||
	    DlgSendMessage::ACT_NEW_FROM_TMP == action) {
		const QAbstractItemModel *tableModel =
		    ui->messageList->model();
		Q_ASSERT(0 != tableModel);
		QModelIndex index = tableModel->index(
		    ui->messageList->currentIndex().row(), 0);
		msgId = tableModel->itemData(index).first().toLongLong();
		deliveryTime = msgDeliveryTime(index);
	}

	const QString userName =
	    acntTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	const AccountModel::SettingsMap &accountInfo =
	    AccountModel::globAccounts[userName];

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return;
		}
	}

	/* Method connectToIsds() acquires account information. */
	QString dbId = globAccountDbPtr->dbId(userName + "___True");
	Q_ASSERT(!dbId.isEmpty());
	QString senderName =
	    globAccountDbPtr->senderNameGuess(userName + "___True");
	QList<QString> accountData =
	    globAccountDbPtr->getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		return;
	}

	QString dbType = accountData.at(0);
	bool dbEffectiveOVM = (accountData.at(1) == "1") ? true : false;
	bool dbOpenAddressing = (accountData.at(2) == "1") ? true : false;

	showStatusTextWithTimeout(tr("Create and send a message."));

	QString lastAttachAddPath;
	if (globPref.use_global_paths) {
		lastAttachAddPath = globPref.add_file_to_attachments_path;
	} else {
		lastAttachAddPath = accountInfo.lastAttachAddPath();
	}

	QString pdzCredit("0");
	if (dbOpenAddressing) {
		pdzCredit = getPDZCreditFromISDS();
	}

	QDialog *newMessageDialog = new DlgSendMessage(*dbSet, dbId,
	    senderName, (DlgSendMessage::Action) action, msgId, deliveryTime,
	    userName, dbType, dbEffectiveOVM, dbOpenAddressing,
	    lastAttachAddPath, pdzCredit, this);

	if (newMessageDialog->exec() == QDialog::Accepted) {

		showStatusTextWithTimeout(tr("Message from account \"%1\" was "
		    "send.").arg(accountInfo.accountName()));

		/*
		 * Message model must be regenerated to show the sent if
		 * residing on sent messages.
		 *
		 * TODO -- Regenerating year list (as this could add entries).
		 */
		if (selectedAcntIndex.isValid()) {
			switch (AccountModel::nodeType(selectedAcntIndex)) {
			case AccountModel::nodeRecentSent:
			case AccountModel::nodeAll:
			case AccountModel::nodeSent:
			case AccountModel::nodeSentYear:
				accountItemCurrentChanged(selectedAcntIndex);
				break;
			default:
				/* Do nothing. */
				break;
			}
		}
	}

	if (!globPref.use_global_paths) {
		m_add_attach_dir = lastAttachAddPath;
		storeExportPath();
	}

	setDefaultProgressStatus();
}


/* ========================================================================= */
/*
 * Add account action and dialog.
 */
void MainWindow::addNewAccount(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *newAccountDialog = new DlgCreateAccount(QString(),
	   DlgCreateAccount::ACT_ADDNEW, this);

	connect(newAccountDialog,
	    SIGNAL(getAccountUserDataboxInfo(AccountModel::SettingsMap)),
	    this, SLOT(getAccountUserDataboxInfo(AccountModel::SettingsMap)));

	showStatusTextWithTimeout(tr("Create a new account."));

	if (QDialog::Accepted == newAccountDialog->exec()) {
		if (ui->accountList->model()->rowCount() > 0) {
			activeAccountMenuAndButtons(true);
			saveSettings();
		}
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

	const QModelIndex index = ui->accountList->currentIndex();
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	QStandardItem *itemTop = AccountModel::itemTop(item);
	int currentTopRow = itemTop->row();

	if (currentTopRow < 0) {
		return;
	}

	const QString userName = itemTop->data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	const QString accountName =
	    AccountModel::globAccounts[userName].accountName();

	QString dlgTitleText = tr("Remove account ") + itemTop->text();
	QString questionText = tr("Do you want to remove account") + " '" +
	    itemTop->text() + "' (" + userName + ")?";
	QString checkBoxText = tr("Delete also message database from storage");
	QString detailText = tr(
	    "Warning: If you delete the message database then all locally "
	    "accessible messages that are not stored on the ISDS server "
	    "will be lost.");

	QDialog *yesNoCheckDlg = new YesNoCheckboxDialog(dlgTitleText,
	    questionText, checkBoxText, detailText, this);
	int retVal = yesNoCheckDlg->exec();

	switch (retVal) {
	case YesNoCheckboxDialog::YesChecked:
		/* Delete account and its message db set. */
		if (itemTop->hasChildren()) {
			itemTop->removeRows(0, itemTop->rowCount());
		}
		globAccountDbPtr->deleteAccountInfo(userName + "___True");
		ui->accountList->model()->removeRow(currentTopRow);
		if (globMessageDbsPtr->deleteDbSet(dbSet)) {
			showStatusTextWithTimeout(tr("Account '%1' was deleted "
			    "together with message database file.").arg(accountName));
		} else {
			showStatusTextWithTimeout(tr("Account '%1' was deleted "
			    "but its message database was not deleted.").arg(accountName));
		}
		saveSettings();
		break;
	case YesNoCheckboxDialog::YesUnchecked:
		/* Delete account and remove its items from the treeview */
		if (itemTop->hasChildren()) {
			itemTop->removeRows(0, itemTop->rowCount());
		}
		globAccountDbPtr->deleteAccountInfo(userName + "___True");
		ui->accountList->model()->removeRow(currentTopRow);
		showStatusTextWithTimeout(tr("Account '%1' was deleted.")
		    .arg(accountName));
		saveSettings();
		break;
	default:
		break;
	}

	if (ui->accountList->model()->rowCount() < 1) {
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

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return;
		}
	}

	/* Method connectToIsds() acquires account information. */
	const QString dbId = globAccountDbPtr->dbId(userName + "___True");
	Q_ASSERT(!dbId.isEmpty());

	const AccountModel::SettingsMap &accountInfo =
	    AccountModel::globAccounts[userName];

	showStatusTextWithTimeout(tr("Change password of account "
	    "\"%1\".").arg(accountInfo.accountName()));

	QDialog *changePwd = new DlgChangePwd(dbId, userName, this);
	changePwd->exec();
}


/* ========================================================================= */
/*
 * Shows account properties dialog.
 */
void MainWindow::manageAccountProperties(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);
	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	showStatusTextWithTimeout(tr("Change properties of account \"%1\".")
	    .arg(AccountModel::globAccounts[userName].accountName()));

	QDialog *editAccountDialog = new DlgCreateAccount(userName,
	    DlgCreateAccount::ACT_EDIT, this);

	connect(editAccountDialog, SIGNAL(changedAccountProperties(QString)),
	    this, SLOT(updateAccountListEntry(QString)));

	if (QDialog::Accepted == editAccountDialog->exec()) {
		showStatusTextWithTimeout(tr("Account \"%1\" was updated.")
		    .arg(userName));
		saveSettings();
	}
}


/* ========================================================================= */
/*
 * Move selected account up.
 */
void MainWindow::moveSelectedAccountUp(void)
/* ========================================================================= */
{
	debugSlotCall();

	QStandardItemModel *itemModel = qobject_cast<QStandardItemModel *>
	    (ui->accountList->model());
	QModelIndex index = ui->accountList->currentIndex();
	const QStandardItem *item = m_accountModel.itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);
	int currentTopRow = itemTop->row();

	if (currentTopRow == 0) {
		return;
	}

	int newRow = currentTopRow-1;
	QList<QStandardItem *> list = itemModel->takeRow(currentTopRow);
	itemModel->insertRow(newRow, list);
	//ui->accountList->expandAll();
	index = itemModel->indexFromItem(itemTop);
	ui->accountList->setCurrentIndex(index.child(0,0));

	showStatusTextWithTimeout(tr("Account was moved up."));

}

/* ========================================================================= */
/*
 * Move selected account down.
 */
void MainWindow::moveSelectedAccountDown(void)
/* ========================================================================= */
{
	debugSlotCall();

	QStandardItemModel *itemModel = qobject_cast<QStandardItemModel *>
	    (ui->accountList->model());
	QModelIndex index = ui->accountList->currentIndex();
	const QStandardItem *item = m_accountModel.itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);
	int currentTopRow = itemTop->row();
	int topItemCount = m_accountModel.rowCount()-1;

	if (currentTopRow == topItemCount) {
		return;
	}

	int newRow = currentTopRow+1;
	QList<QStandardItem *> list = itemModel->takeRow(currentTopRow) ;
	itemModel->insertRow(newRow, list);
	//ui->accountList->expandAll();
	index = itemModel->indexFromItem(itemTop);
	ui->accountList->setCurrentIndex(index.child(0,0));

	showStatusTextWithTimeout(tr("Account was moved down."));
}


/* ========================================================================= */
/*
 * Change data directory dialog.
 */
void MainWindow::changeDataDirectory(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QModelIndex index = ui->accountList->currentIndex();
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	QStandardItem *itemTop = AccountModel::itemTop(item);

	const QString userName = itemTop->data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	const AccountModel::SettingsMap &itemSettings =
	    AccountModel::globAccounts[userName];

	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}

	showStatusTextWithTimeout(tr("Change data dierctory of account \"%1\".")
	    .arg(itemSettings.accountName()));

	QDialog *change_directory = new DlgChangeDirectory(dbDir, this);

	connect(change_directory, SIGNAL(sentNewPath(QString, QString, QString)),
	    this, SLOT(receiveNewDataPath(QString, QString, QString)));

	change_directory->exec();
}


/* ========================================================================= */
/*
 * Receive and store new account database path. Change data
 *     directory path in settings.
 */
void MainWindow::receiveNewDataPath(QString oldDir, QString newDir,
    QString action)
/* ========================================================================= */
{
	debugSlotCall();

	const QModelIndex index = ui->accountList->currentIndex();
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	item = AccountModel::itemTop(item);

	const QString userName = item->data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	/* Get current settings. */
	AccountModel::SettingsMap &itemSettings =
	    AccountModel::globAccounts[userName];

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	/* Move account database into new directory */
	if ("move" == action) {
		if (dbSet->moveToLocation(newDir)) {
			itemSettings.setDbDir(newDir);
			saveSettings();

			logInfo("Database files for '%s' have been moved from '%s' to '%s'.\n",
			    userName.toUtf8().constData(),
			    oldDir.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' have been successfully moved to\n\n'%2'."
			        ).arg(userName).arg(newDir),
			    QMessageBox::Ok);
		} else {
			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' could not be moved to\n\n'%2'."
			        ).arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}

	/* Copy account database into new directory */
	} else if ("copy" == action) {
		if (dbSet->copyToLocation(newDir)) {
			itemSettings.setDbDir(newDir);
			saveSettings();

			logInfo("Database files for '%s' have been copied from '%s' to '%s'.\n",
			    userName.toUtf8().constData(),
			    oldDir.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' have been successfully copied to\n\n'%2'."
			        ).arg(userName).arg(newDir),
			    QMessageBox::Ok);
		} else {
			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' could not be copied to\n\n'%2'."
			        ).arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}

	/* Create a new account database into new directory */
	} else if ("new" == action) {
		if (dbSet->reopenLocation(newDir,
		        MessageDbSet::DO_SINGLE_FILE)) {
			itemSettings.setDbDir(newDir);
			saveSettings();

			logInfo("Database files for '%s' have been created in '%s'.\n",
			    userName.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("New database files for '%1' have been successfully created in\n\n'%2'."
			        ).arg(userName).arg(newDir),
			    QMessageBox::Ok);
		} else {
			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("New database files for '%1' could not be created in\n\n'%2'."
			        ).arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}
	} else {
		Q_ASSERT(0);
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

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return;
		}
	}

	/* Method connectToIsds() acquires account information. */
	const QList<QString> accountData =
	    globAccountDbPtr->getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		return;
	}

	QString dbType = accountData.at(0);
	bool dbEffectiveOVM = (accountData.at(1) == "1") ? true : false;
	bool dbOpenAddressing = (accountData.at(2) == "1") ? true : false;

	showStatusTextWithTimeout(tr("Find databoxes from account \"%1\".")
	    .arg(AccountModel::globAccounts[userName].accountName()));

	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_BLANK, 0,
	    dbType, dbEffectiveOVM, dbOpenAddressing, this, userName);
	dsSearch->exec();
}


/* ========================================================================= */
/*
 * Clear message filter text
 */
void MainWindow::clearFilterField(void)
/* ========================================================================= */
{
	debugSlotCall();

	m_filterLine->clear();
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
	columnList.append(1);
	columnList.append(2);
	m_messageListProxyModel.setFilterKeyColumns(columnList);
}


/* ========================================================================= */
/*
 * Set received message column widths and sort order.
 */
void MainWindow::setReceivedColumnWidths(void)
/* ========================================================================= */
{
	debugFuncCall();

	int i;

	ui->messageList->resizeColumnToContents(0);
	ui->messageList->setColumnWidth(1, m_received_1);
	ui->messageList->setColumnWidth(2, m_received_2);
	for (i = 3; i < (DbMsgsTblModel::receivedItemIds.size() - 3); ++i) {
		ui->messageList->resizeColumnToContents(i);
	}
	/* Last three columns display icons. */
	for (; i < DbMsgsTblModel::receivedItemIds.size(); ++i) {
		ui->messageList->setColumnWidth(i, 24);
	}
	if (m_sort_order == "SORT_ASCENDING") {
		ui->messageList->sortByColumn(m_sort_column,
		    Qt::AscendingOrder);
	} else if (m_sort_order == "SORT_DESCENDING") {
		ui->messageList->sortByColumn(m_sort_column,
		    Qt::DescendingOrder);
	}
}

/* ========================================================================= */
/*
 * Set sent message column widths and sort order.
 */
void MainWindow::setSentColumnWidths(void)
/* ========================================================================= */
{
	debugFuncCall();

	int i;

	ui->messageList->resizeColumnToContents(0);
	ui->messageList->setColumnWidth(1, m_sent_1);
	ui->messageList->setColumnWidth(2, m_sent_2);
	for (i = 3; i < (DbMsgsTblModel::sentItemIds.size() - 1); ++i) {
		ui->messageList->resizeColumnToContents(i);
	}
	/* Last column displays an icon. */
	for (; i < DbMsgsTblModel::receivedItemIds.size(); ++i) {
		ui->messageList->setColumnWidth(i, 24);
	}
	if (m_sort_order == "SORT_ASCENDING") {
		ui->messageList->sortByColumn(m_sort_column,
		    Qt::AscendingOrder);
	} else if (m_sort_order == "SORT_DESCENDING") {
		ui->messageList->sortByColumn(m_sort_column,
		    Qt::DescendingOrder);
	}
}

/* ========================================================================= */
/*
 * Set new sent/received message column widths.
 */
void MainWindow::onTableColumnResized(int index, int oldSize, int newSize)
/* ========================================================================= */
{
	debugSlotCall();

	(void) oldSize;
	QModelIndex current = ui->accountList->currentIndex();

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeRecentReceived:
	case AccountModel::nodeReceived:
	case AccountModel::nodeReceivedYear:
		if (index == 1) {
			m_received_1 = newSize;
		} else if (index == 2) {
			m_received_2 = newSize;
		}
		break;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		if (index == 1) {
			m_sent_1 = newSize;
		} else if (index == 2) {
			m_sent_2 = newSize;
		}
		break;
	default:
		break;
	}
}

/* ========================================================================= */
/*
 * Set actual sort order for current column.
 */
void MainWindow::onTableColumnHeaderSectionClicked(int column)
/* ========================================================================= */
{
	debugSlotCall();

	m_sort_column = column;
	if (ui->messageList->horizontalHeader()->sortIndicatorOrder() ==
	    Qt::AscendingOrder) {
		m_sort_order = "SORT_ASCENDING";
	} else if (ui->messageList->horizontalHeader()->sortIndicatorOrder() ==
	           Qt::DescendingOrder) {
		m_sort_order = "SORT_DESCENDING";
	} else {
		m_sort_order = "";
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


/* ========================================================================= */
/*
* Refresh AccountList
*/
void MainWindow::refreshAccountListFromWorker(const QString &userName)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex acntTopIdx =
	    m_accountModel.indexFromItem(itemFromUserName(userName));

	if (!acntTopIdx.isValid()) {
		Q_ASSERT(0);
		return;
	}

	/* Redraw views' content. */
	regenerateAccountModelYears(acntTopIdx);
	/*
	 * Force repaint.
	 * TODO -- A better solution?
	 */
	ui->accountList->repaint();
	accountItemCurrentChanged(ui->accountList->currentIndex());
}


/* ========================================================================= */
/*
* Set ProgressBar value and Status bar text.
*/
void MainWindow::setProgressBarFromWorker(QString label, int value)
/* ========================================================================= */
{
	debugSlotCall();

	m_statusProgressBar->setFormat(label);
	m_statusProgressBar->setValue(value);
	m_statusProgressBar->repaint();
}


/* ========================================================================= */
/*
 * Verify message. Compare hash with hash stored in ISDS.
 */
qdatovka_error MainWindow::verifyMessage(const QString &userName, qint64 dmId,
    const QDateTime &deliveryTime)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());

	isds_error status;

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return Q_CONNECT_ERROR;
		}
	}

	struct isds_hash *hashIsds = NULL;

	status = isds_download_message_hash(isdsSessions.session(userName),
	    QString::number(dmId).toUtf8().constData(), &hashIsds);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return Q_ISDS_ERROR;
	}

	struct isds_hash *hashLocal = NULL;
	hashLocal = (struct isds_hash *) malloc(sizeof(struct isds_hash));

	if (hashLocal == NULL) {
		free(hashLocal);
		return Q_GLOBAL_ERROR;
	}

	memset(hashLocal, 0, sizeof(struct isds_hash));
	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	QStringList hashLocaldata = messageDb->msgsGetHashFromDb(dmId);

	/* TODO - check if hash info is in db */
	if (hashLocaldata.isEmpty()) {
		isds_hash_free(&hashLocal);
		isds_hash_free(&hashIsds);
		return Q_SQL_ERROR;
	}

	QByteArray rawHash = QByteArray::fromBase64(hashLocaldata[0].toUtf8());
	hashLocal->length = (size_t)rawHash.size();
	hashLocal->algorithm =
	    (isds_hash_algorithm)convertHashAlg2(hashLocaldata[1]);
	hashLocal->value = malloc(hashLocal->length);
	memcpy(hashLocal->value, rawHash.data(), hashLocal->length);

	status = isds_hash_cmp(hashIsds, hashLocal);

	isds_hash_free(&hashIsds);
	isds_hash_free(&hashLocal);

	if (IE_NOTEQUAL == status) {
		return Q_NOTEQUAL;
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return Q_ISDS_ERROR;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
 * Get data about logged in user and his box.
 */
bool MainWindow::getOwnerInfoFromLogin(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	struct isds_DbOwnerInfo *db_owner_info = NULL;

	isds_error status = isds_GetOwnerInfoFromLogin(isdsSessions.session(
	    userName), &db_owner_info);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_DbOwnerInfo_free(&db_owner_info);
		return false;
	}

	QString birthDate;
	if ((NULL != db_owner_info->birthInfo) &&
	    (NULL != db_owner_info->birthInfo->biDate)) {
		birthDate = tmBirthToDbFormat(db_owner_info->birthInfo->biDate);
	}

	int ic = 0;
	if (NULL != db_owner_info->ic) {
		ic = QString(db_owner_info->ic).toInt();
	}

	QString key = userName + "___True";

	globAccountDbPtr->insertAccountIntoDb(
	    key,
	    db_owner_info->dbID,
	    convertDbTypeToString(*db_owner_info->dbType),
	    ic,
	    db_owner_info->personName ?
		db_owner_info->personName->pnFirstName : NULL,
	    db_owner_info->personName ?
		db_owner_info->personName->pnMiddleName : NULL,
	    db_owner_info->personName ?
		db_owner_info->personName->pnLastName : NULL,
	    db_owner_info->personName ?
		db_owner_info->personName->pnLastNameAtBirth : NULL,
	    db_owner_info->firmName,
	    birthDate,
	    db_owner_info->birthInfo ?
		db_owner_info->birthInfo->biCity : NULL,
	    db_owner_info->birthInfo ?
		db_owner_info->birthInfo->biCounty : NULL,
	    db_owner_info->birthInfo ?
		db_owner_info->birthInfo->biState : NULL,
	    db_owner_info->address ?
		db_owner_info->address->adCity : NULL,
	    db_owner_info->address ?
		db_owner_info->address->adStreet : NULL,
	    db_owner_info->address ?
		db_owner_info->address->adNumberInStreet : NULL,
	    db_owner_info->address ?
		db_owner_info->address->adNumberInMunicipality : NULL,
	    db_owner_info->address ?
		db_owner_info->address->adZipCode : NULL,
	    db_owner_info->address ?
		db_owner_info->address->adState : NULL,
	    db_owner_info->nationality,
	    db_owner_info->identifier,
	    db_owner_info->registryCode,
	    (int)*db_owner_info->dbState,
	    *db_owner_info->dbEffectiveOVM,
	    *db_owner_info->dbOpenAddressing);

	isds_DbOwnerInfo_free(&db_owner_info);

	return true;
}


/* ========================================================================= */
/*
 * Get information about password expiration date.
 */
bool MainWindow::getPasswordInfoFromLogin(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	isds_error status;
	struct timeval *expiration = NULL;
	QString expirDate;
	bool retval = false;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	status = isds_get_password_expiration(isdsSessions.session(userName),
	    &expiration);

	if (IE_SUCCESS == status) {
		if (NULL != expiration) {
			expirDate = timevalToDbFormat(expiration);
		} else {
			/* Password without expiration. */
			expirDate.clear();
		}
		retval = true;
	} else {
		expirDate.clear();
	}

	QString key = userName + "___True";

	globAccountDbPtr->setPwdExpirIntoDb(key, expirDate);

	if (NULL != expiration) {
		free(expiration);
	}
	return retval;
}


/* ========================================================================= */
/*
* Get data about logged in user.
*/
bool MainWindow::getUserInfoFromLogin(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	struct isds_DbUserInfo *db_user_info = NULL;

	isds_error status = isds_GetUserInfoFromLogin(isdsSessions.session(
	    userName), &db_user_info);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_DbUserInfo_free(&db_user_info);
		return false;
	}

	QString key = userName + "___True";

	globAccountDbPtr->insertUserIntoDb(
	    key,
	    convertUserTypeToString(*db_user_info->userType),
	    (int)*db_user_info->userPrivils,
	    db_user_info->personName ?
		db_user_info->personName->pnFirstName : NULL,
	    db_user_info->personName ?
		db_user_info->personName->pnMiddleName : NULL,
	    db_user_info->personName ?
		db_user_info->personName->pnLastName : NULL,
	    db_user_info->personName ?
		db_user_info->personName->pnLastNameAtBirth : NULL,
	    db_user_info->address ?
		db_user_info->address->adCity : NULL,
	    db_user_info->address ?
		db_user_info->address->adStreet : NULL,
	    db_user_info->address ?
		db_user_info->address->adNumberInStreet : NULL,
	    db_user_info->address ?
		db_user_info->address->adNumberInMunicipality : NULL,
	    db_user_info->address ?
		db_user_info->address->adZipCode : NULL,
	    db_user_info->address ?
		db_user_info->address->adState : NULL,
	    db_user_info->biDate ?
		tmBirthToDbFormat(db_user_info->biDate) : NULL,
	    db_user_info->ic ? QString(db_user_info->ic).toInt() : 0,
	    db_user_info->firmName,
	    db_user_info->caStreet,
	    db_user_info->caCity,
	    db_user_info->caZipCode,
	    db_user_info->caState
	    );

	isds_DbUserInfo_free(&db_user_info);

	return true;
}


/* ========================================================================= */
/*
 * About application dialog.
 */
void MainWindow::aboutApplication(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *abDialog = new aboutDialog(this);
	abDialog->exec();
}


/* ========================================================================= */
/*
 * Show import database directory dialog.
 */
void MainWindow::showImportDatabaseDialog(void)
/* ========================================================================= */
{
	QDialog *prepareCreateAccount = new CreateAccountFromDbDialog(this);
	connect(prepareCreateAccount,
	    SIGNAL(returnAction(bool)), this,
	    SLOT(prepareCreateAccountFromDatabaseFile(bool)));
	prepareCreateAccount->exec();
}


/* ========================================================================= */
/*
 * Prepare import database directory.
 */
void MainWindow::prepareCreateAccountFromDatabaseFile(bool fromDirectory)
/* ========================================================================= */
{
	debugSlotCall();

	QString importDir;
	QStringList fileList, filePathList;
	QStringList nameFilter("*.db");
	QDir directory(QDir::home());
	fileList.clear();
	filePathList.clear();

	if (fromDirectory) {
		importDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory"), m_on_import_database_dir_activate,
		    QFileDialog::ShowDirsOnly |
		    QFileDialog::DontResolveSymlinks);

		if (importDir.isEmpty()) {
			return;
		}

		m_on_import_database_dir_activate = importDir;
		directory.setPath(importDir);
		fileList = directory.entryList(nameFilter);

		if (fileList.isEmpty()) {
			qDebug() << "No *.db selected file(s)";
			/* TODO - show dialog*/
			showStatusTextWithTimeout(tr("Database file(s) not found in "
			    "selected directory."));
			return;
		}

		for (int i = 0; i < fileList.size(); ++i) {
			filePathList.append(importDir + "/" + fileList.at(i));
		}
	} else {
		filePathList = QFileDialog::getOpenFileNames(this,
		    tr("Select db file(s)"), m_on_import_database_dir_activate,
		    tr("Database file (*.db)"));

		if (filePathList.isEmpty()) {
			qDebug() << "No *.db selected file(s)";
			showStatusTextWithTimeout(
			    tr("Database file(s) not selected."));
			return;
		}

		m_on_import_database_dir_activate =
		    QFileInfo(filePathList.at(0)).absoluteDir().absolutePath();
	}

	createAccountFromDatabaseFileList(filePathList);
}


/* ========================================================================= */
/*
 * Create accounts from list of database directory to application
 */
void MainWindow::createAccountFromDatabaseFileList(
    const QStringList &filePathList)
/* ========================================================================= */
{

	debugFuncCall();

	const int dbFilesCnt = filePathList.size();

	if (0 == dbFilesCnt) {
		return;
	}

	QStringList currentAccountList;
	QString errMsg;

	int accountCount = ui->accountList->model()->rowCount();
	for (int i = 0; i < accountCount; i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		const QString userName =
		    index.data(ROLE_ACNT_USER_NAME).toString();
		Q_ASSERT(!userName.isEmpty());
		currentAccountList.append(userName);
	}

	for (int i = 0; i < dbFilesCnt; ++i) {

		QFileInfo file(filePathList.at(i));
		QString dbFileName = file.fileName();
		QString dbUserName;
		QString dbYearFlag;
		bool dbTestingFlag;

		/* Split and check the database file name. */
		if (!isValidDatabaseFileName(dbFileName, dbUserName,
		    dbYearFlag, dbTestingFlag, errMsg)) {
			QMessageBox::warning(this,
			    tr("Create account: %1").arg(dbUserName),
			    tr("File") + ": " + filePathList.at(i) +
			    "\n\n" + errMsg,
			    QMessageBox::Ok);
			continue;
		}

		/* Check whether account already exists. */
		bool exists = false;
		for (int j = 0; j < accountCount; ++j) {
			if (currentAccountList.at(j) == dbUserName) {
				exists = true;
				break;
			}
		}

		if (exists) {
			errMsg = tr(
			    "Account with user name '%1' and "
			    "its message database already exist. "
			    "New account was not created and "
			    "selected database file was not "
			    "associated with this account.").
			    arg(dbUserName);
			QMessageBox::warning(this,
			    tr("Create account: %1").arg(dbUserName),
			    tr("File") + ": " + filePathList.at(i) +
			    "\n\n" + errMsg,
			    QMessageBox::Ok);
			continue;
		}

		AccountModel::SettingsMap itemSettings;
		itemSettings.setTestAccount(dbTestingFlag);
		itemSettings.setAccountName(dbUserName);
		itemSettings.setUserName(dbUserName);
		itemSettings.setLoginMethod(LIM_USERNAME);
		itemSettings.setPassword("");
		itemSettings.setRememberPwd(false);
		itemSettings.setSyncWithAll(false);
		itemSettings.setDbDir(m_on_import_database_dir_activate);
		m_accountModel.addAccount(dbUserName, itemSettings);
		errMsg = tr("Account with name '%1' has been "
		    "created (user name '%1').").arg(dbUserName)
		    + " " +
		    tr("This database file has been set as "
		    "actual message database for this account. "
		    "Maybe you have to change account "
		    "properties for correct login to the "
		    "server Datové schránky.");

		QMessageBox::information(this,
		    tr("Create account: %1").arg(dbUserName),
		    tr("File") + ": " + filePathList.at(i) +
		    "\n\n" + errMsg,
		    QMessageBox::Ok);

		saveSettings();
	}

	activeAccountMenuAndButtons(true);
	ui->accountList->expandAll();
}


/* ========================================================================= */
/*
 * Authenticate message from ZFO file.
 */
qdatovka_error MainWindow::authenticateMessageFromZFO(void)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	const QString userName =
	    acntTopIdx.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	QString attachFileName = QFileDialog::getOpenFileName(this,
	    tr("Add ZFO file"), "", tr("ZFO file (*.zfo)"));

	if (attachFileName.isNull()) {
		return Q_CANCEL;
	}

	size_t length;
	isds_error status;
	QByteArray bytes;
	QFile file(attachFileName);

	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Couldn't open the file" << attachFileName;
			return Q_FILE_ERROR;
		}

		bytes = file.readAll();
		length = bytes.size();
	} else {
		return Q_FILE_ERROR;
	}

	showStatusTextPermanently(tr("Verifying the ZFO file \"%1\"")
	    .arg(attachFileName));

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return Q_CONNECT_ERROR;
		}
	}

	status = isds_authenticate_message(isdsSessions.session(userName),
	    bytes.data(), length);

	if (IE_NOTEQUAL == status) {
		return Q_NOTEQUAL;
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return Q_ISDS_ERROR;
	}

	return Q_SUCCESS;
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
	case Q_SUCCESS:
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
	case Q_NOTEQUAL:
		showStatusTextWithTimeout(tr("Server Datové schránky confirms "
		    "that the message is not authentic."));
		QMessageBox::critical(this, tr("Message is not authentic"),
		    tr("Message was <b>not</b> authenticated as processed "
		    "by the system Datové schránky.") + "<br/><br/>" +
		    tr("It is either not a valid ZFO file or it was modified "
		    "since it was downloaded from Datové schránky."),
		    QMessageBox::Ok);
		break;
	case Q_ISDS_ERROR:
	case Q_CONNECT_ERROR:
		showStatusTextWithTimeout(tr("Message authentication failed."));
		QMessageBox::warning(this, tr("Message authentication failed"),
		    tr("Authentication of message has been stopped because "
		    "the connection to server Datové schránky failed!\n"
		    "Check your internet connection."),
		    QMessageBox::Ok);
		break;
	case Q_FILE_ERROR:
		showStatusTextWithTimeout(tr("Message authentication failed."));
		QMessageBox::warning(this, tr("Message authentication failed"),
		    tr("Authentication of message has been stopped because "
		    "the message file has wrong format!"),
		    QMessageBox::Ok);
		break;
	case Q_CANCEL:
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
	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	QString userName;
	qint64 dmId = -1;
	QDateTime deliveryTime;
	{
		QModelIndex acntTopIdx = ui->accountList->currentIndex();
		acntTopIdx = AccountModel::indexTop(acntTopIdx);
		userName = acntTopIdx.data(ROLE_ACNT_USER_NAME).toString();

		QModelIndex msgIdx = firstMsgColumnIdxs.first();
		dmId = msgIdx.sibling(msgIdx.row(), 0).data().toLongLong();
		deliveryTime = msgDeliveryTime(msgIdx);
	}
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(deliveryTime.isValid());

	switch (verifyMessage(userName, dmId, deliveryTime)) {
	case Q_SUCCESS:
		showStatusTextWithTimeout(tr("Server Datové schránky confirms "
		    "that the message is valid."));
		QMessageBox::information(this, tr("Message is valid"),
		    tr("Message was <b>successfully verified</b> "
		    "against data on the server Datové schránky.") +
		    "<br/><br/>" +
		    tr("This message has passed through the system of "
		    "Datové schránky and has not been tampered with since."),
		    QMessageBox::Ok);
		break;
	case Q_NOTEQUAL:
		showStatusTextWithTimeout(tr("Server Datové schránky confirms "
		    "that the message is not valid."));
		QMessageBox::critical(this, tr("Message is not valid"),
		    tr("Message was <b>not</b> authenticated as processed "
		    "by the system Datové schránky.") + "<br/><br/>" +
		    tr("It is either not a valid ZFO file or it was modified "
		    "since it was downloaded from Datové schránky."),
		     QMessageBox::Ok);
		break;
	case Q_ISDS_ERROR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::warning(this, tr("Verification failed"),
		    tr("Authentication of message has been stopped because "
		    "the connection to server Datové schránky failed!\n"
		    "Check your internet connection."),
		    QMessageBox::Ok);
		break;
	case Q_SQL_ERROR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::warning(this, tr("Verification error"),
		    tr("The message hash is not in local database.\nPlease "
		    "download complete message from ISDS and try again."),
		    QMessageBox::Ok);
		break;
	case Q_GLOBAL_ERROR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::critical(this, tr("Verification error"),
		    tr("The message hash cannot be verified because an internal"
		    " error occurred!\nTry again."),
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


/* ========================================================================= */
/*
 * View message from file dialog.
 */
void MainWindow::viewMessageFromZFO(void)
/* ========================================================================= */
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
	QDialog *viewDialog = new DlgViewZfo(fileName, this);
	viewDialog->exec();
}


/* ========================================================================= */
/*
 * Export correspondence overview dialog.
 */
void MainWindow::exportCorrespondenceOverview(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	const QString userName = index.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	const QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QDialog *correspondence_overview = new DlgCorrespondenceOverview(
	    *dbSet, userName, m_export_correspond_dir, dbId, this);

	correspondence_overview->exec();
	storeExportPath();
}


/* ========================================================================= */
/*
 * Show dialog with settings of import ZFO file(s) into database.
 */
void MainWindow::showImportZFOActionDialog(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *importZfo = new ImportZFODialog(this);
	connect(importZfo,
	    SIGNAL(returnZFOAction(enum ImportZFODialog::ZFOtype,
	        enum ImportZFODialog::ZFOaction)),
	    this,
	    SLOT(createZFOListForImport(enum ImportZFODialog::ZFOtype,
	        enum ImportZFODialog::ZFOaction)));
	importZfo->exec();
}


/* ========================================================================= */
/*
 * Create ZFO file(s) list for import into database.
 */
void MainWindow::createZFOListForImport(enum ImportZFODialog::ZFOtype zfoType,
    enum ImportZFODialog::ZFOaction importType)
/* ========================================================================= */
{
	debugSlotCall();

	bool includeSubdir = false;
	QString importDir;
	QStringList fileList, filePathList;
	QStringList nameFilter("*.zfo");
	QDir directory(QDir::home());
	fileList.clear();
	filePathList.clear();

	switch (importType) {
	case ImportZFODialog::IMPORT_FROM_SUBDIR:
		includeSubdir = true;
	case ImportZFODialog::IMPORT_FROM_DIR:
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

		break;

	case ImportZFODialog::IMPORT_SEL_FILES:
		filePathList = QFileDialog::getOpenFileNames(this,
		    tr("Select ZFO file(s)"), m_import_zfo_path,
		    tr("ZFO file (*.zfo)"));

		if (filePathList.isEmpty()) {
			qDebug() << "ZFO-IMPORT:" <<"No *.zfo selected file(s)";
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

	prepareZFOImportIntoDatabase(filePathList, zfoType);
}


/* ========================================================================= */
/*
 * Create account info for ZFO file(s) import into database.
 */
QList<MainWindow::AccountDataStruct> MainWindow::createAccountInfoForZFOImport(void)
/* ========================================================================= */
{
	debugFuncCall();

	AccountDataStruct accountData;
	QList<AccountDataStruct> accountList;
	accountList.clear();

	/* get username, accountName, ID of databox and pointer to database
	 * for all accounts from settings */
	for (int i = 0; i < ui->accountList->model()->rowCount(); i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		const QString userName =
		    index.data(ROLE_ACNT_USER_NAME).toString();
		Q_ASSERT(!userName.isEmpty());
		MessageDbSet *dbSet = accountDbSet(userName, this);
		Q_ASSERT(0 != dbSet);
		accountData.acntIndex = index;
		accountData.username = userName;
		accountData.accountName =
		    AccountModel::globAccounts[userName].accountName();
		accountData.databoxID =
		    globAccountDbPtr->dbId(userName + "___True");
		accountData.messageDbSet = dbSet;
		accountList.append(accountData);
	}

	return accountList;
}


/* ========================================================================= */
/*
 * Get message type of import ZFO file (message/delivery/unknown).
 */
int MainWindow::getMessageTypeFromZFO(const QString &file)
/* ========================================================================= */
{
	debugFuncCall();

	int zfoType = -1;
	struct isds_message *message = NULL;
	struct isds_ctx *dummy_session = NULL;

	dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		qDebug() << "ZFO-TYPE:"<< "Cannot create dummy ISDS session.";
		return zfoType;
	}

	/* Check ZFO type */
	message = loadZfoFile(dummy_session, file,
	    ImportZFODialog::IMPORT_MESSAGE_ZFO);
	if (NULL == message) {
		message = loadZfoFile(dummy_session, file,
		    ImportZFODialog::IMPORT_DELIVERY_ZFO);
		if (NULL == message) {
			/* ZFO format unknown */
			zfoType = 0;
		} else {
			/* ZFO is delivery info */
			zfoType = 2;
		}
	} else {
		/* ZFO is message */
		zfoType = 1;
	}

	isds_message_free(&message);
	isds_ctx_free(&dummy_session);

	return zfoType;
}

/* ========================================================================= */
/*
 * Prepare import ZFO file(s) into database by ZFO type.
 */
void MainWindow::prepareZFOImportIntoDatabase(const QStringList &files,
    enum ImportZFODialog::ZFOtype zfoType)
/* ========================================================================= */
{
	debugFuncCall();

	int zfoCnt =  files.size();

	qDebug() << "ZFO-IMPORT:" << "number of ZFO:" << zfoCnt;

	if (zfoCnt == 0) {
		qDebug() << "ZFO-IMPORT:" << "No *.zfo file(s) in the fileList";
		showStatusTextWithTimeout(tr("No ZFO file(s) for import."));
		return;
	}

	QList<AccountDataStruct> const accountList =
	    createAccountInfoForZFOImport();

	if (accountList.isEmpty()) {
		qDebug() << "ZFO-IMPORT:" << "There is no account for import.";
		showStatusTextWithTimeout(tr("There is no account for "
		    "import of ZFO file(s)."));
		return;
	}

	int zfoFileType = 0;
	QPair<QString,QString> impZFOInfo;
	QList<QPair<QString,QString>> errorFilesList; // red
	QList<QPair<QString,QString>> existFilesList; // black
	QList<QPair<QString,QString>> successFilesList; // green
	QStringList messageZFOList;
	QStringList deliveryZFOList;


	/* sort ZFOs by format type */
	for (int i = 0; i < zfoCnt; i++) {
		/* retrun -1=error, 0=unknown, 1=message, 2=delivery info */
		zfoFileType = getMessageTypeFromZFO(files.at(i));
		qDebug() << i << zfoFileType << files.at(i);


		if (zfoFileType == 0) {
			impZFOInfo.first = files.at(i);
			impZFOInfo.second = tr("Wrong ZFO format. This "
			    "file does not contain correct data for import.");
			errorFilesList.append(impZFOInfo);
		} else if (zfoFileType == 1) {
			messageZFOList.append(files.at(i));
		} else if (zfoFileType == 2) {
			deliveryZFOList.append(files.at(i));
		} else {
			impZFOInfo.first = files.at(i);
			impZFOInfo.second = tr("Error during file parsing.");
			errorFilesList.append(impZFOInfo);
		}
	}

	switch (zfoType) {
	case ImportZFODialog::IMPORT_ALL_ZFO:
		qDebug() << "ZFO-IMPORT:" << "IMPORT_ALL_ZFO";
		if (messageZFOList.isEmpty() && deliveryZFOList.isEmpty()) {
			QMessageBox::warning(this, tr("No ZFO file(s)"),
			    tr("The selection does not contain "
			        "any valid ZFO files."),
			    QMessageBox::Ok);
			return;
		}
		/* First, import messages. */
		importMessageZFO(accountList, messageZFOList,
		    successFilesList, existFilesList, errorFilesList);
		/* Second, import delivery information. */
		importDeliveryInfoZFO(accountList, deliveryZFOList,
		    successFilesList, existFilesList, errorFilesList);
		break;

	case ImportZFODialog::IMPORT_MESSAGE_ZFO:
		qDebug() << "ZFO-IMPORT:" << "IMPORT_MESSAGE_ZFO";
		if (messageZFOList.isEmpty()) {
			QMessageBox::warning(this, tr("No ZFO file(s)"),
			    tr("The selection does not contain "
			         "any valid ZFO messages."),
			    QMessageBox::Ok);
			return;
		}
		importMessageZFO(accountList, messageZFOList,
		    successFilesList, existFilesList, errorFilesList);
		break;

	case ImportZFODialog::IMPORT_DELIVERY_ZFO:
		qDebug() << "ZFO-IMPORT:" << "IMPORT_DELIVERY_ZFO";
		if (deliveryZFOList.isEmpty()) {
			QMessageBox::warning(this, tr("No ZFO file(s)"),
			    tr("The selection does not contain any valid "
			        "ZFO delivery infos."),
			    QMessageBox::Ok);
			return;
		}
		importDeliveryInfoZFO(accountList, deliveryZFOList,
		    successFilesList, existFilesList, errorFilesList);
		break;

	default:
		return;
		break;
	}

	showStatusTextWithTimeout(tr("Import of ZFO file(s) ... Completed"));

	showNotificationDialogWithResult(zfoCnt, successFilesList,
	    existFilesList, errorFilesList);
}


/* ========================================================================= */
/*
 * Execute the import of delivery info ZFO file(s) into database.
 */
void MainWindow::importDeliveryInfoZFO(
    const QList<AccountDataStruct> &accountList, const QStringList &files,
    QList<QPair<QString,QString>> &successFilesList,
    QList<QPair<QString,QString>> &existFilesList,
    QList<QPair<QString,QString>> &errorFilesList)
/* ========================================================================= */
{
	int fileCnt = files.size();
	QPair<QString,QString> importZFOInfo;
	QString pInfoText = "";
	QString nInfoText = "";
	QString eInfoText = "";

	/* for every ZFO file detect if message is in the database */
	for (int i = 0; i < fileCnt; ++i) {

		pInfoText = "";
		nInfoText = "";
		eInfoText = "";

		showStatusTextPermanently(tr("Processing of "
		    "delivery info ZFO: %1 ...").arg(files.at(i)));

		struct isds_message *message = NULL;
		struct isds_ctx *dummy_session = NULL;

		dummy_session = isds_ctx_create();
		if (NULL == dummy_session) {
			qDebug() << "Cannot create dummy ISDS session.";
			showStatusTextWithTimeout(tr("Import of ZFO file(s) "
			    "failed!"));
			/* TODO */
			return;
		}

		message = loadZfoFile(dummy_session, files.at(i),
		    ImportZFODialog::IMPORT_DELIVERY_ZFO);
		if (NULL == message || message->envelope == NULL) {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = tr("Wrong ZFO "
			    "format. This file does not contain correct "
			    "data for import.");
			errorFilesList.append(importZFOInfo);
			isds_ctx_free(&dummy_session);
			continue;
		}

		int resISDS = 0;
		bool imported = false;
		bool exists = false;
		qint64 dmId = QString(message->envelope->dmID).toLongLong();
		QDateTime deliveryTime = timevalToDateTime(message->envelope->dmDeliveryTime);
		Q_ASSERT(deliveryTime.isValid());

		for (int j = 0; j < accountList.size(); j++) {
			/* check if message envelope is in database */
			MessageDb *messageDb =
			    accountList.at(j).messageDbSet->accessMessageDb(
			        deliveryTime, true);
			Q_ASSERT(0 != messageDb);
			if (-1 != messageDb->msgsStatusIfExists(dmId)) {
				/* check if raw is in database */
				if (!messageDb->isDeliveryInfoRawDb(dmId)) {
					/* Is/was ZFO message in ISDS */
					resISDS = isImportMsgInISDS(files.at(i),
					    accountList.at(j).acntIndex);
					if (resISDS == MSG_IS_IN_ISDS) {
						if (Q_SUCCESS ==
						    Worker::storeDeliveryInfo(true,
						    *(accountList.at(j).messageDbSet), message)) {
							pInfoText += tr("Imported as delivery "
							    "info for message "
							    "\"%1\", account \"%2\".").
							    arg(dmId).arg(accountList.at(j).accountName);
							pInfoText += "<br/>";
							imported = true;
						} else {
							nInfoText =
							    tr("File has "
							    "not been imported "
							    "because an error "
							    "was detected "
							    "during insertion "
							    "process.");
						}
					} else if (resISDS == MSG_IS_NOT_IN_ISDS) {
						nInfoText = tr("Message \"%1\""
						    " does not exists on the server "
						    "Datové schránky.").arg(dmId);
					} else if (resISDS == MSG_FILE_ERROR) {
						nInfoText = tr("Couldn't open this file "
						    "for authentication on the "
						    "server Datové schránky.");
					} else {
						QMessageBox msgBox(this);
						msgBox.setIcon(QMessageBox::Warning);
						msgBox.setWindowTitle(tr("ZFO import problem"));
						msgBox.setText(tr("Do you want to continue with import?"));
						msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
						msgBox.setDefaultButton(QMessageBox::No);
						if (QMessageBox::No == msgBox.exec()) {
							nInfoText = tr("It is not possible "
							    "to connect to server Datové "
							    "schránky and verify validity of "
							    "this ZFO file.");
							nInfoText += "<br/><br/>" + tr("Action was canceled by user...");
							importZFOInfo.first = files.at(i);
							importZFOInfo.second = nInfoText;
							errorFilesList.append(importZFOInfo);
							isds_message_free(&message);
							isds_ctx_free(&dummy_session);
							showStatusTextWithTimeout(tr("Import of ZFO file(s) was canceled"));
							return;
						} else {
							nInfoText = tr("It is not possible "
							    "to connect to server Datové "
							    "schránky and verify validity of "
							    "this ZFO file.");
						}
					}
				} else {
					exists = true;
					eInfoText += tr("Delivery info for message \"%1\" already exists in the "
					    "local database, account \"%2\".").
					    arg(dmId).arg(accountList.at(j).accountName);
					eInfoText += "<br/>";
				}
			} else {
				nInfoText = tr("This file (delivery info) has "
				    "not been inserted into database because "
				    "there isn't any related message (%1) in "
				    "the databases.").arg(dmId);
			}
		} // for

		if (imported) {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = pInfoText;
			successFilesList.append(importZFOInfo);
		} else if (exists){
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = eInfoText;
			existFilesList.append(importZFOInfo);
		} else {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = nInfoText;
			errorFilesList.append(importZFOInfo);
		}

		isds_message_free(&message);
		isds_ctx_free(&dummy_session);
	}
}


/* ========================================================================= */
/*
 * Execute the import of message ZFO file(s) into database.
 */
void  MainWindow::importMessageZFO(const QList<AccountDataStruct> &accountList,
    const QStringList &files, QList<QPair<QString,QString>> &successFilesList,
    QList<QPair<QString,QString>> &existFilesList,
    QList<QPair<QString,QString>> &errorFilesList)
/* ========================================================================= */
{
	int fileCnt = files.size();
	QPair<QString,QString> importZFOInfo;
	QString pInfoText = "";
	QString nInfoText = "";
	QString eInfoText = "";

	/* for every ZFO file detect its database and message type */
	for (int i = 0; i < fileCnt; ++i) {

		pInfoText = "";
		nInfoText = "";
		eInfoText = "";

		showStatusTextPermanently(tr("Processing of "
		    "message ZFO: %1 ...").arg(files.at(i)));

		struct isds_message *message = NULL;
		struct isds_ctx *dummy_session = NULL;

		dummy_session = isds_ctx_create();
		if (NULL == dummy_session) {
			qDebug() << "Cannot create dummy ISDS session.";
			showStatusTextWithTimeout(tr("Import of ZFO file(s) "
			    "failed!"));
			/* TODO */
			return;
		}

		message = loadZfoFile(dummy_session, files.at(i),
		    ImportZFODialog::IMPORT_MESSAGE_ZFO);
		if (NULL == message || message->envelope == NULL) {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = tr("Wrong ZFO format. "
			    "File does not contain correct data for import.");
			errorFilesList.append(importZFOInfo);
			isds_ctx_free(&dummy_session);
			continue;
		}

		QString dbIDSender = message->envelope->dbIDSender;
		QString dbIDRecipient = message->envelope->dbIDRecipient;

		/* save database username where message will be inserted */
		QString isSent;
		QString isReceived;
		qint64 dmId = QString(message->envelope->dmID).toLongLong();
		QDateTime deliveryTime = timevalToDateTime(message->envelope->dmDeliveryTime);
		Q_ASSERT(deliveryTime.isValid());
		int resISDS = 0;
		bool import = false;
		bool exists = false;

		/* message type recognition {sent,received}, insert into DB */
		for (int j = 0; j < accountList.size(); j++) {
			MessageDb *messageDb =
			    accountList.at(j).messageDbSet->accessMessageDb(
			        deliveryTime, true);

			/* is sent */
			if (accountList.at(j).databoxID == dbIDSender) {

				isSent = accountList.at(j).username;
				qDebug() << dmId << "isSent" << isSent;

				/* Is/was ZFO message in ISDS */
				resISDS = isImportMsgInISDS(files.at(i),
				    accountList.at(j).acntIndex);
				if (resISDS == MSG_IS_IN_ISDS) {
					if (-1 == messageDb->msgsStatusIfExists(dmId)) {
						Worker::storeEnvelope(MSG_SENT, *(accountList.at(j).messageDbSet), message->envelope);
						if (Q_SUCCESS == Worker::storeMessage(true, MSG_SENT, *(accountList.at(j).messageDbSet), message, "", 0, 0)) {
							import = true;
							pInfoText += tr("Imported as sent message "
							    "\"%1\" into account \"%2\".").
							    arg(dmId).arg(accountList.at(j).accountName);
							pInfoText += "<br/>";
						} else {
							nInfoText =
							    tr("File has "
							    "not been imported "
							    "because an error "
							    "was detected "
							    "during insertion "
							    "process.");
						}
					} else {
						exists = true;
						eInfoText += tr("Message \"%1\" already exists in the "
						    "local database, account \"%2\".").
						    arg(dmId).arg(accountList.at(j).accountName);
						eInfoText += "<br/>";
					}
				} else if (resISDS == MSG_IS_NOT_IN_ISDS) {
					nInfoText = tr("Message \"%1\""
					    " does not exists on the server "
					    "Datové schránky.").arg(dmId);
				} else if (resISDS == MSG_FILE_ERROR) {
					nInfoText = tr("Couldn't open this file "
					    "for authentication on the "
					    "server Datové schránky.");
				} else {
					QMessageBox msgBox(this);
					msgBox.setIcon(QMessageBox::Warning);
					msgBox.setWindowTitle(tr("ZFO import problem"));
					msgBox.setText(tr("Do you want to continue with import?"));
					msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
					msgBox.setDefaultButton(QMessageBox::No);
					if (QMessageBox::No == msgBox.exec()) {
						nInfoText = tr("It is not possible "
						    "to connect to server Datové "
						    "schránky and verify validity of "
						    "this ZFO file.");
						nInfoText += "<br/><br/>" + tr("Action was canceled by user...");
						importZFOInfo.first = files.at(i);
						importZFOInfo.second = nInfoText;
						errorFilesList.append(importZFOInfo);
						isds_message_free(&message);
						isds_ctx_free(&dummy_session);
						showStatusTextWithTimeout(tr("Import of ZFO file(s) was canceled"));
						return;
					} else {
						nInfoText = tr("It is not possible "
						    "to connect to server Datové "
						    "schránky and verify validity of "
						    "this ZFO file.");
					}
				}
			}

			/* is received */
			if (accountList.at(j).databoxID == dbIDRecipient) {
				isReceived = accountList.at(j).username;
				qDebug() << dmId << "isReceived" << isReceived;
				resISDS = isImportMsgInISDS(files.at(i),
				    accountList.at(j).acntIndex);

				if (resISDS == MSG_IS_IN_ISDS) {
					if (-1 == messageDb->msgsStatusIfExists(dmId)) {
						Worker::storeEnvelope(MSG_RECEIVED, *(accountList.at(j).messageDbSet), message->envelope);
						if (Q_SUCCESS == Worker::storeMessage(true, MSG_RECEIVED, *(accountList.at(j).messageDbSet), message, "", 0, 0)) {
							import = true;
							/* update message state into database */
							messageDb->msgSetProcessState(dmId, SETTLED, false);
							pInfoText += tr("Imported as received message "
							    "\"%1\" into account \"%2\".").
							    arg(dmId).arg(accountList.at(j).accountName);
							pInfoText += "<br/>";
						} else {
							nInfoText =
							    tr("File has "
							    "not been imported "
							    "because an error "
							    "was detected "
							    "during insertion "
							    "process.");
						}
					} else {
						exists = true;
						eInfoText += tr("Message \"%1\" already exists in the "
						    "local database, account \"%2\".").
						    arg(dmId).arg(accountList.at(j).accountName);
						eInfoText += "<br/>";
					}
				} else if (resISDS == MSG_IS_NOT_IN_ISDS) {
					nInfoText = tr("Message \"%1\""
					    " does not exists on the server "
					    "Datové schránky.").arg(dmId);
				} else if (resISDS == MSG_FILE_ERROR) {
					nInfoText = tr("Couldn't open this file "
					    "for authentication on the "
					    "server Datové schránky.");
				} else {
					QMessageBox msgBox(this);
					msgBox.setIcon(QMessageBox::Warning);
					msgBox.setWindowTitle(tr("ZFO import problem"));
					msgBox.setText(tr("Do you want to continue with import?"));
					msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
					msgBox.setDefaultButton(QMessageBox::No);
					if (QMessageBox::No == msgBox.exec()) {
						nInfoText = tr("It is not possible "
						    "to connect to server Datové "
						    "schránky and verify validity of "
						    "this ZFO file.");
						nInfoText += "<br/><br/>" + tr("Action was canceled by user...");
						importZFOInfo.first = files.at(i);
						importZFOInfo.second = nInfoText;
						errorFilesList.append(importZFOInfo);
						isds_message_free(&message);
						isds_ctx_free(&dummy_session);
						showStatusTextWithTimeout(tr("Import of ZFO file(s) was canceled"));
						return;
					} else {
						nInfoText = tr("It is not possible "
						    "to connect to server Datové "
						    "schránky and verify validity of "
						    "this ZFO file.");
					}
				}
			}
		} //for

		/* */
		if (isReceived.isNull() && isSent.isNull()) {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = tr("For this file does not "
			    "exist correct databox or relevant account.");
			errorFilesList.append(importZFOInfo);
		} else if (import) {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = pInfoText;
			successFilesList.append(importZFOInfo);
		} else if (exists) {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = eInfoText;
			existFilesList.append(importZFOInfo);
		} else {
			importZFOInfo.first = files.at(i);
			importZFOInfo.second = nInfoText;
			errorFilesList.append(importZFOInfo);
		}

		isds_message_free(&message);
		isds_ctx_free(&dummy_session);
	} //for
}


/* ========================================================================= */
/*
 * Check if import ZFO file is/was in ISDS.
 */
int MainWindow::isImportMsgInISDS(const QString &zfoFile,
    QModelIndex accountIndex)
/* ========================================================================= */
{
	Q_ASSERT(accountIndex.isValid());

	size_t length;
	isds_error status;
	QByteArray bytes;
	QFile file(zfoFile);

	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Couldn't open the file" << zfoFile;
			return MSG_FILE_ERROR;
		}

		bytes = file.readAll();
		length = bytes.size();
	} else {
		return MSG_FILE_ERROR;
	}

	accountIndex = AccountModel::indexTop(accountIndex);
	const QString userName =
	    accountIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return MSG_ISDS_ERROR;
		}
	}

	status = isds_authenticate_message(isdsSessions.session(userName),
	    bytes.data(), length);

	// not in ISDS
	if (IE_NOTEQUAL == status) {
		return MSG_IS_NOT_IN_ISDS;
	}

	if (IE_SUCCESS == status) {
		// is/was in ISDS
		return MSG_IS_IN_ISDS;
	} else {
		// any error
		qDebug() << status << isds_strerror(status);
		return MSG_ISDS_ERROR;
	}
}


/* ========================================================================= */
/*
 * Show ZFO import notification dialog with results of import
 */
void MainWindow::showNotificationDialogWithResult(int filesCnt,
    const QList<QPair<QString,QString>> &successFilesList,
    const QList<QPair<QString,QString>> &existFilesList,
    const QList<QPair<QString,QString>> &errorFilesList)
/* ========================================================================= */
{
	debugFuncCall();

	QDialog *importZfoResult = new ImportZFOResultDialog(filesCnt,
	    errorFilesList, successFilesList, existFilesList, this);
	importZfoResult->exec();
}


/* ========================================================================= */
/*
 * Show help/manual in a internet browser.
 */
void MainWindow::showHelp(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDesktopServices::openUrl(QUrl(DATOVKA_ONLINE_HELP_URL,
	    QUrl::TolerantMode));
}


/* ========================================================================= */
/*
 * Go to homepage.
 */
void MainWindow::goHome(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDesktopServices::openUrl(QUrl(DATOVKA_HOMEPAGE_URL, QUrl::TolerantMode));
}


/* ========================================================================= */
/*
 * Export message into as ZFO file dialogue.
 */
void MainWindow::exportSelectedMessageAsZFO(const QString &attachPath,
    QString userName, qint64 dmID, const QDateTime &delivTime)
/* ========================================================================= */
{
	debugSlotCall();

	qint64 dmId;
	QDateTime deliveryTime;

	if ((dmID == -1) || !delivTime.isValid()) {
		QModelIndexList firstMsgColumnIdxs =
		    ui->messageList->selectionModel()->selectedRows(0);
		if (1 != firstMsgColumnIdxs.size()) {
			return;
		}

		const QModelIndex &msgIdx = firstMsgColumnIdxs.first();

		Q_ASSERT(msgIdx.isValid());
		if (!msgIdx.isValid()) {
			showStatusTextWithTimeout(tr("Export of message to "
			    "ZFO was not successful!"));
			return;
		}

		dmId = msgIdx.data().toLongLong();
		deliveryTime = msgDeliveryTime(msgIdx);
	} else {
		dmId = dmID;
		deliveryTime = delivTime;
	}
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(deliveryTime.isValid());

	if (userName.isEmpty()) {
		QModelIndex acntIndex = ui->accountList->currentIndex();
		acntIndex = AccountModel::indexTop(acntIndex);
		userName = acntIndex.data(ROLE_ACNT_USER_NAME).toString();
	}
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsMessageBase64(dmId);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Message export error!"));
		msgBox.setText(tr("Cannot export complete message '%1'.").
		    arg(dmId));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download the whole message before "
		        "exporting.") + "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId, deliveryTime)) {
				showStatusTextWithTimeout(tr("Export of message "
				"\"%1\" to ZFO was not successful!")
				.arg(dmId));
				return;
			} else {
				base64 = messageDb->msgsMessageBase64(dmId);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message "
			"\"%1\" to ZFO was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName =
	    createFilenameFromFormatString(globPref.message_filename_format,
	    QString::number(dmId), dbId, userName, "",
	    entry.dmDeliveryTime, entry.dmAcceptanceTime, entry.dmAnnotation,
	    entry.dmSender);

	if (attachPath.isNull()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".zfo";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".zfo";
	}

	Q_ASSERT(!fileName.isEmpty());

	if (dmID == -1) {
		fileName = QFileDialog::getSaveFileName(this,
		    tr("Save message as ZFO file"), fileName,
		    tr("ZFO file (*.zfo)"));
	}

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings if attachPath was not set */
	if (attachPath.isNull()) {
		m_on_export_zfo_activate =
		    QFileInfo(fileName).absoluteDir().absolutePath();
		storeExportPath();
	}

	QByteArray data = QByteArray::fromBase64(base64);

	enum WriteFileState ret = writeFile(fileName, data);
	if (WF_SUCCESS == ret) {
		showStatusTextWithTimeout(tr("Export of message \"%1\" to "
		    "ZFO was successful!").arg(dmId));
	} else {
		showStatusTextWithTimeout(tr("Export of message \"%1\" to "
		    "ZFO was not successful.").arg(dmId));
		QMessageBox::warning(this,
		    tr("Error exporting message '%1'.").arg(dmId),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Download complete message synchronously without worker and thread
 */
bool MainWindow::downloadCompleteMessage(qint64 dmId,
    const QDateTime &deliveryTime)
/* ========================================================================= */
{
	debugFuncCall();

	/* selection().indexes() ? */

	enum MessageDirection msgDirect = MSG_RECEIVED;

	QModelIndex index = ui->accountList->selectionModel()->currentIndex();
	switch (AccountModel::nodeType(index)) {
	case AccountModel::nodeRecentReceived:
	case AccountModel::nodeReceived:
	case AccountModel::nodeReceivedYear:
		msgDirect = MSG_RECEIVED;
		break;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		msgDirect = MSG_SENT;
		break;
	default:
		break;
	}

	const QString userName = userNameFromItem();
	Q_ASSERT(!userName.isEmpty());
	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return false;
		}
	}

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	QString errMsg;
	if (Q_SUCCESS == Worker::downloadMessage(userName,
	        MessageDb::MsgId(dmId, deliveryTime), true,
	        msgDirect, *dbSet, errMsg, QString(), 0, 0)) {
		/* TODO -- Wouldn't it be better with selection changed? */
		postDownloadSelectedMessageAttachments(userName, dmId);
		return true;
	}

	return false;
}


/* ========================================================================= */
/*
 * Export delivery information as ZFO file dialog.
 */
void MainWindow::exportDeliveryInfoAsZFO(const QString &attachPath,
    const QString &attachFileName, const QString &formatString, qint64 dmID,
    const QDateTime &delivTime)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info to ZFO was not successful!"));
		return;
	}

	qint64 dmId;
	QDateTime deliveryTime;

	if ((dmID == -1) || !delivTime.isValid()) {
		dmId = msgIdx.data().toLongLong();
		deliveryTime = msgDeliveryTime(msgIdx);
	} else {
		dmId = dmID;
		deliveryTime = delivTime;
	}
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(deliveryTime.isValid());

	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);
	const QString userName =
	    acntTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Delivery info export error!"));
		msgBox.setText(
		    tr("Cannot export delivery info for message '%1'.").
		    arg(dmId));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download message before export.") +
		    "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId, deliveryTime)) {
				showStatusTextWithTimeout(tr("Export of "
				    "message delivery info \"%1\" to ZFO was "
				    "not successful!").arg(dmId));
				return;
			} else {
				base64 =
				    messageDb->msgsGetDeliveryInfoBase64(dmId);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message delivery "
			"info \"%1\" to ZFO was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName =
	    createFilenameFromFormatString(formatString,
	    QString::number(dmId), dbId, userName, attachFileName,
	    entry.dmDeliveryTime, entry.dmAcceptanceTime, entry.dmAnnotation,
	    entry.dmSender);

	if (attachPath.isNull()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".zfo";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".zfo";
	}

	Q_ASSERT(!fileName.isEmpty());

	if (dmID == -1) {
		fileName = QFileDialog::getSaveFileName(this,
		    tr("Save delivery info as ZFO file"), fileName,
		    tr("ZFO file (*.zfo)"));
	}

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info \"%1\" to ZFO was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings if attachPath was not set */
	if (attachPath.isNull()) {
		m_on_export_zfo_activate =
		    QFileInfo(fileName).absoluteDir().absolutePath();
		storeExportPath();
	}

	QByteArray data = QByteArray::fromBase64(base64);

	enum WriteFileState ret = writeFile(fileName, data);
	if (WF_SUCCESS == ret) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info \"%1\" to ZFO was successful.").arg(dmId));
	} else {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info \"%1\" to ZFO was not successful!").arg(dmId));
		QMessageBox::warning(this,
		    tr("Error exporting message delivery info '%1'.")
		        .arg(dmId),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Export delivery information as PDF file dialogue.
 */
void MainWindow::exportDeliveryInfoAsPDF(const QString &attachPath,
    const QString &attachFileName, const QString &formatString,
    qint64 dmID, const QDateTime &delivTime)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message delivery info "
		    "to PDF was not successful!"));
		return;
	}

	qint64 dmId;
	QDateTime deliveryTime;

	if ((dmID == -1) || !delivTime.isValid()) {
		dmId = msgIdx.data().toLongLong();
		deliveryTime = msgDeliveryTime(msgIdx);
	} else {
		dmId = dmID;
		deliveryTime = delivTime;
	}
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(deliveryTime.isValid());

	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);
	const QString userName =
	    acntTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);;
		msgBox.setWindowTitle(tr("Delivery info export error!"));
		msgBox.setText(
		    tr("Cannot export delivery info for message '%1'.").
		        arg(dmId));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download message before export.") +
		    "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId, deliveryTime)) {
				showStatusTextWithTimeout(tr("Export of "
				    "message delivery info \"%1\" to PDF was "
				    "not successful!").arg(dmId));
				return;
			} else {
				base64 =
				    messageDb->msgsGetDeliveryInfoBase64(dmId);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message delivery "
			"info \"%1\" to PDF was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName =
	    createFilenameFromFormatString(formatString,
	    QString::number(dmId), dbId, userName, attachFileName,
	    entry.dmDeliveryTime, entry.dmAcceptanceTime, entry.dmAnnotation,
	    entry.dmSender);

	if (attachPath.isNull()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".pdf";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".pdf";
	}

	if (dmID == -1) {
		fileName = QFileDialog::getSaveFileName(this,
		    tr("Save delivery info as PDF file"), fileName,
		    tr("PDF file (*.pdf)"));
		//, QString(), 0, QFileDialog::DontUseNativeDialog);
	}

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info \"%1\" to PDF was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings if attachPath was not set */
	if (attachPath.isNull()) {
		m_on_export_zfo_activate =
		    QFileInfo(fileName).absoluteDir().absolutePath();
		storeExportPath();
	}

	QTextDocument doc;
	doc.setHtml(messageDb->deliveryInfoHtmlToPdf(dmId));

	showStatusTextPermanently(tr("Printing of delivery info \"%1\" to "
	    "PDF. Please wait...").arg(dmId));

	QPrinter printer;
	printer.setOutputFileName(fileName);
	printer.setOutputFormat(QPrinter::PdfFormat);
	doc.print(&printer);

	showStatusTextWithTimeout(tr("Export of message delivery info "
	    "\"%1\" to PDF was successful.").arg(dmId));
}


/* ========================================================================= */
/*
 * Export selected message envelope as PDF file dialog.
 */
void MainWindow::exportMessageEnvelopeAsPDF(const QString &attachPath,
    qint64 dmID, const QDateTime &delivTime)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message envelope to "
		    "PDF was not successful!"));
		return;
	}

	qint64 dmId;
	QDateTime deliveryTime;

	if ((dmID == -1) || !delivTime.isValid()) {
		dmId = msgIdx.data().toLongLong();
		deliveryTime = msgDeliveryTime(msgIdx);
	} else {
		dmId = dmID;
		deliveryTime = delivTime;
	}
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(deliveryTime.isValid());

	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);
	const QString userName =
	    acntTopIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsMessageBase64(dmId);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);;
		msgBox.setWindowTitle(tr("Message export error!"));
		msgBox.setText(tr("Cannot export complete message '%1'.").
		    arg(dmId));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download the whole message before "
		        "exporting.") + "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId, deliveryTime)) {
				showStatusTextWithTimeout(tr("Export of message "
				"envelope \"%1\" to PDF was not successful!")
				.arg(dmId));
				return;
			} else {
				base64 = messageDb->msgsMessageBase64(dmId);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message "
			"envelope \"%1\" to PDF was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName =
	    createFilenameFromFormatString(globPref.message_filename_format,
	    QString::number(dmId), dbId, userName, "",
	    entry.dmDeliveryTime, entry.dmAcceptanceTime, entry.dmAnnotation,
	    entry.dmSender);

	if (attachPath.isNull()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".pdf";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".pdf";
	}

	if (dmID == -1) {
		fileName = QFileDialog::getSaveFileName(this,
		    tr("Save message envelope as PDF file"), fileName,
		    tr("PDF file (*.pdf)"));
	}

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message "
		    "envelope \"%1\" to PDF was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings if attachPath was not set */
	if (attachPath.isNull()) {
		m_on_export_zfo_activate =
		    QFileInfo(fileName).absoluteDir().absolutePath();
		storeExportPath();
	}

	QList<QString> accountData =
	    globAccountDbPtr->getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message "
		    "envelope \"%1\" to PDF was not successful!").arg(dmId));
		return;
	}

	QTextDocument doc;
	doc.setHtml(messageDb->envelopeInfoHtmlToPdf(dmId, accountData.at(0)));

	showStatusTextPermanently(tr("Printing of message envelope \"%1\" to "
	    "PDF. Please wait...").arg(dmId));

	QPrinter printer;
	printer.setOutputFileName(fileName);
	printer.setOutputFormat(QPrinter::PdfFormat);
	doc.print(&printer);

	showStatusTextWithTimeout(tr("Export of message envelope \"%1\" to "
	    "PDF was successful.").arg(dmId));
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
	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();
	qint64 dmId = msgIdx.data().toLongLong();
	QDateTime deliveryTime = msgDeliveryTime(msgIdx);
	Q_ASSERT(deliveryTime.isValid());

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	QByteArray base64 = messageDb->msgsMessageBase64(dmId);
	if (base64.isEmpty()) {
		QMessageBox msgBox(this);;
		msgBox.setWindowTitle(tr("Datovka - Export error!"));
		msgBox.setText(tr("Cannot export the message ") + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		  tr("First you must download message before its export..."));
		msgBox.exec();
		return;
	}

	QString fileName =
	    QString(TMP_ATTACHMENT_PREFIX "DDZ_%1.zfo").arg(dmId);
	Q_ASSERT(!fileName.isEmpty());

	if (fileName.isEmpty()) {
		return;
	}

	QByteArray data = QByteArray::fromBase64(base64);

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Message '%1' stored to "
		    "temporary file '%2'.").arg(dmId).arg(fileName));
		QDesktopServices::openUrl(QUrl("file:///" + fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		showStatusTextWithTimeout(tr("Message '%1' couldn't be "
		    "stored to temporary file.").arg(dmId));
		QMessageBox::warning(this,
		    tr("Error opening message '%1'.").arg(dmId),
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

	/* First column. */
	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();
	qint64 dmId = msgIdx.data().toLongLong();
	QDateTime deliveryTime = msgDeliveryTime(msgIdx);
	Q_ASSERT(deliveryTime.isValid());

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
	if (base64.isEmpty()) {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Datovka - Export error!"));
		msgBox.setText(tr("Cannot export the message ") + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		  tr("First you must download message before its export..."));
		msgBox.exec();
		return;
	}

	QString fileName =
	    QString(TMP_ATTACHMENT_PREFIX "DDZ_%1_info.zfo").arg(dmId);
	Q_ASSERT(!fileName.isEmpty());

	if (fileName.isEmpty()) {
		return;
	}

	QByteArray data = QByteArray::fromBase64(base64);

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Message delivery information "
		    "'%1' stored to temporary file '%2'.").arg(dmId)
		    .arg(fileName));
		QDesktopServices::openUrl(QUrl("file:///" + fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		showStatusTextWithTimeout(tr("Message delivery information "
		    "'%1' couldn't be stored to temporary file.").arg(dmId));
		QMessageBox::warning(this,
		    tr("Error opening message '%1'.").arg(dmId),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * View signature details.
 */
void MainWindow::showSignatureDetails(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* First column. */
	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();
	qint64 dmId = msgIdx.data().toLongLong();
	QDateTime deliveryTime = msgDeliveryTime(msgIdx);
	Q_ASSERT(deliveryTime.isValid());

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	if (0 == dbSet) {
		Q_ASSERT(0);
		return;
	}

	QDialog *signature_detail = new DlgSignatureDetail(*dbSet, dmId,
	    deliveryTime, this);
	signature_detail->exec();
}


/* ========================================================================= */
/*
* This is call if connection to ISDS fails. Message info for user is generated.
*/
void MainWindow::showConnectionErrorMessageBox(int status,
    const QString &accountName, QString isdsMsg)
/* ========================================================================= */
{
	QString msgBoxTitle;
	QString msgBoxContent;

	if (isdsMsg.isEmpty()) {
		isdsMsg = isds_strerror((isds_error)status);
	}

	switch(status) {
	case IE_NOT_LOGGED_IN:
		msgBoxTitle = accountName + ": " + tr("Authentication error!");
		msgBoxContent =
		    tr("It was not possible to connect to your Databox "
		    "from account \"%1\"").arg(accountName) + "." +
		    + "<br><br>" +
		    "<b>" + accountName + ": " + isdsMsg
		    + "</b>" + "<br><br>" +
		    tr("Please check your credentials including the test-"
			"environment setting and login method.") + "<br>" +
		    tr("It is possible that your password has expired - "
			"in this case, you need to use the official web "
			"interface of Datové schránky to change it.");
		break;

	case IE_PARTIAL_SUCCESS:
		msgBoxTitle = accountName +
		    ": " + tr("OTP authentication error!");
		msgBoxContent =
		    tr("It was not possible to connect to your Databox.")
		    + "<br><br>" +
		    "<b>" + isdsMsg + "</b>"
		    + "<br><br>" +
		    tr("Please check your credentials including the test-"
			"environment setting.") + "<br>" +
		    tr("It is aslo possible that your password has expired - "
			"in this case, you need to use the official web "
			"interface of Datové schránky to change it.");
		break;

	case IE_TIMED_OUT:
		msgBoxTitle = accountName +
		    ": " + tr("Connection to ISDS error!");
		msgBoxContent =
		    tr("It was not possible to establish a connection "
		    "within a set time.") + "<br><br>" +
		    "<b>" + isdsMsg + "</b>"
		    + "<br><br>" +
		    tr("This is either caused by an extremely slow and/or "
		    "unstable connection or by an improper setup.") + "<br>" +
		    tr("Please check your internet connection and try again.")
		    + "<br><br>" + tr("It might be necessary to use a proxy to "
		    "connect to the server. If yes, please set it up in the "
		    "File/Proxy settings menu.");
		break;

	case IE_INVAL:
	case IE_ENUM:
	case IE_NOMEM:
	case IE_INVALID_CONTEXT:
	case IE_NOTSUP:
	case IE_HTTP:
	case IE_ERROR:
		msgBoxTitle = accountName +
		    ": " + tr("Datovka internal error!");
		msgBoxContent =
		    tr("It was not possible to establish a connection "
		    "to server Datové Schránky.") + "<br><br>" +
		    tr("ISDS error: ") + isdsMsg + "<br><br>" +
		    "<b>" + tr("Datovka internal error!") + "</b>" + "<br><br>" +
		    tr("Please check your internet connection and try again.");
		break;

	default:
		msgBoxTitle = accountName +
		    ": " + tr("Connection to ISDS error!");
		msgBoxContent =
		    tr("It was not possible a connection between your computer "
		    "and the server of Datove schranky.") + "<br><br>" +
		    "<b>" + tr("Connection to ISDS failed!") + "</b>" + "<br><br>" +
		    tr("ISDS error: ") + isdsMsg + "<br><br>" +
		    tr("This is usually caused by either lack of internet "
		    "connection or by a firewall on the way.") + "<br>" +
		    tr("Please check your internet connection and try again.")
		    + "<br><br>" + tr("It might be necessary to use a proxy to "
		    "connect to the server. If yes, please set it up in the "
		    "File/Proxy settings menu.");
		break;
	}

	showStatusTextWithTimeout(tr("It was not possible to connect to your"
	    " databox from account \"%1\".").arg(accountName));

	QMessageBox::critical(this, msgBoxTitle, msgBoxContent, QMessageBox::Ok);
}


/* ========================================================================= */
/*
* Check if connection to ISDS fails.
*/
bool MainWindow::checkConnectionError(int status, const QString &accountName,
    const QString &isdsMsg, MainWindow *mw)
/* ========================================================================= */
{
	switch (status) {
	case IE_SUCCESS:
		if (0 != mw) {
			mw->statusOnlineLabel->setText(tr("Mode: online"));
		}
		return true;
		break;
	default:
		logError("Account '%s'; %d %s\n",
		    accountName.toUtf8().constData(),
		    status, isds_strerror((isds_error) status));
		if (0 != mw) {
			mw->showConnectionErrorMessageBox(status, accountName,
			    isdsMsg);
		}
		return false;
		break;
	}
}


/* ========================================================================= */
/*
* Login to ISDS server by username and password only.
*/
bool MainWindow::loginMethodUserNamePwd(
    AccountModel::SettingsMap &accountInfo,
    MainWindow *mw, const QString &pwd)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;
	bool ret = false;
	QString isdsMsg;
	const QString userName = accountInfo.userName();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	if (!isdsSessions.holdsSession(userName)) {
		isdsSessions.createCleanSession(userName,
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString usedPwd = accountInfo.password();

	// is mainwindow, show dialogs
	if (0 != mw) {
		// when pwd is not stored in settings then open account dialog
		if (usedPwd.isEmpty()) {
			QDialog *editAccountDialog = new DlgCreateAccount(
			    userName, DlgCreateAccount::ACT_PWD, mw);
			if (QDialog::Accepted == editAccountDialog->exec()) {
				usedPwd = accountInfo.password();
			} else {
				accountInfo.setRememberPwd(false);
				accountInfo.setPassword("");
				mw->showStatusTextWithTimeout(
				    tr("It was not possible to connect to "
				    "your databox from account \"%1\".")
				    .arg(accountInfo.accountName()));
				return false;
			}
		}
		// login to ISDS
		status = isdsLoginUserName(isdsSessions.session(userName),
		    userName, usedPwd, accountInfo.isTestAccount());
		isdsMsg = isdsLongMessage(isdsSessions.session(userName));
		ret = checkConnectionError(status, accountInfo.accountName(),
		    isdsMsg, mw);

		// if error, to show account dialog again
		while (!ret) {
			QDialog *editAccountDialog = new DlgCreateAccount(
			    userName, DlgCreateAccount::ACT_PWD, mw);
			if (QDialog::Accepted == editAccountDialog->exec()) {
				usedPwd = accountInfo.password();
				status = isdsLoginUserName(isdsSessions.session(userName),
				    userName, usedPwd, accountInfo.isTestAccount());
				isdsMsg = isdsLongMessage(isdsSessions.session(userName));
				ret = checkConnectionError(status,
				    accountInfo.accountName(), isdsMsg, mw);
			} else {
				accountInfo.setRememberPwd(false);
				accountInfo.setPassword("");
				mw->showStatusTextWithTimeout(
				    tr("It was not possible to connect to "
				    "your databox from account \"%1\".")
				    .arg(accountInfo.accountName()));
				return false;
			}
		}
		// save new pwd to settings
		mw->saveSettings();
		ret = true;

	// is CLI, disable dialogs
	} else {
		if (!pwd.isEmpty()) {
			usedPwd = pwd;
		} else if (usedPwd.isEmpty()) {
			logError("Unknown password for account '%s'.\n",
			    accountInfo.accountName().toUtf8().constData());
			return false;
		}
		status = isdsLoginUserName(isdsSessions.session(userName),
		    userName, usedPwd, accountInfo.isTestAccount());
		ret = (IE_SUCCESS == status);
	}

	/* Set longer time-out. */
	isdsSessions.setSessionTimeout(userName,
	    globPref.isds_download_timeout_ms);

	return ret;
}


/* ========================================================================= */
/*
 * Converts PKCS #12 certificate into PEM format.
 */
bool MainWindow::p12CertificateToPem(const QString &p12Path,
    const QString &certPwd, QString &pemPath, const QString &userName)
/* ========================================================================= */
{
	QByteArray p12Data;
	QByteArray pemData;

	pemPath = QString();

	{
		/* Read the data. */
		QFile p12File(p12Path);
		if (!p12File.open(QIODevice::ReadOnly)) {
			return false;
		}

		p12Data = p12File.readAll();
		p12File.close();
	}

	void *pem = NULL;
	size_t pem_size;
	if (0 != p12_to_pem((void *) p12Data.constData(), p12Data.size(),
	        certPwd.toUtf8().constData(), &pem, &pem_size)) {
		return false;
	}

	QFileInfo fileInfo(p12Path);
	QString pemTmpPath = globPref.confDir() + QDir::separator() +
	    userName + "_" + fileInfo.fileName() + "_.pem";

	QFile pemFile(pemTmpPath);
	if (!pemFile.open(QIODevice::WriteOnly)) {
		free(pem); pem = NULL;
		return false;
	}

	if ((long) pem_size != pemFile.write((char *) pem, pem_size)) {
		free(pem); pem = NULL;
		return false;
	}

	free(pem); pem = NULL;

	if (!pemFile.flush()) {
		return false;
	}

	pemFile.close();

	pemPath = pemTmpPath;

	return true;
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate only.
*/
bool MainWindow::loginMethodCertificateOnly(
    AccountModel::SettingsMap &accountInfo,
    MainWindow *mw, const QString &key)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;
	bool ret = false;
	const QString userName = accountInfo.userName();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	if (!isdsSessions.holdsSession(userName)) {
		isdsSessions.createCleanSession(userName,
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString certPath = accountInfo.p12File();

	if (certPath.isEmpty()) {
		if (0 != mw) {
			QDialog *editAccountDialog = new DlgCreateAccount(userName,
			    DlgCreateAccount::ACT_CERT, mw);
			if (QDialog::Accepted == editAccountDialog->exec()) {
				certPath = accountInfo.p12File();
				mw->saveSettings();
			} else {
				mw->showStatusTextWithTimeout(
				    tr("It was not possible to connect to "
				    "your databox from account \"%1\".")
				    .arg(accountInfo.accountName()));
				return false;
			}
		} else {
			logError("Unknown certificate for account '%s'.\n",
			    accountInfo.accountName().toUtf8().constData());
			return false;
		}
	}

	QString passphrase = accountInfo._passphrase();
	/*
	 * Don't test for isEmpty()!
	 * See difference between isNull() and isEmpty().
	 */
	if (passphrase.isNull()) {
		if (0 != mw) {
			/* Ask the user for password. */
			bool ok;
			QString enteredText = QInputDialog::getText(
			    mw, tr("Password required"),
			    tr("Account: %1\n"
			        "User name: %2\n"
			        "Certificate file: %3\n"
			        "Enter password to unlock certificate file:")
			        .arg(accountInfo.accountName()).arg(userName)
			        .arg(certPath),
			    QLineEdit::Password, QString(), &ok);
			if (ok) {
				passphrase = enteredText;
			} else {
				/* Aborted. */
				return false;
			}
		} else if (!key.isNull()) {
			passphrase = key;
		} else {
			logError("Unknown certificate pass-phrase for "
			    "account '%s'.\n",
			    accountInfo.accountName().toUtf8().constData());
			return false;
		}
	}

	QString ext = QFileInfo(certPath).suffix().toUpper();
	if ("P12" == ext) {
		/* Read PKCS #12 file and convert to PEM. */
		QString createdPemPath;
		if (p12CertificateToPem(certPath, passphrase, createdPemPath,
		        userName)) {
			certPath = createdPemPath;
		} else {
			if (0 != mw) {
				QMessageBox::critical(mw,
				    tr("Cannot decode certificate."),
				    tr("The certificate file '%1' cannot be decoded by "
				        "using the supplied password.").arg(certPath),
				    QMessageBox::Ok);
			}
			logError("%s\n", "Cannot decode certificate.");
			return false;
		}
	} else if ("PEM" == ext) {
		/* TODO -- Check the passphrase. */
	} else {
		if (0 != mw) {
			QMessageBox::critical(mw,
			    tr("Certificate format not supported"),
			    tr("The certificate file '%1' suffix does not match "
			        "one of the supported file formats. Supported "
			        "suffixes are:").arg(certPath) + " p12, pem",
			    QMessageBox::Ok);
		}
		logError("%s\n", "Certificate format not supported.");
		return false;
	}

	status = isdsLoginSystemCert(isdsSessions.session(userName),
	    certPath, passphrase, accountInfo.isTestAccount());

	if (IE_SUCCESS == status) {
		/* Store the certificate password. */
		accountInfo._setPassphrase(passphrase);

		/*
		 * TODO -- Notify the user that he should protect his
		 * certificates with a password?
		 */
	} else {
		accountInfo.setRememberPwd(false);
		accountInfo.setPassword("");
	}

	mw->saveSettings();

	/* Set longer time-out. */
	isdsSessions.setSessionTimeout(userName,
	    globPref.isds_download_timeout_ms);

	return ret;
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate, username and password.
*/
bool MainWindow::loginMethodCertificateUserPwd(
    AccountModel::SettingsMap &accountInfo,
    MainWindow *mw, const QString &pwd, const QString &key)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	const QString userName = accountInfo.userName();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	if (!isdsSessions.holdsSession(userName)) {
		isdsSessions.createCleanSession(userName,
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString certPath = accountInfo.p12File();
	QString usedPwd = accountInfo.password();

	if (usedPwd.isEmpty() || certPath.isEmpty()) {
		if (0 != mw) {
			QDialog *editAccountDialog = new DlgCreateAccount(userName,
			    DlgCreateAccount::ACT_CERTPWD, mw);
			if (QDialog::Accepted == editAccountDialog->exec()) {
				certPath = accountInfo.p12File();
				usedPwd = accountInfo.password();
			} else {
				mw->showStatusTextWithTimeout(
				    tr("It was not possible to connect to "
				    "your databox from account \"%1\".")
				    .arg(accountInfo.accountName()));
				return false;
			}
		} else if (!pwd.isEmpty()) {
			usedPwd = pwd;
		} else {
			logError("Unknown certificate or password for account '%s'.\n",
			    accountInfo.accountName().toUtf8().constData());
			return false;
		}
	}

	QString passphrase = accountInfo._passphrase();
	/*
	 * Don't test for isEmpty()!
	 * See difference between isNull() and isEmpty().
	 */
	if (passphrase.isNull()) {
		if (0 != mw) {
			/* Ask the user for password. */
			bool ok;
			QString enteredText = QInputDialog::getText(
			    mw, tr("Password required"),
			    tr("Account: %1\n"
			        "User name: %2\n"
			        "Certificate file: %3\n"
			        "Enter password to unlock certificate file:")
			        .arg(accountInfo.accountName()).arg(userName)
			        .arg(certPath),
			    QLineEdit::Password, QString(), &ok);
			if (ok) {
				passphrase = enteredText;
			} else {
				/* Aborted. */
				return false;
			}
		} else if (!key.isNull()) {
			passphrase = key;
		} else {
			logError("Unknown certificate pass-phrase for "
			    "account '%s'.\n",
			    accountInfo.accountName().toUtf8().constData());
			return false;
		}
	}

	QString ext = QFileInfo(certPath).suffix().toUpper();
	if ("P12" == ext) {
		/* Read PKCS #12 file and convert to PEM. */
		QString createdPemPath;
		if (p12CertificateToPem(certPath, passphrase, createdPemPath,
		        userName)) {
			certPath = createdPemPath;
		} else {
			if (0 != mw) {
				QMessageBox::critical(mw,
				    tr("Cannot decode certificate."),
				    tr("The certificate file '%1' cannot be decoded by "
				        "using the supplied password.").arg(certPath),
				    QMessageBox::Ok);
			}
			logError("%s\n", "Cannot decode certificate.");
			return false;
		}
	} else if ("PEM" == ext) {
		/* TODO -- Check the passphrase. */
	} else {
		if (0 != mw) {
			QMessageBox::critical(mw,
			    tr("Certificate format not supported"),
			    tr("The certificate file '%1' suffix does not match "
			        "one of the supported file formats. Supported "
			        "suffixes are:").arg(certPath) + " p12, pem",
			    QMessageBox::Ok);
		}
		logError("%s\n", "Certificate format not supported.");
		return false;
	}

	status = isdsLoginUserCertPwd(isdsSessions.session(userName),
	    userName, usedPwd, certPath, passphrase,
	    accountInfo.isTestAccount());

	QString isdsMsg = isdsLongMessage(isdsSessions.session(userName));

	if (IE_SUCCESS == status) {
		/* Store the certificate password. */
		accountInfo._setPassphrase(passphrase);

		/*
		 * TODO -- Notify the user that he should protect his
		 * certificates with a password?
		 */
	}

	isdsSessions.setSessionTimeout(userName,
	    globPref.isds_download_timeout_ms); /* Set longer time-out. */

	return checkConnectionError(status, accountInfo.accountName(),
	    isdsMsg, mw);
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate and databox ID.
*/
bool MainWindow::loginMethodCertificateIdBox(
    AccountModel::SettingsMap &accountInfo, MainWindow *mw)
/* ========================================================================= */
{
#if 0 /* This function is unused */
	isds_error status = IE_ERROR;

	const QString userName = accountInfo.userName();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	if (!isdsSessions.holdsSession(userName)) {
		isdsSessions.createCleanSession(userName,
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString certPath = accountInfo.p12File();
	QString idBox;

	QDialog *editAccountDialog = new DlgCreateAccount(userName,
	    DlgCreateAccount::ACT_IDBOX, this);
	if (QDialog::Accepted == editAccountDialog->exec()) {
		certPath = accountInfo.p12File();
		idBox = accountInfo.userName();
		saveSettings();
	} else {
		showStatusTextWithTimeout(tr("It was not possible to "
		    "connect to your databox from account \"%1\".")
		    .arg(accountInfo.accountName()));
		return false;
	}

	QString passphrase = accountInfo._passphrase();
	/*
	 * Don't test for isEmpty()!
	 * See difference between isNull() and isEmpty().
	 */
	if (passphrase.isNull()) {
		/* Ask the user for password. */
		bool ok;
		QString enteredText = QInputDialog::getText(
		    this, tr("Password required"),
		    tr("Account: %1\n"
		        "User name: %2\n"
		        "Certificate file: %3\n"
		        "Enter password to unlock certificate file:")
		        .arg(accountInfo.accountName()).arg(userName)
		        .arg(certPath),
		    QLineEdit::Password, QString(), &ok);
		if (ok) {
			passphrase = enteredText;
		} else {
			/* Aborted. */
			return false;
		}
	}

	QString ext = QFileInfo(certPath).suffix().toUpper();
	if ("P12" == ext) {
		/* Read PKCS #12 file and convert to PEM. */
		QString createdPemPath;
		if (p12CertificateToPem(certPath, passphrase, createdPemPath,
		        userName)) {
			certPath = createdPemPath;
		} else {
			QMessageBox::critical(this,
			    tr("Cannot decode certificate."),
			    tr("The certificate file '%1' cannot be decoded by "
			        "using the supplied password.").arg(certPath),
			    QMessageBox::Ok);
			return false;
		}
	} else if ("PEM" == ext) {
		/* TODO -- Check the passphrase. */
	} else {
		QMessageBox::critical(this,
		    tr("Certificate format not supported"),
		    tr("The certificate file '%1' suffix does not match "
		        "one of the supported file formats. Supported "
		        "suffixes are:").arg(certPath) + " p12, pem",
		    QMessageBox::Ok);
		return false;
	}

	status = isdsLoginUserCert(isdsSessions.session(userName),
	    idBox, certPath, passphrase, accountInfo.isTestAccount());

	if (IE_SUCCESS == status) {
		/* Store the certificate password. */
		accountInfo._setPassphrase(passphrase);

		/*
		 * TODO -- Notify the user that he should protect his
		 * certificates with a password?
		 */
	}

	isdsSessions.setSessionTimeout(userName,
	    globPref.isds_download_timeout_ms); /* Set longer time-out. */

	QString isdsMsg = isdsLongMessage(isdsSessions.session(userName));

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog, isdsMsg);
#else
	(void) accountInfo;
	(void) mw;
	return false;
#endif
}


/* ========================================================================= */
/*
* Login to ISDS server by username, password and OTP code.
*/
bool MainWindow::loginMethodUserNamePwdOtp(
    AccountModel::SettingsMap &accountInfo,
    MainWindow *mw, const QString &pwd, const QString &otp)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	const QString userName = accountInfo.userName();

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	if (!isdsSessions.holdsSession(userName)) {
		isdsSessions.createCleanSession(userName,
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString usedPwd = accountInfo.password();
	if (usedPwd.isEmpty()) {
		if (0 != mw) {
			QDialog *editAccountDialog = new DlgCreateAccount(userName,
			    DlgCreateAccount::ACT_PWD, mw);
			if (QDialog::Accepted == editAccountDialog->exec()) {
				usedPwd = accountInfo.password();
				mw->saveSettings();
			} else if (!pwd.isEmpty()) {
				usedPwd = pwd;
			} else {
				mw->showStatusTextWithTimeout(
				    tr("It was not possible to connect to "
				    "your databox from account \"%1\".")
				    .arg(accountInfo.accountName()));
				return false;
			}
		} else {
			logError("Unknown password for account '%s'.\n",
			    accountInfo.accountName().toUtf8().constData());
			return false;
		}
	}

	QString isdsMsg;

	/* HOTP - dialog info */
	QString msgTitle = tr("Enter OTP security code");
	QString msgBody = tr("Account \"%1\" requires authentication via OTP "
	    "<br/>security code for connection to databox.")
	    .arg(accountInfo.accountName()) + "<br/><br/>" +
	    tr("Enter OTP security code for account") + "<br/><b>"
	    + accountInfo.accountName() + " </b>(" + userName + ").";

	isds_otp_resolution otpres = OTP_RESOLUTION_SUCCESS;

	/* SMS TOTP */
	/* First phase - send SMS request */
	if (accountInfo.loginMethod() == LIM_TOTP) {
		if (0 == mw) {
			logError("%s\n", "SMS authentication is not supported "
			     "without running GUI.");
			return false;
		}

		/* show Premium SMS request dialog */
		QMessageBox::StandardButton reply = QMessageBox::question(mw,
		    tr("SMS code for account ") + accountInfo.accountName(),
		    tr("Account \"%1\" requires authentication via security code "
		    "for connection to databox.").arg(accountInfo.accountName())
		    + "<br/>" +
		    tr("Security code will be sent you via Premium SMS.") +
		    "<br/><br/>" +
		    tr("Do you want to send Premium SMS with "
		    "security code into your mobile phone?"),
		    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

		if (reply == QMessageBox::No) {
			mw->showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your data box from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}

		status = isdsLoginUserOtp(isdsSessions.session(userName),
		    userName, usedPwd, accountInfo.isTestAccount(),
		    accountInfo.loginMethod(), QString(), otpres);

		isdsMsg = isdsLongMessage(isdsSessions.session(userName));

		isdsSessions.setSessionTimeout(userName,
		    globPref.isds_download_timeout_ms); /* Set time-out. */

		if (IE_PARTIAL_SUCCESS == status) {
			msgTitle = tr("Enter SMS security code");
			msgBody = tr("SMS security code for account \"%1\"<br/>"
			    "has been sent on your mobile phone...")
			    .arg(accountInfo.accountName())
			     + "<br/><br/>" +
			    tr("Enter SMS security code for account")
			    + "<br/><b>"
			    + accountInfo.accountName()
			    + " </b>(" + userName + ").";
		} else if (IE_NOT_LOGGED_IN == status) {
			msgTitle = tr("Authentication by SMS failed");
			msgBody = tr("It was not possible sent SMS with OTP "
			    "security code for account \"%1\"")
			    .arg(accountInfo.accountName()) + "<br/><br/>" +
			    "<b>" + isdsMsg + "</b>" + "<br/><br/>" +
			    tr("Please try again later or you have to use the "
			    "official web interface of Datové schránky for "
			    "access to your data box.");
			QMessageBox::critical(mw, msgTitle, msgBody,
			    QMessageBox::Ok);
			mw->showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your data box from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		} else {
			/* There were other errors. */
			msgTitle = tr("Login error");
			msgBody = tr("An error occurred while preparing "
			    "request for SMS with OTP security code.") +
			    "<br/><br/>" +
			    tr("Please try again later or you have to use the "
			    "official web interface of Datové schránky for "
			    "access to your data box.");
			QMessageBox::critical(mw, msgTitle, msgBody,
			    QMessageBox::Ok);
			mw->showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your data box from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}
	}

	/* Second phase - Authentization with OTP code */
	bool ok;
	bool repeat = false;
	int count = 0;
	QString otpCode;
	do {
		count++;
		if (0 != mw) {
			do {
				otpCode = QInputDialog::getText(mw, msgTitle,
				    msgBody, QLineEdit::Normal, otpCode, &ok,
				    Qt::WindowStaysOnTopHint);
				if (!ok) {
					mw->showStatusTextWithTimeout(
					    tr("It was not possible to "
					    "connect to your databox from "
					    "account \"%1\".").
					    arg(accountInfo.accountName()));
					return false;
				}
			} while (otpCode.isEmpty());
		} else if (!otp.isEmpty()) {
			otpCode = otp;
		}

		/* sent security code to ISDS */
		status = isdsLoginUserOtp(isdsSessions.session(userName),
		    userName, usedPwd, accountInfo.isTestAccount(),
		    accountInfo.loginMethod(), otpCode, otpres);

		isdsMsg = isdsLongMessage(isdsSessions.session(userName));

		isdsSessions.setSessionTimeout(userName,
		    globPref.isds_download_timeout_ms); /* Set time-out. */

		/* OTP login notification */
		if (status == IE_NOT_LOGGED_IN) {
			switch (otpres) {
			case OTP_RESOLUTION_BAD_AUTHENTICATION:
				msgTitle = tr("Enter security code again");
				msgBody = tr("The security code for "
				    "this account was not accepted!")
				    + "<br/><br/>" +
				    tr("Account: ") + "<b>"
				    + accountInfo.accountName()
				    + " </b>(" + userName + ")"
				    + "<br/><br/>" +
				    tr("Enter the correct security code again.");
				if (count > 1) {
					repeat = false;
				} else {
					repeat = true;
					otpCode.clear();
				}
				break;
			case OTP_RESOLUTION_ACCESS_BLOCKED:
			case OTP_RESOLUTION_PASSWORD_EXPIRED:
			case OTP_RESOLUTION_UNAUTHORIZED:
			default:
				repeat = false;
				break;
			}
		} else {
			repeat = false;
		}

	} while (repeat);

	return checkConnectionError(status, accountInfo.accountName(),
	    isdsMsg, mw);
}


/* ========================================================================= */
/*
 * Connect to databox from exist account
 */
bool MainWindow::connectToIsds(const QString &userName, MainWindow *mw,
    const QString &pwd, const QString &otpKey)
/* ========================================================================= */
{
	bool loginRet = false;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	/* Check password expiration. */
	AccountModel::SettingsMap &accountInfo =
	    AccountModel::globAccounts[userName];
	if (!accountInfo._pwdExpirDlgShown()) {
		/* Notify only once. */
		accountInfo._setPwdExpirDlgShown(true);

		QString dbDateTimeString = globAccountDbPtr->getPwdExpirFromDb(
		    userName + "___True");
		const QDateTime dbDateTime = QDateTime::fromString(
		    dbDateTimeString, "yyyy-MM-dd HH:mm:ss.000000");
		const QDate dbDate = dbDateTime.date();

		if (dbDate.isValid()) {
			qint64 daysTo = QDate::currentDate().daysTo(dbDate);
			if (daysTo < PWD_EXPIRATION_NOTIFICATION_DAYS) {
				logWarning("Password of account '%lu' expires "
				    "in %lu days.\n",
				    accountInfo.accountName().toUtf8().constData(),
				    daysTo);
				if ((0 != mw) && (QMessageBox::Yes ==
				    mw->showDialogueAboutPwdExpir(
				        accountInfo.accountName(),
				        userName, daysTo,
				        dbDateTime))) {
					mw->showStatusTextWithTimeout(
					    tr("Change password of account "
					        "\"%1\".")
					    .arg(accountInfo.accountName()));
					QString dbId = globAccountDbPtr->dbId(
					    userName + "___True");
					QDialog *changePwd = new DlgChangePwd(
					    dbId, userName, mw);
					changePwd->exec();
				}
			}
		}
	}

	/* Login method based on username and password */
	if (accountInfo.loginMethod() == LIM_USERNAME) {
		loginRet = loginMethodUserNamePwd(accountInfo, mw, pwd);

	/* Login method based on certificate only */
	} else if (accountInfo.loginMethod() == LIM_CERT) {
		loginRet = loginMethodCertificateOnly(accountInfo, mw, otpKey);

	/* Login method based on certificate together with username */
	} else if (accountInfo.loginMethod() == LIM_USER_CERT) {

		loginRet = loginMethodCertificateUserPwd(accountInfo,
		    mw, pwd, otpKey);

		/* TODO - next method is situation when certificate will be used
		 * and password missing. The username shifts meaning to box ID.
		 * This is used for hosted services. It is not dokumented and
		 * we not support this method now.

		if (accountInfo.password().isEmpty()) {
			return loginMethodCertificateIdBox(accountInfo, mw);
		}
		*/

	/* Login method based username, password and OTP */
	} else {
		loginRet = loginMethodUserNamePwdOtp(accountInfo,
		    mw, pwd, otpKey);
	}

	if (!loginRet) {
		/* Break on error. */
		return loginRet;
	}

	if (!getOwnerInfoFromLogin(userName)) {
		logWarning("Owner information for account '%s' (login %s) "
		    "could not be acquired.\n",
		    accountInfo.accountName().toUtf8().constData(),
		    userName.toUtf8().constData());
	}
	if (!getUserInfoFromLogin(userName)) {
		logWarning("User information for account '%s' (login %s) "
		    "could not be acquired.\n",
		    accountInfo.accountName().toUtf8().constData(),
		    userName.toUtf8().constData());
	}
	if (!getPasswordInfoFromLogin(userName)) {
		logWarning("Password information for account '%s' (login %s) "
		    "could not be acquired.\n",
		    accountInfo.accountName().toUtf8().constData(),
		    userName.toUtf8().constData());
	}

	/* Get account information if possible. */
	/*
	 * TODO -- Is '___True' somehow related to the testing state
	 * of an account?
	 *
	 * TODO -- The account information should be updated periodically.
	 * Currently they are only acquired when they are missing.

	QString dbId = globAccountDbPtr->dbId(userName + "___True");
	if (dbId.isEmpty()) {
		// Acquire user information.
		qWarning() << "Missing user entry for" << userName
		    << "in account db.";

		if (!getOwnerInfoFromLogin(userName)) {
			return false;
		}

		if (!getUserInfoFromLogin(userName)) {
			return false;
		}

		dbId = globAccountDbPtr->dbId(userName + "___True");
	}
	Q_ASSERT(!dbId.isEmpty());
	 */

	return loginRet;
}


/* ========================================================================= */
/*
 * First connect to databox from new account
 */
bool MainWindow::firstConnectToIsds(AccountModel::SettingsMap &accountInfo,
    bool showDialog)
/* ========================================================================= */
{
	debugFuncCall();

	bool ret = false;

	/* Login method based on username and password */
	if (accountInfo.loginMethod() == LIM_USERNAME) {
		ret = loginMethodUserNamePwd(accountInfo,
		    showDialog ? this : 0);

	/* Login method based on certificate only */
	} else if (accountInfo.loginMethod() == LIM_CERT) {
		ret = loginMethodCertificateOnly(accountInfo,
		    showDialog ? this : 0);

	/* Login method based on certificate together with username */
	} else if (accountInfo.loginMethod() == LIM_USER_CERT) {

		ret = loginMethodCertificateUserPwd(accountInfo,
		    showDialog ? this : 0);

		/* TODO - next method is situation when certificate will be used
		 * and password missing. The username shifts meaning to box ID.
		 * This is used for hosted services. It is not dokumented and
		 * we not support this method now.

		if (accountInfo.password().isEmpty()) {
			return loginMethodCertificateIdBox(accountInfo,
			    showDialog ? mw : 0);
		}
		*/

	/* Login method based username, password and OTP */
	} else {
		ret = loginMethodUserNamePwdOtp(accountInfo,
		    showDialog ? this : 0);
	}

	if (ret) {
		if (!getOwnerInfoFromLogin(accountInfo.userName())) {
			//TODO: return false;
		}
		if (!getUserInfoFromLogin(accountInfo.userName())) {
			//TODO: return false;
		}
		if (!getPasswordInfoFromLogin(accountInfo.userName())) {
			//TODO: return false;
		}
	}

	return ret;
}


/* ========================================================================= */
/*
 * Check if some worker is not working on the background and show
 * dialog if user want to close application
 */
void MainWindow::closeEvent(QCloseEvent *event)
/* ========================================================================= */
{
	debugFuncCall();

	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("Datovka"));

	/* check if some worker works now
	 * if any worker is not working, lock mutex and show exit dialog,
	 * else waits for worker until he is done.
	*/
	if (Worker::downloadMessagesMutex.tryLock()) {
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setText(tr("Do you want to close application Datovka?"));
		msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::No == msgBox.exec()) {
			event->ignore();
		}
		Worker::downloadMessagesMutex.unlock();

	} else {
		event->ignore();
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText(tr("Datovka cannot be closed now because "
		    "downloading of messages on the background is running..."));
		msgBox.setInformativeText(tr("Wait until the action will "
		    "finished and try again."));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	}
}


/* ========================================================================= */
/*
 * Updates the account model according to the change properties.
 */
void MainWindow::updateAccountListEntry(const QString &userName)
/* ========================================================================= */
{
	debugSlotCall();

	int row = ui->accountList->model()->rowCount();
	QModelIndex index;
	for (int i = 0; i < row; i++) {
		index = ui->accountList->model()->index(i,0);
		if (userName == index.data(ROLE_ACNT_USER_NAME).toString()) {
			QStandardItem *accountItem =
			    m_accountModel.itemFromIndex(index);
			accountItem->setText(
			    AccountModel::globAccounts[userName].accountName());
		}
	}
}


/* ========================================================================= */
/*
 * Verify if is a connection to ISDS and databox exists for a new account
 */
void MainWindow::getAccountUserDataboxInfo(
    AccountModel::SettingsMap accountInfo)
/* ========================================================================= */
{
	debugSlotCall();

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!firstConnectToIsds(accountInfo, true)) {
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

	QModelIndex index = m_accountModel.addAccount(accountInfo.accountName(),
	    accountInfo);

	qDebug() << "Changing selection" << index;
	/* get current account model */
	ui->accountList->selectionModel()->setCurrentIndex(index,
	    QItemSelectionModel::ClearAndSelect);
	/* Expand the tree. */
	ui->accountList->expand(index);
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

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	messageItemsSetProcessStatus(firstMsgColumnIdxs, procSt);
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
	if (!AccountModel::nodeTypeIsReceived(ui->accountList->
	        selectionModel()->currentIndex())) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();

	DbMsgsTblModel *messageModel = (DbMsgsTblModel *)
	    m_messageListProxyModel.sourceModel();
	Q_ASSERT(0 != messageModel);

	for (QModelIndexList::const_iterator it = firstMsgColumnIdxs.begin();
	     it != firstMsgColumnIdxs.end(); ++it) {
		qint64 dmId = it->data().toLongLong();
		QDateTime deliveryTime = msgDeliveryTime(*it);
		Q_ASSERT(deliveryTime.isValid());

		MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime,
		    false);
		Q_ASSERT(0 != messageDb);

		messageDb->smsgdtSetLocallyRead(dmId, read);

		/*
		 * Mark message as read without reloading
		 * the whole model.
		 */
		messageModel->overrideRead(dmId, read);
		/* Inform the view that the model has changed. */
		emit messageModel->dataChanged(
		    it->sibling(it->row(), 0),
		    it->sibling(it->row(), messageModel->columnCount() - 1));

	}

	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(ui->accountList->
	    selectionModel()->currentIndex());
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
	if (!AccountModel::nodeTypeIsReceived(ui->accountList->
	        selectionModel()->currentIndex())) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userNameFromItem(), this);
	Q_ASSERT(0 != dbSet);

	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();

	DbMsgsTblModel *messageModel = (DbMsgsTblModel *)
	    m_messageListProxyModel.sourceModel();
	Q_ASSERT(0 != messageModel);

	for (QModelIndexList::const_iterator it = firstMsgColumnIdxs.begin();
	     it != firstMsgColumnIdxs.end(); ++it) {
		qint64 dmId = it->data().toLongLong();
		QDateTime deliveryTime = msgDeliveryTime(*it);
		Q_ASSERT(deliveryTime.isValid());

		MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime,
		    false);
		Q_ASSERT(0 != messageDb);

		messageDb->msgSetProcessState(dmId, state, false);

		/*
		 * Mark message as read without reloading
		 * the whole model.
		 */
		messageModel->overrideProcessing(dmId, state);
		/* Inform the view that the model has changed. */
		emit messageModel->dataChanged(
		    it->sibling(it->row(), 0),
		    it->sibling(it->row(), messageModel->columnCount() - 1));

	}

	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(ui->accountList->
	    selectionModel()->currentIndex());
}


/* ========================================================================= */
/*
 * Show advanced message search dialogue.
 */
void MainWindow::showMsgAdvancedSearchDlg(void)
/* ========================================================================= */
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

	QPair <QString, MessageDbSet *> userNameAndMsgDbSet;
	QList< QPair<QString, MessageDbSet *> > messageDbList;

	/* get pointer to database for current accounts */
	QModelIndex currIndex = ui->accountList->currentIndex();
	currIndex = AccountModel::indexTop(currIndex);
	const QString userName =
	    currIndex.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	userNameAndMsgDbSet.first = userName;
	userNameAndMsgDbSet.second = dbSet;
	messageDbList.append(userNameAndMsgDbSet);

	/* get pointer to database for other accounts */
	for (int i = 0; i < ui->accountList->model()->rowCount(); i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		if (currIndex != index) {
			const QString userName =
			    index.data(ROLE_ACNT_USER_NAME).toString();
			Q_ASSERT(!userName.isEmpty());
			MessageDbSet *dbSet = accountDbSet(userName, this);
			userNameAndMsgDbSet.first = userName;
			userNameAndMsgDbSet.second = dbSet;
			messageDbList.append(userNameAndMsgDbSet);
		}
	}

	dlgMsgSearch = new DlgMsgSearch(messageDbList, userName, this,
	    Qt::Window);
	connect(dlgMsgSearch, SIGNAL(focusSelectedMsg(QString, qint64, QString, int)),
	    this, SLOT(messageItemFromSearchSelection(QString, qint64, QString, int)));
	connect(dlgMsgSearch, SIGNAL(finished(int)),
	    this, SLOT(msgAdvancedDlgFinished(int)));
	dlgMsgSearch->show();
	m_searchDlgActive = true;
}


/* ========================================================================= */
/*
 * On message dialogue exit.
 */
void MainWindow::msgAdvancedDlgFinished(int result)
/* ========================================================================= */
{
	(void) result;

	debugSlotCall();

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

	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("Password expiration"));
	msgBox.setIcon(QMessageBox::Information);
	if (days < 0) {
		msgBox.setText(tr("According to the last available information, "
		    "your password for account '%1' (login '%2') "
		    "expired %3 days ago (%4).")
		    .arg(accountName).arg(userName).arg(days*(-1))
		    .arg(dateTime.toString("dd.MM.yyyy hh:mm:ss")));
		msgBox.setInformativeText(tr("You have to change your password "
		    "from the ISDS web interface. "
		    "Your new password will be valid for 90 days."));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
	} else {
		msgBox.setText(tr("According to the last available information, "
		    "your password for account '%1' (login '%2') "
		    "will expire in %3 days (%4).")
		    .arg(accountName).arg(userName).arg(days)
		    .arg(dateTime.toString("dd.MM.yyyy hh:mm:ss")));
		msgBox.setInformativeText(tr("You can change your password now, "
		    "or later using the 'Change password' command. "
		    "Your new password will be valid for 90 days.\n\n"
		    "Change password now?"));
		msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
		msgBox.setDefaultButton(QMessageBox::No);
	}

	return msgBox.exec();
}


/* ========================================================================= */
/*
 * Show remaining PDZ credit.
 */
QString MainWindow::getPDZCreditFromISDS(void)
/* ========================================================================= */
{
	debugFuncCall();

	QString str("0");

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);
	const QString userName =
	    acntTopIdx.data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return str;
		}
	}

	isds_error status;
	long int credit;
	char *email = NULL;
	struct isds_list *history = NULL;

	status = isds_get_commercial_credit(isdsSessions.session(userName),
	    dbId.toStdString().c_str(), NULL, NULL, &credit, &email, &history);

	isds_list_free(&history);

	if (IE_SUCCESS != status) {
		qDebug() << status <<
		    isdsLongMessage(isdsSessions.session(userName));
		return str;
	}

	if (credit > 0) {
		str = programLocale.toString((float)credit / 100, 'f', 2);
	}

	return str;
}


/* ========================================================================= */
/*
 * Show message time stamp expiration dialogue.
 */
void MainWindow::showMsgTmstmpExpirDialog(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* Generate dialog showing message content. */
	QDialog *timestampExpirDialog = new TimestampExpirDialog(this);
	connect(timestampExpirDialog,
	    SIGNAL(returnAction(enum TimestampExpirDialog::TSaction)),
	    this,
	    SLOT(prepareMsgTmstmpExpir(enum TimestampExpirDialog::TSaction)));
	timestampExpirDialog->exec();
}


/* ========================================================================= */
/*
 * Prepare message timestamp expiration based on action.
 */
void MainWindow::prepareMsgTmstmpExpir(
    enum TimestampExpirDialog::TSaction action)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex index;
	QString userName;
	bool includeSubdir = false;
	QString importDir;
	QStringList fileList, filePathList;
	QStringList nameFilter("*.zfo");
	QDir directory(QDir::home());
	fileList.clear();
	filePathList.clear();

	switch (action) {
	case TimestampExpirDialog::CHECK_TIMESTAMP_CURRENT:
		/* Process the selected account. */
		index = ui->accountList->currentIndex();
		index = AccountModel::indexTop(index);
		userName = index.data(ROLE_ACNT_USER_NAME).toString();
		Q_ASSERT(!userName.isEmpty());
		showStatusTextPermanently(tr("Checking time stamps in "
		    "account '%1'...").arg(
		        AccountModel::globAccounts[userName].accountName()));
		checkMsgsTmstmpExpiration(userName, QStringList());
		break;

	case TimestampExpirDialog::CHECK_TIMESTAMP_ALL:
		for (int i = 0; i < ui->accountList->model()->rowCount(); ++i) {
			index = m_accountModel.index(i, 0);
			userName = index.data(ROLE_ACNT_USER_NAME).toString();
			Q_ASSERT(!userName.isEmpty());
			showStatusTextPermanently(
			    tr("Checking time stamps in account '%1'...").arg(
			        AccountModel::globAccounts[userName].accountName()));
			checkMsgsTmstmpExpiration(userName, QStringList());
		}
		break;

	case TimestampExpirDialog::CHECK_TIMESTAMP_ZFO_SUB:
		includeSubdir = true;
	case TimestampExpirDialog::CHECK_TIMESTAMP_ZFO:
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

	statusBar->clearMessage();
}


void MainWindow::showStatusTextWithTimeout(const QString &qStr)
{
	statusBar->clearMessage();
	statusBar->showMessage((qStr), TIMER_STATUS_TIMEOUT_MS);
}

void MainWindow::showStatusTextPermanently(const QString &qStr)
{
	statusBar->clearMessage();
	statusBar->showMessage((qStr), 0);
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
	QString infoText;
	bool showExportOption = true;

	if (userName.isEmpty()) {

		msgCnt = filePathList.count();

		struct isds_message *message = NULL;
		struct isds_ctx *dummy_session = NULL;

		dummy_session = isds_ctx_create();
		if (NULL == dummy_session) {
			qDebug() << "Cannot create dummy ISDS session.";
			showStatusTextWithTimeout(tr("Loading of ZFO file(s) "
			    "failed!"));
			/* TODO */
			return;
		}

		for (int i = 0; i < msgCnt; ++i) {

			message = loadZfoFile(dummy_session, filePathList.at(i),
			    ImportZFODialog::IMPORT_MESSAGE_ZFO);
			if (NULL == message || message->envelope == NULL) {
				errorMsgFileNames.append(filePathList.at(i));
				continue;
			}

			if (NULL == message->envelope->timestamp) {
				errorMsgFileNames.append(filePathList.at(i));
				continue;
			}

			tstData = QByteArray(
			    (char *) message->envelope->timestamp,
			    (int) message->envelope->timestamp_length);

			if (tstData.isEmpty()) {
				errorMsgFileNames.append(filePathList.at(i));
				continue;
			}

			if (DlgSignatureDetail::signingCertExpiresBefore(
			        tstData,
			        globPref.timestamp_expir_before_days)) {
				expirMsgFileNames.append(filePathList.at(i));
			}
		}

		isds_message_free(&message);
		isds_ctx_free(&dummy_session);

		infoText = tr("Time stamp expiration check of ZFO files "
		    "finished with result:")
		    + "<br/><br/>" +
		    tr("Total of ZFO files: %1").arg(msgCnt)
		    + "<br/><b>" +
		    tr("ZFO files with time stamp expiring within %1 days: %2")
			.arg(globPref.timestamp_expir_before_days)
			.arg(expirMsgFileNames.count())
		    + "</b><br/>" +
		    tr("Unchecked ZFO files: %1").arg(errorMsgFileNames.count());

		showExportOption = false;

	} else {
		MessageDbSet *dbSet = accountDbSet(userName, this);
		Q_ASSERT(0 != dbSet);

		QList<MessageDb::MsgId> msgIdList = dbSet->getAllMessageIDsFromDB();
		msgCnt = msgIdList.count();

		foreach (const MessageDb::MsgId &mId, msgIdList) {
			MessageDb *messageDb = dbSet->accessMessageDb(mId.deliveryTime, false);
			Q_ASSERT(0 != messageDb);

			tstData = messageDb->msgsTimestampRaw(mId.dmId);
			if (tstData.isEmpty()) {
				errorMsgIds.append(mId);
				continue;
			}
			if (DlgSignatureDetail::signingCertExpiresBefore(tstData,
			    globPref.timestamp_expir_before_days)) {
				expirMsgIds.append(mId);
			}
		}

		infoText = tr("Time stamp expiration check "
		    "in account '%1' finished with result:").arg(
		        AccountModel::globAccounts[userName].accountName())
		    + "<br/><br/>" +
		    tr("Total of messages in database: %1").arg(msgCnt)
		    + "<br/><b>" +
		    tr("Messages with time stamp expiring within %1 days: %2")
			.arg(globPref.timestamp_expir_before_days)
			.arg(expirMsgIds.count())
		    + "</b><br/>" +
		    tr("Unchecked messages: %1").arg(errorMsgIds.count());
	}

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setWindowTitle(tr("Time stamp expiration check results"));
	msgBox.setText(infoText);

	if (!expirMsgIds.isEmpty() || !expirMsgFileNames.isEmpty() ||
	    !errorMsgIds.isEmpty() || !errorMsgFileNames.isEmpty()) {
		infoText = tr("See details for more info...") + "<br/><br/>";
		if (!expirMsgIds.isEmpty() && showExportOption) {
			infoText += "<b>" +
			    tr("Do you want to export the expiring "
			    "messages to ZFO?") + "</b><br/><br/>";
		}
		msgBox.setInformativeText(infoText);

		infoText.clear();
		if (!expirMsgIds.isEmpty() || !errorMsgIds.isEmpty()) {
			for (int i = 0; i < expirMsgIds.count(); ++i) {
				infoText += tr("Time stamp of message %1 expires "
				    "within specified interval.").arg(expirMsgIds.at(i).dmId);
				if (((expirMsgIds.count() - 1) != i) ||
				    errorMsgIds.count()) {
					infoText += "\n";
				}
			}
			for (int i = 0; i < errorMsgIds.count(); ++i) {
				infoText += tr("Time stamp of message %1 "
				    "is not present.").arg(errorMsgIds.at(i).dmId);
				if ((expirMsgIds.count() - 1) != i) {
					infoText += "\n";
				}
			}
		} else {
			for (int i = 0; i < expirMsgFileNames.count(); ++i) {
				infoText += tr("Time stamp of message %1 expires "
				    "within specified interval.").arg(expirMsgFileNames.at(i));
				if (((expirMsgFileNames.count() - 1) != i) ||
				    errorMsgFileNames.count()) {
					infoText += "\n";
				}
			}
			for (int i = 0; i < errorMsgFileNames.count(); ++i) {
				infoText += tr("Time stamp of message %1 "
				    "is not present.").arg(errorMsgFileNames.at(i));
				if ((expirMsgFileNames.count() - 1) != i) {
					infoText += "\n";
				}
			}
		}
		msgBox.setDetailedText(infoText);

		if (!expirMsgIds.isEmpty() && showExportOption) {
			msgBox.setStandardButtons(QMessageBox::Yes
			    | QMessageBox::No);
			msgBox.setDefaultButton(QMessageBox::No);
		} else {
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
		}
	} else {
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
	}

	if (QMessageBox::Yes == msgBox.exec()) {
		if (!userName.isEmpty()) {
			exportExpirMessagesToZFO(userName, expirMsgIds);
		}
	}
}


/* ========================================================================= */
/*
 * Export messages with expired time stamp to ZFO.
 */
void MainWindow::exportExpirMessagesToZFO(const QString &userName,
    const QList<MessageDb::MsgId> &expirMsgIds)
/* ========================================================================= */
{
	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Export ZFO"), m_on_export_zfo_activate,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (newDir.isEmpty()) {
		return;
	}

	foreach (const MessageDb::MsgId &mId, expirMsgIds) {
		exportSelectedMessageAsZFO(newDir, userName, mId.dmId,
		    mId.deliveryTime);
	}
}


/* ========================================================================= */
/*
 * Split database filename into mandatory entries.
 */
bool MainWindow::isValidDatabaseFileName(QString inDbFileName,
    QString &dbUserName, QString &dbYear, bool &dbTestingFlag, QString &errMsg)
/* ========================================================================= */
{
	QStringList fileNameParts;
	bool ret = false;
	errMsg = "";
	dbUserName = "";
	dbYear = "";

	if (inDbFileName.contains("___")) {
		// get username from filename
		fileNameParts = inDbFileName.split("_");
		if (fileNameParts.isEmpty() || fileNameParts.count() <= 1) {
			errMsg = tr("This file does not contain a valid "
			    "database filename.");
			return ret;
		}
		if (fileNameParts[0].isEmpty() ||
		    fileNameParts[0].length() != 6) {
			errMsg = tr("This file does not contain a valid "
			    "username in the database filename.");
			return ret;
		}
		dbUserName = fileNameParts[0];

		// get year from filename
		if (fileNameParts[1].isEmpty()) {
			dbYear = "";
		} else if (fileNameParts[1].length() == 4) {
			dbYear = fileNameParts[1];
		} else {
			errMsg = tr("This database file does not contain "
			    "valid year in the database filename.");
			dbYear = "";
			return ret;
		}

		// get testing flag from filename
		fileNameParts = inDbFileName.split(".");
		if (fileNameParts.isEmpty()) {
			errMsg = tr("This file does not contain valid "
			    "database filename.");
			return ret;
		}
		fileNameParts = fileNameParts[0].split("___");
		if (fileNameParts.isEmpty()) {
			errMsg = tr("This database file does not contain "
			    "valid database filename.");
			return ret;
		}

		if (fileNameParts[1] == "1") {
			dbTestingFlag = true;
		} else if (fileNameParts[1] == "0") {
			dbTestingFlag = false;
		} else {
			errMsg = tr("This file does not contain a valid "
			    "account type flag or filename has wrong format.");
			dbTestingFlag = false;
			return ret;
		}
	} else {
		errMsg = tr("This file does not contain a valid message "
		    "database or filename has wrong format.");
		return ret;
	}

	return true;
}


/* ========================================================================= */
/*
 * Prepare import of messages from database.
 */
void MainWindow::prepareMsgsImportFromDatabase(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* get list of selected database files */
	QStringList files = QFileDialog::getOpenFileNames(this,
		    tr("Select database file(s)"),
		    m_on_import_database_dir_activate, tr("DB file (*.db)"));

	if (files.isEmpty()) {
		qDebug() << "No *.db selected file(s)";
		showStatusTextWithTimeout(tr("Database file(s) not selected."));
		return;
	}

	/* remember import path */
	m_on_import_database_dir_activate =
		    QFileInfo(files.at(0)).absoluteDir().absolutePath();

	doMsgsImportFromDatabase(files, userNameFromItem());
}


/* ========================================================================= */
/*
 *  Import of messages from database to selected account
 */
void MainWindow::doMsgsImportFromDatabase(const QStringList &dbFileList,
    const QString &aUserName)
/* ========================================================================= */
{
	debugFuncCall();

	MessageDbSet *srcDbSet = NULL;
	QString dbDir;
	QString dbFileName;
	QString dbUserName;
	QString dbYearFlag;
	bool dbTestingFlag;
	int sMsgCnt = 0;
	QString msg;
	QStringList errImportList;

	for (int i = 0; i < dbFileList.count(); ++i) {

		QFileInfo file(dbFileList.at(i));
		dbDir = file.path();
		dbFileName = file.fileName();

		qDebug() << dbFileList.at(i) << "file is analysed...";

		/* split and check the database file name */
		if (!isValidDatabaseFileName(dbFileName, dbUserName,
		    dbYearFlag, dbTestingFlag, msg)) {
			qDebug() << msg;
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(aUserName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			continue;
		}

		qDebug() << dbUserName << dbYearFlag << dbTestingFlag;

		/* check if file username is relevant to account */
		if (aUserName != dbUserName) {
			msg = tr("This database cannot import into selected"
			    " account because usernames do not correspond.");
			qDebug() << msg;
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(aUserName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			continue;
		}

		MessageDbSet::Organisation dbType;

		if (dbYearFlag.isEmpty()) {
			dbType = MessageDbSet::DO_SINGLE_FILE;
		} else if (QDate::fromString(dbYearFlag, "YYYY").isValid()) {
			dbType = MessageDbSet::DO_YEARLY;
		} else {
			dbType = MessageDbSet::DO_UNKNOWN;
		}

		DbContainer temporaryDbCont("TEMPORARYDBS");
		/* open source database file */
		srcDbSet = temporaryDbCont.accessDbSet(dbDir, dbUserName,
		    dbTestingFlag, dbType, false);
		if (NULL == srcDbSet) {
			msg = tr("Failed to open database file.");
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(aUserName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			qDebug() << dbFileList.at(i) << msg;
			continue;
		}

		qDebug() << dbFileList.at(i) << "is open";

		QString dboxId = globAccountDbPtr->dbId(aUserName + "___True");

		/* get all msgs from source database */
		QList<MessageDb::MsgId> msgIdList =
		    srcDbSet->getAllMessageIDsFromDB();

		MessageDbSet *dstDbSet = accountDbSet(aUserName, this);

		errImportList.clear();
		sMsgCnt = 0;

		// over all msgs
		foreach (const MessageDb::MsgId &mId, msgIdList) {

			qDebug() << mId.dmId << mId.deliveryTime;

			/* select source database via delivery time */
			MessageDb *srcDb =
			    srcDbSet->accessMessageDb(mId.deliveryTime, false);

			/* select destination database via delivery time */
			MessageDb *dstDb =
			    dstDbSet->accessMessageDb(mId.deliveryTime, true);
			if (0 == dstDb) {
				/* TODO */
				continue;
			}
			Q_ASSERT(0 != dstDb);

			/* check if msg exists in destination database */
			if (-1 != dstDb->msgsStatusIfExists(mId.dmId)) {
				msg = tr("Message '%1' already exists in "
				    "database for this account.").arg(mId.dmId);
				errImportList.append(msg);
				continue;
			}

			/* check if msg is relevant for account databox ID  */
			if (!srcDb->isRelevantMsgForImport(mId.dmId, dboxId)) {
				msg = tr("Message '%1' cannot be imported "
				    "into this account. Message does not "
				    "contain any valid ID of databox "
				    "corresponding with this account.").
				    arg(mId.dmId);
				errImportList.append(msg);
				continue;
			}

			/* copy all msg data to account database */
			if (!dstDb->copyCompleteMsgDataToAccountDb(
			    dbFileList.at(i), mId.dmId)) {
				msg = tr("Message '%1' cannot be inserted "
				    "into database of this account. An error "
				    "occurred during insertion procedure.").
				    arg(mId.dmId);
				errImportList.append(msg);
				continue;
			}
			sMsgCnt++;
		}

		QMessageBox msgBox(this);
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Messages import result"));
		msg = tr("Import of messages into account '%1' "
		    "finished with result:").arg(aUserName) +
		    "<br/><br/>" + tr("Source database file: '%1'").
		    arg(dbFileList.at(i));
		msgBox.setText(msg);
		msg = tr("Total of messages in database: %1").
		    arg(msgIdList.count())
		    + "<br/><b>" +
		    tr("Imported messages: %1").arg(sMsgCnt)
		    + "<br/>" +
		    tr("Non-imported messages: %1").arg(errImportList.count()) +
		    "</b><br/>";
		msgBox.setInformativeText(msg);
		if (errImportList.count() > 0) {
			msg = "";
			for (int m = 0; m < errImportList.count(); ++ m) {
				msg += errImportList.at(m) + "\n";
			}
			msgBox.setDetailedText(msg);
		}
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	}
}


/* ========================================================================= */
/*
 * Split message database slot.
 */
void MainWindow::splitMsgDbByYearsSlot(void)
/* ========================================================================= */
{
	debugSlotCall();
	splitMsgDbByYears(userNameFromItem());
}


/* ========================================================================= */
/*
 * Split message database into new databases contain messages for single years.
 */
void MainWindow::splitMsgDbByYears(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	/* Get user name and db location. */
	AccountModel::SettingsMap &itemSettings =
	    AccountModel::globAccounts[userName];

	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}

	/* TODO - test account detection is not implemented now */
	QString testacnt = "0";
	//if (itemSettings.isTestAccount()) {
	//	testacnt = "1";
	//}

	MessageDb *messageDb = accountMessageDb(userName, this);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return;
	}

	int cYear = QDate::currentDate().year();
	qDebug() << "currentYear:" << cYear;
	QString currentYear = QString::number(cYear);
	QStringList yearList = messageDb->getAllUniqueYearsFormMsgs();
	qDebug() << "yearList:" << yearList;

	/* TODO - messages for current year are ingnored now */
	for (int i = 0; i < yearList.count(); ++i) {
		if (currentYear == yearList.at(i) ) {
			continue;
		}

		/* TODO - test account detection is not implemented now */
		QString newDbName = userName + "_" + yearList.at(i);
		MessageDb *newMsgDb = accountMessageDb(newDbName, this);
		messageDb->copyRelevantMsgsToNewDb(dbDir + "/" + newDbName
		    + "___" + testacnt + ".db", yearList.at(i));
	}
}
