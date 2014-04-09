
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTableView>
#include <QStackedWidget>

#include "datovka.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "dlg_sent_message.h"
#include "dlg_create_account.h"
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


#define accountInfoLine(title, value) \
	"<div><strong>" + (title) + ": </strong>" + (value) + "</div>"


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
	connect(ui->accountList, SIGNAL(clicked(QModelIndex)),
	    this, SLOT(treeItemClicked(QModelIndex)));
	ui->accountList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->accountList,
	    SIGNAL(customContextMenuRequested(QPoint)),
	    this, SLOT(treeItemRightClicked(QPoint)));

	m_confDirName = confDir();
	m_confFileName = m_confDirName + "/" + CONF_FILE;

	qDebug() << m_confDirName << m_confFileName;

	/* Load configuration file. */
	ensureConfPresence();
	loadSettings();

	/* Show banner. */
	ui->messageStackedWidget->setCurrentIndex(0);
	ui->accountTextInfo->setHtml(createDatovkaBanner(VERSION));

	/* Open accounts database. */
	m_accountDb.openDb(m_confDirName + "/" + ACCOUNT_DB_FILE);
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
	preferences->show();
}


/* ========================================================================= */
void MainWindow::on_actionProxy_settings_triggered()
/* ========================================================================= */
{
	QDialog *Proxy = new ProxyDialog(this);
	Proxy->show();
}


/* ========================================================================= */
void MainWindow::treeItemClicked(const QModelIndex &index)
/* ========================================================================= */
{
	QString html;
	const QStandardItem *item = m_accountModel.itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);
	MessageDb *db;

	Q_ASSERT(0 != itemTop);

	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_SETINGS).toMap();

	const QString &userName = itemSettings[USER].toString();
	QString dbDir = itemSettings[DB_DIR].toString();

	Q_ASSERT(!userName.isEmpty());
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = m_confDirName;
	}

	qDebug() << "Selected user account" << userName << dbDir;
	qDebug() << index.model() << item->text();
	qDebug() << index.parent().row()  << " - " << index.row();

	db = m_messageDbs.accessMessageDb(userName, dbDir);
	Q_ASSERT(0 != db);

	/* Depending on which item was clicked show/hide elements. */
	if (index.parent().row() == -1) {
		/* Clicked account. */
		ui->messageStackedWidget->setCurrentIndex(0);
		html = createAccountInfo(*item);
		ui->accountTextInfo->setHtml(html);
	} else if (index.row() == 0) {
		/* Received messages. */
		ui->messageStackedWidget->setCurrentIndex(1);
		ui->messageList->setModel(db->receivedModel());
//		ui->messageList->horizontalHeader()->moveSection(5,3);
	} else if (index.row() == 1) {
		/* Sent messages. */
		ui->messageStackedWidget->setCurrentIndex(1);
		ui->messageList->setModel(db->sentModel());
	} else {
		/* Messages total. */
		ui->messageStackedWidget->setCurrentIndex(0);
		html = createAccountInfoAllField(tr("All messages"));
		ui->accountTextInfo->setHtml(html);
		//m_accountModel.addYearItemToAccount(index, "2014");
	}
}


/* ========================================================================= */
void MainWindow::treeItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QModelIndex index = ui->accountList->indexAt(point);
	QStandardItem *item = m_accountModel.itemFromIndex(index);
	QMenu *menu = new QMenu;

	if (0 != item) {
		treeItemClicked(index);

		menu->addAction(QIcon(ICON_16x16_PATH + QString("datovka-account-sync.png")),
		    QString(tr("Get messages")),
		    this, SLOT(on_actionGet_messages_triggered()));
		menu->addAction(QIcon(ICON_16x16_PATH + QString("datovka-message.png")),
		    QString(tr("Create message")),
		    this, SLOT(on_actionCreate_message_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("tick_16.png")),
		    QString(tr("Mark all as read")),
		    this, SLOT(on_actionMark_all_as_read_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("user_16.png")),
		    QString(tr("Change password")),
		    this, SLOT(on_actionChange_password_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("letter_16.png")),
		    QString(tr("Account properties")),
		    this, SLOT(on_actionAccount_properties_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("delete_16.png")),
		    QString(tr("Remove Account")),
		    this, SLOT(on_actionDelete_account_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("up_16.png")),
		    QString(tr("Move account up")),
		    this, SLOT(on_actionMove_account_up_triggered()));
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("down_16.png")),
		    QString(tr("Move account down")),
		    this, SLOT(on_actionMove_account_down_triggered()));
		menu->addSeparator();
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("folder_16.png")),
		    QString(tr("Change data directory")),
		    this, SLOT(on_actionChange_data_directory_triggered()));
	} else {
		menu->addAction(QIcon(ICON_3PARTY_PATH + QString("plus_16.png")),
		    QString(tr("Add new account")),
		    this, SLOT(on_actionAdd_account_triggered()));
	}

	menu->exec(QCursor::pos());
}


