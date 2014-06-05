
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QTableView>
#include <QTemporaryFile>

#include "datovka.h"
#include "src/common.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
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

	/* Message list. */
	ui->messageList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->messageList, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(messageItemRightClicked(QPoint)));

	qDebug() << "Load " << globPref.loadConfPath();
	qDebug() << "Save" << globPref.saveConfPath();

	/* Change "\" to "/" */

	fixBackSlashesInFile(globPref.loadConfPath());

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(
	    QCoreApplication::applicationVersion()));
	ui->accountTextInfo->setReadOnly(true);

	/* Confioguration directory and file must exist. */
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
	/* TODO -- Check whether received of sent messages are shown? */
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
	m_statusProgressBar->setFormat(tr("Idle"));
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
	QAbstractTableModel *msgTblMdl;

	Q_ASSERT(current.isValid());
	if (!current.isValid()) {
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
	Q_ASSERT(!dbId.isEmpty());

	//qDebug() << "Clicked row" << current.row();
	//qDebug() << "Clicked type" << AccountModel::nodeType(current);

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
		setMessageActionVisibility(false);
		html = createAccountInfo(*accountItem);
		break;
	case AccountModel::nodeRecentReceived:
		msgTblMdl = messageDb->msgsRcvdWithin90DaysModel(dbId);
		//ui->messageList->horizontalHeader()->moveSection(5,3);
		break;
	case AccountModel::nodeRecentSent:
		msgTblMdl = messageDb->msgsSntWithin90DaysModel(dbId);
		break;
	case AccountModel::nodeAll:
		setMessageActionVisibility(false);
		html = createAccountInfoAllField(tr("All messages"),
		    messageDb->msgsRcvdYearlyCounts(dbId),
		    messageDb->msgsSntYearlyCounts(dbId));
		break;
	case AccountModel::nodeReceived:
		msgTblMdl = messageDb->msgsRcvdModel(dbId);
		break;
	case AccountModel::nodeSent:
		msgTblMdl = messageDb->msgsSntModel(dbId);
		break;
	case AccountModel::nodeReceivedYear:
		/* TODO -- Parameter check. */
		msgTblMdl = messageDb->msgsRcvdInYearModel(dbId,
		    accountItem->text());
		break;
	case AccountModel::nodeSentYear:
		/* TODO -- Parameter check. */
		msgTblMdl = messageDb->msgsSntInYearModel(dbId,
		    accountItem->text());
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
		goto setmodel;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		/* Set specific column width. */
		setSentColumnWidths();

setmodel:
		ui->messageStackedWidget->setCurrentIndex(1);
		Q_ASSERT(0 != msgTblMdl);
		ui->messageList->setModel(msgTblMdl);
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

		MessageDb *messageDb = accountMessageDb();
		int msgId = msgTblMdl->itemData(index).first().toInt();

		/* Generate and show message information. */
		ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId));
		ui->messageInfo->setReadOnly(true);

		/* Enable buttons according to databse content. */
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
		    SLOT(on_actionMark_all_as_read_triggered()));
		menu->addAction(
		    QIcon(ICON_16x16_PATH "datovka-message-reply.png"),
		    tr("Reply"), this,
		    SLOT(on_actionReply_to_the_sender_triggered()));
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
	fileName = "qdatovka_XXXXXX_" + fileName;

	//qDebug() << "Selected file: " << fileName;

	if (fileName.isEmpty()) {
		return;
	}

	QTemporaryFile fout(QDir::tempPath() + "/" + fileName);
	if (!fout.open()) {
		return; /* TODO -- Error message. */
	}

	fout.setAutoRemove(false);

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
	QModelIndex selectedIndex =
	    ui->messageList->selectionModel()->currentIndex();
	QModelIndex accountIndex = ui->accountList->currentIndex();
	accountIndex = AccountModel::indexTop(accountIndex);
	    /* selection().indexes() ? */

	m_statusProgressBar->setFormat(tr("downloadMessage"));
	m_statusProgressBar->repaint();

	/* TODO - set last bool parametr true, if current message is sent */
	/* TODO -- Check return value. */
	downloadMessage(accountIndex, selectedIndex, false, false);

	setDefaultProgressStatus();

	/* TODO -- Reload content of attachment list. */
}


/* ========================================================================= */
/*
 * Generate account info HTML message.
 */
QString MainWindow::createAccountInfo(const QStandardItem &item) const
/* ========================================================================= */
{
	const AccountModel::SettingsMap &itemSettings =
	    item.data(ROLE_CONF_SETINGS).toMap();

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
	    accountItemTop->data(ROLE_CONF_SETINGS).toMap();
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
	    accountItemTop->data(ROLE_CONF_SETINGS).toMap();
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
			    ROLE_CONF_SETINGS).toMap()[USER].toString();
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
	regenerateAccountModelYears();

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
		    itemTop->data(ROLE_CONF_SETINGS).toMap();
		const QString &userName = itemSettings[USER].toString();

		settings.beginGroup("default_account");
		settings.setValue("username", userName);
		settings.endGroup();
	}
}


