
#include <cmath> /* ceil(3) */
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QPrinter>
//#include <QPrinterInfo>
#include <QSettings>
#include <QStackedWidget>
#include <QTableView>
#include <QTemporaryFile>
#include "datovka.h"

#include "src/common.h"
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/pkcs7.h"
#include "ui_datovka.h"


#define WIN_POSITION_HEADER "window_position"
#define WIN_POSITION_X "x"
#define WIN_POSITION_Y "y"
#define WIN_POSITION_W "w"
#define WIN_POSITION_H "h"

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
    m_received_1(200),
    m_received_2(200),
    m_sent_1(200),
    m_sent_2(200),
    m_sort_column(0),
    m_sort_order(""),
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
	m_searchLine = new QLineEdit(this);
	connect(m_searchLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterMessages(QString)));
	m_searchLine->setFixedWidth(200);
	ui->toolBar->addWidget(m_searchLine);
	ui->toolBar->addAction(QIcon(ICON_3PARTY_PATH "delete_16.png"),
	    tr("Clear search field"), this,
	    SLOT(on_actionSearchClear_triggered()));

	/* Create status bar label */
	QLabel* statusLabel = new QLabel(this);
	statusLabel->setText(tr("Status:"));
	statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	ui->statusBar->addWidget(statusLabel,1);

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

	/* It fires when any column was resized. */
	connect(ui->messageList->horizontalHeader(),
	    SIGNAL(sectionClicked(int)),
	    this, SLOT(onTableColumnSort(int)));

	/* Message/attachment related buttons. */
	connect(ui->downloadComplete, SIGNAL(clicked()), this,
	    SLOT(downloadSelectedMessageAttachments()));
	connect(ui->saveAttachment, SIGNAL(clicked()), this,
	    SLOT(saveSelectedAttachmentToFile()));
	connect(ui->openAttachment, SIGNAL(clicked()), this,
	    SLOT(openSelectedAttachment()));
//	ui->verifySignature
//	ui->signatureDetails
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
 * Create and open Preferences dialog
 */
void MainWindow::on_actionPreferences_triggered()
/* ========================================================================= */
{
	QDialog *dlgPrefs = new DlgPreferences(this);
	dlgPrefs->exec();
}


/* ========================================================================= */
/*
 * Create and open Proxy settings dialog
 */
void MainWindow::on_actionProxy_settings_triggered()
/* ========================================================================= */
{
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
	(void) previous; /* Unused. */

	QString html;
	DbMsgsTblModel *msgTblMdl;

//	Q_ASSERT(current.isValid());
	if (!current.isValid()) {
		/* May occur on deleting last account. */
		ui->messageList->selectionModel()->disconnect(
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(messageItemSelectionChanged(QModelIndex,
		         QModelIndex)));

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
		getOwnerInfoFromLogin(AccountModel::indexTop(current));
		/* TODO -- What to do when no ISDS connection is present? */
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
		/* Clear message info. */
		ui->messageInfo->clear();
		/* Select last message in list if there are some messages. */
		itemModel = ui->messageList->model();
		/* enable/disable buttons */
		if ((0 != itemModel) && (0 < itemModel->rowCount())) {
			QModelIndex lastIndex =
			    itemModel->index(itemModel->rowCount() - 1, 0);
			ui->messageList->setCurrentIndex(lastIndex);
			ui->actionReply_to_the_sender->setEnabled(true);
			ui->actionVerify_a_message->setEnabled(true);
			ui->menuMessage->setEnabled(true);
			ui->actionAuthenticate_message_file->setEnabled(true);
			ui->actionExport_corespondence_overview->
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
		//accountItemSelectionChanged(index);

		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-account-sync.png"),
		    tr("Get messages"),
		    this, SLOT(on_actionGet_messages_triggered()));
		menu->addAction(QIcon(ICON_16x16_PATH "datovka-message.png"),
		    tr("Create message"),
		    this, SLOT(on_actionCreate_message_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "tick_16.png"),
		    tr("Mark all as read"),
		    this, SLOT(on_actionMark_all_as_read_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "user_16.png"),
		    tr("Change password"),
		    this, SLOT(on_actionChange_password_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "letter_16.png"),
		    tr("Account properties"),
		    this, SLOT(on_actionAccount_properties_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "delete_16.png"),
		    tr("Remove Account"),
		    this, SLOT(on_actionDelete_account_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "up_16.png"),
		    tr("Move account up"),
		    this, SLOT(on_actionMove_account_up_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH "down_16.png"),
		    tr("Move account down"),
		    this, SLOT(on_actionMove_account_down_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH "folder_16.png"),
		    tr("Change data directory"),
		    this, SLOT(on_actionChange_data_directory_triggered()));
	} else {
		menu->addAction(QIcon(ICON_3PARTY_PATH "plus_16.png"),
		    tr("Add new account"),
		    this, SLOT(on_actionAdd_account_triggered()));
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
	ui->downloadComplete->setEnabled(true);
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
		ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId));
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

		/* Connect new slot. */
		connect(ui->messageAttachmentList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(attachmentItemSelectionChanged(QModelIndex,
		        QModelIndex)));
	} else {
		ui->messageInfo->setHtml("");
		ui->messageInfo->setReadOnly(true);
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
	QModelIndex index = ui->messageList->indexAt(point);
	QMenu *menu = new QMenu;

	if (index.isValid()) {
		//messageItemSelectionChanged(index);

		/* TODO */
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-download.png"),
		    tr("Download message signed"), this,
		    SLOT(downloadSelectedMessageAttachments()));
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-reply.png"),
		    tr("Reply on message"), this,
		    SLOT(on_actionReply_to_the_sender_triggered()));
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH "delete_16.png"),
		    tr("Delete message"), this,
		    SLOT(on_actionDelete_message_triggered()))->
		    setEnabled(ui->actionDelete_message->isEnabled());
		menu->addAction(
		    tr("Export message to ZFO"), this,
		    SLOT(on_actionExport_as_ZFO_triggered()))->
		    setEnabled(ui->actionExport_as_ZFO->isEnabled());
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-verify.png"),
		    tr("Verified messages"), this,
		    SLOT(on_actionReply_to_the_sender_triggered()));
		menu->addSeparator();
	} else {
		menu->addAction(QIcon(ICON_16x16_PATH "datovka-message.png"),
		    tr("Create a new message"),
		    this, SLOT(on_actionSent_message_triggered()));
	}
	menu->exec(QCursor::pos());
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
 * Open attachment in default application.
 */
