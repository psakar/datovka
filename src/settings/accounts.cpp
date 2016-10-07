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

#include "src/common.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"

namespace CredNames {
	const QString creds(QLatin1String("credentials"));

	const QString acntName(QLatin1String("name"));
	const QString userName(QLatin1String("username"));
	const QString lMethod(QLatin1String("login_method"));
	const QString pwd(QLatin1String("password"));
	const QString testAcnt(QLatin1String("test_account"));
	const QString rememberPwd(QLatin1String("remember_password"));
	const QString dbDir(QLatin1String("database_dir"));
	const QString syncWithAll(QLatin1String("sync_with_all"));
	const QString p12File(QLatin1String("p12file"));
	const QString lstMsgId(QLatin1String("last_message_id"));
	const QString lstSaveAtchPath(QLatin1String("last_save_attach_path"));
	const QString lstAddAtchPath(QLatin1String("last_add_attach_path"));
	const QString lstCorrspPath(QLatin1String("last_export_corresp_path"));
	const QString lstZfoPath(QLatin1String("last_export_zfo_path"));
}

/* The following are not stored into the configuration file. */
/* Only set on new accounts. */
#define _CREATED_FROM_SCRATCH "_created_from_cratch"
#define _PKEY_PASSPHRASE "_pkey_passphrase"
#define _PWD_EXPIR_DLG_SHOWN "_pwd_expir_dlg_shown"

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
	return m_parentType::operator[](CredNames::acntName).toString();
}

void AcntSettings::setAccountName(const QString &name)
{
	m_parentType::operator[](CredNames::acntName) = name;
}

QString AcntSettings::userName(void) const
{
	return m_parentType::operator[](CredNames::userName).toString();
}

void AcntSettings::setUserName(const QString &userName)
{
	m_parentType::operator[](CredNames::userName) = userName;
}

QString AcntSettings::loginMethod(void) const
{
	return m_parentType::operator[](CredNames::lMethod).toString();
}

void AcntSettings::setLoginMethod(const QString &method)
{
	m_parentType::operator[](CredNames::lMethod) = method;
}

QString AcntSettings::password(void) const
{
	return m_parentType::operator[](CredNames::pwd).toString();
}

void AcntSettings::setPassword(const QString &pwd)
{
	m_parentType::operator[](CredNames::pwd) = pwd;
}

bool AcntSettings::isTestAccount(void) const
{
	return m_parentType::operator[](CredNames::testAcnt).toBool();
}

void AcntSettings::setTestAccount(bool isTesting)
{
	m_parentType::operator[](CredNames::testAcnt) = isTesting;
}

bool AcntSettings::rememberPwd(void) const
{
	return m_parentType::operator[](CredNames::rememberPwd).toBool();
}

void AcntSettings::setRememberPwd(bool remember)
{
	m_parentType::operator[](CredNames::rememberPwd) = remember;
}

QString AcntSettings::dbDir(void) const
{
	return m_parentType::operator[](CredNames::dbDir).toString();
}

void AcntSettings::setDbDir(const QString &path)
{
	if (path == globPref.confDir()) {
		/* Default path is empty. */
		m_parentType::operator[](CredNames::dbDir) = QString();
	} else {
		m_parentType::operator[](CredNames::dbDir) = path;
	}
}

bool AcntSettings::syncWithAll(void) const
{
	return m_parentType::operator[](CredNames::syncWithAll).toBool();
}

void AcntSettings::setSyncWithAll(bool sync)
{
	m_parentType::operator[](CredNames::syncWithAll) = sync;
}

QString AcntSettings::p12File(void) const
{
	return m_parentType::operator[](CredNames::p12File).toString();
}

void AcntSettings::setP12File(const QString &p12)
{
	m_parentType::operator[](CredNames::p12File) = p12;
}

qint64 AcntSettings::lastMsg(void) const
{
	return m_parentType::value(CredNames::lstMsgId, -1).toLongLong();
}

void AcntSettings::setLastMsg(qint64 dmId)
{
	m_parentType::insert(CredNames::lstMsgId, dmId);
}

QString AcntSettings::lastAttachSavePath(void) const
{
	return m_parentType::operator[](CredNames::lstSaveAtchPath).toString();
}

void AcntSettings::setLastAttachSavePath(const QString &path)
{
	m_parentType::operator[](CredNames::lstSaveAtchPath) = path;
}

QString AcntSettings::lastAttachAddPath(void) const
{
	return m_parentType::operator[](CredNames::lstAddAtchPath).toString();
}

void AcntSettings::setLastAttachAddPath(const QString &path)
{
	m_parentType::operator[](CredNames::lstAddAtchPath) = path;
}

QString AcntSettings::lastCorrespPath(void) const
{
	return m_parentType::operator[](CredNames::lstCorrspPath).toString();
}

