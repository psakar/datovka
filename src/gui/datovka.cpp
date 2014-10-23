#include <cmath> /* ceil(3) */
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QPrinter>
//#include <QPrinterInfo>
#include <QSettings>
#include <QStackedWidget>
#include <QTableView>
#include <QTemporaryFile>
#include <QTimer>

#include "datovka.h"
#include "src/common.h"
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/gui/dlg_change_directory.h"
#include "src/gui/dlg_correspondence_overview.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_view_zfo.h"
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

#define showStatusText(qStr) \
	do { \
		statusBar->clearMessage(); \
		statusBar->showMessage((qStr), TIMER_STATUS_TIMEOUT_MS); \
	} while(0)

/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
    m_statusProgressBar(NULL),
    m_accountModel(this),
    m_accountDb("accountDb"),
    m_messageModel(NULL),
    m_messageDbs(),
    m_searchLine(NULL),
    m_messageListProxyModel(this),
    m_lastSelectedMessageId(-1),
    m_lastStoredMessageId(-1),
    m_received_1(200),
    m_received_2(200),
    m_sent_1(200),
    m_sent_2(200),
    m_sort_column(0),
    m_sort_order(""),
    m_export_correspond_dir(""),
    m_on_export_zfo_activate(""),
    m_on_import_database_dir_activate(""),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	/* Generate messages search filter */
	QWidget *spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	// toolBar is a pointer to an existing toolbar
	ui->toolBar->addWidget(spacer);

	QLabel *searchLabel = new QLabel;
	searchLabel->setText(tr("Search: "));
	ui->toolBar->addWidget(searchLabel);

	/* Message filter field. */
	m_searchLine = new QLineEdit(this);
	connect(m_searchLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterMessages(QString)));
	m_searchLine->setFixedWidth(200);
	ui->toolBar->addWidget(m_searchLine);
	/* Clear message filter button. */
	m_pushButton = new QPushButton(this);
	m_pushButton->setIcon(QIcon(ICON_3PARTY_PATH "delete_16.png"));
	m_pushButton->setToolTip(tr("Clear search field"));
	ui->toolBar->addWidget(m_pushButton);
	connect(m_pushButton, SIGNAL(clicked()), this,
	    SLOT(clearFilterField()));

	/* Create info status bar */
	statusBar = new QStatusBar(this);
	statusBar->setSizeGripEnabled(false);
	ui->statusBar->addWidget(statusBar,1);
	showStatusText(tr("Welcome"));

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
	ui->messageList->setSelectionMode(QAbstractItemView::SingleSelection);

	qDebug() << "Load " << globPref.loadConfPath();
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
	    SLOT(accountItemSelectionChanged(QModelIndex, QModelIndex)));

	/* Enable sorting of message table items. */
	ui->messageList->setSortingEnabled(true);

	/* Set default column size. */
	/* TODO -- Check whether received or sent messages are shown? */
	setReciveidColumnWidths();

	/* Attachment list. */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* Selection model may not be set. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemSelectionChanged(QModelIndex,
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
	    this, SLOT(onTableColumnSort(int)));

	/* Connect non-automatic menu actions. */
	connectTopMenuBarSlots();
	connectTopToolBarSlots();
	connectMessageActionBarSlots();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this,
	    SLOT(synchroniseAllAccounts()));

	// initialization of Timer
	if (globPref.download_on_background) {

		if (globPref.timer_value > 4) {
			timeout = globPref.timer_value * 60000;
			timer->start(timeout);
			qDebug() << "Timer set on" << globPref.timer_value <<
			    "minutes";
		} else {
			timeout = TIMER_DEFAULT_TIMEOUT_MS;
			timer->start(timeout);
			qDebug() << "Timer set on" <<
			    TIMER_DEFAULT_TIMEOUT_MS/60000 << "minutes";
		}
	}

	QTimer::singleShot(RUN_FIRST_ACTION_MS, this,
	    SLOT(setWindowsAfterInit()));
}


/* ========================================================================= */
/*
 * Do actions after main window initialization.
 */
void MainWindow::setWindowsAfterInit(void)
/* ========================================================================= */
{
	if (globPref.check_new_versions) {

		if (globPref.send_stats_with_version_checks) {
			checkNewDatovkaVersion(QCoreApplication::applicationVersion());
		} else {
			checkNewDatovkaVersion(QString());
		}
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
void MainWindow::checkNewDatovkaVersion(QString version)
/* ========================================================================= */
{
	debug_func_call();

	if (version.isNull()) {
		/* TODO - check new version only */
	} else {
		/* TODO - sent current version and check new version */
	}
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

	/* remove " symbols from passwords in dsgui.conf */
	removeQuoteFromAccountPassword(globPref.loadConfPath());

	delete timer;
	delete ui;
}


/* ========================================================================= */
/*
 * Shows the application preferences dialog.
 */
void MainWindow::applicationPreferences(void)
/* ========================================================================= */
{
	debug_func_call();

	QDialog *dlgPrefs = new DlgPreferences(this);
	dlgPrefs->exec();

	// set actuall timer value from settings if is enable
	if (globPref.download_on_background) {
		if (globPref.timer_value > 4) {
			timeout = globPref.timer_value * 60000;
			timer->start(timeout);
			qDebug() << "Timer set on" << globPref.timer_value <<
			    "minutes";
		} else {
			timeout = TIMER_DEFAULT_TIMEOUT_MS;
			timer->start(timeout);
			qDebug() << "Timer set on" <<
			    TIMER_DEFAULT_TIMEOUT_MS/60000 << "minutes";
		}
	} else {
		timer->stop();
	}
}


/* ========================================================================= */
/*
 * Proxy setting dialog.
 */
void MainWindow::proxySettings(void)
/* ========================================================================= */
{
	debug_func_call();

	QDialog *dlgProxy = new DlgProxysets(this);
	dlgProxy->exec();
}


/* ========================================================================= */
/*
 * Redraws widgets according to selected account item.
 */
void MainWindow::accountItemSelectionChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	debug_func_call();

	(void) previous; /* Unused. */

	QString html;
	DbMsgsTblModel *msgTblMdl = 0;

//	Q_ASSERT(current.isValid());
	if (!current.isValid()) {
		/* May occur on deleting last account. */
		ui->messageList->selectionModel()->disconnect(
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(messageItemSelectionChanged(QModelIndex,
		         QModelIndex)));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutAboutToBeChanged()), this,
		    SLOT(messageItemStoreSelectionOnModelChange()));
		ui->messageList->model()->disconnect(
		    SIGNAL(layoutChanged()), this,
		    SLOT(messageItemRestoreSelection()));

		/* Decouple model and show banner page. */
		ui->messageList->setModel(0);
		ui->accountTextInfo->setHtml(createDatovkaBanner(
		    QCoreApplication::applicationVersion()));
		ui->accountTextInfo->setReadOnly(true);
		return;
	}

	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(current);
	QString userName = accountUserName(accountItem);
	MessageDb *messageDb = accountMessageDb(accountItem);

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

		/* TODO - paasword dialog is shown before gui is loaded */

		if (!getOwnerInfoFromLogin(AccountModel::indexTop(current),
		    false)) {
		/* TODO -- What to do when no ISDS connection is present? */
			return;
		}
		dbId = m_accountDb.dbId(userName + "___True");
	}
	Q_ASSERT(!dbId.isEmpty());

	//qDebug() << "Clicked row" << current.row();
	//qDebug() << "Clicked type" << AccountModel::nodeType(current);

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
		setMessageActionVisibility(false);
		html = createAccountInfo(*accountItem);
		ui->actionDelete_message->setEnabled(false);
		break;
	case AccountModel::nodeRecentReceived:
		msgTblMdl = messageDb->msgsRcvdWithin90DaysModel(dbId);
		//ui->messageList->horizontalHeader()->moveSection(5,3);
		ui->actionDelete_message->setEnabled(false);
		break;
	case AccountModel::nodeRecentSent:
		msgTblMdl = messageDb->msgsSntWithin90DaysModel(dbId);
		ui->actionDelete_message->setEnabled(false);
		break;
	case AccountModel::nodeAll:
		setMessageActionVisibility(false);
		html = createAccountInfoAllField(tr("All messages"),
		    messageDb->msgsRcvdYearlyCounts(dbId),
		    messageDb->msgsSntYearlyCounts(dbId));
		ui->actionDelete_message->setEnabled(false);
		break;
	case AccountModel::nodeReceived:
		msgTblMdl = messageDb->msgsRcvdModel(dbId);
		ui->actionDelete_message->setEnabled(true);
		break;
	case AccountModel::nodeSent:
		msgTblMdl = messageDb->msgsSntModel(dbId);
		ui->actionDelete_message->setEnabled(true);
		break;
	case AccountModel::nodeReceivedYear:
		/* TODO -- Parameter check. */
		msgTblMdl = messageDb->msgsRcvdInYearModel(dbId,
		    accountItem->text());
		ui->actionDelete_message->setEnabled(true);
		break;
	case AccountModel::nodeSentYear:
		/* TODO -- Parameter check. */
		msgTblMdl = messageDb->msgsSntInYearModel(dbId,
		    accountItem->text());
		ui->actionDelete_message->setEnabled(true);
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
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(messageItemSelectionChanged(QModelIndex,
		         QModelIndex)));
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
		/* Set specific column width. */
		setReciveidColumnWidths();
		received = true;
		goto setmodel;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		/* Set specific column width. */
		setSentColumnWidths();
		received = false;

