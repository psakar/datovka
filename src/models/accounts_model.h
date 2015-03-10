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
#define REMEMBER_PWD "remember_password"
#define DB_DIR "database_dir"
#define SYNC_WITH_ALL "sync_with_all"
#define P12FILE "p12file"
#define LAST_MSG_ID "last_message_id"
#define LAST_SAVE_ATTACH "last_save_attach_path"
#define LAST_ADD_ATTACH "last_add_attach_path"
#define LAST_CORRESPOND "last_export_corresp_path"
#define LAST_ZFO "last_export_zfo_path"

/* Only set on new accounts. */
#define _CREATED_FROM_SCRATCH "_created_from_cratch"


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
	private: /* Prohibit these methods in public interface. */
		QVariant operator[](const QString &key);
		const QVariant operator[](const QString &key) const;
	public:
		SettingsMap(void);
		SettingsMap(const QMap<QString, QVariant> &map);
		inline QString accountName(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    ACCOUNT_NAME).toString();
		}
		inline void setAccountName(const QString &name)
		{
			QMap<QString, QVariant>::operator[](
			    ACCOUNT_NAME) = name;
		}
		inline QString userName(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    USER).toString();
		}
		inline void setUserName(const QString &userName)
		{
			QMap<QString, QVariant>::operator[](USER) = userName;
		}
		inline QString loginMethod(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    LOGIN).toString();
		}
		inline void setLoginMethod(const QString &method)
		{
			QMap<QString, QVariant>::operator[](LOGIN) = method;
		}
		inline QString password(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    PWD).toString();
		}
		inline void setPassword(const QString &pwd)
		{
			QMap<QString, QVariant>::operator[](PWD) = pwd;
		}
		inline bool isTestAccount(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    TEST_ACCOUNT).toBool();
		}
		inline void setTestAccount(bool isTesting)
		{
			QMap<QString, QVariant>::operator[](
			    TEST_ACCOUNT) = isTesting;
		}
		inline bool rememberPwd(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    REMEMBER_PWD).toBool();
		}
		inline void setRememberPwd(bool remember)
		{
			QMap<QString, QVariant>::operator[](
			    REMEMBER_PWD) = remember;
		}
		inline QString dbDir(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    DB_DIR).toString();
		}
		void setDbDir(const QString &path);
		inline bool syncWithAll(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    SYNC_WITH_ALL).toBool();
		}
		inline void setSyncWithAll(bool sync)
		{
			QMap<QString, QVariant>::operator[](
			    SYNC_WITH_ALL) = sync;
		}
		inline QString p12File(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    P12FILE).toString();
		}
		inline void setP12File(const QString &p12)
		{
			QMap<QString, QVariant>::operator[](P12FILE) = p12;
		}
		inline qint64 lastMsg(void) const
		{
			return QMap<QString, QVariant>::value(
			    LAST_MSG_ID, -1).toLongLong();
		}
		inline void setLastMsg(qint64 dmId)
		{
			QMap<QString, QVariant>::insert(
			    LAST_MSG_ID, dmId);
		}
		inline QString lastAttachSavePath(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    LAST_SAVE_ATTACH).toString();
		}
		inline void setLastAttachSavePath(const QString &path)
		{
			QMap<QString, QVariant>::operator[](
			    LAST_SAVE_ATTACH) = path;
		}
		inline QString lastAttachAddPath(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    LAST_ADD_ATTACH).toString();
		}
		inline void setLastAttachAddPath(const QString &path)
		{
			QMap<QString, QVariant>::operator[](
			    LAST_ADD_ATTACH) = path;
		}
		inline QString lastCorrespPath(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    LAST_CORRESPOND).toString();
		}
		inline void setLastCorrespPath(const QString &path)
		{
			QMap<QString, QVariant>::operator[](
			    LAST_CORRESPOND) = path;
		}
		inline QString lastZFOExportPath(void) const
		{
			return QMap<QString, QVariant>::operator[](
			    LAST_ZFO).toString();
		}
		inline void setLastZFOExportPath(const QString &path)
		{
			QMap<QString, QVariant>::operator[](LAST_ZFO) = path;
		}
		inline bool _createdFromScratch(void) const
		{
			return QMap<QString, QVariant>::value(
			    _CREATED_FROM_SCRATCH, false).toBool();
		}
		inline void _setCreatedFromScratch(bool fromScratch)
		{
			QMap<QString, QVariant>::insert(_CREATED_FROM_SCRATCH,
			    fromScratch);
		}
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
	 * @brief Returns true when node type is received.
	 */
	static
	bool nodeTypeIsReceived(const QModelIndex &index);

	/*!
	 * @brief Returns true when node type is sent.
	 */
	static
	bool nodeTypeIsSent(const QModelIndex &index);

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
