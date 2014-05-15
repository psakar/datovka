

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
#define P12FILE "p12file"


/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QStandardItemModel {

public:
	typedef QMap<QString, QVariant> SettingsMap;

	/*
	 * |
	 * +- nodeAccountTop (account X)
	 * |   |
	 * |   +- nodeRecentReceived
	 * |   +- nodeRecentSent
	 * |   +- nodeAll
	 * |      |
	 * |      +- nodeReceived
	 * |      |  |
	 * |      |  +- nodeReceivedYear (yyyy)
	 * |      |  +- nodeReceivedYear (zzzz)
	 * |      |  .
	 * |      |  .
	 * |      |  .
	 * |      |
	 * |      +- nodeSent
	 * |         |
	 * |         +- nodeSentYear (aaaa)
	 * |         +- nodeSentYear (bbbb)
	 * |         .
	 * |         .
	 * |         .
	 * |
	 * +- nodeAccountTop (account Y)
	 *    |
	 *    .
	 *    .
	 *    .
	 */
	typedef enum {
		nodeUnknown = 0,
		nodeAccountTop,
		nodeRecentReceived,
		nodeRecentSent,
		nodeAll,
		nodeReceived,
		nodeSent,
		nodeReceivedYear,
		nodeSentYear
	} NodeType;


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
	 * @brief Returns node type.
	 */
	static
	NodeType nodeType(const QModelIndex &index);

	/*!
	 * @brief Returns pointer to related top-most item.
	 */
	static
	const QStandardItem * itemTop(const QStandardItem *item);
	static
	QStandardItem * itemTop(QStandardItem *item);

	/*!
	 * @brief Returns index to related top-most item.
	 */
	static
	QModelIndex indexTop(const QModelIndex &index);

#if 0
	/*!
	 * @brief Get user name of the account.
	 */
	static
	QString userName(const QStandardItem &item);
#endif

	/*!
	 * @brief Add received year node into account.
	 */
	bool addNodeReceivedYear(QStandardItem *item, const QString &year);

	/*!
	 * @brief Add sent year node into account.
	 */
	bool addNodeSentYear(QStandardItem *item, const QString &year);

	/*!
	 * @brief Delete all year-related nodes in model.
	 */
	void removeAllYearNodes(void);
private:

};


#endif /* _ACCOUNTS_MODEL_H_ */
