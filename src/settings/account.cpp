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

#include <QDir>

#include "src/common.h"
#include "src/datovka_shared/crypto/crypto_pwd.h"
#include "src/datovka_shared/crypto/crypto_wrapped.h"
#include "src/datovka_shared/log/log.h"
#include "src/settings/account.h"

/* Null objects - for convenience. */
static const QByteArray nullByteArray;
static const QString nullString;

namespace CredNames {
	const QString creds(QLatin1String("credentials"));

	const QString acntName(QLatin1String("name"));
	const QString userName(QLatin1String("username"));
	const QString lMethod(QLatin1String("login_method"));
	const QString pwd(QLatin1String("password"));
	const QString pwdAlg(QLatin1String("password_alg"));
	const QString pwdSalt(QLatin1String("password_salt"));
	const QString pwdIv(QLatin1String("password_iv"));
	const QString pwdCode(QLatin1String("password_code"));
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

/*!
 * @brief Login method names as stored in configuration file.
 */
namespace MethodNames {
	static const QString uNamePwd(QLatin1String("username"));
	static const QString uNameCrt(QLatin1String("certificate"));
	static const QString uNamePwdCrt(QLatin1String("user_certificate"));
	static const QString uNamePwdHotp(QLatin1String("hotp"));
	static const QString uNamePwdTotp(QLatin1String("totp"));
}

/*!
 * @brief Converts login method string to identifier.
 *
 * @param[in] str Identifier string as used in configuration file.
 * @return Identifier value.
 */
static
enum AcntSettings::LogInMethod methodStrToEnum(const QString &str)
{
	if (str == MethodNames::uNamePwd) {
		return AcntSettings::LIM_UNAME_PWD;
	} else if (str == MethodNames::uNameCrt) {
		return AcntSettings::LIM_UNAME_CRT;
	} else if (str == MethodNames::uNamePwdCrt) {
		return AcntSettings::LIM_UNAME_PWD_CRT;
	} else if (str == MethodNames::uNamePwdHotp) {
		return AcntSettings::LIM_UNAME_PWD_HOTP;
	} else if (str == MethodNames::uNamePwdTotp) {
		return AcntSettings::LIM_UNAME_PWD_TOTP;
	} else {
		return AcntSettings::LIM_UNKNOWN;
	}
}

/*!
 * @brief Converts login method identifier to string.
 *
 * @param[in] val Identifier value as used in the programme.
 * @return Identifier string as used in configuration file.
 */
static
const QString &methodEnumToStr(enum AcntSettings::LogInMethod val)
{
	static const QString nullStr;

	switch (val) {
	case AcntSettings::LIM_UNAME_PWD:
		return MethodNames::uNamePwd;
		break;
	case AcntSettings::LIM_UNAME_CRT:
		return MethodNames::uNameCrt;
		break;
	case AcntSettings::LIM_UNAME_PWD_CRT:
		return MethodNames::uNamePwdCrt;
		break;
	case AcntSettings::LIM_UNAME_PWD_HOTP:
		return MethodNames::uNamePwdHotp;
		break;
	case AcntSettings::LIM_UNAME_PWD_TOTP:
		return MethodNames::uNamePwdTotp;
		break;
	case AcntSettings::LIM_UNKNOWN:
	default:
		Q_ASSERT(0);
		return nullStr;
		break;
	}
}

/*!
 * @brief PIMPL AcntSettings class.
 */
class AcntSettingsPrivate {
	//Q_DISABLE_COPY(AcntSettingsPrivate)
public:
	AcntSettingsPrivate(void)
	    : m_accountName(), m_userName(),
	    m_loginMethod(AcntSettings::LIM_UNKNOWN), m_password(), m_pwdAlg(),
	    m_pwdSalt(), m_pwdIv(), m_pwdCode(), m_isTestAccount(false),
	    m_rememberPwd(false), m_dbDir(), m_syncWithAll(false), m_p12File(),
	    m_lastMsg(-1), m_lastAttachSavePath(), m_lastAttachAddPath(),
	    m_lastCorrespPath(), m_lastZFOExportPath(),
	    _m_createdFromScratch(false), _m_passphrase(), _m_otp(),
	    _m_pwdExpirDlgShown(false)
	{ }

