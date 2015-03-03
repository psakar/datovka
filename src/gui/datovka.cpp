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
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_account_from_db.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_change_directory.h"
#include "src/gui/dlg_correspondence_overview.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/gui/dlg_import_zfo_result.h"
#include "src/gui/dlg_yes_no_checkbox.h"
#include "src/log/log.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "ui_datovka.h"


#define WIN_POSITION_HEADER "window_position"
#define WIN_POSITION_X "x"
#define WIN_POSITION_Y "y"
#define WIN_POSITION_W "w"
#define WIN_POSITION_H "h"

QNetworkAccessManager* nam;

#define showStatusTextWithTimeout(qStr) \
	do { \
		statusBar->clearMessage(); \
		statusBar->showMessage((qStr), TIMER_STATUS_TIMEOUT_MS); \
	} while(0)

#define showStatusTextPermanently(qStr) \
	do { \
		statusBar->clearMessage(); \
		statusBar->showMessage((qStr), 0); \
	} while(0)

/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
    m_statusProgressBar(NULL),
    m_syncAcntThread(0),
    m_syncAcntWorker(0),
    m_accountModel(this),
    m_accountDb("accountDb"),
    m_messageDbs(),
    m_filterLine(NULL),
    m_messageListProxyModel(this),
    m_messageMarker(this),
    m_lastSelectedMessageId(-1),
    m_lastStoredMessageId(-1),
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
	    "Datovka - Free interface for Datov\303\251 schr\303\241nky"));
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

	qDebug() << "Load" << globPref.loadConfPath();
	qDebug() << "Save" << globPref.saveConfPath();

	/* Change "\" to "/" */
	fixBackSlashesInFile(globPref.loadConfPath());

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(
	    QCoreApplication::applicationVersion()));
	ui->accountTextInfo->setReadOnly(true);

	/* Configuration directory and file must exist. */
	ensureConfPresence();

	/* Open accounts database. */
	if (!m_accountDb.openDb(globPref.accountDbPath())) {
		qWarning() << "Error opening account db"
		    << globPref.accountDbPath();
	}

	/* Load configuration file. */
	loadSettings();
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
	    this, SLOT(msgSetSelectedMessageRead()));

	/* Initialisation of message download timer. */
	connect(&m_timerSyncAccounts, SIGNAL(timeout()), this,
	    SLOT(synchroniseAllAccounts()));
	if (globPref.download_on_background) {
		if (globPref.timer_value > 4) {
			m_timeoutSyncAccounts = globPref.timer_value * 60000;
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
			qDebug() << "Timer set on" << globPref.timer_value <<
			    "minutes";
		} else {
			m_timeoutSyncAccounts = TIMER_DEFAULT_TIMEOUT_MS;
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
			qDebug() << "Timer set on" <<
			    TIMER_DEFAULT_TIMEOUT_MS/60000 << "minutes";
		}
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
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
			qDebug() << "Timer set on" << globPref.timer_value <<
			    "minutes";
		} else {
			m_timeoutSyncAccounts = TIMER_DEFAULT_TIMEOUT_MS;
			m_timerSyncAccounts.start(m_timeoutSyncAccounts);
			qDebug() << "Timer set on" <<
			    TIMER_DEFAULT_TIMEOUT_MS/60000 << "minutes";
		}
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

	/* Disable the menu, re-enable only on received messages. */
	ui->messageStateCombo->setEnabled(false);

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
		    SLOT(messageItemRestoreSelection()));

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
	QString userName = accountUserName(accountItem);
	MessageDb *messageDb = accountMessageDb(accountItem);
	if (0 == messageDb) {
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
		    SLOT(messageItemRestoreSelection()));

		/* Get user name and db location. */
		const QStandardItem *accountItemTop =
		    AccountModel::itemTop(accountItem);
		const AccountModel::SettingsMap itemSettings =
		    accountItemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		const QString userName = itemSettings.userName();
		Q_ASSERT(!userName.isEmpty());

		QString dbDir = itemSettings.dbDir();
		if (dbDir.isEmpty()) {
			/* Set default directory name. */
			dbDir = globPref.confDir();
		}

		QString dbFilePath =
		    DbContainer::constructDbFileName(userName,
		        dbDir, itemSettings.isTestAccount());

		/* Decouple model and show banner page. */
		ui->messageList->setModel(0);
		ui->messageStackedWidget->setCurrentIndex(0);
		QString htmlMessage = "<div style=\"margin-left: 12px;\">"
		    "<h3>" + tr("Database access error") + "</h3>" "<br/>";
		htmlMessage += "<div>";
		htmlMessage += tr("Database file '%1' cannot be accessed."
		    ).arg(dbFilePath);
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

	setAccountStoragePaths(accountItem);

	//qDebug() << "Selected user account" << userName << dbDir;
	//qDebug() << current.model() << accountItem->text();
	//qDebug() << "\n";

	/*
	 * TODO -- Is '___True' somehow related to the testing state
	 * of an account?
	 */
	QString dbId = m_accountDb.dbId(userName + "___True");
	//qDebug() << "Selected data box ID" << dbId;
	if (dbId.isEmpty()) {
		/* Get user information. */
		qWarning() << "Missing user entry of" << userName
		    << "in account db.";

		if (!isMainWindow) {
			/* eliminate several problems as:
			 * password dialog is shown before gui is loaded
			 * database was not still open
			*/
			return;
		}

		if (!getOwnerInfoFromLogin(AccountModel::indexTop(current))) {
		/* TODO -- What to do when no ISDS connection is present? */
			return;
		}
		dbId = m_accountDb.dbId(userName + "___True");
	}
	Q_ASSERT(!dbId.isEmpty());

	//qDebug() << "Clicked row" << current.row();
	//qDebug() << "Clicked type" << AccountModel::nodeType(current);

	/*
	 * Disconnect message clicked. This slot will be enabled only for
	 * received messages.
	 */
	ui->messageList->disconnect(SIGNAL(clicked(QModelIndex)),
	    this, SLOT(messageItemClicked(QModelIndex)));

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
		setMessageActionVisibility(false);
		html = createAccountInfo(*accountItem);
		ui->actionDelete_message_from_db->setEnabled(false);
		ui->actionDelete_message_from_server->setEnabled(false);
		break;
	case AccountModel::nodeRecentReceived:
		msgTblMdl = messageDb->msgsRcvdWithin90DaysModel(dbId);
		//ui->messageList->horizontalHeader()->moveSection(5,3);
		ui->actionDelete_message_from_db->setEnabled(false);
		ui->actionDelete_message_from_server->setEnabled(false);
		ui->actionReply_to_the_sender->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeRecentSent:
		msgTblMdl = messageDb->msgsSntWithin90DaysModel(dbId);
		ui->actionDelete_message_from_db->setEnabled(false);
		ui->actionDelete_message_from_server->setEnabled(false);
		ui->actionReply_to_the_sender->setEnabled(false);
		break;
	case AccountModel::nodeAll:
		setMessageActionVisibility(false);
		html = createAccountInfoAllField(tr("All messages"),
		    messageDb->msgsRcvdYearlyCounts(dbId, DESCENDING),
		    messageDb->msgsSntYearlyCounts(dbId, DESCENDING));
		ui->actionDelete_message_from_db->setEnabled(false);
		ui->actionDelete_message_from_server->setEnabled(false);
		break;
	case AccountModel::nodeReceived:
		msgTblMdl = messageDb->msgsRcvdModel(dbId);
		ui->actionDelete_message_from_db->setEnabled(true);
		ui->actionDelete_message_from_server->setEnabled(true);
		ui->actionReply_to_the_sender->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeSent:
		msgTblMdl = messageDb->msgsSntModel(dbId);
		ui->actionDelete_message_from_db->setEnabled(true);
		ui->actionDelete_message_from_server->setEnabled(true);
		ui->actionReply_to_the_sender->setEnabled(false);
		break;
	case AccountModel::nodeReceivedYear:
		/* TODO -- Parameter check. */
		msgTblMdl = messageDb->msgsRcvdInYearModel(dbId,
		    accountItem->text());
		ui->actionDelete_message_from_db->setEnabled(true);
		ui->actionDelete_message_from_server->setEnabled(true);
		ui->actionReply_to_the_sender->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeSentYear:
		/* TODO -- Parameter check. */
		msgTblMdl = messageDb->msgsSntInYearModel(dbId,
		    accountItem->text());
		ui->actionDelete_message_from_db->setEnabled(true);
		ui->actionDelete_message_from_server->setEnabled(true);
		ui->actionReply_to_the_sender->setEnabled(false);
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
		    SLOT(messageItemRestoreSelection()));
	}

	/* Depending on which item was clicked show/hide elements. */
	QAbstractItemModel *itemModel;
	bool received = true;

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
	case AccountModel::nodeAll:
		ui->messageStackedWidget->setCurrentIndex(0);
		ui->accountTextInfo->setHtml(html);
		ui->accountTextInfo->setReadOnly(true);
		break;
	case AccountModel::nodeRecentReceived:
	case AccountModel::nodeReceived:
	case AccountModel::nodeReceivedYear:
		/* Set model. */
		Q_ASSERT(0 != msgTblMdl);
		m_messageListProxyModel.setSortRole(ROLE_MSGS_DB_PROXYSORT);
		m_messageListProxyModel.setSourceModel(msgTblMdl);
		ui->messageList->setModel(&m_messageListProxyModel);
		ui->messageStateCombo->setEnabled(true);
		/* Set specific column width. */
		setReceivedColumnWidths();
		received = true;
		goto setmodel;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
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
		    SLOT(messageItemRestoreSelection()));
		/* Clear message info. */
		ui->messageInfo->clear();
		/* Clear attachment list. */
		messageItemsSelectionChanged(QItemSelection());
		/* Select last message in list if there are some messages. */
		itemModel = ui->messageList->model();
		/* enable/disable buttons */
		if ((0 != itemModel) && (0 < itemModel->rowCount())) {
			messageItemRestoreSelection();
			//ui->actionReply_to_the_sender->setEnabled(true);
			ui->actionVerify_a_message->setEnabled(true);
			ui->menuMessage->setEnabled(true);
			ui->actionAuthenticate_message_file->setEnabled(true);
			ui->actionExport_correspondence_overview->
			    setEnabled(true);
		} else {
			ui->actionReply_to_the_sender->setEnabled(false);
			ui->actionVerify_a_message->setEnabled(false);
			ui->menuMessage->setEnabled(false);
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

	QModelIndex index = ui->accountList->indexAt(point);
	QMenu *menu = new QMenu;
#ifdef PORTABLE_APPLICATION
	QAction *action;
#endif /* PORTABLE_APPLICATION */

	if (index.isValid()) {
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-account-sync.png"),
		    tr("Get messages"),
		    this, SLOT(synchroniseSelectedAccount()));
		menu->addAction(QIcon(ICON_16x16_PATH "datovka-message.png"),
		    tr("Create message"),
		    this, SLOT(createAndSendMessage()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "tick_16.png"),
		    tr("Mark all as read"),
		    this, SLOT(accountItemMarkAllRead()));
		menu->addSeparator();
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
	} else {
		menu->addAction(QIcon(ICON_3PARTY_PATH "plus_16.png"),
		    tr("Add new account"),
		    this, SLOT(addNewAccount()));
	}

	menu->exec(QCursor::pos());
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
	ui->downloadComplete->setEnabled(false);
	ui->saveAttachments->setEnabled(false);
	ui->saveAttachment->setEnabled(false);
	ui->openAttachment->setEnabled(false);
	ui->verifySignature->setEnabled(false);
	ui->signatureDetails->setEnabled(false);
	ui->actionSave_attachment->setEnabled(false);
	ui->actionOpen_attachment->setEnabled(false);
