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

#include <QFont>
#include <QIcon>

#include "src/common.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/settings/preferences.h"

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

/* The following are not stored into the configuration file. */
/* Only set on new accounts. */
#define _CREATED_FROM_SCRATCH "_created_from_cratch"
#define _PKEY_PASSPHRASE "_pkey_passphrase"
#define _PWD_EXPIR_DLG_SHOWN "_pwd_expir_dlg_shown"

/*
 * For index navigation QModellIndex::internalId() is used. The value encodes
 * the index of the top account node (which is also the index into the array
 * of user names) and the actual node type.
 *
 * The 4 least significant bits hold the node type.
 */
#define TYPE_BITS 4
#define TYPE_MASK 0x0f

/*!
 * @brief Generates model index internal identifier.
 *
 * @param[in] uNameIdx Index into the list of user names.
 * @param[in] nodeType Node type.
 * @return Internal identifier.
 */
#define internalIdCreate(uNameIdx, nodeType) \
	((((quintptr) (uNameIdx)) << TYPE_BITS) | (nodeType))

/*!
 * @brief Change the node type in internal identifier.
 *
 * @param[in] intId    Internal identifier.
 * @param[in] nodeType New type to be set.
 * @return Internal identifier with new type.
 */
#define internalIdChangeType(intId, nodeType) \
	(((intId) & ~((quintptr) TYPE_MASK)) | (nodeType))

/*!
 * @brief Obtain node type from the internal identifier.
 *
 * @param[in] intId Internal identifier.
 * @return Node type.
 */
#define internalIdNodeType(intId) \
	((enum NodeType) ((intId) & TYPE_MASK))

/*!
 * @brief Obtain index into user name list from internal identifier.
 *
 * @param[in] indId Internal identifier.
 * @return Index into user name list.
 */
#define internalIdUserNameIndex(intId) \
	(((unsigned) (intId)) >> TYPE_BITS)

/*!
 * @brief Used for sorting credentials.
 *
 * @param[in] s1  credentials[0-9]*
 * @param[in] s2  credentials[0-9]*
 * @return True if s1 comes before s2.
 *
 * @note The number is taken by its value rather like a string of characters.
 * cred < cred1 < cred2 < ... < cred10 < ... < cred100 < ...
 */
static
bool credentialsLessThan(const QString &s1, const QString &s2)
{
	QRegExp trailingNumRe("(.*[^0-9]+)*([0-9]+)");
	QString a1, a2;
	int n1, n2;
	int pos;

	pos = trailingNumRe.indexIn(s1);
	if (pos > -1) {
		a1 = trailingNumRe.cap(1);
		n1 = trailingNumRe.cap(2).toInt();
	} else {
		a1 = s1;
		n1 = -1;
	}

	pos = trailingNumRe.indexIn(s2);
	if (pos > -1) {
		a2 = trailingNumRe.cap(1);
		n2 = trailingNumRe.cap(2).toInt();
	} else {
		a2 = s2;
		n2 = -1;
	}

	return (a1 != a2) ? (a1 < a2) : (n1 < n2);
}

AcntSettings::AcntSettings(void)
    : QMap<QString, QVariant>()
{
}

AcntSettings::AcntSettings(const QMap<QString, QVariant> &map)
    : QMap<QString, QVariant>(map)
{
}

bool AcntSettings::isValid(void) const
{
	return !m_parentType::isEmpty() &&
	    !accountName().isEmpty() && !userName().isEmpty();
}

QString AcntSettings::accountName(void) const
{
	return m_parentType::operator[](ACCOUNT_NAME).toString();
}

void AcntSettings::setAccountName(const QString &name)
{
	m_parentType::operator[](ACCOUNT_NAME) = name;
}

QString AcntSettings::userName(void) const
{
	return m_parentType::operator[](USER).toString();
}

void AcntSettings::setUserName(const QString &userName)
{
	m_parentType::operator[](USER) = userName;
}

QString AcntSettings::loginMethod(void) const
{
	return m_parentType::operator[](LOGIN).toString();
}

void AcntSettings::setLoginMethod(const QString &method)
{
	m_parentType::operator[](LOGIN) = method;
}

QString AcntSettings::password(void) const
{
	return m_parentType::operator[](PWD).toString();
}

void AcntSettings::setPassword(const QString &pwd)
{
	m_parentType::operator[](PWD) = pwd;
}

bool AcntSettings::isTestAccount(void) const
{
	return m_parentType::operator[](TEST_ACCOUNT).toBool();
}

