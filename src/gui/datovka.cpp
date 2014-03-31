

#include <QDebug>
#include <QTableView>

#include "datovka.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "ui_datovka.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    accountModel(),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	QStandardItemModel *tablemodel = this->initRecievedMessageListTable();
	for(int i=0; i<3; i++) {
		this->addMessageIntoRecieved(tablemodel, i, "12365","Dodávka světelných mečů","Orion","12.3.2014 12:12","12.3.2014 12:15");
	}

	QStandardItemModel *tablemodel2 = this->initSentMessageListTable();
	for(int i=0; i<3; i++) {
		this->addMessageIntoSent(tablemodel2, i, "12365","Dodávka světelných mečů","Orion","12.3.2014 12:12","12.3.2014 12:15", "Ok");
	}

	ui->AccountList->setModel(&accountModel);
	ui->AccountList->expandAll();
	connect(ui->AccountList, SIGNAL(clicked(QModelIndex)),
	    this, SLOT(treeItemClicked(QModelIndex)));
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


/**
 * @brief MainWindow::InitRecievedMessageListTable
 * @return
 */
QStandardItemModel* MainWindow::initRecievedMessageListTable() {

    QTableView *RecievedMessageTableView = ui->ReceivedMessageList;
    QStandardItemModel *standardModel = new QStandardItemModel(0,6,this) ;

    // add header
    standardModel->setHorizontalHeaderItem(0, new QStandardItem(QString(tr("Id"))));
    standardModel->setHorizontalHeaderItem(1, new QStandardItem(QString(tr("Message Subject"))));
    standardModel->setHorizontalHeaderItem(2, new QStandardItem(QString(tr("Sender"))));
    standardModel->setHorizontalHeaderItem(3, new QStandardItem(QString(tr("Recieved"))));
    standardModel->setHorizontalHeaderItem(4, new QStandardItem(QString(tr("Accepted"))));
    standardModel->setHorizontalHeaderItem(5, new QStandardItem(QString(tr("Locally read"))));

    //register the model
    RecievedMessageTableView->setModel(standardModel);
    return standardModel;
}


QStandardItemModel* MainWindow::initSentMessageListTable() {

    QTableView *SentMessageTableView = ui->SentMessageList;
    QStandardItemModel *standardModel = new QStandardItemModel(0,6,this) ;
    // add header
    standardModel->setHorizontalHeaderItem(0, new QStandardItem(QString(tr("Id"))));
    standardModel->setHorizontalHeaderItem(1, new QStandardItem(QString(tr("Message Subject"))));
    standardModel->setHorizontalHeaderItem(2, new QStandardItem(QString(tr("Recipient"))));
    standardModel->setHorizontalHeaderItem(3, new QStandardItem(QString(tr("Recieved"))));
    standardModel->setHorizontalHeaderItem(4, new QStandardItem(QString(tr("Accepted"))));
    standardModel->setHorizontalHeaderItem(5, new QStandardItem(QString(tr("Status"))));
    //register the model
    SentMessageTableView->setModel(standardModel);
    return standardModel;
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


/**
 * @brief MainWindow::AddMessageIntoRecieved
 * @param Id
 * @param Title
 * @param Sender
 * @param Delivered
 * @param Accepted
 * @return
 */
bool MainWindow::addMessageIntoRecieved(QStandardItemModel* model,int row, QString Id, QString Title,
    QString Sender, QString Delivered, QString Accepted){

    QStandardItem *item = new QStandardItem(Id);
    model->setItem(row,0, item);
    item = new QStandardItem(Title);
    model->setItem(row,1, item);
    item = new QStandardItem(Sender);
    model->setItem(row,2, item);
    item = new QStandardItem(Delivered);
    model->setItem(row,3, item);
    item = new QStandardItem(Accepted);
    model->setItem(row,4, item);
    return true;
}

/**
 * @brief MainWindow::AddMessageIntoSent
 * @param Id
 * @param Title
 * @param Recipient
 * @param Status
 * @param Delivered
 * @param Accepted
 * @return
 */
bool MainWindow::addMessageIntoSent(QStandardItemModel* model, int row, QString Id, QString Title,
    QString Recipient, QString Status, QString Delivered, QString Accepted) {

    QStandardItem *item = new QStandardItem(Id);
    model->setItem(row,0, item);
    item = new QStandardItem(Title);
    model->setItem(row,1, item);
    item = new QStandardItem(Recipient);
    model->setItem(row,2, item);
    item = new QStandardItem(Status);
    model->setItem(row,3, item);
    item = new QStandardItem(Delivered);
    model->setItem(row,4, item);
    item = new QStandardItem(Accepted);
    model->setItem(row,5, item);
    return true;
}

QString MainWindow::createAccountInfo(QString accountName) {

	QString html=QString("<h3>") + accountName + QString("</h3>");
	return html;
}


void MainWindow::setAccountInfo(QString html) {

	QTextEdit *AccountTextInfo = ui->AccountTextInfo;
	AccountTextInfo->setHtml(html);
}


