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

#pragma once

#include <QScopedPointer>
#include <QSettings>
#include <QString>

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

class AcntSettingsPrivate;
/*!
 * @brief Holds account settings.
 */
class AcntSettings {
	Q_DECLARE_PRIVATE(AcntSettings)

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
	AcntSettings(const AcntSettings &other);
#ifdef Q_COMPILER_RVALUE_REFS
	AcntSettings(AcntSettings &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
	~AcntSettings(void);

	AcntSettings &operator=(const AcntSettings &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
	AcntSettings &operator=(AcntSettings &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

	bool operator==(const AcntSettings &other) const;
	bool operator!=(const AcntSettings &other) const;

	friend void swap(AcntSettings &first, AcntSettings &second) Q_DECL_NOTHROW;

	bool isNull(void) const;

	void clear(void);

	bool isValid(void) const;
	const QString &accountName(void) const;
	void setAccountName(const QString &an);
#ifdef Q_COMPILER_RVALUE_REFS
	void setAccountName(QString &&an);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QString &userName(void) const;
	void setUserName(const QString &un);
#ifdef Q_COMPILER_RVALUE_REFS
	void setUserName(QString &&un);
#endif /* Q_COMPILER_RVALUE_REFS */
	enum LogInMethod loginMethod(void) const;
	void setLoginMethod(enum LogInMethod method);
	const QString &password(void) const;
	void setPassword(const QString &pwd);
#ifdef Q_COMPILER_RVALUE_REFS
	void setPassword(QString &&pwd);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QString &pwdAlg(void) const;
	void setPwdAlg(const QString &pwdAlg);
#ifdef Q_COMPILER_RVALUE_REFS
	void setPwdAlg(QString &&pwdAlg);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QByteArray &pwdSalt(void) const;
	void setPwdSalt(const QByteArray &pwdSalt);
#ifdef Q_COMPILER_RVALUE_REFS
	void setPwdSalt(QByteArray &&pwdSalt);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QByteArray &pwdIv(void) const;
	void setPwdIv(const QByteArray &pwdIv);
#ifdef Q_COMPILER_RVALUE_REFS
	void setPwdIv(QByteArray &&pwdIv);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QByteArray &pwdCode(void) const;
	void setPwdCode(const QByteArray &pwdCode);
#ifdef Q_COMPILER_RVALUE_REFS
	void setPwdCode(QByteArray &&pwdCode);
#endif /* Q_COMPILER_RVALUE_REFS */
	bool isTestAccount(void) const;
	void setTestAccount(bool isTesting);
	bool rememberPwd(void) const;
	void setRememberPwd(bool remember);
	const QString &dbDir(void) const;
	void setDbDir(const QString &path, const QString &confDir);
#ifdef Q_COMPILER_RVALUE_REFS
	void setDbDir(QString &&path, const QString &confDir);
#endif /* Q_COMPILER_RVALUE_REFS */
	bool syncWithAll(void) const;
	void setSyncWithAll(bool sync);
	const QString &p12File(void) const;
	void setP12File(const QString &p12);
#ifdef Q_COMPILER_RVALUE_REFS
	void setP12File(QString &&p12);
#endif /* Q_COMPILER_RVALUE_REFS */
	qint64 lastMsg(void) const;
	void setLastMsg(qint64 dmId);
	const QString &lastAttachSavePath(void) const;
	void setLastAttachSavePath(const QString &path);
#ifdef Q_COMPILER_RVALUE_REFS
	void setLastAttachSavePath(QString &&path);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QString &lastAttachAddPath(void) const;
	void setLastAttachAddPath(const QString &path);
#ifdef Q_COMPILER_RVALUE_REFS
	void setLastAttachAddPath(QString &&path);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QString &lastCorrespPath(void) const;
	void setLastCorrespPath(const QString &path);
#ifdef Q_COMPILER_RVALUE_REFS
	void setLastCorrespPath(QString &&path);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QString &lastZFOExportPath(void) const;
	void setLastZFOExportPath(const QString &path);
#ifdef Q_COMPILER_RVALUE_REFS
	void setLastZFOExportPath(QString &&path);
#endif /* Q_COMPILER_RVALUE_REFS */

	bool _createdFromScratch(void) const;
	void _setCreatedFromScratch(bool fromScratch);
	const QString &_passphrase(void) const;
	void _setPassphrase(const QString &passphrase);
#ifdef Q_COMPILER_RVALUE_REFS
	void _setPassphrase(QString &&passphrase);
#endif /* Q_COMPILER_RVALUE_REFS */
	const QString &_otp(void) const;
	void _setOtp(const QString &otpCode);
#ifdef Q_COMPILER_RVALUE_REFS
	void _setOtp(QString &&otpCode);
#endif /* Q_COMPILER_RVALUE_REFS */
	bool _pwdExpirDlgShown(void) const;
	void _setPwdExpirDlgShown(bool pwdExpirDlgShown);

	/*!
	 * @brief Used to decrypt the password.
	 *
	 * @param[in] oldPin PIN value used to decrypt old passwords.
	 */
	void decryptPassword(const QString &oldPin);

	/*!
	 * @brief Load content from settings group.
	 *
	 * @note Content is not erased before new settings is loaded.
	 *
	 * @param[in] confDir Configuration directory path.
	 * @param[in] settings Settings structure to load data from.
	 * @param[in] group Name of group to work with.
	 */
	void loadFromSettings(const QString &confDir,
	    const QSettings &settings, const QString &group);

	/*!
	 * @brief Save content to settings.
	 *
	 * @param[in]  pinVal PIN value to be used for password encryption.
	 * @param[in]  confDir Configuration directory path.
	 * @param[out] settings Settings structure to write/append data into.
	 * @param[in]  group Name of group to write to, group is create only
	 *                   when non-empty string supplied.
	 */
	void saveToSettings(const QString &pinVal, const QString &confDir,
	    QSettings &settings, const QString &group) const;

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
	QScopedPointer<AcntSettingsPrivate> d_ptr; // std::unique_ptr ?
};