void AcntSettings::setTestAccount(bool isTesting)
{
	m_parentType::operator[](TEST_ACCOUNT) = isTesting;
}

bool AcntSettings::rememberPwd(void) const
{
	return m_parentType::operator[](REMEMBER_PWD).toBool();
}

void AcntSettings::setRememberPwd(bool remember)
{
	m_parentType::operator[](REMEMBER_PWD) = remember;
}

QString AcntSettings::dbDir(void) const
{
	return m_parentType::operator[](DB_DIR).toString();
}

void AcntSettings::setDbDir(const QString &path)
{
	if (path == globPref.confDir()) {
		/* Default path is empty. */
		m_parentType::operator[](DB_DIR) = QString();
	} else {
		m_parentType::operator[](DB_DIR) = path;
	}
}

bool AcntSettings::syncWithAll(void) const
{
	return m_parentType::operator[](SYNC_WITH_ALL).toBool();
}

void AcntSettings::setSyncWithAll(bool sync)
{
	m_parentType::operator[](SYNC_WITH_ALL) = sync;
}

QString AcntSettings::p12File(void) const
{
	return m_parentType::operator[](P12FILE).toString();
}

void AcntSettings::setP12File(const QString &p12)
{
	m_parentType::operator[](P12FILE) = p12;
}

qint64 AcntSettings::lastMsg(void) const
{
	return m_parentType::value(LAST_MSG_ID, -1).toLongLong();
}

void AcntSettings::setLastMsg(qint64 dmId)
{
	m_parentType::insert(LAST_MSG_ID, dmId);
}

QString AcntSettings::lastAttachSavePath(void) const
{
	return m_parentType::operator[](LAST_SAVE_ATTACH).toString();
}

void AcntSettings::setLastAttachSavePath(const QString &path)
{
	m_parentType::operator[](LAST_SAVE_ATTACH) = path;
}

QString AcntSettings::lastAttachAddPath(void) const
{
	return m_parentType::operator[](LAST_ADD_ATTACH).toString();
}

void AcntSettings::setLastAttachAddPath(const QString &path)
{
	m_parentType::operator[](LAST_ADD_ATTACH) = path;
}

QString AcntSettings::lastCorrespPath(void) const
{
	return m_parentType::operator[](LAST_CORRESPOND).toString();
}

void AcntSettings::setLastCorrespPath(const QString &path)
{
	m_parentType::operator[](LAST_CORRESPOND) = path;
}

QString AcntSettings::lastZFOExportPath(void) const
{
	return m_parentType::operator[](LAST_ZFO).toString();
}

void AcntSettings::setLastZFOExportPath(const QString &path)
{
	m_parentType::operator[](LAST_ZFO) = path;
}

bool AcntSettings::_createdFromScratch(void) const
{
	return m_parentType::value(_CREATED_FROM_SCRATCH, false).toBool();
}

void AcntSettings::_setCreatedFromScratch(bool fromScratch)
{
	m_parentType::insert(_CREATED_FROM_SCRATCH, fromScratch);
}

QString AcntSettings::_passphrase(void) const
{
	return m_parentType::value(_PKEY_PASSPHRASE, QString()).toString();
}

void AcntSettings::_setPassphrase(const QString &passphrase)
{
	m_parentType::insert(_PKEY_PASSPHRASE, passphrase);
}

bool AcntSettings::_pwdExpirDlgShown(void) const
{
	return m_parentType::value(_PWD_EXPIR_DLG_SHOWN, false).toBool();
}

void AcntSettings::_setPwdExpirDlgShown(bool pwdExpirDlgShown)
{
	m_parentType::insert(_PWD_EXPIR_DLG_SHOWN, pwdExpirDlgShown);
}

