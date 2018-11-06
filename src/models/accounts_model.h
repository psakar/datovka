/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>

#include "src/settings/accounts.h"

/* Login method descriptors. */
#define LIM_USERNAME "username"
#define LIM_CERT "certificate"
#define LIM_USER_CERT "user_certificate"
#define LIM_HOTP "hotp"
#define LIM_TOTP "totp"

/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QAbstractItemModel {
    Q_OBJECT

public:
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
	 * @brief Sorting of yearly nodes.
	 */
	enum Sorting {
		UNSORTED = 0,
		ASCENDING,
		DESCENDING
	};

	/*!
	 * @brief Unread message yearly counter.
	 */
	class YearCounter {
	public:
		YearCounter(void)
		    : dbOpened(false), unread(0)
		{ }

		YearCounter(bool o, unsigned u)
		    : dbOpened(o), unread(u)
		{ }

		bool dbOpened; /*!< True if underlying database has been opened. */
		unsigned unread; /*!< Number of unread messages. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Pointer to parent object.
	 */
	explicit AccountModel(QObject *parent = Q_NULLPTR);

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
	 * @brief Move rows.
	 *
	 * @param[in] sourceParent Source parent.
	 * @param[in] sourceRow Source row.
	 * @param[in] count Number of rows to be moved.
	 * @param[in] destinationParent Destination parent.
	 * @param[in] destinationChild Row to move data into.
	 * @return If move performed.
	 */
	virtual
	bool moveRows(const QModelIndex &sourceParent, int sourceRow,
	    int count, const QModelIndex &destinationParent,
	    int destinationChild) Q_DECL_OVERRIDE;

	/*!
	 * @brief Remove rows.
	 *
	 * @param[in] row Starting row.
	 * @param[in] count Number of rows to be removed.
	 * @param[in] parent Parent item the row is relative to.
	 * @return True if the rows were successfully removed.
	 */
	virtual
	bool removeRows(int row, int count,
	    const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns the drop actions supported by this model.
	 *
	 * @return Supported drop actions.
	 */
	virtual
	Qt::DropActions supportedDropActions(void) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns item flags for given index.
	 *
	 * @brief[in] index Index specifying the item.
	 * @return Item flags.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns the list of allowed MIME types.
	 *
	 * @return List of MIME types.
	 */
	virtual
	QStringList mimeTypes(void) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns object containing serialised attachment data.
	 *
	 * @param[in] indexes List of indexes.
	 * @return Pointer to newly allocated mime data object, Q_NULLPTR on error.
	 */
	virtual
	QMimeData *mimeData(
	    const QModelIndexList &indexes) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns whether the model accepts drops of given mime data.
	 *
	 * @param[in] data Data to be dropped.
	 * @param[in] action Type of drop action.
	 * @param[in] row Target row.
	 * @param[in] column Target column.
	 * @param[in] parent Parent index.
	 * @return True if drop is accepted.
	 */
	virtual
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
	    int row, int column,
	    const QModelIndex &parent) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Handles data supplied by drop operation.
	 *
	 * @param[in] data Data to be dropped.
	 * @param[in] action Type of drop action.
	 * @param[in] row Target row.
	 * @param[in] column Target column.
	 * @param[in] parent Parent index.
	 * @return True if data are handled by the model.
	 */
	virtual
	bool dropMimeData(const QMimeData *data, Qt::DropAction action,
	    int row, int column,
	    const QModelIndex &parent) Q_DECL_OVERRIDE;

	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] confDir Configuration directory path.
	 * @param[in] settings Settings structure to be read.
	 */
	void loadFromSettings(const QString &confDir,
	    const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 *
	 * @param[in]  pinVal PIN value to be used for password encryption.
	 * @param[in]  confDir Configuration directory path.
	 * @param[out] settings Setting structure to store the model content
	 *     into.
	 */
	void saveToSettings(const QString &pinVal, const QString &confDir,
	    QSettings &settings) const;

	/*!
	 * @brief Add account.
	 *
	 * @patam[in]  acntSettings Settings data to be added into the model.
	 * @param[out] idx          Index of newly added account if specified.
	 * @return -2 if account already exists,
	 *         -1 if account could not be added,
	 *          0 if account was added.
	 */
	int addAccount(const AcntSettings &acntSettings,
	    QModelIndex *idx = Q_NULLPTR);

	/*!
	 * @brief Delete account.
	 *
	 * @param[in] userName User name.
	 */
	void deleteAccount(const QString &userName);

	/*!
	 * @brief Returns user name for given node.
	 *
	 * @param[in] index Data index.
	 * @return User name of the account which the node belongs to
	 *     or null string on error.
	 */
	QString userName(const QModelIndex &index) const;

	/*!
	 * @brief Returns top node related to user name.
	 *
	 * @param[in] userName Sought user name.
	 * @return Top node index or invalid index if no such name found.
	 */
	QModelIndex topAcntIndex(const QString &userName) const;

	/*!
	 * @brief Returns node type.
	 *
	 * @param[in] index Data index.
	 * @return Node type related to the supplied index.
	 */
	static
	enum NodeType nodeType(const QModelIndex &index);

	/*!
	 * @brief Checks whether node type is received.
	 *
	 * @param[in] index Data index.
	 * @return True if node type is received.
	 */
	static
	bool nodeTypeIsReceived(const QModelIndex &index);

	/*!
	 * @brief Checks whether node type is sent.
	 *
	 * @param[in] index Data index.
	 * @return True if node type is received.
	 */
	static
	bool nodeTypeIsSent(const QModelIndex &index);

	/*!
	 * @brief Set number of unread messages in recent model nodes.
	 *
	 * @param[in] userName   User name.
	 * @param[in] nodeType   May be nodeRecentReceived or nodeRecentSent.
	 * @param[in] unreadMsgs Number of unread messages.
	 * @return True on success.
	 */
	bool updateRecentUnread(const QString &userName,
	    enum NodeType nodeType, unsigned unreadMsgs = 0);

	/*!
	 * @brief Update year nodes.
	 *
	 * @param[in] userName User name.
	 * @param[in] nodeType May be nodeReceivedYear or nodeSentYear.
	 * @param[in] yearlyUnreadList List of paired years and unread messages
	 *                             numbers.
	 * @param[in] sorting Sorting.
	 * @param[in] prohibitYearRemoval Set to true if year nodes should not be removed.
	 * @return True on success.
	 */
	bool updateYearNodes(const QString &userName, enum NodeType nodeType,
	    const QList< QPair<QString, YearCounter> > &yearlyUnreadList,
	    enum Sorting sorting, bool prohibitYearRemoval = false);

	/*!
	 * @brief Update existing year node in account.
	 *
	 * @param[in] userName User name.
	 * @param[in] nodeType May be nodeReceivedYear or nodeSentYear.
	 * @param[in] year Year string.
	 * @param[in] yCounter Whether database is opened and number of unread messages.
	 * @return True on success.
	 */
	bool updateYear(const QString &userName, enum NodeType nodeType,
	    const QString &year, const YearCounter &yCounter);

	/*!
	 * @brief Delete all year-related nodes in model.
	 */
	void removeAllYearNodes(void);

	/*!
	 * @brief Move related data by given number of of positions.
	 *
	 * @param[in] userName User name.
	 * @param[in] shunt    Amount of positions the account should be moved.
	 *                     Negative values move towards begin positive to
	 *                     the end.
	 * @return True on success.
	 */
	bool changePosition(const QString &userName, int shunt);

private slots:
	/*!
	 * @brief This slot handles changes of account data.
	 *
	 * @param[in] userName User name defining the changed account.
	 */
	void handleAccountDataChange(const QString &userName);

private:
	/*!
	 * @brief Delete year-related nodes in model for given account.
	 */
	void removeYearNodes(const QModelIndex &topIndex);

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

	/*!
	 * @brief Returns the row of the top node related to the account.
	 *
	 * @param[in] userName Sought user name.
	 * @return Top node row or -1 if no such name found.
	 */
	int topAcntRow(const QString &userName) const;

	/*
	 * Model indexes hold a value that is effectively the index into this
	 * list. This value serves for accessing model data. Therefore, the
	 * ordering of the list must not change during the lifespan of the
	 * model.
	 *
	 * Should an entry be deleted then only the value at the corresponding
	 * position should be set to a null string.
	 * Addition of new entries is performed by appending new values.
	 */
	QList<QString> m_userNames; /*!<
	                             * List of user names that are used to
	                             * access the global accounts.
	                             */
	/*
	 * The index into the mapping list is the actual top account row.
	 * The value stored in the field is the index into the list of user
	 * names.
	 *
	 * The size of the list is the actual number of account stored in the
	 * model.
	 */
	QList<int> m_row2UserNameIdx; /*!< Mapping from row to user name. */

	/*!
	 * @brief Holds additional information about the displayed data.
	 */
	class AccountCounters {
	public:
		/*!
		 * @brief Constructor.
		 */
		AccountCounters(void)
		    : unreadRecentReceived(0), unreadRecentSent(0),
		    receivedGroups(), sentGroups(),
		    unreadReceivedGroups(), unreadSentGroups()
		{
		}

		unsigned unreadRecentReceived; /*!< Number of unread recent received messages. */
		unsigned unreadRecentSent; /*!< Number of unread recent sent messages. */
		QList<QString> receivedGroups; /*!< Groups of unread messages. */
		QList<QString> sentGroups; /*!< Groups of unread messages. */
		QMap<QString, YearCounter> unreadReceivedGroups; /*< Number of unread messages. */
		QMap<QString, YearCounter> unreadSentGroups; /*!< Number of unread messages. */
	};

	QMap<QString, AccountCounters> m_countersMap; /*!< Unread counters. */
};