/* ========================================================================= */
/*
 * Regenerates account model according to the database content.
 */
bool MainWindow::regenerateAccountModelYears(void)
/* ========================================================================= */
{
	QStandardItem *itemTop;
	MessageDb *db;
	QList<QString> yearList;

	m_accountModel.removeAllYearNodes();

	//qDebug() << "Generating years";

	for (int i = 0; i < m_accountModel.rowCount(); ++i) {
		itemTop = m_accountModel.item(i, 0);
		Q_ASSERT(0 != itemTop);
		const AccountModel::SettingsMap &itemSettings =
		    itemTop->data(ROLE_CONF_SETINGS).toMap();
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
		yearList = db->msgsRcvdYears(dbId);
		for (int j = 0; j < yearList.size(); ++j) {
			//qDebug() << yearList.value(j);
			m_accountModel.addNodeReceivedYear(itemTop,
			    yearList.value(j));
		}
		/* Sent. */
		yearList = db->msgsSntYears(dbId);
		for (int j = 0; j < yearList.size(); ++j) {
			//qDebug() << yearList.value(j);
			m_accountModel.addNodeSentYear(itemTop,
			    yearList.value(j));
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

	MessageDb *messageDb = accountMessageDb();

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb,
	    DlgSendMessage::ACT_NEW, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_CONF_SETINGS).toMap(), this);
	if (newMessageDialog->exec() == QDialog::Accepted) {
		downloadMessageList(index, "sent");
	}
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
	newAccountDialog->exec();
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
		if(itemTop->hasChildren()) {
			itemTop->removeRows(0, itemTop->rowCount());
		}
		ui->accountList->model()->removeRow(currentTopRow);
		/* TODO - delete database */
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
	index = AccountModel::indexTop(index);

	QDialog *changePwd = new DlgChangePwd(dbId, *(ui->accountList),
	    index.data(ROLE_CONF_SETINGS).toMap(), this);
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

void MainWindow::on_actionChange_data_directory_triggered()
{

}

void MainWindow::on_actionMark_all_as_read_triggered()
{

}

void MainWindow::on_actionGet_messages_triggered()
{
	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	m_statusProgressBar->setFormat(tr("GetListOfSentMessages"));
	m_statusProgressBar->repaint();
	downloadMessageList(index,"sent");
	m_statusProgressBar->setFormat(tr("GetListOfReceivedMessages"));
	m_statusProgressBar->repaint();
	downloadMessageList(index,"received");
	m_statusProgressBar->setFormat(tr("GetMessageStateChanges"));
	m_statusProgressBar->repaint();
	getListSentMessageStateChanges(index);
	m_statusProgressBar->setFormat(tr("getPasswordInfo"));
	m_statusProgressBar->repaint();
	getPasswordInfo(index);
	setDefaultProgressStatus();

}

void MainWindow::on_actionReply_to_the_sender_triggered()
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

	MessageDb *messageDb = accountMessageDb();

	QVector<QString> replyTo = messageDb->msgsReplyDataTo(
	    tableModel->itemData(index).first().toInt());

	/* TODO */
	index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb,
	    DlgSendMessage::ACT_REPLY, *(ui->accountList), *(ui->messageList),
	    index.data(ROLE_CONF_SETINGS).toMap(), this,
	    replyTo[0], replyTo[1], replyTo[2], replyTo[3]);
	if (newMessageDialog->exec() == QDialog::Accepted) {
		downloadMessageList(index, "sent");
	}
}

/* ========================================================================= */
/*
* Active search databox dialog
 */
void MainWindow::on_actionFind_databox_triggered()
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->currentIndex();
	index = AccountModel::indexTop(index);

	QString userName = accountUserName();
	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_BLANK, 0,
	    index.data(ROLE_CONF_SETINGS).toMap(), this, userName);
	dsSearch->show();
}