void AccountsMap::loadFromSettings(const QSettings &settings)
{
	QStringList groups = settings.childGroups();
	QRegExp credRe(CREDENTIALS".*");
	AcntSettings itemSettings;

	/* Clear present rows. */
	this->clear();

	QStringList credetialList;
	/* Get list of credentials. */
	for (int i = 0; i < groups.size(); ++i) {
		/* Matches regular expression. */
		if (credRe.exactMatch(groups.at(i))) {
			credetialList.append(groups.at(i));
		}
	}

	/* Sort the credentials list. */
	qSort(credetialList.begin(), credetialList.end(), credentialsLessThan);

	/* For all credentials. */
	foreach(QString group, credetialList) {
		itemSettings.clear();
		/*
		 * String containing comma character are loaded as
		 * a string list.
		 *
		 * FIXME -- Any white-space characters trailing
		 * the comma are lost.
		 */
		itemSettings.setAccountName(settings.value(
		    group + "/" + ACCOUNT_NAME, "").toStringList().join(", "));
		itemSettings.setUserName(settings.value(
		    group + "/" + USER, "").toString());
		itemSettings.setLoginMethod(settings.value(
		    group + "/" + LOGIN, "").toString());
		itemSettings.setPassword(fromBase64(settings.value(
		    group + "/" + PWD, "").toString()));
		itemSettings.setTestAccount(settings.value(
		    group + "/" + TEST_ACCOUNT, "").toBool());
		itemSettings.setRememberPwd(settings.value(
		    group + "/" + REMEMBER_PWD, "").toBool());
		itemSettings.setDbDir(settings.value(
		    group + "/" + DB_DIR, "").toString());
		itemSettings.setSyncWithAll(settings.value(
		    group + "/" + SYNC_WITH_ALL, "").toBool());
		itemSettings.setP12File(settings.value(
		    group + "/" + P12FILE, "").toString());
		itemSettings.setLastMsg(settings.value(
		    group + "/" + LAST_MSG_ID, "").toLongLong());
		itemSettings.setLastAttachSavePath(settings.value(
		    group + "/" + LAST_SAVE_ATTACH, "").toString());
		itemSettings.setLastAttachAddPath(settings.value(
		    group + "/" + LAST_ADD_ATTACH, "").toString());
		itemSettings.setLastCorrespPath(settings.value(
		    group + "/" + LAST_CORRESPOND, "").toString());
		itemSettings.setLastZFOExportPath(settings.value(
		    group + "/" + LAST_ZFO, "").toString());

		/* Associate map with item node. */
		Q_ASSERT(!itemSettings.userName().isEmpty());
		this->operator[](itemSettings.userName()) = itemSettings;
	}
}

AccountsMap AccountModel::globAccounts;

AccountModel::AccountModel(QObject *parent)
    : QAbstractItemModel(parent),
    m_userNames(),
    m_row2UserNameIdx(),
    m_countersMap()
{
	/* Automatically handle signalled changes. */
	connect(&globAccounts, SIGNAL(accountDataChanged(QString)),
	    this, SLOT(handleAccountDataChange(QString)));
}

QModelIndex AccountModel::index(int row, int column,
    const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	enum NodeType type = nodeUnknown;

	/* Parent type. */
	if (!parent.isValid()) {
		type = nodeRoot;
	} else {
		type = nodeType(parent);
	}

	type = childNodeType(type, row); /* Child type. */

	if (nodeUnknown == type) {
		return QModelIndex();
	}

	quintptr internalId = 0;
	if (nodeAccountTop == type) {
		/* Set top node row and type. */
		Q_ASSERT(row < m_row2UserNameIdx.size());
		const int uNameIdx = m_row2UserNameIdx.at(row);
		Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));
		Q_ASSERT(!m_userNames.at(uNameIdx).isEmpty());
		internalId = internalIdCreate(uNameIdx, type);
	} else {
		/* Preserve top node row from parent, change type. */
		internalId = internalIdChangeType(parent.internalId(), type);
	}

	return createIndex(row, column, internalId);
}

QModelIndex AccountModel::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	quintptr internalId = index.internalId();

	/* Child type. */
	enum NodeType type = internalIdNodeType(internalId);

	int parentRow = -1;
	type = parentNodeType(type, &parentRow); /* Parent type. */

	if ((nodeUnknown == type) || (nodeRoot == type)) {
		return QModelIndex();
	}

	if (parentRow < 0) {
		Q_ASSERT(nodeAccountTop == type);
		/* Determine the row of the account top node. */
		const int uNameIdx = internalIdUserNameIndex(internalId);
		for (int row = 0; row < m_row2UserNameIdx.size(); ++row) {
			if (uNameIdx == m_row2UserNameIdx.at(row)) {
				parentRow = row;
				break;
			}
		}
	}

	Q_ASSERT(parentRow >= 0);

	/* Preserve top node row from child. */
	return createIndex(parentRow, 0,
	    internalIdChangeType(internalId, type));
}

int AccountModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0) {
		return 0;
	}

	if (!parent.isValid()) {
		/* Root. */
		return m_row2UserNameIdx.size();
	}

	int rows = 0;

	switch (nodeType(parent)) {
	case nodeAccountTop:
		rows = 3;
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
		rows = 0;
		break;
	case nodeAll:
		rows = 2;
		break;
	case nodeReceived:
		{
			const QString uName(userName(parent));
			Q_ASSERT(!uName.isEmpty());
			rows = m_countersMap[uName].receivedGroups.size();
		}
		break;
	case nodeSent:
		{
			const QString uName(userName(parent));
			Q_ASSERT(!uName.isEmpty());
			rows = m_countersMap[uName].sentGroups.size();
		}
		break;
	default:
		rows = 0;
		break;
	}

	return rows;
}

int AccountModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	const QString uName(userName(index));
	if (uName.isEmpty()) {
		Q_ASSERT(0);
		return QVariant();
	}
	const AcntSettings &accountInfo(globAccounts[uName]);
	enum NodeType type = internalIdNodeType(index.internalId());

	switch (role) {
	case Qt::DisplayRole:
		switch (type) {
		case nodeAccountTop:
			return accountInfo.accountName();
			break;
		case nodeRecentReceived:
			{
				QString label(tr("Recent Received"));
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				if (cntrs.unreadRecentReceived > 0) {
					label += QString(" (%1)").arg(
					    cntrs.unreadRecentReceived);
				}
				return label;
			}
			break;
		case nodeRecentSent:
			return tr("Recent Sent");
			break;
		case nodeAll:
			return tr("All");
			break;
		case nodeReceived:
			return tr("Received");
			break;
		case nodeSent:
			return tr("Sent");
			break;
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				QString label(cntrs.receivedGroups[row]);
				Q_ASSERT(!label.isEmpty());
				Q_ASSERT(
				    cntrs.unreadReceivedGroups.find(label) !=
				    cntrs.unreadReceivedGroups.constEnd());
				if (cntrs.unreadReceivedGroups[label] > 0) {
					label += QString(" (%1)").arg(
					    cntrs.unreadReceivedGroups[label]);
				}
				return label;
			}
			break;
		case nodeSentYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.sentGroups.size());
				return cntrs.sentGroups[row];
			}
			break;
		default:
			Q_ASSERT(0);
			return QVariant();
			break;
		}
		break;

	case Qt::DecorationRole:
		switch (type) {
		case nodeAccountTop:
			return QIcon(ICON_3PARTY_PATH +
			    QStringLiteral("letter_16.png"));
			break;
		case nodeRecentReceived:
		case nodeReceived:
		case nodeReceivedYear:
			return QIcon(ICON_16x16_PATH +
			    QStringLiteral("datovka-message-download.png"));
			break;
		case nodeRecentSent:
		case nodeSent:
		case nodeSentYear:
			return QIcon(ICON_16x16_PATH +
			    QStringLiteral("datovka-message-reply.png"));
			break;
		default:
			return QVariant();
			break;
		}
		break;

	case Qt::FontRole:
		switch (type) {
		case nodeAccountTop:
			{
				QFont retFont;
				retFont.setBold(true);
				return retFont;
			}
			break;
		case nodeRecentReceived:
			{
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				if (cntrs.unreadRecentReceived > 0) {
					QFont retFont;
					retFont.setBold(true);
					return retFont;
				}
			}
			break;
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				const QString &label(cntrs.receivedGroups[row]);
				Q_ASSERT(!label.isEmpty());
				Q_ASSERT(
				    cntrs.unreadReceivedGroups.find(label) !=
				    cntrs.unreadReceivedGroups.constEnd());
				if (cntrs.unreadReceivedGroups[label] > 0) {
					QFont retFont;
					retFont.setBold(true);
					return retFont;
				}
			}
			break;
		default:
			return QVariant();
			break;
		}
		break;

	case ROLE_PLAIN_DISPLAY:
		switch (type) {
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				return cntrs.receivedGroups[row];
			}
			break;
		case nodeSentYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.sentGroups.size());
				return cntrs.sentGroups[row];
			}
			break;
		default:
			return QVariant();
			break;
		}
		break;

	default:
		return QVariant();
		break;
	}

	return QVariant(); /* Is this really necessary? */
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	if ((Qt::Horizontal == orientation) && (Qt::DisplayRole == role) &&
	    (0 == section)) {
		return tr("Accounts");
	}

	return QVariant();
}

Qt::ItemFlags AccountModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return 0;
	}

	return QAbstractItemModel::flags(index) & ~Qt::ItemIsEditable;
}

