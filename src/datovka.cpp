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
    topLevel->setFont(0,font);
    topLevel->setIcon(0,QIcon(":/icons/3party/letter_16.png"));

    QTreeWidgetItem * item = new QTreeWidgetItem();
    item->setText(0,"Recent Recieved");
    item->setIcon(0,QIcon(":/icons/16x16/datovka-message-download.png"));
    topLevel->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,"Recent Sent");
    item->setIcon(0,QIcon(":/icons/16x16/datovka-message-reply.png"));
    topLevel->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,"All");
    //item->setIcon(0,QIcon(":/icons/16x16/datovka-message-reply.png"));
    topLevel->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,"Recieved");
    item->setIcon(0,QIcon(":/icons/16x16/datovka-message-download.png"));
    topLevel->child(2)->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,"Sent");
    item->setIcon(0,QIcon(":/icons/16x16/datovka-message-reply.png"));
    topLevel->child(2)->addChild(item);

    treeWidget->addTopLevelItem(topLevel);

    return true;
}

/**
 * @brief MainWindow::AddMessageIntoTable
 * @return true
 */
bool MainWindow::AddMessageIntoTable(){

    QTableWidget *TableWidget = ui->ReceivedMessageList;
    int newRow = TableWidget->rowCount();
    TableWidget->insertRow(newRow);
    QTableWidgetItem *item = new QTableWidgetItem;
    item->setText("xxx1");
    TableWidget->setItem(newRow,0, item);
    item = new QTableWidgetItem;
    item->setText("xxx2");
    TableWidget->setItem(newRow,1, item);
    item = new QTableWidgetItem;
    item->setText("xxx3");
    TableWidget->setItem(newRow,2, item);
    item = new QTableWidgetItem;
    item->setText("xxx4");
    TableWidget->setItem(newRow,3, item);
    item = new QTableWidgetItem;
    item->setText("xxx5");
    TableWidget->setItem(newRow,4, item);
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
        this->AddMessageIntoTable();
}
