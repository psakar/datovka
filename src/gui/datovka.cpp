

#include <QDebug>
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


#define accountInfoLine(title, value) \
	"<div><strong>" + (title) + ": </strong>" + (value) + "</div>"


/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
    m_accountModel(this),
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
	QDialog *Preferences = new PreferencesDialog(this);
	Preferences->show();
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

		menu->addAction(QString(tr("Option 1")),
		    this, SLOT(slotOption1()));
		menu->addAction(QString(tr("Option 2")),
		    this, SLOT(slotOption2()));
	} else {
		menu->addAction(QString(tr("Add new")),
		    this, SLOT(slotAddNew()));
	}

	menu->exec(QCursor::pos());
}


/* ========================================================================= */
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
QString MainWindow::createAccountInfoAllField(const QString &accountName)
/* ========================================================================= */
{
	QString html = QString("<h3>") + accountName + QString("</h3>");

	html.append(accountInfoLine(tr("Received messages"), accountName));
	/* TODO - add count of received messages */
	html.append(accountInfoLine(tr("Sent messages"), accountName));
	/* TODO - add count of sent messages */
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

	/* Accounts. */
	m_accountModel.loadFromSettings(settings);
	ui->accountList->setModel(&m_accountModel);
//	ui->AccountList->expandAll();

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
