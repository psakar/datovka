#include "datovka.h"
#include "ui_datovka.h"
#include "preferences.h"
#include "proxysets.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ShowOnlyInfo();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionPreferences_triggered()
{
    QDialog *Preferences = new PreferencesDialog(this);
    this->AddAccountToTree("Testovací účet 1");
    this->AddAccountToTree("Testovací účet 2");
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




    treeWidget->addTopLevelItem(topLevel);

    return true;
}



void MainWindow::ShowOnlyInfo(){

    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->ReceivedMessageList,SLOT(hide()));
    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->SentMessageList,SLOT(hide()));
    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->AttachFileList,SLOT(hide()));
    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->SaveAllButton,SLOT(hide()));
    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->SaveFileButton,SLOT(hide()));
    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->OpenFileButton,SLOT(hide()));
    connect(ui->AccountList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),ui->DeleteFileButton,SLOT(hide()));

}