/* ========================================================================= */
/*
* Reply message private slot
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
	QAbstractItemModel *tableModel = ui->messageList->model();
	if (0 == tableModel) {
		return;
	}

	if (tableModel != &m_messageListProxyModel) {
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
bool MainWindow::downloadMessageList(const QModelIndex &acntTopIdx,
    const QString messageType)
/* ========================================================================= */
{
	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return false;
	}
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

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
		return false;
	}

	struct isds_list *box;
	box = messageList;
	int newcnt = 0;
	int allcnt = 0;
	MessageDb *messageDb = accountMessageDb();

	while (0 != box) {
		allcnt++;
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
			    timevalToDbFormat(item->envelope->dmDeliveryTime),
			    timevalToDbFormat(item->envelope->dmAcceptanceTime),
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

	isds_list_free(&messageList);

	if (newcnt > 0) {
		QModelIndex itemindex;
		if (messageType == "received") {
			itemindex = acntTopIdx.child(0,0);
		} else {
			itemindex = acntTopIdx.child(1,0);
		}
		QString label = itemindex.data().toString();
		label = label + " (" + QString::number(newcnt) + ")";
		QStandardItem *accountitem =
		    m_accountModel.itemFromIndex(itemindex);
		accountitem->setText(label);
		QFont font;
		font.setBold(true);
		accountitem->setFont(font);
	}

	if (messageType == "received") {
		qDebug() << "#Received total:" << allcnt;
		qDebug() << "#Received new:" << newcnt;
	} else {
		qDebug() << "#Sent total:" << allcnt;
		qDebug() << "#Sent new:" << newcnt;
	}

	return true;
}


/* ========================================================================= */
/*!
 * @brief Download attachments, envelope and raw for specific message.
 */