//	ui->messageStateCombo->setEnabled(false);

	/* Disable model for attachment list. */
	ui->messageAttachmentList->setModel(0);

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
	ui->downloadComplete->setEnabled(true);
//	ui->messageStateCombo->setEnabled(true);

	if (1 == firstColumnIdxs.size()) {
		const QModelIndex &index = firstColumnIdxs.first();

		MessageDb *messageDb = accountMessageDb(0);
		Q_ASSERT(0 != messageDb);
		int msgId = index.data().toInt();
		/* Remember last selected message. */
		messageItemStoreSelection(msgId);

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

		int msgState = messageDb->msgGetProcessState(msgId);

		/* msgState is -1 if message is not in database */
		if (msgState >= 0) {
			ui->messageStateCombo->setCurrentIndex(msgState);
		} else {
			/* insert message state into database */
			messageDb->msgSetProcessState(msgId, UNSETTLED, true);
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
		} else {
			ui->saveAttachments->setEnabled(false);
			ui->actionSave_all_attachments->setEnabled(false);
		}

		ui->messageAttachmentList->resizeColumnToContents(3);

		/* Connect new slot. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemCurrentChanged(QModelIndex,
		        QModelIndex)));
	} else {
		ui->messageInfo->setHtml("");
		ui->messageInfo->setReadOnly(true);
		ui->saveAttachments->setEnabled(false);
		/* TODO */
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

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	int msgId = index.sibling(index.row(), 0).data().toInt();

	/* Get message state from database and toggle the value. */
	bool isRead = messageDb->smsgdtLocallyRead(msgId);
	messageDb->smsgdtSetLocallyRead(msgId, !isRead);

	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(ui->accountList->
	    selectionModel()->currentIndex());

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

	if (index.isValid()) {

		QMenu *menu = new QMenu;

		/* Remember last selected message. */
		messageItemStoreSelection(
		    index.model()->itemData(index).first().toInt());

		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-download.png"),
		    tr("Download message signed"), this,
		    SLOT(downloadSelectedMessageAttachments()));
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-reply.png"),
		    tr("Reply to message"), this,
		    SLOT(createAndSendMessageReply()));
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH "label_16.png"),
		    tr("Signature details"), this,
		    SLOT(showSignatureDetails()));
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-verify.png"),
		    tr("Authenticate message"), this,
		    SLOT(verifyMessage()));
		menu->addSeparator();
		menu->addAction(
		    tr("Open message externally"), this,
		    SLOT(openSelectedMessageExternally()))->
		    setEnabled(ui->actionOpen_message_externally->isEnabled());
		menu->addAction(
		    tr("Open delivery info externally"), this,
		    SLOT(openDeliveryInfoExternally()))->
		    setEnabled(ui->actionOpen_delivery_info_externally->
		    isEnabled());
		menu->addSeparator();
		menu->addAction(
		    tr("Export message as ZFO"), this,
		    SLOT(exportSelectedMessageAsZFO()))->
		    setEnabled(ui->actionExport_as_ZFO->isEnabled());
		menu->addAction(
		    tr("Export delivery info as ZFO"), this,
		    SLOT(exportDeliveryInfoAsZFO()))->
		    setEnabled(ui->actionExport_delivery_info_as_ZFO->
		    isEnabled());
		menu->addAction(
		    tr("Export delivery info as PDF"), this,
		    SLOT(exportDeliveryInfoAsPDF()))->
		    setEnabled(ui->actionExport_delivery_info_as_PDF->
		    isEnabled());
		menu->addAction(
		    tr("Export message envelope as PDF"), this,
		    SLOT(exportMessageEnvelopeAsPDF()))->
		    setEnabled(ui->actionExport_message_envelope_as_PDF->
		    isEnabled());
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH "delete_16.png"),
		    tr("Delete message"), this,
		    SLOT(deleteMessage()))->
		    setEnabled(ui->actionDelete_message_from_db->isEnabled());
		menu->exec(QCursor::pos());
	}
}


/* ========================================================================= */
/*
 * Saves message selection.
 */
