#include <QDebug>
#include <QFile>
#include <QInputDialog>
#include <QObject>

#include "isds_sessions.h"



GlobIsdsSessions isdsSessions;


/* ========================================================================= */
GlobIsdsSessions::GlobIsdsSessions(void)
/* ========================================================================= */
    : m_sessions()
{
	isds_error status;

	/* Initialise libisds. */
	status = isds_init();
	Q_ASSERT(IE_SUCCESS == status);
	if (IE_SUCCESS != status) {
		qWarning() << "Unsuccessful ISDS initialisation.";
		/* TODO -- What to do on failure? */
	}

	/* Logging. */
	//isds_set_logging(ILF_ALL, ILL_ALL);
}


/* ========================================================================= */
GlobIsdsSessions::~GlobIsdsSessions(void)
/* ========================================================================= */
{
	isds_error status;
	struct isds_ctx *isdsSession = NULL;

	/* Free all contexts. */
	foreach (isdsSession, m_sessions) {
		Q_ASSERT(0 != isdsSession);

		status = isds_logout(isdsSession);
		if (IE_SUCCESS != status) {
			qWarning() << "Error in ISDS logout procedure.";
		}

		status = isds_ctx_free(&isdsSession);
		if (IE_SUCCESS != status) {
			qDebug() << "Error freeing ISDS session.";
		}

		status = isds_cleanup();
		if (IE_SUCCESS != status) {
			qDebug() << "Unsuccessful ISDS clean-up.";
		}
	 }

	status = isds_cleanup();
	if (IE_SUCCESS != status) {
		qDebug() << "Unsuccessful ISDS clean-up.";
	}
}


/* ========================================================================= */
bool GlobIsdsSessions::holdsSession(const QString &userName) const
/* ========================================================================= */
{
	return 0 != m_sessions.value(userName, 0);
}


/* ========================================================================= */
/*
 * Is connect to databox given by account index
 */
bool GlobIsdsSessions::isConnectToIsds(QString userName)
/* ========================================================================= */
{
	isds_error status;

	if (!isdsSessions.holdsSession(userName)) {
		return false;
	}

	status = isds_ping(isdsSessions.session(userName));
	if (IE_SUCCESS == status) {
		return true;
	}
	return false;
}


/* ========================================================================= */
/*
 * Creates new session.
 */
struct isds_ctx * GlobIsdsSessions::createCleanSession(const QString &userName)
/* ========================================================================= */
{
	isds_error status;
	struct isds_ctx *isds_session = NULL;

	/* User name should not exist. */
	Q_ASSERT(0 == m_sessions.value(userName, 0));

	isds_session = isds_ctx_create();
	if (NULL == isds_session) {
		qDebug() << "Error creating ISDS session.";
		goto fail;
	}

	status = isds_set_timeout(isds_session, TIMEOUT_MS);
	if (IE_SUCCESS != status) {
		qDebug() << "Error setting time-out.";
		goto fail;
	}

	m_sessions.insert(userName, isds_session);

	return isds_session;
fail:
	if (NULL != isds_session) {
		status = isds_ctx_free(&isds_session);
		if (IE_SUCCESS != status) {
			qDebug() << "Error freeing ISDS session.";
		}
	}
	return 0;
}


/* ========================================================================= */
/*
 * Returns associated session.
 */
struct isds_ctx * GlobIsdsSessions::session(const QString &userName) const
/* ========================================================================= */
{
	Q_ASSERT(0 != m_sessions.value(userName, 0));

	return m_sessions.value(userName);
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
	    userName.toStdString().c_str(),
	    pwd.toStdString().c_str(),
	    NULL, NULL);
}


/* ========================================================================= */
/*
 * Log in using system certificate.
 */
