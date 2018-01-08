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

#include <cstdlib> /* malloc(3) */
#include <cstring> /* memset(3) */
#include <QFile>

#include "src/common.h"
#include "src/io/imports.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/settings/preferences.h"

IsdsSessions *globIsdsSessionsPtr = Q_NULLPTR;

/*!
 * @brief Returns logging facility name.
 *
 * @param[in] facility Logging facility identifier.
 * @return String with facility name.
 */
static
const char *logFacilityName(isds_log_facility facility)
{
	const char *str = "libisds unknown";

	switch (facility) {
	case ILF_NONE:
		str = "libisds none";
		break;
	case ILF_HTTP:
		str = "libisds http";
		break;
	case ILF_SOAP:
		str = "libisds soap";
		break;
	case ILF_ISDS:
		str = "libisds isds";
		break;
	case ILF_FILE:
		str = "libisds file";
		break;
	case ILF_SEC:
		str = "libisds sec";
		break;
	case ILF_XML:
		str = "libisds xml";
		break;
	default:
		break;
	}

	return str;
}

/*!
 * @brief Logging callback used in libisds.
 *
 * @param[in] facility Logging facility identifier.
 * @param[in] level Urgency level.
 * @param[in] message Null-terminated string.
 * @param[in] length Passed string length without terminating null character.
 * @param[in] data Pointer to data passed on every invocation of the callback.
 */
static
void logCallback(isds_log_facility facility, isds_log_level level,
    const char *message, int length, void *data)
{
	Q_UNUSED(data);
	Q_UNUSED(length);
	const char *logFac = logFacilityName(facility);

	switch (level) {
	case ILL_NONE:
	case ILL_CRIT:
	case ILL_ERR:
		logErrorMl("%s: %s", logFac, message);
		break;
	case ILL_WARNING:
		logWarningMl("%s: %s", logFac, message);
		break;
	case ILL_INFO:
		logInfoMl("%s: %s", logFac, message);
		break;
	case ILL_DEBUG:
		logDebugMlLv3("%s: %s", logFac, message);
		break;
	default:
		logError("Unknown ISDS log level %d.\n", level);
		break;
	}
}

IsdsSessions::IsdsSessions(void)
    : m_sessions()
{
	isds_error status;

	/* Initialise libisds. */
	status = isds_init();
	if (IE_SUCCESS != status) {
		Q_ASSERT(0);
		logError("%s\n", "Unsuccessful ISDS initialisation.");
		/* TODO -- What to do on failure? */
	}

	isds_set_log_callback(logCallback, NULL);

	/* Logging. */
#if !defined(Q_OS_WIN)
	isds_set_logging(ILF_ALL, ILL_ALL);
#else /* defined(Q_OS_WIN) */
	/*
	 * There is a issue related to logging when using libisds compiled
	 * with MinGW. See https://gitlab.labs.nic.cz/datovka/datovka/issues/233
	 * for more details.
	 */
	if (globLog.logVerbosity() < 3) {
		/* Don't write transferred data into log. */
		isds_set_logging(ILF_ALL, ILL_INFO);
	} else {
		logInfoNL("%s",
		    "Allowing unrestricted logging from inside libisds.");
		isds_set_logging(ILF_ALL, ILL_ALL);
	}
#endif /* !defined(Q_OS_WIN) */
}

IsdsSessions::~IsdsSessions(void)
{
	isds_error status;
	struct isds_ctx *isdsSession = NULL;

	/* Free all contexts. */
	foreach (isdsSession, m_sessions) {
		Q_ASSERT(NULL != isdsSession);

		status = isds_logout(isdsSession);
		if (IE_SUCCESS != status) {
			logWarningNL("%s", "Error in ISDS logout procedure.");
		}

		status = isds_ctx_free(&isdsSession);
		if (IE_SUCCESS != status) {
			logWarningNL("%s", "Error freeing ISDS session.");
		}
	}

	status = isds_cleanup();
	if (IE_SUCCESS != status) {
		logWarningNL("%s", "Unsuccessful ISDS clean-up.");
	}
}

bool IsdsSessions::holdsSession(const QString &userName) const
{
	return NULL != m_sessions.value(userName, NULL);
}

struct isds_ctx * IsdsSessions::session(const QString &userName) const
{
	return m_sessions.value(userName, NULL);
}

