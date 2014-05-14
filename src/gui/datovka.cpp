
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QTableView>

#include "datovka.h"
#include "src/common.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_create_account.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_preferences.h"
#include "src/gui/dlg_proxysets.h"
#include "src/gui/dlg_send_message.h"
#include "src/io/db_tables.h"
#include "src/io/isds_sessions.h"
#include "src/io/pkcs7.h"
#include "ui_datovka.h"


#define WIN_PREFIX "AppData/Roaming"
#define CONF_SUBDIR ".dsgui"
#define CONF_FILE "dsgui.conf"
#define ACCOUNT_DB_FILE "messages.shelf.db"

#define WIN_POSITION_HEADER "window_position"
#define WIN_POSITION_X "x"
#define WIN_POSITION_Y "y"
#define WIN_POSITION_W "w"
#define WIN_POSITION_H "h"

/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
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
	ui->toolBar->addAction(
	    QIcon(ICON_3PARTY_PATH + QString("delete_16.png")),
	    "", this, SLOT(on_actionSearchClear_triggered()));

	/* Account list. */
	ui->accountList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->accountList, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(treeItemRightClicked(QPoint)));

	/* Message list. */
	ui->messageList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->messageList, SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(tableItemRightClicked(QPoint)));

	m_confDirName = confDir();
	m_confFileName = m_confDirName + "/" + CONF_FILE;

	qDebug() << m_confDirName << m_confFileName;

	/* Change "\" to "/" */

	fixBackSlashesInFile(m_confFileName);

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(VERSION));
	ui->accountTextInfo->setReadOnly(true);

	/* Open accounts database. */
	m_accountDb.openDb(m_confDirName + "/" + ACCOUNT_DB_FILE);

	/* Load configuration file. */
	ensureConfPresence();
	loadSettings();
	/* Account list must already be set in order to connect this signal. */
	connect(ui->accountList->selectionModel(),
	    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
	    SLOT(treeItemSelectionChanged(QModelIndex, QModelIndex)));

	/* Enable sort of table items */
	ui->messageList->setSortingEnabled(true);

	/* set default columns size */
	setReciveidColumnWidths();

	/* Is it fires when any collumn was resized */
	connect(ui->messageList->horizontalHeader(),
	    SIGNAL(sectionResized(int, int, int)),
	    this, SLOT(onTableColumnResized(int, int, int)));

	/* Is it fires when any collumn was resized */
	connect(ui->messageList->horizontalHeader(),
	    SIGNAL(sectionClicked(int)),
	    this, SLOT(onTableColumnSort(int)));

	createIsdsContextForAllDataBoxes();
	connectToAllDataBoxes();

}


bool MainWindow::createIsdsContextForAllDataBoxes(void)
{
	int row = ui->accountList->model()->rowCount();
	for (int i = 0; i < row; i++) {
		QModelIndex index = m_accountModel.index(i,0);
		bool ret = createIsdsContext(index);
	}
	return true;
}


bool MainWindow::connectToAllDataBoxes(void)
{
	int row = ui->accountList->model()->rowCount();
	for (int i = 0; i < row; i++) {
		QModelIndex index = m_accountModel.index(i,0);
		connectToDataBox(index);
	}
	return true;
}


int MainWindow::createIsdsContext(const QModelIndex &index)
{
	/* Create isds context for account index */
	QStandardItem *accountItem = m_accountModel.itemFromIndex(index);
	const AccountModel::SettingsMap &itemSettings =
	    accountItem->data(ROLE_CONF_SETINGS).toMap();
	QString userName = itemSettings[USER].toString();

	if (isdsSessions.holdsSession(userName)) {
		return EXIT_SUCCESS;
	}

	if (0 != isdsSessions.createCleanSession(userName)) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}


