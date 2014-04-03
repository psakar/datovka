

#ifndef _ACCOUNTS_MODEL_H_
#define _ACCOUNTS_MODEL_H_


#include <QSettings>
#include <QStandardItemModel>


#define CREDENTIALS "credentials"
#define NAME "name"
#define USER "username"
#define LOGIN "login_method"
#define PWD "password"
#define TEST "test_account"
#define REMEMBER "remember_password"
#define DB_DIR "database_dir"
#define SYNC "sync_with_all"


#define ROLE_SETINGS (Qt::UserRole + 1)
#define ROLE_DB (Qt::UserRole + 2)


/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QStandardItemModel {

public:
	typedef QMap<QString, QVariant> SettingsMap;

	/*!
	 * @brief Empty account model constructor.
	 */
	AccountModel(QObject *parent = 0);

	/*!
	 * @brief Load data from supplied settings.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 */
	void saveToSettings(QSettings &settings) const;

	/*!
	 * @brief Add account.
	 */
	bool addAccount(const QString &name,
	    const QVariant &data = QVariant());

	/*!
	 * @brief Returns pointer to related top-most item.
	 */
	static
	const QStandardItem * itemTop(const QStandardItem *item);

	/*!
	 * @brief Get user name of the account.
	 */
	QString userName(const QStandardItem &item);

	bool addYearItemToAccount(const QModelIndex &parent,
	    const QString &year);
private:

};


#endif /* _ACCOUNTS_MODEL_H_ */