bool IsdsSessions::isConnectedToIsds(const QString &userName)
{
	isds_error ping_status;

	if (!holdsSession(userName)) {
		return false;
	}

	setSessionTimeout(userName, ISDS_PING_TIMEOUT_MS);
	ping_status = isds_ping(session(userName));
	setSessionTimeout(userName, globPref.isds_download_timeout_ms);

	return IE_SUCCESS == ping_status;
}

struct isds_ctx *IsdsSessions::createCleanSession(const QString &userName,
    unsigned int connectionTimeoutMs)
{
	isds_error status;
	struct isds_ctx *isds_session = NULL;

	/* User name should not exist. */
	Q_ASSERT(!holdsSession(userName));

	isds_session = isds_ctx_create();
	if (NULL == isds_session) {
		logErrorNL("Error creating ISDS session for user '%s'.",
		    userName.toUtf8().constData());
		goto fail;
	}

	status = isds_set_timeout(isds_session, connectionTimeoutMs);
	if (IE_SUCCESS != status) {
		logErrorNL("Error setting time-out for user '%s'.",
		    userName.toUtf8().constData());
		goto fail;
	}

	m_sessions.insert(userName, isds_session);
	return isds_session;

fail:
	if (NULL != isds_session) {
		status = isds_ctx_free(&isds_session);
		if (IE_SUCCESS != status) {
			logWarningNL(
			    "Error freeing ISDS session for user '%s'.",
			    userName.toUtf8().constData());
		}
	}
	return NULL;
}

bool IsdsSessions::setSessionTimeout(const QString &userName,
    unsigned int timeoutMs)
{
	struct isds_ctx *isds_session = m_sessions.value(userName, NULL);
	if (NULL == isds_session) {
		Q_ASSERT(0);
		return false;
	}

	isds_error status = isds_set_timeout(isds_session, timeoutMs);
	if (IE_SUCCESS != status) {
		logErrorNL("Error setting time-out for user '%s'.",
		    userName.toUtf8().constData());
		return false;
	}

	return true;
}


/* ========================================================================= */
/*
 * Log in using user name and password.
 */
isds_error isdsLoginUserName(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!pwd.isEmpty());

	return isds_login(isdsSession,
	    testingSession ? isds_testing_locator : isds_locator,
	    userName.toUtf8().constData(),
	    pwd.toUtf8().constData(),
	    NULL, NULL);
}


/* ========================================================================= */
/*
 * Log in using system certificate.
 */
isds_error isdsLoginSystemCert(struct isds_ctx *isdsSession,
    const QString &certPath, const QString &passphrase, bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!certPath.isEmpty());

	isds_error status = IE_ERROR;

	if (certPath.isNull() || certPath.isEmpty()) {
		return status;
	}

	isds_pki_credentials *pki_credentials = NULL;
	pki_credentials = (struct isds_pki_credentials *)
	    malloc(sizeof(struct isds_pki_credentials));
	if (pki_credentials == NULL) {
		return status;
	}
	memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

	QFileInfo certFile(certPath);
	QString ext = certFile.suffix();
	ext = ext.toUpper();

	if (ext == "PEM") {
		pki_credentials->certificate_format = PKI_FORMAT_PEM;
		pki_credentials->key_format = PKI_FORMAT_PEM;
	} else if (ext == "DER") {
		pki_credentials->certificate_format = PKI_FORMAT_DER;
		pki_credentials->key_format = PKI_FORMAT_DER;
	} else if (ext == "P12") {
		/* TODO - convert p12 to pem */
		pki_credentials->certificate_format = PKI_FORMAT_PEM;
		pki_credentials->key_format = PKI_FORMAT_PEM;
	}

	pki_credentials->engine = NULL;
	pki_credentials->certificate = strdup(certPath.toUtf8().constData());
	pki_credentials->key = strdup(certPath.toUtf8().constData());
	pki_credentials->passphrase = strdup(passphrase.toUtf8().constData());

	status = isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    NULL, NULL, pki_credentials, NULL);

	isds_pki_credentials_free(&pki_credentials);

	return status;

}


/* ========================================================================= */
/*
 * Log in using user certificate without password. Username = IDdatabox
 */