	AcntSettingsPrivate &operator=(const AcntSettingsPrivate &other) Q_DECL_NOTHROW
	{
		m_accountName = other.m_accountName;
		m_userName = other.m_userName;
		m_loginMethod = other.m_loginMethod;
		m_password = other.m_password;
		m_pwdAlg = other.m_pwdAlg;
		m_pwdSalt = other.m_pwdSalt;
		m_pwdIv = other.m_pwdIv;
		m_pwdCode = other.m_pwdCode;
		m_isTestAccount = other.m_isTestAccount;
		m_rememberPwd = other.m_rememberPwd;
		m_dbDir = other.m_dbDir;
		m_syncWithAll = other.m_syncWithAll;
		m_p12File = other.m_p12File;

		m_lastMsg = other.m_lastMsg;
		m_lastAttachSavePath = other.m_lastAttachSavePath;
		m_lastAttachAddPath = other.m_lastAttachAddPath;
		m_lastCorrespPath = other.m_lastCorrespPath;
		m_lastZFOExportPath = other.m_lastZFOExportPath;

		_m_createdFromScratch = other._m_createdFromScratch;
		_m_passphrase = other._m_passphrase;
		_m_otp = other._m_otp;
		_m_pwdExpirDlgShown = other._m_pwdExpirDlgShown;

		return *this;
	}

	bool operator==(const AcntSettingsPrivate &other) const
	{
		return (m_accountName == other.m_accountName) &&
		    (m_userName == other.m_userName) &&
		    (m_loginMethod == other.m_loginMethod) &&
		    (m_password == other.m_password) &&
		    (m_pwdAlg == other.m_pwdAlg) &&
		    (m_pwdSalt == other.m_pwdSalt) &&
		    (m_pwdIv == other.m_pwdIv) &&
		    (m_pwdCode == other.m_pwdCode) &&
		    (m_isTestAccount == other.m_isTestAccount) &&
		    (m_rememberPwd == other.m_rememberPwd) &&
		    (m_dbDir == other.m_dbDir) &&
		    (m_syncWithAll == other.m_syncWithAll) &&
		    (m_p12File == other.m_p12File) &&

		    (m_lastMsg == other.m_lastMsg) &&
		    (m_lastAttachSavePath == other.m_lastAttachSavePath) &&
		    (m_lastAttachAddPath == other.m_lastAttachAddPath) &&
		    (m_lastCorrespPath == other.m_lastCorrespPath) &&
		    (m_lastZFOExportPath == other.m_lastZFOExportPath) &&

		    (_m_createdFromScratch == other._m_createdFromScratch) &&
		    (_m_passphrase == other._m_passphrase) &&
		    (_m_otp == other._m_otp) &&
		    (_m_pwdExpirDlgShown == other._m_pwdExpirDlgShown);
	}

	QString m_accountName;
	QString m_userName;
	enum AcntSettings::LogInMethod m_loginMethod;
	QString m_password;
	QString m_pwdAlg;
	QByteArray m_pwdSalt;
	QByteArray m_pwdIv;
	QByteArray m_pwdCode;
	bool m_isTestAccount;
	bool m_rememberPwd;
	QString m_dbDir;
	bool m_syncWithAll;
	QString m_p12File;

	qint64 m_lastMsg;
	QString m_lastAttachSavePath;
	QString m_lastAttachAddPath;
	QString m_lastCorrespPath;
	QString m_lastZFOExportPath;