bool MainWindow::connectToDataBox(const QModelIndex &index)
{
	isds_error status = IE_SUCCESS;

	QStandardItem *accountItem = m_accountModel.itemFromIndex(index);
	const AccountModel::SettingsMap &itemSettings =
	    accountItem->data(ROLE_CONF_SETINGS).toMap();

	const QString &login_method  = itemSettings[LOGIN].toString();
	const QString &userName  = itemSettings[USER].toString();
	QString password  = itemSettings[PWD].toString();
	const bool testAccount  = itemSettings[TEST].toBool();

	if (!isdsSessions.holdsSession(userName)) {
		return false;
	}

	/* Login method based on username and password */
	if (login_method == "username") {

		if (password.isEmpty()) {
			/* TODO -- Ask for password if password is empty. */
			password = "xxx";
		}
		status = isdsLoginUserName(isdsSessions.session(userName),
		    userName, password, testAccount);

	/* Login method based on certificate only */
	} else if (login_method == "certificate") {

		isds_pki_credentials *pki_credentials = NULL;
		QString certPath = itemSettings[P12FILE].toString();
		//pki_credentials->engine = NULL;
		//pki_credentials->certificate_format = PKI_FORMAT_DER;
		// malloc
		//pki_credentials->certificate = (char *) certPath.toStdString().c_str();
		//pki_credentials->key_format = PKI_FORMAT_DER;
		//pki_credentials->key = NULL;
		//pki_credentials->passphrase = NULL;

		QString iDbox = "TODO";
		/* TODO -- The account is not identified with a user name! */

		status = isdsLoginCert(isdsSessions.session(iDbox),
		    pki_credentials, testAccount);

	/* Login method based on certificate together with username */
	} else if (login_method == "user_certificate") {

		isds_pki_credentials *pki_credentials = NULL;
		QString certPath = itemSettings[P12FILE].toString();
		//pki_credentials->engine = NULL;
		//pki_credentials->certificate_format = PKI_FORMAT_DER;
		// malloc
		//pki_credentials->certificate = (char *) certPath.toStdString().c_str();
		//pki_credentials->key_format = PKI_FORMAT_DER;
		//pki_credentials->key = NULL;
		//pki_credentials->passphrase = NULL;

		if (password.isNull()) {
			status = isdsLoginUserCert(
			    isdsSessions.session(userName),
			    userName /* boxId */,
			    pki_credentials, testAccount);
		} else {
			status = isdsLoginUserCertPwd(
			    isdsSessions.session(userName),
			    userName /* boxId */, password,
			    pki_credentials, testAccount);
		}

	/* Login method based username and otp */
	} else {

		isds_otp *opt = NULL;

		if (isdsSessions.holdsSession(userName)) {
			status = isdsLoginUserOtp(
			    isdsSessions.session(userName), userName, password,
			    opt, testAccount);
		}

	}

	if (IE_SUCCESS != status) {
		qDebug() << "Error connecting to ISDS.";
		return false;
	}
	return true;
}



/* ========================================================================= */
/*
 * Changes all occurrences of '\' to '/' in given file.
 */
void MainWindow::fixBackSlashesInFile(const QString &fileName)
/* ========================================================================= */
{
	QString line;
	QFile file(fileName);

	if (file.open(QIODevice::ReadWrite|QIODevice::Text)) {
		QTextStream in(&file);
		line = in.readAll();
		line.replace(QString("\\"), QString("/"));
		file.reset();
		in << line;
		file.close();
	} else {
		qDebug() << "Error: Cannot open ini file";
	}
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
 * Redraws widgets according to selected item.
 */
void MainWindow::treeItemSelectionChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	(void) previous; /* Unused. */

	QString html;
	QAbstractTableModel *tableModel;

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
	qDebug() << "Selected data box ID" << dbId;
	//Q_ASSERT(!dbId.isEmpty());

	//qDebug() << "Clicked row" << current.row();
	//qDebug() << "Clicked type" << AccountModel::nodeType(current);

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
		html = createAccountInfo(*accountItem);
		break;
	case AccountModel::nodeRecentReceived:
		tableModel = messageDb->msgsRcvdWithin90DaysModel(dbId);
		//ui->messageList->horizontalHeader()->moveSection(5,3);
		break;
	case AccountModel::nodeRecentSent:
		tableModel = messageDb->msgsSntWithin90DaysModel(dbId);
		break;
	case AccountModel::nodeAll:
		html = createAccountInfoAllField(tr("All messages"),
		    messageDb->msgsRcvdYearlyCounts(dbId),
		    messageDb->msgsSntYearlyCounts(dbId));
		break;
	case AccountModel::nodeReceived:
		tableModel = messageDb->msgsRcvdModel(dbId);
		break;
	case AccountModel::nodeSent:
		tableModel = messageDb->msgsSntModel(dbId);
		break;
	case AccountModel::nodeReceivedYear:
		/* TODO -- Parameter check. */
		tableModel = messageDb->msgsRcvdInYearModel(dbId,
		    accountItem->text());
		break;
	case AccountModel::nodeSentYear:
		/* TODO -- Parameter check. */
		tableModel = messageDb->msgsSntInYearModel(dbId,
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
		    SLOT(tableItemSelectionChanged(QModelIndex, QModelIndex)));
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
		/*  set columns width*/
		setReciveidColumnWidths();
		goto setmodel;
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeSent:
	case AccountModel::nodeSentYear:
		/*  set columns width*/
		setSentColumnWidths();

setmodel:
		ui->messageStackedWidget->setCurrentIndex(1);
		Q_ASSERT(0 != tableModel);
		ui->messageList->setModel(tableModel);
		/* Apply message filter. */
		filterMessages(m_searchLine->text());
		/* Connect new slot. */
		connect(ui->messageList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(tableItemSelectionChanged(QModelIndex, QModelIndex)));
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
 * Account context right menu
 */
void MainWindow::treeItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->indexAt(point);
	QMenu *menu = new QMenu;

	if (index.isValid()) {
		//treeItemSelectionChanged(index);

		menu->addAction(
		   QIcon(ICON_16x16_PATH + QString("datovka-account-sync.png")),
		    tr("Get messages"),
		    this, SLOT(on_actionGet_messages_triggered()));
		menu->addAction(
		    QIcon(ICON_16x16_PATH + QString("datovka-message.png")),
		    tr("Create message"),
		    this, SLOT(on_actionCreate_message_triggered()));
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("tick_16.png")),
		    tr("Mark all as read"),
		    this, SLOT(on_actionMark_all_as_read_triggered()));
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("user_16.png")),
		    tr("Change password"),
		    this, SLOT(on_actionChange_password_triggered()));
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("letter_16.png")),
		    tr("Account properties"),
		    this, SLOT(on_actionAccount_properties_triggered()));
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("delete_16.png")),
		    tr("Remove Account"),
		    this, SLOT(on_actionDelete_account_triggered()));
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("up_16.png")),
		    tr("Move account up"),
		    this, SLOT(on_actionMove_account_up_triggered()));
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("down_16.png")),
		    tr("Move account down"),
		    this, SLOT(on_actionMove_account_down_triggered()));
		menu->addSeparator();
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("folder_16.png")),
		    tr("Change data directory"),
		    this, SLOT(on_actionChange_data_directory_triggered()));
	} else {
		menu->addAction(
		    QIcon(ICON_3PARTY_PATH + QString("plus_16.png")),
		    tr("Add new account"),
		    this, SLOT(on_actionAdd_account_triggered()));
	}

	menu->exec(QCursor::pos());
}