isds_error isdsLoginUserCert(struct isds_ctx *isdsSession,
    const QString &idBox, const QString &certPath, const QString &passphrase,
    bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!idBox.isEmpty());
	Q_ASSERT(!certPath.isEmpty());

	isds_error status = IE_ERROR;

	if (certPath.isNull() || certPath.isEmpty()) {
		return status;
	}

	if (idBox.isNull() || idBox.isEmpty()) {
		return status;
	}

	isds_pki_credentials *pki_credentials = NULL;

	pki_credentials = (struct isds_pki_credentials *)
	    malloc(sizeof(struct isds_pki_credentials));
	if (pki_credentials == NULL) {
		return status;
	}
	memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

	QFileInfo certFile(certPath);
	QString ext = certFile.suffix();
	ext = ext.toUpper();

	if (ext == "PEM") {
		pki_credentials->certificate_format = PKI_FORMAT_PEM;
		pki_credentials->key_format = PKI_FORMAT_PEM;
	} else if (ext == "DER") {
		pki_credentials->certificate_format = PKI_FORMAT_DER;
		pki_credentials->key_format = PKI_FORMAT_DER;
	} else if (ext == "P12") {
		/* TODO - convert p12 to pem */
		pki_credentials->certificate_format = PKI_FORMAT_PEM;
		pki_credentials->key_format = PKI_FORMAT_PEM;
	}

	pki_credentials->engine = NULL;
	pki_credentials->certificate = strdup(certPath.toUtf8().constData());
	pki_credentials->key = strdup(certPath.toUtf8().constData());
	pki_credentials->passphrase = strdup(passphrase.toUtf8().constData());

	status = isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    idBox.toUtf8().constData(), NULL, pki_credentials, NULL);

	isds_pki_credentials_free(&pki_credentials);

	return status;
}


/* ========================================================================= */
/*!
 * @brief Log in using user certificate with password.
 */
isds_error isdsLoginUserCertPwd(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, const QString &certPath,
    const QString &passphrase, bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!certPath.isEmpty());
	Q_ASSERT(!pwd.isEmpty());

	isds_error status = IE_ERROR;

	if (certPath.isNull() || certPath.isEmpty()) {
		return status;
	}

	if (userName.isNull() || userName.isEmpty()) {
		return status;
	}

	QString password = pwd;
	isds_pki_credentials *pki_credentials = NULL;

	pki_credentials = (struct isds_pki_credentials *)
	    malloc(sizeof(struct isds_pki_credentials));
	if (pki_credentials == NULL) {
		return status;
	}
	memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

	QFileInfo certFile(certPath);
	QString ext = certFile.suffix();
	ext = ext.toUpper();

	if (ext == "PEM") {
		pki_credentials->certificate_format = PKI_FORMAT_PEM;
		pki_credentials->key_format = PKI_FORMAT_PEM;
	} else if (ext == "DER") {
		pki_credentials->certificate_format = PKI_FORMAT_DER;
		pki_credentials->key_format = PKI_FORMAT_DER;
	} else if (ext == "P12") {
		/* TODO - convert p12 to pem */
		pki_credentials->certificate_format = PKI_FORMAT_PEM;
		pki_credentials->key_format = PKI_FORMAT_PEM;
	}

	pki_credentials->engine = NULL;
	pki_credentials->certificate = strdup(certPath.toUtf8().constData());
	pki_credentials->key = strdup(certPath.toUtf8().constData());
	pki_credentials->passphrase = strdup(passphrase.toUtf8().constData());

	status = isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    userName.toUtf8().constData(), password.toUtf8().constData(),
	    pki_credentials, NULL);

	isds_pki_credentials_free(&pki_credentials);

	return status;
}


/* ========================================================================= */
/*
 * Log in using opt.
 */
isds_error isdsLoginUserOtp(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, bool testingSession,
    enum AcntSettings::LogInMethod otpMethod, const QString &otpCode,
    isds_otp_resolution &res)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!pwd.isEmpty());

	struct isds_otp *otp = NULL;
	isds_error status = IE_ERROR;

	if (userName.isEmpty()) {
		goto fail;
	}

	otp = (struct isds_otp *) malloc(sizeof(struct isds_otp));
	if (otp == NULL) {
		goto fail;
	}
	memset(otp, 0, sizeof(struct isds_otp));

	if (otpMethod == AcntSettings::LIM_UNAME_PWD_HOTP) {
		otp->method = OTP_HMAC;
	} else {
		otp->method = OTP_TIME;
	}

	if (otpCode.isEmpty()) {
		otp->otp_code = NULL;
	} else {
		QByteArray optCodeBytes = otpCode.toLocal8Bit();
		const char *old_str =
		    optCodeBytes.data(); /* '\0' terminated */
		size_t len = strlen(old_str) + 1;
		otp->otp_code = (char *) malloc(len);
		if (NULL == otp->otp_code) {
			goto fail;
		}
		memcpy(otp->otp_code, old_str, len);
	}

	status = isds_login(isdsSession,
	    testingSession ? isds_otp_testing_locator : isds_otp_locator,
	    userName.toUtf8().constData(), pwd.toUtf8().constData(),
	    NULL, otp);

	res = otp->resolution;

