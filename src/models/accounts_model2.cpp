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

#include "src/log/log.h"
#include "src/models/accounts_model2.h"
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

AccountModel2::SettingsMap::SettingsMap(void)
    : QMap<QString, QVariant>()
{
}

AccountModel2::SettingsMap::SettingsMap(const QMap<QString, QVariant> &map)
    : QMap<QString, QVariant>(map)
{
}

bool AccountModel2::SettingsMap::isValid(void) const
{
	return !QMap<QString, QVariant>::isEmpty() &&
	    !accountName().isEmpty() && !userName().isEmpty();
}

QString AccountModel2::SettingsMap::accountName(void) const
{
	return QMap<QString, QVariant>::operator[](ACCOUNT_NAME).toString();
}

void AccountModel2::SettingsMap::setAccountName(const QString &name)
{
	QMap<QString, QVariant>::operator[](ACCOUNT_NAME) = name;
}

QString AccountModel2::SettingsMap::userName(void) const
{
	return QMap<QString, QVariant>::operator[](USER).toString();
}

void AccountModel2::SettingsMap::setUserName(const QString &userName)
{
	QMap<QString, QVariant>::operator[](USER) = userName;
}

QString AccountModel2::SettingsMap::loginMethod(void) const
{
	return QMap<QString, QVariant>::operator[](LOGIN).toString();
}

void AccountModel2::SettingsMap::setLoginMethod(const QString &method)
{
	QMap<QString, QVariant>::operator[](LOGIN) = method;
}

QString AccountModel2::SettingsMap::password(void) const
{
	return QMap<QString, QVariant>::operator[](PWD).toString();
}

void AccountModel2::SettingsMap::setPassword(const QString &pwd)
{
	QMap<QString, QVariant>::operator[](PWD) = pwd;
}

bool AccountModel2::SettingsMap::isTestAccount(void) const
{
	return QMap<QString, QVariant>::operator[](TEST_ACCOUNT).toBool();
}

void AccountModel2::SettingsMap::setTestAccount(bool isTesting)
{
	QMap<QString, QVariant>::operator[](TEST_ACCOUNT) = isTesting;
}

bool AccountModel2::SettingsMap::rememberPwd(void) const
{
	return QMap<QString, QVariant>::operator[](REMEMBER_PWD).toBool();
}

void AccountModel2::SettingsMap::setRememberPwd(bool remember)
{
	QMap<QString, QVariant>::operator[](REMEMBER_PWD) = remember;
}

QString AccountModel2::SettingsMap::dbDir(void) const
{
	return QMap<QString, QVariant>::operator[](DB_DIR).toString();
}

void AccountModel2::SettingsMap::setDbDir(const QString &path)
{
	if (path == globPref.confDir()) {
		/* Default path is empty. */
		QMap<QString, QVariant>::operator[](DB_DIR) = QString();
	} else {
		QMap<QString, QVariant>::operator[](DB_DIR) = path;
	}
}

bool AccountModel2::SettingsMap::syncWithAll(void) const
{
	return QMap<QString, QVariant>::operator[](SYNC_WITH_ALL).toBool();
}

void AccountModel2::SettingsMap::setSyncWithAll(bool sync)
{
	QMap<QString, QVariant>::operator[](SYNC_WITH_ALL) = sync;
}

QString AccountModel2::SettingsMap::p12File(void) const
{
	return QMap<QString, QVariant>::operator[](P12FILE).toString();
}

void AccountModel2::SettingsMap::setP12File(const QString &p12)
{
	QMap<QString, QVariant>::operator[](P12FILE) = p12;
}

qint64 AccountModel2::SettingsMap::lastMsg(void) const
{
	return QMap<QString, QVariant>::value(LAST_MSG_ID, -1).toLongLong();
}

void AccountModel2::SettingsMap::setLastMsg(qint64 dmId)
{
	QMap<QString, QVariant>::insert(LAST_MSG_ID, dmId);
}

QString AccountModel2::SettingsMap::lastAttachSavePath(void) const
{
	return QMap<QString, QVariant>::operator[](LAST_SAVE_ATTACH).toString();
}

void AccountModel2::SettingsMap::setLastAttachSavePath(const QString &path)
{
	QMap<QString, QVariant>::operator[](LAST_SAVE_ATTACH) = path;
}

QString AccountModel2::SettingsMap::lastAttachAddPath(void) const
{
	return QMap<QString, QVariant>::operator[](LAST_ADD_ATTACH).toString();
}

void AccountModel2::SettingsMap::setLastAttachAddPath(const QString &path)
{
	QMap<QString, QVariant>::operator[](LAST_ADD_ATTACH) = path;
}

