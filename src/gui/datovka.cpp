

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


/* ========================================================================= */
MainWindow::MainWindow(QWidget *parent)
/* ========================================================================= */
    : QMainWindow(parent),
    accountModel(),
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


MainWindow::~MainWindow()
{
	delete ui;
}


void MainWindow::on_actionPreferences_triggered()
{
	QDialog *Preferences = new PreferencesDialog(this);
	Preferences->show();
}


void MainWindow::on_actionProxy_settings_triggered()
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
	QStandardItem *item = accountModel.itemFromIndex(index);

	qDebug() << index.model() << item->text();
	qDebug() << index.parent().row()  << " - " << index.row();

	/* Depending on which item was clicked show/hide elements. */

	if (index.parent().row() == -1) {
		sentMessageList->hide();
		receivedMessageList->hide();
		accountTextInfo->show();
		splitter_2->hide();
		html = createAccountInfo(item->text());
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
		//accountModel.addYearItemToAccount(index, "2014");
	}
}



QString MainWindow::addItemOfAccountInfo(QString title, QString data)
{
	QString item;
	item = QString("<div><strong>") + QString(title) +
	       QString("</strong>") + data + QString("</div>");
	return item;
}

QString MainWindow::createAccountInfo(QString accountName)
{
	QString html = QString("<h3>") + accountName + QString("</h3>");

	html += addItemOfAccountInfo(tr("Account name: "),accountName);
	html += addItemOfAccountInfo(tr("Username: "),accountName);
	html += addItemOfAccountInfo(tr("Databox ID: "),accountName);
	html += addItemOfAccountInfo(tr("Type of Databox: "),accountName);
	html += addItemOfAccountInfo(tr("IČ: "),accountName);
	html += addItemOfAccountInfo(tr("Company name: "),accountName);
	html += addItemOfAccountInfo(tr("Address: "),accountName);
	html += addItemOfAccountInfo(tr("City: "),accountName);
	html += addItemOfAccountInfo(tr("State: "),accountName);
	html += addItemOfAccountInfo(tr("Nationatily: "),accountName);
	html += addItemOfAccountInfo(tr("Efective OVM: "),accountName);
	html += addItemOfAccountInfo(tr("Open addressing: "),accountName);
	html += addItemOfAccountInfo(tr("Password expiration date: "),accountName);

	return html;
}

QString MainWindow::createAccountInfoAllField(QString accountName)
{
	QString html = QString("<h3>") + accountName + QString("</h3>");
	html += addItemOfAccountInfo(tr("Received messages: "),accountName);
	/* TODO - add count of recevied messages */
	html += addItemOfAccountInfo(tr("Sent messages: "),accountName);
	/* TODO - add count of sent messages */
	return html;
}



void MainWindow::setAccountInfoToWidget(QString html)
{
	QTextEdit *AccountTextInfo = ui->AccountTextInfo;
	AccountTextInfo->setHtml(html);
}


/* ========================================================================= */
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
void MainWindow::loadSettings(void)
/* ========================================================================= */
{
	QSettings settings(m_confFileName, QSettings::IniFormat);

	qDebug() << settings.allKeys();

	/* Accounts. */
	ui->AccountList->setModel(&accountModel);
	ui->AccountList->expandAll();
	connect(ui->AccountList, SIGNAL(clicked(QModelIndex)),
	    this, SLOT(treeItemClicked(QModelIndex)));

	/* Received messages. */
	ui->ReceivedMessageList->setModel(&receivedModel);

	/* Sent messages. */
	ui->SentMessageList->setModel(&sentModel);
}


/* ========================================================================= */
void MainWindow::saveSettings(void)
/* ========================================================================= */
{
}

void MainWindow::on_actionCreate_message_triggered()
{
    QDialog *newMessageDialog = new dlg_sent_message(this);
    newMessageDialog->show();
}