fail:
	if (NULL != otp) {
		if (NULL != otp->otp_code) {
			free(otp->otp_code);
		}
		free(otp);
	}
	return status;
}


/* ========================================================================= */
/*
 * Add items into isds_PersonName structure.
 */
struct isds_PersonName * isds_PersonName_create(const QString &pnFirstName,
    const QString &pnMiddleName, const QString &pnLastName,
    const QString &pnLastNameAtBirth)
/* ========================================================================= */
{
	struct isds_PersonName *pn = NULL;

	pn = (struct isds_PersonName *) malloc(sizeof(*pn));
	if (NULL == pn) {
		goto fail;
	}
	memset(pn, 0, sizeof(*pn));

	if (!pnFirstName.isEmpty()) {
		pn->pnFirstName = strdup(pnFirstName.toUtf8().constData());
		if (NULL == pn->pnFirstName) {
			goto fail;
		}
	}

	if (!pnMiddleName.isEmpty()) {
		pn->pnMiddleName = strdup(pnMiddleName.toUtf8().constData());
		if (NULL == pn->pnMiddleName) {
			goto fail;
		}
	}

	if (!pnLastName.isEmpty()) {
		pn->pnLastName = strdup(pnLastName.toUtf8().constData());
		if (NULL == pn->pnLastName) {
			goto fail;
		}
	}

	if (!pnLastNameAtBirth.isEmpty()) {
		pn->pnLastNameAtBirth =
		    strdup(pnLastNameAtBirth.toUtf8().constData());
		if (NULL == pn->pnLastNameAtBirth) {
			goto fail;
		}
	}

	return pn;

fail:
	isds_PersonName_free(&pn);
	return NULL;
}

/* ========================================================================= */
/*
 * Add items into isds_BirthInfo structure.
 */
struct isds_BirthInfo * isds_BirthInfo_createConsume(struct tm *biDate,
    const QString &biCity, const QString &biCountry, const QString &biState)
/* ========================================================================= */
{
	struct isds_BirthInfo *bi = NULL;

	bi = (struct isds_BirthInfo *) malloc(sizeof(*bi));
	if (NULL == bi) {
		goto fail;
	}
	memset(bi, 0, sizeof(*bi));

//	bi->biDate

	if (!biCity.isEmpty()) {
		bi->biCity = strdup(biCity.toUtf8().constData());
		if (NULL == bi->biCity) {
			goto fail;
		}
	}

	if (!biCountry.isEmpty()) {
		bi->biCounty = strdup(biCountry.toUtf8().constData());
		if (NULL == bi->biCounty) {
			goto fail;
		}
	}

	if (!biState.isEmpty()) {
		bi->biState = strdup(biState.toUtf8().constData());
		if (NULL == bi->biState) {
			goto fail;
		}
	}

	/* Consumed pointers. */
	bi->biDate = biDate;

	return bi;

fail:
	isds_BirthInfo_free(&bi);
	return NULL;
}


/* ========================================================================= */
/*
 * Add items into isds_Address structure.
 */
struct isds_Address * isds_Address_create(const QString &adCity,
    const QString &adStreet, const QString &adNumberInStreet,
    const QString &adNumberInMunicipality, const QString &adZipCode,
    const QString &adState)