	/* The following are not stored into the configuration file. */
	bool _m_createdFromScratch; /* Only set on new accounts. */
	QString _m_passphrase;
	QString _m_otp;
	bool _m_pwdExpirDlgShown;
};

AcntSettings::AcntSettings(void)
    : d_ptr(Q_NULLPTR)
{
}

AcntSettings::AcntSettings(const AcntSettings &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) AcntSettingsPrivate) : Q_NULLPTR)
{
	Q_D(AcntSettings);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
AcntSettings::AcntSettings(AcntSettings &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

AcntSettings::~AcntSettings(void)
{
}

/*!
 * @brief Ensures private account settings presence.
 *
 * @note Returns if private account settings could not be allocated.
 */
#define ensureAcntSettingsPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			AcntSettingsPrivate *p = new (std::nothrow) AcntSettingsPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

AcntSettings &AcntSettings::operator=(const AcntSettings &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureAcntSettingsPrivate(*this);
	Q_D(AcntSettings);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
AcntSettings &AcntSettings::operator=(AcntSettings &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool AcntSettings::operator==(const AcntSettings &other) const
{
	Q_D(const AcntSettings);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool AcntSettings::operator!=(const AcntSettings &other) const
{
	return !operator==(other);
}

bool AcntSettings::isNull(void) const
{
	Q_D(const AcntSettings);
	return d == Q_NULLPTR;
}

void AcntSettings::clear(void)
{
	d_ptr.reset(Q_NULLPTR);
}

bool AcntSettings::isValid(void) const
{
	return !isNull() &&
	    !accountName().isEmpty() && !userName().isEmpty();
}

const QString &AcntSettings::accountName(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_accountName;
}

void AcntSettings::setAccountName(const QString &an)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_accountName = an;
}

#ifdef Q_COMPILER_RVALUE_REFS
void  AcntSettings::setAccountName(QString &&an)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_accountName = an;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &AcntSettings::userName(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_userName;
}

void AcntSettings::setUserName(const QString &un)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_userName = un;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setUserName(QString &&un)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_userName = un;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum AcntSettings::LogInMethod AcntSettings::loginMethod(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return LIM_UNKNOWN;
	}

	return d->m_loginMethod;
}

void AcntSettings::setLoginMethod(enum LogInMethod method)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_loginMethod = method;
}

const QString &AcntSettings::password(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_password;
}

void AcntSettings::setPassword(const QString &pwd)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_password = pwd;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setPassword(QString &&pwd)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_password = pwd;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &AcntSettings::pwdAlg(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_pwdAlg;
}

void AcntSettings::setPwdAlg(const QString &pwdAlg)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdAlg = pwdAlg;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setPwdAlg(QString &&pwdAlg)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdAlg = pwdAlg;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QByteArray &AcntSettings::pwdSalt(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullByteArray;
	}

	return d->m_pwdSalt;
}

void AcntSettings::setPwdSalt(const QByteArray &pwdSalt)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdSalt = pwdSalt;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setPwdSalt(QByteArray &&pwdSalt)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdSalt = pwdSalt;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QByteArray &AcntSettings::pwdIv(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullByteArray;
	}

	return d->m_pwdIv;
}

void AcntSettings::setPwdIv(const QByteArray &pwdIv)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdIv = pwdIv;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setPwdIv(QByteArray &&pwdIv)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdIv = pwdIv;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QByteArray &AcntSettings::pwdCode(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullByteArray;
	}

	return d->m_pwdCode;
}

void AcntSettings::setPwdCode(const QByteArray &pwdCode)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdCode = pwdCode;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setPwdCode(QByteArray &&pwdCode)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_pwdCode = pwdCode;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool AcntSettings::isTestAccount(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}

	return d->m_isTestAccount;
}

void AcntSettings::setTestAccount(bool isTesting)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_isTestAccount = isTesting;
}

bool AcntSettings::rememberPwd(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}

	return d->m_rememberPwd;
}

void AcntSettings::setRememberPwd(bool remember)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_rememberPwd = remember;
}

const QString &AcntSettings::dbDir(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_dbDir;
}

void AcntSettings::setDbDir(const QString &path, const QString &confDir)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_dbDir = (path == confDir) ? QString() : path; /* Default path is empty. */
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setDbDir(QString &&path, const QString &confDir)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_dbDir = (path == confDir) ? QString() : path; /* Default path is empty. */
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool AcntSettings::syncWithAll(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}

	return d->m_syncWithAll;
}

void AcntSettings::setSyncWithAll(bool sync)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_syncWithAll = sync;
}

const QString &AcntSettings::p12File(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_p12File;
}

void AcntSettings::setP12File(const QString &p12)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_p12File = p12;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setP12File(QString &&p12)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_p12File = p12;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 AcntSettings::lastMsg(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}

	return d->m_lastMsg;
}

void AcntSettings::setLastMsg(qint64 dmId)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastMsg = dmId;
}

const QString &AcntSettings::lastAttachSavePath(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_lastAttachSavePath;
}