bool MainWindow::downloadMessage(const QModelIndex &acntIdx,
    const QModelIndex &msgIdx, bool signedMsg, bool sentMessage)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	/* First column. */
	QString dmId = msgIdx.sibling(msgIdx.row(), 0).data().toString();

	qDebug() << "Downloading complete message" << dmId;

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

	// message structures - all members
	struct isds_message *message = NULL;

	isds_error status;

	/* download signed message? */
	if (signedMsg) {
		/* sent or received message? */
		if  (sentMessage) {
			status = isds_get_signed_sent_message(
			    isdsSessions.session(accountInfo.userName()),
			    dmId.toStdString().c_str(),
			    &message);
		} else {
			status = isds_get_signed_received_message(
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
		return false;
	}

	MessageDb *messageDb = accountMessageDb();
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

	/* update message envelope in db */
	(messageDb->msgsUpdateMessageEnvelope(dmID,
	    /* TODO - set correctly next two values */
	    true, "tReturnedMessage",
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
	    timevalToDbFormat(message->envelope->dmDeliveryTime),
	    timevalToDbFormat(message->envelope->dmAcceptanceTime),
	    convertHexToDecIndex(*message->envelope->dmMessageStatus),
	    (int)*message->envelope->dmAttachmentSize,
	    message->envelope->dmType,
	    (sentMessage) ? "sent" : "received"))
	    ? qDebug() << "Message envelope was updated..."
	    : qDebug() << "ERROR: Message envelope update!";

	/* insert/update hash into db */
	QString hashValue = QByteArray((char*)message->envelope->hash->value,
		    message->envelope->hash->length).toBase64();
	(messageDb->msgsInsertUpdateMessageHash(dmID,
	    hashValue, convertHashAlg(message->envelope->hash->algorithm)))
	? qDebug() << "Message hash was stored into db..."
	: qDebug() << "ERROR: Message hash insert!";

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

	/* Download and save delivery info and message events */
	(getReceivedsDeliveryInfo(acntIdx, msgIdx, false))
	? qDebug() << "Delivery info of message was processed..."
	: qDebug() << "ERROR: Delivery info of message not found!";

	getMessageAuthor(acntIdx, msgIdx);


	/*  Mark this message as downloaded in ISDS */
	(markMessageAsDownloaded(acntIdx, msgIdx))
	? qDebug() << "Message was marked as downloaded..."
	: qDebug() << "ERROR: Message was not marked as downloaded!";

	isds_list_free(&message->documents);
	isds_message_free(&message);

	qDebug() << "downloadMessage(): Done!";

	return true;
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
	int count = ui->accountList->model()->rowCount();
	//qDebug() << count;
	for (int i = 0; i < count; i++) {
		qDebug() << "Downloading messages for account" << i;
		QModelIndex index = m_accountModel.index(i, 0);
			downloadMessageList(index,"sent");
			 /* TODO -- Check return value. */
			 downloadMessageList(index,"received");
	}
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
bool MainWindow::markMessageAsDownloaded(const QModelIndex &acntIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId = msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
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
bool MainWindow::getReceivedsDeliveryInfo(const QModelIndex &acntIdx,
    const QModelIndex &msgIdx, bool signedMsg)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
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

	MessageDb *messageDb = accountMessageDb();
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
		    timevalToDbFormat(item->time), item->description);
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
bool MainWindow::getSentDeliveryInfo(const QModelIndex &acntIdx,
    int msgIdx, bool signedMsg)
/* ========================================================================= */
{
	QString dmId = QString::number(msgIdx);
	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
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

	MessageDb *messageDb = accountMessageDb();
	int dmID = atoi(message->envelope->dmID);

	struct isds_list *event;
	event = message->envelope->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb->msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time), item->description);
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
bool MainWindow::getListSentMessageStateChanges(const QModelIndex &acntIdx)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

	struct isds_list *stateList = NULL;
	isds_error status;

	status = isds_get_list_of_sent_message_state_changes(
	    isdsSessions.session(accountInfo.userName()),NULL,NULL, &stateList);

	if (IE_SUCCESS != status) {
		isds_list_free(&stateList);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	struct isds_list *stateListFirst = NULL;
	stateListFirst = stateList;

	while (0 != stateList) {
		isds_message_status_change *item =
		    (isds_message_status_change *) stateList->data;
		int dmId = atoi(item->dmID);
		getSentDeliveryInfo(acntIdx, dmId, false);
		stateList = stateList->next;
	}

	isds_list_free(&stateListFirst);

	return true;
}


/* ========================================================================= */
/*
* Get password expiration info for account index
*/
bool MainWindow::getPasswordInfo(const QModelIndex &acntIdx)
/* ========================================================================= */
{
	isds_error status;
	struct timeval *expiration = NULL;
	QString expirDate;

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	QString key = accountInfo.userName() + "___True";

	if (accountInfo.loginMethod() != "username" &&
	    accountInfo.loginMethod() != "user_certificate") {
		expirDate = "";
		m_accountDb.setPwdExpirIntoDb(key, expirDate);
		return true;
	} else {

		if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
			isdsSessions.connectToIsds(accountInfo);
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
bool MainWindow::getMessageAuthor(const QModelIndex &acntIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
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

	MessageDb *messageDb = accountMessageDb();
	int dmID = atoi(dmId.toStdString().c_str());

	(messageDb->addMessageAuthorInfo(dmID,
	    convertSenderTypeToString((int)*sender_type), sender_name))
	? qDebug() << "Author info of message was added..."
	: qDebug() << "ERROR: Author info of message wrong!";

	return true;
}


/* ========================================================================= */
/*
* Verify message = comparison message hash with hash stored in ISDS.
*/
bool MainWindow::verifyMessage(const QModelIndex &acntIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

	isds_error status;
	struct isds_hash *hashIsds = NULL;

	status = isds_download_message_hash(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &hashIsds);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}

	struct isds_hash *hashLocal = NULL;
	hashLocal = (struct isds_hash *)
	    malloc(sizeof(struct isds_hash));

	if (hashLocal == NULL) {
		free(hashLocal);
		return false;
	}

	memset(hashLocal, 0, sizeof(struct isds_hash));
	MessageDb *messageDb = accountMessageDb();
	int dmID = atoi(dmId.toStdString().c_str());

	QList<QString> hashLocaldata;
	hashLocaldata = messageDb->msgsGetHashFromDb(dmID);

	/* TODO - check if hash info is in db */
	if (hashLocaldata[0].isEmpty()) {
		isds_hash_free(&hashLocal);
		isds_hash_free(&hashIsds);
		return false;
	}

	hashLocal->value = (void*)hashLocaldata[0].toStdString().c_str();
	hashLocal->length = (size_t)hashLocaldata[0].size();
	hashLocal->algorithm =
	    (isds_hash_algorithm)convertHashAlg2(hashLocaldata[1]);

	status = isds_hash_cmp(hashIsds, hashLocal);

	isds_hash_free(&hashLocal);
	isds_hash_free(&hashIsds);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}
	return true;
}


/* ========================================================================= */
/*
* Delete message from long term storage in ISDS.
*/
bool MainWindow::eraseMessage(const QModelIndex &acntIdx,
    const QModelIndex &msgIdx)
/* ========================================================================= */
{
	Q_ASSERT(msgIdx.isValid());
	if (!msgIdx.isValid()) {
		return false;
	}

	QString dmId =  msgIdx.sibling(msgIdx.row(), 0).data().toString();

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

	isds_error status;
	/* TODO - set true if message is received or false if is sent */
	bool incoming = true;

	status = isds_delete_message_from_storage(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(),
	    (incoming) ? true :false);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}
	qDebug() << "Message" << dmId << "was deleted from ISDS";

	/* TODO - delete message data from db (all tables) */

	return true;
}


/* ========================================================================= */
/*
* Get data about logged in user and his box.
*/
bool MainWindow::getOwnerInfoFromLogin(const QModelIndex &acntIdx)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
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
	QString bithDAte = "";
	if (0 != db_owner_info->birthInfo->biDate) {
		struct tm *birthDate = db_owner_info->birthInfo->biDate;
		bithDAte = QString::number(birthDate->tm_year) + "-" +
		QString::number(birthDate->tm_mon) + "-" +
		QString::number(birthDate->tm_mday);
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
	    bithDAte,
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
bool MainWindow::getUserInfoFromLogin(const QModelIndex &acntIdx)
/* ========================================================================= */
{

	const AccountModel::SettingsMap accountInfo =
	    acntIdx.data(ROLE_CONF_SETINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
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