/* ========================================================================= */
{
	struct isds_Address *addr = NULL;

	addr = (struct isds_Address *) malloc(sizeof(*addr));
	if (NULL == addr) {
		goto fail;
	}
	memset(addr, 0, sizeof(*addr));

	if (!adCity.isEmpty()) {
		addr->adCity = strdup(adCity.toUtf8().constData());
		if (NULL == addr->adCity) {
			goto fail;
		}
	}

	if (!adStreet.isEmpty()) {
		addr->adStreet = strdup(adStreet.toUtf8().constData());
		if (NULL == addr->adStreet) {
			goto fail;
		}
	}

	if (!adNumberInStreet.isEmpty()) {
		addr->adNumberInStreet =
		    strdup(adNumberInStreet.toUtf8().constData());
		if (NULL == addr->adNumberInStreet) {
			goto fail;
		}
	}

	if (!adNumberInMunicipality.isEmpty()) {
		addr->adNumberInMunicipality =
		    strdup(adNumberInMunicipality.toUtf8().constData());
		if (NULL == addr->adNumberInMunicipality) {
			goto fail;
		}
	}

	if (!adZipCode.isEmpty()) {
		addr->adZipCode = strdup(adZipCode.toUtf8().constData());
		if (NULL == addr->adZipCode) {
			goto fail;
		}
	}

	if (!adState.isEmpty()) {
		addr->adState = strdup(adState.toUtf8().constData());
		if (NULL == addr->adState) {
			goto fail;
		}
	}

	return addr;

fail:
	isds_Address_free(&addr);
	return NULL;
}


/* ========================================================================= */
/*
 * Create new isds_DbOwnerInfo structure according to the supplied
 *     values.
 */
struct isds_DbOwnerInfo * isds_DbOwnerInfo_createConsume(const QString &dbID,
    isds_DbType dbType, const QString &ic,
    struct isds_PersonName *personName, const QString &firmName,
    struct isds_BirthInfo *birthInfo, struct isds_Address *address,
    const QString &nationality, const QString &email, const QString telNumber,
    const QString &identifier, const QString &registryCode, long int dbState,
    bool dbEffectiveOVM, bool dbOpenAddressing)
/* ========================================================================= */
{
	struct isds_DbOwnerInfo *doi = NULL;

	doi = (struct isds_DbOwnerInfo *) malloc(sizeof(*doi));
	if (doi == NULL) {
		goto fail;
	}
	memset(doi, 0, sizeof(*doi));

	if (!dbID.isEmpty()) {
		doi->dbID = strdup(dbID.toUtf8().constData());
		if (NULL == doi->dbID) {
			goto fail;
		}
	}
	doi->dbType = (isds_DbType *) malloc(sizeof(*doi->dbType));
	if (NULL == doi->dbType) {
		goto fail;
	}
	*doi->dbType = dbType;
	if (!ic.isEmpty()) {
		doi->ic = strdup(ic.toUtf8().constData());
		if (NULL == doi->ic) {
			goto fail;
		}
	}
//	doi->personName
	if (!firmName.isEmpty()) {
		doi->firmName = strdup(firmName.toUtf8().constData());
		if (NULL == doi->firmName) {
			goto fail;
		}
	}
//	doi->birthInfo
//	doi->address
	if (!nationality.isEmpty()) {
		doi->nationality = strdup(nationality.toUtf8().constData());
		if (NULL == doi->nationality) {
			goto fail;
		}
	}
	if (!email.isEmpty()) {
		doi->email = strdup(email.toUtf8().constData());
		if (NULL == doi->email) {
			goto fail;
		}
	}
	if (!telNumber.isEmpty()) {
		doi->telNumber = strdup(telNumber.toUtf8().constData());
		if (NULL == doi->telNumber) {
			goto fail;
		}
	}
	if (!identifier.isEmpty()) {
		doi->identifier = strdup(identifier.toUtf8().constData());
		if (NULL == doi->identifier) {
			goto fail;
		}
	}
	if (!registryCode.isEmpty()) {
		doi->registryCode = strdup(registryCode.toUtf8().constData());
		if (NULL == doi->registryCode) {
			goto fail;
		}
	}
	doi->dbState = (long int *) malloc(sizeof(*doi->dbState));
	if (NULL == doi->dbState) {
		goto fail;
	}
	*doi->dbState = dbState;
	doi->dbEffectiveOVM = (_Bool *) malloc(sizeof(*doi->dbEffectiveOVM));
	if (NULL == doi->dbEffectiveOVM) {
		goto fail;
	}
	*doi->dbEffectiveOVM = dbEffectiveOVM;
	doi->dbOpenAddressing = (_Bool *) malloc(sizeof(*doi->dbOpenAddressing));
	if (NULL == doi->dbOpenAddressing) {
		goto fail;
	}
	*doi->dbOpenAddressing = dbOpenAddressing;

	/* COnsumed pointers. */
	doi->personName = personName;
	doi->address = address;
	doi->birthInfo = birthInfo;

	return doi;

fail:
	isds_DbOwnerInfo_free(&doi);
	return NULL;
}


