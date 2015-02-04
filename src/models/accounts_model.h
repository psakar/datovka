/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */


#ifndef _ACCOUNTS_MODEL_H_
#define _ACCOUNTS_MODEL_H_


#include <QMap>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>
#include <QVariant>


#define CREDENTIALS "credentials"
#define ACCOUNT_NAME "name"
#define USER "username"
#define LOGIN "login_method"
#define PWD "password"
#define TEST_ACCOUNT "test_account"
#define REMEMBER "remember_password"
#define DB_DIR "database_dir"
#define SYNC "sync_with_all"
#define P12FILE "p12file"
#define LASTMSG "last_message_id"
#define LASTATTACH "last_attach_path"
#define LASTCORRESP "last_export_corresp_path"
#define LASTZFO "last_export_zfo_path"


/* Login method descriptors. */
#define LIM_USERNAME "username"
#define LIM_CERT "certificate"
#define LIM_USER_CERT "user_certificate"
#define LIM_HOTP "hotp"
#define LIM_TOTP "totp"


/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QStandardItemModel {
	Q_OBJECT

public:
	class SettingsMap : public QMap<QString, QVariant> {
	public:
		SettingsMap(void);
		SettingsMap(const QMap<QString, QVariant> &map);
		inline QString accountName(void) const
		{
			return (*this)[ACCOUNT_NAME].toString();
		}
		inline void setAccountName(const QString &name)
		{
			(*this)[ACCOUNT_NAME] = name;
		}
		inline QString userName(void) const
		{
			return (*this)[USER].toString();
		}
		inline void setUserName(const QString &userName)
		{
			(*this)[USER] = userName;
		}
		inline QString loginMethod(void) const
		{
			return (*this)[LOGIN].toString();
		}
		inline void setLoginMethod(const QString &method)
		{
			(*this)[LOGIN] = method;
		}
		inline QString password(void) const
		{
			return (*this)[PWD].toString();
		}
		inline void setPassword(const QString &pwd)
		{
			(*this)[PWD] = pwd;
		}
		inline bool isTestAccount(void) const
		{
			return (*this)[TEST_ACCOUNT].toBool();
		}
		inline void setTestAccount(bool isTesting)
		{
			(*this)[TEST_ACCOUNT] = isTesting;
		}
		inline QString dbDir(void) const
		{
			return (*this)[DB_DIR].toString();
		}
		void setDbDir(const QString &path);
		void setLastMsg(const QString &dmId);
		void setLastAttachPath(const QString &path);
		void setLastCorrespPath(const QString &path);
		void setLastZFOExportPath(const QString &path);
		QString certPath(void) const;
		QString lastMsg(void) const;
		QString lastAttachPath(void) const;
		QString lastCorrespPath(void) const;
		QString lastZFOExportPath(void) const;
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
	enum NodeType {
		nodeUnknown = 0,
		nodeAccountTop,
		nodeRecentReceived,
		nodeRecentSent,
		nodeAll,
		nodeReceived,
		nodeSent,
		nodeReceivedYear,
		nodeSentYear
	};


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