void AccountModel::loadFromSettings(const QSettings &settings)
{
	/* Load into global account settings. */
	globAccounts.loadFromSettings(settings);

	QStringList groups = settings.childGroups();
	QRegExp credRe(CREDENTIALS".*");

	/* Clear present rows. */
	this->removeRows(0, this->rowCount());

	QStringList credetialList;
	/* Get list of credentials. */
	for (int i = 0; i < groups.size(); ++i) {
		/* Matches regular expression. */
		if (credRe.exactMatch(groups.at(i))) {
			credetialList.append(groups.at(i));
		}
	}

	/* Sort the credentials list. */
	qSort(credetialList.begin(), credetialList.end(), credentialsLessThan);

	beginResetModel();

	m_userNames.clear();
	m_row2UserNameIdx.clear();
	m_countersMap.clear();

	/* For all credentials. */
	foreach(const QString &group, credetialList) {
		const QString userName(settings.value(group + "/" + USER,
		    QString()).toString());

		/* Add user name into the model. */
		m_countersMap[userName] = AccountCounters();

		m_userNames.append(userName);
		m_row2UserNameIdx.append(m_userNames.size() - 1);
	}

	endResetModel();
}

void AccountModel::saveToSettings(QSettings &settings) const
{
	QString groupName;

	for (int row = 0; row < m_row2UserNameIdx.size(); ++row) {
		const int uNameIdx = m_row2UserNameIdx.at(row);
		Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));
		const QString &userName(m_userNames.at(uNameIdx));
		const AcntSettings &itemSettings(globAccounts[userName]);

		Q_ASSERT(userName == itemSettings.userName());

		groupName = CREDENTIALS;
		if (row > 0) {
			groupName.append(QString::number(row + 1));
		}
		settings.beginGroup(groupName);

		settings.setValue(ACCOUNT_NAME, itemSettings.accountName());
		settings.setValue(USER, itemSettings.userName());
		settings.setValue(LOGIN, itemSettings.loginMethod());
		settings.setValue(TEST_ACCOUNT, itemSettings.isTestAccount());
		settings.setValue(REMEMBER_PWD, itemSettings.rememberPwd());
		if (itemSettings.rememberPwd()) {
			if (!itemSettings.password().isEmpty()) {
				settings.setValue(PWD,
				    toBase64(itemSettings.password()));
			}
		}

		if (!itemSettings.dbDir().isEmpty()) {
			if (itemSettings.dbDir() != globPref.confDir()) {
				settings.setValue(DB_DIR,
				    itemSettings.dbDir());
			}
		}
		if (!itemSettings.p12File().isEmpty()) {
			settings.setValue(P12FILE, itemSettings.p12File());
		}

		settings.setValue(SYNC_WITH_ALL, itemSettings.syncWithAll());

		if (0 <= itemSettings.lastMsg()) {
			settings.setValue(LAST_MSG_ID, itemSettings.lastMsg());
		}

		/* Save last attachments save path. */
		if (!itemSettings.lastAttachSavePath().isEmpty()) {
			settings.setValue(LAST_SAVE_ATTACH,
			    itemSettings.lastAttachSavePath());
		}

		/* Save last attachments add path. */
		if (!itemSettings.lastAttachAddPath().isEmpty()) {
			settings.setValue(LAST_ADD_ATTACH,
			    itemSettings.lastAttachAddPath());
		}

		/* Save last correspondence export path. */
		if (!itemSettings.lastCorrespPath().isEmpty()) {
			settings.setValue(LAST_CORRESPOND,
			    itemSettings.lastCorrespPath());
		}

		/* save last ZFO export path */
		if (!itemSettings.lastZFOExportPath().isEmpty()) {
			settings.setValue(LAST_ZFO,
			    itemSettings.lastZFOExportPath());
		}

		settings.endGroup();
	}
}

int AccountModel::addAccount(const AcntSettings &acntSettings, QModelIndex *idx)
{
	const QString userName(acntSettings.userName());

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return -1;
	}

	if (globAccounts.find(userName) != globAccounts.end()) {
		logErrorNL("Account with user name '%s' already exists.",
		    userName.toUtf8().constData());
		return -2;
	}

	Q_ASSERT(!m_userNames.contains(userName));

	beginInsertRows(QModelIndex(), rowCount(), rowCount());

	globAccounts[userName] = acntSettings;

	m_countersMap[userName] = AccountCounters();

	m_userNames.append(userName);
	m_row2UserNameIdx.append(m_userNames.size() - 1);

	endInsertRows();

	if (0 != idx) {
		*idx = index(m_row2UserNameIdx.size() - 1, 0, QModelIndex());
	}

	return 0;
}