/* ========================================================================= */
/*
 * Sets content of widgets according to selected message.
 */
void MainWindow::tableItemSelectionChanged(const QModelIndex &current,
    const QModelIndex &previous)
/* ========================================================================= */
{
	(void) previous; /* Unused. */

	const QAbstractItemModel *tableModel = current.model();

	/* Disable message related buttons. */
	ui->downloadComplete->setEnabled(false);
	ui->saveAttachment->setEnabled(false);
	ui->openAttachment->setEnabled(false);
	ui->verifySignature->setEnabled(false);
	ui->signatureDetails->setEnabled(false);

	if (0 != tableModel) {
		QModelIndex index = tableModel->index(
		    current.row(), 0); /* First column. */

		MessageDb *messageDb = accountMessageDb();
		int msgId = tableModel->itemData(index).first().toInt();

		ui->messageInfo->setHtml(messageDb->descriptionHtml(msgId));
		ui->messageInfo->setReadOnly(true);

		/* Enable buttons according to databse content. */
		ui->verifySignature->setEnabled(
		    !messageDb->msgsVerificationAttempted(msgId));
		ui->signatureDetails->setEnabled(true);

	} else {
		ui->messageInfo->setHtml("");
		ui->messageInfo->setReadOnly(true);
	}

	/* TODO */
}


/* ========================================================================= */
/*
 * Generates menu to selected item. (And redraw widgets.)
 */
void MainWindow::tableItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QModelIndex index = ui->messageList->indexAt(point);
	QMenu *menu = new QMenu;

	if (index.isValid()) {
		//tableItemSelectionChanged(index);

		/* TODO */
		menu->addAction(QIcon(ICON_16x16_PATH +
		    QString("datovka-message-download.png")),
		    tr("Download message signed"), this,
		    SLOT(on_actionMark_all_as_read_triggered()));
		menu->addAction(QIcon(ICON_16x16_PATH +
		    QString("datovka-message-reply.png")), tr("Reply"), this,
		    SLOT(on_actionReply_to_the_sender_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_16x16_PATH +
		    QString("datovka-message-verify.png")),
		    tr("Verified messages"), this,
		    SLOT(on_actionReply_to_the_sender_triggered()));
		menu->addSeparator();
	} else {
		menu->addAction(QIcon(ICON_16x16_PATH +
		    QString("datovka-message.png")),
		    tr("Create a new message"),
		    this, SLOT(on_actionSent_message_triggered()));
	}
	menu->exec(QCursor::pos());
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
	html.append(strongAccountInfoLine(tr("Password expiration date"),
	    tr("unknown or without expiration")));

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
		dbDir = m_confDirName;
	}
	db = m_messageDbs.accessMessageDb(userName, dbDir,
	    itemSettings[TEST].toBool());
	Q_ASSERT(0 != db);

	return db;
}