void MainWindow::messageItemStoreSelection(long msgId)
/* ========================================================================= */
{
	debugSlotCall();

	m_lastSelectedMessageId = msgId;
	qDebug() << "Last selected" << m_lastSelectedMessageId;
	if (-1 == msgId) {
		return;
	}

	/*
	 * If we selected a message from last received then store the
	 * selection to the model.
	 */
	QModelIndex acntIdx = ui->accountList->currentIndex();
	if (AccountModel::nodeRecentReceived ==
	    AccountModel::nodeType(acntIdx)) {

		qDebug() << "Storing recent received selection into the model"
		    << msgId;

		acntIdx = AccountModel::indexTop(acntIdx);
		AccountModel::SettingsMap accountInfo =
		    acntIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		accountInfo.setLastMsg(QString::number(msgId, 10));
		ui->accountList->model()->setData(acntIdx, accountInfo,
		    ROLE_ACNT_CONF_SETTINGS);
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
	AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	accountInfo.setLastAttachSavePath(m_save_attach_dir);
	accountInfo.setLastAttachAddPath(m_add_attach_dir);
	accountInfo.setLastCorrespPath(m_export_correspond_dir);
	accountInfo.setLastZFOExportPath(m_on_export_zfo_activate);
	ui->accountList->model()->setData(acntIdx, accountInfo,
	    ROLE_ACNT_CONF_SETTINGS);
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
	qDebug() << "Last stored position" << m_lastStoredMessageId;
}


/* ========================================================================= */
/*
 * Restores message selection.
 */
void MainWindow::messageItemRestoreSelection(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex index;

	const QAbstractItemModel *model = ui->messageList->model();
	Q_ASSERT(0 != model);

	int rowCount = model->rowCount();
	int row = 0;

	if (0 == rowCount) {
		/* Do nothing on empty model. */
		return;
	}

	/* If the ID does not exist then don't search for it. */
	if (-1 == m_lastStoredMessageId) {
		row = rowCount;
	}

	/* Find and select the message with the ID. */
	for (; row < rowCount; ++row) {
		/*
		 * TODO -- Search in a more resource-saving way.
		 * Eliminate index copying, use smarter search.
		 */
		index = model->index(row, 0);
		if (index.data().toInt() == m_lastStoredMessageId) {
			break;
		}
	}

	if (row < rowCount) {
		/* Message found. */
		ui->messageList->setCurrentIndex(index);
		if (index.isValid()) {
			ui->messageList->scrollTo(index);
		}
	} else {
		/*
		 * If we selected a message from last received then restore the
		 * selection according to the model.
		 */
		QModelIndex acntIdx = ui->accountList->currentIndex();
		if (AccountModel::nodeRecentReceived ==
		    AccountModel::nodeType(acntIdx)) {
			if (GlobPreferences::SELECT_NEWEST ==
			    globPref.after_start_select) {
				/*
				 * Search for the message with the largest id.
				 */				
				index = model->index(0, 0);
				QModelIndex newIndex = index;
				int largestSoFar = index.data().toInt();
				for (row = 1; row < rowCount; ++row) {
					/*
					 * TODO -- Search in a more
					 * resource-saving way.
					 * Eliminate index copying, use
					 * smarter search.
					 */
					index = model->index(row, 0);
					if (largestSoFar < model->
						index(row, 0).data().toInt()) {
						index = model->index(row, 0);
						largestSoFar =
						    index.data().toInt();
						newIndex = index;
					}
				}
				if (newIndex.isValid()) {
					ui->messageList->setCurrentIndex(newIndex);
					ui->messageList->scrollTo(newIndex);
				}

			} else if (GlobPreferences::SELECT_LAST_VISITED ==
			    globPref.after_start_select) {
				acntIdx = AccountModel::indexTop(acntIdx);
				const AccountModel::SettingsMap accountInfo =
				    acntIdx.data(ROLE_ACNT_CONF_SETTINGS).
					toMap();
				QString msgLastId = accountInfo.lastMsg();
				if (!msgLastId.isEmpty()) {
					int msgId = msgLastId.toInt();

					/*
					 * Find and select the message with the
					 * ID.
					 */
					for (row = 0; row < rowCount; ++row) {
						/*
						 * TODO -- Search in a more
						 * resource-saving way.
						 * Eliminate index copying, use
						 * smarter search.
						 */
						index = model->index(row, 0);
						if (index.data().toInt() ==
						    msgId) {
							break;
						}
					}
					if (row < rowCount) {
						/* Set selection if found. */
						ui->messageList->
						    setCurrentIndex(index);
						if (index.isValid()) {
							ui->messageList->scrollTo(index);
						}
					}
				}
			} else if (GlobPreferences::SELECT_NOTHING ==
				    globPref.after_start_select) {
					/* Select last row. */
					index = model->index(rowCount - 1, 0);
					ui->messageList->
					    setCurrentIndex(index);
					if (index.isValid()) {
						ui->messageList->scrollTo(index);
					}
			} else {
				Q_ASSERT(0);
			}
		} else {
			/* Select last row. */
			index = model->index(rowCount - 1, 0);
			ui->messageList->setCurrentIndex(index);
			if (index.isValid()) {
				ui->messageList->scrollTo(index);
			}
		}
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
	QString dmId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toString();

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

	QString dmId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toString();

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
		Q_ASSERT(!fileName.isEmpty());
		if (fileName.isEmpty()) {
			unspecifiedFailed = true;
			continue;
		}

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
void MainWindow::clearInfoInStatusBarAndShowDialog(QString msgID)
/* ========================================================================= */
{
	debugSlotCall();

	showStatusTextWithTimeout(tr("It was not possible download complete "
	    "message \"%1\" from ISDS server.").arg(msgID));

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setWindowTitle(tr("Download message error"));
	msgBox.setText(tr("It was not possible to download a complete "
	    "message \"%1\" from server Datové schránky.").arg(msgID));
	msgBox.setInformativeText(tr("A connection error occured or "
	    "the message has already been deleted from the server."));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}


/* ========================================================================= */
/*
 * Set tablewidget when message download worker is done.
 */
void MainWindow::postDownloadSelectedMessageAttachments(
    const QModelIndex &acntTopIdx, const QString &dmId)
/* ========================================================================= */
{
	debugSlotCall();

	showStatusTextWithTimeout(tr("Message \"%1\" "
	    " was downloaded from ISDS server.").arg(dmId));

	QModelIndex accountTopIndex =
	    AccountModel::indexTop(ui->accountList->currentIndex());
	Q_ASSERT(accountTopIndex.isValid());

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
		if (index.data().toString() == dmId) {
			msgIdIdx = index;
			break;
		}
	}

	if (!msgIdIdx.isValid()) {
		return;
	}

	qint64 msgId = dmId.toLongLong();

	/*
	 * Mark message as having attachment downloaded without reloading
	 * the whole model.
	 */
	messageModel->overrideDownloaded(msgId, true);
	QItemSelection storedSelection =
	    ui->messageList->selectionModel()->selection();
	/* Inform the view that the model has changed. */
	emit messageModel->dataChanged(
	    msgIdIdx.sibling(msgIdIdx.row(), 0),
	    msgIdIdx.sibling(msgIdIdx.row(), messageModel->columnCount() - 1));
	ui->messageList->selectionModel()->select(storedSelection,
	    QItemSelectionModel::Select);

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

	QStandardItem *accountTopItem =
	    m_accountModel.itemFromIndex(accountTopIndex);
	MessageDb *messageDb = accountMessageDb(accountTopItem);
	Q_ASSERT(0 != messageDb);

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId,
	    ui->verifySignature));
	ui->messageInfo->setReadOnly(true);

	QAbstractTableModel *fileTblMdl = messageDb->flsModel(msgId);
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
 * Mark all messages as read in selected account item.
 */
void MainWindow::accountItemMarkAllRead(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* Save current index. */
	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	const QAbstractItemModel *msgTblMdl = ui->messageList->model();
	Q_ASSERT(0 != msgTblMdl);

	int count = msgTblMdl->rowCount();

	for (int i = 0; i < count; ++i) {
		QModelIndex index = msgTblMdl->index(i, 0);
		int msgId = msgTblMdl->itemData(index).first().toInt();
		messageDb->smsgdtSetLocallyRead(msgId);
	}

	/* Regenerate account tree. */
	regenerateAllAccountModelYears();

	showStatusTextWithTimeout(tr("All messages were "
	    "marked as locally read."));

	/* Restore selection. */
	if (selectedAcntIndex.isValid()) {
		ui->accountList->selectionModel()->setCurrentIndex(
		    selectedAcntIndex, QItemSelectionModel::ClearAndSelect);
		accountItemCurrentChanged(selectedAcntIndex);
		/*
		 * TODO -- When using on year account item then the first
		 * switching on a parent item (Received) does not redraw the
		 * message list.
		 */
	}
}


/* ========================================================================= */
/*
 * Delete selected message from local database and ISDS.
 */
void MainWindow::deleteMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	if (!acntTopIdx.isValid()) {
		return;
	}

	QModelIndexList msgIdxList =
	    ui->messageList->selectionModel()->selectedRows();

	if (msgIdxList.isEmpty()) {
		return;
	}

	QString dlgTitleText, questionText, checkBoxText, detailText, dmId;

	int msgIdxCnt = msgIdxList.size();
	if (msgIdxCnt == 1) {
		dmId = msgIdxList.at(0).sibling(
		    msgIdxList.at(0).row(), 0).data().toString();
		dlgTitleText = tr("Delete message %1").arg(dmId);
		questionText = tr("Do you want to delete "
		    "message '%1'?").arg(dmId);
		checkBoxText = tr("Delete this message also from server ISDS");
		detailText = tr("Warning: If you delete the message "
		    "from ISDS then this message will be lost forever.");
	} else {
		dlgTitleText = tr("Delete messages");
		questionText = tr("Do you want to delete selected messages?");
		checkBoxText =tr("Delete these messages also from server ISDS");
		detailText = tr("Warning: If you delete selected messages from "
		"ISDS then these message will be lost forever.");
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

	for (int i = 0; i < msgIdxCnt; i++) {
		dmId = msgIdxList.at(i).sibling(
		    msgIdxList.at(i).row(), 0).data().toString();

		/* Save current account index */
		QModelIndex selectedAcntIndex =
		    ui->accountList->currentIndex();

		switch (eraseMessage(acntTopIdx, dmId, delMsgIsds)) {
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
qdatovka_error MainWindow::eraseMessage(const QModelIndex &acntTopIdx,
    const QString &dmId, bool delFromIsds)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	if (!delFromIsds) {
		if (messageDb->msgsDeleteMessageData(dmID)) {
			qDebug() << "Message" << dmID <<
			    "was deleted from local database";
			showStatusTextWithTimeout(tr("Message \"%1\" was "
			    "deleted from local database.").arg(dmID));
			return Q_SUCCESS;
		}
	} else {

		isds_error status;
		if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
			if (!connectToIsds(acntTopIdx, true)) {
				return Q_CONNECT_ERROR;
			}
		}

		bool incoming = true;
		QModelIndex index = ui->accountList->
		    selectionModel()->currentIndex();

		switch (AccountModel::nodeType(index)) {
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
		status = isds_delete_message_from_storage(isdsSessions.session(
		    accountInfo.userName()), dmId.toStdString().c_str(),
		    incoming);

		if (IE_SUCCESS == status) {
			if (messageDb->msgsDeleteMessageData(dmID)) {
				qDebug() << "Message" << dmID <<
				    "was deleted from ISDS and local databse";
				showStatusTextWithTimeout(tr("Message \"%1\" "
				    "was deleted from ISDS and local database.")
				    .arg(dmID));
				return Q_SUCCESS;
			} else {
				qDebug() << "Message" << dmID <<
				    "was deleted only from ISDS.";
				showStatusTextWithTimeout(tr("Message \"%1\" "
				    "was deleted only from ISDS.").arg(dmID));
				return Q_SQL_ERROR;
			}
		} else if (IE_INVAL == status) {
			qDebug() << "Error: "<< status << isds_strerror(status);
			if (messageDb->msgsDeleteMessageData(dmID)) {
				qDebug() << "Message" << dmID <<
				    "was deleted only from local database.";
				showStatusTextWithTimeout(tr("Message \"%1\" "
				    "was deleted only from local database.")
				    .arg(dmID));
				return Q_ISDS_ERROR;
			}
		}
	}

	qDebug() << "Message" << dmID << "was not deleted.";
	showStatusTextWithTimeout(tr("Message \"%1\" was not deleted.")
	    .arg(dmID));
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

		/* Try connecting to ISDS, just to generate log-in dialogue. */
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
			isConnectActive = connectToIsds(index, true);
		}

		if (isConnectActive) {
			const QStandardItem *accountItem =
			    m_accountModel.itemFromIndex(index);
			MessageDb *messageDb = accountMessageDb(accountItem);
			if (0 == messageDb) {
				continue;
			}

			Worker::jobList.append(Worker::Job(index, messageDb,
			    MSG_RECEIVED));
			Worker::jobList.append(Worker::Job(index, messageDb,
			    MSG_SENT));

			appended = true;
		}

	}

	if (!appended) {
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
	QStandardItem *accountItem = m_accountModel.itemFromIndex(index);
	MessageDb *messageDb = accountMessageDb(accountItem);
	if (0 == messageDb) {
		return;
	}

	/* Try connecting to ISDS, just to generate log-in dialogue. */
	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(index, true)) {
			return;
		}
	}

	Worker::jobList.append(Worker::Job(index, messageDb, MSG_RECEIVED));
	Worker::jobList.append(Worker::Job(index, messageDb, MSG_SENT));

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
	MessageDb *messageDb = 0;

	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);

	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	QStringList dmIdStrs;
	foreach (QModelIndex index, firstMsgColumnIdxs) {
		dmIdStrs.append(index.data().toString());
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
		QStandardItem *accountTopItem =
		     m_accountModel.itemFromIndex(accountTopIndex);

		messageDb = accountMessageDb(accountTopItem);
		Q_ASSERT(0 != messageDb);
	}

	/* Try connecting to ISDS, just to generate log-in dialogue. */
	const AccountModel::SettingsMap accountInfo =
	    accountTopIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(accountTopIndex, true)) {
			return;
		}
	}

	foreach (QString dmIdStr, dmIdStrs) {
		/* Using prepend() just to outrun other jobs. */
		Worker::jobList.append(
		    Worker::Job(accountTopIndex, messageDb, msgDirection,
		        dmIdStr));
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

	const AccountModel::SettingsMap accountInfo =
	    job.acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	showStatusTextPermanently(
	    tr("Synchronise account \"%1\" with ISDS server.")
	        .arg(accountInfo.accountName()));

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(job.acntTopIdx, true)) {
			return;
		}
	}

	m_syncAcntThread = new QThread();
	m_syncAcntWorker = new Worker(m_accountDb);
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
		    SIGNAL(refreshAccountList(const QModelIndex)),
		    this,
		    SLOT(refreshAccountListFromWorker(const QModelIndex)));
	}
	{
		/* Downloading attachment. */
		connect(m_syncAcntWorker,
		    SIGNAL(refreshAttachmentList(const QModelIndex, QString)),
		    this, SLOT(postDownloadSelectedMessageAttachments(
		        const QModelIndex, QString)));
		connect(m_syncAcntWorker,
		    SIGNAL(clearStatusBarAndShowDialog(QString)),
		    this, SLOT(clearInfoInStatusBarAndShowDialog(QString)));
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
	}
}


/* ========================================================================= */
/*
 * Generate account info HTML message.
 */