void MainWindow::openSelectedAttachment(void)
/* ========================================================================= */
{
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
	QModelIndex messageIndex =
	    ui->messageList->selectionModel()->currentIndex();
	Q_ASSERT(messageIndex.isValid());
	QModelIndex accountIndex = ui->accountList->currentIndex();
	Q_ASSERT(accountIndex.isValid());
	accountIndex = AccountModel::indexTop(accountIndex);
	    /* selection().indexes() ? */

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

	m_statusProgressBar->setFormat("downloadMessage");
	m_statusProgressBar->repaint();

	/* TODO -- Check return value. */
	downloadMessage(accountIndex, messageIndex, true, incoming);

	setDefaultProgressStatus();

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

	int msgId = messageIndex.sibling(messageIndex.row(), 0).data().toInt();
	//qDebug() << "message index" << msgId;

	/* Show files related to message message. */
	MessageDb *messageDb = accountMessageDb(0);
	Q_ASSERT(0 != messageDb);

	/* Generate and show message information. */
	ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId));
	ui->messageInfo->setReadOnly(true);

	QAbstractTableModel *fileTblMdl = messageDb->flsModel(msgId);
	Q_ASSERT(0 != fileTblMdl);
	//qDebug() << "Setting files";
	ui->messageAttachmentList->setModel(fileTblMdl);
	/* First three columns contain hidden data. */
	ui->messageAttachmentList->setColumnHidden(0, true);
	ui->messageAttachmentList->setColumnHidden(1, true);
	ui->messageAttachmentList->setColumnHidden(2, true);

	/* Connect new slot. */
	connect(ui->messageAttachmentList->selectionModel(),
	    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
	    SLOT(attachmentItemSelectionChanged(QModelIndex,
	        QModelIndex)));
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
				html.append(strongAccountInfoLine(
				    accntinfTbl.attrProps[key].desc,
				    QString::number(
				        accountEntry.value(key).toInt())));
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
	html.append(strongAccountInfoLine(tr("Password expiration date"),
	    m_accountDb.getPwdExpirFromDb(key)));

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
	    tr("QDatovka - Free interface for Datové schránky") + "</h2>";
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
		dbDir = GlobPreferences::confDir();
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
	if (!QDir(GlobPreferences::confDir()).exists()) {
		QDir(GlobPreferences::confDir()).mkpath(".");
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
	int tmp = settings.value("panes/hpaned1", 200).toInt();
	sizes[0] = tmp;
	sizes[1] = w - sizes[0];;
	ui->hSplitterAccount->setSizes(sizes);

	// set messagelistspliter - vSplitterMessage
	sizes = ui->vSplitterMessage->sizes();
	sizes[0] = settings.value("panes/message_pane", 235).toInt();
	sizes[1] = h - SH_OFFS - sizes[0];
	ui->vSplitterMessage->setSizes(sizes);

	// set message/mesageinfospliter - hSplitterMessageInfo
	sizes = ui->hSplitterMessageInfo->sizes();
	sizes[0] = settings.value("panes/message_display_pane", 258).toInt();
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
				ui->actionAccount_properties->setEnabled(true);
				ui->actionChange_password->setEnabled(true);
				ui->actionCreate_message->setEnabled(true);
				ui->actionFind_databox->setEnabled(true);
				ui->actionDownload_messages->setEnabled(true);
				ui->actionRecieved_all->setEnabled(true);
				break;
			}
		}
	} else {

		defaultUiMainWindowSettings();
	}
}

/* ========================================================================= */
/*
 *  Set default settings of mainwindow.
 */
void MainWindow::defaultUiMainWindowSettings(void) const
/* ========================================================================= */
{
	// TopMenu
	ui->menuDatabox->setEnabled(false);
	ui->menuMessage->setEnabled(false);
	// ToolBar
	ui->actionRecieved_all->setEnabled(false);
	ui->actionDownload_messages->setEnabled(false);
	ui->actionCreate_message->setEnabled(false);
	ui->actionReply_to_the_sender->setEnabled(false);
	ui->actionVerify_a_message->setEnabled(false);
	ui->actionAccount_properties->setEnabled(false);
	ui->actionChange_password->setEnabled(false);
	// Menu: File
	ui->actionDelete_account->setEnabled(false);
	ui->actionSync_all_accounts->setEnabled(false);
	// Menu: Tools
	ui->actionFind_databox->setEnabled(false);
	ui->actionAuthenticate_message_file->setEnabled(false);
	ui->actionExport_corespondence_overview->setEnabled(false);
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
	ui->actionRecieved_all->setEnabled(action);
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

	//qDebug() << "All: " << settings.allKeys();
	//qDebug() << "Groups: " << settings.childGroups();
	//qDebug() << "Keys:" << settings.childKeys();

	/* Window geometry. */
	loadWindowGeometry(settings);

	/* Global preferences. */
	globPref.loadFromSettings(settings);

	/* Proxy settings. */
	globProxSet.loadFromSettings(settings);

	/* Accounts. */
	m_accountModel.loadFromSettings(settings);
	ui->accountList->setModel(&m_accountModel);

	//ui->accountList->expandAll();

	/* Select last-used account. */
	setDefaultAccount(settings);

	/* Scan databases. */
	regenerateAllAccountModelYears();

	/* Load collapse info of account items from settings */
	loadAccountCollapseInfo(settings);

	/* Received Sent messages Column widths */
	loadSentReceivedMessagesColumnWidth(settings);
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
		dbDir = GlobPreferences::confDir();
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
		dbDir = GlobPreferences::confDir();
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
			dbDir = GlobPreferences::confDir();
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

	settings.clear();

	/* Window geometry. */
	saveWindowGeometry(settings);

	/* Global preferences. */
	globPref.saveToSettings(settings);

	/* Proxy settings. */
	globProxSet.saveToSettings(settings);

	/* Accounts. */
	m_accountModel.saveToSettings(settings);

	/* Store last-used account. */
	saveAccountIndex(settings);

	/* TODO */
	saveSentReceivedColumnWidth(settings);

	/* Store account collapses */
	saveAccountCollapseInfo(settings);

	settings.sync();
}


/* ========================================================================= */
/*
* Create message slot
 */
void MainWindow::on_actionCreate_message_triggered()
/* ========================================================================= */
{
	on_actionSent_message_triggered();
}


