/*
 * Copyright (C) 2014-2016 CZ.NIC
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
#include <QMimeDatabase>
#include <QMimeType>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QPrinter>
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
#include "src/io/filesystem.h"
#include "src/io/message_db_single.h"
#include "src/io/message_db_set_container.h"
#include "src/models/files_model.h"
#include "src/views/table_home_end_filter.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h"
#include "src/worker/task_authenticate_message.h"
#include "src/worker/task_erase_message.h"
#include "src/worker/task_download_message.h"
#include "src/worker/task_download_message_list.h"
#include "src/worker/task_download_owner_info.h"
#include "src/worker/task_download_password_info.h"
#include "src/worker/task_download_user_info.h"
#include "src/worker/task_import_zfo.h"
#include "src/worker/task_verify_message.h"
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

/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
    m_accountModel(this),
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
    ui(new Ui::MainWindow),
    mui_filterLine(0),
    mui_clearFilterLineButton(0),
    mui_statusBar(0),
    mui_statusDbMode(0),
    mui_statusOnlineLabel(0),
    mui_statusProgressBar(0)
{
	setUpUi();

	/* Single instance emitter. */
	connect(&globSingleInstanceEmitter, SIGNAL(messageReceived(QString)),
	    this, SLOT(processSingleInstanceMessages(QString)));

	/* Worker-related processing signals. */
	connect(&globMsgProcEmitter,
	    SIGNAL(downloadMessageFinished(QString, qint64, QDateTime, int,
	        QString)),
	    this,
	    SLOT(collectDownloadMessageStatus(QString, qint64, QDateTime, int,
	        QString)));
	connect(&globMsgProcEmitter,
	    SIGNAL(downloadMessageListFinished(QString, int, int, QString,
	        bool, int, int, int, int)), this,
	    SLOT(collectDownloadMessageListStatus(QString, int, int, QString,
	        bool, int, int, int, int)));
	connect(&globMsgProcEmitter,
	    SIGNAL(importZfoFinished(QString, int, QString)), this,
	    SLOT(collectImportZfoStatus(QString, int, QString)));
	connect(&globMsgProcEmitter, SIGNAL(progressChange(QString, int)),
	    this, SLOT(updateProgressBar(QString, int)));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64)));
	connect(&globWorkPool, SIGNAL(finished()),
	    this, SLOT(workersFinished()));

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
	ui->messageList->installEventFilter(new TableHomeEndFilter(this));

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
	    SLOT(attachmentItemDoubleClicked(QModelIndex)));
	ui->messageAttachmentList->installEventFilter(new TableHomeEndFilter(this));

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
	if (globPref.download_on_background) {
		if (globPref.timer_value > 4) {
			m_timeoutSyncAccounts = globPref.timer_value * 60000;
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

	if (!globPref.store_messages_on_disk) {
		msgStrg = tr("memory");
	}

	if (!globPref.store_additional_data_on_disk) {
		acntStrg = tr("memory");
	}

	mui_statusDbMode->setText(tr("Storage:") + " " + msgStrg + " | "
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
 * Show text in the status bar with timeout
 */
void MainWindow::showStatusTextWithTimeout(const QString &qStr)
/* ========================================================================= */
{
	clearStatusBar();
	mui_statusBar->showMessage((qStr), TIMER_STATUS_TIMEOUT_MS);
}


/* ========================================================================= */
/*
 * Show text in the status bar without timeout
 */
void MainWindow::showStatusTextPermanently(const QString &qStr)
/* ========================================================================= */
{
	clearStatusBar();
	mui_statusBar->showMessage((qStr), 0);
}


/* ========================================================================= */
/*
 * Slot: Clear progress bar and set default text
 */
void MainWindow::clearProgressBar(void)
/* ========================================================================= */
{
	mui_statusProgressBar->setFormat(PL_IDLE);
	mui_statusProgressBar->setValue(0);
}


/* ========================================================================= */
/*
 * Slot: Clear status bar
 */
void MainWindow::clearStatusBar(void)
/* ========================================================================= */
{
	mui_statusBar->clearMessage();
}


/* ========================================================================= */
/*
 * Slot: Update ProgressBar text and value.
 */
void MainWindow::updateProgressBar(const QString &label, int value)
 /* ========================================================================= */
{
	mui_statusProgressBar->setFormat(label);
	mui_statusProgressBar->setValue(value);
	mui_statusProgressBar->repaint();
}


/* ========================================================================= */
/*
 * Slot: Update StatusBar text.
 */
void MainWindow::updateStatusBarText(const QString &text)
/* ========================================================================= */
{
	//debugSlotCall();
	showStatusTextWithTimeout(text);
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
	QAbstractTableModel *msgTblMdl = 0;

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
		ui->messageList->setModel(0);
		ui->messageStackedWidget->setCurrentIndex(0);
		ui->accountTextInfo->setHtml(createDatovkaBanner(
		    QCoreApplication::applicationVersion()));
		ui->accountTextInfo->setReadOnly(true);
		return;
	}

	const QString userName(m_accountModel.userName(current));
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	if (0 == dbSet) {
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
		    AccountModel::globAccounts[userName]);

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
		setMessageActionVisibility(0);
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
		msgTblMdl = dbSet->msgsRcvdModel();
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
		msgTblMdl = dbSet->msgsSntModel();
		ui->actionDelete_message_from_db->setEnabled(true);
#endif /* DISABLE_ALL_TABLE */
		break;
	case AccountModel::nodeReceivedYear:
		/* TODO -- Parameter check. */
		msgTblMdl = dbSet->msgsRcvdInYearModel(
		    current.data(ROLE_PLAIN_DISPLAY).toString());
		ui->actionDelete_message_from_db->setEnabled(true);
		connect(ui->messageList, SIGNAL(clicked(QModelIndex)),
		    this, SLOT(messageItemClicked(QModelIndex)));
		break;
	case AccountModel::nodeSentYear:
		/* TODO -- Parameter check. */
		msgTblMdl = dbSet->msgsSntInYearModel(
		    current.data(ROLE_PLAIN_DISPLAY).toString());
		ui->actionDelete_message_from_db->setEnabled(true);
		break;
	default:
		Q_ASSERT(0);
		break;
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
		/* Select last message in list if there are some messages. */
		itemModel = ui->messageList->model();
		/* enable/disable buttons */
		if ((0 != itemModel) && (0 < itemModel->rowCount())) {
			messageItemRestoreSelectionOnModelChange();
			ui->actionAuthenticate_message_file->setEnabled(true);
			ui->actionExport_correspondence_overview->
			    setEnabled(true);
			ui->actionCheck_message_timestamp_expiration->
			    setEnabled(true);
		} else {
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

	if (acntIdx.isValid()) {
		bool received = AccountModel::nodeTypeIsReceived(acntIdx);

		menu->addAction(ui->actionGet_messages);
		menu->addAction(ui->actionSend_message);
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

	if (1 == firstColumnIdxs.size()) {
		const QModelIndex &index = firstColumnIdxs.first();

		MessageDbSet *dbSet = accountDbSet(
		    m_accountModel.userName(currentAccountModelIndex()), this);
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
			if (globPref.message_mark_as_read_timeout >= 0) {
				qDebug() << "Starting timer to mark as "
				    "read for message" << msgId;
				m_messageMarker.setSingleShot(true);
				m_messageMarker.start(
				    globPref.message_mark_as_read_timeout);
			}
		} else {
			m_messageMarker.stop();
		}

		/* Generate and show message information. */
		ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId, 0));
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

		/* Show files related to message message. */
		QAbstractTableModel *fileTblMdl = messageDb->flsModel(msgId);
		Q_ASSERT(0 != fileTblMdl);
		//qDebug() << "Setting files";
		ui->messageAttachmentList->setModel(fileTblMdl);
		/* First three columns contain hidden data. */
		ui->messageAttachmentList->setColumnHidden(
		    DbFlsTblModel::ATTACHID_COL, true);
		ui->messageAttachmentList->setColumnHidden(
		    DbFlsTblModel::MSGID_COL, true);
		ui->messageAttachmentList->setColumnHidden(
		    DbFlsTblModel::CONTENT_COL, true);

		if (ui->messageAttachmentList->model()->rowCount() > 0) {
			ui->actionSave_all_attachments->setEnabled(true);
		}

		ui->messageAttachmentList->resizeColumnToContents(
		    DbFlsTblModel::FNAME_COL);

		/* Connect new slot. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(attachmentItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
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

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()), this);
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
	DbMsgsTblModel *messageModel = dynamic_cast<DbMsgsTblModel *>(
	    m_messageListProxyModel.sourceModel());
	Q_ASSERT(0 != messageModel);

	messageModel->overrideRead(msgId, !isRead);

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
	}
	menu->addAction(ui->actionExport_as_ZFO);
	menu->addAction(ui->actionExport_delivery_info_as_ZFO);
	menu->addAction(ui->actionExport_delivery_info_as_PDF);
	menu->addAction(ui->actionExport_message_envelope_as_PDF);
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
	menu->addAction(ui->actionDelete_message_from_db);

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
	QModelIndex acntIdx(currentAccountModelIndex());
	m_lastSelectedAccountNodeType = AccountModel::nodeType(acntIdx);
	if (AccountModel::nodeRecentReceived == m_lastSelectedAccountNodeType) {

		qDebug() << "Storing recent received selection into the model"
		    << msgId;

		const QString userName(m_accountModel.userName(acntIdx));
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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	AcntSettings &accountInfo(AccountModel::globAccounts[userName]);
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
				const QString userName(
				    m_accountModel.userName(acntIdx));
				Q_ASSERT(!userName.isEmpty());

				msgLastId = AccountModel::globAccounts[userName]
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
	while (yearIdx.isValid() && (yearIdx.data().toString() != year)) {
		yearIdx = yearIdx.sibling(++childRow, 0);
	}

	if (!yearIdx.isValid()) {
		return QModelIndex();
	}

	return yearIdx;
}


/* ========================================================================= */
/*
 * Return index for message with given properties.
 */
QModelIndex MainWindow::messageIndex(qint64 msgId) const
/* ========================================================================= */
{
	debugFuncCall();

	const QAbstractItemModel *model = ui->messageList->model();
	Q_ASSERT(0 != model);

	int rowCount = model->rowCount();

	if (0 == rowCount) {
		/* Do nothing on empty model. */
		return QModelIndex();
	}

	/* Find and select the message with the ID. */
	for (int row = 0; row < rowCount; ++row) {
		/*
		 * TODO -- Search in a more resource-saving way.
		 * Eliminate index copying, use smarter search.
		 */
		if (model->index(row, 0).data().toLongLong() == msgId) {
			return model->index(row, 0);
		}
	}

	return QModelIndex();
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
	QModelIndex msgIdx(messageIndex(msgId));

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

	/* Unused. */
	(void) selected;
	(void) deselected;

	QModelIndexList selectedIndexes;
	{
		QItemSelectionModel *selectionModel =
		    ui->messageAttachmentList->selectionModel();
		if (0 == selectionModel) {
			Q_ASSERT(0);
			return;
		}
		selectedIndexes = selectionModel->selectedRows(0);
	}

	setAttachmentActionVisibility(selectedIndexes.size());
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

	QModelIndexList selectedIndexes;
	{
		QItemSelectionModel *selectionModel =
		    ui->messageAttachmentList->selectionModel();
		if (0 == selectionModel) {
			Q_ASSERT(0);
			return;
		}
		selectedIndexes = selectionModel->selectedRows(0);
	}

	Q_ASSERT(selectedIndexes.size() > 0);

	QMenu *menu = new QMenu;

	if (selectedIndexes.size() == 1) {
		menu->addAction(ui->actionOpen_attachment);
	}
	menu->addAction(ui->actionSave_selected_attachments);
	menu->addSeparator();
	menu->addAction(ui->actionEmail_selected_attachments);

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
	openSelectedAttachment();
}


/* ========================================================================= */
/*
 * Saves selected attachment to file.
 */
void MainWindow::saveSelectedAttachmentsToFile(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList attachmentIndexes;
	{
		QItemSelectionModel *selectionModel =
		    ui->messageAttachmentList->selectionModel();
		if (0 == selectionModel) {
			Q_ASSERT(0);
			return;
		}
		attachmentIndexes = selectionModel->selectedRows(0);
	}

	QModelIndex messageIndex(currentSingleMessageIndex());
	if (!messageIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	foreach (const QModelIndex &attachmentIndex, attachmentIndexes) {
		saveAttachmentToFile(messageIndex, attachmentIndex);
	}
}

/* ========================================================================= */
void MainWindow::saveAttachmentToFile(const QModelIndex &messageIndex,
   const QModelIndex &attachmentIndex)
/* ========================================================================= */
{
	if (!attachmentIndex.isValid()) {
		Q_ASSERT(0);
		showStatusTextWithTimeout(tr("Saving attachment of message to "
		    "files was not successful!"));
		return;
	}

	qint64 dmId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toLongLong();

	QModelIndex fileNameIndex = attachmentIndex.sibling(
	    attachmentIndex.row(), DbFlsTblModel::FNAME_COL);
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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	QDateTime deliveryTime = msgDeliveryTime(messageIndex);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	fileName = fileNameFromFormat(globPref.attachment_filename_format,
	    dmId, dbId, userName, fileName, entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

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
	QModelIndex dataIndex = attachmentIndex.sibling(attachmentIndex.row(),
	    DbFlsTblModel::CONTENT_COL);
	if (!dataIndex.isValid()) {
		Q_ASSERT(0);
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

	QModelIndex messageIndex(currentSingleMessageIndex());

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

	if (newDir.isEmpty()) {
		return;
	}

	if (!globPref.use_global_paths) {
		m_save_attach_dir = newDir;
		storeExportPath();
	}

	bool unspecifiedFailed = false;
	QList<QString> unsuccessfullFiles;

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	QDateTime deliveryTime = msgDeliveryTime(messageIndex);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QList<MessageDb::FileData> attachList =
	    messageDb->getFilesFromMessage(dmId);
	if (attachList.isEmpty()) {

		if (!messageMissingOfferDownload(dmId, deliveryTime,
		        tr("Message export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			logErrorNL("Could not access database of "
			    "freshly downloaded message '%d'.", dmId);
			return;
		}

		attachList = messageDb->getFilesFromMessage(dmId);
		if (attachList.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	foreach (const MessageDb::FileData &attach, attachList) {
		QString fileName(attach.dmFileDescr);
		if (fileName.isEmpty()) {
			unspecifiedFailed = true;
			Q_ASSERT(0);
			continue;
		}

		fileName = fileNameFromFormat(
		    globPref.attachment_filename_format,
		    dmId, dbId, userName, fileName, entry.dmDeliveryTime,
		    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

		fileName = newDir + QDir::separator() + fileName;

		QByteArray data(
		    QByteArray::fromBase64(attach.dmEncodedContent));

		if (WF_SUCCESS !=
		    writeFile(nonconflictingFileName(fileName), data)) {
			unsuccessfullFiles.append(fileName);
			continue;
		}

		if (globPref.delivery_info_for_every_file) {
			if (globPref.all_attachments_save_zfo_delinfo) {
				exportDeliveryInfoAsZFO(newDir, attach.dmFileDescr,
				    globPref.delivery_filename_format_all_attach,
				    userName, dmId, deliveryTime, false);
			}
			if (globPref.all_attachments_save_pdf_delinfo) {
				exportDeliveryInfoAsPDF(newDir, attach.dmFileDescr,
				  globPref.delivery_filename_format_all_attach,
				  userName, dmId, deliveryTime, false);
			}
		}
	}

	if (globPref.all_attachments_save_zfo_msg) {
		exportMessageAsZFO(newDir, userName, dmId, deliveryTime, false);
	}

	if (globPref.all_attachments_save_pdf_msgenvel) {
		exportMessageEnvelopeAsPDF(newDir, userName, dmId, deliveryTime,
		    false);
	}

	if (!globPref.delivery_info_for_every_file) {
		if (globPref.all_attachments_save_zfo_delinfo) {
			exportDeliveryInfoAsZFO(newDir, "",
			    globPref.delivery_filename_format,
			    userName, dmId, deliveryTime, false);
		}
		if (globPref.all_attachments_save_pdf_delinfo) {
			exportDeliveryInfoAsPDF(newDir, "",
			    globPref.delivery_filename_format,
			    userName, dmId, deliveryTime, false);
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

	QModelIndex selectedIndex;

	{
		QModelIndexList attachmentIndexes;

		QItemSelectionModel *selectionModel =
		    ui->messageAttachmentList->selectionModel();
		if (0 == selectionModel) {
			Q_ASSERT(0);
			return;
		}
		attachmentIndexes = selectionModel->selectedRows(0);

		if (attachmentIndexes.size() != 1) {
			Q_ASSERT(0);
			return;
		}

		selectedIndex = attachmentIndexes[0];
	}

	if (!selectedIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	QModelIndex fileNameIndex = selectedIndex.sibling(selectedIndex.row(),
	    DbFlsTblModel::FNAME_COL);
	Q_ASSERT(fileNameIndex.isValid());
	if(!fileNameIndex.isValid()) {
		return;
	}
	QString attachName = fileNameIndex.data().toString();
	Q_ASSERT(!attachName.isEmpty());
	if (attachName.isEmpty()) {
		return;
	}
	attachName.replace(QRegExp("\\s"), "_").replace(
	    QRegExp("[^a-zA-Z\\d\\.\\-_]"), "x");
	/* TODO -- Add message id into file name? */
	QString fileName = TMP_ATTACHMENT_PREFIX + attachName;

	/* Get data from base64. */
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(),
	    DbFlsTblModel::CONTENT_COL);
	if (!dataIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}

	QByteArray data =
	    QByteArray::fromBase64(dataIndex.data().toByteArray());

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		showStatusTextWithTimeout(tr("Attachment '%1' stored to "
		    "temporary file '%2'.").arg(attachName).arg(fileName));
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
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

void MainWindow::processSingleInstanceMessages(const QString &message)
{
	debugSlotCall();

	logDebugLv0NL("Received message '%s'.", message.toUtf8().constData());

	if (SingleInstance::msgRaiseMainWindow == message) {
		this->show();
		this->raise();
		this->activateWindow();
	}
}

/* ========================================================================= */
/*
 * Workers finished.
 */
void MainWindow::workersFinished(void)
/* ========================================================================= */
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

	if (globPref.download_on_background) {
		m_timerSyncAccounts.start(m_timeoutSyncAccounts);
	}
}

void MainWindow::collectDownloadMessageStatus(const QString &usrName,
    qint64 msgId, const QDateTime &deliveryTime, int result,
    const QString &errDesc)
{
	debugSlotCall();

	/* Unused. */
	(void) deliveryTime;

	if (TaskDownloadMessage::DM_SUCCESS == result) {
		/* Refresh account and attachment list. */
		refreshAccountList(usrName);

		if (0 <= msgId) {
			postDownloadSelectedMessageAttachments(usrName, msgId);
		}
	} else {
		/* Notify the user. */
		QMessageBox msgBox(this);

		showStatusTextWithTimeout(tr("It was not possible download "
		    "complete message \"%1\" from ISDS server.").arg(msgId));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Download message error"));
		msgBox.setText(tr("It was not possible to download a complete "
		    "message \"%1\" from server Datové schránky.").arg(msgId));
		if (!errDesc.isEmpty()) {
			msgBox.setInformativeText(tr("ISDS: ") + errDesc);
		} else {
			msgBox.setInformativeText(tr("A connection error "
			    "occurred or the message has already been deleted "
			    "from the server."));
		}

		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
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
		QMessageBox msgBox(this);

		QString errorMessage = (MSG_RECEIVED == direction) ?
		    tr("It was not possible download received message list from"
		        " ISDS server.") :
		    tr("It was not possible download sent message list from"
		        " ISDS server.");

		showStatusTextWithTimeout(errorMessage);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Download message list error"));
		msgBox.setText(errorMessage);
		if (!errDesc.isEmpty()) {
			msgBox.setInformativeText(tr("ISDS: ") + errDesc);
		} else {
			msgBox.setInformativeText(
			    tr("A connection error occurred."));
		}

		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
	}
}

void MainWindow::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId)
{
	debugSlotCall();

	/* Unused. */
	(void) userName;
	(void) transactId;
	(void) resultDesc;
	(void) isPDZ;
	(void) dmId;

	if (TaskSendMessage::SM_SUCCESS == result) {
		showStatusTextWithTimeout(tr(
		    "Message from '%1' (%2) has been successfully sent to '%3' (%4).").
		    arg(AccountModel::globAccounts[userName].accountName()).
		    arg(userName).arg(recipientName).arg(dbIDRecipient));

		/* Refresh account list. */
		refreshAccountList(userName);
	} else {
		showStatusTextWithTimeout(tr(
		    "Error while sending message from '%1' (%2) to '%3' (%4).").
		    arg(AccountModel::globAccounts[userName].accountName()).
		    arg(userName).arg(recipientName).arg(dbIDRecipient));
	}

	clearProgressBar();
}

/* ========================================================================= */
/*
 * Set tablewidget when message download worker is done.
 */
void MainWindow::postDownloadSelectedMessageAttachments(
    const QString &userName, qint64 dmId)
/* ========================================================================= */
{
	debugFuncCall();

	showStatusTextWithTimeout(tr("Message \"%1\" "
	    " was downloaded from ISDS server.").arg(dmId));

	const QString currentUserName(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (currentUserName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	/* Do nothing if account was changed. */
	if (userName != currentUserName) {
		return;
	}

	DbMsgsTblModel *messageModel = dynamic_cast<DbMsgsTblModel *>(
	    m_messageListProxyModel.sourceModel());
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
	ui->messageList->selectionModel()->select(storedMsgSelection,
	    QItemSelectionModel::ClearAndSelect);

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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
		    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		    this,
		    SLOT(attachmentItemsSelectionChanged(QItemSelection,
		        QItemSelection)));
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()), this);
	Q_ASSERT(0 != dbSet);
	QDateTime deliveryTime = msgDeliveryTime(msgIdIdx);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(dmId, 0));
	ui->messageInfo->setReadOnly(true);

	QAbstractTableModel *fileTblMdl = messageDb->flsModel(dmId);
	Q_ASSERT(0 != fileTblMdl);
	ui->messageAttachmentList->setModel(fileTblMdl);
	/* First three columns contain hidden data. */
	ui->messageAttachmentList->setColumnHidden(
	    DbFlsTblModel::ATTACHID_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    DbFlsTblModel::MSGID_COL, true);
	ui->messageAttachmentList->setColumnHidden(
	    DbFlsTblModel::CONTENT_COL, true);

	if (ui->messageAttachmentList->model()->rowCount() > 0) {
		ui->actionSave_all_attachments->setEnabled(true);
	} else {
		ui->actionSave_all_attachments->setEnabled(false);
	}

	ui->messageAttachmentList->resizeColumnToContents(
	    DbFlsTblModel::FNAME_COL);

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

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx),
	    this);
	Q_ASSERT(0 != dbSet);

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

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx),
	    this);
	Q_ASSERT(0 != dbSet);
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

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx),
	    this);
	Q_ASSERT(0 != dbSet);

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

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx),
	    this);
	Q_ASSERT(0 != dbSet);

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

	QModelIndex acntIdx(currentAccountModelIndex());

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx),
	    this);
	Q_ASSERT(0 != dbSet);
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

	MessageDbSet *dbSet = accountDbSet(m_accountModel.userName(acntIdx),
	    this);
	Q_ASSERT(0 != dbSet);

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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	if (userName.isEmpty()) {
		Q_ASSERT(0);
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
	QModelIndex selectedAcntIndex(currentAccountModelIndex());

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

	bool incoming = true;
	{
		QModelIndex acntIdx(currentAccountModelIndex());

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
	}

	if (delFromIsds) {
		if (!isdsSessions.isConnectedToIsds(userName)) {
			if (!connectToIsds(userName, this)) {
				return Q_ISDS_ERROR;
			}
		}
	}

	QString errorStr, longErrorStr;
	TaskEraseMessage *task;

	task = new (std::nothrow) TaskEraseMessage(userName, dbSet, dmId,
	    deliveryTime, incoming, delFromIsds);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	TaskEraseMessage::Result result = task->m_result;
	errorStr = task->m_isdsError;
	longErrorStr = task->m_isdsLongError;
	delete task;

	switch (result) {
	case TaskEraseMessage::NOT_DELETED:
		showStatusTextWithTimeout(
		    tr("Message \"%1\" was not deleted.").arg(dmId));
		return Q_ISDS_ERROR;
		break;
	case TaskEraseMessage::DELETED_ISDS:
		showStatusTextWithTimeout(tr(
		    "Message \"%1\" was deleted only from ISDS.").arg(dmId));
		return Q_SQL_ERROR;
		break;
	case TaskEraseMessage::DELETED_LOCAL:
		if (delFromIsds) {
			showStatusTextWithTimeout(tr(
			    "Message \"%1\" was deleted only from local database.")
			    .arg(dmId));
			return Q_ISDS_ERROR;
		} else {
			showStatusTextWithTimeout(tr(
			    "Message \"%1\" was deleted from local database.")
			    .arg(dmId));
			return Q_SUCCESS;
		}
		break;
	case TaskEraseMessage::DELETED_ISDS_LOCAL:
		showStatusTextWithTimeout(tr(
		    "Message \"%1\" was deleted from ISDS and local database.")
		    .arg(dmId));
		return Q_SUCCESS;
		break;
	default:
		Q_ASSERT(0);
		break;
	}

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

		const QString userName(m_accountModel.userName(index));
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

			TaskDownloadMessageList *task;

			task = new (std::nothrow) TaskDownloadMessageList(
			    userName, dbSet, MSG_RECEIVED,
			    globPref.auto_download_whole_messages);
			task->setAutoDelete(true);
			globWorkPool.assignLo(task);

			task = new (std::nothrow) TaskDownloadMessageList(
			    userName, dbSet, MSG_SENT,
			    globPref.auto_download_whole_messages);
			task->setAutoDelete(true);
			globWorkPool.assignLo(task);

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

	if (globWorkPool.working()) {
		ui->actionSync_all_accounts->setEnabled(false);
		ui->actionGet_messages->setEnabled(false);
	}
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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
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

	TaskDownloadMessageList *task;

	task = new (std::nothrow) TaskDownloadMessageList(userName, dbSet,
	    MSG_RECEIVED, globPref.auto_download_whole_messages);
	task->setAutoDelete(true);
	globWorkPool.assignLo(task);

	task = new (std::nothrow) TaskDownloadMessageList(userName, dbSet,
	    MSG_SENT, globPref.auto_download_whole_messages);
	task->setAutoDelete(true);
	globWorkPool.assignLo(task);

	if (globWorkPool.working()) {
		ui->actionSync_all_accounts->setEnabled(false);
		ui->actionGet_messages->setEnabled(false);
	}
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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

	if (firstMsgColumnIdxs.isEmpty()) {
		return;
	}

	QList<MessageDb::MsgId> msgIds;
	foreach (const QModelIndex &idx, firstMsgColumnIdxs) {
		msgIds.append(MessageDb::MsgId(idx.data().toLongLong(),
		    msgDeliveryTime(idx)));
	}

	QString userName;
	{
		QModelIndex accountIndex(currentAccountModelIndex());
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

		userName = m_accountModel.userName(accountIndex);
	}

	/* Try connecting to ISDS, just to generate log-in dialogue. */
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
		TaskDownloadMessage *task;

		task = new (std::nothrow) TaskDownloadMessage(
		    userName, dbSet, msgDirection, id.dmId, id.deliveryTime);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task, WorkerPool::PREPEND);
	}

	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);
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
 * Get message db set related to given account.
 */
