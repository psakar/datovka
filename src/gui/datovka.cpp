#include "datovka.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "ui_datovka.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QStandardItem *treerootnode = this->InitAccountListTree(tr("Accounts"));
    QStandardItemModel *tablemodel = this->InitRecievedMessageListTable();
    for(int i=0; i<3; i++) {
        this->AddMessageIntoRecieved(tablemodel, i, "12365","Dodávka světelných mečů","Orion","12.3.2014 12:12","12.3.2014 12:15");
    }
    QStandardItemModel *tablemodel2 = this->InitSentMessageListTable();
    for(int i=0; i<3; i++) {
        this->AddMessageIntoSent(tablemodel2, i, "12365","Dodávka světelných mečů","Orion","12.3.2014 12:12","12.3.2014 12:15", "Ok");
    }

    this->AddAccountToTree(treerootnode, "Testovací účet 1");
    this->AddAccountToTree(treerootnode, "Testovací účet 2");
    this->ShowOnlyInfo();

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
 * @brief MainWindow::InitAccountListTree
 * @param HeaderTitle
 * @return
 */
QStandardItem* MainWindow::InitAccountListTree(QString HeaderTitle) {

    QTreeView *AccountTreeView = ui->AccountList;
    QStandardItemModel *standardModel = new QStandardItemModel() ;
    // add header
    standardModel->setHorizontalHeaderItem(0, new QStandardItem(HeaderTitle));
    QStandardItem *rootNode = standardModel->invisibleRootItem();
    //register the model
    AccountTreeView->setModel(standardModel);

    connect(AccountTreeView, SIGNAL(clicked(QModelIndex)),
         this, SLOT(TreeItemClicked(QModelIndex)));

    return rootNode;
}

/**
 * @brief MainWindow::InitRecievedMessageListTable
 * @return
 */
QStandardItemModel* MainWindow::InitRecievedMessageListTable() {

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


QStandardItemModel* MainWindow::InitSentMessageListTable() {

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


/**
 * @brief MainWindow::AddAccountToTree add account into TreeWidget
 * @return true if success
 */
bool MainWindow::AddAccountToTree(QStandardItem* rootNode, QString AccountName){

    //defining a couple of items
    QTreeView *AccountTreeView = ui->AccountList;
    QStandardItem *Account = new QStandardItem(AccountName);
    QFont font;
    font.setBold(true);
    //font.setItalic(true);
    Account->setFont(font);
    Account->setIcon(QIcon(ICON_3PARTY_PATH + QString("letter_16.png")));

    QStandardItem *RecentRecieved = new QStandardItem(tr("Recent Recieved"));
    RecentRecieved->setIcon(QIcon(ICON_16x16_PATH + QString("datovka-message-download.png")));

    QStandardItem *RecentSent = new QStandardItem(tr("Recent Sent"));
    RecentSent->setIcon(QIcon(ICON_16x16_PATH + QString("datovka-message-reply.png")));

    QStandardItem *All = new QStandardItem(tr("All"));

    QStandardItem *AllRecieved = new QStandardItem(tr("Recent Recieved"));
    AllRecieved->setIcon(QIcon(ICON_16x16_PATH + QString("datovka-message-download.png")));

    QStandardItem *AllSent = new QStandardItem(tr("Recent Sent"));
    AllSent->setIcon(QIcon(ICON_16x16_PATH + QString("datovka-message-reply.png")));

    //building up the hierarchy
    rootNode->appendRow(Account);

    Account->appendRow(RecentRecieved);
    Account->appendRow(RecentSent);
    Account->appendRow(All);
    All->appendRow(AllRecieved);
    All->appendRow(AllSent);

    AccountTreeView->expandAll();
/*
     //selection changes shall trigger a slot
     QItemSelectionModel *selectionModel= AccountTreeView->selectionModel();
     connect(selectionModel, SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
             this, SLOT(selectionChangedSlot(const QItemSelection &, const QItemSelection &)));
*/
    return true;
}


 void MainWindow::TreeItemClicked(const QModelIndex &index)
 {

    qDebug() << index.model();
    qDebug() << index.row()  << " - " << index.column()  << " - "<< index.parent().row();

    QTableView *SentMessageList = ui->SentMessageList;
    QTableView *ReceivedMessageList = ui->ReceivedMessageList;
    QTextEdit *AccountTextInfo = ui->AccountTextInfo;
    QSplitter *splitter_2 = ui->splitter_2;

    if (index.parent().row() == -1) {
        SentMessageList->hide();
        ReceivedMessageList->hide();
        AccountTextInfo->show();
        splitter_2->hide();
    } else if (index.row() == 0) {
        SentMessageList->hide();
        ReceivedMessageList->show();
        AccountTextInfo->hide();
        splitter_2->show();
    } else if (index.row() == 1) {
        SentMessageList->show();
        ReceivedMessageList->hide();
        AccountTextInfo->hide();
        splitter_2->show();
    } else {
        SentMessageList->hide();
        ReceivedMessageList->hide();
        AccountTextInfo->show();
        splitter_2->hide();
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
bool MainWindow::AddMessageIntoRecieved(QStandardItemModel* model,int row, QString Id, QString Title,
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
bool MainWindow::AddMessageIntoSent(QStandardItemModel* model, int row, QString Id, QString Title,
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

/**
 * @brief MainWindow::ShowOnlyInfo
 */
void MainWindow::ShowOnlyInfo(){
   //connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->SentMessageList,SLOT(hide()));
   //connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->AccountTextInfo,SLOT(hide()));
}


void MainWindow::SetAccountInfotext(int Account, QString html){

    QString text="<html><body>DFgsdghdhsfghf</body></html>";
    QTextEdit *AccountTextInfo = ui->AccountTextInfo;
    AccountTextInfo->setHtml(text);

}


