
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QTableView>

#include "datovka.h"
#include "dlg_change_pwd.h"
#include "dlg_create_account.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "dlg_send_message.h"
#include "src/common.h"
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
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);

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

	/* Load configuration file. */
	ensureConfPresence();
	loadSettings();
	/* Account list must already be set in order to connect this signal. */
	connect(ui->accountList->selectionModel(),
	    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
	    SLOT(treeItemSelectionChanged(QModelIndex, QModelIndex)));

	/* Open accounts database. */
	m_accountDb.openDb(m_confDirName + "/" + ACCOUNT_DB_FILE);
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
void MainWindow::on_actionPreferences_triggered()
/* ========================================================================= */
{
	QDialog *preferences = new PreferencesDialog(this);
	preferences->exec();
}


/* ========================================================================= */
void MainWindow::on_actionProxy_settings_triggered()
/* ========================================================================= */
{
	QDialog *Proxy = new ProxyDialog(this);
	Proxy->exec();
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

//	qDebug() << "Selected user account" << userName << dbDir;
//	qDebug() << current.model() << accountItem->text();
//	qDebug() << "\n";

	/*
	 * TODO -- Is '___True' somehow related to the testing state
	 * of an account?
	 */
	QString dbId = m_accountDb.dbId(userName + "___True");
	qDebug() << "Selected data box ID" << dbId;
	//Q_ASSERT(!dbId.isEmpty());

//	qDebug() << "Clicked row" << current.row();
//	qDebug() << "Clicked type" << AccountModel::nodeType(current);

	switch (AccountModel::nodeType(current)) {
	case AccountModel::nodeAccountTop:
		html = createAccountInfo(*accountItem);
		break;
	case AccountModel::nodeRecentReceived:
		tableModel = messageDb->msgsRcvdWithin90DaysModel(dbId);
//		ui->messageList->horizontalHeader()->moveSection(5,3);
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
		break;
	case AccountModel::nodeRecentReceived:
	case AccountModel::nodeRecentSent:
	case AccountModel::nodeReceived:
	case AccountModel::nodeSent:
	case AccountModel::nodeReceivedYear:
	case AccountModel::nodeSentYear:
		ui->messageStackedWidget->setCurrentIndex(1);
		ui->messageList->setModel(tableModel);
		/* Connect new slot. */
		connect(ui->messageList->selectionModel(),
		    SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
		    SLOT(tableItemSelectionChanged(QModelIndex, QModelIndex)));
		/* Clear message info. */
		ui->messageInfo->clear();
		/* Select last message in list if there are some messages. */
		itemModel = ui->messageList->model();
		if ((0 != itemModel) && (0 < itemModel->rowCount())) {
			QModelIndex lastIndex =
			    itemModel->index(itemModel->rowCount() - 1, 0);
			ui->messageList->setCurrentIndex(lastIndex);
			ui->actionReply_to_the_sender->setEnabled(true);
			ui->actionVerify_a_message->setEnabled(true);
		} else {
			ui->actionReply_to_the_sender->setEnabled(false);
			ui->actionVerify_a_message->setEnabled(false);
		}

		break;
	default:
		Q_ASSERT(0);
		break;
	}
}