/* ========================================================================= */
/*
* Send message slot
 */
void MainWindow::on_actionSent_message_triggered()
/* ========================================================================= */
{
	/*
	 * TODO -- This method copies on_actionReply_to_the_sender_triggered().
	 * Delete one of them.
	 */
	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	MessageDb *messageDb = accountMessageDb(0);

	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, dbId,
	    DlgSendMessage::ACT_NEW, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(), this);
	if (newMessageDialog->exec() == QDialog::Accepted) {
		downloadMessageList(index, "sent");
	}
	setDefaultProgressStatus();
}


/* ========================================================================= */
/*
 * Create account dialog for addition new.
 */
void MainWindow::on_actionAdd_account_triggered()
/* ========================================================================= */
{
	QDialog *newAccountDialog = new DlgCreateAccount(*(ui->accountList),
	   m_accountDb, DlgCreateAccount::ACT_ADDNEW, this);
	if (QDialog::Accepted == newAccountDialog->exec()) {
		if (ui->accountList->model()->rowCount() > 0) {
			activeAccountMenuAndButtons(true);
			saveSettings();
		}
	}
}


/* ========================================================================= */
/*
* Delete account
 */
void MainWindow::on_actionDelete_account_triggered()
/* ========================================================================= */
{
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
}

/* ========================================================================= */
/*
 * Change user password for account.
 */
void MainWindow::on_actionChange_password_triggered()
/* ========================================================================= */
{
	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	QDialog *changePwd = new DlgChangePwd(dbId, *(ui->accountList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(), this);
	changePwd->exec();
}

/* ========================================================================= */
/*
 * Create account dialog for updates.
 */
void MainWindow::on_actionAccount_properties_triggered()
/* ========================================================================= */
{
	QDialog *editAccountDialog = new DlgCreateAccount(*(ui->accountList),
	   m_accountDb, DlgCreateAccount::ACT_EDIT, this);
	editAccountDialog->exec();
}


/* ========================================================================= */
/*
 * Move account up (currentRow - 1).
 */
void MainWindow::on_actionMove_account_up_triggered()
/* ========================================================================= */
{
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
* Move account down (currentRow + 1).
 */
void MainWindow::on_actionMove_account_down_triggered()
/* ========================================================================= */
{
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
* Change data directory
 */
void MainWindow::on_actionChange_data_directory_triggered()
/* ========================================================================= */
{
	/* TODO - Change data directory */
	qDebug() << "on_actionChange_data_directory_triggered";

}


/* ========================================================================= */
/*
* Download sent/received message list for current (selected) account
 */
void MainWindow::on_actionMark_all_as_read_triggered()
/* ========================================================================= */
{
	qDebug() << "on_actionMark_all_as_read_triggered";
	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	/* TODO */
/*
	for (int i = 0; i < ; i++) {

		(markMessageAsDownloaded(index, msgIdx))
		? qDebug() << "Message was marked as downloaded..."
		: qDebug() << "ERROR: Message was not marked as downloaded!";
	}
*/
}


/* ========================================================================= */
/*
* Download sent/received message list for current (selected) account
 */
void MainWindow::on_actionGet_messages_triggered()
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	QStandardItem *itemTop = AccountModel::itemTop(item);

	qDebug() << "------------------------------------------------";
	qDebug() << "Downloading message list for account" << itemTop->text();
	qDebug() << "------------------------------------------------";

	m_statusProgressBar->setFormat("GetListOfReceivedMessages");
	m_statusProgressBar->repaint();
	if (Q_CONNECT_ERROR == downloadMessageList(index,"received")) {
		setDefaultProgressStatus();
		qDebug() << "An error occurred!";
		return;
	}
	m_statusProgressBar->setFormat("GetListOfSentMessages");
	m_statusProgressBar->repaint();
	downloadMessageList(index,"sent");
	m_statusProgressBar->setFormat("GetMessageStateChanges");
	m_statusProgressBar->repaint();
	getListSentMessageStateChanges(index);
	m_statusProgressBar->setFormat("getPasswordInfo");
	m_statusProgressBar->repaint();
	getPasswordInfo(index);
	setDefaultProgressStatus();
	qDebug() << "ALL DONE!";
}


/* ========================================================================= */
/*
* Create reply dialog and sent reply message from current (selected) account
 */
void MainWindow::on_actionReply_to_the_sender_triggered()
/* ========================================================================= */
{
	/*
	 * TODO -- This method copies on_actionSent_message_triggered().
	 * Delete one of them.
	 */

	const QAbstractItemModel *tableModel = ui->messageList->model();
	Q_ASSERT(0 != tableModel);
	QModelIndex index = tableModel->index(
	    ui->messageList->currentIndex().row(), 0); /* First column. */
	/* TODO -- Reimplement this construction. */

	MessageDb *messageDb = accountMessageDb(0);

	QVector<QString> replyTo = messageDb->msgsReplyDataTo(
	    tableModel->itemData(index).first().toInt());

	/* TODO */
	index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);


	QString userName = accountUserName();
	QString dbId = m_accountDb.dbId(userName + "___True");

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, dbId,
	    DlgSendMessage::ACT_REPLY, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(), this,
	    replyTo[0], replyTo[1], replyTo[2], replyTo[3]);
	if (newMessageDialog->exec() == QDialog::Accepted) {
		downloadMessageList(index, "sent");
	}
	setDefaultProgressStatus();
}

/* ========================================================================= */
/*
* Active search databox dialog
 */
void MainWindow::on_actionFind_databox_triggered()
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->currentIndex();
	Q_ASSERT(index.isValid());
	index = AccountModel::indexTop(index);

	QString userName = accountUserName();
	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_BLANK, 0,
	    index.data(ROLE_ACNT_CONF_SETTINGS).toMap(), this, userName);
	dsSearch->show();
}


/* ========================================================================= */
/*
* Create reply dialog and sent reply message from current (selected) account
 */
void MainWindow::on_actionReply_triggered()
/* ========================================================================= */
{
	on_actionReply_to_the_sender_triggered();
}


/* ========================================================================= */
/*
* Clear message filter text
 */
void MainWindow::on_actionSearchClear_triggered(void)
/* ========================================================================= */
{
	m_searchLine->clear();
}


/* ========================================================================= */
/*
* Message filter
 */