QString MainWindow::createAccountInfo(const QStandardItem &topItem)
/* ========================================================================= */
{
	const AccountModel::SettingsMap &itemSettings =
	    topItem.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString html;

	html.append("<div style=\"margin-left: 12px;\">");
	html.append("<h3>");
	if (itemSettings.isTestAccount()) {
		html.append(tr("Test account"));
	} else {
		html.append(tr("Standard account"));
	}
	html.append("</h3>");

	html.append(strongAccountInfoLine(tr("Account name"),
	    itemSettings.accountName()));
	html.append("<br/>");
	html.append(strongAccountInfoLine(tr("User name"),
	    itemSettings.userName()));

	AccountEntry accountEntry;
	accountEntry = m_accountDb.accountEntry(
	    itemSettings.userName() + "___True");

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

	html.append("<br/>");
	QString key = itemSettings.userName() + "___True";
	QString info = m_accountDb.getPwdExpirFromDb(key);
	if (info.isEmpty()) {
		info = tr("unknown or without expiration");
	}

	html.append(strongAccountInfoLine(tr("Password expiration date"),
	    info));

	html.append("<br/>");
	MessageDb *db = accountMessageDb(&topItem);
	Q_ASSERT(0 != db);
	QString dbFilePath = db->fileName();
	if (MessageDb::memoryLocation == dbFilePath) {
		dbFilePath = tr("Database is stored in memory. "
		    "Data will be lost on application exit.");
	}
	html.append(strongAccountInfoLine(tr("Local database file location"),
	    dbFilePath));

	html.append("</div>");

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
 * Generate banner.
 */
QString MainWindow::createDatovkaBanner(const QString &version) const
/* ========================================================================= */
{
	QString html = "<br><center>";
	html += "<h2>" +
	    tr("Datovka - Free interface for Datové schránky") + "</h2>";
#ifdef PORTABLE_APPLICATION
	html += "<h3>" + tr("Portable version") + "</h3>";
#endif /* PORTABLE_APPLICATION */
	html += strongAccountInfoLine(tr("Version"), version);
	html += QString("<br><img src=") + ICON_128x128_PATH +
	    "datovka.png />";
	html += "<h3>" + tr("Powered by") + "</h3>";
	html += QString("<br><img src=") + ICON_128x128_PATH + "cznic.png />";
	html += "</center>";
	return html;
}


/* ========================================================================= */
/*
 * Returns user name related to given account item.
 */
QString MainWindow::accountUserName(const QStandardItem *accountItem) const
/* ========================================================================= */
{
	const QStandardItem *accountItemTop;

	if (0 == accountItem) {
		accountItem = m_accountModel.itemFromIndex(
		    ui->accountList->selectionModel()->currentIndex());
	}

	accountItemTop = AccountModel::itemTop(accountItem);
	Q_ASSERT(0 != accountItemTop);

	const AccountModel::SettingsMap &itemSettings =
	    accountItemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	QString userName = itemSettings.userName();
	Q_ASSERT(!userName.isEmpty());

	return userName;
}


/* ========================================================================= */
/*
 * Get message db to selected account item.
 */
MessageDb * MainWindow::accountMessageDb(const QStandardItem *accountItem)
/* ========================================================================= */
{
	const QStandardItem *accountItemTop;
	MessageDb *db = NULL;
	int flags, dbPresenceCode;

	if (0 == accountItem) {
		accountItem = m_accountModel.itemFromIndex(
		    ui->accountList->selectionModel()->currentIndex());
	}

	accountItemTop = AccountModel::itemTop(accountItem);
	Q_ASSERT(0 != accountItemTop);

	/* Get user name and db location. */
	const AccountModel::SettingsMap itemSettings =
	    accountItemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	const QString userName = itemSettings.userName();
	Q_ASSERT(!userName.isEmpty());

	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}

	flags = 0;
	if (itemSettings.isTestAccount()) {
		flags |= DBC_FLG_TESTING;
	}
	if (itemSettings._createdFromScratch()) {
		/* Check database structure on account creation. */
		flags |= DBC_FLG_CHECK_QUICK;
	}
	dbPresenceCode =
	    DbContainer::checkExistingDbFile(userName, dbDir, flags);

	switch (dbPresenceCode) {
	case DBC_ERR_OK:
		{
			if (itemSettings._createdFromScratch()) {
				/* Notify the user on account creation. */
				QString dbFilePath =
				    DbContainer::constructDbFileName(userName,
				        dbDir, itemSettings.isTestAccount());
				QMessageBox::information(this,
				    tr("Datovka: Database file present"),
				    tr("Database file for account '%1' "
				        "already exists.").arg(userName) +
				    "\n\n" +
				    tr("The existing database file '%1' is "
				        "going to be used.").arg(dbFilePath) +
				    "\n\n" +
				    tr("If you want to use a new blank file "
				        "then delete, rename or move the "
				        "existing file so that the "
				        "application can create a new empty "
				        "file."),
				    QMessageBox::Ok);
			}
			db = m_messageDbs.accessMessageDb(userName, dbDir,
			    itemSettings.isTestAccount(), false);
		}
		break;
	case DBC_ERR_MISSFILE:
		{
			if (!itemSettings._createdFromScratch()) {
				/* Not on account creation. */
				QString dbFilePath =
				    DbContainer::constructDbFileName(userName,
				        dbDir, itemSettings.isTestAccount());
				QMessageBox::warning(this,
				    tr("Datovka: Problem loading database"),
				    tr("Could not load data from the database "
				        "for account '%1'").arg(userName) +
				    "\n\n" +
				    tr("Database file '%1' is missing.").arg(
				        dbFilePath) +
				    "\n\n" +
				    tr("I'll try to create an empty one."),
				    QMessageBox::Ok);
			}
			db = m_messageDbs.accessMessageDb(userName, dbDir,
			    itemSettings.isTestAccount(), true);
		}
		break;
	case DBC_ERR_NOTAFILE:
		{
			/* Notify the user that the location is not a file. */
			QString dbFilePath =
			    DbContainer::constructDbFileName(userName,
			        dbDir, itemSettings.isTestAccount());
			QMessageBox::warning(this,
			    tr("Datovka: Problem loading database"),
			    tr("Could not load data from the database "
			        "for account '%1'").arg(userName) +
			    "\n\n" +
			    tr("Database location '%1' is not a file.").arg(
			        dbFilePath),
			    QMessageBox::Ok);
		}
		break;
	case DBC_ERR_ACCESS:
		{
			/* Notify that the user does not have enough rights. */
			QString dbFilePath =
			    DbContainer::constructDbFileName(userName,
			        dbDir, itemSettings.isTestAccount());
			QMessageBox::warning(this,
			    tr("Datovka: Problem loading database"),
			    tr("Could not load data from the database "
			        "for account '%1'").arg(userName) +
			    "\n\n" +
			    tr("Database file '%1' cannot be accessed.").arg(
			        dbFilePath) +
			    "\n\n" +
			    tr("You don't have enough access rights to use "
			        "the file."),
			    QMessageBox::Ok);
		}
		break;
	case DBC_ERR_CREATE:
		{
			/* This error should not be returned. */
		}
		break;
	case DBC_ERR_DATA:
		{
			/*
			 * Database file is not a database file or is
			 * corrupted.
			 */
			QString dbFilePath =
			    DbContainer::constructDbFileName(userName,
			        dbDir, itemSettings.isTestAccount());
			QMessageBox::warning(this,
			    tr("Datovka: Problem loading database"),
			    tr("Could not load data from the database "
			        "for account '%1'").arg(userName) +
			    "\n\n" +
			    tr("Database file '%1' cannot used.").arg(
			        dbFilePath) +
			    "\n\n" +
			    tr("The file either does not contain an sqlite "
			        "database or the file is corrupted."),
			    QMessageBox::Ok);
		}
		break;
	default:
		/* The code should not end here. */
		break;
	}

	if (itemSettings._createdFromScratch()) {
		/* Notify only once. */
		const QModelIndex index = ui->accountList->currentIndex();
		QStandardItem *item = m_accountModel.itemFromIndex(index);
		item = AccountModel::itemTop(item);
		AccountModel::SettingsMap itemSett =
		    item->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		itemSett._setCreatedFromScratch(false);
		item->setData(itemSett, ROLE_ACNT_CONF_SETTINGS);
	}

	/*
	 * TODO -- Give the user some recovery options such as
	 * move/rename/remove the corrupted file or remove/ignore the affected
	 * account.
	 */

	if (NULL == db) {
		/*
		 * TODO -- generate notification dialogue and give the user
		 * a choice between aborting program and skipping account?
		 */
		QString dbFilePath = DbContainer::constructDbFileName(userName,
		    dbDir, itemSettings.isTestAccount());
		QMessageBox::critical(this,
		    tr("Datovka: Database opening error"),
		    tr("Could not load data from the database "
		        "for account '%1'").arg(userName) +
		    "\n\n" +
		    tr("Database file '%1' cannot be created or is "
		        "corrupted.").arg(dbFilePath),
		    QMessageBox::Ok);
		/*
		 * The program has to be aborted right now. The method
		 * QCoreApplication::exit(EXIT_FAILURE) uses the event loop
		 * whereas some event may be already planned and will crash
		 * because of the returnning NULL pointer.
		 * Therefore exit() should be used.
		 */
	}

	return db;
}



/* ========================================================================= */
/*
 * Get storage paths to selected account item.
 */
void MainWindow::setAccountStoragePaths(const QStandardItem *accountItem)
/* ========================================================================= */
{
	debugFuncCall();

	const QStandardItem *accountItemTop;

	if (0 == accountItem) {
		accountItem = m_accountModel.itemFromIndex(
		    ui->accountList->selectionModel()->currentIndex());
	}

	accountItemTop = AccountModel::itemTop(accountItem);
	Q_ASSERT(0 != accountItemTop);

	const AccountModel::SettingsMap &itemSettings =
	    accountItemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();

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
void MainWindow::ensureConfPresence(void) const
/* ========================================================================= */
{
	if (!QDir(globPref.confDir()).exists()) {
		QDir(globPref.confDir()).mkpath(".");
	}
	if (!QFile(globPref.loadConfPath()).exists()) {
		QFile file(globPref.loadConfPath());
		file.open(QIODevice::ReadWrite);
		file.close();
	}
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
			const AccountModel::SettingsMap &itemSettings =
			    item->data(ROLE_ACNT_CONF_SETTINGS).toMap();
			QString user = itemSettings.userName();
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
	    SLOT(accountItemMarkAllRead()));
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
#ifdef PORTABLE_APPLICATION
	ui->actionChange_data_directory->setEnabled(false);
#endif /* PORTABLE_APPLICATION */

	/* Message. */
	connect(ui->actionDownload_message_signed, SIGNAL(triggered()), this,
	    SLOT(downloadSelectedMessageAttachments()));
	connect(ui->actionReply, SIGNAL(triggered()), this,
	    SLOT(createAndSendMessageReply()));
	connect(ui->actionSignature_detail, SIGNAL(triggered()), this,
	    SLOT(showSignatureDetails()));
	connect(ui->actionAuthenticate_message, SIGNAL(triggered()), this,
	    SLOT(verifyMessage()));
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

	/* Help. */
	connect(ui->actionAbout_Datovka, SIGNAL(triggered()), this,
	    SLOT(aboutApplication()));
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
	    SLOT(verifyMessage()));
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
	ui->actionAuthenticate_message_file->setEnabled(false);
	ui->actionExport_correspondence_overview->setEnabled(false);
}


/* ========================================================================= */
/*
 *  Set default settings of mainwindow.
 */