/* ========================================================================= */
void MainWindow::treeItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->indexAt(point);
	QMenu *menu = new QMenu;

	if (index.isValid()) {
//		treeItemSelectionChanged(index);

		menu->addAction(QIcon(ICON_16x16_PATH + QString("datovka-account-sync.png")),
		    tr("Get messages"),
		    this, SLOT(on_actionGet_messages_triggered()));
		menu->addAction(QIcon(ICON_16x16_PATH + QString("datovka-message.png")),
		    tr("Create message"),
		    this, SLOT(on_actionCreate_message_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("tick_16.png")),
		    tr("Mark all as read"),
		    this, SLOT(on_actionMark_all_as_read_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("user_16.png")),
		    tr("Change password"),
		    this, SLOT(on_actionChange_password_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("letter_16.png")),
		    tr("Account properties"),
		    this, SLOT(on_actionAccount_properties_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("delete_16.png")),
		    tr("Remove Account"),
		    this, SLOT(on_actionDelete_account_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("up_16.png")),
		    tr("Move account up"),
		    this, SLOT(on_actionMove_account_up_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("down_16.png")),
		    tr("Move account down"),
		    this, SLOT(on_actionMove_account_down_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("folder_16.png")),
		    tr("Change data directory"),
		    this, SLOT(on_actionChange_data_directory_triggered()));
	} else {
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("plus_16.png")),
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
	Q_ASSERT(0 != tableModel);
	QModelIndex index = tableModel->index(
	    current.row(), 0); /* First column. */

	MessageDb *messageDb = accountMessageDb();

	ui->messageInfo->setHtml(
	    messageDb->descriptionHtml(
	        tableModel->itemData(index).first().toInt()));

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
//		tableItemSelectionChanged(index);

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
	for (int i = 0; i < AccountEntry::knownAttrs.size(); ++i) {
		const QString &key = AccountEntry::knownAttrs[i].first;
		if (accountEntry.hasValue(key) &&
		    !AccountEntry::attrProps[key].desc.isEmpty()) {
			switch (AccountEntry::knownAttrs[i].second) {
			case DB_INTEGER:
				html.append(strongAccountInfoLine(
				    AccountEntry::attrProps[key].desc,
				    QString::number(
				        accountEntry.value(key).toInt())));
				break;
			case DB_TEXT:
				html.append(strongAccountInfoLine(
				    AccountEntry::attrProps[key].desc,
				    accountEntry.value(key).toString()));
				break;
			case DB_BOOLEAN:
				html.append(strongAccountInfoLine(
				    AccountEntry::attrProps[key].desc,
				    accountEntry.value(key).toBool() ?
				        tr("Yes") : tr("No")));
				break;
			case DB_DATETIME:
				html.append(strongAccountInfoLine(
				    AccountEntry::attrProps[key].desc,
				    dateTimeFromDbFormat(
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
	QString html = ("<div style=\"margin-left: 12px;\">");
	html.append ("<h3>" + accountName + "</h3>");

	html.append(strongAccountInfoLine(tr("Received messages"), ""));
	html.append("<div style=\"margin-left: 12px;\">");
	if (0 == receivedCounts.size()) {
		html.append(tr("none"));
	} else {
		for (int i = 0; i < receivedCounts.size(); ++i) {
			html.append(accountInfoLine(receivedCounts[i].first,
			    QString::number(receivedCounts[i].second)));
		}
	}
	html.append("</div>");

	html.append("<br/>");

	html.append(strongAccountInfoLine(tr("Sent messages"), ""));
	html.append("<div style=\"margin-left: 12px;\">");
	if (0 == sentCounts.size()) {
		html.append(tr("none"));
	} else {
		for (int i = 0; i < sentCounts.size(); ++i) {
			html.append(accountInfoLine(sentCounts[i].first,
			    QString::number(sentCounts[i].second)));
		}
	}
	html.append("</div>");

	html.append("</div>");
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
				break;
			}
		}
	}
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

//	qDebug() << "All: " << settings.allKeys();
//	qDebug() << "Groups: " << settings.childGroups();
//	qDebug() << "Keys:" << settings.childKeys();

	/* Window geometry. */
	loadWindowGeometry(settings);

	/* Global preferences. */
	globPref.loadFromSettings(settings);

	/* Proxy settings. */
	globProxSet.loadFromSettings(settings);

	/* Accounts. */
	m_accountModel.loadFromSettings(settings);
	ui->accountList->setModel(&m_accountModel);

	ui->accountList->expandAll();

	/* Open accounts database. */
	m_accountDb.openDb(m_confDirName + "/" + ACCOUNT_DB_FILE);

	/* Select last-used account. */
	setDefaultAccount(settings);

	/* Scan databases. */
	regenerateAccountModelYears();

	/* Received messages. */

	/* Sent messages. */
}


/* ========================================================================= */
/*
 * Store current account user name to settings.
 */
void MainWindow::saveAccountIndex(QSettings &settings) const
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->currentIndex();
	const QStandardItem *item = m_accountModel.itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);

	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_CONF_SETINGS).toMap();
	const QString &userName = itemSettings[USER].toString();

	settings.beginGroup("default_account");
	settings.setValue("username", userName);
	settings.endGroup();
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

//	qDebug() << "Generating years";

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
//			qDebug() << yearList.value(j);
			m_accountModel.addNodeReceivedYear(itemTop,
			    yearList.value(j));
		}
		/* Sent. */
		yearList = db->msgsSntYears(dbId);
		for (int j = 0; j < yearList.size(); ++j) {
//			qDebug() << yearList.value(j);
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

	saveAccountIndex(settings);

	/* Global preferences. */
	globPref.saveToSettings(settings);

	/* Proxy settings. */
	globProxSet.saveToSettings(settings);

	/* Accounts. */
	m_accountModel.saveToSettings(settings);

	/* Store last-used account. */
	saveAccountIndex(settings);

	/* TODO */

	settings.sync();
}


/* ========================================================================= */
void MainWindow::on_actionCreate_message_triggered()
/* ========================================================================= */
{
	on_actionSent_message_triggered();
}

/* ========================================================================= */
void MainWindow::on_actionSent_message_triggered()
/* ========================================================================= */
{
	/*
	 * TODO -- This method copies on_actionReply_to_the_sender_triggered().
	 * Delete one of them.
	 */

	MessageDb *messageDb = accountMessageDb();

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, this,
	    ui->accountList, ui->messageList, "New");
	newMessageDialog->show();
}

/* ========================================================================= */
/*
 * Create account dialog for addition new.
 */
void MainWindow::on_actionAdd_account_triggered()
/* ========================================================================= */
{
	QDialog *newAccountDialog =
	    new CreateNewAccountDialog(this, ui->accountList, "New");
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

	QDialog *changePwd = new changePassword(this, ui->accountList, dbId);
	changePwd->exec();
}

/* ========================================================================= */
/*
 * Create account dialog for updates.
 */
void MainWindow::on_actionAccount_properties_triggered()
/* ========================================================================= */
{
	QDialog *newAccountDialog =
	    new CreateNewAccountDialog(this, ui->accountList, "Edit");
	newAccountDialog->exec();
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
	ui->accountList->expandAll();
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
	ui->accountList->expandAll();
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

	QDialog *newMessageDialog = new DlgSendMessage(*messageDb, this,
	    ui->accountList, ui->messageList, "Reply",
	    replyTo[0], replyTo[1], replyTo[2], replyTo[3]);
	newMessageDialog->show();
}