void AcntSettings::setLastAttachSavePath(const QString &path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastAttachSavePath = path;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setLastAttachSavePath(QString &&path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastAttachSavePath = path;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &AcntSettings::lastAttachAddPath(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_lastAttachAddPath;
}

void AcntSettings::setLastAttachAddPath(const QString &path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastAttachAddPath = path;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setLastAttachAddPath(QString &&path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastAttachAddPath = path;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &AcntSettings::lastCorrespPath(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_lastCorrespPath;
}

void AcntSettings::setLastCorrespPath(const QString &path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastCorrespPath = path;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setLastCorrespPath(QString &&path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastCorrespPath = path;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &AcntSettings::lastZFOExportPath(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_lastZFOExportPath;
}

void AcntSettings::setLastZFOExportPath(const QString &path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastZFOExportPath = path;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::setLastZFOExportPath(QString &&path)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->m_lastZFOExportPath = path;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool AcntSettings::_createdFromScratch(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}

	return d->_m_createdFromScratch;
}

void AcntSettings::_setCreatedFromScratch(bool fromScratch)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->_m_createdFromScratch = fromScratch;
}

const QString &AcntSettings::_passphrase(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->_m_passphrase;
}

void AcntSettings::_setPassphrase(const QString &passphrase)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->_m_passphrase = passphrase;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::_setPassphrase(QString &&passphrase)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->_m_passphrase = passphrase;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &AcntSettings::_otp(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->_m_otp;
}

void AcntSettings::_setOtp(const QString &otpCode)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->_m_otp = otpCode;
}

#ifdef Q_COMPILER_RVALUE_REFS
void AcntSettings::_setOtp(QString &&otpCode)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->_m_otp = otpCode;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool AcntSettings::_pwdExpirDlgShown(void) const
{
	Q_D(const AcntSettings);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}

	return d->_m_pwdExpirDlgShown;
}

void AcntSettings::_setPwdExpirDlgShown(bool pwdExpirDlgShown)
{
	ensureAcntSettingsPrivate();
	Q_D(AcntSettings);
	d->_m_pwdExpirDlgShown = pwdExpirDlgShown;
}

/*!
 * @brief Restores password value,
 *
 * @note The PIN value may not be known when the settings are read. Therefore
 *     password decryption is performed somewhere else.
 * @todo Modify the programme to ensure that pin is known at the time when
 *     account data is read.
 *
 * @param[in,out] aData Account data to store password into.
 * @param[in]     settings Settings structure.
 * @param[in]     groupName Settings group name.
 */
static
void readPwdData(AcntSettings &aData, const QSettings &settings,
    const QString &groupName)
{
	QString prefix;
	if (!groupName.isEmpty()) {
		prefix = groupName + QLatin1String("/");
	}

	{
		QString pwd(settings.value(prefix + CredNames::pwd,
		    QString()).toString());
		aData.setPassword(
		    QString::fromUtf8(QByteArray::fromBase64(pwd.toUtf8())));
	}

	aData.setPwdAlg(settings.value(prefix + CredNames::pwdAlg,
	    QString()).toString());

	aData.setPwdSalt(QByteArray::fromBase64(
	    settings.value(prefix + CredNames::pwdSalt,
	        QString()).toString().toUtf8()));

	aData.setPwdIv(QByteArray::fromBase64(
	    settings.value(prefix + CredNames::pwdIv,
	        QString()).toString().toUtf8()));

	aData.setPwdCode(QByteArray::fromBase64(
	    settings.value(prefix + CredNames::pwdCode,
	        QString()).toString().toUtf8()));

	if (!aData.password().isEmpty() && !aData.pwdCode().isEmpty()) {
		logWarningNL(
		    "Account with username '%s' has both encrypted and unencrypted password set.",
		    aData.userName().toUtf8().constData());
	}
}

void AcntSettings::decryptPassword(const QString &oldPin)
{
	if (!password().isEmpty()) {
		/* Password already stored in decrypted form. */
		logDebugLv0NL(
		    "Password for username '%s' already held in decrypted form.",
		    userName().toUtf8().constData());
		return;
	}

	if (oldPin.isEmpty()) {
		/*
		 * Old PIN not given, password already should be in plain
		 * format.
		 */
		logDebugLv0NL(
		    "No PIN supplied to decrypt password for username '%s'.",
		    userName().toUtf8().constData());
		return;
	}

	if (!pwdAlg().isEmpty() && !pwdSalt().isEmpty() &&
	    !pwdCode().isEmpty()) {
		logDebugLv0NL("Decrypting password for username '%s'.",
		    userName().toUtf8().constData());
		QString decrypted(decryptPwd(pwdCode(), oldPin, pwdAlg(),
		    pwdSalt(), pwdIv()));
		if (decrypted.isEmpty()) {
			logWarningNL(
			    "Failed decrypting password for username '%s'.",
			    userName().toUtf8().constData());
		}

		/* Store password. */
		if (!decrypted.isEmpty()) {
			setPassword(decrypted);
		}
	}
}

void AcntSettings::loadFromSettings(const QString &confDir,
    const QSettings &settings, const QString &group)
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
	    QString()).toStringList().join(", "));
	setUserName(settings.value(prefix + CredNames::userName,
	    QString()).toString());
	setLoginMethod(methodStrToEnum(
	    settings.value(prefix + CredNames::lMethod, QString()).toString()));
	readPwdData(*this, settings, group);
	setTestAccount(settings.value(prefix + CredNames::testAcnt,
	    QString()).toBool());
	setRememberPwd(settings.value(prefix + CredNames::rememberPwd,
	    QString()).toBool());
	setDbDir(
	    settings.value(prefix + CredNames::dbDir, QString()).toString(),
	    confDir);
	setSyncWithAll(settings.value(prefix + CredNames::syncWithAll,
	    QString()).toBool());
	setP12File(settings.value(prefix + CredNames::p12File,
	    QString()).toString());
	setLastMsg(settings.value(prefix + CredNames::lstMsgId,
	    QString()).toLongLong());
	setLastAttachSavePath(settings.value(prefix + CredNames::lstSaveAtchPath,
	    QString()).toString());
	setLastAttachAddPath(settings.value(prefix + CredNames::lstAddAtchPath,
	    QString()).toString());
	setLastCorrespPath(settings.value(prefix + CredNames::lstCorrspPath,
	    QString()).toString());
	setLastZFOExportPath(settings.value(prefix + CredNames::lstZfoPath,
	    QString()).toString());
}