setmodel:
		ui->messageStackedWidget->setCurrentIndex(1);
		Q_ASSERT(0 != msgTblMdl);
		m_messageModel = msgTblMdl;
		ui->messageList->setModel(m_messageModel);
		/* Apply message filter. */
		filterMessages(m_searchLine->text());
		/* Connect new slot. */
		connect(ui->messageList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(messageItemSelectionChanged(QModelIndex,
		        QModelIndex)));
		connect(ui->messageList->model(),
		    SIGNAL(layoutAboutToBeChanged()), this,
		    SLOT(messageItemStoreSelectionOnModelChange()));
		connect(ui->messageList->model(),
		    SIGNAL(layoutChanged()), this,
		    SLOT(messageItemRestoreSelection()));
		/* Clear message info. */
		ui->messageInfo->clear();
		/* Clear attachment list. */
		messageItemSelectionChanged(QModelIndex());
		/* Select last message in list if there are some messages. */
		itemModel = ui->messageList->model();
		/* enable/disable buttons */
		if ((0 != itemModel) && (0 < itemModel->rowCount())) {
			messageItemRestoreSelection();
			ui->actionReply_to_the_sender->setEnabled(true);
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
	received ? setReciveidColumnWidths() : setSentColumnWidths();
}

/* ========================================================================= */
/*
 * Generates menu to selected account item.
 *     (And redraw widgets.)
 */
void MainWindow::accountItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->indexAt(point);
	QMenu *menu = new QMenu;

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
		menu->addAction(QIcon(ICON_3PARTY_PATH "folder_16.png"),
		    tr("Change data directory"),
		    this, SLOT(changeDataDirectory()));
	} else {
		menu->addAction(QIcon(ICON_3PARTY_PATH "plus_16.png"),
		    tr("Add new account"),
		    this, SLOT(addNewAccount()));
	}

	menu->exec(QCursor::pos());
}


/* ========================================================================= */
/*
 * Sets content of widgets according to selected message.
 */
void MainWindow::messageItemSelectionChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	(void) previous; /* Unused. */

	/* Disable message/attachment related buttons. */
	ui->downloadComplete->setEnabled(false);
	ui->saveAttachments->setEnabled(false);
	ui->saveAttachment->setEnabled(false);
	ui->openAttachment->setEnabled(false);
	ui->verifySignature->setEnabled(false);
	ui->signatureDetails->setEnabled(false);
	ui->actionSave_attachment->setEnabled(false);
	ui->actionOpen_attachment->setEnabled(false);
	/*
	 * Disconnect slot from model as we want to prevent a signal to be
	 * handled multiple times.
	 */
	if (0 != ui->messageAttachmentList->selectionModel()) {
		/* New model hasn't been set yet. */
		ui->messageAttachmentList->selectionModel()->disconnect(
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemSelectionChanged(QModelIndex,
		         QModelIndex)));
	}

	/* Disable model for attachment list. */
	ui->messageAttachmentList->setModel(0);

	if (!current.isValid()) {
		/* Invalid message selected. */
		messageItemStoreSelection(-1);
		/* End if invalid item is selected. */
		return;
	}

	const QAbstractItemModel *msgTblMdl = current.model();

	/* Disable message/attachment related buttons. */
	ui->downloadComplete->setEnabled(true);

	if (0 != msgTblMdl) {
		QModelIndex index = msgTblMdl->index(
		    current.row(), 0); /* First column. */

		MessageDb *messageDb = accountMessageDb(0);
		Q_ASSERT(0 != messageDb);
		int msgId = msgTblMdl->itemData(index).first().toInt();
		/* Remember last selected message. */
		messageItemStoreSelection(msgId);

		/* Mark message locally read. */
		if (!messageDb->smsgdtLocallyRead(msgId)) {
			messageDb->smsgdtSetLocallyRead(msgId);
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
			Q_ASSERT(0 != m_messageModel);
			QModelIndex modelIndex =
			    ui->messageList->selectionModel()->currentIndex();
			Q_ASSERT(modelIndex.isValid());
			m_messageModel->overideRead(
			    current.sibling(modelIndex.row(),
			        0).data().toInt(), true);
		}

		/* Generate and show message information. */
		ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId,
		    ui->verifySignature));
		ui->messageInfo->setReadOnly(true);

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
		    SLOT(attachmentItemSelectionChanged(QModelIndex,
		        QModelIndex)));
	} else {
		ui->messageInfo->setHtml("");
		ui->messageInfo->setReadOnly(true);
		ui->saveAttachments->setEnabled(false);
	}

	/* TODO */
}


/* ========================================================================= */
/*
 * Generates menu to selected message item.
 *     (And redraw widgets.)
 */
void MainWindow::messageItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex index = ui->messageList->indexAt(point);
	QMenu *menu = new QMenu;

	/* Remember last selected message. */
	messageItemStoreSelection(
	    index.model()->itemData(index).first().toInt());

	if (index.isValid()) {
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
		    tr("Export message as ZFO"), this,
		    SLOT(exportSelectedMessageAsZFO()))->
		    setEnabled(ui->actionExport_as_ZFO->isEnabled());
		menu->addAction(
		    tr("Open message externally"), this,
		    SLOT(openSelectedMessageExternally()))->
		    setEnabled(ui->actionOpen_message_externally->isEnabled());
		menu->addAction(
		    tr("Open delivery info externally"), this,
		    SLOT(openDeliveryInfoExternally()))->
		    setEnabled(ui->actionOpen_delivery_info_externally->
		    isEnabled());
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
		    SLOT(messageItemDeleteMessage()))->
		    setEnabled(ui->actionDelete_message->isEnabled());
	} else {
		menu->addAction(QIcon(ICON_16x16_PATH "datovka-message.png"),
		    tr("Create a new message"),
		    this, SLOT(createAndSendMessage()));
	}
	menu->exec(QCursor::pos());
}


/* ========================================================================= */
/*
 * Saves message selection.
 */
void MainWindow::messageItemStoreSelection(long msgId)
/* ========================================================================= */
{
	debug_func_call();

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
 * Saves message selection when model changes.
 */
void MainWindow::messageItemStoreSelectionOnModelChange(void)
/* ========================================================================= */
{
	debug_func_call();

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
	debug_func_call();

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
					}
				}
				ui->messageList-> setCurrentIndex(index);
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
					}
				}
			} else if (GlobPreferences::SELECT_NOTHING ==
				    globPref.after_start_select) {
					/* Select last row. */
					index = model->index(rowCount - 1, 0);
					ui->messageList->
					    setCurrentIndex(index);
			} else {
				Q_ASSERT(0);
			}
		} else {
			/* Select last row. */
			index = model->index(rowCount - 1, 0);
			ui->messageList->setCurrentIndex(index);
		}
	}
}


/* ========================================================================= */
/*
 * Redraws widgets according to selected attachment item.
 */
void MainWindow::attachmentItemSelectionChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	(void) previous;

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
	QModelIndex index = ui->messageAttachmentList->indexAt(point);
	QMenu *menu = new QMenu;

	if (index.isValid()) {
		//attachmentItemSelectionChanged(index);

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
	debug_func_call();

	QModelIndex selectedIndex =
	    ui->messageAttachmentList->selectionModel()->currentIndex();
	    /* selection().indexes() ? */

	//qDebug() << "Save attachment to file." << selectedIndex;

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
	QString fileName = fileNameIndex.data().toString();
	Q_ASSERT(!fileName.isEmpty());
	/* TODO -- Remember directory? */
	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save attachment"), fileName);

	//qDebug() << "Selected file: " << fileName;

	if (fileName.isEmpty()) {
		return;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		return; /* TODO -- Error message. */
	}

	/* Get data from base64. */
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(), 2);
	Q_ASSERT(dataIndex.isValid());
	if (!dataIndex.isValid()) {
		return;
	}
	//qDebug() << "Data index." << dataIndex;

	QByteArray data =
	    QByteArray::fromBase64(dataIndex.data().toByteArray());

	int written = fout.write(data);
	if (written != data.size()) {
		/* TODO -- Error message? */
	}

	fout.close();
}


/* ========================================================================= */
/*
 * Save all attachments to dir.
 */
