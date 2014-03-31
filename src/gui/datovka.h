

#ifndef _DATOVKA_H_
#define _DATOVKA_H_


#include <QMainWindow>
#include <QStandardItemModel>

#include "src/common.h"
#include "src/models/accounts_model.h"
#include "src/models/messages_remote_models.h"


namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_actionPreferences_triggered();
	void on_actionProxy_settings_triggered();
	void treeItemClicked(const QModelIndex &index);
	QString createAccountInfo(QString accountName);
	void setAccountInfo(QString html);

private:
	AccountModel accountModel;
	ReceivedMessagesRemoteModel receivedModel;
	SentMessagesRemoteModel sentModel;
	Ui::MainWindow *ui;
};

#endif /* _DATOVKA_H_ */