isds_error isdsLoginSystemCert(struct isds_ctx *isdsSession,
    const QString &certPath, bool testingSession)
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
		free(pki_credentials);
			return status;
	}
	memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

	/* TODO - set correct cert format and certificate/key */
	pki_credentials->engine = NULL;
	pki_credentials->certificate_format = PKI_FORMAT_DER;
	pki_credentials->certificate = strdup(certPath.toStdString().c_str());
	pki_credentials->key_format = PKI_FORMAT_DER;
	pki_credentials->key = strdup(certPath.toStdString().c_str());
	pki_credentials->passphrase = NULL;

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
    const QString &idBox, const QString &certPath, bool testingSession)
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
		free(pki_credentials);
			return status;
	}
	memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

	/* TODO - set correct cert format and certificate/key */
	pki_credentials->engine = NULL;
	pki_credentials->certificate_format = PKI_FORMAT_DER;
	pki_credentials->certificate = strdup(certPath.toStdString().c_str());
	pki_credentials->key_format = PKI_FORMAT_DER;
	pki_credentials->key = strdup(certPath.toStdString().c_str());
	pki_credentials->passphrase = NULL;

	status = isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    idBox.toStdString().c_str(), NULL, pki_credentials, NULL);

	isds_pki_credentials_free(&pki_credentials);

	return status;
}


/* ========================================================================= */
/*!
 * @brief Log in using user certificate with password.
 */
isds_error isdsLoginUserCertPwd(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, const QString &certPath,
    bool testingSession)
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
		free(pki_credentials);
			return status;
	}
	memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

	/* TODO - set correct cert format and certificate/key */
	pki_credentials->engine = NULL;
	pki_credentials->certificate_format = PKI_FORMAT_DER;
	pki_credentials->certificate = strdup(certPath.toStdString().c_str());
	pki_credentials->key_format = PKI_FORMAT_DER;
	pki_credentials->key = strdup(certPath.toStdString().c_str());
	pki_credentials->passphrase = NULL;

	status = isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    userName.toStdString().c_str(), password.toStdString().c_str(),
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
    const QString &otpMethod, const QString &otpCode)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!pwd.isEmpty());

	isds_error status = IE_ERROR;

	if (userName.isNull() || userName.isEmpty()) {
		return status;
	}

	struct isds_otp *otp = NULL;

	otp = (struct isds_otp *)
	    malloc(sizeof(struct isds_otp));

	if (otp == NULL) {
		free(otp);
		return status;
	}
	memset(otp, 0, sizeof(struct isds_otp));

	if (otpMethod == "hopt") {
		otp->method = OTP_HMAC;
	} else {
		otp->method = OTP_TIME;
	}

	if (otpCode.isNull() || otpCode.isEmpty()) {
		otp->otp_code = NULL;
	} else {
		char *new_str;
		const char *old_str = otpCode.toStdString().c_str();
		size_t len = strlen(old_str) + 1;
		new_str = (char *) malloc(len);
		memcpy(new_str, old_str, len);
		otp->otp_code = new_str;
	}

	status = isds_login(isdsSession,
	    testingSession ? isds_otp_testing_locator : isds_otp_locator,
	    userName.toStdString().c_str(), pwd.toStdString().c_str(),
	    NULL, otp);

	free(otp->otp_code);
	free(otp);

	return status;
}


/* ========================================================================= */
/*
 * Add items into isds_PersonName structure.
 */
isds_PersonName * isds_PersonName_add(const QString &pnFirstName,
    const QString &pnMiddleName, const QString &pnLastName,
    const QString &pnLastNameAtBirth)
/* ========================================================================= */
{
	struct isds_PersonName *tmp = NULL;

	tmp =(struct isds_PersonName *)
	    malloc(sizeof(struct isds_PersonName));

	if (tmp == NULL) {
		free(tmp);
		return NULL;
	}

	tmp->pnFirstName = !pnFirstName.isEmpty() ?
	    strdup(pnFirstName.toStdString().c_str()) : NULL;
	tmp->pnMiddleName = !pnMiddleName.isEmpty() ?
	    strdup(pnMiddleName.toStdString().c_str()) : NULL;
	tmp->pnLastName = !pnLastName.isEmpty() ?
	    strdup(pnLastName.toStdString().c_str()) : NULL;
	tmp->pnLastNameAtBirth = !pnLastNameAtBirth.isEmpty() ?
	    strdup(pnLastNameAtBirth.toStdString().c_str()) : NULL;

	return tmp;
}