void MainWindow::filterMessages(const QString &text)
/* ========================================================================= */
{
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
* Set new sent/recivied message column widths.
 */
void MainWindow::onTableColumnResized(int index, int oldSize, int newSize)
/* ========================================================================= */
{
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
* Load collapse info of account items from settings
*/
void MainWindow::loadAccountCollapseInfo(QSettings &settings)
/* ========================================================================= */
{
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
* Download sent/received message list from ISDS for current account index
*/
qdatovka_error MainWindow::downloadMessageList(const QModelIndex &acntTopIdx,
    const QString messageType)
/* ========================================================================= */
{
	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	m_statusProgressBar->setValue(10);

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo, this)) {
			qDebug() << "Error connection to ISDS";
			return Q_CONNECT_ERROR;
		}
	}

	m_statusProgressBar->setValue(20);

	isds_error status = IE_ERROR;
	struct isds_list *messageList = NULL;

	/* Download sent/received message list from ISDS for current account */
	if (messageType == "sent") {
		status = isds_get_list_of_sent_messages(isdsSessions.
		    session(accountInfo.userName()),
		    NULL, NULL, NULL,
		    //MESSAGESTATE_SENT |  MESSAGESTATE_STAMPED |
		    //MESSAGESTATE_INFECTED | MESSAGESTATE_DELIVERED,
		    MESSAGESTATE_ANY,
		    0, NULL, &messageList);
	} else if (messageType == "received") {
		status = isds_get_list_of_received_messages(isdsSessions.
		    session(accountInfo.userName()),
		    NULL, NULL, NULL,
		    //MESSAGESTATE_DELIVERED | MESSAGESTATE_SUBSTITUTED,
		    MESSAGESTATE_ANY,
		    0, NULL, &messageList);
	}

	if (status != IE_SUCCESS) {
		qDebug() << status << isds_strerror(status);
		isds_list_free(&messageList);
		return Q_ISDS_ERROR;
	}

	m_statusProgressBar->setValue(30);

	struct isds_list *box;
	box = messageList;
	int newcnt = 0;
	int allcnt = 0;

	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(acntTopIdx);
	MessageDb *messageDb = accountMessageDb(accountItem);

	while (0 != box) {
		allcnt++;
		box = box->next;
	}

	box = messageList;

	int delta = 0;
	int diff = 0;

	if (allcnt == 0) {
		m_statusProgressBar->setValue(60);
	} else {
		delta = ceil(70 / allcnt);
	}

	while (0 != box) {

		diff = diff + delta;
		m_statusProgressBar->setValue(30+diff);

		isds_message *item = (isds_message *) box->data;
		int dmId = atoi(item->envelope->dmID);

		if (!messageDb->isInMessageDb(dmId)) {

			QString dmAmbiguousRecipient;
			if (0 == item->envelope->dmAmbiguousRecipient) {
				dmAmbiguousRecipient = "0";
			} else {
				dmAmbiguousRecipient = QString::number(
				    *item->envelope->dmAmbiguousRecipient);
			}

			QString dmLegalTitleYear;
			if (0 == item->envelope->dmLegalTitleYear) {
				dmLegalTitleYear = "";
			} else {
				dmLegalTitleYear = QString::number(
				    *item->envelope->dmLegalTitleYear);
			}

			QString dmLegalTitleLaw;
			if (0 == item->envelope->dmLegalTitleLaw) {
				dmLegalTitleLaw = "";
			} else {
				dmLegalTitleLaw = QString::number(
				    *item->envelope->dmLegalTitleLaw);
			}

			QString dmSenderOrgUnitNum;
			if (0 == item->envelope->dmSenderOrgUnitNum) {
				dmSenderOrgUnitNum = "";
			} else {
				dmSenderOrgUnitNum =
				    *item->envelope->dmSenderOrgUnitNum != 0 ?
				    QString::number(*item->envelope->
				    dmSenderOrgUnitNum) : "";
			}

			QString dmRecipientOrgUnitNum;
			if (0 == item->envelope->dmRecipientOrgUnitNum) {
				dmRecipientOrgUnitNum = "";
			} else {
				dmRecipientOrgUnitNum =
				    *item->envelope->dmRecipientOrgUnitNum != 0
				    ? QString::number(*item->envelope->
				    dmRecipientOrgUnitNum) : "";
			}

			QString dmDeliveryTime = "";
			if (0 != item->envelope->dmDeliveryTime) {
				dmDeliveryTime = timevalToDbFormat(
				    item->envelope->dmDeliveryTime);
			}
			QString dmAcceptanceTime = "";
			if (0 != item->envelope->dmAcceptanceTime) {
				dmAcceptanceTime = timevalToDbFormat(
				    item->envelope->dmAcceptanceTime);
			}

			/* insert message envelope in db */
			(messageDb->msgsInsertMessageEnvelope(dmId,
			    /* TODO - set correctly next two values */
			    false, "tReturnedMessage",
			    item->envelope->dbIDSender,
			    item->envelope->dmSender,
			    item->envelope->dmSenderAddress,
			    (int)*item->envelope->dmSenderType,
			    item->envelope->dmRecipient,
			    item->envelope->dmRecipientAddress,
			    dmAmbiguousRecipient,
			    item->envelope->dmSenderOrgUnit,
			    dmSenderOrgUnitNum,
			    item->envelope->dbIDRecipient,
			    item->envelope->dmRecipientOrgUnit,
			    dmRecipientOrgUnitNum,
			    item->envelope->dmToHands,
			    item->envelope->dmAnnotation,
			    item->envelope->dmRecipientRefNumber,
			    item->envelope->dmSenderRefNumber,
			    item->envelope->dmRecipientIdent,
			    item->envelope->dmSenderIdent,
			    dmLegalTitleLaw,
			    dmLegalTitleYear,
			    item->envelope->dmLegalTitleSect,
			    item->envelope->dmLegalTitlePar,
			    item->envelope->dmLegalTitlePoint,
			    item->envelope->dmPersonalDelivery,
			    item->envelope->dmAllowSubstDelivery,
			    (char*)item->envelope->timestamp,
			    dmDeliveryTime,
			    dmAcceptanceTime,
			    convertHexToDecIndex(*item->envelope->dmMessageStatus),
			    (int)*item->envelope->dmAttachmentSize,
			    item->envelope->dmType,
			    messageType))
			? qDebug() << "Message envelope" << dmId <<
			    "was inserted into db..."
			: qDebug() << "ERROR: Message envelope " << dmId <<
			    "insert!";
			newcnt++;
		}
		box = box->next;

	}

	m_statusProgressBar->setValue(100);

	isds_list_free(&messageList);

	/* Redraw views' content. */
	regenerateAccountModelYears(acntTopIdx);
	/*
	 * Force repaint.
	 * TODO -- A better solution?
	 */
	ui->accountList->repaint();
	accountItemSelectionChanged(ui->accountList->currentIndex());

	if (messageType == "received") {
		qDebug() << "#Received total:" << allcnt;
		qDebug() << "#Received new:" << newcnt;
	} else {
		qDebug() << "#Sent total:" << allcnt;
		qDebug() << "#Sent new:" << newcnt;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*!
 * @brief Download attachments, envelope and raw for specific message.
 */
qdatovka_error MainWindow::downloadMessage(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx, bool signedMsg, bool incoming)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}

	m_statusProgressBar->setValue(10);

	/* First column. */
	QString dmId = msgIdx.sibling(msgIdx.row(), 0).data().toString();

	qDebug() << "Downloading complete message" << dmId;

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo, this)) {
			qDebug() << "Error connection to ISDS";
			return Q_CONNECT_ERROR;
		}
	}


	m_statusProgressBar->setValue(20);

	// message structures - all members
	struct isds_message *message = NULL;

	isds_error status;

	/* download signed message? */
	if (signedMsg) {
		/* sent or received message? */
		if  (incoming) {
			status = isds_get_signed_received_message(
			    isdsSessions.session(accountInfo.userName()),
			    dmId.toStdString().c_str(),
			    &message);
		} else {
			status = isds_get_signed_sent_message(
			    isdsSessions.session(accountInfo.userName()),
			    dmId.toStdString().c_str(),
			    &message);
		}
	} else {
		status = isds_get_received_message(isdsSessions.session(
		    accountInfo.userName()),
		    dmId.toStdString().c_str(),
		    &message);
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_message_free(&message);
		return Q_ISDS_ERROR;
	}

	m_statusProgressBar->setValue(50);

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(message->envelope->dmID);

	/* get signed raw data from message */
	if (signedMsg) {
		QString raw = QByteArray((char*)message->raw,
		    message->raw_length).toBase64();

		(messageDb->msgsInsertUpdateMessageRaw(dmID, raw, 0))
		? qDebug() << "Message raw data were updated..."
		: qDebug() << "ERROR: Message raw data update!";
	}

	QString timestamp = QByteArray((char *)message->envelope->timestamp,
	    message->envelope->timestamp_length).toBase64();
	QString dmAmbiguousRecipient;
	if (0 == message->envelope->dmAmbiguousRecipient) {
		dmAmbiguousRecipient = "0";
	} else {
		dmAmbiguousRecipient = QString::number(
		    *message->envelope->dmAmbiguousRecipient);
	}
	QString dmLegalTitleYear;
	if (0 == message->envelope->dmLegalTitleYear) {
		dmLegalTitleYear = "";
	} else {
		dmLegalTitleYear = QString::number(
		    *message->envelope->dmLegalTitleYear);
	}
	QString dmLegalTitleLaw;
	if (0 == message->envelope->dmLegalTitleLaw) {
		dmLegalTitleLaw = "";
	} else {
		dmLegalTitleLaw = QString::number(
		    *message->envelope->dmLegalTitleLaw);
	}
	QString dmSenderOrgUnitNum;
	if (0 == message->envelope->dmSenderOrgUnitNum) {
		dmSenderOrgUnitNum = "";
	} else {
		dmSenderOrgUnitNum =
		    *message->envelope->dmSenderOrgUnitNum != 0
		    ? QString::number(*message->envelope->
		    dmSenderOrgUnitNum) : "";
	}
	QString dmRecipientOrgUnitNum;
	if (0 == message->envelope->dmRecipientOrgUnitNum) {
		dmRecipientOrgUnitNum = "";
	} else {
		dmRecipientOrgUnitNum =
		  *message->envelope->dmRecipientOrgUnitNum != 0
		    ? QString::number(*message->envelope->
		    dmRecipientOrgUnitNum) : "";
	}
	QString dmDeliveryTime = "";
	if (0 != message->envelope->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(
		    message->envelope->dmDeliveryTime);
	}
	QString dmAcceptanceTime = "";
	if (0 != message->envelope->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(
		    message->envelope->dmAcceptanceTime);
	}

	/* update message envelope in db */
	(messageDb->msgsUpdateMessageEnvelope(dmID,
	    /* TODO - set correctly next two values */
	    false, "tReturnedMessage",
	    message->envelope->dbIDSender,
	    message->envelope->dmSender,
	    message->envelope->dmSenderAddress,
	    (int)*message->envelope->dmSenderType,
	    message->envelope->dmRecipient,
	    message->envelope->dmRecipientAddress,
	    dmAmbiguousRecipient,
	    message->envelope->dmSenderOrgUnit,
	    dmSenderOrgUnitNum,
	    message->envelope->dbIDRecipient,
	    message->envelope->dmRecipientOrgUnit,
	    dmRecipientOrgUnitNum,
	    message->envelope->dmToHands,
	    message->envelope->dmAnnotation,
	    message->envelope->dmRecipientRefNumber,
	    message->envelope->dmSenderRefNumber,
	    message->envelope->dmRecipientIdent,
	    message->envelope->dmSenderIdent,
	    dmLegalTitleLaw,
	    dmLegalTitleYear,
	    message->envelope->dmLegalTitleSect,
	    message->envelope->dmLegalTitlePar,
	    message->envelope->dmLegalTitlePoint,
	    message->envelope->dmPersonalDelivery,
	    message->envelope->dmAllowSubstDelivery,
	    timestamp,
	    dmDeliveryTime,
	    dmAcceptanceTime,
	    convertHexToDecIndex(*message->envelope->dmMessageStatus),
	    (int)*message->envelope->dmAttachmentSize,
	    message->envelope->dmType,
	    (incoming) ? "received" : "sent"))
	    ? qDebug() << "Message envelope was updated..."
	    : qDebug() << "ERROR: Message envelope update!";

	m_statusProgressBar->setValue(60);

	/* insert/update hash into db */
	QString hashValue = QByteArray((char*)message->envelope->hash->value,
	    message->envelope->hash->length).toBase64();
	(messageDb->msgsInsertUpdateMessageHash(dmID,
	    hashValue, convertHashAlg(message->envelope->hash->algorithm)))
	? qDebug() << "Message hash was stored into db..."
	: qDebug() << "ERROR: Message hash insert!";

	m_statusProgressBar->setValue(70);
	/* Insert/update all attachment files */
	struct isds_list *file;
	file = message->documents;
	while (0 != file) {
		isds_document *item = (isds_document *) file->data;

		QString dmEncodedContent = QByteArray((char *)item->data,
		    item->data_length).toBase64();

		/* Insert/update file to db */
		(messageDb->msgsInsertUpdateMessageFile(dmID,
		   item->dmFileDescr, item->dmUpFileGuid, item->dmFileGuid,
		   item->dmMimeType, item->dmFormat,
		   convertAttachmentType(item->dmFileMetaType),
		   dmEncodedContent))
		? qDebug() << "Message file" << item->dmFileDescr <<
		    "was stored into db..."
		: qDebug() << "ERROR: Message file" << item->dmFileDescr <<
		    "insert!";
		file = file->next;
	}

	m_statusProgressBar->setValue(80);

	if (incoming) {
		/* Download and save delivery info and message events */
		(getReceivedsDeliveryInfo(acntTopIdx, msgIdx, signedMsg))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";

		m_statusProgressBar->setValue(90);
		getMessageAuthor(acntTopIdx, msgIdx);

		/*  Mark this message as downloaded in ISDS */
		(markMessageAsDownloaded(acntTopIdx, msgIdx))
		? qDebug() << "Message was marked as downloaded..."
		: qDebug() << "ERROR: Message was not marked as downloaded!";
	} else {
		/* Download and save delivery info and message events */
		(getSentDeliveryInfo(acntTopIdx, dmID, true))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";
		m_statusProgressBar->setValue(90);
		getMessageAuthor(acntTopIdx, msgIdx);
	}

	m_statusProgressBar->setValue(100);

	isds_list_free(&message->documents);
	isds_message_free(&message);

	qDebug() << "downloadMessage(): Done!";

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
* Download message list from ISDS for current account
*/
void MainWindow::on_actionDownload_messages_triggered()
/* ========================================================================= */
{
	on_actionGet_messages_triggered();
}