void MainWindow::saveAllAttachmentsToDir(void)
/* ========================================================================= */
{
	debug_func_call();

	int attachments = ui->messageAttachmentList->model()->rowCount();

	QString newdir = QFileDialog::getExistingDirectory(this,
	    tr("Save attachments"), QDir::homePath(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (newdir.isNull() || newdir.isEmpty()) {
		return;
	}

	for (int i = 0; i < attachments; ++i) {

		QModelIndex index = ui->messageAttachmentList->model()->index(i,0);

		Q_ASSERT(index.isValid());
		if (!index.isValid()) {
			return;
		}

		QModelIndex fileNameIndex = index.sibling(index.row(), 3);
		Q_ASSERT(fileNameIndex.isValid());
		if(!fileNameIndex.isValid()) {
			return;
		}

		QString fileName = fileNameIndex.data().toString();
		Q_ASSERT(!fileName.isEmpty());

		fileName = newdir + "/" + fileName;

		QFile fout(fileName);
		if (!fout.open(QIODevice::WriteOnly)) {
			return;
		}

		QModelIndex dataIndex = index.sibling(index.row(), 2);
		Q_ASSERT(dataIndex.isValid());
		if (!dataIndex.isValid()) {
			return;
		}

		QByteArray data =
		    QByteArray::fromBase64(dataIndex.data().toByteArray());

		int written = fout.write(data);
		if (written != data.size()) {
		/* TODO ? */
		}

		fout.close();
	}
}


/* ========================================================================= */
/*
 * Open attachment in default application.
 */
void MainWindow::openSelectedAttachment(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex selectedIndex =
	    ui->messageAttachmentList->selectionModel()->currentIndex();
	    /* selection().indexes() ? */

	//qDebug() << "Open attachment." << selectedIndex;

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
	QString fileName = fileNameIndex.data().toString();
	Q_ASSERT(!fileName.isEmpty());
	/* TODO -- Add message id into file name? */
	fileName = TMP_ATTACHMENT_PREFIX + fileName;

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

	/* Get data from base64. */
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(), 2);
	Q_ASSERT(dataIndex.isValid());
	if (!dataIndex.isValid()) {
		return;
	}
	//qDebug() << "Data index." << dataIndex;

	QByteArray data =
	    QByteArray::fromBase64(dataIndex.data().toByteArray());

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
 * Downloads the attachments for the selected message.
 */
void MainWindow::downloadSelectedMessageAttachments(void)
/* ========================================================================= */
{
	debug_func_call();

	bool incoming = true;

	QModelIndex messageIndex =
	    ui->messageList->selectionModel()->currentIndex();
	Q_ASSERT(messageIndex.isValid());

	QString dmIDs = messageIndex.sibling(
	    messageIndex.row(), 0).data().toString();

	showStatusText(tr("Download complete message \"%1\" "
	    "from ISDS server...").arg(dmIDs));

	QModelIndex accountIndex = ui->accountList->currentIndex();
	Q_ASSERT(accountIndex.isValid());
	accountIndex = AccountModel::indexTop(accountIndex);
	QStandardItem *accountItem = m_accountModel.itemFromIndex(accountIndex);

	QModelIndex index = ui->accountList->selectionModel()->currentIndex();
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

	QList<MessageDb*> messageDbList;
	QList<bool> downloadThisAccount;

	messageDbList.clear();
	downloadThisAccount.clear();
	downloadThisAccount.append(incoming);

	MessageDb *messageDb = accountMessageDb(accountItem);
	messageDbList.append(messageDb);

	const AccountModel::SettingsMap accountInfo =
	    accountIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!connectToIsds(accountIndex, true)) {
			return;
		}
	}

	threadDownMsgComplete = new QThread();
	workerDownMsgComplete = new Worker(accountIndex,
	    dmIDs, m_accountDb, m_accountModel, 0, messageDbList,
	    downloadThisAccount, 0);

	workerDownMsgComplete->moveToThread(threadDownMsgComplete);

	connect(workerDownMsgComplete, SIGNAL(valueChanged(QString, int)),
	    this, SLOT(setProgressBarFromWorker(QString, int)));

	connect(workerDownMsgComplete,
	    SIGNAL(refreshAttachmentList(const QModelIndex, QString)),
	    this, SLOT(postDownloadSelectedMessageAttachments(
	        const QModelIndex, QString)));


	connect(workerDownMsgComplete, SIGNAL(workRequested()),
	    threadDownMsgComplete, SLOT(start()));
	connect(threadDownMsgComplete, SIGNAL(started()),
	    workerDownMsgComplete, SLOT(downloadCompleteMessage()));
	connect(workerDownMsgComplete, SIGNAL(finished()),
	    threadDownMsgComplete, SLOT(quit()), Qt::DirectConnection);
	connect(threadDownMsgComplete, SIGNAL(finished()),
	    this, SLOT(deleteThreadDownMsgComplete()));

	workerDownMsgComplete->requestWork();
}


/* ========================================================================= */
/*
 * Set tablewidget when message download worker is done.
 */
void MainWindow::postDownloadSelectedMessageAttachments(
    const QModelIndex acntTopIdx, QString dmId)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex messageIndex =
	    ui->messageList->selectionModel()->currentIndex();
	Q_ASSERT(messageIndex.isValid());
	QModelIndex accountIndex = ui->accountList->currentIndex();
	Q_ASSERT(accountIndex.isValid());
	accountIndex = AccountModel::indexTop(accountIndex);
	QStandardItem *accountItem = m_accountModel.itemFromIndex(accountIndex);

	QString msgIds = messageIndex.sibling(
	    messageIndex.row(), 0).data().toString();

	/* Test if account index or message index were changed */
	if (!(accountIndex == acntTopIdx && msgIds == dmId)) {
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
		    SLOT(attachmentItemSelectionChanged(QModelIndex,
		         QModelIndex)));
	}

	MessageDb *messageDb = accountMessageDb(accountItem);

	int msgId = messageIndex.sibling(
	    messageIndex.row(), 0).data().toInt();

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId,
	    ui->verifySignature));
	ui->messageInfo->setReadOnly(true);

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
	    SLOT(attachmentItemSelectionChanged(QModelIndex,
	        QModelIndex)));
}


/* ========================================================================= */
/*
 * Mark all messages as read in selected account item.
 */
void MainWindow::accountItemMarkAllRead(void)
/* ========================================================================= */
{
	debug_func_call();

	/* Save current index. */
	QModelIndex selectedAcntIndex = ui->accountList->currentIndex();

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	const QAbstractItemModel *msgTblMdl = ui->messageList->model();
	Q_ASSERT(0 != msgTblMdl);

	int count = msgTblMdl->rowCount();

	if (0 != msgTblMdl) {
		for (int i = 0; i < count; i++) {
			QModelIndex index = msgTblMdl->index(i, 0);
			int msgId = msgTblMdl->itemData(index).first().toInt();
			messageDb->smsgdtSetLocallyRead(msgId);
		}
	}

	/* Regenerate account tree. */
	regenerateAllAccountModelYears();

	/* Restore selection. */
	if (selectedAcntIndex.isValid()) {
		ui->accountList->selectionModel()->setCurrentIndex(
		    selectedAcntIndex, QItemSelectionModel::ClearAndSelect);
		accountItemSelectionChanged(selectedAcntIndex);
		/*
		 * TODO -- When using on year account item then the first
		 * switching on a parent item (Received) does not redraw the
		 * message list.
		 */
	}
}


/* ========================================================================= */
/*
 * Deletes selected message from message list.
 */
void MainWindow::messageItemDeleteMessage(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this,
	    tr("Delete message ") + dmId,
	    tr("Do you want to delete message") +  " '" + dmId +
	    "'?",
	    QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
		switch (eraseMessage(acntTopIdx, dmId)) {
		case Q_SUCCESS:
			/*
			 *  Hide deleted message in view. The view/model will
			 *  be regenerated according to the updated content
			 *  of the DB when the account selection has been
			 *  changed.
			 */
			ui->messageList->hideRow(msgIdx.row());
			break;
		default:
			break;
		}
	}
}


/* ========================================================================= */
/*
 * Downloads new messages from server for all accounts.
 */
void MainWindow::synchroniseAllAccounts(void)
/* ========================================================================= */
{
	debug_func_call();

	/*
	 * TODO -- The actual work (function) which the worker performs should
	 * be defined somewhere outside of the worker object.
	 */

	showStatusText(tr("Synchronise all accounts with ISDS server..."));

	if (globPref.download_on_background) {
		timer->stop();
	}

	int accountCount = ui->accountList->model()->rowCount();
	QList<MessageDb*> messageDbList;
	QList<bool> downloadThisAccounts;
	messageDbList.clear();
	downloadThisAccounts.clear();
	bool isConnectActive = true;

	for (int i = 0; i < accountCount; i++) {

		QModelIndex index = m_accountModel.index(i, 0);
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

		isConnectActive = true;

		if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
			isConnectActive = connectToIsds(index, true);
		}

		downloadThisAccounts.append(isConnectActive);

		const QStandardItem *accountItem =
		    m_accountModel.itemFromIndex(index);
		MessageDb *messageDb = accountMessageDb(accountItem);
		messageDbList.append(messageDb);

	}

	ui->actionSync_all_accounts->setEnabled(false);
	ui->actionReceived_all->setEnabled(false);
	ui->actionDownload_messages->setEnabled(false);
	ui->actionGet_messages->setEnabled(false);

	threadSyncAll = new QThread();
	workerSyncAll = new Worker(QModelIndex(), QString(),
	    m_accountDb, m_accountModel, accountCount, messageDbList,
	    downloadThisAccounts, 0);
	workerSyncAll->moveToThread(threadSyncAll);

	connect(workerSyncAll, SIGNAL(valueChanged(QString, int)),
	    this, SLOT(setProgressBarFromWorker(QString, int)));
	connect(workerSyncAll,
	    SIGNAL(changeStatusBarInfo(bool, QString, int, int, int, int)),
	    this,
	    SLOT(dataFromWorkerToStatusBarInfo(bool, QString, int, int, int, int)));
	connect(workerSyncAll, SIGNAL(refreshAccountList(const QModelIndex)),
	    this, SLOT(refreshAccountListFromWorker(const QModelIndex)));
	connect(workerSyncAll, SIGNAL(workRequested()),
	    threadSyncAll, SLOT(start()));
	connect(threadSyncAll, SIGNAL(started()),
	    workerSyncAll, SLOT(syncAllAccounts()));
	connect(workerSyncAll, SIGNAL(finished()), threadSyncAll,
	    SLOT(quit()), Qt::DirectConnection);
	connect(threadSyncAll, SIGNAL(finished()), this,
	    SLOT(deleteThreadSyncAll()));

	workerSyncAll->requestWork();
}


/* ========================================================================= */
/*
* Set info status bar from worker.
*/
void MainWindow::dataFromWorkerToStatusBarInfo(bool completed,
    QString accoutName, int rt, int rn, int st, int sn)
/* ========================================================================= */
{
	if (completed) {
		if (accoutName.isNull()) {
			showStatusText(tr("Messages on the server") + ": " +
			    QString::number(rt) + " " + tr("received") +
			    " (" + QString::number(rn) + " " + tr("news")
			    + "); "+ QString::number(st) + " " + tr("sent") +
			    " (" + QString::number(sn) + " " + tr("news") + ")");
		} else {
			showStatusText(accoutName + ": "+
			    tr("Messages on the server") + ": " +
			    QString::number(rt) + " " + tr("received") +
			    " (" + QString::number(rn) + " " + tr("news")
			    + "); "+ QString::number(st) + " " + tr("sent") +
			    " (" + QString::number(sn) + " " + tr("news") + ")");
		}
	} else {
		showStatusText(tr("Synchronise accounts %1 with ISDS server...")
		    .arg(accoutName));
	}
}