/* ========================================================================= */
/*
 * Add items into isds_BirthInfo structure.
 */
 isds_BirthInfo * isds_BirthInfo_add(struct tm *biDate,
    const QString &biCity, const QString &biCountry,
    const QString &biState)
/* ========================================================================= */
{
	struct isds_BirthInfo *tmp = NULL;

	tmp =(struct isds_BirthInfo *)
	    malloc(sizeof(struct isds_BirthInfo));

	if (tmp == NULL) {
		free(tmp);
		return NULL;
	}

	tmp->biDate = biDate;
	tmp->biCity = !biCity.isEmpty() ?
	    strdup(biCity.toStdString().c_str()) : NULL;
	tmp->biCounty = !biCountry.isEmpty() ?
	    strdup(biCountry.toStdString().c_str()) : NULL;
	tmp->biState = !biState.isEmpty() ?
	    strdup(biState.toStdString().c_str()) : NULL;

	return tmp;
}


/* ========================================================================= */
/*
 * Add items into isds_Address structure.
 */
isds_Address * isds_Address_add(const QString &adCity,
    const QString &adStreet, const QString &adNumberInStreet,
    const QString &adNumberInMunicipality, const QString &adZipCode,
    const QString &adState)
/* ========================================================================= */
{
	struct isds_Address *tmp = NULL;

	tmp =(struct isds_Address *)
	    malloc(sizeof(struct isds_Address));

	if (tmp == NULL) {
		free(tmp);
		return NULL;
	}

	tmp->adCity = !adCity.isEmpty() ?
	    strdup(adCity.toStdString().c_str()) : NULL;
	tmp->adStreet = !adStreet.isEmpty() ?
	    strdup(adStreet.toStdString().c_str()) : NULL;
	tmp->adNumberInStreet = !adNumberInStreet.isEmpty() ?
	    strdup(adNumberInStreet.toStdString().c_str()) : NULL;
	tmp->adNumberInMunicipality = !adNumberInMunicipality.isEmpty() ?
	    strdup(adNumberInMunicipality.toStdString().c_str()) : NULL;
	tmp->adZipCode = !adZipCode.isEmpty() ?
	    strdup(adZipCode.toStdString().c_str()) : NULL;
	tmp->adState = !adState.isEmpty() ?
	    strdup(adState.toStdString().c_str()) : NULL;

	return tmp;
}

/* ========================================================================= */
/*
 * Create DbOwnerInfo structure and Search DataBoxes.
 */
void isds_DbOwnerInfo_search(struct isds_list **result,
    const QString &userName, const QString &dbID,
    isds_DbType dbType, const QString &ic,
    struct isds_PersonName *personName, const QString &firmName,
    struct isds_BirthInfo *birthInfo, struct isds_Address *address,
    const QString &nationality, const QString &email, const QString telNumber,
    const QString &identifier, const QString &registryCode, long int dbState,
    bool dbEffectiveOVM, bool dbOpenAddressing)