/* ========================================================================= */
/*
* Download message list from ISDS for all accounts
*/
void MainWindow::on_actionSync_all_accounts_triggered()
/* ========================================================================= */
{
	bool success = true;
	int count = ui->accountList->model()->rowCount();
	for (int i = 0; i < count; i++) {
		QModelIndex index = m_accountModel.index(i, 0);
		QStandardItem *item = m_accountModel.itemFromIndex(index);
		QStandardItem *itemTop = AccountModel::itemTop(item);
		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading message list for account"
		    << itemTop->text();
		qDebug() << "-----------------------------------------------";
		m_statusProgressBar->setFormat("GetListOfReceivedMessages");
		m_statusProgressBar->repaint();
		if (Q_CONNECT_ERROR == downloadMessageList(index,"received")) {
			setDefaultProgressStatus();
			success = false;
			continue;
		}
		m_statusProgressBar->setFormat("GetListOfSentMessages");
		m_statusProgressBar->repaint();
		downloadMessageList(index,"sent");
		m_statusProgressBar->setFormat("GetMessageStateChanges");
		m_statusProgressBar->repaint();
		getListSentMessageStateChanges(index);
		m_statusProgressBar->setFormat("getPasswordInfo");
		m_statusProgressBar->repaint();
		getPasswordInfo(index);
		setDefaultProgressStatus();
	}
	qDebug() << "-----------------------------------------------";
	success ? qDebug() << "All DONE!" : qDebug() << "An error occurred!";
}