/* ========================================================================= */
/*
 * Generate account info HTML message.
 */
QString MainWindow::createAccountInfo(const QStandardItem &item)
/* ========================================================================= */
{
	const AccountModel::SettingsMap &itemSettings =
	    item.data(ROLE_SETINGS).toMap();

	QString html;

	html.append("<h3>");
	if (itemSettings[TEST].toBool()) {
		html.append(tr("Test account"));
	} else {
		html.append(tr("Standard account"));
	}
	html.append("</h3>");

	html.append(accountInfoLine(tr("Account name"),
	    itemSettings[NAME].toString()));
	html.append("<br>");
	html.append(accountInfoLine(tr("User name"),
	    itemSettings[USER].toString()));

	QString unknown = "unknown";

	html.append(accountInfoLine(tr("Databox ID"), unknown));
	html.append(accountInfoLine(tr("Type of Databox"), unknown));
	html.append(accountInfoLine(tr("IČ"), unknown));
	html.append(accountInfoLine(tr("Company name"), unknown));
	html.append(accountInfoLine(tr("Address"), unknown));
	html.append(accountInfoLine(tr("City"), unknown));
	html.append(accountInfoLine(tr("State"), unknown));
	html.append(accountInfoLine(tr("Nationality"), unknown));
	html.append(accountInfoLine(tr("Effective OVM"), unknown));
	html.append(accountInfoLine(tr("Open addressing"), unknown));
	html.append("<br>");
	html.append(accountInfoLine(tr("Password expiration date"), unknown));

	return html;
}


/* ========================================================================= */
/*
 * Generate overall account information.
 */
QString MainWindow::createAccountInfoAllField(const QString &accountName)
/* ========================================================================= */
{
	QString html = "<h3>" + accountName + "</h3>";

	html.append(accountInfoLine(tr("Received messages"), accountName));
	/* TODO - add count of received messages */
	html.append(accountInfoLine(tr("Sent messages"), accountName));
	/* TODO - add count of sent messages */
	return html;
}


/* ========================================================================= */
/*
 * Generate banner.
 */
QString MainWindow::createDatovkaBanner(const QString &version)
/* ========================================================================= */
{
	QString html = "<br><center>";
	html += "<h2>" +
	    tr("QDatovka - Free interface for Datové schránky") + "</h2>";
	html += accountInfoLine(tr("Version"), version);
	html += QString("<br><img src=") + ICON_128x128_PATH +
	    "datovka.png />";
	html += "<h3>" + tr("Powered by") + "</h3>";
	html += QString("<br><img src=") + ICON_128x128_PATH + "cznic.png />";
	html += "</center>";
	return html;
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
void MainWindow::ensureConfPresence(void)
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

	/* Received messages. */

	/* Sent messages. */
}


/* ========================================================================= */
/*!
 * @brief Store current setting to configuration file.
 */
void MainWindow::saveSettings(void)
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
	QDialog *newMessageDialog = new dlg_sent_message(this);
	newMessageDialog->show();
}

void MainWindow::on_actionAdd_account_triggered()
{
	QDialog *newAccountDialog = new CreateNewAccountDialog(this);
	newAccountDialog->show();
}

void MainWindow::on_actionDelete_account_triggered()
{

}

void MainWindow::on_actionChange_password_triggered()
{

}

void MainWindow::on_actionAccount_properties_triggered()
{

}

void MainWindow::on_actionMove_account_up_triggered()
{

}

void MainWindow::on_actionMove_account_down_triggered()
{

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
