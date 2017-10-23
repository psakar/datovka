/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <QMap>
#include <QSettings>
#include <QString>
#include <QVariant>

/*!
 * @brief Defines labels used in credentials.
 */
namespace CredNames {
	extern const QString creds;

	extern const QString acntName;
	extern const QString userName;
	extern const QString lMethod;
	extern const QString pwd;
	extern const QString pwdAlg;
	extern const QString pwdSalt;
	extern const QString pwdIv;
	extern const QString pwdCode;
	extern const QString testAcnt;
	extern const QString rememberPwd;
	extern const QString dbDir;
	extern const QString syncWithAll;
	extern const QString p12File;
	extern const QString lstMsgId;
	extern const QString lstSaveAtchPath;
	extern const QString lstAddAtchPath;
	extern const QString lstCorrspPath;
	extern const QString lstZfoPath;
}

/*!
 * @brief Holds account settings.
 */
class AcntSettings : public QMap<QString, QVariant> {
public:
	/*!
	 * @brief Login method identifier.
	 */
	enum LogInMethod {
		LIM_UNKNOWN, /*!< Unknown method. */
		LIM_UNAME_PWD, /*!< User name and password. */
		LIM_UNAME_CRT, /*!< User name and certificate. */
		LIM_UNAME_PWD_CRT, /*!< User name, password and certificate. */
		LIM_UNAME_PWD_HOTP, /*!< User name, password and HOTP. */
		LIM_UNAME_PWD_TOTP /*!< User name, password and TOTP (SMS). */
	};

	/*!
	 * @brief Constructor.
	 */
	AcntSettings(void);
	AcntSettings(const QMap<QString, QVariant> &map);

	bool isValid(void) const;
	QString accountName(void) const;
	void setAccountName(const QString &name);
	QString userName(void) const;
	void setUserName(const QString &userName);
	enum LogInMethod loginMethod(void) const;
	void setLoginMethod(enum LogInMethod method);
	QString password(void) const;
	void setPassword(const QString &pwd);
	QString pwdAlg(void) const;
	void setPwdAlg(const QString &pwdAlg);
	QByteArray pwdSalt(void) const;
	void setPwdSalt(const QByteArray &pwdSalt);
	QByteArray pwdIv(void) const;
	void setPwdIv(const QByteArray &pwdIv);
	QByteArray pwdCode(void) const;
	void setPwdCode(const QByteArray &pwdCode);
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
	QString _otp(void) const;
	void _setOtp(const QString &otpCode);
	bool _pwdExpirDlgShown(void) const;
	void _setPwdExpirDlgShown(bool pwdExpirDlgShown);

	/*!
	 * @brief Load content from settings group.
	 *
	 * @note Content is not erased before new settings is loaded.
	 *
	 * @param[in] settings Settings structure to load data from.
	 * @param[in] group Name of group to work with.
	 */
	void loadFromSettings(const QSettings &settings, const QString &group);

	/*!
	 * @brief Save content to settings.
	 *
	 * @param[out] settings Settings structure to write/append data into.
	 * @param[in]  group Name of group to write to, group is create only
	 *                   when non-empty string supplied.
	 */
	void saveToSettings(QSettings &settings, const QString &group) const;

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
	bool credentialsLessThan(const QString &s1, const QString &s2);

private:
	/* Prohibit these methods in public interface. */
	QVariant operator[](const QString &key);
	const QVariant operator[](const QString &key) const;

	typedef QMap<QString, QVariant> m_parentType;
};

#endif /* _ACCOUNT_H_ */