/* ========================================================================= */
/*
 * Get configuration directory name.
 */
QString MainWindow::confDir(void)
/* ========================================================================= */
{
	QDir homeDir(QDir::homePath());

	if (homeDir.exists(WIN_PREFIX) && !homeDir.exists(CONF_SUBDIR)) {
		/* Set windows directory. */
		homeDir.cd(WIN_PREFIX);
	}

	return homeDir.path() + "/" + CONF_SUBDIR;
}


/* ========================================================================= */
/*
 * Create configuration file if not present.
 */
void MainWindow::ensureConfPresence(void) const
/* ========================================================================= */
{
	if (!QDir(m_confDirName).exists()) {
		QDir(m_confDirName).mkpath(".");
	}
	if (!QFile(m_confFileName).exists()) {
		QFile file(m_confFileName);
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
	int tmp = settings.value("panes/hpaned1", 0).toInt();
	sizes[0] = tmp;
	sizes[1] = w - sizes[0];;
	ui->hSplitterAccount->setSizes(sizes);

	// set messagelistspliter - vSplitterMessage
	sizes = ui->vSplitterMessage->sizes();
	sizes[0] = settings.value("panes/message_pane", 0).toInt();
	sizes[1] = h - SH_OFFS - sizes[0];
	ui->vSplitterMessage->setSizes(sizes);

	// set message/mesageinfospliter - hSplitterMessageInfo
	sizes = ui->hSplitterMessageInfo->sizes();
	sizes[0] = settings.value("panes/message_display_pane", 0).toInt();
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
				treeItemSelectionChanged(index.child(0,0));
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
	QSettings settings(m_confFileName, QSettings::IniFormat);

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
	m_sort_column =	settings.value("message_ordering/sort_column", 0).toInt();
	/* Sort column saturation from old datovka */
	if (m_sort_column > 5) {
		m_sort_column = 1;
	}
	m_sort_order = settings.value("message_ordering/sort_order", "").toString();
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
			dbDir = m_confDirName;
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
	QSettings settings(m_confFileName + "x", QSettings::IniFormat);

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

	MessageDb *messageDb = accountMessageDb();

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb,
	    DlgSendMessage::ACT_NEW, *(ui->accountList), *(ui->messageList),
	    this);
	newMessageDialog->show();
}

/* ========================================================================= */
/*
 * Create account dialog for addition new.
 */
void MainWindow::on_actionAdd_account_triggered()
/* ========================================================================= */
{
	QDialog *newAccountDialog = new DlgCreateAccount(*(ui->accountList),
	    DlgCreateAccount::ACT_ADDNEW, this);
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

	QDialog *changePwd = new DlgChangePwd(dbId, *(ui->accountList), this);
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
	    DlgCreateAccount::ACT_EDIT, this);
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

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb,
	    DlgSendMessage::ACT_REPLY, *(ui->accountList), *(ui->messageList),
	    this, replyTo[0], replyTo[1], replyTo[2], replyTo[3]);
	newMessageDialog->show();
}

/* ========================================================================= */
/*
* Active search databox dialog
 */
void MainWindow::on_actionFind_databox_triggered()
/* ========================================================================= */
{
	QString userName = accountUserName();
	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_BLANK, 0, this, userName);
	dsSearch->show();
}


/* ========================================================================= */
/*
* Reply meesage private slot
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
	Q_ASSERT(0 != tableModel);

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
* Set recivied message column widths and sort order.
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

void MainWindow::on_actionDownload_messages_triggered()
{
	isds_error status;
/*
	QString message_id = "1786066";
	struct isds_message *message = NULL;

	status = isds_get_received_envelope(isdsSessions.session(accountUserName()),
	    message_id.toStdString().c_str(), &message);

	qDebug() << status << isds_strerror(status);
*/

	struct isds_list *messageList = NULL;

	status = isds_get_list_of_received_messages(isdsSessions.session(accountUserName()),
	    NULL, NULL, NULL, MESSAGESTATE_ANY, 0, NULL, &messageList);

	qDebug() << status << isds_strerror(status);

	struct isds_list *box;
	box = messageList;

	while (0 != box) {

		isds_message *item = (isds_message *) box->data;
		qDebug() << item->envelope->dmID;
		qDebug() << item->envelope->dmAnnotation;
		qDebug() << item->envelope->dmSender;
		qDebug() << item->envelope->dmDeliveryTime->tv_sec;

		box = box->next;
	}
}
