

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
    receivedModel(this),
    sentModel(this),
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
	QStandardItem *item = m_accountModel.itemFromIndex(index);

	qDebug() << index.model() << item->text();
	qDebug() << index.parent().row()  << " - " << index.row();

	/* Depending on which item was clicked show/hide elements. */

	if (index.parent().row() == -1) {
		ui->messageStackedWidget->setCurrentIndex(0);
		html = createAccountInfo(*item);
		setAccountInfoToWidget(html);
	} else if (index.row() == 0) {
		ui->messageStackedWidget->setCurrentIndex(1);
	} else if (index.row() == 1) {
		ui->messageStackedWidget->setCurrentIndex(1);
	} else {
		ui->messageStackedWidget->setCurrentIndex(0);
		html = createAccountInfoAllField(tr("All messages"));
		setAccountInfoToWidget(html);
		//m_accountModel.addYearItemToAccount(index, "2014");
	}
}


/* ========================================================================= */
void MainWindow::treeItemRightClicked(const QPoint &point)
/* ========================================================================= */
{
	QStandardItem *item = m_accountModel.itemFromIndex(ui->accountList->indexAt(point));

	if (0 != item) {
		QMenu *menu = new QMenu;
		menu->addAction(QString("Import"), menu, SLOT(slotImport()));
		menu->addAction(QString("Export"), menu, SLOT(slotExport()));
		menu->exec(QCursor::pos());
	}
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
void MainWindow::setAccountInfoToWidget(const QString &html)
/* ========================================================================= */
{
	ui->accountTextInfo->setHtml(html);
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
	ui->messageList->setModel(&receivedModel);

	/* Sent messages. */
	ui->messageList->setModel(&sentModel);
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
/*!
 * @brief Open/create message database related to item.
 */
void MainWindow::openMessageDb(const QStandardItem &item)
/* ========================================================================= */
{
	const AccountModel::SettingsMap &settings =
	    item.data(ROLE_SETINGS).toMap();

	QString dbFile = settings[USER].toString();
	Q_ASSERT(!dbFile.isEmpty());
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
