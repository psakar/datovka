

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
	~MainWindow(void);

private slots:
	void on_actionPreferences_triggered();
	void on_actionProxy_settings_triggered();
	void treeItemClicked(const QModelIndex &index);

    void on_actionCreate_message_triggered();

    void on_actionSent_message_triggered();

private:
	/*!
	 * @brief Get configuration directory name.
	 */
	static
	QString confDir(void);

	/*!
	 * @brief Create configuration file if not present.
	 */
	void ensureConfPresence(void);

	/*!
	 * @brief Load and apply setting from configuration file.
	 */
	void loadSettings(void);

	/*!
	 * @brief Store current setting to configuration file.
	 */
	void saveSettings(void);

	/*
	 * @brief Generate account info HTML message.
	 */
	QString createAccountInfo(const QStandardItem &item);
	QString createAccountInfoAllField(const QString &accountName);
	void setAccountInfoToWidget(const QString &html);

	/* Configuration file related. */
	QString m_confDirName;
	QString m_confFileName;

	AccountModel m_accountModel;
	ReceivedMessagesRemoteModel receivedModel;
	SentMessagesRemoteModel sentModel;
	Ui::MainWindow *ui;
};

#endif /* _DATOVKA_H_ */