void AccountModel::deleteAccount(const QString &userName)
{
	int row = topAcntRow(userName);

	if (row < 0) {
		Q_ASSERT(0);
		return;
	}

	int uNameIdx = m_row2UserNameIdx.at(row);
	Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));
	Q_ASSERT(userName == m_userNames.at(uNameIdx));

	beginRemoveRows(QModelIndex(), row, row);

	m_userNames[uNameIdx] = QString();
	m_row2UserNameIdx.removeAt(row);

	m_countersMap.remove(userName);

	globAccounts.remove(userName);

	endRemoveRows();
}

QString AccountModel::userName(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QString();
	}

	int uNameIdx = internalIdUserNameIndex(index.internalId());

	if (uNameIdx < m_userNames.size()) {
		return m_userNames.at(uNameIdx);
	} else {
		return QString();
	}
}

QModelIndex AccountModel::topAcntIndex(const QString &userName) const
{
	int row = topAcntRow(userName);
	if (row >= 0) {
		return index(row, 0);
	}

	return QModelIndex();
}

enum AccountModel::NodeType AccountModel::nodeType(const QModelIndex &index)
{
	if (!index.isValid()) {
		return nodeUnknown; /* nodeRoot ? */
	}

	/* TODO -- Add runtime value check? */
	return internalIdNodeType(index.internalId());
}