QString AccountModel2::SettingsMap::lastCorrespPath(void) const
{
	return QMap<QString, QVariant>::operator[](LAST_CORRESPOND).toString();
}

void AccountModel2::SettingsMap::setLastCorrespPath(const QString &path)
{
	QMap<QString, QVariant>::operator[](LAST_CORRESPOND) = path;
}

QString AccountModel2::SettingsMap::lastZFOExportPath(void) const
{
	return QMap<QString, QVariant>::operator[](LAST_ZFO).toString();
}

void AccountModel2::SettingsMap::setLastZFOExportPath(const QString &path)
{
	QMap<QString, QVariant>::operator[](LAST_ZFO) = path;
}

bool AccountModel2::SettingsMap::_createdFromScratch(void) const
{
	return QMap<QString, QVariant>::value(_CREATED_FROM_SCRATCH,
	    false).toBool();
}

void AccountModel2::SettingsMap::_setCreatedFromScratch(bool fromScratch)
{
	QMap<QString, QVariant>::insert(_CREATED_FROM_SCRATCH, fromScratch);
}

QString AccountModel2::SettingsMap::_passphrase(void) const
{
	return QMap<QString, QVariant>::value(_PKEY_PASSPHRASE,
	    QString()).toString();
}

void AccountModel2::SettingsMap::_setPassphrase(const QString &passphrase)
{
	QMap<QString, QVariant>::insert(_PKEY_PASSPHRASE, passphrase);
}

bool AccountModel2::SettingsMap::_pwdExpirDlgShown(void) const
{
	return QMap<QString, QVariant>::value(_PWD_EXPIR_DLG_SHOWN,
	    false).toBool();
}

void AccountModel2::SettingsMap::_setPwdExpirDlgShown(bool pwdExpirDlgShown)
{
	QMap<QString, QVariant>::insert(_PWD_EXPIR_DLG_SHOWN, pwdExpirDlgShown);
}

AccountModel2::AccountsMap AccountModel2::globAccounts;
enum AccountModel2::NodeType AccountModel2::m_nodeTypes[] = {
	nodeUnknown,
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

AccountModel2::AccountModel2(QObject *parent)
    : QAbstractItemModel(parent),
    m_userNames()
{
}

QModelIndex AccountModel2::index(int row, int column,
    const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	enum NodeType type = nodeUnknown; /* Parent type. */

	if (!parent.isValid()) {
		type = nodeRoot;
	} else {
		type = nodeType(parent);
	}

	type = childNodeType(row, type); /* Child type. */

	if (nodeUnknown != type) {
		return createIndex(row, column, m_nodeTypes + type);
	} else {
		return QModelIndex();
	}
}

int AccountModel2::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0) {
		return 0;
	}

	if (!parent.isValid()) {
		/* Root. */
		return m_userNames.size();
	}

	switch (nodeType(parent)) {
	case nodeAccountTop:
		return 3;
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
		return 0;
		break;
	case nodeAll:
		return 2;
		break;
	case nodeReceived:
		return 0; /* TODO -- Obtain years. */
		break;
	case nodeSent:
		return 0; /* TODO -- Obtain years. */
		break;
	default:
		return 0;
		break;
	}
}

int AccountModel2::columnCount(const QModelIndex &parent) const
{
	/* Unused */
	(void) parent;

	return 1;
}

Qt::ItemFlags AccountModel2::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return 0;
	}

	return QAbstractItemModel::flags(index) & ~Qt::ItemIsEditable;
}

int AccountModel2::addAccount(const SettingsMap &settingsMap, QModelIndex *idx)
{
	const QString userName(settingsMap.userName());

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

	m_userNames.append(userName);
	globAccounts[userName] = settingsMap;

	if (0 != idx) {
		*idx = index(m_userNames.size() - 1, 0, QModelIndex());
	}

	return 0;
}

enum AccountModel2::NodeType AccountModel2::nodeType(const QModelIndex &index)
{
	enum NodeType *typePtr =
	    static_cast<enum NodeType*>(index.internalPointer());

	if (0 == typePtr) {
		return nodeUnknown;
	} else {
		return *typePtr;
	}
}

enum AccountModel2::NodeType AccountModel2::childNodeType(int row,
    enum AccountModel2::NodeType parentType)
{
	switch (parentType) {
	case nodeUnknown:
		return nodeUnknown;
		break;
	case nodeRoot:
		return nodeAccountTop;
		break;
	case nodeAccountTop:
		switch (row) {
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
		switch (row) {
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

enum AccountModel2::NodeType AccountModel2::nodeTypeTraversed(
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
