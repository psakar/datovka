#ifndef _DATOVKA_H_
#define _DATOVKA_H_

#include "src/common.h"
#include <QMainWindow>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    QStandardItem* InitAccountListTree(QString HeaderTitle);
    QStandardItemModel* InitRecievedMessageListTable();
    QStandardItemModel* InitSentMessageListTable();
    void on_actionPreferences_triggered();
    void ShowOnlyInfo();
    bool AddAccountToTree(QStandardItem* rootNode, QString AccountName);
    bool AddMessageIntoRecieved(QStandardItemModel* model,int row, QString Id, QString Title,
    QString Sender, QString Delivered, QString Accepted);
    bool AddMessageIntoSent(QStandardItemModel* model,int row, QString Id, QString Title,
    QString Recipient, QString Status, QString Delivered, QString Accepted);
    void on_actionProxy_settings_triggered();
    void TreeItemClicked(const QModelIndex &index);
    void SetAccountInfotext(int Account, QString html);

private:
	Ui::MainWindow *ui;
};

#endif /* _DATOVKA_H_ */