bool AccountModel::nodeTypeIsReceived(const QModelIndex &index)
{
	switch (nodeType(index)) {
	case nodeRecentReceived:
	case nodeReceived:
	case nodeReceivedYear:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool AccountModel::nodeTypeIsSent(const QModelIndex &index)
{
	switch (nodeType(index)) {
	case nodeRecentSent:
	case nodeSent:
	case nodeSentYear:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool AccountModel::updateRecentUnread(const QString &userName,
    enum AccountModel::NodeType nodeType, unsigned unreadMsgs)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QModelIndex topIndex(topAcntIndex(userName));
	if (!topIndex.isValid()) {
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	unsigned *unreadRecent = 0;
	QModelIndex childIndex;

	if (nodeRecentReceived == nodeType) {
		unreadRecent = &cntrs.unreadRecentReceived;
		/* Get recently received node. */
		childIndex = topIndex.child(0, 0);
	} else if (nodeRecentSent == nodeType) {
		unreadRecent = &cntrs.unreadRecentSent;
		/* Get recently sent node. */
		childIndex = topIndex.child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	Q_ASSERT(0 != unreadRecent);
	Q_ASSERT(childIndex.isValid());

	*unreadRecent = unreadMsgs;
	emit dataChanged(childIndex, childIndex);

	return true;
}

bool AccountModel::appendYear(const QString &userName,
    enum AccountModel::NodeType nodeType, const QString &year,
    unsigned unreadMsgs)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QModelIndex topIndex(topAcntIndex(userName));
	if (!topIndex.isValid()) {
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	QList<QString> *groups = 0;
	QMap<QString, unsigned> *unreadGroups = 0;
	QModelIndex childTopIndex;

	if (nodeReceivedYear == nodeType) {
		groups = &cntrs.receivedGroups;
		unreadGroups = &cntrs.unreadReceivedGroups;
		/* Get received node. */
		childTopIndex = topIndex.child(2, 0).child(0, 0);
	} else if (nodeSentYear == nodeType) {
		groups = &cntrs.sentGroups;
		unreadGroups = &cntrs.unreadSentGroups;
		/* Get sent node. */
		childTopIndex = topIndex.child(2, 0).child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	Q_ASSERT(0 != groups);
	Q_ASSERT(0 != unreadGroups);
	Q_ASSERT(childTopIndex.isValid());

	int rows = groups->size();
	beginInsertRows(childTopIndex, rows, rows);
	groups->append(year);
	(*unreadGroups)[year] = unreadMsgs;
	endInsertRows();

	return true;
}

bool AccountModel::updateYearNodes(const QString &userName,
    enum AccountModel::NodeType nodeType,
    const QList< QPair<QString, unsigned> > &yearlyUnreadList)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QModelIndex topIndex(topAcntIndex(userName));
	if (!topIndex.isValid()) {
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	QList<QString> *groups = 0;
	QMap<QString, unsigned> *unreadGroups = 0;
	QModelIndex childTopIndex;

	if (nodeReceivedYear == nodeType) {
		groups = &cntrs.receivedGroups;
		unreadGroups = &cntrs.unreadReceivedGroups;
		/* Get received node. */
		childTopIndex = topIndex.child(2, 0).child(0, 0);
	} else if (nodeSentYear == nodeType) {
		groups = &cntrs.sentGroups;
		unreadGroups = &cntrs.unreadSentGroups;
		/* Get sent node. */
		childTopIndex = topIndex.child(2, 0).child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	Q_ASSERT(0 != groups);
	Q_ASSERT(0 != unreadGroups);
	Q_ASSERT(childTopIndex.isValid());

	/* Delete old. */
	int rows = groups->size();
	if (rows > 0) {
		beginRemoveRows(childTopIndex, 0, rows - 1);
		groups->clear();
		unreadGroups->clear();
		endRemoveRows();
	}

	/* Add new. */
	rows = yearlyUnreadList.size();
	if (rows > 0) {
		beginInsertRows(childTopIndex, 0, rows - 1);
		typedef QPair<QString, unsigned> Pair;
		foreach (const Pair &pair, yearlyUnreadList) {
			groups->append(pair.first);
			(*unreadGroups)[pair.first] = pair.second;
		}
		endInsertRows();
	}

	return true;
}

bool AccountModel::updateYear(const QString &userName,
    enum AccountModel::NodeType nodeType, const QString &year,
    unsigned unreadMsgs)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QModelIndex topIndex(topAcntIndex(userName));
	if (!topIndex.isValid()) {
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	QList<QString> *groups = 0;
	QMap<QString, unsigned> *unreadGroups = 0;
	QModelIndex childIndex; /* Top index of children. */

	if (nodeReceivedYear == nodeType) {
		groups = &cntrs.receivedGroups;
		unreadGroups = &cntrs.unreadReceivedGroups;
		/* Get received node. */
		childIndex = topIndex.child(2, 0).child(0, 0);
	} else if (nodeSentYear == nodeType) {
		groups = &cntrs.sentGroups;
		unreadGroups = &cntrs.unreadSentGroups;
		/* Get sent node. */
		childIndex = topIndex.child(2, 0).child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	Q_ASSERT(0 != groups);
	Q_ASSERT(0 != unreadGroups);
	Q_ASSERT(childIndex.isValid());

	int row = 0;
	for (; row < groups->size(); ++row) {
		if (year == groups->at(row)) {
			break;
		}
	}
	if (row >= groups->size()) {
		return false;
	}
	childIndex = childIndex.child(row, 0); /* Child index. */

	Q_ASSERT(childIndex.isValid());

	(*unreadGroups)[year] = unreadMsgs;
	emit dataChanged(childIndex, childIndex);

	return true;
}

void AccountModel::removeAllYearNodes(void)
{
	for (int row = 0; row < rowCount(); ++row) {
		QModelIndex topIndex(index(row, 0));

		Q_ASSERT(topIndex.isValid());

		removeYearNodes(topIndex);
	}
}

bool AccountModel::changePosition(const QString &userName, int shunt)
{
	if (0 == shunt) {
		return false;
	}

	int row = topAcntRow(userName);
	if ((row < 0) || (row >= m_row2UserNameIdx.size())) {
		Q_ASSERT(0);
		return false;
	}
	int newRow = row + shunt;
	if ((newRow < 0) || (newRow >= m_row2UserNameIdx.size())) {
		return false;
	}

	int destChild = newRow;

	if (shunt > 0) {
		/*
		 * Because we move one item.
		 * See QAbstractItemModel::beginMoveRows() documentation.
		 */
		destChild += 1;
	}

	beginMoveRows(QModelIndex(), row, row, QModelIndex(), destChild);

	int auxNameIdx = m_row2UserNameIdx.at(row);
	m_row2UserNameIdx[row] = m_row2UserNameIdx.at(newRow);
	m_row2UserNameIdx[newRow] = auxNameIdx;

	endMoveRows();

	return true;
}

void AccountModel::handleAccountDataChange(const QString &userName)
{
	QModelIndex topIndex(topAcntIndex(userName));

	if (topIndex.isValid()) {
		emit dataChanged(topIndex, topIndex);
	}
}

void AccountModel::removeYearNodes(const QModelIndex &topIndex)
{
	Q_ASSERT(topIndex.isValid());

	const QString uName(userName(topIndex));
	Q_ASSERT(!uName.isEmpty());

	AccountCounters &cntrs(m_countersMap[uName]);

	/* Received. */
	QModelIndex childTopIndex = topIndex.child(2, 0).child(0, 0);
	int rows = rowCount(childTopIndex);
	if (rows > 0) {
		beginRemoveRows(childTopIndex, 0, rows - 1);
		cntrs.receivedGroups.clear();
		cntrs.unreadReceivedGroups.clear();
		endRemoveRows();
	}

	/* Sent. */
	childTopIndex = topIndex.child(2, 0).child(1, 0);
	rows = rowCount(childTopIndex);
	if (rows > 0) {
		beginRemoveRows(childTopIndex, 0, rows - 1);
		cntrs.sentGroups.clear();
		cntrs.unreadSentGroups.clear();
		endRemoveRows();
	}
}

enum AccountModel::NodeType AccountModel::childNodeType(
    enum AccountModel::NodeType parentType, int childRow)
{
	switch (parentType) {
	case nodeUnknown:
		return nodeUnknown;
		break;
	case nodeRoot:
		return nodeAccountTop;
		break;
	case nodeAccountTop:
		switch (childRow) {
		case 0:
			return nodeRecentReceived;
			break;
		case 1:
			return nodeRecentSent;
			break;
		case 2:
			return nodeAll;
			break;
		default:
			return nodeUnknown;
			break;
		}
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
		return nodeUnknown;
		break;
	case nodeAll:
		switch (childRow) {
		case 0:
			return nodeReceived;
			break;
		case 1:
			return nodeSent;
			break;
		default:
			return nodeUnknown;
			break;
		}
		break;
	case nodeReceived:
		return nodeReceivedYear;
		break;
	case nodeSent:
		return nodeSentYear;
		break;
	case nodeReceivedYear:
	case nodeSentYear:
		return nodeUnknown;
		break;
	default:
		Q_ASSERT(0);
		return nodeUnknown;
		break;
	}
}

enum AccountModel::NodeType AccountModel::parentNodeType(
    enum AccountModel::NodeType childType, int *parentRow)
{
	switch (childType) {
	case nodeUnknown:
	case nodeRoot:
		if (0 != parentRow) {
			*parentRow = -1;
		}
		return nodeUnknown;
		break;
	case nodeAccountTop:
		if (0 != parentRow) {
			*parentRow = -1;
		}
		return nodeRoot;
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
	case nodeAll:
		if (0 != parentRow) {
			*parentRow = -1;
		}
		return nodeAccountTop;
		break;
	case nodeReceived:
	case nodeSent:
		if (0 != parentRow) {
			*parentRow = 2;
		}
		return nodeAll;
		break;
	case nodeReceivedYear:
		if (0 != parentRow) {
			*parentRow = 0;
		}
		return nodeReceived;
		break;
	case nodeSentYear:
		if (0 != parentRow) {
			*parentRow = 1;
		}
		return nodeSent;
		break;
	default:
		Q_ASSERT(0);
		if (0 != parentRow) {
			*parentRow = -1;
		}
		return nodeUnknown;
		break;
	}
}

int AccountModel::topAcntRow(const QString &userName) const
{
	int foundRow = -1;

	for (int row = 0; row < m_row2UserNameIdx.size(); ++row) {
		const int uNameIdx = m_row2UserNameIdx.at(row);
		Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));

		if (userName == m_userNames.at(uNameIdx)) {
			foundRow = row;
			break;
		}
	}

	return foundRow;
}

enum AccountModel::NodeType AccountModel::nodeTypeTraversed(
    const QModelIndex &index)
{
	if (-1 == index.parent().row()) {
		return nodeAccountTop;
	} else if (-1 == index.parent().parent().row()) {
		switch (index.row()) {
		case 0:
			return nodeRecentReceived;
			break;
		case 1:
			return nodeRecentSent;
			break;
		case 2:
			return nodeAll;
			break;
		default:
			return nodeUnknown;
			break;
		}
	} else if (-1 == index.parent().parent().parent().row()) {
		switch (index.row()) {
		case 0:
			return nodeReceived;
			break;
		case 1:
			return nodeSent;
			break;
		default:
			return nodeUnknown;
			break;
		}
	} else if (-1 == index.parent().parent().parent().parent().row()) {
		switch (index.parent().row()) {
		case 0:
			return nodeReceivedYear;
			break;
		case 1:
			return nodeSentYear;
			break;
		default:
			return nodeUnknown;
			break;
		}
	} else {
		return nodeUnknown;
	}
}