/* ========================================================================= */
{
	struct isds_DbOwnerInfo *tmp = NULL;

	tmp =(struct isds_DbOwnerInfo *)
	    malloc(sizeof(struct isds_DbOwnerInfo));

	if (tmp == NULL) {
		free(tmp);
		return;
	}

	memset(tmp, 0, sizeof(struct isds_DbOwnerInfo));

	tmp->dbID = !dbID.isEmpty() ?
	    strdup(dbID.toStdString().c_str()) : NULL;
	tmp->ic = !ic.isEmpty() ?
	    strdup(ic.toStdString().c_str()) : NULL;
	tmp->firmName = !firmName.isEmpty() ?
	    strdup(firmName.toStdString().c_str()) : NULL;
	tmp->nationality = !nationality.isEmpty() ?
	    strdup(nationality.toStdString().c_str()) : NULL;
	tmp->email = !email.isEmpty() ?
	    strdup(email.toStdString().c_str()) : NULL;
	tmp->telNumber = !telNumber.isEmpty() ?
	    strdup(telNumber.toStdString().c_str()) : NULL;
	tmp->identifier = !identifier.isEmpty() ?
	    strdup(identifier.toStdString().c_str()) : NULL;
	tmp->registryCode = !registryCode.isEmpty() ?
	    strdup(registryCode.toStdString().c_str()) : NULL;

	tmp->dbType = &dbType;
	tmp->dbEffectiveOVM = &dbEffectiveOVM;
	tmp->dbOpenAddressing = &dbOpenAddressing;
	tmp->personName = personName;
	tmp->address = address;
	tmp->birthInfo = birthInfo;
	tmp->dbState = &dbState;

	isds_error status;
	status = isds_FindDataBox(isdsSessions.session(userName), tmp, result);

	qDebug() << status << isds_strerror(status);
}

/* ========================================================================= */
/*
 * Create DbUserInfo structure and Search DataBoxes.
 */
isds_DbUserInfo  * isds_DbOwnerInfo_add(const QString &userID,
    isds_UserType userType, long int userPrivils,
    struct isds_PersonName *personName, struct isds_Address *address,
    const QString &ic, const QString &firmName, const QString &caStreet,
    const QString &caCity, const QString &caZipCode, const QString &caState)
/* ========================================================================= */
{
	struct isds_DbUserInfo  *tmp = NULL;

	tmp =(struct isds_DbUserInfo  *)
	    malloc(sizeof(struct isds_DbUserInfo));

	if (tmp == NULL) {
		free(tmp);
		return NULL;
	}

	memset(tmp, 0, sizeof(struct isds_DbUserInfo));

	tmp->userID = !userID.isEmpty() ?
	    strdup(userID.toStdString().c_str()) : NULL;
	tmp->ic = !ic.isEmpty() ?
	    strdup(ic.toStdString().c_str()) : NULL;
	tmp->firmName = !firmName.isEmpty() ?
	    strdup(firmName.toStdString().c_str()) : NULL;
	tmp->caStreet = !caStreet.isEmpty() ?
	    strdup(caStreet.toStdString().c_str()) : NULL;
	tmp->caCity = !caCity.isEmpty() ?
	    strdup(caCity.toStdString().c_str()) : NULL;
	tmp->caZipCode = !caZipCode.isEmpty() ?
	    strdup(caZipCode.toStdString().c_str()) : NULL;
	tmp->caState = !caState.isEmpty() ?
	    strdup(caState.toStdString().c_str()) : NULL;

	tmp->userType = &userType;
	tmp->personName = personName;
	tmp->address = address;
	tmp->userPrivils = &userPrivils;

	return tmp;
}


/* ========================================================================= */
/*
 * Create a isds message from zfo file.
 */
struct isds_message * loadZfoFile(struct isds_ctx *isdsSession,
    const QString &fileName)
/* ========================================================================= */
{
	isds_error status;
	isds_raw_type raw_type;
	struct isds_message *message = NULL;

	Q_ASSERT(NULL != isdsSession);

	QFile file(fileName);
	QByteArray content;

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Cannot open file" << fileName;
		goto fail;
	}

	content = file.readAll();
	file.close();

	status = isds_guess_raw_type(isdsSession, &raw_type, content.data(),
	    content.size());
	if (IE_SUCCESS != status) {
		qWarning() << "Cannot guess content type of file" << fileName;
		goto fail;
	}

	status = isds_load_message(isdsSession, raw_type, content.data(),
	    content.size(), &message, BUFFER_COPY);
	if (IE_SUCCESS != status) {
		qWarning() << "Error while loading message from file"
		    << fileName;
		goto fail;
	}

	return message;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}
	return NULL;
}
