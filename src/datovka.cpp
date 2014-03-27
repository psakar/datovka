#include "datovka.h"
#include "dlg_preferences.h"
#include "dlg_proxysets.h"
#include "ui_datovka.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->AddAccountToTree("Testovací účet 1");
    this->AddAccountToTree("Testovací účet 2");
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
 * @brief MainWindow::AddAccountToTree add account into TreeWidget
 * @return true if success
 */
bool MainWindow::AddAccountToTree(QString AccountName){

    QTreeWidget *treeWidget = ui->AccountList;
    QTreeWidgetItem* topLevel = new QTreeWidgetItem();
    topLevel->setText(0,AccountName);
    QFont font;
    font.setBold(true);
    font.setItalic(true);
    topLevel->setFont(0,font);
    topLevel->setIcon(0,QIcon(ICON_3PARTY_PATH + QString("letter_16.png")));
    QTreeWidgetItem * item = new QTreeWidgetItem();
    item->setText(0,tr("Recent Recieved"));
    item->setIcon(0,QIcon(ICON_16x16_PATH + QString("datovka-message-download.png")));
    topLevel->addChild(item);
    item = new QTreeWidgetItem();
    item->setText(0,tr("Recent Sent"));
    item->setIcon(0,QIcon(ICON_16x16_PATH + QString("datovka-message-reply.png")));
    topLevel->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,tr("All"));
    //item->setIcon(0,QIcon(ICON_16x16_PATH + QString("datovka-message-reply.png")));
    topLevel->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,tr("Recieved"));
    item->setIcon(0,QIcon(ICON_16x16_PATH + QString("datovka-message-download.png")));
    topLevel->child(2)->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,tr("Sent"));
    item->setIcon(0,QIcon(ICON_16x16_PATH + QString("datovka-message-reply.png")));
    topLevel->child(2)->addChild(item);

    treeWidget->addTopLevelItem(topLevel);

    return true;
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
bool MainWindow::AddMessageIntoRecieved(QString Id, QString Title,
    QString Sender, QString Delivered, QString Accepted){

    QTableWidget *TableWidget = ui->ReceivedMessageList;
    int newRow = TableWidget->rowCount();
    TableWidget->insertRow(newRow);
    QTableWidgetItem *item = new QTableWidgetItem;
    item->setText(Id);
    TableWidget->setItem(newRow,0, item);
    item = new QTableWidgetItem;
    item->setText(Title);
    TableWidget->setItem(newRow,1, item);
    item = new QTableWidgetItem;
    item->setText(Sender);
    TableWidget->setItem(newRow,2, item);
    item = new QTableWidgetItem;
    item->setText(Delivered);
    TableWidget->setItem(newRow,3, item);
    item = new QTableWidgetItem;
    item->setText(Accepted);
    TableWidget->setItem(newRow,4, item);
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
bool MainWindow::AddMessageIntoSent(QString Id, QString Title,
    QString Recipient, QString Status, QString Delivered, QString Accepted) {

    QTableWidget *TableWidget = ui->SentMessageList;
    int newRow = TableWidget->rowCount();
    TableWidget->insertRow(newRow);
    QTableWidgetItem *item = new QTableWidgetItem;
    item->setText(Id);
    TableWidget->setItem(newRow,0, item);
    item = new QTableWidgetItem;
    item->setText(Title);
    TableWidget->setItem(newRow,1, item);
    item = new QTableWidgetItem;
    item->setText(Recipient);
    TableWidget->setItem(newRow,2, item);
    item = new QTableWidgetItem;
    item->setText(Status);
    TableWidget->setItem(newRow,3, item);
    item = new QTableWidgetItem;
    item->setText(Delivered);
    TableWidget->setItem(newRow,4, item);
    item = new QTableWidgetItem;
    item->setText(Accepted);
    TableWidget->setItem(newRow,5, item);
    return true;
}

/**
 * @brief MainWindow::ShowOnlyInfo
 */
void MainWindow::ShowOnlyInfo(){
   connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->SentMessageList,SLOT(hide()));
   connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->AccountTextInfo,SLOT(hide()));
}

void MainWindow::on_actionTest_triggered()
{
        this->AddMessageIntoRecieved("12365","Dodavka svetelnych mecu","Orion","12.12.12 12:12","YES");
}
