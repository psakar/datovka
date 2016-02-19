/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#ifndef _ACCOUNTS_MODEL2_H_
#define _ACCOUNTS_MODEL2_H_

#include <QAbstractItemModel>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QString>
#include <QVariant>

/*!
 * @brief Account hierarchy.
 */
class AccountModel2: public QAbstractItemModel {
    Q_OBJECT

public:
	/*!
	 * @brief Holds account settings.
	 */
	class SettingsMap : public QMap<QString, QVariant> {
	public:
		/*!
		 * @brief Constructor.
		 */
		SettingsMap(void);
		SettingsMap(const QMap<QString, QVariant> &map);

		bool isValid(void) const;
		QString accountName(void) const;
		void setAccountName(const QString &name);
		QString userName(void) const;
		void setUserName(const QString &userName);
		QString loginMethod(void) const;
		void setLoginMethod(const QString &method);
		QString password(void) const;
		void setPassword(const QString &pwd);
		bool isTestAccount(void) const;
		void setTestAccount(bool isTesting);
		bool rememberPwd(void) const;
		void setRememberPwd(bool remember);
		QString dbDir(void) const;
		void setDbDir(const QString &path);
		bool syncWithAll(void) const;
		void setSyncWithAll(bool sync);
		QString p12File(void) const;
		void setP12File(const QString &p12);
		qint64 lastMsg(void) const;
		void setLastMsg(qint64 dmId);
		QString lastAttachSavePath(void) const;
		void setLastAttachSavePath(const QString &path);
		QString lastAttachAddPath(void) const;
		void setLastAttachAddPath(const QString &path);
		QString lastCorrespPath(void) const;
		void setLastCorrespPath(const QString &path);
		QString lastZFOExportPath(void) const;
		void setLastZFOExportPath(const QString &path);
		bool _createdFromScratch(void) const;
		void _setCreatedFromScratch(bool fromScratch);
		QString _passphrase(void) const;
		void _setPassphrase(const QString &passphrase);
		bool _pwdExpirDlgShown(void) const;
		void _setPwdExpirDlgShown(bool pwdExpirDlgShown);
	private:
		/* Prohibit these methods in public interface. */
		QVariant operator[](const QString &key);
		const QVariant operator[](const QString &key) const;
	};

	/*!
	 * @brief Associative array mapping user name to settings.
	 */
	class AccountsMap : public QMap<QString, SettingsMap> {
	public:
		/*!
		 * @brief Load data from supplied settings.
		 */
		void loadFromSettings(const QSettings &settings);
	};

	/*!
	 * @brief Holds account data related to account.
	 *
	 * @note Key is userName. The user name is held by the user name list.
	 */
	static
	AccountsMap globAccounts;

	/*
	 * nodeRoot (Invisible.)
	 * |
	 * +- nodeAccountTop (account X)
	 * |  |
	 * |  +- nodeRecentReceived
	 * |  +- nodeRecentSent
	 * |  +- nodeAll
	 * |     |
	 * |     +- nodeReceived
	 * |     |  |
	 * |     |  +- nodeReceivedYear (yyyy)
	 * |     |  +- nodeReceivedYear (zzzz)
	 * |     |  .
	 * |     |  .
	 * |     |  .
	 * |     |
	 * |     +- nodeSent
	 * |        |
	 * |        +- nodeSentYear (aaaa)
	 * |        +- nodeSentYear (bbbb)
	 * |        .
	 * |        .
	 * |        .
	 * |
	 * +- nodeAccountTop (account Y)
	 * |  |
	 * .  .
	 * .  .
	 * .  .
	 *
	 * The identifier number must fit into TYPE_BITS number of bits.
	 */
	enum NodeType {
		nodeUnknown = 0, /* Must start at 0. */
		nodeRoot,
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
	 * @brief Constructor.
	 *
	 * @param[in] parent Pointer to parent object.
	 */
	explicit AccountModel2(QObject *parent = 0);

	/*!
	 * @brief Return index specified by supplied parameters.
	 *
	 * @param[in] row    Item row.
	 * @param[in] column Parent column.
	 * @param[in] parent Parent index.
	 * @return Index to desired element or invalid index on error.
	 */
	virtual
	QModelIndex index(int row, int column,
	    const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Return parent index of the item with the given index.
	 *
	 * @param[in] index Child node index.
	 * @return Index of the parent node or invalid index on error.
	 */
	virtual
	QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Return number of rows under the given parent.
	 *
	 * @param[in] parent Parent node index.
	 * @return Number of rows.
	 */
	virtual
	int rowCount(const QModelIndex &parent = QModelIndex()) const
	    Q_DECL_OVERRIDE;

	/*!
	 * @brief Return the number of columns for the children of given parent.
	 *
	 * @param[in] parent Parent node index.
	 * @return Number of columns.
	 */
	virtual
	int columnCount(const QModelIndex &parent = QModelIndex()) const
	    Q_DECL_OVERRIDE;

	/*!
	 * @brief Return data stored in given location under given role.
	 *
	 * @param[in] index Index specifying the item.
	 * @param[in] role  Data role.
	 * @return Data from model.
	 */
	virtual
	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns header data in given location under given role.
	 *
	 * @brief[in] section     Header position.
	 * @brief[in] orientation Header orientation.
	 * @brief[in] role        Data role.
	 * @return Header data from model.
	 */
	virtual
	QVariant headerData(int section, Qt::Orientation orientation,
	    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns item flags for given index.
	 *
	 * @brief[in] index Index specifying the item.
	 * @return Item flags.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] settings Settings structure to be read.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 *
	 * @param[out] settings Setting structure to store the model content
	 *     into.
	 */
	void saveToSettings(QSettings &settings) const;

	/*!
	 * @brief Add account.
	 *
	 * @patam[in]  settingsMap Settings data to be added into the model.
	 * @param[out] idx         Index of newly added account if specified.
	 * @return -2 if account already exists,
	 *         -1 if account could not be added,
	 *          0 if account was added.
	 */
	int addAccount(const SettingsMap &settingsMap, QModelIndex *idx = 0);

	/*!
	 * @brief Returns node type.
	 *
	 * @param[in] index Data index.
	 * @return Node type related to the supplied index.
	 */
	static
	enum NodeType nodeType(const QModelIndex &index);

	/*!
	 * @brief Returns child node type for given row.
	 *
	 * @param[in] parentType Parent node type.
	 * @param[in] childRow   Child row.
	 * @return Child node type; unknown type on error.
	 */
	static
	enum NodeType childNodeType(enum NodeType parentType, int childRow);

	/*!
	 * @brief Returns parent node type for given child type.
	 *
	 * @note The row is set when it can be clearly determined from the node
	 *     type. If it cannot be determined then it is set to -1.
	 *
	 * @param[in]  childType Child node type.
	 * @param[out] parentRow Pointer to row value that should be set.
	 * @return Parent node type; unknown type on error.
	 */
	static
	enum NodeType parentNodeType(enum NodeType childType, int *parentRow);

private:
	/*!
	 * @brief Determines node type by traversing node structure.
	 *
	 * @param[in] index Data index.
	 * @return Node type related to the supplied index.
	 */
	static
	enum NodeType nodeTypeTraversed(const QModelIndex &index);

	QList<QString> m_userNames; /*!<
	                             * List of user names that are used to
	                             * access the global accounts.
	                             */
};

#endif /* _ACCOUNTS_MODEL2_H_ */
