

#ifndef _DATOVKA_H_
#define _DATOVKA_H_


#include <QMainWindow>
#include <QStandardItemModel>

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


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

	/*!
	 * @brief Redraws widgets according to selected item.
	 */
	void treeItemClicked(const QModelIndex &index);

	/*!
	 * @brief Generates menu to selected item. (And redraw widgets.)
	 */
	void treeItemRightClicked(const QPoint &point);

	void on_actionCreate_message_triggered();

	void on_actionSent_message_triggered();

	void on_actionAdd_account_triggered();

	void on_actionDelete_account_triggered();

	void on_actionChange_password_triggered();

	void on_actionAccount_properties_triggered();

	void on_actionMove_account_up_triggered();

	void on_actionMove_account_down_triggered();

	void on_actionChange_data_directory_triggered();

	void on_actionMark_all_as_read_triggered();

	void on_actionGet_messages_triggered();

private:
	/*!
	 * @brief Get configuration directory name.
	 */
	static
	QString confDir(void);

	/*!
	 * @brief Create configuration file if not present.
	 */
	void ensureConfPresence(void) const;

	/*!
	 * @brief Load and apply setting from configuration file.
	 */
	void loadSettings(void);

	/*!
	 * @brief Store current setting to configuration file.
	 */
	void saveSettings(void) const;

	/*!
	 * @brief Sets geometry from settings.
	 */
	void loadWindowGeometry(const QSettings &settings);

	/*!
	 * @brief Store geometry to settings.
	 */
	void saveWindowGeometry(QSettings &settings) const;

	/*!
	 * @brief Generate account info HTML message.
	 */
	QString createAccountInfo(const QStandardItem &item) const;

	/*!
	 * @brief Generate overall account information.
	 */
	QString createAccountInfoAllField(const QString &accountName) const;

	/*!
	 * @brief Generate banner.
	 */
	QString createDatovkaBanner(const QString &version) const;

	QString m_confDirName; /*!< Configuration directory location. */
	QString m_confFileName; /*!< Configuration file location. */

	/* Account tree view data model. */
	AccountModel m_accountModel; /*!<
	                              * Account model. Generated from
	                              * configuration file.
	                              */
	AccountDb m_accountDb; /*!< Account information database. */
	dbContainer m_messageDbs; /*!< Map of message databases. */

	Ui::MainWindow *ui;
};


#endif /* _DATOVKA_H_ */