/* ========================================================================= */
/*
* Download sent/received message list for current (selected) account
*/
void MainWindow::synchroniseSelectedAccount(void)
/* ========================================================================= */
{
	debug_func_call();

	/*
	 * TODO -- Save/restore the position of selected account and message.
	 */

	QList<MessageDb*> messageDbList;
	QList<bool> downloadThisAccount;

	messageDbList.clear();
	downloadThisAccount.clear();
	downloadThisAccount.append(true);

	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);
	QStandardItem *accountItem = m_accountModel.itemFromIndex(index);

	MessageDb *messageDb = accountMessageDb(accountItem);
	messageDbList.append(messageDb);

	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	showStatusText(tr("Synchronise account \"%1\" with ISDS server...")
	    .arg(accountInfo.accountName()));

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!connectToIsds(index, true)) {
			return;
		}
	}

	threadSyncOne = new QThread();
	workerSyncOne = new Worker(index, QString(), m_accountDb,
	    m_accountModel, 0, messageDbList, downloadThisAccount, 0);
	workerSyncOne->moveToThread(threadSyncOne);

	connect(workerSyncOne, SIGNAL(valueChanged(QString, int)),
	    this, SLOT(setProgressBarFromWorker(QString, int)));
	connect(workerSyncOne,
	    SIGNAL(changeStatusBarInfo(bool, QString, int, int, int, int)),
	    this,
	    SLOT(dataFromWorkerToStatusBarInfo(bool, QString, int, int, int, int)));
	connect(workerSyncOne, SIGNAL(refreshAccountList(const QModelIndex)),
	    this, SLOT(refreshAccountListFromWorker(const QModelIndex)));
	connect(workerSyncOne, SIGNAL(workRequested()),
	    threadSyncOne, SLOT(start()));
	connect(threadSyncOne, SIGNAL(started()),
	    workerSyncOne, SLOT(syncOneAccount()));
	connect(workerSyncOne, SIGNAL(finished()),
	    threadSyncOne, SLOT(quit()), Qt::DirectConnection);
	connect(threadSyncOne, SIGNAL(finished()),
	    this, SLOT(deleteThreadSyncOne()));

	workerSyncOne->requestWork();
}


/* ========================================================================= */
/*
 * Generate account info HTML message.
 */
QString MainWindow::createAccountInfo(const QStandardItem &topItem) const
/* ========================================================================= */
{
	const AccountModel::SettingsMap &itemSettings =
	    topItem.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString html;

	html.append("<div style=\"margin-left: 12px;\">");
	html.append("<h3>");
	if (itemSettings[TEST].toBool()) {
		html.append(tr("Test account"));
	} else {
		html.append(tr("Standard account"));
	}
	html.append("</h3>");

	html.append(strongAccountInfoLine(tr("Account name"),
	    itemSettings[NAME].toString()));
	html.append("<br>");
	html.append(strongAccountInfoLine(tr("User name"),
	    itemSettings[USER].toString()));

	AccountEntry accountEntry;
	accountEntry = m_accountDb.accountEntry(
	    itemSettings[USER].toString() + "___True");

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
					    QString::number(
					        accountEntry.value(key).toInt())));
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
			default:
				Q_ASSERT(0);
				break;
			}
		}
	}

	html.append("<br>");
	QString key = itemSettings[USER].toString() + "___True";
	QString info = m_accountDb.getPwdExpirFromDb(key);
	if (info.isEmpty()) {
		info = tr("unknown or without expiration");
	}

	html.append(strongAccountInfoLine(tr("Password expiration date"),
	    info));

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
	QString userName = itemSettings[USER].toString();
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
	MessageDb *db;

	if (0 == accountItem) {
		accountItem = m_accountModel.itemFromIndex(
		    ui->accountList->selectionModel()->currentIndex());
	}

	accountItemTop = AccountModel::itemTop(accountItem);
	Q_ASSERT(0 != accountItemTop);

	/* Get user name and db location. */
	const AccountModel::SettingsMap &itemSettings =
	    accountItemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	const QString &userName = itemSettings[USER].toString();
	Q_ASSERT(!userName.isEmpty());

	QString dbDir = itemSettings[DB_DIR].toString();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}
	db = m_messageDbs.accessMessageDb(userName, dbDir,
	    itemSettings[TEST].toBool());
	Q_ASSERT(0 != db);

	return db;
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
	debug_func_call();

	QString username = settings.value("default_account/username", "")
	   .toString();
	if (!username.isEmpty()) {
		int topItemCount = m_accountModel.rowCount();
		for (int i = 0; i < topItemCount; i++) {
			const QStandardItem *item = m_accountModel.item(i,0);
			QString user = item->data(
			    ROLE_ACNT_CONF_SETTINGS).toMap()[USER].toString();
			if (user == username) {
				QModelIndex index = m_accountModel.
				    indexFromItem(item);
				ui->accountList->
				    setCurrentIndex(index.child(0,0));
				accountItemSelectionChanged(index.child(0,0));
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
	debug_func_call();

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
	    SLOT(importDatabaseDirectory()));
	connect(ui->actionProxy_settings, SIGNAL(triggered()), this,
	    SLOT(proxySettings()));
	connect(ui->actionPreferences, SIGNAL(triggered()), this,
	    SLOT(applicationPreferences()));
	/* actionQuit -- connected in ui file. */

	/* Databox. */
	connect(ui->actionGet_messages, SIGNAL(triggered()), this,
	    SLOT(synchroniseSelectedAccount()));
	connect(ui->actionSent_message, SIGNAL(triggered()), this,
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
	connect(ui->actionDelete_message, SIGNAL(triggered()), this,
	    SLOT(messageItemDeleteMessage()));

	/* Tools. */
	connect(ui->actionFind_databox, SIGNAL(triggered()), this,
	    SLOT(findDatabox()));
	connect(ui->actionAuthenticate_message_file, SIGNAL(triggered()), this,
	    SLOT(authenticateMessageFile()));
	connect(ui->actionView_message_from_ZPO_file, SIGNAL(triggered()), this,
	    SLOT(viewMessageFromZFO()));
	connect(ui->actionExport_correspondence_overview, SIGNAL(triggered()), this,
	    SLOT(exportCorrespondenceOverview()));

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
	debug_func_call();

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
	debug_func_call();

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
	/* Downloading attachments also trigers signature verification. */
	connect(ui->verifySignature, SIGNAL(clicked()), this,
	    SLOT(downloadSelectedMessageAttachments()));
	connect(ui->signatureDetails, SIGNAL(clicked()), this,
	    SLOT(showSignatureDetails()));
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
		const QString &userName = itemSettings[USER].toString();

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
	 * Several nodes may be updated at once, because som messages may be
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
	const QString &userName = itemSettings[USER].toString();
	Q_ASSERT(!userName.isEmpty());
	QString dbDir = itemSettings[DB_DIR].toString();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}
	db = m_messageDbs.accessMessageDb(userName, dbDir,
	    itemSettings[TEST].toBool());
	Q_ASSERT(0 != db);
	QString dbId = m_accountDb.dbId(userName + "___True");

	/* Received. */
	unreadMsgs = db->msgsRcvdUnreadWithin90Days(dbId);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = db->msgsRcvdYears(dbId);
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
	yearList = db->msgsSntYears(dbId);
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
	debug_func_call();

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
	const QString &userName = itemSettings[USER].toString();
	Q_ASSERT(!userName.isEmpty());
	QString dbDir = itemSettings[DB_DIR].toString();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}
	db = m_messageDbs.accessMessageDb(userName, dbDir,
	    itemSettings[TEST].toBool());
	Q_ASSERT(0 != db);
	QString dbId = m_accountDb.dbId(userName + "___True");

	/* Received. */
	unreadMsgs = db->msgsRcvdUnreadWithin90Days(dbId);
	m_accountModel.updateRecentUnread(topItem,
	    AccountModel::nodeRecentReceived, unreadMsgs);
	yearList = db->msgsRcvdYears(dbId);
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
	yearList = db->msgsSntYears(dbId);
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
	debug_func_call();

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
		const QString &userName = itemSettings[USER].toString();
		Q_ASSERT(!userName.isEmpty());
		QString dbDir = itemSettings[DB_DIR].toString();
		if (dbDir.isEmpty()) {
			/* Set default directory name. */
			dbDir = globPref.confDir();
		}
		db = m_messageDbs.accessMessageDb(userName, dbDir,
		    itemSettings[TEST].toBool());
		Q_ASSERT(0 != db);
		QString dbId = m_accountDb.dbId(userName + "___True");

		/* Received. */
		unreadMsgs = db->msgsRcvdUnreadWithin90Days(dbId);
		m_accountModel.updateRecentUnread(itemTop,
		    AccountModel::nodeRecentReceived, unreadMsgs);
		yearList = db->msgsRcvdYears(dbId);
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
		yearList = db->msgsSntYears(dbId);
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
}


/* ========================================================================= */
/*
 * Creates and sends new message.
 */