/* ========================================================================= */
/*
* Download message list from ISDS for all accounts (click from toolbar)
*/
void MainWindow::on_actionRecieved_all_triggered()
/* ========================================================================= */
{
	on_actionSync_all_accounts_triggered();
}


/* ========================================================================= */
/*
* Set message as downloaded from ISDS.
*/
bool MainWindow::markMessageAsDownloaded(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId = msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	isds_error status;
	status = isds_mark_message_read(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str());

	if (IE_SUCCESS != status) {
		return false;
	}
	return true;
}

/* ========================================================================= */
/*
* Download message delivery info, raw and get list of events message
*/
bool MainWindow::getReceivedsDeliveryInfo(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx, bool signedMsg)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	// message and envleople structures
	struct isds_message *message = NULL;
	isds_error status;

	(signedMsg)
	? status = isds_get_signed_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message)
	: status = isds_get_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_message_free(&message);
		return false;
	}

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(message->envelope->dmID);

	/* get signed raw data from message */
	if (signedMsg) {
		QString raw = QByteArray((char*)message->raw,
		    message->raw_length).toBase64();
		(messageDb->msgsInsertUpdateDeliveryRaw(dmID, raw))
		? qDebug() << "Message raw delivery info was updated..."
		: qDebug() << "ERROR: Message raw delivery info update!";
	}

	struct isds_list *event;
	event = message->envelope->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb->msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type),
		    item->description);
		event = event->next;
	}

	isds_list_free(&message->envelope->events);
	isds_message_free(&message);

	return true;
}


/* ========================================================================= */
/*
* Download sent message delivery info and get list of events message
*/
bool MainWindow::getSentDeliveryInfo(const QModelIndex &acntTopIdx,
    int msgIdx, bool signedMsg)
/* ========================================================================= */
{
	QString dmId = QString::number(msgIdx);
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	// message and envleople structures
	struct isds_message *message = NULL;
	isds_error status;

	(signedMsg)
	? status = isds_get_signed_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message)
	: status = isds_get_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message);

	if (IE_SUCCESS != status) {
		isds_message_free(&message);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	/* TODO - if signedMsg == true then decode signed message (raw ) */

	const QStandardItem *accountItem =
	    m_accountModel.itemFromIndex(acntTopIdx);
	MessageDb *messageDb = accountMessageDb(accountItem);

	int dmID = atoi(message->envelope->dmID);

	struct isds_list *event;
	event = message->envelope->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb->msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type),
		    item->description);
		event = event->next;
	}

	isds_list_free(&message->envelope->events);
	isds_message_free(&message);

	return true;
}


