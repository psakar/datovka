#include "datovka.h"
#include "ui_datovka.h"
#include "preferences.h"
#include "proxysets.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
