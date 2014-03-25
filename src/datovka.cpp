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
    this->AddAccountToTree();
    this->AddAccountToTree();
    Preferences->show();
}

void MainWindow::on_actionProxy_settings_triggered()
{
    QDialog *Proxy = new ProxyDialog(this);
    Proxy->show();
}


bool MainWindow::AddAccountToTree(){

    QString name = "AHOJ";

    QTreeWidget *treeWidget = ui->AccountList;
    QTreeWidgetItem* topLevel = new QTreeWidgetItem();
    topLevel->setText(0,name);
    topLevel->icon(0);

    QTreeWidgetItem * item = new QTreeWidgetItem();
    item->setText(0,"Recent Recieved");
    topLevel->addChild(item);

    item = new QTreeWidgetItem();
    item->setText(0,"Recent Sent");
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
