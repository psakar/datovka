

#ifndef _DATOVKA_H_
#define _DATOVKA_H_


#include <QMainWindow>
#include <QStandardItemModel>

#include "src/common.h"
#include "src/models/accounts_model.h"


namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
	QStandardItemModel * initRecievedMessageListTable();
	QStandardItemModel * initSentMessageListTable();
	void on_actionPreferences_triggered();
	bool addMessageIntoRecieved(QStandardItemModel* model,int row, QString Id, QString Title,
	QString Sender, QString Delivered, QString Accepted);
	bool addMessageIntoSent(QStandardItemModel* model,int row, QString Id, QString Title,
	QString Recipient, QString Status, QString Delivered, QString Accepted);
	void on_actionProxy_settings_triggered();
	void treeItemClicked(const QModelIndex &index);
	QString createAccountInfo(QString accountName);
	void setAccountInfo(QString html);

private:
	AccountModel accountModel;
	Ui::MainWindow *ui;
};

#endif /* _DATOVKA_H_ */