void MainWindow::createAndSendMessage(void)
/* ========================================================================= */
{
	debug_func_call();

	/*
	 * TODO -- This method copies createAndSendMessageReply().
	 * Delete one of them.
	 */
	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

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

	if (!isdsSessions.isConnectToIsds(userName)) {
		if (!connectToIsds(index, true)) {
			/* TODO - dialog to inform user about error */

			return;
		}
	}

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, dbId,
	    DlgSendMessage::ACT_NEW, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(),
	    dbType, dbEffectiveOVM, dbOpenAddressing, this);
	if (newMessageDialog->exec() == QDialog::Accepted) {
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
			if (!connectToIsds(index, true)) {
				/* TODO */
				//return Q_CONNECT_ERROR;
			}
		}

		/* Messages counters total/news are returned from worker */
		int total = 0, news = 0;
		Worker::downloadMessageList(index, "sent", *messageDb,
		    QString(), m_statusProgressBar, NULL, total, news);
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
	debug_func_call();

	QDialog *newAccountDialog = new DlgCreateAccount(*(ui->accountList),
	   m_accountDb, QModelIndex(), DlgCreateAccount::ACT_ADDNEW, this);

	connect(newAccountDialog, SIGNAL(getAccountUserDataboxInfo(
	    QModelIndex, bool)),
	    this, SLOT(getOwnerInfoFromLogin(const QModelIndex, bool)));

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
	debug_func_call();

	const QModelIndex index = ui->accountList->currentIndex();
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	QStandardItem *itemTop = AccountModel::itemTop(item);
	int currentTopRow = itemTop->row();

	if (currentTopRow < 0) {
		return;
	}

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this,
	    tr("Remove account ") + itemTop->text(),
	    tr("Do you want to remove account") +  " '" + itemTop->text() +
	    "'?",
	    QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {

		QString userName = accountUserName();
		const AccountModel::SettingsMap &itemSettings =
		    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		QString dbDir = itemSettings[DB_DIR].toString();

		if (itemTop->hasChildren()) {
			itemTop->removeRows(0, itemTop->rowCount());
		}
		m_accountDb.deleteAccountInfo(userName + "___True");

		ui->accountList->model()->removeRow(currentTopRow);

		/* Delete message db from disk. */
		MessageDb *db;
		db = m_messageDbs.accessMessageDb(userName, dbDir,
		    itemSettings[TEST].toBool());
		Q_ASSERT(0 != db);
		m_messageDbs.deleteMessageDb(db);

		/* Save changed configuration. */
		saveSettings();
	}

	showStatusText(tr("Account \"%1\" was deleted...").arg(itemTop->text()));

	if (ui->accountList->model()->rowCount() < 1) {
		defaultUiMainWindowSettings();
	}
}


/* ========================================================================= */
/*
 * Delete new account if is not possilbe connect to isds
 */
bool MainWindow::deleteNewAccount(const QModelIndex acntTopIdx)
/* ========================================================================= */
{
	debug_func_call();

	QStandardItem *item = m_accountModel.itemFromIndex(acntTopIdx);
	QStandardItem *itemTop = AccountModel::itemTop(item);
	int currentTopRow = itemTop->row();

	if (currentTopRow < 0) {
		return false;
	}

	QString userName = accountUserName();
	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	QString dbDir = itemSettings[DB_DIR].toString();

	if (itemTop->hasChildren()) {
		itemTop->removeRows(0, itemTop->rowCount());
	}

	m_accountDb.deleteAccountInfo(userName + "___True");

	ui->accountList->model()->removeRow(currentTopRow);

	/* Delete message db from disk. */
	MessageDb *db;
	db = m_messageDbs.accessMessageDb(userName, dbDir,
	    itemSettings[TEST].toBool());
	Q_ASSERT(0 != db);
	m_messageDbs.deleteMessageDb(db);

	/* Save changed configuration. */
	saveSettings();

	if (ui->accountList->model()->rowCount() < 1) {
		defaultUiMainWindowSettings();
	}

	return true;
}


/* ========================================================================= */
/*
 * Shows change password dialog.
 */
void MainWindow::changeAccountPassword(void)
/* ========================================================================= */
{
	debug_func_call();

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!connectToIsds(index, true)) {
			return;
		}
	}

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
	debug_func_call();

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);
	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

//	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
//		if (!connectToIsds(index, true)) {
//			//return;
//		}
//	}

	QDialog *editAccountDialog = new DlgCreateAccount(*(ui->accountList),
	   m_accountDb, QModelIndex(), DlgCreateAccount::ACT_EDIT, this);
	//editAccountDialog->exec();

	if (QDialog::Accepted == editAccountDialog->exec()) {
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
	debug_func_call();

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
}

/* ========================================================================= */
/*
 * Move selected account down.
 */
void MainWindow::moveSelectedAccountDown(void)
/* ========================================================================= */
{
	debug_func_call();

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
}


/* ========================================================================= */
/*
 * Change data directory dialog.
 */
void MainWindow::changeDataDirectory(void)
/* ========================================================================= */
{
	debug_func_call();

	const QModelIndex index = ui->accountList->currentIndex();
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	QStandardItem *itemTop = AccountModel::itemTop(item);

	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString dbDir = itemSettings[DB_DIR].toString();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}

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
	debug_func_call();

	const QModelIndex index = ui->accountList->currentIndex();
	QStandardItem *item = m_accountModel.itemFromIndex(index);

	/* Get current settings. */
	AccountModel::SettingsMap itemSettings =
	    m_accountModel.settingsMap(item);

	QString fileName;

	/* 1 = is test account, 0 = is legal account */
	if (itemSettings[TEST].toBool()) {
		fileName = itemSettings[USER].toString() + "___1.db";
	} else {
		fileName = itemSettings[USER].toString() + "___0.db";
	}

	MessageDb *messageDb = m_messageDbs.accessMessageDb(
	    itemSettings[USER].toString(), itemSettings[DB_DIR].toString(),
	    itemSettings[TEST].toBool());
	Q_ASSERT(0 != messageDb);

	qDebug() << fileName << action;

	/* Move account database into new directory */
	if ("move" == action) {
		if (m_messageDbs.moveMessageDb(messageDb, newDir)) {

			itemSettings.setDirectory(newDir);
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

			itemSettings.setDirectory(newDir);
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

			itemSettings.setDirectory(newDir);
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
	debug_func_call();

	/*
	 * TODO -- This method copies createAndSendMessage().
	 * Delete one of them.
	 */

	const QAbstractItemModel *tableModel = ui->messageList->model();
	Q_ASSERT(0 != tableModel);
	QModelIndex index = tableModel->index(
	    ui->messageList->currentIndex().row(), 0); /* First column. */
	/* TODO -- Reimplement this construction. */

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	QVector<QString> replyTo = messageDb->msgsReplyDataTo(
	    tableModel->itemData(index).first().toInt());

	/* TODO */
	index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

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

	if (!isdsSessions.isConnectToIsds(userName)) {
		if (!connectToIsds(index, true)) {
			return;
		}
	}

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, dbId,
	    DlgSendMessage::ACT_REPLY, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(),
	    dbType, dbEffectiveOVM, dbOpenAddressing, this,
	    replyTo[0], replyTo[1], replyTo[2], replyTo[3]);
	if (newMessageDialog->exec() == QDialog::Accepted) {
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
			if (!connectToIsds(index, true)) {
				/* TODO */
				//return Q_CONNECT_ERROR;
			}
		}

		/* Messages counters total/news are returned from worker */
		int total = 0, news = 0;
		Worker::downloadMessageList(index, "sent", *messageDb,
		    QString(), m_statusProgressBar, NULL, total, news);
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
	debug_func_call();

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	const AccountModel::SettingsMap accountInfo =
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
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
	debug_func_call();

	m_searchLine->clear();
}


/* ========================================================================= */
/*
* Message filter
 */
void MainWindow::filterMessages(const QString &text)
/* ========================================================================= */
{
	debug_func_call();

	QAbstractItemModel *tableModel = m_messageModel;
	if (0 == tableModel) {
		return;
	}

	if (tableModel != &m_messageListProxyModel) { /* TODO -- UNNECESSARY */
		/* Prohibit the proxy model to be the source of itself. */
		m_messageListProxyModel.setSourceModel(tableModel);
	}
	ui->messageList->setModel(&m_messageListProxyModel);

	m_messageListProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));
	/* Filter according to second column. */
	m_messageListProxyModel.setFilterKeyColumn(1);
}


/* ========================================================================= */
/*
* Set received message column widths and sort order.
 */