/* ========================================================================= */
/*
* Get list of sent message state changes
*/
bool MainWindow::getListSentMessageStateChanges(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	m_statusProgressBar->setValue(10);

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	m_statusProgressBar->setValue(20);

	struct isds_list *stateList = NULL;
	isds_error status;

	status = isds_get_list_of_sent_message_state_changes(
	    isdsSessions.session(accountInfo.userName()),NULL,NULL, &stateList);

	if (IE_SUCCESS != status) {
		isds_list_free(&stateList);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	m_statusProgressBar->setValue(30);

	struct isds_list *stateListFirst = NULL;
	stateListFirst = stateList;

	int allcnt = 0;

	while (0 != stateList) {
		allcnt++;
		stateList = stateList->next;
	}

	stateListFirst = stateList;
	int delta = 0;
	int diff = 0;

	if (allcnt == 0) {
		m_statusProgressBar->setValue(60);
	} else {
		delta = ceil(70 / allcnt);
	}

	while (0 != stateListFirst) {
		isds_message_status_change *item =
		    (isds_message_status_change *) stateListFirst->data;
		diff = diff + delta;
		m_statusProgressBar->setValue(30+diff);
		int dmId = atoi(item->dmID);
		/* Download and save delivery info and message events */
		(getSentDeliveryInfo(acntTopIdx, dmId, true))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";

		stateListFirst = stateListFirst->next;
	}

	m_statusProgressBar->setValue(100);

	isds_list_free(&stateList);

	regenerateAccountModelYears(acntTopIdx);

	return true;
}


/* ========================================================================= */
/*
* Get password expiration info for account index
*/
bool MainWindow::getPasswordInfo(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	isds_error status;
	struct timeval *expiration = NULL;
	QString expirDate;

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString key = accountInfo.userName() + "___True";

	if (accountInfo.loginMethod() != "username" &&
	    accountInfo.loginMethod() != "user_certificate") {
		expirDate = "";
		m_accountDb.setPwdExpirIntoDb(key, expirDate);
		return true;
	} else {

		if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
			isdsSessions.connectToIsds(accountInfo, this);
		}

		status = isds_get_password_expiration(
		    isdsSessions.session(accountInfo.userName()), &expiration);

		if (IE_SUCCESS == status) {
			if (0 != expiration) {
				expirDate = timevalToDbFormat(expiration);
				m_accountDb.setPwdExpirIntoDb(key, expirDate);
				return true;
			}
		}
		return true;
	}
	return false;
}


/* ========================================================================= */
/*
* Get additional info about author (sender)
*/
bool MainWindow::getMessageAuthor(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	isds_error status;
	isds_sender_type *sender_type = NULL;
	char * raw_sender_type = NULL;
	char * sender_name = NULL;

	status = isds_get_message_sender(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(),
	    &sender_type, &raw_sender_type, &sender_name);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(dmId.toStdString().c_str());

	(messageDb->addMessageAuthorInfo(dmID,
	    convertSenderTypeToString((int)*sender_type), sender_name))
	? qDebug() << "Author info of message was added..."
	: qDebug() << "ERROR: Author info of message wrong!";

	return true;
}


/* ========================================================================= */
/*
 * Verify message. Compare hash with hash stored in ISDS.
 */
qdatovka_error MainWindow::verifyMessage(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	isds_error status;
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
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo, this)) {
			qDebug() << "Error connection to ISDS";
			return Q_CONNECT_ERROR;
		};
	}

	isds_error status;
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
			qDebug() << "Message" << dmID << "was deleted from ISDS and db";
			return Q_SUCCESS;
		} else {
			qDebug() << "Message" << dmID << "was deleted from ISDS and db";
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
bool MainWindow::getOwnerInfoFromLogin(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}

	struct isds_DbOwnerInfo *db_owner_info = NULL;
	isds_error status;
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
		birthDate = tmToDbFormat(db_owner_info->birthInfo->biDate);
	}

	m_accountDb.insertAccountIntoDb(
	    username,
	    db_owner_info->dbID,
	    convertDbTypeToString(*db_owner_info->dbType),
	    atoi(db_owner_info->ic),
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
	    (int)*db_owner_info->dbEffectiveOVM,
	    (int)*db_owner_info->dbOpenAddressing);

	return true;
}


/* ========================================================================= */
/*
* Get data about logged in user.
*/
bool MainWindow::getUserInfoFromLogin(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo, this);
	}
	struct isds_DbUserInfo *db_user_info = NULL;
	isds_error status;

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
* Delete message slot
*/
void MainWindow::on_actionDelete_message_triggered()
/* ========================================================================= */
{
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
		if (Q_CONNECT_ERROR == eraseMessage(acntTopIdx, dmId)) {
		}
	}
}


/* ========================================================================= */
/*
* Download selected message attachments
*/
void MainWindow::on_actionDownload_message_signed_triggered()
/* ========================================================================= */
{
	downloadSelectedMessageAttachments();
}


/* ========================================================================= */
/*
* Download selected message attachments
*/
void MainWindow::on_actionAbout_Datovka_triggered()
/* ========================================================================= */
{
	QDialog *abDialog = new aboutDialog(this);
	abDialog->exec();
}


/* ========================================================================= */
/*
* Import database directory
*/
void MainWindow::on_actionImport_database_directory_triggered()
/* ========================================================================= */
{
	/* TODO - Import database directory */
	qDebug() << "on_actionImport_database_directory_triggered";
}


/* ========================================================================= */
/*
* Open attachment in associate program
*/
void MainWindow::on_actionOpen_attachment_triggered()
/* ========================================================================= */
{
	openSelectedAttachment();
}


/* ========================================================================= */
/*
* Save attachment to file
*/
void MainWindow::on_actionSave_attachment_triggered()
/* ========================================================================= */
{
	saveSelectedAttachmentToFile();
}


