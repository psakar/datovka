

#ifndef _ACCOUNTS_MODEL_H_
#define _ACCOUNTS_MODEL_H_


#include <QSettings>
#include <QStandardItemModel>


#define NAME "name"
#define USER "username"
#define LOGIN "login_method"
#define PWD "password"
#define TEST "test_account"
#define REMEMBER "remember_password"
#define SYNC "sync_with_all"


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
	 * @brief Add account.
	 */
	bool addAccount(const QString &name,
	    const QVariant &data = QVariant());
	bool addYearItemToAccount(const QModelIndex &parent, const QString &year);
private:

};


#endif /* _ACCOUNTS_MODEL_H_ */