void MainWindow::setReciveidColumnWidths(void)
/* ========================================================================= */
{
	ui->messageList->resizeColumnToContents(0);
	ui->messageList->setColumnWidth(1, m_received_1);
	ui->messageList->setColumnWidth(2, m_received_2);
	ui->messageList->resizeColumnToContents(3);
	ui->messageList->resizeColumnToContents(4);
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
	debug_func_call();

	ui->messageList->resizeColumnToContents(0);
	ui->messageList->setColumnWidth(1, m_sent_1);
	ui->messageList->setColumnWidth(2, m_sent_2);
	ui->messageList->resizeColumnToContents(3);
	ui->messageList->resizeColumnToContents(4);
	ui->messageList->resizeColumnToContents(5);
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
	debug_func_call();

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
void MainWindow::onTableColumnSort(int column)
/* ========================================================================= */
{
	debug_func_call();

	m_sort_column = column;
	if (ui->messageList->horizontalHeader()->sortIndicatorOrder()
	    == Qt::AscendingOrder) {
		m_sort_order = "SORT_ASCENDING";
	} else if (ui->messageList->horizontalHeader()->sortIndicatorOrder()
	    == Qt::DescendingOrder) {
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
	debug_func_call();

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
	debug_func_call();

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
	debug_func_call();

	/* Redraw views' content. */
	regenerateAccountModelYears(acntTopIdx);
	/*
	 * Force repaint.
	 * TODO -- A better solution?
	 */
	ui->accountList->repaint();
	accountItemSelectionChanged(ui->accountList->currentIndex());
}


/* ========================================================================= */
/*
* Set ProgressBar value and Status bar text.
*/
void MainWindow::setProgressBarFromWorker(QString label, int value)
/* ========================================================================= */
{
	m_statusProgressBar->setFormat(label);
	m_statusProgressBar->setValue(value);
	m_statusProgressBar->repaint();
}


/* ========================================================================= */
/*
* Delete worker and thread objects, enable buttons and set timer
*/
void MainWindow::deleteThreadSyncAll(void)
/* ========================================================================= */
{
	debug_func_call();

	int accountCount = ui->accountList->model()->rowCount();
	if (accountCount > 0) {
		ui->actionSync_all_accounts->setEnabled(true);
		ui->actionReceived_all->setEnabled(true);
		ui->actionDownload_messages->setEnabled(true);
		ui->actionGet_messages->setEnabled(true);
	}

	delete workerSyncAll;
	delete threadSyncAll;

	if (globPref.download_on_background) {
		timer->start(timeout);
	}

	qDebug() << "Delete Worker and Thread objects";
}


/* ========================================================================= */
/*
* Delete worker and thread objects, enable buttons
*/
void MainWindow::deleteThreadSyncOne(void)
/* ========================================================================= */
{
	debug_func_call();

	int accountCount = ui->accountList->model()->rowCount();
	if (accountCount > 0) {
		ui->actionSync_all_accounts->setEnabled(true);
		ui->actionReceived_all->setEnabled(true);
		ui->actionDownload_messages->setEnabled(true);
		ui->actionGet_messages->setEnabled(true);
	}

	delete workerSyncOne;
	delete threadSyncOne;

	qDebug() << "Delete Worker and Thread objects";
}


/* ========================================================================= */
/*
* Delete worker and thread objects, enable buttons
*/
void MainWindow::deleteThreadDownMsgComplete(void)
/* ========================================================================= */
{
	debug_func_call();

	int accountCount = ui->accountList->model()->rowCount();
	if (accountCount > 0) {
		ui->actionSync_all_accounts->setEnabled(true);
		ui->actionReceived_all->setEnabled(true);
		ui->actionDownload_messages->setEnabled(true);
		ui->actionGet_messages->setEnabled(true);
	}

	delete workerDownMsgComplete;
	delete threadDownMsgComplete;

	qDebug() << "Delete Worker and Thread objects";
}


/* ========================================================================= */
/*
 * Verify message. Compare hash with hash stored in ISDS.
 */
qdatovka_error MainWindow::verifySelectedMessage(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	debug_func_call();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
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
	int dmID = atoi(dmId.toStdString().c_str());

	QList<QString> hashLocaldata;
	hashLocaldata = messageDb->msgsGetHashFromDb(dmID);

	/* TODO - check if hash info is in db */
	if (hashLocaldata[0].isEmpty()) {
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
* Delete message from long term storage in ISDS.
*/
qdatovka_error MainWindow::eraseMessage(const QModelIndex &acntTopIdx,
    QString dmId)
/* ========================================================================= */
{
	debug_func_call();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!connectToIsds(acntTopIdx, true)) {
			return Q_CONNECT_ERROR;
		}
	}

	bool incoming = true;

	QModelIndex index = ui->accountList->selectionModel()->currentIndex();
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

	status = isds_delete_message_from_storage(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), incoming);

	if (IE_SUCCESS == status) {
		MessageDb *messageDb = accountMessageDb(0);
		int dmID = atoi(dmId.toStdString().c_str());
		if (messageDb->msgsDeleteMessageData(dmID)) {
			qDebug() << "Message" << dmID <<
			    "was deleted from ISDS and db";
			showStatusText(tr("Message %1 was deleted from "
			    "ISDS and db").arg(dmID));
			return Q_SUCCESS;
		} else {
			qDebug() << "Message" << dmID <<
			    "was deleted from ISDS and db";
			return Q_SQL_ERROR;
		}
	} else if (IE_INVAL == status) {
		qDebug() << "Error: " << status << isds_strerror(status);
		return Q_ISDS_ERROR;
	} else {
		return Q_ISDS_ERROR;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
 * Get data about logged in user and his box.
 */
bool MainWindow::getOwnerInfoFromLogin(const QModelIndex &acntTopIdx, bool add)
/* ========================================================================= */
{
	debug_func_call();

	if (!add) {
		return false;
	}

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!connectToIsds(acntTopIdx, false)) {

			QString msgBoxTitle = accountInfo.accountName() +
			    ": " + tr("Account error");
			QString msgBoxContent =
			    tr("It was not possible to get user info and "
			    "databox info for this account from server.")
			    +"<br><br><b>" +
			    tr("Connection to ISDS or user authentication failed!")
			    + "</b><br><br>" +
			    tr("Please check your internet connection and "
			    "try again or it is possible that your password "
			    "(certificate) has expired - in this case, you "
			    "need to use the official web interface of Datové "
			    "schránky to change it.")
			    + "<br><br><b>" + tr("The account \"") +
			    accountInfo.accountName() + "\" ("
			    + accountInfo.userName() + ")"+
			    tr(" was not created!") + "</b>";

			deleteNewAccount(acntTopIdx);

			QMessageBox::critical(this,
			    msgBoxTitle,
			    msgBoxContent,
			    QMessageBox::Ok);

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
	debug_func_call();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
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
	debug_func_call();

	QDialog *abDialog = new aboutDialog(this);
	abDialog->exec();
}


/* ========================================================================= */
/*
 * Import database directory dialog.
 */
void MainWindow::importDatabaseDirectory(void)
/* ========================================================================= */
{
	debug_func_call();

	QString importDir = QFileDialog::getExistingDirectory(this,
	    tr("Import database directory"), m_on_import_database_dir_activate,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (importDir.isEmpty() || importDir.isNull()) {
		showStatusText(tr("Import database directory was canceled..."));
		return;
	}

	m_on_import_database_dir_activate = importDir;

	qDebug() << "New database files path:" << importDir;

	QStringList nameFilter("*.db");
	QDir directory(importDir);
	QStringList dbFiles = directory.entryList(nameFilter);

	if (dbFiles.isEmpty()) {
		qDebug() << "No *.db files in the path" << importDir;
		showStatusText(tr("Database file not found in selected "
		    "directory..."));
		return;
	}

	QStringList fileNameParts;
	// List of test accounts
	QList<QString> testAccounts;
	// list of legal/legitimate accounts
	QList<QString> legalAccounts;
	QString accountName;

	for (int i = 0; i < dbFiles.size(); ++i) {
		if (dbFiles.at(i).contains("___")) {
			fileNameParts = dbFiles.at(i).split("___");
			accountName = fileNameParts[0];
			fileNameParts = fileNameParts[1].split(".");
			if (fileNameParts[0] == "1") {
				testAccounts.append(accountName);
			} else {
				legalAccounts.append(accountName);
			}
		}
	}

	if (testAccounts.empty() && legalAccounts.empty()) {
		qDebug() << "No valid account database file(s) in the path"
		    << importDir;
		showStatusText(tr("Import failed. No valid db file in "
		    "directory..."));
		return;
	}

	AccountModel::SettingsMap itemSettings;

	QStringList currentAccountList;
	int accountCount = ui->accountList->model()->rowCount();
	for (int i = 0; i < accountCount; i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		currentAccountList.append(accountInfo.userName());
	}

	int totalAccounts = 0;

	for (int i = 0; i < legalAccounts.size(); ++i) {
		accountName = legalAccounts.at(i);
		bool isInAccountList = false;
		for (int j = 0; j < currentAccountList.size(); ++j) {
			if (currentAccountList.at(j) == accountName) {
				isInAccountList = true;
				break;
			}
		}

		if (!isInAccountList) {
			itemSettings[NAME] = accountName;
			itemSettings[USER] = accountName;
			itemSettings[LOGIN] = "username";
			itemSettings[PWD] = "";
			itemSettings[REMEMBER] = true;
			itemSettings[TEST] = false;
			itemSettings[SYNC] = true;
			itemSettings[DB_DIR] = importDir;
			m_accountModel.addAccount(accountName, itemSettings);
			totalAccounts++;
			qDebug() << "Found legal account:" << accountName <<
			"...was added";
		} else {
			qDebug() << "Found legal account:" << accountName <<
			    "...already exists";
		}
	}

	for (int i = 0; i < testAccounts.size(); ++i) {

		accountName = testAccounts.at(i);
		bool isInAccountList = false;
		for (int j = 0; j < currentAccountList.size(); ++j) {
			if (currentAccountList.at(j) == accountName) {
				isInAccountList = true;
				break;
			}
		}

		if (!isInAccountList) {
			itemSettings[NAME] = accountName;
			itemSettings[USER] = accountName;
			itemSettings[LOGIN] = "username";
			itemSettings[PWD] = "";
			itemSettings[REMEMBER] = true;
			itemSettings[TEST] = true;
			itemSettings[SYNC] = true;
			itemSettings[DB_DIR] = importDir;
			m_accountModel.addAccount(accountName, itemSettings);
			totalAccounts++;
			qDebug() << "Found test account:" << accountName <<
			"...was added";
		} else {
			qDebug() << "Found test account:" << accountName <<
			    "...already exists";
		}
	}

	qDebug() << totalAccounts <<
	    "database file(s) was/were imported from path" << importDir;

	if (totalAccounts > 1) {
		showStatusText(tr("%1 database files were imported...")
		    .arg(totalAccounts));
	} else {
		showStatusText(tr("%1 database file was imported...")
		    .arg(totalAccounts));
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
	debug_func_call();

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

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
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
	debug_func_call();

	switch (authenticateMessageFromZFO()) {
	case Q_SUCCESS:
		showStatusText(tr("ISDS confirms that the message is valid..."));
		QMessageBox::information(this, tr("Message is authentic"),
		    tr("ISDS confirms that the message is valid."),
		    QMessageBox::Ok);
		break;
	case Q_NOTEQUAL:
		showStatusText(tr("ISDS confirms that the message is invalid..."));
		QMessageBox::warning(this, tr("Message is not authentic"),
		    tr("ISDS confirms that the message is invalid."),
		    QMessageBox::Ok);
		break;
	case Q_ISDS_ERROR:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::critical(this, tr("Message authentication error"),
		    tr("Authentication of message has been stopped because "
		    "the connection to ISDS failed!\nCheck your internet "
		    "connection."),
		    QMessageBox::Ok);
		break;
	case Q_CONNECT_ERROR:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::critical(this, tr("Message authentication error"),
		    tr("Authentication of message has been stopped because the "
		    "connection to ISDS failed!\nCheck your internet "
		    "connection."),
		    QMessageBox::Ok);
		break;
	case Q_FILE_ERROR:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::critical(this, tr("Message authentication error"),
		    tr("Authentication of message has been stopped because "
		    "the message file has wrong format!"),
		    QMessageBox::Ok);
		break;
	case Q_CANCEL:
		break;
	default:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::critical(this, tr("Message authentication error"),
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
	debug_func_call();

	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	switch (verifySelectedMessage(acntTopIdx, msgIdx)) {
	case Q_SUCCESS:
		showStatusText(tr("ISDS confirms that the message is valid..."));
		QMessageBox::information(this, tr("Message is valid"),
		    tr("Hash of message corresponds to ISDS message "
		    "hash.\nMessage is valid."),
		    QMessageBox::Ok);
		break;
	case Q_NOTEQUAL:
		showStatusText(tr("ISDS confirms that the message is invalid..."));
		QMessageBox::warning(this, tr("Message is not authentic"),
		    tr("ISDS confirms that the message hash is "
		    "invalid!\nMessage is invalid."),
		    QMessageBox::Ok);
		break;
	case Q_ISDS_ERROR:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::warning(this, tr("Authenticate message error"),
		    tr("The message hash cannot be verified because the "
		    "connection to ISDS failed!\nCheck your internet "
		    "connection."),
		    QMessageBox::Ok);
		break;
	case Q_SQL_ERROR:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::warning(this, tr("Authenticate message warning"),
		    tr("The message hash is not in local database.\nPlease "
		    "download complete message form ISDS and try again."),
		    QMessageBox::Ok);
		break;
	case Q_GLOBAL_ERROR:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::critical(this, tr("Authenticate message error"),
		    tr("The message hash cannot be verified because an internal"
		    " error occurred!\nTry again."),
		    QMessageBox::Ok);
		break;
	default:
		showStatusText(tr("Message authentication failed..."));
		QMessageBox::critical(this, tr("Authenticate message error"),
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
	debug_func_call();

	struct isds_ctx *dummy_session = NULL; /* Logging purposes. */
	struct isds_message *message = NULL;
	QDialog *viewDialog;
	QDir dirPath;

	QString fileName = QFileDialog::getOpenFileName(this,
	    tr("Add ZFO file"), m_on_export_zfo_activate,
	    tr("ZFO file (*.zfo)"));

	if (fileName.isEmpty()) {
		goto fail;
	}

	dirPath = QFileInfo(fileName).absoluteDir();
	m_on_export_zfo_activate = dirPath.absolutePath();

	dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		qDebug() << "Cannot create dummy ISDS session.";
		
	}

	message = loadZfoFile(dummy_session, fileName);
	if (NULL == message) {
		qDebug() << "Cannot parse file" << fileName;
		QMessageBox::warning(this,
		    tr("Content parsing error"),
		    tr("Cannot parse the content of ") + fileName + ".",
		    QMessageBox::Ok);
		goto fail;
	}

	/* Generate dialog showing message content. */
	viewDialog = new DlgViewZfo(message, this);
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
	debug_func_call();

	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QDialog *correspondence_overview = new DlgCorrespondenceOverview(
	    *messageDb, dbId, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(),
	    m_export_correspond_dir,
	    this);

	connect(correspondence_overview,
	    SIGNAL(showNotificationDialog(QList<QString>, int)), this,
	    SLOT(setAndShowNotificationDialog(QList<QString>, int)));

	correspondence_overview->exec();
}

/* ========================================================================= */
/*
 * Show help.
 */
void MainWindow::showHelp(void)
/* ========================================================================= */
{
	debug_func_call();
	/* TODO - load help content from html file to browser */
}


/* ========================================================================= */
/*
 * Export message into as ZFO file dialog.
 */
void MainWindow::exportSelectedMessageAsZFO(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusText(tr("Export of message to "
		    "ZFO was not successful!"));
		return;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(messageDb->msgsGetMessageRaw(dmID)).toUtf8();
	if (raw.isEmpty()) {

		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Message export error!"));
		msgBox.setText(tr("Can not export complete message.")
		    + " " + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("You must download message firstly before export.") +
		    "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId)) {
				showStatusText(tr("Export of message "
				"\"%1\" to ZFO was not successful!")
				.arg(dmId));
				return;
			} else {
				raw = QString(messageDb->
				     msgsGetMessageRaw(dmID)).toUtf8();
			}
		} else {
			showStatusText(tr("Export of message "
			"\"%1\" to ZFO was not successful!").arg(dmId));
			return;
		}
	}

	QDir dirPath;

	QString fileName = m_on_export_zfo_activate + "/" +
	    "DDZ_" + dmId + ".zfo";

	Q_ASSERT(!fileName.isEmpty());

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save message as ZFO file"), fileName);

	if (fileName.isEmpty()) {
		showStatusText(tr("Export of message \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	dirPath = QFileInfo(fileName).absoluteDir();
	m_on_export_zfo_activate = dirPath.absolutePath();

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		showStatusText(tr("Export of message \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
		return;
	}

	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {
		showStatusText(tr("Export of message \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
	} else {
		showStatusText(tr("Export of message \"%1\" to "
		    "ZFO was successful...").arg(dmId));
	}

	fout.close();
}


/* ========================================================================= */
/*
 * Download complete message synchronously without worker and thread
 */
bool MainWindow::downloadCompleteMessage(QString dmId)
/* ========================================================================= */
{
	QModelIndex accountIndex = ui->accountList->currentIndex();
	Q_ASSERT(accountIndex.isValid());
	accountIndex = AccountModel::indexTop(accountIndex);
	    /* selection().indexes() ? */

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	bool incoming = true;

	QModelIndex index = ui->accountList->selectionModel()->currentIndex();
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

	const AccountModel::SettingsMap accountInfo =
	    accountIndex.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!connectToIsds(accountIndex, true)) {
			return false;
		}
	}

	if (Q_SUCCESS == Worker::downloadMessage(
	    accountIndex, dmId, true, incoming, *messageDb, false, QString(),
	    0, 0)) {
		/* TODO -- Wouldn't it be better with selection changed?*/
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
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusText(tr("Export of message delivery info to "
		    "ZFO was not successful!"));
		return;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(messageDb->msgsGetDeliveryInfoRaw(dmID)).toUtf8();
	if (raw.isEmpty()) {

		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Delivery info export error!"));
		msgBox.setText(tr("Can not export delivery info for message.")
		    + " " + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		    tr("You must download message firstly before export.") +
		    "\n\n" +
		    tr("Do you want to download complete message now?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if (QMessageBox::Yes == msgBox.exec()) {
			if (!downloadCompleteMessage(dmId)) {
				showStatusText(tr("Export of message delivery"
				" info \"%1\" to ZFO was not successful!")
				.arg(dmId));
				return;
			} else {
				raw = QString(messageDb->
				     msgsGetDeliveryInfoRaw(dmID)).toUtf8();
			}
		} else {
			showStatusText(tr("Export of message delivery "
			"info \"%1\" to ZFO was not successful!").arg(dmId));
			return;
		}
	}

	QDir dirPath;

	QString fileName = m_on_export_zfo_activate + "/" +
	    "DDZ_" + dmId + "_info.zfo";

	Q_ASSERT(!fileName.isEmpty());

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save delivery info as ZFO file"), fileName);

	if (fileName.isEmpty()) {
		showStatusText(tr("Export of message delivery info \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	dirPath = QFileInfo(fileName).absoluteDir();
	m_on_export_zfo_activate = dirPath.absolutePath();

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		showStatusText(tr("Export of message delivery info \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
		return;
	}

	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {
		showStatusText(tr("Export of message delivery info \"%1\" to "
		    "ZFO was not successful!").arg(dmId));
	} else {
		showStatusText(tr("Export of message delivery info \"%1\" to "
		    "ZFO was successful...").arg(dmId));
	}

	fout.close();
}


/* ========================================================================= */
/*
 * Export delivery information as PDF file dialog.
 */
void MainWindow::exportDeliveryInfoAsPDF(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusText(tr("Export of message devilery info to "
		    "PDF was not successful!"));
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	int dmID = atoi(dmId.toStdString().c_str());

	QDir dirPath;

	QString fileName = m_on_export_zfo_activate + "/" +
	    "DD_" + dmId + ".pdf";

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save delivery info as PDF file"), fileName);

	if (fileName.isEmpty()) {
		showStatusText(tr("Export of message devilery info \"%1\" to "
		    "PDF was not successful!").arg(dmId));
	}

	/* remember path for settings */
	dirPath = QFileInfo(fileName).absoluteDir();
	m_on_export_zfo_activate = dirPath.absolutePath();

	MessageDb *messageDb = accountMessageDb(0);

	QTextDocument doc;
	doc.setHtml(messageDb->deliveryInfoHtmlToPdf(dmID));

	/* TODO - Slow printer initialization */

	QDialog pdf_dialog(this);
	pdf_dialog.setModal(false);
	pdf_dialog.setWindowTitle(tr("PDF printing"));
	pdf_dialog.show();

	QPrinter printer;
	printer.setOutputFileName(fileName);
	printer.setOutputFormat(QPrinter::PdfFormat);
	doc.print(&printer);
	pdf_dialog.close();

	showStatusText(tr("Export of message devilery info \"%1\" to "
	    "PDF was successful...").arg(dmId));
}


/* ========================================================================= */
/*
 * Export selected message envelope as PDF file dialog.
 */
void MainWindow::exportMessageEnvelopeAsPDF(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		showStatusText(tr("Export of message envelope to "
		    "PDF was not successful!"));
		return;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	int dmID = atoi(dmId.toStdString().c_str());

	QDir dirPath;

	QString fileName = m_on_export_zfo_activate + "/" +
	    "OZ_" + dmId + ".pdf";

	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save message envelope as PDF file"), fileName);

	if (fileName.isEmpty()) {
		showStatusText(tr("Export of message envelope \"%1\" to "
		    "PDF was not successful!").arg(dmId));
		return;
	}

	/* remember path for settings */
	dirPath = QFileInfo(fileName).absoluteDir();
	m_on_export_zfo_activate = dirPath.absolutePath();

	MessageDb *messageDb = accountMessageDb(0);
	QString userName = accountUserName();
	QList<QString> accountData =
	    m_accountDb.getUserDataboxInfo(userName + "___True");

	if (accountData.isEmpty()) {
		showStatusText(tr("Export of message envelope \"%1\" to "
		    "PDF was not successful!").arg(dmId));
		return;
	}

	QTextDocument doc;
	doc.setHtml(messageDb->envelopeInfoHtmlToPdf(dmID, accountData.at(0)));

	/* TODO - Slow printer initialization */

	QDialog pdf_dialog(this);
	pdf_dialog.setModal(false);
	pdf_dialog.setWindowTitle(tr("PDF printing"));
	pdf_dialog.show();

	QPrinter printer;
	printer.setOutputFileName(fileName);
	printer.setOutputFormat(QPrinter::PdfFormat);
	doc.print(&printer);
	pdf_dialog.close();

	showStatusText(tr("Export of message envelope \"%1\" to "
	    "PDF was successful...").arg(dmId));
}


/* ========================================================================= */
/*
 * Open selected message in external application.
 */
void MainWindow::openSelectedMessageExternally(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(messageDb->msgsGetMessageRaw(dmID)).toUtf8();
	if (raw.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Datovka - Export error!"));
		msgBox.setText(tr("Can not export the message ") + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		  tr("You must download message firstly before its export..."));
		msgBox.exec();
		return;
	}

	QString fileName = TMP_ATTACHMENT_PREFIX "DDZ_" + dmId + ".zfo";
	Q_ASSERT(!fileName.isEmpty());

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

	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {

	}

	fout.close();

	QDesktopServices::openUrl(QUrl("file://" + fileName));
}


/* ========================================================================= */
/*
 * Open delivery information externally.
 */
void MainWindow::openDeliveryInfoExternally(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(messageDb->msgsGetMessageRaw(dmID)).toUtf8();
	if (raw.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Datovka - Export error!"));
		msgBox.setText(tr("Can not export the message ") + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		  tr("You must download message firstly before its export..."));
		msgBox.exec();
		return;
	}

	QString fileName = TMP_ATTACHMENT_PREFIX "DDZ_" + dmId + "_info.zfo";
	Q_ASSERT(!fileName.isEmpty());

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

	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {

	}

	fout.close();

	QDesktopServices::openUrl(QUrl("file://" + fileName));
}


/* ========================================================================= */
/*
 * View signature details.
 */
void MainWindow::showSignatureDetails(void)
/* ========================================================================= */
{
	debug_func_call();

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);
	if (0 == messageDb) {
		return;
	}
	int dmID = atoi(dmId.toStdString().c_str());

	QDialog *signature_detail = new DlgSignatureDetail(*messageDb, dmID,
	    this);
	signature_detail->exec();
}


/* ========================================================================= */
/*
* This is call if connection to ISDS fails. Message info for user is generated.
*/
void MainWindow::showConnectionErrorMessageBox(int status, QString accountName)
/* ========================================================================= */
{
	QString msgBoxTitle = "";
	QString msgBoxContent = "";

	switch(status) {
	case IE_NOT_LOGGED_IN:
		msgBoxTitle = accountName + ": " + tr("Authentication error!");
		msgBoxContent =
		    tr("It was not possible to connect to your Databox.") + "<br><br>" +
		    "<b>" + tr("Authorization failed!") + "</b>" + "<br><br>" +
		    tr("Please check your credentials including the test-"
		        "environment setting.") + "<br>" +
		    tr("It is possible that your password has expired - "
		        "in this case, you need to use the official web "
		        "interface of Datové schránky to change it.");
		break;

	case IE_PARTIAL_SUCCESS:
		msgBoxTitle = accountName +
		    ": " + tr("OTP authentication error!");
		msgBoxContent =
		    tr("It was not possible to connect to your Databox.") + "<br><br>" +
		    "<b>" + tr("Authorization via OTP failed!") + "</b>" + "<br><br>" +
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
		    "<b>" + tr("Connection to ISDS timeout!") + "</b>" + "<br><br>" +
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
		    tr("This is usually caused by either lack of internet "
		    "connection or by a firewall on the way.") + "<br>" +
		    tr("Please check your internet connection and try again.")
		    + "<br><br>" + tr("It might be necessary to use a proxy to "
		    "connect to the server. If yes, please set it up in the "
		    "File/Proxy settings menu.");
		break;
	}

	showStatusText(tr("It was not possible to connect to your"
	    " databox form account \"%1\"...").arg(accountName));

	QMessageBox::critical(this, msgBoxTitle, msgBoxContent, QMessageBox::Ok);
}


/* ========================================================================= */
/*
* Check if connection to ISDS fails.
*/
bool MainWindow::checkConnectionError(int status, QString accountName,
    bool showDialog)
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
			showConnectionErrorMessageBox(status, accountName);
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

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName());
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
			return false;
		}
	}

	status = isdsLoginUserName(isdsSessions.session(accountInfo.userName()),
	    accountInfo.userName(), pwd, accountInfo.testAccount());

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog);
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

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName());
	}

	QString certPath = accountInfo.certPath();
	if (certPath.isNull() || certPath.isEmpty()) {
		QDialog *editAccountDialog = new DlgCreateAccount(
		    *(ui->accountList), m_accountDb, acntTopIdx,
		    DlgCreateAccount::ACT_CERT, this);
		if (QDialog::Accepted == editAccountDialog->exec()) {
			const AccountModel::SettingsMap accountInfoNew =
			    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			certPath = accountInfoNew.certPath();
			saveSettings();
		} else {
			return false;
		}
	}

	status = isdsLoginSystemCert(
	    isdsSessions.session(accountInfo.userName()),
	    certPath, accountInfo.testAccount());

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog);
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

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName());
	}

	QString certPath = accountInfo.certPath();
	QString pwd = accountInfo.password();

	if (pwd.isNull() || pwd.isEmpty() ||
	    certPath.isNull() || certPath.isEmpty()) {

		QDialog *editAccountDialog = new DlgCreateAccount(
		    *(ui->accountList), m_accountDb, acntTopIdx,
		    DlgCreateAccount::ACT_CERTPWD, this);
		if (QDialog::Accepted == editAccountDialog->exec()) {
			const AccountModel::SettingsMap accountInfoNew =
			    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			certPath = accountInfoNew.certPath();
			pwd = accountInfoNew.password();
			saveSettings();
		} else {
			return false;
		}
	}

	status = isdsLoginUserCertPwd(
	    isdsSessions.session(accountInfo.userName()),
	    accountInfo.userName(), pwd, certPath, accountInfo.testAccount());


	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog);
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

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName());
	}

	QString certPath = accountInfo.certPath();
	QString idBox;

	QDialog *editAccountDialog = new DlgCreateAccount(
	    *(ui->accountList), m_accountDb, acntTopIdx,
	    DlgCreateAccount::ACT_IDBOX, this);
	if (QDialog::Accepted == editAccountDialog->exec()) {
		const AccountModel::SettingsMap accountInfoNew =
		    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
		certPath = accountInfoNew.certPath();
		idBox = accountInfoNew.userName();
		saveSettings();
	} else {
		return false;
	}

	status = isdsLoginUserCert(isdsSessions.session(accountInfo.userName()),
	    idBox, certPath, accountInfo.testAccount());

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog);
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

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		isdsSessions.createCleanSession(accountInfo.userName());
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
			return false;
		}
	}
	/* sent OTP request */
	status = isdsLoginUserOtp(
	    isdsSessions.session(accountInfo.userName()),
	    accountInfo.userName(), pwd,
	    accountInfo.testAccount(),
	    accountInfo.loginMethod(),
	    QString());
	if (!checkConnectionError(status, accountInfo.accountName(), true)) {
		return false;
	}
	bool ok;
	QString code = "";
	while (code.isEmpty()) {
		code = QInputDialog::getText(this,
		    tr("Server connection for ")
		    + accountInfo.accountName(),
		    tr("Enter security code for account ")
		    + accountInfo.accountName()
		    + " (" + accountInfo.userName() + ")",
		    QLineEdit::Password, "", &ok,
		    Qt::WindowStaysOnTopHint);
		if (!ok) {
			return false;
		}
	}

	/* sent OTP security code */
	status = isdsLoginUserOtp(
	    isdsSessions.session(accountInfo.userName()),
	    accountInfo.userName(), pwd,
	    accountInfo.testAccount(), accountInfo.loginMethod(), code);

	return checkConnectionError(status, accountInfo.accountName(),
	    showDialog);
}