/* ========================================================================= */
/*
 * Create DbUserInfo structure and Search DataBoxes.
 */
struct isds_DbUserInfo * isds_DbUserInfo_createConsume(const QString &userID,
    isds_UserType userType, long int userPrivils,
    struct isds_PersonName *personName, struct isds_Address *address,
    const QString &ic, const QString &firmName, const QString &caStreet,
    const QString &caCity, const QString &caZipCode, const QString &caState)
/* ========================================================================= */
{
	struct isds_DbUserInfo *dui = NULL;

	dui = (struct isds_DbUserInfo *) malloc(sizeof(*dui));
	if (NULL == dui) {
		goto fail;
	}
	memset(dui, 0, sizeof(*dui));

	if (!userID.isEmpty()) {
		dui->userID = strdup(userID.toUtf8().constData());
		if (NULL == dui->userID) {
			goto fail;
		}
	}

	dui->userType = (isds_UserType *) malloc(sizeof(*dui->userType));
	if (NULL == dui->userType) {
		goto fail;
	}
	*dui->userType = userType;

	dui->userPrivils = (long int *) malloc(sizeof(*dui->userPrivils));
	if (NULL == dui->userPrivils) {
		goto fail;
	}
	*dui->userPrivils = userPrivils;

//	dui->personName

//	dui->address

//	bui->biDate = NULL;

	if (!ic.isEmpty()) {
		dui->ic = strdup(ic.toUtf8().constData());
		if (NULL == dui->ic) {
			goto fail;
		}
	}

	if (!firmName.isEmpty()) {
		dui->firmName = strdup(firmName.toUtf8().constData());
		if (NULL == dui->firmName) {
			goto fail;
		}
	}

	if (!caStreet.isEmpty()) {
		dui->caStreet = strdup(caStreet.toUtf8().constData());
		if (NULL == dui->caStreet) {
			goto fail;
		}
	}

	if (!caCity.isEmpty()) {
		dui->caCity = strdup(caCity.toUtf8().constData());
		if (NULL == dui->caCity) {
			goto fail;
		}
	}

	if (!caZipCode.isEmpty()) {
		dui->caZipCode = strdup(caZipCode.toUtf8().constData());
		if (NULL == dui->caZipCode) {
			goto fail;
		}
	}

	if (!caState.isEmpty()) {
		dui->caState = strdup(caState.toUtf8().constData());
		if (NULL == dui->caState) {
			goto fail;
		}
	}

	/* Consumed pointers. */
	dui->personName = personName;
	dui->address = address;

	return dui;

fail:
	isds_DbUserInfo_free(&dui);
	return NULL;
}

struct isds_message *loadZfoData(struct isds_ctx *isdsSession,
    const QByteArray &rawMsgData, int zfoType)
{
	isds_error status;
	isds_raw_type raw_type;
	struct isds_message *message = NULL;

	Q_ASSERT(NULL != isdsSession);

	status = isds_guess_raw_type(isdsSession, &raw_type, rawMsgData.data(),
	    rawMsgData.size());
	if (IE_SUCCESS != status) {
		logErrorNL("%s", "Cannot guess message content type.");
		goto fail;
	}

	if (zfoType == Imports::IMPORT_MESSAGE) {
		status = isds_load_message(isdsSession, raw_type,
		    rawMsgData.data(), rawMsgData.size(), &message,
		    BUFFER_COPY);
	} else {
		status = isds_load_delivery_info(isdsSession, raw_type,
		    rawMsgData.data(), rawMsgData.size(), &message,
		    BUFFER_COPY);
	}

	if (IE_SUCCESS != status) {
		logErrorNL("%s", "Cannot load message content.");
		goto fail;
	}

	return message;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}
	return NULL;
}

struct isds_message *loadZfoFile(struct isds_ctx *isdsSession,
    const QString &fileName, int zfoType)
{
	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly)) {
		logErrorNL("Cannot open file '%s'.",
		    fileName.toUtf8().constData());
		return NULL;
	}

	QByteArray content(file.readAll());
	file.close();

	struct isds_message *message =
	    loadZfoData(isdsSession, content, zfoType);
	if (NULL == message) {
		logErrorNL("Error while loading data from file '%s'.",
		    fileName.toUtf8().constData());
		return NULL;
	}

	return message;
}