MessageDbSet * MainWindow::accountDbSet(const QString &userName,
    MainWindow *mw)
/* ========================================================================= */
{
	MessageDbSet *dbSet = NULL;
	int flags, dbPresenceCode;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return 0;
	}

	/* Get user name and db location. */
	AcntSettings &itemSettings(AccountModel::globAccounts[userName]);

	if (!itemSettings.isValid()) {
		logWarning(
		    "Attempting to accessing database for user name '%s'. "
		    "The account seems not to exist.\n",
		    userName.toUtf8().constData());
		return 0;
	}

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
			    MessageDbSet::DO_UNKNOWN,
			    MessageDbSet::CM_MUST_EXIST);
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
			    MessageDbSet::DO_YEARLY,
			    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
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

	const AcntSettings &itemSettings(AccountModel::globAccounts[userName]);

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

	QString userName = settings.value("default_account/username", "")
	   .toString();
	if (!userName.isEmpty()) {
		QModelIndex acntTopIdx = m_accountModel.topAcntIndex(userName);
		if (acntTopIdx.isValid()) {
			ui->accountList->setCurrentIndex(
			    acntTopIdx.child(0, 0));
			accountItemCurrentChanged(acntTopIdx.child(0, 0));
			ui->menuDatabox->setEnabled(true);
			ui->actionDelete_account->setEnabled(true);
			ui->actionSync_all_accounts->setEnabled(true);
			ui->actionFind_databox->setEnabled(true);
			ui->actionMsgAdvancedSearch->setEnabled(true);
			ui->actionImport_ZFO_file_into_database->
			    setEnabled(true);
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
	    this, SLOT(addNewAccount()));
	connect(ui->actionDelete_account, SIGNAL(triggered()),
	    this, SLOT(deleteSelectedAccount()));
	    /* Separator. */
	connect(ui->actionImport_database_directory, SIGNAL(triggered()),
	    this, SLOT(showImportDatabaseDialog()));
	    /* Separator. */
	connect(ui->actionProxy_settings, SIGNAL(triggered()),
	    this, SLOT(proxySettings()));
	   /* Separator. */
	connect(ui->actionPreferences, SIGNAL(triggered()),
	    this, SLOT(applicationPreferences()));
	/* actionQuit -- connected in ui file. */

	/* Data box menu. */
	connect(ui->actionGet_messages, SIGNAL(triggered()),
	    this, SLOT(synchroniseSelectedAccount()));
	connect(ui->actionSend_message, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessage()));
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
	connect(ui->actionSplit_database_by_years, SIGNAL(triggered()),
	    this, SLOT(splitMsgDbByYearsSlot()));

	/* Message menu. */
	connect(ui->actionDownload_message_signed, SIGNAL(triggered()),
	    this, SLOT(downloadSelectedMessageAttachments()));
	connect(ui->actionReply, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessageReply()));
	connect(ui->actionCreate_message_from_template, SIGNAL(triggered()),
	    this, SLOT(createAndSendMessageFromTmpl()));
	    /* Separator. */
	connect(ui->actionSignature_detail, SIGNAL(triggered()),
	    this, SLOT(showSignatureDetails()));
	connect(ui->actionAuthenticate_message, SIGNAL(triggered()),
	    this, SLOT(verifySelectedMessage()));
	    /* Separator. */
	connect(ui->actionOpen_message_externally, SIGNAL(triggered()),
	    this, SLOT(openSelectedMessageExternally()));
	connect(ui->actionOpen_delivery_info_externally, SIGNAL(triggered()),
	    this, SLOT(openDeliveryInfoExternally()));
	    /* Separator. */
	connect(ui->actionExport_as_ZFO, SIGNAL(triggered()),
	    this, SLOT(exportSelectedMessagesAsZFO()));
	connect(ui->actionExport_delivery_info_as_ZFO, SIGNAL(triggered()),
	    this, SLOT(exportSelectedDeliveryInfosAsZFO()));
	connect(ui->actionExport_delivery_info_as_PDF, SIGNAL(triggered()),
	    this, SLOT(exportSelectedDeliveryInfosAsPDF()));
	connect(ui->actionExport_message_envelope_as_PDF, SIGNAL(triggered()),
	    this, SLOT(exportSelectedMessageEnvelopesAsPDF()));
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
	connect(ui->actionView_message_from_ZPO_file, SIGNAL(triggered()),
	    this, SLOT(viewMessageFromZFO()));
	connect(ui->actionExport_correspondence_overview, SIGNAL(triggered()),
	    this, SLOT(exportCorrespondenceOverview()));
	connect(ui->actionCheck_message_timestamp_expiration, SIGNAL(triggered()),
	    this, SLOT(showMsgTmstmpExpirDialog()));
	    /* Separator. */
	connect(ui->actionMsgAdvancedSearch, SIGNAL(triggered()),
	    this, SLOT(showMsgAdvancedSearchDlg()));

	/* Help. */
	connect(ui->actionAbout_Datovka, SIGNAL(triggered()),
	    this, SLOT(aboutApplication()));
	connect(ui->actionHomepage, SIGNAL(triggered()),
	    this, SLOT(goHome()));
	connect(ui->actionHelp, SIGNAL(triggered()),
	    this, SLOT(showHelp()));

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
	connect(ui->messageStateCombo, SIGNAL(currentIndexChanged(int)),
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
 * Enables menu actions according to message selection.
 */
void MainWindow::setMessageActionVisibility(int numSelected) const
/* ========================================================================= */
{
	/* Top menu + menu items. */
	ui->menuMessage->setEnabled(numSelected > 0);

	ui->actionDownload_message_signed->setEnabled(numSelected > 0);
	ui->actionReply->setEnabled(numSelected == 1);
	ui->actionCreate_message_from_template->setEnabled(numSelected == 1);
	    /* Separator. */
	ui->actionSignature_detail->setEnabled(numSelected == 1);
	ui->actionAuthenticate_message->setEnabled(numSelected == 1);
	    /* Separator. */
	ui->actionOpen_message_externally->setEnabled(numSelected == 1);
	ui->actionOpen_delivery_info_externally->setEnabled(numSelected == 1);
	    /* Separator. */
	ui->actionExport_as_ZFO->setEnabled(numSelected > 0);
	ui->actionExport_delivery_info_as_ZFO->setEnabled(numSelected > 0);
	ui->actionExport_delivery_info_as_PDF->setEnabled(numSelected > 0);
	ui->actionExport_message_envelope_as_PDF->setEnabled(numSelected > 0);
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

/* ========================================================================= */
/*
 *  Active/Inactive account menu and buttons in the mainwindow.
 */
void MainWindow::activeAccountMenuAndButtons(bool action) const
/* ========================================================================= */
{
	ui->menuDatabox->setEnabled(action);
	ui->actionAccount_properties->setEnabled(action);
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
	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	/* Received. */
	unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_RECEIVED);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Received" << yearList.value(j);
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_RECEIVED,
		    yearList.value(j));
		m_accountModel.updateYear(userName,
		    AccountModel::nodeReceivedYear, yearList.value(j),
		    unreadMsgs);
	}
	/* Sent. */
	//unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_SENT);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentSent, 0);
	yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
	for (int j = 0; j < yearList.size(); ++j) {
		//qDebug() << "Sent" << yearList.value(j);
		//unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_SENT,
		//    yearList.value(j));
		m_accountModel.updateYear(userName, AccountModel::nodeSentYear,
		    yearList.value(j), 0);
	}
	return true;
}