void MainWindow::setMessageActionVisibility(bool action) const
/* ========================================================================= */
{
	ui->menuMessage->setEnabled(action);
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

		const AccountModel::SettingsMap &itemSettings =
		    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		const QString userName = itemSettings.userName();

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
	MessageDb *db;
	QList<QString> yearList;
	int unreadMsgs;

	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	/* Get database id. */
	topItem = m_accountModel.itemFromIndex(index);
	Q_ASSERT(0 != topItem);
	const AccountModel::SettingsMap &itemSettings =
	    topItem->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	const QString userName = itemSettings.userName();
	Q_ASSERT(!userName.isEmpty());
	db = accountMessageDb(topItem);
	Q_ASSERT(0 != db);
	QString dbId = m_accountDb.dbId(userName + "___True");

	/* Received. */
	unreadMsgs = db->msgsRcvdUnreadWithin90Days(dbId);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = db->msgsRcvdYears(dbId, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Received" << yearList.value(j);
		unreadMsgs = db->msgsRcvdUnreadInYear(dbId, yearList.value(j));
		m_accountModel.updateYear(topItem,
		    AccountModel::nodeReceivedYear, yearList.value(j),
		    unreadMsgs);
	}
	/* Sent. */
	//unreadMsgs = db->msgsSntUnreadWithin90Days(dbId);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentSent, 0);
	yearList = db->msgsSntYears(dbId, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Sent" << yearList.value(j);
		//unreadMsgs = db->msgsSntUnreadInYear(dbId, yearList.value(j));
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
	MessageDb *db;
	QList<QString> yearList;
	int unreadMsgs;

	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	m_accountModel.removeYearNodes(index);

	/* Get database id. */
	topItem = m_accountModel.itemFromIndex(index);
	Q_ASSERT(0 != topItem);
	const AccountModel::SettingsMap &itemSettings =
	    topItem->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	const QString userName = itemSettings.userName();
	Q_ASSERT(!userName.isEmpty());
	db = accountMessageDb(topItem);
	Q_ASSERT(0 != db);
	QString dbId = m_accountDb.dbId(userName + "___True");

	/* Received. */
	unreadMsgs = db->msgsRcvdUnreadWithin90Days(dbId);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = db->msgsRcvdYears(dbId, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Received" << yearList.value(j);
		unreadMsgs = db->msgsRcvdUnreadInYear(dbId, yearList.value(j));
		m_accountModel.addYear(topItem, AccountModel::nodeReceivedYear,
		    yearList.value(j), unreadMsgs);
	}
	/* Sent. */
	//unreadMsgs = db->msgsSntUnreadWithin90Days(dbId);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentSent, 0);
	yearList = db->msgsSntYears(dbId, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Sent" << yearList.value(j);
		//unreadMsgs = db->msgsSntUnreadInYear(dbId, yearList.value(j));
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
	MessageDb *db;
	QList<QString> yearList;
	int unreadMsgs;

	m_accountModel.removeAllYearNodes();

	//qDebug() << "Generating years";

	for (int i = 0; i < m_accountModel.rowCount(); ++i) {
		/* Get database ID. */
		itemTop = m_accountModel.item(i, 0);
		Q_ASSERT(0 != itemTop);
		const AccountModel::SettingsMap &itemSettings =
		    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		const QString userName = itemSettings.userName();
		Q_ASSERT(!userName.isEmpty());
		db = accountMessageDb(itemTop);
		if (0 == db) {
			/*
			 * Skip creation of leaves when no database is present.
			 */
			continue;
		}
		QString dbId = m_accountDb.dbId(userName + "___True");

		/* Received. */
		unreadMsgs = db->msgsRcvdUnreadWithin90Days(dbId);
		m_accountModel.updateRecentUnread(itemTop,
		    AccountModel::nodeRecentReceived, unreadMsgs);
		yearList = db->msgsRcvdYears(dbId, DESCENDING);
		for (int j = 0; j < yearList.size(); ++j) {
			//qDebug() << yearList.value(j);
			unreadMsgs = db->msgsRcvdUnreadInYear(dbId,
			    yearList.value(j));
			m_accountModel.addYear(itemTop,
			    AccountModel::nodeReceivedYear, yearList.value(j),
			    unreadMsgs);
		}
		/* Sent. */
		//unreadMsgs = db->msgsSntUnreadWithin90Days(dbId);
		m_accountModel.updateRecentUnread(itemTop,
		    AccountModel::nodeRecentSent, 0);
		yearList = db->msgsSntYears(dbId, DESCENDING);
		for (int j = 0; j < yearList.size(); ++j) {
			//qDebug() << yearList.value(j);
			//unreadMsgs = db->msgsSntUnreadInYear(dbId,
			//    yearList.value(j));
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
 * Creates and sends new message.
 */
void MainWindow::createAndSendMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	/*
	 * TODO -- This method copies createAndSendMessageReply().
	 * Delete one of them.
	 */
	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();
	QModelIndex acntTopIndex = AccountModel::indexTop(selectedAcntIndex);

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");
	QString senderName = m_accountDb.senderNameGuess(userName + "___True");
	QList<QString> accountData =
	    m_accountDb.getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		return;
	}

	QString dbType = accountData.at(0);
	bool dbEffectiveOVM = (accountData.at(1) == "1") ? true : false;
	bool dbOpenAddressing = (accountData.at(2) == "1") ? true : false;

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(acntTopIndex, true)) {
			return;
		}
	}

	showStatusTextWithTimeout(tr("Create and send a new message."));

	AccountModel::SettingsMap accountInfo =
	    acntTopIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	QString lastAttachAddPath;
	if (globPref.use_global_paths) {
		lastAttachAddPath = globPref.add_file_to_attachments_path;
	} else {
		lastAttachAddPath = accountInfo.lastAttachAddPath();
	}
	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, dbId, senderName,
	    DlgSendMessage::ACT_NEW, *(ui->accountList), *(ui->messageList),
	    accountInfo, dbType, dbEffectiveOVM, dbOpenAddressing,
	    lastAttachAddPath, this);

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

	QDialog *newAccountDialog = new DlgCreateAccount(*(ui->accountList),
	   m_accountDb, QModelIndex(), DlgCreateAccount::ACT_ADDNEW, this);

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

	QString userName = accountUserName();
	MessageDb *db = accountMessageDb(itemTop);
	Q_ASSERT(0 != db);

	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	QString accountName = itemSettings.accountName();

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
		/* Delete account and its message db */
		if (itemTop->hasChildren()) {
			itemTop->removeRows(0, itemTop->rowCount());
		}
		m_accountDb.deleteAccountInfo(userName + "___True");
		ui->accountList->model()->removeRow(currentTopRow);
		if (m_messageDbs.deleteMessageDb(db)) {
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
		m_accountDb.deleteAccountInfo(userName + "___True");
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

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(index, true)) {
			return;
		}
	}

	showStatusTextWithTimeout(tr("Change password of account "
	    "\"%1\".").arg(accountInfo.accountName()));

	QDialog *changePwd = new DlgChangePwd(dbId, *(ui->accountList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(), this);
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
	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

//	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
//		if (!connectToIsds(index, true)) {
//			//return;
//		}
//	}

	showStatusTextWithTimeout(tr("Change properties of account "
	    "\"%1\".").arg(accountInfo.accountName()));

	QDialog *editAccountDialog = new DlgCreateAccount(*(ui->accountList),
	   m_accountDb, QModelIndex(), DlgCreateAccount::ACT_EDIT, this);
	//editAccountDialog->exec();

	if (QDialog::Accepted == editAccountDialog->exec()) {
		showStatusTextWithTimeout(tr("Account \"%1\" was updated.")
		    .arg(accountInfo.userName()));
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

	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();

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

	/* Get current settings. */
	AccountModel::SettingsMap itemSettings =
	    m_accountModel.settingsMap(item);

	QString fileName;

	/* 1 = is test account, 0 = is legal account */
	if (itemSettings.isTestAccount()) {
		fileName = itemSettings.userName() + "___1.db";
	} else {
		fileName = itemSettings.userName() + "___0.db";
	}

	MessageDb *messageDb = accountMessageDb(item);
	Q_ASSERT(0 != messageDb);

	qDebug() << fileName << action;

	/* Move account database into new directory */
	if ("move" == action) {
		if (m_messageDbs.moveMessageDb(messageDb, newDir)) {

			itemSettings.setDbDir(newDir);
			m_accountModel.setSettingsMap(item, itemSettings);
			saveSettings();

			qDebug() << "Move" << fileName << "from"
			    << oldDir << "to" << newDir << "...done";

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("Database file") + "\n\n" + fileName + "\n\n" +
			    tr("was successfully moved to") + "\n\n"
			    + newDir,
			    QMessageBox::Ok);
		} else {
			qDebug() << "Move" << fileName << "from"
			    << oldDir << "to" << newDir << "...error";

			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("Database file") + "\n\n" + fileName + "\n\n" +
			    tr("was NOT successfully moved to") + "\n\n"
			    + newDir,
			    QMessageBox::Ok);
		}

	/* Copy account database into new directory */
	} else if ("copy" == action) {
		if (m_messageDbs.copyMessageDb(messageDb, newDir)) {

			itemSettings.setDbDir(newDir);
			m_accountModel.setSettingsMap(item, itemSettings);
			saveSettings();

			qDebug() << "Copy" << fileName << "from"
			    << oldDir << "to" << newDir << "...done";

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("Database file") + "\n\n" + fileName + "\n\n" +
			    tr("was successfully copied to") + "\n\n"
			    + newDir,
			    QMessageBox::Ok);
		} else {
			qDebug() << "Copy" << fileName << "from"
			    << oldDir << "to" << newDir << "...error";

			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("Database file") + "\n\n" + fileName + "\n\n" +
			    tr("was NOT successfully copied to") + "\n\n"
			    + newDir,
			    QMessageBox::Ok);
		}

	/* Create a new account database into new directory */
	} else if ("new" == action) {
		if (m_messageDbs.reopenMessageDb(messageDb, newDir)) {

			itemSettings.setDbDir(newDir);
			m_accountModel.setSettingsMap(item, itemSettings);
			saveSettings();

			qDebug() << "Create new" << fileName << "in"
			    << newDir << "...done";

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("New database file") + "\n\n" + fileName + "\n\n" +
			    tr("was successfully created to") + "\n\n"
			    + newDir,
			    QMessageBox::Ok);
		} else {
			qDebug() << "Create new" << fileName << "in"
			    << newDir << "...error";

			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("New database file") + "\n\n" + fileName + "\n\n" +
			    tr("was NOT successfully created to") + "\n\n"
			    + newDir,
			    QMessageBox::Ok);
		}
	} else {
		Q_ASSERT(0);
	}
}


/* ========================================================================= */
/*
 * Creates and sends a message reply for selected account.
 */
void MainWindow::createAndSendMessageReply(void)
/* ========================================================================= */
{
	debugSlotCall();

	/*
	 * TODO -- This method copies createAndSendMessage().
	 * Delete one of them.
	 */

	/* First column. */
	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QAbstractItemModel *tableModel = ui->messageList->model();
	Q_ASSERT(0 != tableModel);

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	QVector<QString> replyData = messageDb->msgsReplyDataTo(
	    firstMsgColumnIdxs.first().data().toLongLong());

	/* TODO */
	QModelIndex acntTopIndex =
	    AccountModel::indexTop(ui->accountList->currentIndex());
	Q_ASSERT(acntTopIndex.isValid());

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");


	QString senderName = m_accountDb.senderNameGuess(userName + "___True");
	QList<QString> accountData =
	    m_accountDb.getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		return;
	}

	QString dbType = accountData.at(0);
	bool dbEffectiveOVM = (accountData.at(1) == "1") ? true : false;
	bool dbOpenAddressing = (accountData.at(2) == "1") ? true : false;

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(acntTopIndex, true)) {
			return;
		}
	}

	showStatusTextWithTimeout(tr("Create and send reply on the message."));

	AccountModel::SettingsMap accountInfo =
	    acntTopIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	QString lastAttachAddPath;
	if (globPref.use_global_paths) {
		lastAttachAddPath = globPref.add_file_to_attachments_path;
	} else {
		lastAttachAddPath = accountInfo.lastAttachAddPath();
	}

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, dbId, senderName,
	    DlgSendMessage::ACT_REPLY, *(ui->accountList), *(ui->messageList),
	    accountInfo, dbType, dbEffectiveOVM, dbOpenAddressing,
	    lastAttachAddPath, this, replyData[0], replyData[1], replyData[2],
	    replyData[3], replyData[4], replyData[5], replyData[6],
	    replyData[7], replyData[8]);
	if (newMessageDialog->exec() == QDialog::Accepted) {

		showStatusTextWithTimeout(tr("Message from account \"%1\" was "
		    "send.").arg(accountInfo.accountName()));

		/*
		 * Message model must not be regenerated because when
		 * generating a message reply the user must be on received
		 * messages.
		 *
		 * TODO -- Regenerate year list (as this could add entries).
		 */
	}

	if (!globPref.use_global_paths) {
		m_add_attach_dir = lastAttachAddPath;
		storeExportPath();
	}

	setDefaultProgressStatus();
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

	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(index, true)) {
			return;
		}
	}

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");
	QList<QString> accountData =
	    m_accountDb.getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		return;
	}

	QString dbType = accountData.at(0);
	bool dbEffectiveOVM = (accountData.at(1) == "1") ? true : false;
	bool dbOpenAddressing = (accountData.at(2) == "1") ? true : false;

	showStatusTextWithTimeout(tr("Find databoxes from "
	    "account \"%1\".").arg(accountInfo.accountName()));

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
	for (i = 3; i < (MessageDb::receivedItemIds.size() - 3); ++i) {
		ui->messageList->resizeColumnToContents(i);
	}
	/* Last three columns display icons. */
	for (; i < MessageDb::receivedItemIds.size(); ++i) {
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
	for (i = 3; i < (MessageDb::sentItemIds.size() - 1); ++i) {
		ui->messageList->resizeColumnToContents(i);
	}
	/* Last column displays an icon. */
	for (; i < MessageDb::receivedItemIds.size(); ++i) {
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
void MainWindow::refreshAccountListFromWorker(const QModelIndex acntTopIdx)
/* ========================================================================= */
{
	debugSlotCall();

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
qdatovka_error MainWindow::verifySelectedMessage(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(acntTopIdx, true)) {
			return Q_CONNECT_ERROR;
		}
	}

	struct isds_hash *hashIsds = NULL;

	status = isds_download_message_hash(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &hashIsds);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return Q_ISDS_ERROR;
	}

	struct isds_hash *hashLocal = NULL;
	hashLocal = (struct isds_hash *)
	    malloc(sizeof(struct isds_hash));

	if (hashLocal == NULL) {
		free(hashLocal);
		return Q_GLOBAL_ERROR;
	}

	memset(hashLocal, 0, sizeof(struct isds_hash));
	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QStringList hashLocaldata = messageDb->msgsGetHashFromDb(dmID);

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
bool MainWindow::getOwnerInfoFromLogin(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	debugSlotCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(acntTopIdx, true)) {
			return false;
		}
	}

	struct isds_DbOwnerInfo *db_owner_info = NULL;

	status = isds_GetOwnerInfoFromLogin(isdsSessions.session(
	    accountInfo.userName()), &db_owner_info);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}

	QString username = accountInfo.userName() + "___True";
	QString birthDate;
	if ((NULL != db_owner_info->birthInfo) &&
	    (NULL != db_owner_info->birthInfo->biDate)) {
		birthDate = tmBirthToDbFormat(db_owner_info->birthInfo->biDate);
	}

	int ic = 0;
	if (NULL != db_owner_info->ic) {
		ic = atoi(db_owner_info->ic);
	}

	m_accountDb.insertAccountIntoDb(
	    username,
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

	return true;
}