/* ========================================================================= */
/*
 * Connect to databox
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
	if (accountInfo.loginMethod() == "username") {
		return loginMethodUserNamePwd(acntTopIdx, accountInfo,
		    showDialog);

	/* Login method based on certificate only */
	} else if (accountInfo.loginMethod() == "certificate") {
		return loginMethodCertificateOnly(acntTopIdx, accountInfo,
		    showDialog);

	/* Login method based on certificate together with username */
	} else if (accountInfo.loginMethod() == "user_certificate") {

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
 * set and show correspondence error dialog
 */
void MainWindow::setAndShowNotificationDialog(QList<QString> errorDmId,
   int successCnt)
/* ========================================================================= */
{
	debug_func_call();

	QString msg;

	msg = tr("There were some errors during saving of the overview:") +
	    "\n\n";

	for (int i = 0; i < errorDmId.count(); ++i) {
		msg += tr("Message") + " " + errorDmId.at(i) + " " +
		   tr("does not contain data necessary for ZFO export") + ".\n";
		if (i > 10) {
			msg += tr("And many more") + "...\n";
			break;
		}
	}

	msg += "\n" + QString::number(successCnt) + " " +
	    tr("messages were successfully exported to ZFO") + ".\n";

	QMessageBox::warning(this,
	    QObject::tr("Correspondence export error"), msg, QMessageBox::Ok);
}