void AcntSettings::setLastCorrespPath(const QString &path)
{
	m_parentType::operator[](CredNames::lstCorrspPath) = path;
}

QString AcntSettings::lastZFOExportPath(void) const
{
	return m_parentType::operator[](CredNames::lstZfoPath).toString();
}

void AcntSettings::setLastZFOExportPath(const QString &path)
{
	m_parentType::operator[](CredNames::lstZfoPath) = path;
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

void AcntSettings::loadFromSettings(const QSettings &settings,
    const QString &group)
{
	QString prefix;
	if (!group.isEmpty()) {
		prefix = group + "/";
	}

	/*
	 * String containing comma character are loaded as a string list.
	 *
	 * FIXME -- Any white-space characters trailing the comma are lost.
	 */
	setAccountName(settings.value(prefix + CredNames::acntName,
	    "").toStringList().join(", "));
	setUserName(settings.value(prefix + CredNames::userName,
	    "").toString());
	setLoginMethod(settings.value(prefix + CredNames::lMethod,
	    "").toString());
	setPassword(fromBase64(settings.value(prefix + CredNames::pwd,
	    "").toString()));
	setTestAccount(settings.value(prefix + CredNames::testAcnt,
	    "").toBool());
	setRememberPwd(settings.value(prefix + CredNames::rememberPwd,
	    "").toBool());
	setDbDir(settings.value(prefix + CredNames::dbDir,
	    "").toString());
	setSyncWithAll(settings.value(prefix + CredNames::syncWithAll,
	    "").toBool());
	setP12File(settings.value(prefix + CredNames::p12File,
	    "").toString());
	setLastMsg(settings.value(prefix + CredNames::lstMsgId,
	    "").toLongLong());
	setLastAttachSavePath(settings.value(prefix + CredNames::lstSaveAtchPath,
	    "").toString());
	setLastAttachAddPath(settings.value(prefix + CredNames::lstAddAtchPath,
	    "").toString());
	setLastCorrespPath(settings.value(prefix + CredNames::lstCorrspPath,
	    "").toString());
	setLastZFOExportPath(settings.value(prefix + CredNames::lstZfoPath,
	    "").toString());
}

void AcntSettings::saveToSettings(QSettings &settings,
    const QString &group) const
{
	if (!group.isEmpty()) {
		settings.beginGroup(group);
	}

	settings.setValue(CredNames::acntName, accountName());
	settings.setValue(CredNames::userName, userName());
	settings.setValue(CredNames::lMethod, loginMethod());
	settings.setValue(CredNames::testAcnt, isTestAccount());
	settings.setValue(CredNames::rememberPwd, rememberPwd());
	if (rememberPwd()) {
		if (!password().isEmpty()) {
			settings.setValue(CredNames::pwd, toBase64(password()));
		}
	}

	if (!dbDir().isEmpty()) {
		if (dbDir() != globPref.confDir()) {
			settings.setValue(CredNames::dbDir, dbDir());
		}
	}
	if (!p12File().isEmpty()) {
		settings.setValue(CredNames::p12File, p12File());
	}

	settings.setValue(CredNames::syncWithAll, syncWithAll());

	if (0 <= lastMsg()) {
		settings.setValue(CredNames::lstMsgId, lastMsg());
	}

	/* Save last attachments save path. */
	if (!lastAttachSavePath().isEmpty()) {
		settings.setValue(CredNames::lstSaveAtchPath,
		    lastAttachSavePath());
	}

	/* Save last attachments add path. */
	if (!lastAttachAddPath().isEmpty()) {
		settings.setValue(CredNames::lstAddAtchPath,
		    lastAttachAddPath());
	}

	/* Save last correspondence export path. */
	if (!lastCorrespPath().isEmpty()) {
		settings.setValue(CredNames::lstCorrspPath, lastCorrespPath());
	}

	/* save last ZFO export path */
	if (!lastZFOExportPath().isEmpty()) {
		settings.setValue(CredNames::lstZfoPath, lastZFOExportPath());
	}

	if (!group.isEmpty()) {
		settings.endGroup();
	}
}

bool AcntSettings::credentialsLessThan(const QString &s1, const QString &s2)
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

void AccountsMap::loadFromSettings(const QSettings &settings)
{
	QStringList groups = settings.childGroups();
	QRegExp credRe(CredNames::creds + ".*");
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
	qSort(credetialList.begin(), credetialList.end(),
	    AcntSettings::credentialsLessThan);

	/* For all credentials. */
	foreach(const QString &group, credetialList) {
		itemSettings.clear();

		itemSettings.loadFromSettings(settings, group);

		/* Associate map with item node. */
		Q_ASSERT(!itemSettings.userName().isEmpty());
		this->operator[](itemSettings.userName()) = itemSettings;
	}
}