/* ========================================================================= */
/*
* Get data about logged in user.
*/
bool MainWindow::getUserInfoFromLogin(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(acntTopIdx, true)) {
			return false;
		}
	}

	struct isds_DbUserInfo *db_user_info = NULL;


	status = isds_GetUserInfoFromLogin(isdsSessions.session(
	    accountInfo.userName()), &db_user_info);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}

	/* TODO - insert data into db */

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
void MainWindow::createAccountFromDatabaseFileList(QStringList filePathList)
/* ========================================================================= */
{

	debugFuncCall();

	int dbCnt = filePathList.size();

	if (dbCnt == 0) {
		return;
	}

	QPair<QString,QString> importDBinfo;
	QStringList fileNameParts;
	QString accountName;

	AccountModel::SettingsMap itemSettings;

	QStringList currentAccountList;
	int accountCount = ui->accountList->model()->rowCount();
	for (int i = 0; i < accountCount; i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		currentAccountList.append(accountInfo.userName());
	}

	for (int i = 0; i < filePathList.size(); ++i) {
		importDBinfo.first = filePathList.at(i);
		QString file = QFileInfo(filePathList.at(i)).fileName();
		if (file.contains("___")) {
			fileNameParts = file.split("___");
			accountName = fileNameParts[0];
			fileNameParts = fileNameParts[1].split(".");

			bool exists = false;
			for (int j = 0; j < accountCount; ++j) {
				if (currentAccountList.at(j) == accountName) {
					exists = true;
					break;
				}
			}

			if (exists) {
				importDBinfo.second =
				    tr("Account with username '%1' and "
				    "its message database already exist. "
				    "New account was not created and "
				    "selected database file was not "
				    "associated with this account.").
				    arg(accountName);
			} else {
				if (fileNameParts[0] == "1") {
					itemSettings.setTestAccount(true);
				} else if (fileNameParts[0] == "0") {
					itemSettings.setTestAccount(false);
				} else {
					importDBinfo.second =
					    tr("This file does not contain a "
					    "valid message database or file "
					    "name has wrong format.");
				}
				itemSettings.setAccountName(accountName);
				itemSettings.setUserName(accountName);
				itemSettings.setLoginMethod(LIM_USERNAME);
				itemSettings.setPassword("");
				itemSettings.setRememberPwd(false);
				itemSettings.setSyncWithAll(false);
				itemSettings.setDbDir(
				    m_on_import_database_dir_activate);
				m_accountModel.addAccount(accountName,
				    itemSettings);
				importDBinfo.second =
				    tr("Account with name '%1' has been "
				    "created (username '%1').").arg(accountName)
				    + " " +
				    tr("This database file has been set as "
				    "actual message database for this account. "
				    "Maybe you have to change account "
				    "properties for correct login to the "
				    "server Datové schránky.");
			}
		} else {
			importDBinfo.first = filePathList.at(i);
			importDBinfo.second = tr("This file does not contain a"
			    " valid message database or file name has wrong "
			    "format.");
		}

		QMessageBox::information(this,
		    tr("Create account: %1").arg(accountName),
		    tr("File") + ": " + importDBinfo.first +
		    "\n\n" + importDBinfo.second,
		    QMessageBox::Ok);
	}

	saveSettings();
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

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

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

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(acntTopIdx, true)) {
			return Q_CONNECT_ERROR;
		}
	}

	status = isds_authenticate_message(isdsSessions.session(
	    accountInfo.userName()), bytes.data(), length);

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
void MainWindow::verifyMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	/* First column. */
	QModelIndexList firstMsgColumnIdxs =
	    ui->messageList->selectionModel()->selectedRows(0);
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	switch (verifySelectedMessage(acntTopIdx, msgIdx)) {
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

	struct isds_ctx *dummy_session = NULL; /* Logging purposes. */
	struct isds_message *message = NULL;
	int zfoType = ImportZFODialog::IMPORT_MESSAGE_ZFO;
	QDialog *viewDialog;

	QString fileName = QFileDialog::getOpenFileName(this,
	    tr("Add ZFO file"), m_on_export_zfo_activate,
	    tr("ZFO file (*.zfo)"));

	if (fileName.isEmpty()) {
		goto fail;
	}

	m_on_export_zfo_activate =
	    QFileInfo(fileName).absoluteDir().absolutePath();

	dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		qDebug() << "Cannot create dummy ISDS session.";

	}

	message = loadZfoFile(dummy_session, fileName,
	    ImportZFODialog::IMPORT_MESSAGE_ZFO);
	if (NULL == message) {
		message = loadZfoFile(dummy_session, fileName,
		    ImportZFODialog::IMPORT_DELIVERY_ZFO);
		    zfoType = ImportZFODialog::IMPORT_DELIVERY_ZFO;
		if (NULL == message) {
			qDebug() << "Cannot parse file" << fileName;
			QMessageBox::warning(this,
			    tr("Content parsing error"),
			    tr("Cannot parse the content of ") + fileName + ".",
			    QMessageBox::Ok);
			goto fail;
		}
	}

	/* Generate dialog showing message content. */
	viewDialog = new DlgViewZfo(message, zfoType, this);
	viewDialog->exec();

	isds_message_free(&message);
	isds_ctx_free(&dummy_session);

	return;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}
	if (NULL != dummy_session) {
		isds_ctx_free(&dummy_session);
	}
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

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QDialog *correspondence_overview = new DlgCorrespondenceOverview(
	    *messageDb, dbId,
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(),
	    m_export_correspond_dir,
	    this);

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
	connect(importZfo, SIGNAL(returnZFOAction(int, int)),
	    this, SLOT(createZFOListForImport(int, int)));
	importZfo->exec();
}


/* ========================================================================= */
/*
 * Create ZFO file(s) list for import into database.
 */
void MainWindow::createZFOListForImport(int zfoType, int importType)
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
QList<MainWindow::accountDataStruct> MainWindow::createAccountInfoForZFOImport(void)
/* ========================================================================= */
{
	debugFuncCall();

	QString userName;
	accountDataStruct accountData;
	QList<accountDataStruct> accountList;
	accountList.clear();

	/* get username, accountName, ID of databox and pointer to database
	 * for all accounts from settings */
	for (int i = 0; i < ui->accountList->model()->rowCount(); i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		QStandardItem *accountItem = m_accountModel.itemFromIndex(index);
		MessageDb *messageDb = accountMessageDb(accountItem);
		Q_ASSERT(0 != messageDb);
		userName = accountInfo.userName();
		accountData.acntIndex = index;
		accountData.username = userName;
		accountData.accountName = accountInfo.accountName();
		accountData.databoxID = m_accountDb.dbId(userName + "___True");
		accountData.messageDb = messageDb;
		accountList.append(accountData);
	}

	return accountList;
}


/* ========================================================================= */
/*
 * Get message type of import ZFO file (message/delivery/unknown).
 */
