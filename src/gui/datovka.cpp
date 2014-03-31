

#include <QDebug>
#include <QTableView>

#include "datovka.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "ui_datovka.h"


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
	QTableView *sentMessageList = ui->SentMessageList;
	QTableView *receivedMessageList = ui->ReceivedMessageList;
	QTextEdit *accountTextInfo = ui->AccountTextInfo;
	QSplitter *splitter_2 = ui->splitter_2;
	QStandardItem *item = accountModel.itemFromIndex(index);

	qDebug() << index.model() << item->text();
	qDebug() << index.row()  << " - " << index.column()
	    << " - "<< index.parent().row();

	/* Depending on which item was clicked show/hide elements. */

	if (index.parent().row() == -1) {
		sentMessageList->hide();
		receivedMessageList->hide();
		accountTextInfo->show();
		splitter_2->hide();
		QString html = createAccountInfo(item->text());
		setAccountInfo(html);
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
		QString html = createAccountInfo(tr("All messages"));
		setAccountInfo(html);
	}
}


QString MainWindow::createAccountInfo(QString accountName)
{
	QString html=QString("<h3>") + accountName + QString("</h3>");
	return html;
}


void MainWindow::setAccountInfo(QString html)
{
	QTextEdit *AccountTextInfo = ui->AccountTextInfo;
	AccountTextInfo->setHtml(html);
}