/* ========================================================================= */
/*
* Authenticate message form db
*/
qdatovka_error MainWindow::authenticateMessageFromDb(const QModelIndex &acntTopIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo, this)) {
			qDebug() << "Error connection to ISDS";
			return Q_CONNECT_ERROR;
		};
	}

	size_t length;
	isds_error status;
	QByteArray bytes;


	length = bytes.size();

	/* TODO - get message raw from db */

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
* Authenticate message form ZFO file
*/
qdatovka_error MainWindow::authenticateMessageFromZFO(void)
/* ========================================================================= */
{
	qDebug() << __func__;

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
	}

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo, this)) {
			qDebug() << "Error connection to ISDS";
			return Q_CONNECT_ERROR;
		};
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
* Authenticate message slot
*/
void MainWindow::on_actionAuthenticate_message_file_triggered(void)
/* ========================================================================= */
{
	switch (authenticateMessageFromZFO()) {
	case Q_SUCCESS:
		QMessageBox::information(this, tr("Message is authentic"),
		    tr("ISDS confirms that the message is valid."),
		    QMessageBox::Ok);
		break;
	case Q_NOTEQUAL:
		QMessageBox::warning(this, tr("Message is not authentic"),
		    tr("ISDS confirms that the message is invalid."),
		    QMessageBox::Ok);
		break;
	case Q_ISDS_ERROR:
		QMessageBox::warning(this, tr("Error authentication of message"),
		    tr("Authentication of message have been stopped"
		        "because the connection to ISDS failed!"),
		    QMessageBox::Ok);
		break;
	default:
		break;
	}
}


/* ========================================================================= */
/*
* Authenticate message slot
*/
void MainWindow::on_actionAuthenticate_message_triggered(void)
/* ========================================================================= */
{
	on_actionVerify_a_message_triggered();
}


/* ========================================================================= */
/*
* Verify message slot
*/
void MainWindow::on_actionVerify_a_message_triggered(void)
/* ========================================================================= */
{
	QModelIndex acntTopIdx = ui->accountList->currentIndex();
	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	acntTopIdx = AccountModel::indexTop(acntTopIdx);

	switch (verifyMessage(acntTopIdx, msgIdx)) {
	case Q_SUCCESS:
		QMessageBox::information(this, tr("Message is valid"),
		    tr("Hash of message corresponds to ISDS message hash."),
		    QMessageBox::Ok);
		break;
	case Q_NOTEQUAL:
		QMessageBox::warning(this, tr("Message is not authentic"),
		    tr("ISDS confirms that the message hash is invalid."),
		    QMessageBox::Ok);
		break;
	case Q_ISDS_ERROR:
		QMessageBox::warning(this, tr("Authenticate message error"),
		    tr("The message hash is not valid or connection to ISDS failed!"),
		    QMessageBox::Ok);
		break;
	default:
		break;
	}
}


/* ========================================================================= */
/*
* View message content of ZFO file slot
*/
void MainWindow::on_actionView_message_from_ZPO_file_triggered(void)
/* ========================================================================= */
{
	struct isds_ctx *dummy_session = NULL; /* Logging purposes. */
	struct isds_message *message = NULL;
	QDialog *viewDialog;

	qDebug() << __func__;

	QString fileName = QFileDialog::getOpenFileName(this,
	    tr("Add ZFO file"), "", tr("ZFO file (*.zfo)"));

	if (fileName.isEmpty()) {
		goto fail;
	}

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
* Show help from html file
*/
void MainWindow::on_actionHepl_triggered(void)
/* ========================================================================= */
{
	qDebug() << "on_actionHepl_triggered";
	/* TODO - load help content from html file to browser */
}


/* ========================================================================= */
/*
* Export message into as ZFO file on local disk
*/
void MainWindow::on_actionExport_as_ZFO_triggered(void)
/* ========================================================================= */
{
	qDebug() << __func__;

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
		msgBox.setWindowTitle(tr("QDatovka - Export error!"));
		msgBox.setText(tr("Can not export the message ") + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		  tr("You must download message firstly before its export..."));
		msgBox.exec();
		return;
	}

	QString fileName = "DDZ_" + dmId + ".zfo";
	Q_ASSERT(!fileName.isEmpty());
	/* TODO -- Remember directory? */
	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save message as ZFO file"), fileName);

	if (fileName.isEmpty()) {
		return;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		return; /* TODO -- Error message. */
	}


	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {

	}

	fout.close();
}


/* ========================================================================= */
/*
* Export message delivery info as ZFO file on local disk
*/
void MainWindow::on_actionExport_delivery_info_as_ZFO_triggered(void)
/* ========================================================================= */
{
	qDebug() << __func__;

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	MessageDb *messageDb = accountMessageDb(0);
	int dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(messageDb->msgsGetDeliveryInfoRaw(dmID)).toUtf8();
	if (raw.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("QDatovka - Export error!"));
		msgBox.setText(tr("Can not export the delivery info ") + dmId);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setInformativeText(
		  tr("You must download message firstly before export..."));
		msgBox.exec();
		return;
	}

	QString fileName = "DDZ_" + dmId + "_info.zfo";
	Q_ASSERT(!fileName.isEmpty());
	/* TODO -- Remember directory? */
	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save delivery info as ZFO file"), fileName);

	if (fileName.isEmpty()) {
		return;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		return; /* TODO -- Error message. */
	}


	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {

	}

	fout.close();
}


/* ========================================================================= */
/*
* Export message delivery info as PDF file on local disk
*/
void MainWindow::on_actionExport_delivery_info_as_PDF_triggered(void)
/* ========================================================================= */
{
	qDebug() << __func__;

	QString fileName;

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	int dmID = atoi(dmId.toStdString().c_str());

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	fileName = "DD_" + dmId + ".pdf";
	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save delivery info as PDF file"), fileName);

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
}


/* ========================================================================= */
/*
* Export message envelope as PDF file on local disk
*/
void MainWindow::on_actionExport_message_envelope_as_PDF_triggered(void)
/* ========================================================================= */
{
	qDebug() << __func__;

	QString fileName;

	QModelIndex msgIdx = ui->messageList->selectionModel()->currentIndex();
	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();
	int dmID = atoi(dmId.toStdString().c_str());

	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return;
	}

	fileName = "OZ_" + dmId + ".pdf";
	fileName = QFileDialog::getSaveFileName(this,
	    tr("Save message envelope as PDF file"), fileName);

	MessageDb *messageDb = accountMessageDb(0);

	QTextDocument doc;
	doc.setHtml(messageDb->envelopeInfoHtmlToPdf(dmID));

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

}


/* ========================================================================= */
/*
* Open message externaly
*/
void MainWindow::on_actionOpen_message_externally_triggered(void)
/* ========================================================================= */
{
	qDebug() << __func__;
}


/* ========================================================================= */
/*
* Open message delivery info externaly
*/
void MainWindow::on_actionOpen_delivery_info_externally_triggered(void)
/* ========================================================================= */
{
	qDebug() << __func__;
}
