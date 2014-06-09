

#ifndef _ACCOUNTS_MODEL_H_
#define _ACCOUNTS_MODEL_H_


#include <QMap>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>
#include <QVariant>


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
	class SettingsMap : public QMap<QString, QVariant> {
	public:
		SettingsMap(void);
		SettingsMap(const QMap<QString, QVariant> &map);

		QString loginMethod(void) const;
		QString userName(void) const;
		QString password(void) const;
		void setPassword(const QString &pwd);
		bool testAccount(void) const;
		QString certPath(void) const;
	};

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
	 * @brief Compute viewed data.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

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

	/*!
	 * @brief Set number of unread messages in recent received.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool updateRecentReceivedUnread(QStandardItem *item,
	    unsigned unreadMsgs = 0);

	/*!
	 * @brief Add received year node into account.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] year       Year string.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool addNodeReceivedYear(QStandardItem *item, const QString &year,
	    unsigned unreadMsgs = 0);

	/*!
	 * @brief Set number of unread messages in recent sent.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool updateRecentSentUnread(QStandardItem *item,
	    unsigned unreadMsgs = 0);

	/*!
	 * @brief Add sent year node into account.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] year       Year string.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool addNodeSentYear(QStandardItem *item, const QString &year,
	    unsigned unreadMsgs = 0);

	/*!
	 * @brief Delete all year-related nodes in model.
	 */
	void removeAllYearNodes(void);
private:

};


#endif /* _ACCOUNTS_MODEL_H_ */