int MainWindow::getMessageTypeFromZFO(QString file)
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
    int zfoType)
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

	QList<accountDataStruct> const accountList =
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
    const QList<accountDataStruct> &accountList, const QStringList &files,
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
		int dmId = atoi(message->envelope->dmID);

		for (int j = 0; j < accountList.size(); j++) {
			/* check if message envelope is in database */
			if (-1 != accountList.at(j).messageDb->msgsStatusIfExists(dmId)) {
				/* check if raw is in database */
				if (!accountList.at(j).messageDb->isDeliveryInfoRawDb(dmId)) {
					/* Is/was ZFO message in ISDS */
					resISDS = isImportMsgInISDS(files.at(i),
					    accountList.at(j).acntIndex);
					if (resISDS == MSG_IS_IN_ISDS) {
						if (Q_SUCCESS ==
						    Worker::storeDeliveryInfo(true,
						    *(accountList.at(j).messageDb), message)) {
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
void  MainWindow::importMessageZFO(const QList<accountDataStruct> &accountList,
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
		int dmId = atoi(message->envelope->dmID);
		int resISDS = 0;
		bool import = false;
		bool exists = false;

		/* message type recognition {sent,received}, insert into DB */
		for (int j = 0; j < accountList.size(); j++) {
			/* is sent */
			if (accountList.at(j).databoxID == dbIDSender) {

				isSent = accountList.at(j).username;
				qDebug() << dmId << "isSent" << isSent;

				/* Is/was ZFO message in ISDS */
				resISDS = isImportMsgInISDS(files.at(i),
				    accountList.at(j).acntIndex);
				if (resISDS == MSG_IS_IN_ISDS) {
					if (-1 == accountList.at(j).messageDb->
					    msgsStatusIfExists(dmId)) {
						Worker::storeEnvelope(MSG_SENT, *(accountList.at(j).messageDb), message->envelope);
						if (Q_SUCCESS == Worker::storeMessage(true, MSG_SENT, *(accountList.at(j).messageDb), message, "", 0, 0)) {
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
					if (-1 == accountList.at(j).messageDb->
					    msgsStatusIfExists(dmId)) {
						Worker::storeEnvelope(MSG_RECEIVED, *(accountList.at(j).messageDb), message->envelope);
						if (Q_SUCCESS == Worker::storeMessage(true, MSG_RECEIVED, *(accountList.at(j).messageDb), message, "", 0, 0)) {
							import = true;
							/* update message state into database */
							accountList.at(j).messageDb->msgSetProcessState(dmId, SETTLED, false);
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
	const AccountModel::SettingsMap accountInfo =
	    accountIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(accountIndex, true)) {
			return MSG_ISDS_ERROR;
		}
	}

	status = isds_authenticate_message(isdsSessions.session(
	    accountInfo.userName()), bytes.data(), length);

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
 * Show help.
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
 * Export message into as ZFO file dialogue.
 */
void MainWindow::exportSelectedMessageAsZFO(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message to "
		    "ZFO was not successful!"));
		return;
	}

	QString dmId = msgIdx.sibling(msgIdx.row(), 0).data().toString();

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QByteArray base64 = messageDb->msgsMessageBase64(dmID);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Message export error!"));
		msgBox.setText(tr("Cannot export complete message")
		    + " " + dmId + ".");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download the whole message before "
		        "exporting.") + "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId)) {
				showStatusTextWithTimeout(tr("Export of message "
				"\"%1\" to ZFO was not successful!")
				.arg(dmId));
				return;
			} else {
				base64 = messageDb->msgsMessageBase64(dmID);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message "
			"\"%1\" to ZFO was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName = m_on_export_zfo_activate + QDir::separator() +
	    "DDZ_" + dmId + ".zfo";

	Q_ASSERT(!fileName.isEmpty());

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save message as ZFO file"), fileName, tr("ZFO file (*.zfo)"));

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	m_on_export_zfo_activate =
	    QFileInfo(fileName).absoluteDir().absolutePath();
	storeExportPath();

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
bool MainWindow::downloadCompleteMessage(QString dmId)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex accountIndex = ui->accountList->currentIndex();
	Q_ASSERT(accountIndex.isValid());
	accountIndex = AccountModel::indexTop(accountIndex);
	    /* selection().indexes() ? */

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

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

	const AccountModel::SettingsMap accountInfo =
	    accountIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!connectToIsds(accountIndex, true)) {
			return false;
		}
	}

	if (Q_SUCCESS == Worker::downloadMessage(accountIndex, dmId, true,
	        msgDirect, *messageDb, QString(), 0, 0)) {
		/* TODO -- Wouldn't it be better with selection changed? */
		postDownloadSelectedMessageAttachments(accountIndex, dmId);
		return true;
	}

	return false;
}


/* ========================================================================= */
/*
 * Export delivery information as ZFO file dialog.
 */
void MainWindow::exportDeliveryInfoAsZFO(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info to ZFO was not successful!"));
		return;
	}

	QString dmId = msgIdx.sibling(msgIdx.row(), 0).data().toString();

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmID);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Delivery info export error!"));
		msgBox.setText(tr("Cannot export delivery info for message")
		    + " " + dmId + ".");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download message before export.") +
		    "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId)) {
				showStatusTextWithTimeout(tr("Export of "
				    "message delivery info \"%1\" to ZFO was "
				    "not successful!").arg(dmId));
				return;
			} else {
				base64 =
				    messageDb->msgsGetDeliveryInfoBase64(dmID);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message delivery "
			"info \"%1\" to ZFO was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName = m_on_export_zfo_activate + QDir::separator() +
	    "DDZ_" + dmId + "_info.zfo";

	Q_ASSERT(!fileName.isEmpty());

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save delivery info as ZFO file"), fileName,
	    tr("ZFO file (*.zfo)"));

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info \"%1\" to ZFO was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	m_on_export_zfo_activate =
	    QFileInfo(fileName).absoluteDir().absolutePath();
	storeExportPath();

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
void MainWindow::exportDeliveryInfoAsPDF(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message delivery info "
		    "to PDF was not successful!"));
		return;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmID);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);;
		msgBox.setWindowTitle(tr("Delivery info export error!"));
		msgBox.setText(tr("Cannot export delivery info for message")
		    + " " + dmId + ".");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download message before export.") +
		    "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId)) {
				showStatusTextWithTimeout(tr("Export of "
				    "message delivery info \"%1\" to PDF was "
				    "not successful!").arg(dmId));
				return;
			} else {
				base64 =
				    messageDb->msgsGetDeliveryInfoBase64(dmID);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message delivery "
			"info \"%1\" to PDF was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName = m_on_export_zfo_activate + QDir::separator() +
	    "DD_" + dmId + ".pdf";

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save delivery info as PDF file"), fileName,
	    tr("PDF file (*.pdf)"));
	//, QString(), 0, QFileDialog::DontUseNativeDialog);

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message delivery "
		    "info \"%1\" to PDF was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	m_on_export_zfo_activate =
	    QFileInfo(fileName).absoluteDir().absolutePath();
	storeExportPath();

	QTextDocument doc;
	doc.setHtml(messageDb->deliveryInfoHtmlToPdf(dmID));

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
void MainWindow::exportMessageEnvelopeAsPDF(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusTextWithTimeout(tr("Export of message envelope to "
		    "PDF was not successful!"));
		return;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QByteArray base64 = messageDb->msgsMessageBase64(dmID);
	if (base64.isEmpty()) {

		QMessageBox msgBox(this);;
		msgBox.setWindowTitle(tr("Message export error!"));
		msgBox.setText(tr("Cannot export complete message")
		    + " " + dmId + ".");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("First you must download the whole message before "
		        "exporting.") + "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId)) {
				showStatusTextWithTimeout(tr("Export of message "
				"envelope \"%1\" to PDF was not successful!")
				.arg(dmId));
				return;
			} else {
				base64 = messageDb->msgsMessageBase64(dmID);
			}
		} else {
			showStatusTextWithTimeout(tr("Export of message "
			"envelope \"%1\" to PDF was not successful!").arg(dmId));
			return;
		}
	}

	QString fileName = m_on_export_zfo_activate + QDir::separator() +
	    "OZ_" + dmId + ".pdf";

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save message envelope as PDF file"), fileName,
	    tr("PDF file (*.pdf)"));

	if (fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message "
		    "envelope \"%1\" to PDF was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	m_on_export_zfo_activate =
	    QFileInfo(fileName).absoluteDir().absolutePath();
	storeExportPath();

	QString userName = accountUserName();
	QList<QString> accountData =
	    m_accountDb.getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		showStatusTextWithTimeout(tr("Export of message "
		    "envelope \"%1\" to PDF was not successful!").arg(dmId));
		return;
	}

	QTextDocument doc;
	doc.setHtml(messageDb->envelopeInfoHtmlToPdf(dmID, accountData.at(0)));

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
	QString dmId = msgIdx.data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QByteArray base64 = messageDb->msgsMessageBase64(dmID);
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

	QString fileName = TMP_ATTACHMENT_PREFIX "DDZ_" + dmId + ".zfo";
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
	QString dmId = msgIdx.data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	qint64 dmID = dmId.toLongLong();

	QByteArray base64 = messageDb->msgsMessageBase64(dmID);
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

	QString fileName = TMP_ATTACHMENT_PREFIX "DDZ_" + dmId + "_info.zfo";
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
	QString dmId = msgIdx.data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	if (0 == messageDb) {
		return;
	}
	qint64 dmID = dmId.toLongLong();

	QDialog *signature_detail = new DlgSignatureDetail(*messageDb, dmID,
	    this);
	signature_detail->exec();
}


/* ========================================================================= */
/*
* This is call if connection to ISDS fails. Message info for user is generated.
*/
void MainWindow::showConnectionErrorMessageBox(int status, QString accountName,
   QString isdsMsg)
/* ========================================================================= */
{
	QString msgBoxTitle = "";
	QString msgBoxContent = "";

	if (isdsMsg.isNull() || isdsMsg.isEmpty()) {
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
bool MainWindow::checkConnectionError(int status, QString accountName,
    bool showDialog, QString isdsMsg)
/* ========================================================================= */
{
	switch (status) {
	case IE_SUCCESS:
		statusOnlineLabel->setText(tr("Mode: online"));
		return true;
		break;
	default:
		qDebug() << "Account" << accountName << ":"
		    << isds_strerror((isds_error)status) << status;
		if (showDialog) {
			showConnectionErrorMessageBox(status, accountName, isdsMsg);
		}
		return false;
		break;
	}
}


/* ========================================================================= */
/*
* Login to ISDS server by username and password only.
*/
bool MainWindow::loginMethodUserNamePwd(const QModelIndex acntTopIdx,
    const AccountModel::SettingsMap accountInfo, bool showDialog)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName(),
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString pwd = accountInfo.password();

	if (pwd.isNull() || pwd.isEmpty()) {
		QDialog *editAccountDialog = new DlgCreateAccount(
		    *(ui->accountList), m_accountDb, acntTopIdx,
		    DlgCreateAccount::ACT_PWD, this);
		if (QDialog::Accepted == editAccountDialog->exec()) {
			const AccountModel::SettingsMap accountInfoNew =
			    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			pwd = accountInfoNew.password();
			saveSettings();
		} else {
			showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your databox from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}
	}

	status = isdsLoginUserName(
	    isdsSessions.session(accountInfo.userName()),
	    accountInfo.userName(), pwd, accountInfo.isTestAccount());

	isdsSessions.setSessionTimeout(accountInfo.userName(),
	    globPref.isds_download_timeout_ms); /* Set longer time-out. */

	QString isdsMsg =
	    isds_long_message(isdsSessions.session(accountInfo.userName()));

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog, isdsMsg);
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate only.
*/
bool MainWindow::loginMethodCertificateOnly(const QModelIndex acntTopIdx,
    const AccountModel::SettingsMap accountInfo, bool showDialog)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName(),
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString certPath = accountInfo.p12File();
	if (certPath.isNull() || certPath.isEmpty()) {
		QDialog *editAccountDialog = new DlgCreateAccount(
		    *(ui->accountList), m_accountDb, acntTopIdx,
		    DlgCreateAccount::ACT_CERT, this);
		if (QDialog::Accepted == editAccountDialog->exec()) {
			const AccountModel::SettingsMap accountInfoNew =
			    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			certPath = accountInfoNew.p12File();
			saveSettings();
		} else {
			showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your databox from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}
	}

	status = isdsLoginSystemCert(
	    isdsSessions.session(accountInfo.userName()),
	    certPath, accountInfo.isTestAccount());

	isdsSessions.setSessionTimeout(accountInfo.userName(),
	    globPref.isds_download_timeout_ms); /* Set longer time-out. */

	QString isdsMsg =
	    isds_long_message(isdsSessions.session(accountInfo.userName()));

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog, isdsMsg);
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate, username and password.
*/
bool MainWindow::loginMethodCertificateUserPwd(const QModelIndex acntTopIdx,
    const AccountModel::SettingsMap accountInfo, bool showDialog)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName(),
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString certPath = accountInfo.p12File();
	QString pwd = accountInfo.password();

	if (pwd.isNull() || pwd.isEmpty() ||
	    certPath.isNull() || certPath.isEmpty()) {

		QDialog *editAccountDialog = new DlgCreateAccount(
		    *(ui->accountList), m_accountDb, acntTopIdx,
		    DlgCreateAccount::ACT_CERTPWD, this);
		if (QDialog::Accepted == editAccountDialog->exec()) {
			const AccountModel::SettingsMap accountInfoNew =
			    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			certPath = accountInfoNew.p12File();
			pwd = accountInfoNew.password();
			saveSettings();
		} else {
			showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your databox from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}
	}

	status = isdsLoginUserCertPwd(
	    isdsSessions.session(accountInfo.userName()),
	    accountInfo.userName(), pwd, certPath,
	    accountInfo.isTestAccount());


	QString isdsMsg =
	    isds_long_message(isdsSessions.session(accountInfo.userName()));

	isdsSessions.setSessionTimeout(accountInfo.userName(),
	    globPref.isds_download_timeout_ms); /* Set longer time-out. */

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog, isdsMsg);
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate and databox ID.
*/
bool MainWindow::loginMethodCertificateIdBox(const QModelIndex acntTopIdx,
    const AccountModel::SettingsMap accountInfo, bool showDialog)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName(),
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString certPath = accountInfo.p12File();
	QString idBox;

	QDialog *editAccountDialog = new DlgCreateAccount(
	    *(ui->accountList), m_accountDb, acntTopIdx,
	    DlgCreateAccount::ACT_IDBOX, this);
	if (QDialog::Accepted == editAccountDialog->exec()) {
		const AccountModel::SettingsMap accountInfoNew =
		    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		certPath = accountInfoNew.p12File();
		idBox = accountInfoNew.userName();
		saveSettings();
	} else {
		showStatusTextWithTimeout(tr("It was not possible to "
		    "connect to your databox from account \"%1\".")
		    .arg(accountInfo.accountName()));
		return false;
	}

	status = isdsLoginUserCert(isdsSessions.session(
	    accountInfo.userName()),
	    idBox, certPath, accountInfo.isTestAccount());

	isdsSessions.setSessionTimeout(accountInfo.userName(),
	    globPref.isds_download_timeout_ms); /* Set longer time-out. */

	QString isdsMsg =
	    isds_long_message(isdsSessions.session(accountInfo.userName()));

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog, isdsMsg);
}


