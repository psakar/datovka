

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTableView>

#include "datovka.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "dlg_sent_message.h"
#include "ui_datovka.h"


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

	m_confDirName = QDir::homePath() + "/" + CONF_SUBDIR;
	m_confFileName = m_confDirName + "/" + CONF_FILE;

	qDebug() << m_confDirName << m_confFileName;

	ensureConfPresence();
	loadSettings();
}


/* ========================================================================= */
MainWindow::~MainWindow(void)
/* ========================================================================= */
{
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
	QTableView *sentMessageList = ui->SentMessageList;
	QTableView *receivedMessageList = ui->ReceivedMessageList;
	QTextEdit *accountTextInfo = ui->AccountTextInfo;
	QSplitter *splitter_2 = ui->splitter_2;
	QStandardItem *item = m_accountModel.itemFromIndex(index);

	qDebug() << index.model() << item->text();
	qDebug() << index.parent().row()  << " - " << index.row();

	/* Depending on which item was clicked show/hide elements. */

	if (index.parent().row() == -1) {
		/* Clicked account. */
		sentMessageList->hide();
		receivedMessageList->hide();
		accountTextInfo->show();
		splitter_2->hide();
		html = createAccountInfo(*item);
		setAccountInfoToWidget(html);
	} else if (index.row() == 0) {
		sentMessageList->hide();
		receivedMessageList->show();
		accountTextInfo->hide();
		splitter_2->show();
	} else if (index.row() == 1) {
		sentMessageList->show();
		receivedMessageList->hide();
		accountTextInfo->hide();
		splitter_2->show();
	} else {
		sentMessageList->hide();
		receivedMessageList->hide();
		accountTextInfo->show();
		splitter_2->hide();
		html = createAccountInfoAllField(tr("All messages"));
		setAccountInfoToWidget(html);
		//m_accountModel.addYearItemToAccount(index, "2014");
	}
}


/* ========================================================================= */
QString MainWindow::createAccountInfo(const QStandardItem &item)
/* ========================================================================= */
{
	const AccountModel::SettingsMap &itemSettings = item.data().toMap();
	QString html = QString("<h3>") + item.text() + QString("</h3>");

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
	ui->AccountTextInfo->setHtml(html);
}


/* ========================================================================= */
/*
 * Create configuration file if not present.
 */
void MainWindow::ensureConfPresence(void)
/* ========================================================================= */
{
	if (!QDir(m_confDirName).exists()) {
		QDir(QDir::homePath()).mkdir(CONF_SUBDIR);
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
	ui->AccountList->setModel(&m_accountModel);
	ui->AccountList->expandAll();
	connect(ui->AccountList, SIGNAL(clicked(QModelIndex)),
	    this, SLOT(treeItemClicked(QModelIndex)));

	/* Received messages. */
	ui->ReceivedMessageList->setModel(&receivedModel);

	/* Sent messages. */
	ui->SentMessageList->setModel(&sentModel);
}


/* ========================================================================= */
/*!
 * @brief Store current setting to configuration file.
 */
void MainWindow::saveSettings(void)
/* ========================================================================= */
{
	/* TODO */
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
