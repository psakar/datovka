

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
#define LASTMSG "last_message_id"


/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QStandardItemModel {

public:
	class SettingsMap : public QMap<QString, QVariant> {
	public:
		SettingsMap(void);
		SettingsMap(const QMap<QString, QVariant> &map);
		QString accountName(void) const;
		QString loginMethod(void) const;
		QString userName(void) const;
		QString password(void) const;
		void setPassword(QString &pwd);
		void setDirectory(const QString &path);
		void setLastMsg(const QString &dmId);
		bool testAccount(void) const;
		QString certPath(void) const;
		QString lastMsg(void) const;
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
	QModelIndex addAccount(const QString &name,
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
	 * @brief Return related settings map.
	 *
	 * @param[in] item Selected account item.
	 */
	static
	SettingsMap settingsMap(QStandardItem *item);

	/*!
	 * @brief Set settings map to related account.
	 *
	 * @param[in] item Selected account item.
	 * @param[in] map  Settings map to be assigned.
	 * @retunr True if successful.
	 */
	static
	void setSettingsMap(QStandardItem *item, const SettingsMap &map);

	/*!
	 * @brief Set number of unread messages in recent model nodes.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] nodeType   May be nodeRecentReceived or nodeRecentSent.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool updateRecentUnread(QStandardItem *item, NodeType nodeType,
	    unsigned unreadMsgs = 0);

	/*!
	 * @brief Add year node into account.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] nodeType   May be nodeReceivedYear or nodeSentYear.
	 * @param[in] year       Year string.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool addYear(QStandardItem *item, NodeType nodeType,
	    const QString &year, unsigned unreadMsgs = 0);

	/*!
	 * @brief Update existing year node in account.
	 *
	 * @param[in] item       Some item identifying the processed account.
	 * @param[in] nodeType   May be nodeReceivedYear or nodeSentYear.
	 * @param[in] year       Year string.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool updateYear(QStandardItem *item, NodeType nodeType,
	    const QString &year, unsigned unreadMsgs = 0);

	/*!
	 * @brief Delete year-related nodes in model for given account.
	 */
	void removeYearNodes(const QModelIndex &topIndex);

	/*!
	 * @brief Delete all year-related nodes in model.
	 */
	void removeAllYearNodes(void);
private:

};


#endif /* _ACCOUNTS_MODEL_H_ */