/*!
 * @brief Stores encrypted password into settings.
 *
 * @param[in]     pinVal PIN value to be used for password encryption.
 * @param[in,out] settings Settings structure.
 * @param[in]     aData Account data to be stored.
 * @param[in]     password Password to be stored.
 */
static
bool storeEncryptedPwd(const QString &pinVal, QSettings &settings,
    const AcntSettings &aData, const QString &password)
{
	/* Currently only one cryptographic algorithm is supported. */
	const struct pwd_alg *pwdAlgDesc = &aes256_cbc;

	/* Ignore the algorithm settings. */
	const QString pwdAlg(aes256_cbc.name);
	QByteArray pwdSalt(aData.pwdSalt());
	QByteArray pwdIV(aData.pwdIv());

	if (pwdSalt.size() < pwdAlgDesc->key_len) {
		pwdSalt = randomSalt(pwdAlgDesc->key_len);
	}

	if (pwdIV.size() < pwdAlgDesc->iv_len) {
		pwdIV = randomSalt(pwdAlgDesc->iv_len);
	}

	QByteArray pwdCode = encryptPwd(password, pinVal, pwdAlgDesc->name,
	    pwdSalt, pwdIV);
	if (pwdCode.isEmpty()) {
		return false;
	}

	settings.setValue(CredNames::pwdAlg, pwdAlg);
	settings.setValue(CredNames::pwdSalt,
	    QString::fromUtf8(pwdSalt.toBase64()));
	settings.setValue(CredNames::pwdIv,
	    QString::fromUtf8(pwdIV.toBase64()));
	settings.setValue(CredNames::pwdCode,
	    QString::fromUtf8(pwdCode.toBase64()));

	return true;
}

void AcntSettings::saveToSettings(const QString &pinVal, const QString &confDir,
    QSettings &settings, const QString &group) const
{
	if (!group.isEmpty()) {
		settings.beginGroup(group);
	}

	settings.setValue(CredNames::acntName, accountName());
	settings.setValue(CredNames::userName, userName());
	settings.setValue(CredNames::lMethod, methodEnumToStr(loginMethod()));
	settings.setValue(CredNames::testAcnt, isTestAccount());
	settings.setValue(CredNames::rememberPwd, rememberPwd());
	if (rememberPwd() && !password().isEmpty()) {
		bool writePlainPwd = pinVal.isEmpty();
		if (!writePlainPwd) {
			writePlainPwd = !storeEncryptedPwd(pinVal, settings,
			    *this, password());
		}
		if (writePlainPwd) { /* Only when plain or encryption fails. */
			/* Store unencrypted password. */
			settings.setValue(CredNames::pwd, toBase64(password()));
		}
	}

	if (!dbDir().isEmpty()) {
		if (QDir(dbDir()) != QDir(confDir)) {
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

void swap(AcntSettings &first, AcntSettings &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}