/* ========================================================================= */
/*
* Login to ISDS server by username, password and OTP code.
*/
bool MainWindow::loginMethodUserNamePwdOtp(const QModelIndex acntTopIdx,
    const AccountModel::SettingsMap accountInfo, bool showDialog)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName(),
		    ISDS_CONNECT_TIMEOUT_MS);
	}

	QString pwd = accountInfo.password();
	if (pwd.isNull() ||
	    pwd.isEmpty()) {
		QDialog *editAccountDialog = new DlgCreateAccount(
		    *(ui->accountList), m_accountDb, acntTopIdx,
		    DlgCreateAccount::ACT_PWD, this);
		if (QDialog::Accepted == editAccountDialog->exec()) {
			const AccountModel::SettingsMap accountInfoNew =
			    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			pwd = accountInfoNew.password();
			saveSettings();
		} else {
			showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your databox from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}
	}

	QString isdsMsg;

	/* HOTP - dialog info */
	QString msgTitle = tr("Enter OTP security code");
	QString msgBody = tr("Account \"%1\" requires authentication via OTP "
		    "<br/> security code for connection to databox.")
		    .arg(accountInfo.accountName()) + "<br/><br/>" +
		    tr("Enter OTP security code for account")
		    + "<br/><b>"
		    + accountInfo.accountName()
		    + " </b>(" + accountInfo.userName() + ").";

	isds_otp_resolution otpres = OTP_RESOLUTION_SUCCESS;

	/* SMS TOTP */
	if (accountInfo.loginMethod() == LIM_TOTP) {

		/* show Premium SMS request dialog */
		QMessageBox::StandardButton reply = QMessageBox::question(this,
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
			showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your databox from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;
		}

		/* First phase - send SMS request */
		status = isdsLoginUserOtp(
		    isdsSessions.session(accountInfo.userName()),
		    accountInfo.userName(), pwd,
		    accountInfo.isTestAccount(), accountInfo.loginMethod(),
		    QString(), otpres);

		isdsSessions.setSessionTimeout(accountInfo.userName(),
		    globPref.isds_download_timeout_ms); /* Set time-out. */

		/* if SMS was not send */
		if (otpres != OTP_RESOLUTION_TOTP_SENT) {

			isdsMsg = isds_long_message(
			isdsSessions.session(accountInfo.userName()));

			msgTitle = tr("Error of sending SMS");
			msgBody = tr("It was not possible sent SMS with OTP "
			    "security code for account \"%1\"")
			    .arg(accountInfo.accountName()) + "<br><br>" +
			    "<b>" + isdsMsg + "</b>" + "<br><br>" +
			    tr("Please try again later or you have to use the "
			    "official web interface of Datové schránky for "
			    "access to your databox.");
			QMessageBox::critical(this, msgTitle, msgBody,
			    QMessageBox::Ok);

			showStatusTextWithTimeout(tr("It was not possible to "
			    "connect to your databox from account \"%1\".")
			    .arg(accountInfo.accountName()));
			return false;

		} else {
			msgTitle = tr("Enter SMS security code");
			msgBody = tr("SMS security code for account \"%1\"<br/>"
			    "has been sent on your mobile phone...")
			    .arg(accountInfo.accountName())
			     + "<br/><br/>" +
			    tr("Enter SMS security code for account")
			    + "<br/><b>"
			    + accountInfo.accountName()
			    + " </b>(" + accountInfo.userName() + ").";
		}
	}

	/* Second phase - Authentization with OTP code */
	QString otpcode;
	bool ok;
	bool repeat = false;
	int count = 0;
	do {
		count++;
		otpcode = "";
		while (otpcode.isEmpty()) {
			otpcode = QInputDialog::getText(this, msgTitle,
			    msgBody, QLineEdit::Normal, "", &ok,
			    Qt::WindowStaysOnTopHint);
			if (!ok) {
				showStatusTextWithTimeout(
				    tr("It was not possible to "
				    "connect to your databox from "
				    "account \"%1\".").
				    arg(accountInfo.accountName()));
				return false;
			}
		}

		/* sent security code */
		status = isdsLoginUserOtp(
		    isdsSessions.session(accountInfo.userName()),
		    accountInfo.userName(), pwd,
		    accountInfo.isTestAccount(), accountInfo.loginMethod(),
		    otpcode, otpres);

		isdsSessions.setSessionTimeout(accountInfo.userName(),
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
				    + " </b>(" + accountInfo.userName() + ")"
				    + "<br/><br/>" +
				    tr("Enter the correct security code again.");
				if (count > 1) {
					isdsMsg = tr("OTP: Zkontrolujte "
					"přihlašovací údaje a zadejte znova "
					"bezpečnostní kód.");
					repeat = false;
				} else {
					repeat = true;
				}
				break;
			case OTP_RESOLUTION_ACCESS_BLOCKED:
				isdsMsg = tr("OTP: Váš přístup byl na 60 minut"
				" zablokován. Důvodem může být opakované "
				"neúspěšné přihlášení.");
				repeat = false;
				break;
			case OTP_RESOLUTION_PASSWORD_EXPIRED:
				isdsMsg = tr("OTP: Platnost Vašeho hesla nebo "
				"bezpečnostního kódu skončila.");
				repeat = false;
				break;
			case OTP_RESOLUTION_UNAUTHORIZED:
				isdsMsg = tr("OTP: Pro přístup na požadovanou "
				"stránku nemá Váš účet potřebné oprávnění.");
				repeat = false;
				break;
			default:
				isdsMsg = isds_long_message(
				    isdsSessions.session(accountInfo.userName()));
				    repeat = false;
				break;
			}
		} else {
			isdsMsg = isds_long_message(
			    isdsSessions.session(accountInfo.userName()));
			repeat = false;
		}

	} while (repeat);

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog, isdsMsg);
}


/* ========================================================================= */
/*
 * Connect to databox from exist account
 */
bool MainWindow::connectToIsds(const QModelIndex acntTopIdx, bool showDialog)
/* ========================================================================= */
{

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	/* Login method based on username and password */
	if (accountInfo.loginMethod() == LIM_USERNAME) {
		return loginMethodUserNamePwd(acntTopIdx, accountInfo,
		    showDialog);

	/* Login method based on certificate only */
	} else if (accountInfo.loginMethod() == LIM_CERT) {
		return loginMethodCertificateOnly(acntTopIdx, accountInfo,
		    showDialog);

	/* Login method based on certificate together with username */
	} else if (accountInfo.loginMethod() == LIM_USER_CERT) {

		return loginMethodCertificateUserPwd(acntTopIdx, accountInfo,
		    showDialog);

		/* TODO - next method is situation when certificate will be used
		 * and password missing. The username shifts meaning to box ID.
		 * This is used for hosted services. It is not dokumented and
		 * we not support this method now.

		if (accountInfo.password().isNull() ||
		    accountInfo.password().isEmpty()) {
			return loginMethodCertificateIdBox(acntTopIdx,
			    accountInfo, showDialog);
		}
		*/

	/* Login method based username, password and OTP */
	} else {
		return loginMethodUserNamePwdOtp(acntTopIdx, accountInfo,
		    showDialog);
	}
}


/* ========================================================================= */
/*
 * First connect to databox from new account
 */
bool MainWindow::firstConnectToIsds(AccountModel::SettingsMap accountInfo,
    bool showDialog)
/* ========================================================================= */
{
	/* Login method based on username and password */
	if (accountInfo.loginMethod() == LIM_USERNAME) {
		return loginMethodUserNamePwd(QModelIndex(), accountInfo,
		    showDialog);

	/* Login method based on certificate only */
	} else if (accountInfo.loginMethod() == LIM_CERT) {
		return loginMethodCertificateOnly(QModelIndex(), accountInfo,
		    showDialog);

	/* Login method based on certificate together with username */
	} else if (accountInfo.loginMethod() == LIM_USER_CERT) {

		return loginMethodCertificateUserPwd(QModelIndex(), accountInfo,
		    showDialog);

		/* TODO - next method is situation when certificate will be used
		 * and password missing. The username shifts meaning to box ID.
		 * This is used for hosted services. It is not dokumented and
		 * we not support this method now.

		if (accountInfo.password().isNull() ||
		    accountInfo.password().isEmpty()) {
			return loginMethodCertificateIdBox(QModelIndex(),
			    accountInfo, showDialog);
		}
		*/

	/* Login method based username, password and OTP */
	} else {
		return loginMethodUserNamePwdOtp(QModelIndex(), accountInfo,
		    showDialog);
	}
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
 * Verify if is a connection to ISDS and databox exists for a new account
 */
void MainWindow::getAccountUserDataboxInfo(AccountModel::SettingsMap accountInfo)
/* ========================================================================= */
{
	debugSlotCall();

	if (!isdsSessions.isConnectedToIsds(accountInfo.userName())) {
		if (!firstConnectToIsds(accountInfo, false)) {
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
void MainWindow::msgSetSelectedMessageProcessState(int state)
/* ========================================================================= */
{
	debugSlotCall();

	MessageProcessState procSt;
	switch (state) {
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

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	QModelIndex messageIndex =
	    ui->messageList->selectionModel()->currentIndex();

	int msgId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toInt();

	messageDb->msgSetProcessState(msgId, state, false);

	DbMsgsTblModel *messageModel = (DbMsgsTblModel *)
	    m_messageListProxyModel.sourceModel();
	Q_ASSERT(0 != messageModel);
	messageModel->overrideProcessing(
	    messageIndex.sibling(messageIndex.row(), 0).data().toInt(),
	    procSt);
	/* Inform the view that the model has changed. */
	emit messageModel->dataChanged(
	    messageIndex.sibling(messageIndex.row(), 0),
	    messageIndex.sibling(messageIndex.row(),
	    messageModel->columnCount() - 1));
}


/* ========================================================================= */
/*
 * Mark selected message as read.
 */
void MainWindow::msgSetSelectedMessageRead(void)
/* ========================================================================= */
{
	qDebug() << "Timer timed out, marking message"
	    << m_lastSelectedMessageId << "as read.";

	QModelIndex current = ui->messageList->
	    selectionModel()->currentIndex();

	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(current);
	QString userName = accountUserName(accountItem);
	MessageDb *messageDb = accountMessageDb(accountItem);
	Q_ASSERT(0 != messageDb);

	messageDb->smsgdtSetLocallyRead(m_lastSelectedMessageId);
	/*
	 * Reload/update account model only for
	 * affected account.
	 */
	updateExistingAccountModelUnread(ui->accountList->
	    selectionModel()->currentIndex());
	/*
	 * Mark message as read without reloading
	 * the whole model.
	 */
	DbMsgsTblModel *messageModel = (DbMsgsTblModel *)
	    m_messageListProxyModel.sourceModel();
	Q_ASSERT(0 != messageModel);
	messageModel->overrideRead(
	    current.sibling(current.row(), 0).data().toInt(), true);
	/* Inform the view that the model has changed. */
	emit messageModel->dataChanged(
	    current.sibling(current.row(), 0),
	    current.sibling(current.row(),
	        messageModel->columnCount() - 1));
}