/* ========================================================================= */
/*
 * Partially regenerates account model according to the database
 *     content.
 */
bool MainWindow::regenerateAccountModelYears(const QModelIndex &index)
/* ========================================================================= */
{
	debugFuncCall();

	QList<QString> yearList;
	int unreadMsgs;

	Q_ASSERT(index.isValid());

	/* Get database id. */
	const QString userName(m_accountModel.userName(index));
	Q_ASSERT(!userName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	/* Received. */
	unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_RECEIVED);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED, DESCENDING);
	QList< QPair<QString, unsigned> > yearlyUnreadList;
	foreach (const QString &year, yearList) {
		unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_RECEIVED,
		    year);
		yearlyUnreadList.append(QPair<QString, unsigned>(year, unreadMsgs));
	}
	m_accountModel.updateYearNodes(userName, AccountModel::nodeReceivedYear,
	    yearlyUnreadList);
	/* Sent. */
	//unreadMsgs = dbSet->msgsUnreadWithin90Days(MessageDb::TYPE_SENT);
	m_accountModel.updateRecentUnread(userName,
	    AccountModel::nodeRecentSent, 0);
	yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
	yearlyUnreadList.clear();
	foreach (const QString &year, yearList) {
		//unreadMsgs = dbSet->msgsUnreadInYear(MessageDb::TYPE_SENT,
		//    year);
		yearlyUnreadList.append(QPair<QString, unsigned>(year, 0));
	}
	m_accountModel.updateYearNodes(userName, AccountModel::nodeSentYear,
	    yearlyUnreadList);
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

	QModelIndex topIndex;
	QList<QString> yearList;
	int unreadMsgs;

	m_accountModel.removeAllYearNodes();

	//qDebug() << "Generating years";

	for (int i = 0; i < m_accountModel.rowCount(); ++i) {
		/* Get database ID. */
		topIndex = m_accountModel.index(i, 0);
		Q_ASSERT(topIndex.isValid());
		const QString userName(m_accountModel.userName(topIndex));
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
		m_accountModel.updateRecentUnread(userName,
		    AccountModel::nodeRecentReceived, unreadMsgs);
		yearList = dbSet->msgsYears(MessageDb::TYPE_RECEIVED,
		    DESCENDING);
		foreach (const QString &year, yearList) {
			unreadMsgs = dbSet->msgsUnreadInYear(
			    MessageDb::TYPE_RECEIVED, year);
			m_accountModel.appendYear(userName,
			    AccountModel::nodeReceivedYear, year, unreadMsgs);
		}
		/* Sent. */
		//unreadMsgs = dbSet->msgsUnreadWithin90Days(
		//    MessageDb::TYPE_SENT);
		m_accountModel.updateRecentUnread(userName,
		    AccountModel::nodeRecentSent, 0);
		yearList = dbSet->msgsYears(MessageDb::TYPE_SENT, DESCENDING);
		foreach (const QString &year, yearList) {
			//unreadMsgs = dbSet->msgsUnreadInYear(
			//    MessageDb::TYPE_SENT, year);
			m_accountModel.appendYear(userName,
			    AccountModel::nodeSentYear, year, 0);
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
	confFileRemovePwdQuotes(globPref.saveConfPath());
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

	QList<Task::AccountDescr> messageDbList;
	qint64 msgId = -1;
	QDateTime deliveryTime;

	/* get username of selected account */
	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	/* if not reply, get pointers to database for other accounts */
	if (DlgSendMessage::ACT_REPLY != action) {
		for (int i=0; i < ui->accountList->model()->rowCount(); i++) {
			QModelIndex index = m_accountModel.index(i, 0);
			const QString uName(m_accountModel.userName(index));
			Q_ASSERT(!uName.isEmpty());
			MessageDbSet *dbSet = accountDbSet(uName, this);
			Q_ASSERT(0 != dbSet);
			messageDbList.append(Task::AccountDescr(uName, dbSet));
		}
	} else {
		MessageDbSet *dbSet = accountDbSet(userName, this);
		Q_ASSERT(0 != dbSet);
		messageDbList.append(Task::AccountDescr(userName, dbSet));
	}

	/* if is reply or template, ID of selected message is required */
	if (DlgSendMessage::ACT_REPLY == action ||
	    DlgSendMessage::ACT_NEW_FROM_TMP == action) {
		const QAbstractItemModel *tableModel =
		    ui->messageList->model();
		Q_ASSERT(0 != tableModel);
		QModelIndex index = tableModel->index(
		    currentSingleMessageIndex().row(), 0);
		msgId = tableModel->itemData(index).first().toLongLong();
		deliveryTime = msgDeliveryTime(index);
	}

	QDialog *sendMsgDialog = new DlgSendMessage(messageDbList,
	    (DlgSendMessage::Action) action, msgId, deliveryTime,
	    userName, this);

	showStatusTextWithTimeout(tr("Create and send a message."));

	connect(sendMsgDialog,
	    SIGNAL(doActionAfterSentMsgSignal(const QString, const QString)),
	    this, SLOT(doActionAfterSentMsgSlot(const QString, const QString)));

	sendMsgDialog->show();

}


/* ========================================================================= */
/*
 * Slot: Store last add attachment path.
 */
void MainWindow::doActionAfterSentMsgSlot(const QString &userName,
    const QString &lastDir)
/* ========================================================================= */
{
	debugSlotCall();

	/* Unused. */
	(void) userName;

	if (!globPref.use_global_paths) {
		m_add_attach_dir = lastDir;
		storeExportPath();
	}
}


/* ========================================================================= */
/*
 * Add account action and dialog.
 */
void MainWindow::addNewAccount(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *newAccountDialog = new DlgCreateAccount(AcntSettings(),
	    DlgCreateAccount::ACT_ADDNEW, this);

	connect(newAccountDialog,
	    SIGNAL(getAccountUserDataboxInfo(AcntSettings)),
	    this, SLOT(getAccountUserDataboxInfo(AcntSettings)));

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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	const QString accountName(
	    AccountModel::globAccounts[userName].accountName());

	QString dlgTitleText = tr("Remove account ") + accountName;
	QString questionText = tr("Do you want to remove account") + " '" +
	    accountName + "' (" + userName + ")?";
	QString checkBoxText = tr("Delete also message database from storage");
	QString detailText = tr(
	    "Warning: If you delete the message database then all locally "
	    "accessible messages that are not stored on the ISDS server "
	    "will be lost.");

	QDialog *yesNoCheckDlg = new YesNoCheckboxDialog(dlgTitleText,
	    questionText, checkBoxText, detailText, this);
	int retVal = yesNoCheckDlg->exec();
	yesNoCheckDlg->deleteLater();

	if ((YesNoCheckboxDialog::YesChecked == retVal) ||
	    (YesNoCheckboxDialog::YesUnchecked == retVal)) {
		/* Delete account from model. */
		m_accountModel.deleteAccount(userName);
		globAccountDbPtr->deleteAccountInfo(userName + "___True");
	}

	if (YesNoCheckboxDialog::YesChecked == retVal) {
		if (globMessageDbsPtr->deleteDbSet(dbSet)) {
			showStatusTextWithTimeout(tr("Account '%1' was deleted "
			    "together with message database file.")
			    .arg(accountName));
		} else {
			showStatusTextWithTimeout(tr("Account '%1' was deleted "
			    "but its message database was not deleted.")
			    .arg(accountName));
		}
	} else if (YesNoCheckboxDialog::YesUnchecked == retVal) {
		showStatusTextWithTimeout(tr("Account '%1' was deleted.")
		    .arg(accountName));
	}

	if ((YesNoCheckboxDialog::YesChecked == retVal) ||
	    (YesNoCheckboxDialog::YesUnchecked == retVal)) {
		saveSettings();
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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return;
		}
	}

	/* Method connectToIsds() acquires account information. */
	const QString dbId = globAccountDbPtr->dbId(userName + "___True");
	Q_ASSERT(!dbId.isEmpty());

	const AcntSettings &accountInfo(AccountModel::globAccounts[userName]);

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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	showStatusTextWithTimeout(tr("Change properties of account \"%1\".")
	    .arg(AccountModel::globAccounts[userName].accountName()));

	QDialog *editAccountDialog = new DlgCreateAccount(
	    AccountModel::globAccounts[userName], DlgCreateAccount::ACT_EDIT,
	    this);

	if (QDialog::Accepted == editAccountDialog->exec()) {
		showStatusTextWithTimeout(tr("Account \"%1\" was updated.")
		    .arg(userName));
		saveSettings();
	}

	editAccountDialog->deleteLater();
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
	const AcntSettings &itemSettings(AccountModel::globAccounts[userName]);

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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	/* Get current settings. */
	AcntSettings &itemSettings(AccountModel::globAccounts[userName]);

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	/* Move account database into new directory */
	if ("move" == action) {
		if (dbSet->moveToLocation(newDir)) {
			itemSettings.setDbDir(newDir);
			saveSettings();

			logInfo("Database files for '%s' have been moved "
			    "from '%s' to '%s'.\n",
			    userName.toUtf8().constData(),
			    oldDir.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' have been successfully"
			    " moved to\n\n'%2'.").arg(userName).arg(newDir),
			    QMessageBox::Ok);
		} else {
			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' could not be moved "
			        "to\n\n'%2'.").arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}

	/* Copy account database into new directory */
	} else if ("copy" == action) {
		if (dbSet->copyToLocation(newDir)) {
			itemSettings.setDbDir(newDir);
			saveSettings();

			logInfo("Database files for '%s' have been copied "
			    "from '%s' to '%s'.\n",
			    userName.toUtf8().constData(),
			    oldDir.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' have been successfully"
			    " copied to\n\n'%2'.").arg(userName).arg(newDir),
			    QMessageBox::Ok);
		} else {
			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' could not be copied "
			    "to\n\n'%2'.").arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}

	/* Create a new account database into new directory */
	} else if ("new" == action) {
		if (dbSet->reopenLocation(newDir,
		        MessageDbSet::DO_YEARLY,
		        MessageDbSet::CM_CREATE_EMPTY_CURRENT)) {
			itemSettings.setDbDir(newDir);
			saveSettings();

			logInfo("Database files for '%s' have been created "
			    "in '%s'.\n", userName.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(this,
			    tr("Change data directory for current account"),
			    tr("New database files for '%1' have been "
			    "successfully created in\n\n'%2'.")
			    .arg(userName).arg(newDir),
			    QMessageBox::Ok);
		} else {
			QMessageBox::critical(this,
			    tr("Change data directory for current account"),
			    tr("New database files for '%1' could not be "
			    "created in\n\n'%2'.").arg(userName).arg(newDir),
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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
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

	mui_filterLine->clear();
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

	/* Set filter field background colour. */
	if (text.isEmpty()) {
		mui_filterLine->setStyleSheet("QLineEdit{background: white;}");
	} else if (m_messageListProxyModel.rowCount() != 0) {
		mui_filterLine->setStyleSheet(
		    "QLineEdit{background: #afffaf;}");
	} else {
		mui_filterLine->setStyleSheet(
		    "QLineEdit{background: #ffafaf;}");
	}
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
	for (i = 3; i < (DbMsgsTblModel::rcvdItemIds().size() - 3); ++i) {
		ui->messageList->resizeColumnToContents(i);
	}
	/* Last three columns display icons. */
	for (; i < DbMsgsTblModel::rcvdItemIds().size(); ++i) {
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
	for (i = 3; i < (DbMsgsTblModel::sntItemIds().size() - 1); ++i) {
		ui->messageList->resizeColumnToContents(i);
	}
	/* Last column displays an icon. */
	for (; i < DbMsgsTblModel::rcvdItemIds().size(); ++i) {
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
	QModelIndex current(currentAccountModelIndex());

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
* Slot: Refresh AccountList
*/
void MainWindow::refreshAccountList(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	QModelIndex selectedIdx(currentAccountModelIndex());
	const QString selectedUserName(m_accountModel.userName(selectedIdx));
	/* There may be no account selected. */

	enum AccountModel::NodeType nodeType = AccountModel::nodeUnknown;
	MessageDb::MessageType msgType;
	QString year;
	qint64 dmId = -1;

	if (selectedUserName == userName) {
		/* Currently selected is the one being processed. */
		nodeType = AccountModel::nodeType(selectedIdx);
		switch (nodeType) {
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
	regenerateAccountModelYears(m_accountModel.topAcntIndex(userName));

	/*
	 * Force repaint.
	 * TODO -- A better solution?
	 */
	ui->accountList->repaint();
	if ((nodeType != AccountModel::nodeUnknown) && !year.isEmpty()) {
		QModelIndex yearIdx(accountYearlyIndex(userName, year,
		    msgType));

		if (yearIdx.isValid()) {
			ui->accountList->setCurrentIndex(yearIdx);
			accountItemCurrentChanged(selectedIdx);
		}

		if (dmId != -1) {
			QModelIndex msgIdx(messageIndex(dmId));
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


/* ========================================================================= */
/*
 * Get data about logged in user and his box.
 */
bool MainWindow::getOwnerInfoFromLogin(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	TaskDownloadOwnerInfo *task;

	task = new (std::nothrow) TaskDownloadOwnerInfo(userName);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	bool result = task->m_success;
	delete task;

	return result;
}


/* ========================================================================= */
/*
 * Get information about password expiration date.
 */
bool MainWindow::getPasswordInfoFromLogin(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	TaskDownloadPasswordInfo *task;

	task = new (std::nothrow) TaskDownloadPasswordInfo(userName);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	bool result = task->m_success;
	delete task;

	return result;
}


/* ========================================================================= */
/*
* Get data about logged in user.
*/
bool MainWindow::getUserInfoFromLogin(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	TaskDownloadUserInfo *task;

	task = new (std::nothrow) TaskDownloadUserInfo(userName);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	bool result = task->m_success;
	delete task;

	return result;
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
		const QString userName(m_accountModel.userName(index));
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

		AcntSettings itemSettings;
		itemSettings.setTestAccount(dbTestingFlag);
		itemSettings.setAccountName(dbUserName);
		itemSettings.setUserName(dbUserName);
		itemSettings.setLoginMethod(LIM_USERNAME);
		itemSettings.setPassword("");
		itemSettings.setRememberPwd(false);
		itemSettings.setSyncWithAll(false);
		itemSettings.setDbDir(m_on_import_database_dir_activate);
		m_accountModel.addAccount(itemSettings);
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

		refreshAccountList(dbUserName);

		saveSettings();
	}

	activeAccountMenuAndButtons(true);
	ui->accountList->expandAll();
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

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return TaskAuthenticateMessage::AUTH_ISDS_ERROR;
		}
	}

	showStatusTextPermanently(tr("Verifying the ZFO file \"%1\"")
	    .arg(fileName));

	TaskAuthenticateMessage *task;

	task = new (std::nothrow) TaskAuthenticateMessage(userName, fileName);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

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
	qint64 dmId = -1;
	QDateTime deliveryTime;
	{
		QModelIndex msgIdx = firstMsgColumnIdxs.first();
		dmId = msgIdx.sibling(msgIdx.row(), 0).data().toLongLong();
		deliveryTime = msgDeliveryTime(msgIdx);
	}
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(deliveryTime.isValid());

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			showStatusTextWithTimeout(tr("Message verification failed."));
			QMessageBox::critical(this, tr("Verification error"),
			    tr("An undefined error occurred!\nTry again."),
			    QMessageBox::Ok);
			return;
		}
	}

	TaskVerifyMessage *task;

	task = new (std::nothrow) TaskVerifyMessage(userName, dbSet, dmId,
	    deliveryTime);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	TaskVerifyMessage::Result result = task->m_result;
	delete task;

	switch (result) {
	case TaskVerifyMessage::VERIFY_SUCCESS:
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
	case TaskVerifyMessage::VERIFY_NOT_EQUAL:
		showStatusTextWithTimeout(tr("Server Datové schránky confirms "
		    "that the message is not valid."));
		QMessageBox::critical(this, tr("Message is not valid"),
		    tr("Message was <b>not</b> authenticated as processed "
		    "by the system Datové schránky.") + "<br/><br/>" +
		    tr("It is either not a valid ZFO file or it was modified "
		    "since it was downloaded from Datové schránky."),
		     QMessageBox::Ok);
		break;
	case TaskVerifyMessage::VERIFY_ISDS_ERR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::warning(this, tr("Verification failed"),
		    tr("Authentication of message has been stopped because "
		    "the connection to server Datové schránky failed!\n"
		    "Check your internet connection."),
		    QMessageBox::Ok);
		break;
	case TaskVerifyMessage::VERIFY_SQL_ERR:
		showStatusTextWithTimeout(tr("Message verification failed."));
		QMessageBox::warning(this, tr("Verification error"),
		    tr("The message hash is not in local database.\nPlease "
		    "download complete message from ISDS and try again."),
		    QMessageBox::Ok);
		break;
	case TaskVerifyMessage::VERIFY_ERR:
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

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
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
 * Slot: Show dialog with settings of import ZFO file(s) into database.
 */
void MainWindow::showImportZFOActionDialog(void)
/* ========================================================================= */
{
	debugSlotCall();

	QDialog *importZfo = new ImportZFODialog(this);
	connect(importZfo,
	    SIGNAL(returnZFOAction(enum ImportZFODialog::ZFOtype,
	        enum ImportZFODialog::ZFOaction, bool)),
	    this,
	    SLOT(createZFOListForImport(enum ImportZFODialog::ZFOtype,
	        enum ImportZFODialog::ZFOaction, bool)));
	importZfo->exec();
}


/* ========================================================================= */
/*
 * Func: Create ZFO file(s) list for import into database.
 */
void MainWindow::createZFOListForImport(enum ImportZFODialog::ZFOtype zfoType,
    enum ImportZFODialog::ZFOaction importType, bool checkOnServer)
/* ========================================================================= */
{
	debugSlotCall();

	bool includeSubdir = false;
	QString importDir;
	QStringList fileList, filePathList;
	QStringList nameFilter("*.zfo");
	QDir directory(QDir::home());

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

	case ImportZFODialog::IMPORT_SEL_FILES:
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

	prepareZFOImportIntoDatabase(filePathList, zfoType, checkOnServer);

	clearProgressBar();
}

/* ========================================================================= */
void MainWindow::collectImportZfoStatus(const QString &fileName, int result,
    const QString &resultDesc)
/* ========================================================================= */
{
	debugSlotCall();

	logDebugLv0NL("Received import ZFO finished for file '%s' %d: '%s'",
	    fileName.toUtf8().constData(), result,
	    resultDesc.toUtf8().constData());

	switch (result) {
	case TaskImportZfo::IMP_SUCCESS:
		m_importSucceeded.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	case TaskImportZfo::IMP_DB_EXISTS:
		m_importExisted.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	default:
		m_importFailed.append(
		    QPair<QString, QString>(fileName, resultDesc));
		break;
	}

	if (!m_zfoFilesToImport.remove(fileName)) {
		logErrorNL("Processed ZFO file that '%s' the application "
		    "has not been aware of.", fileName.toUtf8().constData());
	}

	if (m_zfoFilesToImport.isEmpty()) {
		showImportZfoResultDialogue(m_numFilesToImport,
		    m_importSucceeded, m_importExisted, m_importFailed);

		m_numFilesToImport = 0;
		m_importSucceeded.clear();
		m_importExisted.clear();
		m_importFailed.clear();

		/* Activate import buttons. */
		ui->actionImport_messages_from_database->setEnabled(true);
		ui->actionImport_ZFO_file_into_database->setEnabled(true);
	}
}

/* ========================================================================= */
/*
 * Func: Create account info for ZFO file(s) import into database.
 */
QList<Task::AccountDescr> MainWindow::createAccountInfoForZFOImport(
    bool activeOnly)
/* ========================================================================= */
{
	debugFuncCall();

	QList<Task::AccountDescr> accountList;

	/* get userName and pointer to database
	 * for all accounts from settings */
	for (int i = 0; i < ui->accountList->model()->rowCount(); i++) {
		QModelIndex index = m_accountModel.index(i, 0);

		QString userName(m_accountModel.userName(index));
		Q_ASSERT(!userName.isEmpty());

		if ((!activeOnly) ||
		    isdsSessions.isConnectedToIsds(userName) ||
		    connectToIsds(userName, this)) {
			MessageDbSet *messageDbSet = accountDbSet(userName,
			    this);
			Q_ASSERT(0 != messageDbSet);

			accountList.append(
			    Task::AccountDescr(userName, messageDbSet));
		}
	}

	return accountList;
}


/* ========================================================================= */
/*
 * Func: Prepare import ZFO file(s) into database by ZFO type.
 */
void MainWindow::prepareZFOImportIntoDatabase(const QStringList &files,
    enum ImportZFODialog::ZFOtype zfoType, bool authenticate)
/* ========================================================================= */
{
	debugFuncCall();

	const QString progressBarTitle = "ZFOImport";
	QPair<QString,QString> impZFOInfo;
	QList<QPair<QString,QString>> errorFilesList; // red
	QList<QPair<QString,QString>> existFilesList; // black
	QList<QPair<QString,QString>> successFilesList; // green
	QSet<QString> messageZfoFiles;
	QSet<QString> deliveryZfoFiles;
	int zfoCnt = files.size();

	m_zfoFilesToImport.clear();
	m_importSucceeded.clear();
	m_importExisted.clear();
	m_importFailed.clear();

	logInfo("Trying to import %d ZFO files.\n", zfoCnt);

	if (zfoCnt == 0) {
		logInfo("%s\n", "No *.zfo files in received file list.");
		showStatusTextWithTimeout(tr("No ZFO files to import."));
		return;
	}

	/*
	 * If there is no authentication request, the all account are going to
	 * be processed. It authentication is required then only active
	 * accounts are going to be selected.
	 */
	const QList<Task::AccountDescr> accountList(
	    createAccountInfoForZFOImport(authenticate));

	if (accountList.isEmpty()) {
		logInfo("%s\n", "No accounts to import into.");
		showStatusTextWithTimeout(tr("There is no account to "
		    "import of ZFO files into."));
		return;
	}

	updateProgressBar(progressBarTitle, 5);

	/* Sort ZFOs by format types. */
	foreach (const QString &file, files) {
		switch (TaskImportZfo::determineFileType(file)) {
		case TaskImportZfo::ZT_UKNOWN:
			impZFOInfo.first = file;
			impZFOInfo.second = tr("Wrong ZFO format. This "
			    "file does not contain correct data for import.");
			errorFilesList.append(impZFOInfo);
			break;
		case TaskImportZfo::ZT_MESSAGE:
			if ((ImportZFODialog::IMPORT_ALL_ZFO == zfoType) ||
			    (ImportZFODialog::IMPORT_MESSAGE_ZFO == zfoType)) {
				messageZfoFiles.insert(file);
				m_zfoFilesToImport.insert(file);
			}
			break;
		case TaskImportZfo::ZT_DELIVERY_INFO:
			if ((ImportZFODialog::IMPORT_ALL_ZFO == zfoType) ||
			    (ImportZFODialog::IMPORT_DELIVERY_ZFO == zfoType)) {
				deliveryZfoFiles.insert(file);
				m_zfoFilesToImport.insert(file);
			}
			break;
		default:
			break;
		}
	}

	m_numFilesToImport = m_zfoFilesToImport.size();

	updateProgressBar(progressBarTitle, 10);

	if (messageZfoFiles.isEmpty() && deliveryZfoFiles.isEmpty()) {
		QMessageBox::warning(this, tr("No ZFO file(s)"),
		    tr("The selection does not contain any valid ZFO files."),
		    QMessageBox::Ok);
		return;
	}

	/* Block import buttons. */
	ui->actionImport_messages_from_database->setEnabled(false);
	ui->actionImport_ZFO_file_into_database->setEnabled(false);

	updateProgressBar(progressBarTitle, 100);

	showStatusTextWithTimeout(tr("Import of ZFO files ... Planned"));

	/* First, import messages. */
	foreach (const QString &fileName, messageZfoFiles) {
		TaskImportZfo *task;

		task = new (std::nothrow) TaskImportZfo(accountList, fileName,
		    TaskImportZfo::ZT_MESSAGE, authenticate);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task);
	}
	/* Second, import delivery information. */
	foreach (const QString &fileName, deliveryZfoFiles) {
		TaskImportZfo *task;

		task = new (std::nothrow) TaskImportZfo(accountList, fileName,
		    TaskImportZfo::ZT_DELIVERY_INFO, authenticate);
		task->setAutoDelete(true);
		globWorkPool.assignLo(task);
	}
}


/* ========================================================================= */
/*
 * Show ZFO import notification dialog with results of import
 */
void MainWindow::showImportZfoResultDialogue(int filesCnt,
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
void MainWindow::exportMessageAsZFO(const QString &attachPath,
    const QString &userName, qint64 dmId, QDateTime deliveryTime,
    bool askLocation)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	/* Delivery time can be invalid. */

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsMessageBase64(dmId);
	if (base64.isEmpty()) {

		if (!messageMissingOfferDownload(dmId, deliveryTime,
		        tr("Message export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			logErrorNL("Could not access database of "
			    "freshly downloaded message '%d'.", dmId);
			return;
		}

		base64 = messageDb->msgsMessageBase64(dmId);
		if (base64.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	QString fileName = fileNameFromFormat(globPref.message_filename_format,
	    dmId, dbId, userName, "", entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	if (attachPath.isEmpty()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".zfo";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".zfo";
	}

	Q_ASSERT(!fileName.isEmpty());

	if (askLocation) {
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
	if (attachPath.isEmpty()) {
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
    QDateTime &deliveryTime)
/* ========================================================================= */
{
	debugFuncCall();

	/* selection().indexes() ? */

	enum MessageDirection msgDirect = MSG_RECEIVED;

	QModelIndex acntIndex(currentAccountModelIndex());
	switch (AccountModel::nodeType(acntIndex)) {
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

	const QString userName(m_accountModel.userName(acntIndex));
	Q_ASSERT(!userName.isEmpty());
	if (!isdsSessions.isConnectedToIsds(userName)) {
		if (!connectToIsds(userName, this)) {
			return false;
		}
	}

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);

	bool ret = false;
	TaskDownloadMessage *task;

	task = new (std::nothrow) TaskDownloadMessage(
	    userName, dbSet, msgDirect, dmId, deliveryTime);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);
	ret = TaskDownloadMessage::DM_SUCCESS == task->m_result;
	if (ret) {
		deliveryTime = task->m_mId.deliveryTime;
	}

	delete task;

	return ret;
}

bool MainWindow::messageMissingOfferDownload(qint64 dmId,
    QDateTime &deliveryTime, const QString &title)
{
	debugFuncCall();

	QMessageBox msgBox(this);

	msgBox.setWindowTitle(title);
	msgBox.setText(tr("Complete message '%1' is missing.").arg(dmId));

	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setInformativeText(
	    tr("First you must download the complete message before export.") +
	    "\n\n" +
	    tr("Do you want to download the complete message now?"));

	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);

	if ((QMessageBox::Yes == msgBox.exec()) &&
	    downloadCompleteMessage(dmId, deliveryTime)) {
		showStatusTextWithTimeout(
		    tr("Complete message '%1' has been downloaded.").
		    arg(dmId));
		return true;
	} else {
		showStatusTextWithTimeout(
		    tr("Complete message '%1' has not been downloaded.").
		    arg(dmId));
		return false;
	}
}

/* ========================================================================= */
/*
 * Export delivery information as ZFO file dialog.
 */
void MainWindow::exportDeliveryInfoAsZFO(const QString &attachPath,
    const QString &attachFileName, const QString &formatString,
    const QString &userName, qint64 dmId, QDateTime deliveryTime,
    bool askLocation)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	/* Delivery time can be invalid. */

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
	if (base64.isEmpty()) {

		if (!messageMissingOfferDownload(dmId, deliveryTime,
		        tr("Delivery info export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			logErrorNL("Could not access database of "
			    "freshly downloaded message '%d'.", dmId);
			return;
		}

		base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
		if (base64.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	QString fileName = fileNameFromFormat(formatString, dmId, dbId,
	    userName, attachFileName, entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	if (attachPath.isEmpty()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".zfo";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".zfo";
	}

	Q_ASSERT(!fileName.isEmpty());

	if (askLocation) {
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
	if (attachPath.isEmpty()) {
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
    const QString &userName, qint64 dmId, QDateTime deliveryTime,
    bool askLocation)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	/* Delivery time can be invalid. */

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
	if (base64.isEmpty()) {

		if (!messageMissingOfferDownload(dmId, deliveryTime,
		        tr("Delivery info export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			logErrorNL("Could not access database of "
			    "freshly downloaded message '%d'.", dmId);
			return;
		}

		base64 = messageDb->msgsGetDeliveryInfoBase64(dmId);
		if (base64.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	QString fileName = fileNameFromFormat(formatString, dmId, dbId,
	    userName, attachFileName, entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	if (attachPath.isEmpty()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".pdf";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".pdf";
	}

	if (askLocation) {
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
	if (attachPath.isEmpty()) {
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
    const QString &userName, qint64 dmId, QDateTime deliveryTime,
    bool askLocation)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	/* Delivery time can be invalid. */

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);
	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(dmId);

	QString dbId = globAccountDbPtr->dbId(userName + "___True");

	QByteArray base64 = messageDb->msgsMessageBase64(dmId);
	if (base64.isEmpty()) {

		if (!messageMissingOfferDownload(dmId, deliveryTime,
		        tr("Message export error!"))) {
			return;
		}

		messageDb = dbSet->accessMessageDb(deliveryTime, false);
		if (0 == messageDb) {
			Q_ASSERT(0);
			logErrorNL("Could not access database of "
			    "freshly downloaded message '%d'.", dmId);
			return;
		}

		base64 = messageDb->msgsMessageBase64(dmId);
		if (base64.isEmpty()) {
			Q_ASSERT(0);
			return;
		}
	}

	QString fileName = fileNameFromFormat(globPref.message_filename_format,
	    dmId, dbId, userName, "", entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	if (attachPath.isEmpty()) {
		fileName = m_on_export_zfo_activate + QDir::separator() +
		    fileName + ".pdf";
	} else {
		fileName = attachPath + QDir::separator() + fileName + ".pdf";
	}

	if (askLocation) {
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
	if (attachPath.isEmpty()) {
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

/*!
 * @brief Creates email header and message body.
 */
static
void createEmailMessage(QString &message, const QString &subj,
    const QString &boundary)
{
	message.clear();

	const QString newLine("\n"); /* "\r\n" ? */

	/* Rudimentary header. */
	message += "Subject: " + subj + newLine;
	message += "MIME-Version: 1.0" + newLine;
	message += "Content-Type: multipart/mixed;" + newLine +
	    " boundary=\"" + boundary + "\"" + newLine;

	/* Body. */
	message += newLine;
	message += "--" + boundary + newLine;
	message += "Content-Type: text/plain; charset=UTF-8" + newLine;
	message += "Content-Transfer-Encoding: 8bit" + newLine;

//	message += newLine + newLine + newLine + newLine;
	message += newLine;
	message += "-- " + newLine; /* Must contain the space. */
	message += " " + QObject::tr("Created using Datovka") + " " VERSION "." + newLine;
	message += " <URL: " DATOVKA_HOMEPAGE_URL ">" + newLine;
}

/*!
 * @brief Adds attachment into email.
 */
static
void addAttachmentToEmailMessage(QString &message, const QString &attachName,
    const QByteArray &base64, const QString &boundary)
{
	const QString newLine("\n"); /* "\r\n" ? */

	QMimeDatabase mimeDb;

	QMimeType mimeType(
	    mimeDb.mimeTypeForData(QByteArray::fromBase64(base64)));

	message += newLine;
	message += "--" + boundary + newLine;
	message += "Content-Type: " + mimeType.name() + "; charset=UTF-8;" + newLine +
	    " name=\"" + attachName +  "\"" + newLine;
	message += "Content-Transfer-Encoding: base64" + newLine;
	message += "Content-Disposition: attachment;" + newLine +
	    " filename=\"" + attachName + "\"" + newLine;

	for (int i = 0; i < base64.size(); ++i) {
		if ((i % 60) == 0) {
			message += newLine;
		}
		message += base64.at(i);
	}
	message += newLine;
}

/*!
 * @brief Adds last line into email.
 */
static
void finishEmailMessage(QString &message, const QString &boundary)
{
	const QString newLine("\n"); /* "\r\n" ? */

	message += newLine + "--" + boundary + "--" + newLine;
}

void MainWindow::exportSelectedMessagesAsZFO(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (0 == firstMsgColumnIdxs.size()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	foreach (const QModelIndex &frstIdx, firstMsgColumnIdxs) {
		if (!frstIdx.isValid()) {
			Q_ASSERT(0);
			return;
		}

		qint64 dmId = frstIdx.data().toLongLong();
		Q_ASSERT(dmId >= 0);
		QDateTime deliveryTime(msgDeliveryTime(frstIdx));

		exportMessageAsZFO(QString(), userName, dmId, deliveryTime,
		    true);
	}
}

void MainWindow::exportSelectedDeliveryInfosAsZFO(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (0 == firstMsgColumnIdxs.size()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	foreach (const QModelIndex &frstIdx, firstMsgColumnIdxs) {
		if (!frstIdx.isValid()) {
			Q_ASSERT(0);
			return;
		}

		qint64 dmId = frstIdx.data().toLongLong();
		Q_ASSERT(dmId >= 0);
		QDateTime deliveryTime(msgDeliveryTime(frstIdx));

		exportDeliveryInfoAsZFO(QString(), QString(),
		    globPref.delivery_filename_format, userName, dmId,
		    deliveryTime, true);
	}
}

void MainWindow::exportSelectedDeliveryInfosAsPDF(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (0 == firstMsgColumnIdxs.size()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	foreach (const QModelIndex &frstIdx, firstMsgColumnIdxs) {
		if (!frstIdx.isValid()) {
			Q_ASSERT(0);
			return;
		}

		qint64 dmId = frstIdx.data().toLongLong();
		Q_ASSERT(dmId >= 0);
		QDateTime deliveryTime(msgDeliveryTime(frstIdx));

		exportDeliveryInfoAsPDF(QString(), QString(),
		    globPref.delivery_filename_format, userName, dmId,
		    deliveryTime, true);
	}
}

void MainWindow::exportSelectedMessageEnvelopesAsPDF(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (0 == firstMsgColumnIdxs.size()) {
		return;
	}

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	foreach (const QModelIndex &frstIdx, firstMsgColumnIdxs) {
		if (!frstIdx.isValid()) {
			Q_ASSERT(0);
			return;
		}

		qint64 dmId = frstIdx.data().toLongLong();
		Q_ASSERT(dmId >= 0);
		QDateTime deliveryTime(msgDeliveryTime(frstIdx));

		exportMessageEnvelopeAsPDF(QString(), userName, dmId,
		    deliveryTime, true);
	}
}

void MainWindow::sendMessagesZfoEmail(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (0 == firstMsgColumnIdxs.size()) {
		return;
	}

	QString emailMessage;
	const QString boundary("-----" +
	    DlgChangePwd::generateRandomString(16) + "_" +
	    QDateTime::currentDateTimeUtc().toString(
	        "dd.MM.yyyy-HH:mm:ss.zzz"));

	QString subject = (1 == firstMsgColumnIdxs.size()) ?
	    tr("Data message") : tr("Data messages");

	subject += " " + firstMsgColumnIdxs.first().data().toString();
	if (firstMsgColumnIdxs.size() > 1) {
		for (int i = 1; i < firstMsgColumnIdxs.size(); ++i) {
			subject += ", " +
			    firstMsgColumnIdxs.at(i).data().toString();
		}
	}

	createEmailMessage(emailMessage, subject, boundary);

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	QDateTime deliveryTime(msgDeliveryTime(firstMsgColumnIdxs.first()));

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	foreach (const QModelIndex &frstIdx, firstMsgColumnIdxs) {
		if (!frstIdx.isValid()) {
			Q_ASSERT(0);
			return;
		}

		qint64 dmId = frstIdx.data().toLongLong();

		QByteArray base64 = messageDb->msgsMessageBase64(dmId);
		if (base64.isEmpty()) {

			if (!messageMissingOfferDownload(dmId, deliveryTime,
			        tr("Message export error!"))) {
				return;
			}

			messageDb = dbSet->accessMessageDb(deliveryTime, false);
			if (0 == messageDb) {
				Q_ASSERT(0);
				logErrorNL("Could not access database of "
				    "freshly downloaded message '%d'.", dmId);
				return;
			}

			base64 = messageDb->msgsMessageBase64(dmId);
			if (base64.isEmpty()) {
				Q_ASSERT(0);
				return;
			}
		}

		QString attachName(QString("DDZ_%1.zfo").arg(dmId));

		if (attachName.isEmpty()) {
			Q_ASSERT(0);
			return;
		}

		addAttachmentToEmailMessage(emailMessage, attachName, base64,
		    boundary);
	}

	finishEmailMessage(emailMessage, boundary);

	QString tmpEmailFile = writeTemporaryFile(
	    TMP_ATTACHMENT_PREFIX "mail.eml", emailMessage.toUtf8());

	if (!tmpEmailFile.isEmpty()) {
		QDesktopServices::openUrl(QUrl::fromLocalFile(tmpEmailFile));
	}
}

void MainWindow::sendAllAttachmentsEmail(void)
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (0 == firstMsgColumnIdxs.size()) {
		return;
	}

	QString emailMessage;
	const QString boundary("-----" +
	    DlgChangePwd::generateRandomString(16) + "_" +
	    QDateTime::currentDateTimeUtc().toString(
	        "dd.MM.yyyy-HH:mm:ss.zzz"));

	QString subject = (1 == firstMsgColumnIdxs.size()) ?
	    tr("Attachments of message") : tr("Attachments of messages");

	subject += " " + firstMsgColumnIdxs.first().data().toString();
	if (firstMsgColumnIdxs.size() > 1) {
		for (int i = 1; i < firstMsgColumnIdxs.size(); ++i) {
			subject += ", " +
			    firstMsgColumnIdxs.at(i).data().toString();
		}
	}

	createEmailMessage(emailMessage, subject, boundary);

	const QString userName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!userName.isEmpty());

	QDateTime deliveryTime(msgDeliveryTime(firstMsgColumnIdxs.first()));

	MessageDbSet *dbSet = accountDbSet(userName, this);
	Q_ASSERT(0 != dbSet);
	MessageDb *messageDb = dbSet->accessMessageDb(deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	foreach (const QModelIndex &frstIdx, firstMsgColumnIdxs) {
		if (!frstIdx.isValid()) {
			Q_ASSERT(0);
			return;
		}

		qint64 dmId = frstIdx.data().toLongLong();

		QList<MessageDb::FileData> attachList =
		    messageDb->getFilesFromMessage(dmId);
		if (attachList.isEmpty()) {

			if (!messageMissingOfferDownload(dmId, deliveryTime,
			        tr("Message export error!"))) {
				return;
			}

			messageDb = dbSet->accessMessageDb(deliveryTime, false);
			if (0 == messageDb) {
				Q_ASSERT(0);
				logErrorNL("Could not access database of "
				    "freshly downloaded message '%d'.", dmId);
				return;
			}

			attachList = messageDb->getFilesFromMessage(dmId);
			if (attachList.isEmpty()) {
				Q_ASSERT(0);
				return;
			}
		}

		foreach (const MessageDb::FileData &attach, attachList) {
			Q_ASSERT(!attach.dmFileDescr.isEmpty());
			Q_ASSERT(!attach.dmEncodedContent.isEmpty());

			addAttachmentToEmailMessage(emailMessage,
			    attach.dmFileDescr, attach.dmEncodedContent,
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

	QModelIndexList attachmentIndexes;
	{
		QItemSelectionModel *selectionModel =
		    ui->messageAttachmentList->selectionModel();
		if (0 == selectionModel) {
			Q_ASSERT(0);
			return;
		}
		attachmentIndexes = selectionModel->selectedRows(0);
	}

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
	    DlgChangePwd::generateRandomString(16) + "_" +
	    QDateTime::currentDateTimeUtc().toString(
	        "dd.MM.yyyy-HH:mm:ss.zzz"));

	QString subject = ((1 == attachmentIndexes.size()) ?
	    tr("Attachment of message %1") : tr("Attachments of message %1")).
	    arg(dmId);

	createEmailMessage(emailMessage, subject, boundary);

	foreach (const QModelIndex &attachIdx, attachmentIndexes) {
		QString attachmentName(attachIdx.sibling(attachIdx.row(),
		    DbFlsTblModel::FNAME_COL).data().toString());
		QByteArray base64Data(attachIdx.sibling(attachIdx.row(),
		    DbFlsTblModel::CONTENT_COL).data().toByteArray());

		addAttachmentToEmailMessage(emailMessage, attachmentName,
		    base64Data, boundary);
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

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();
	qint64 dmId = msgIdx.data().toLongLong();
	QDateTime deliveryTime = msgDeliveryTime(msgIdx);
	Q_ASSERT(deliveryTime.isValid());

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()), this);
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
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
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
	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
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

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()), this);
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
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
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
	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());
	if (1 != firstMsgColumnIdxs.size()) {
		return;
	}

	const QModelIndex &msgIdx = firstMsgColumnIdxs.first();
	qint64 dmId = msgIdx.data().toLongLong();
	QDateTime deliveryTime = msgDeliveryTime(msgIdx);
	if (!deliveryTime.isValid()) {
		return;
	}

	if (!msgIdx.isValid()) {
		Q_ASSERT(0);
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()), this);
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

	QString msgBoxContent;
	QString msgBoxTitle = accountName +
	    ": " + tr("Error during a connection to ISDS server!");

	if (isdsMsg.isEmpty()) {
		isdsMsg = isds_strerror((isds_error)status);
	}


	switch(status) {
	case IE_NOT_LOGGED_IN:
		msgBoxTitle = accountName + ": "
		    + tr("Error during authentication!");
		msgBoxContent =
		    tr("It was not possible to connect to your Databox "
		    "from account \"%1\".").arg(accountName)
		    + "<br><br>" +
		    "<b>" + tr("Authentication failed!")
		    + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("Please check your credentials and login method "
		    "together with your password.") + " " +
		    tr("It is also possible that your password has expired - "
		    "in this case, you need to use the official web "
		    "interface of Datové schránky to change it.");
		break;

	case IE_PARTIAL_SUCCESS:
		msgBoxTitle = accountName +
		    ": " + tr("Error during OTP authentication!");
		msgBoxContent =
		    tr("It was not possible to connect to your Databox.")
		    + "<br><br>" +
		    "<b>" + tr("OTP authentication failed!")
		    + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("Please check your credentials together with entered "
		    "security/SMS code and try again.") + " " +
		    tr("It is aslo possible that your password has expired - "
		    "in this case, you need to use the official web "
		    "interface of Datové schránky to change it.");
		break;

	case IE_TIMED_OUT:
		msgBoxContent =
		    tr("It was not possible to establish a connection "
		    "within a set time.")
		    + "<br><br>" +
		    "<b>" + tr("Timeout for connection to server expired!")
		    + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is either caused by an extremely slow and/or "
		    "unstable connection or by an improper setup.") + " " +
		    tr("Please check your internet connection and try again.")
		    + "<br><br>" + tr("It might be necessary to use a proxy to"
		    " connect to the server. Also is possible that the server"
		    " ISDS is inoperative or busy. Try again later.");
		break;

	case IE_HTTP:
		msgBoxContent =
		    tr("It was not possible to establish a connection between"
		    " your computer and the server Datove schranky.")
		    + "<br><br>" +
		    "<b>" + tr("HTTPS problem occurred or redirect to server"
		    " failed!") + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is usually caused by either lack of internet "
		    "connection or by some problem with the server ISDS.")
		    + "<br><br>" + tr("It is possible that the server ISDS is"
		    " inoperative or busy. Try again later.");
		break;

	case IE_ISDS:
		msgBoxContent =
		    tr("It was not possible to establish a connection between"
		    " your computer and the server Datove schranky.")
		    + "<br><br>" +
		    "<b>" + tr("ISDS server problem or service"
		    " was not found!") + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is usually caused by either lack of internet "
		    "connection or by some problem with the server ISDS.")
		    + "<br><br>" + tr("It is possible that the server ISDS is"
		    " inoperative or busy. Try again later.");
		break;

	case IE_NETWORK:
		msgBoxContent =
		    tr("It was not possible to establish a connection between"
		    " your computer and the server Datove schranky.")
		    + "<br><br>" +
		    "<b>" + tr("Connection to server failed or problem with"
		    " network occurred!") + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is usually caused by either lack of internet "
		    "connection or by a firewall on the way.") + " " +
		    tr("Please check your internet connection and try again.")
		    + "<br><br>" + tr("It might be necessary to use a proxy to"
		    " connect to the server. If yes, please set it up in the "
		    "File/Proxy settings menu.");
		break;

	case IE_CONNECTION_CLOSED:
		msgBoxContent =
		    tr("It was not possible to establish a connection between"
		    " your computer and the server Datove schranky.")
		    + "<br><br>" +
		    "<b>" + tr("Problem with HTTPS connection!")
		    + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is maybe caused by missing certificate for SSL"
		    " communication or application cannot open SSL socket.")
		    + "<br><br>" + tr("It is also possible that some "
		    "connection libraries are missing (CURL, SSL).");
		break;

	case IE_SECURITY:
		msgBoxContent =
		    tr("It was not possible to establish a connection between"
		    " your computer and the server Datove schranky.")
		    + "<br><br>" +
		    "<b>" + tr("HTTPS problem or security problem!")
		    + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is maybe caused by missing SSL certificate"
		    " needed for communication with server or it"
		    " was not possible establish secure connection with"
		    " the server ISDS.")
		    + "<br><br>" + tr("It is possible that the certificate"
		    " expired.");
		break;

	case IE_XML:
	case IE_SOAP:
		msgBoxContent =
		    tr("It was not possible to establish a connection between"
		    " your computer and the server Datove schranky.")
		    + "<br><br>" +
		    "<b>" + tr("SOAP problem or XML problem!")
		    + "</b>" + "<br><br>" +
		    tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("This is maybe caused by error in SOAP or"
		    " XML content for this web service is wrong.")
		    + "<br><br>" + tr("Also it is possible that the server "
		    "ISDS is inoperative or busy. Try again later.");
		break;

	default:
		msgBoxTitle = accountName +
		    ": " + tr("Datovka internal error!");
		msgBoxContent =
		    tr("It was not possible to establish a connection "
		    "to server Datove schranky.") + "<br><br>" +
		    "<b>" + tr("Datovka internal error!") + "</b>" + "<br><br>"
		    + tr("Error: ") + isdsMsg + "<br><br>" +
		    tr("An unexpected error occurred. Please restart "
		    "application and try again or you should contact the "
		    "support for this application.");
		break;
	}

	showStatusTextWithTimeout(tr("It was not possible to connect to your"
	    " databox from account \"%1\".").arg(accountName));

	QMessageBox::critical(this, msgBoxTitle, msgBoxContent,
	    QMessageBox::Ok);
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
			mw->mui_statusOnlineLabel->setText(tr("Mode: online"));
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
bool MainWindow::loginMethodUserNamePwd(AcntSettings &accountInfo,
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
			    accountInfo, DlgCreateAccount::ACT_PWD, mw);
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

		// if authentication error, show account dialog
		while (status == IE_NOT_LOGGED_IN || status == IE_PARTIAL_SUCCESS) {
			QDialog *editAccountDialog = new DlgCreateAccount(
			    accountInfo, DlgCreateAccount::ACT_EDIT, mw);
			if (QDialog::Accepted == editAccountDialog->exec()) {
				usedPwd = accountInfo.password();
				status = isdsLoginUserName(isdsSessions.session(userName),
				    userName, usedPwd, accountInfo.isTestAccount());
				isdsMsg = isdsLongMessage(isdsSessions.session(userName));
				ret = checkConnectionError(status,
				    accountInfo.accountName(), isdsMsg, mw);
			} else {
				// info: we dont reset pwd in this time.
				//accountInfo.setRememberPwd(false);
				//accountInfo.setPassword("");
				mw->showStatusTextWithTimeout(
				    tr("It was not possible to connect to "
				    "your databox from account \"%1\".")
				    .arg(accountInfo.accountName()));
				return false;
			}
		}
		// save new pwd to settings
		mw->saveSettings();

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
bool MainWindow::loginMethodCertificateOnly(AcntSettings &accountInfo,
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
			QDialog *editAccountDialog = new DlgCreateAccount(
			    accountInfo, DlgCreateAccount::ACT_CERT, mw);
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

	struct isds_ctx *session = isdsSessions.session(userName);
	Q_ASSERT(0 != session);

	status = isdsLoginSystemCert(session,
	    certPath, passphrase, accountInfo.isTestAccount());

	if (IE_SUCCESS == status) {
		/* Store the certificate password. */
		accountInfo._setPassphrase(passphrase);
		ret = true;
		/*
		 * TODO -- Notify the user that he should protect his
		 * certificates with a password?
		 */
	}

	if (0 != mw) {
		mw->saveSettings();
	}

	/* Set longer time-out. */
	isdsSessions.setSessionTimeout(userName,
	    globPref.isds_download_timeout_ms);

	return ret;
}


/* ========================================================================= */
/*
* Login to ISDS server by user certificate, username and password.
*/
bool MainWindow::loginMethodCertificateUserPwd(AcntSettings &accountInfo,
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
			QDialog *editAccountDialog = new DlgCreateAccount(
			    accountInfo, DlgCreateAccount::ACT_CERTPWD, mw);
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

	struct isds_ctx *session = isdsSessions.session(userName);
	Q_ASSERT(0 != session);

	status = isdsLoginUserCertPwd(session,
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
bool MainWindow::loginMethodCertificateIdBox(AcntSettings &accountInfo,
    MainWindow *mw)
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

	struct isds_ctx *session = isdsSessions.session(userName);
	Q_ASSERT(0 != session);

	status = isdsLoginUserCert(session,
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

	QString isdsMsg = isdsLongMessage(session);

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
bool MainWindow::loginMethodUserNamePwdOtp(AcntSettings &accountInfo,
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
			QDialog *editAccountDialog = new DlgCreateAccount(
			    accountInfo, DlgCreateAccount::ACT_PWD, mw);
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

		struct isds_ctx *session = isdsSessions.session(userName);
		Q_ASSERT(0 != session);

		status = isdsLoginUserOtp(session,
		    userName, usedPwd, accountInfo.isTestAccount(),
		    accountInfo.loginMethod(), QString(), otpres);

		isdsMsg = isdsLongMessage(session);

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

		struct isds_ctx *session = isdsSessions.session(userName);
		Q_ASSERT(0 != session);

		/* sent security code to ISDS */
		status = isdsLoginUserOtp(session,
		    userName, usedPwd, accountInfo.isTestAccount(),
		    accountInfo.loginMethod(), otpCode, otpres);

		isdsMsg = isdsLongMessage(session);

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
	AcntSettings &accountInfo(AccountModel::globAccounts[userName]);

	if (!accountInfo.isValid()) {
		logWarning("Account for user name '%s' is invalid.\n",
		    userName.toUtf8().constData());
		return false;
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

	return loginRet;
}


/* ========================================================================= */
/*
 * First connect to databox from new account
 */
bool MainWindow::firstConnectToIsds(AcntSettings &accountInfo, bool showDialog)
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

	/*
	 * Check whether currently some tasks are being processed or are
	 * pending. If nothing works finish immediately, else show question.
	 */
	if (globWorkPool.working()) {
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setText(
		    tr("Datovka is currently processing some tasks."));
		msgBox.setInformativeText(tr(
		    "Do you want to abort pending actions and close Datovka?"));
		msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
		msgBox.setDefaultButton(QMessageBox::No);
		if (QMessageBox::Yes == msgBox.exec()) {
			globWorkPool.stop();
			globWorkPool.clear();
		} else {
			event->ignore();
		}
	}
}


/* ========================================================================= */
/*
 * Verify if is a connection to ISDS and databox exists for a new account
 */
void MainWindow::getAccountUserDataboxInfo(AcntSettings accountInfo)
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

	QModelIndex index;
	m_accountModel.addAccount(accountInfo, &index);

	refreshAccountList(accountInfo.userName());

	qDebug() << "Changing selection" << index;
	/* get current account model */
	if (index.isValid()) {
		ui->accountList->selectionModel()->setCurrentIndex(index,
		    QItemSelectionModel::ClearAndSelect);
		/* Expand the tree. */
		ui->accountList->expand(index);
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

	QModelIndexList firstMsgColumnIdxs(currentFrstColMessageIndexes());

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
	if (!AccountModel::nodeTypeIsReceived(currentAccountModelIndex())) {
		return;
	}

	MessageDbSet *dbSet = accountDbSet(
	    m_accountModel.userName(currentAccountModelIndex()), this);
	Q_ASSERT(0 != dbSet);

	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();

	DbMsgsTblModel *messageModel = dynamic_cast<DbMsgsTblModel *>(
	    m_messageListProxyModel.sourceModel());
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
	   m_accountModel.userName(currentAccountModelIndex()), this);
	Q_ASSERT(0 != dbSet);

	QItemSelection storedMsgSelection =
	    ui->messageList->selectionModel()->selection();

	DbMsgsTblModel *messageModel = dynamic_cast<DbMsgsTblModel *>(
	    m_messageListProxyModel.sourceModel());
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
	const QString currentUserName(
	    m_accountModel.userName(currentAccountModelIndex()));
	Q_ASSERT(!currentUserName.isEmpty());
	MessageDbSet *dbSet = accountDbSet(currentUserName, this);
	Q_ASSERT(0 != dbSet);
	userNameAndMsgDbSet.first = currentUserName;
	userNameAndMsgDbSet.second = dbSet;
	messageDbList.append(userNameAndMsgDbSet);

	/* get pointer to database for other accounts */
	for (int i = 0; i < ui->accountList->model()->rowCount(); i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		const QString userName(m_accountModel.userName(index));
		Q_ASSERT(!userName.isEmpty());
		if (currentUserName != userName) {
			MessageDbSet *dbSet = accountDbSet(userName, this);
			userNameAndMsgDbSet.first = userName;
			userNameAndMsgDbSet.second = dbSet;
			messageDbList.append(userNameAndMsgDbSet);
		}
	}

	dlgMsgSearch = new DlgMsgSearch(messageDbList, currentUserName, this,
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
		userName = m_accountModel.userName(currentAccountModelIndex());
		Q_ASSERT(!userName.isEmpty());
		showStatusTextPermanently(tr("Checking time stamps in "
		    "account '%1'...").arg(
		        AccountModel::globAccounts[userName].accountName()));
		checkMsgsTmstmpExpiration(userName, QStringList());
		break;

	case TimestampExpirDialog::CHECK_TIMESTAMP_ALL:
		for (int i = 0; i < ui->accountList->model()->rowCount(); ++i) {
			QModelIndex index = m_accountModel.index(i, 0);
			userName = m_accountModel.userName(index);
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
		exportMessageAsZFO(newDir, userName, mId.dmId,
		    mId.deliveryTime, false);
	}
}


/* ========================================================================= */
/*
 * Func: Split database filename into mandatory entries.
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
		} else if (fileNameParts[1] == "inv") {
			dbYear = fileNameParts[1];
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
 * Slot: Prepare import of messages from database.
 */
void MainWindow::prepareMsgsImportFromDatabase(void)
/* ========================================================================= */
{
	debugSlotCall();

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle(tr("Import of mesages from database"));
	msgBox.setText(tr("This action allow to import messages from selected"
	    " database files into current account. Keep in mind that this "
	    "action may takes a few minutes based on number of messages "
	    "in the imported database. Import progress will be displayed "
	    "in the status bar."));
	msgBox.setInformativeText(tr("Do you want to continue?"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);
	if (QMessageBox::No == msgBox.exec()) {
		return;
	}

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

	doMsgsImportFromDatabase(files,
	    m_accountModel.userName(currentAccountModelIndex()));
}


/* ========================================================================= */
/*
 *  Func: Import of messages from database to selected account
 */
void MainWindow::doMsgsImportFromDatabase(const QStringList &dbFileList,
    const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	/* ProgressBar text and diference */
	const QString progressBarTitle = "DatabaseImport";
	float delta = 0.0;
	float diff = 0.0;
	clearProgressBar();

	QString msg;
	QString dbDir;
	QString dbFileName;
	QString dbUserName;
	QString dbYearFlag;
	bool dbTestingFlag;
	int sMsgCnt = 0;

	QStringList errImportList;

	/* for all selected import database files */
	for (int i = 0; i < dbFileList.count(); ++i) {

		updateProgressBar(progressBarTitle, 0);

		showStatusTextPermanently(tr("Import of messages from %1 "
		    "to account %2").arg(dbFileList.at(i)).arg(userName));

		errImportList.clear();
		sMsgCnt = 0;

		qDebug() << dbFileList.at(i) << "file is analysed...";

		/* get db filename from path */
		QFileInfo file(dbFileList.at(i));
		dbDir = file.path();
		dbFileName = file.fileName();

		updateProgressBar(progressBarTitle, 5);

		showStatusTextPermanently(tr("Import of messages from %1 "
		    "to account %2 is running").arg(dbFileName).arg(userName));

		/* parse and check the import database file name */
		if (!isValidDatabaseFileName(dbFileName, dbUserName,
		    dbYearFlag, dbTestingFlag, msg)) {
			qDebug() << msg;
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(userName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			continue;
		}

		updateProgressBar(progressBarTitle, 10);

		/* check if username of db file is relevant to account */
		if (userName != dbUserName) {
			msg = tr("This database file cannot import into "
			    "selected account because username of account "
			    "and username of database file do not correspond.");
			qDebug() << msg;
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(userName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			continue;
		}

		/* open selected database file as temporary single db */
		MessageDbSingle *srcDbSingle =
		     MessageDbSingle::createNew(dbFileList.at(i),
		     "TEMPORARYDBS");
		if (0 == srcDbSingle) {
			msg = tr("Failed to open import database file.");
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(userName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			qDebug() << dbFileList.at(i) << msg;
			continue;
		}

		qDebug() << dbFileList.at(i) << "is open";

		/* get databox ID for test if imported message is
		 * relevant to destination account */
		QString dboxId = globAccountDbPtr->dbId(userName + "___True");

		/* get all messages from source single database */
		QList<MessageDb::MsgId> msgIdList =
		    srcDbSingle->getAllMessageIDsFromDB();

		/* get database set for selected account */
		MessageDbSet *dstDbSet = accountDbSet(userName, this);
		if (0 == dstDbSet) {
			msg = tr("Failed to open database file of "
			    "target account '%1'").arg(userName);
			QMessageBox::critical(this,
			    tr("Database import: %1").arg(userName),
			    tr("File") + ": " + dbFileList.at(i) +
			    "\n\n" + msg,
			    QMessageBox::Ok);
			qDebug() << dbFileList.at(i) << msg;
			delete srcDbSingle; srcDbSingle = NULL;
			continue;
		}

		updateProgressBar(progressBarTitle, 20);
		int msgs = msgIdList.count();
		delta = 80.0 / msgs;

		// over all messages in source database do import */
		foreach (const MessageDb::MsgId &mId, msgIdList) {

			if (msgs == 0) {
				updateProgressBar(progressBarTitle, 50);
			} else {
				diff += delta;
				updateProgressBar(progressBarTitle, (20+diff));
			}

			showStatusTextPermanently(tr("Importing of message %1"
			    " into account %2 ...").arg(mId.dmId)
			    .arg(userName));

			/* select target database via delivery time for account */
			MessageDb *dstDb =
			    dstDbSet->accessMessageDb(mId.deliveryTime, true);
			if (0 == dstDb) {
				msg = tr("Failed to open database file of "
				    "target account '%1'").arg(userName);
				QMessageBox::critical(this,
				    tr("Database import: %1").arg(userName),
				    tr("File") + ": " + dbFileList.at(i) +
				    "\n\n" + msg,
				    QMessageBox::Ok);
				qDebug() << dbFileList.at(i) << msg;
				continue;
			}

			/* check if msg exists in target database */
			if (-1 != dstDb->msgsStatusIfExists(mId.dmId)) {
				msg = tr("Message '%1' already exists in "
				    "database for this account.").arg(mId.dmId);
				errImportList.append(msg);
				continue;
			}

			/* check if msg is relevant for account databox ID  */
			if (!srcDbSingle->isRelevantMsgForImport(
			    mId.dmId, dboxId)) {
				msg = tr("Message '%1' cannot be imported "
				    "into this account. Message does not "
				    "contain any valid ID of databox "
				    "corresponding with this account.").
				    arg(mId.dmId);
				errImportList.append(msg);
				continue;
			}

			/* copy all msg data to target account database */
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

		updateProgressBar(progressBarTitle, 100);

		delete srcDbSingle; srcDbSingle = NULL;

		/* show import result for import databse file */
		QMessageBox msgBox(this);
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Messages import result"));
		msg = tr("Import of messages into account '%1' "
		    "finished with result:").arg(userName) +
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
		showStatusTextPermanently(tr("Import of messages from %1 "
		    "to account %2 finished").arg(dbFileName).arg(userName));
		msgBox.exec();

		clearProgressBar();
	}

	clearStatusBar();

	/* update account model */
	refreshAccountList(userName);
}


/* ========================================================================= */
/*
 * SLot: Split message database by years.
 */
void MainWindow::splitMsgDbByYearsSlot(void)
/* ========================================================================= */
{
	debugSlotCall();

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle(tr("Database split"));
	msgBox.setText(tr("This action split current account message database "
	    "into several new databases which will contain messages relevant "
	    "by year only. It is recommended for large database because the "
	    "performance of application will be better."));
	msgBox.setInformativeText(tr("Original database file will copy to "
	    "selected directory and new database files will created in "
	    "the same location. If action finished with success, new databases"
	    " will be used instead of original. Restart of application "
	    "is required.")
	    +"\n\n" +
	    tr("Note: Keep in mind that this action may "
	    "takes a few minutes based on number of messages in the database.")
	    + "\n\n" + tr("Do you want to continue?"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);
	if (QMessageBox::Yes == msgBox.exec()) {
		splitMsgDbByYears(
		    m_accountModel.userName(currentAccountModelIndex()));
	}
}


/* ========================================================================= */
/*
 * Func: Show error message box.
 */
void MainWindow::showErrMessageBox(const QString &msgTitle,
    const QString &msgText, const QString &msgInformativeText)
/* ========================================================================= */
{
	clearProgressBar();
	showStatusTextWithTimeout(tr("Split of message database "
	    "finished with error"));
	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(msgTitle);
	msgBox.setText(msgText);
	msgBox.setInformativeText(msgInformativeText);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
	clearStatusBar();
}


/* ========================================================================= */
/*
 * Func: Set back original database path if error during database splitting.
 */
bool MainWindow::setBackOriginDb(MessageDbSet *dbset, const QString &dbDir,
    const QString &userName) {
/* ========================================================================= */

	if (0 == dbset) {
		return false;
	}

	/* set back and open origin database */
	if (!dbset->openLocation(dbDir, dbset->organisation(),
	    MessageDbSet::CM_MUST_EXIST)) {
		return false;
	}

	/* update account model */
	refreshAccountList(userName);

	return true;
}


/* ========================================================================= */
/*
 * Func: Split message database into new databases contain messages
 *       for single years.
 */
bool MainWindow::splitMsgDbByYears(const QString &userName)
/* ========================================================================= */
{
	debugFuncCall();

	/* ProgressBar text and diference */
	const QString progressBarTitle = "DatabaseSplit";
	float delta = 0.0;
	float diff = 0.0;
	clearProgressBar();

	int flags = 0;
	QString newDbDir;
	QString msgText;
	QString msgTitle = tr("Database split: %1").arg(userName);
	QString msgInformativeText = tr("Action was canceled and original "
	    "database file was returned back.");

	/* get current db file location */
	AcntSettings &itemSettings(AccountModel::globAccounts[userName]);
	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		dbDir = globPref.confDir();
	}

	/* is testing account db */
	QString testAcnt = "0";
	if (itemSettings.isTestAccount()) {
		testAcnt = "1";
		flags |= MDS_FLG_TESTING;
	}

	updateProgressBar(progressBarTitle, 5);

	/* get origin message db set based on username */
	MessageDbSet *msgDbSet = accountDbSet(userName, this);
	if (0 == msgDbSet) {
		msgText = tr("Database file for account '%1' "
		    "does not exist.").arg(userName);
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	/*
	 * test if current account already use database files
	 * split according to years
	 */
	if (MessageDbSet::DO_YEARLY == msgDbSet->organisation()) {
		msgText = tr("Database file cannot split by years "
		    "because this account already use database files "
		    "split according to years.");
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	/* get directory for saving of split database files */
	do {
		newDbDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory for new databases"),
		    m_on_import_database_dir_activate,
		    QFileDialog::ShowDirsOnly |
		    QFileDialog::DontResolveSymlinks);

		if (newDbDir.isEmpty()) {
			return false;
		}

		/* new db files cannot save into same location as original db */
		if (dbDir == newDbDir) {
			msgText = tr("Database file cannot split into "
			    "same directory.");
			msgInformativeText = tr("Please, you must choose "
			    "another directory.");
			showErrMessageBox(msgTitle, msgText, msgInformativeText);
		}
	} while (dbDir == newDbDir);

	/* remember import path */
	m_on_import_database_dir_activate = newDbDir;

	showStatusTextPermanently(tr("Copying origin database file to selected "
	    "location"));

	updateProgressBar(progressBarTitle, 10);

	/* copy current account dbset to new location and open here */
	if (!msgDbSet->copyToLocation(newDbDir)) {
		msgText = tr("Cannot copy database file for account '%1' "
		    "to '%2'").arg(userName).arg(newDbDir);
		msgInformativeText = tr("Probably not enough disk space.") +
		    + " " + msgInformativeText;
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	/* get message db for splitting */
	MessageDb *messageDb = msgDbSet->accessMessageDb(QDateTime(), false);
	if (0 == messageDb) {
		setBackOriginDb(msgDbSet, dbDir, userName);
		msgText = tr("Database file for account '%1' "
		    "does not exist.").arg(userName);
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	/* get all unique years from database */
	QStringList yearList = msgDbSet->msgsYears(MessageDb::TYPE_RECEIVED,
	    DESCENDING);
	yearList.append(msgDbSet->msgsYears(MessageDb::TYPE_SENT,
	    DESCENDING));
	yearList.removeDuplicates();

	/* create new db set for splitting of database files */
	MessageDbSet *dstDbSet = NULL;
	DbContainer temporaryDbCont("TEMPORARYDBS");
	/* open destination database file */
	dstDbSet = temporaryDbCont.accessDbSet(newDbDir, userName,
	    flags, MessageDbSet::DO_YEARLY, MessageDbSet::CM_CREATE_ON_DEMAND);
	if (0 == dstDbSet) {
		setBackOriginDb(msgDbSet, dbDir, userName);
		msgText = tr("Set of new database files for account '%1' "
		    "could not be created.").arg(userName);
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	updateProgressBar(progressBarTitle, 20);

	int years = yearList.count();
	if (years > 0) {
		delta = 60.0 / years;
	}

	for (int i = 0; i < years; ++i) {

		diff += delta;
		updateProgressBar(progressBarTitle, (20 + diff));

		showStatusTextPermanently(tr("Creating a new "
		    "database file for year %1").arg(yearList.at(i)));

		QString newDbName = userName + "_" + yearList.at(i);
		qDebug() << "Creating a new database for year" << yearList.at(i);

		QString dateStr = QString("%1-06-06 06:06:06.000")
		    .arg(yearList.at(i));
		QDateTime fakeTime = QDateTime::fromString(dateStr,
		    "yyyy-MM-dd HH:mm:ss.zzz");

		/* Delete the database file if it already exists. */
		QString fullNewDbFileName(newDbDir + "/" +
		    newDbName + "___" + testAcnt + ".db");
		if (QFileInfo::exists(fullNewDbFileName)) {
			if (QFile::remove(fullNewDbFileName)) {
				logInfo("Deleted existing file '%s'.",
				    fullNewDbFileName.toUtf8().constData());
			} else {
				logErrorNL("Cannot delete file '%s'.",
				    fullNewDbFileName.toUtf8().constData());
				msgText = tr("Existing file '%1' could not be deleted.").
				    arg(fullNewDbFileName);
				showErrMessageBox(msgTitle, msgText, msgInformativeText);
				return false;
			}
		}

		/* select destination database via fake delivery time */
		MessageDb *dstDb =
		    dstDbSet->accessMessageDb(fakeTime, true);
		if (0 == dstDb) {
			setBackOriginDb(msgDbSet, dbDir, userName);
			msgText = tr("New database file for account '%1' "
			    "corresponds with year '%2' could not be created.").
			    arg(userName).arg(yearList.at(i));
			msgInformativeText = tr("Messages were not copied.");
			showErrMessageBox(msgTitle, msgText, msgInformativeText);
			return false;
		}

		/* copy all message data to new database */
		if (!messageDb->copyRelevantMsgsToNewDb(fullNewDbFileName,
		        yearList.at(i))) {
			setBackOriginDb(msgDbSet, dbDir, userName);
			msgText = tr("Messages correspond with year "
			    "'%1' for account '%2' were not copied.")
			    .arg(yearList.at(i)).arg(userName);
			showErrMessageBox(msgTitle, msgText, msgInformativeText);
			return false;
		}
	}

	updateProgressBar(progressBarTitle, 85);

	/* set back original database path and removed previous connection */
	if (!msgDbSet->openLocation(dbDir, msgDbSet->organisation(),
	    MessageDbSet::CM_MUST_EXIST)) {
		msgText = tr("Error to set and open original database for "
		    "account '%1'").arg(userName);
		msgInformativeText = tr("Action was canceled and the origin "
		    "database is now used from location:\n'%1'").arg(newDbDir);
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	showStatusTextPermanently(tr("Replacing of new database files to "
	    "origin database location"));

	updateProgressBar(progressBarTitle, 90);

	/* move new database set to origin database path */
	if (!dstDbSet->moveToLocation(dbDir)) {
		msgText = tr("Error when move new databases for "
		    "account '%1'").arg(userName);
		msgInformativeText = tr("Action was canceled because new "
		    "databases cannot move from\n'%1'\nto origin path\n'%2'")
		    .arg(newDbDir).arg(dbDir) + "\n\n" +
		    tr("Probably not enough disk space. The origin database "
		    "is still used.");
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	showStatusTextPermanently(tr("Deleting of old database from "
	    "origin location"));

	/* delete origin database file */
	if (!msgDbSet->deleteLocation()) {
		msgText = tr("Error when removed origin database for "
		    "account '%1'").arg(userName);
		msgInformativeText = tr("Action was canceled. Please, remove "
		    "the origin database file manually from "
		    "origin location:\n'%1'").arg(dbDir);
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	updateProgressBar(progressBarTitle, 95);

	showStatusTextPermanently(tr("Opening of new database files"));

	/* open new database set in the origin location */
	if (!msgDbSet->openLocation(dbDir, msgDbSet->organisation(),
	    MessageDbSet::CM_MUST_EXIST)) {
		msgText = tr("A problem when opening new databases for "
		    "account '%1'").arg(userName);
		msgInformativeText = tr("Action was done but it cannot open "
		    "new database files. Please, restart the application.");
		showErrMessageBox(msgTitle, msgText, msgInformativeText);
		return false;
	}

	updateProgressBar(progressBarTitle, 100);
	clearProgressBar();

	/* show final success notification */
	showStatusTextWithTimeout(tr("Split of message database finished"));
	msgText = tr("Congratulation: message database for "
	    "account '%1' was split successfully. Please, restart the "
	    "application for loading of new databases.").arg(userName);
	msgInformativeText = tr("Note: Original database file was backup to:")
	    + "\n" + newDbDir;
	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setWindowTitle(msgTitle);
	msgBox.setText(msgText);
	msgBox.setInformativeText(msgInformativeText);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();

	clearStatusBar();

	/* refresh account model and account list */
	refreshAccountList(userName);

	return true;
}

void MainWindow::setUpUi(void)
{
	ui->setupUi(this);

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
	ui->messageStateCombo->addItem(QIcon(ICON_14x14_PATH "red.png"),
	    tr("Unsettled"));
	ui->messageStateCombo->addItem(QIcon(ICON_14x14_PATH "yellow.png"),
	    tr("In Progress"));
	ui->messageStateCombo->addItem(QIcon(ICON_14x14_PATH "grey.png"),
	    tr("Settled"));

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(
	    QCoreApplication::applicationVersion()));
	ui->accountTextInfo->setReadOnly(true);
}

void MainWindow::topToolBarSetUp(void)
{
	/* Add actions to the top tool bar. */
	ui->toolBar->addAction(ui->actionSync_all_accounts);
	ui->toolBar->addAction(ui->actionGet_messages);
	ui->toolBar->addSeparator();
	ui->toolBar->addAction(ui->actionSend_message);
	ui->toolBar->addAction(ui->actionReply);
	ui->toolBar->addAction(ui->actionAuthenticate_message);
	ui->toolBar->addSeparator();
	ui->toolBar->addAction(ui->actionMsgAdvancedSearch);
	ui->toolBar->addSeparator();
	ui->toolBar->addAction(ui->actionAccount_properties);
	ui->toolBar->addAction(ui->actionPreferences);

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
	ui->toolBar->addWidget(mui_filterLine);

	/* Clear message filter button. */
	mui_clearFilterLineButton = new QPushButton(this);
	mui_clearFilterLineButton->setIcon(
	    QIcon(ICON_3PARTY_PATH "delete_16.png"));
	mui_clearFilterLineButton->setToolTip(tr("Clear search field"));
	ui->toolBar->addWidget(mui_clearFilterLineButton);
	connect(mui_clearFilterLineButton, SIGNAL(clicked()), this,
	    SLOT(clearFilterField()));
}

void MainWindow::setMenuActionIcons(void)
{
	/* Don't remove the isEnabled() calls. */

	/* File menu. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/16x16/datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/32x32/datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionSync_all_accounts->setIcon(ico);
	}
	    /* Separator. */
	ui->actionAdd_account->isEnabled();
	ui->actionDelete_account->isEnabled();
	    /* Separator. */
	ui->actionImport_database_directory->isEnabled();
	    /* Separator. */
	ui->actionProxy_settings->isEnabled();
	   /* Separator. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/3party/gear_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/3party/gear_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionPreferences->setIcon(ico);
	}
	/* actionQuit -- connected in ui file. */

	/* Data box menu. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/16x16/datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/32x32/datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionGet_messages->setIcon(ico);
	}
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/16x16/datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/32x32/datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionSend_message->setIcon(ico);
	}
	    /* Separator. */
	ui->actionMark_all_as_read->isEnabled();
	    /* Separator. */
	ui->actionChange_password->isEnabled();
	    /* Separator. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/3party/letter_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/3party/letter_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionAccount_properties->setIcon(ico);
	}
	    /* Separator. */
	ui->actionMove_account_up->isEnabled();
	ui->actionMove_account_down->isEnabled();
	    /* Separator. */
	ui->actionChange_data_directory->isEnabled();
	    /* Separator. */
	ui->actionImport_messages_from_database->isEnabled();
	ui->actionImport_ZFO_file_into_database->isEnabled();
	    /* Separator. */
	ui->actionSplit_database_by_years->isEnabled();

	/* Message menu. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/16x16/datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/24x24/datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/32x32/datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionDownload_message_signed->setIcon(ico);
	}
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/16x16/datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/32x32/datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionReply->setIcon(ico);
	}
	ui->actionCreate_message_from_template->isEnabled();
	    /* Separator. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/3party/label_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/3party/label_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionSignature_detail->setIcon(ico);
	}
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/16x16/datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/32x32/datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionAuthenticate_message->setIcon(ico);
	}
	    /* Separator. */
	ui->actionOpen_message_externally->isEnabled();
	ui->actionOpen_delivery_info_externally->isEnabled();
	    /* Separator. */
	ui->actionExport_as_ZFO->isEnabled();
	ui->actionExport_delivery_info_as_ZFO->isEnabled();
	ui->actionExport_delivery_info_as_PDF->isEnabled();
	ui->actionExport_message_envelope_as_PDF->isEnabled();
	    /* Separator. */
	ui->actionEmail_ZFOs->isEnabled();
	ui->actionEmail_all_attachments->isEnabled();
	    /* Separator. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/24x24/save-all.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionSave_all_attachments->setIcon(ico);
	}
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/3party/save_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/3party/save_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionSave_selected_attachments->setIcon(ico);
	}
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/3party/folder_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/3party/folder_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionOpen_attachment->setIcon(ico);
	}
	    /* Separator. */
	ui->actionDelete_message_from_db->isEnabled();

	/* Tools menu. */
	ui->actionFind_databox->isEnabled();
	    /* Separator. */
	ui->actionAuthenticate_message_file->isEnabled();
	ui->actionView_message_from_ZPO_file->isEnabled();
	ui->actionExport_correspondence_overview->isEnabled();
	ui->actionCheck_message_timestamp_expiration->isEnabled();
	    /* Separator. */
	{
		QIcon ico;
		ico.addFile(QStringLiteral(":/icons/3party/search_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(":/icons/3party/search_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui->actionMsgAdvancedSearch->setIcon(ico);
	}

	/* Help. */
	ui->actionAbout_Datovka->isEnabled();
	ui->actionHomepage->isEnabled();
	ui->actionHelp->isEnabled();

	/* Actions that are not shown in the top menu. */
	ui->actionEmail_selected_attachments->isEnabled();
}
