

#include <QDebug>
#include <QInputDialog>
#include <QObject>
#include <QMessageBox>

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
 * Connect to databox
 */
void GlobIsdsSessions::connectToIsds(
    const AccountModel::SettingsMap &accountInfo)
/* ========================================================================= */
{
	isds_error status = IE_SUCCESS;

	QString password = accountInfo.password();

	if (!isdsSessions.holdsSession(accountInfo.userName())) {
		createCleanSession(accountInfo.userName());
	}

	/* Login method based on username and password */
	if (accountInfo.loginMethod() == "username") {

		if (password.isEmpty()) {
			bool ok = false;
			QString text = "";
			while (text.isEmpty()) {
				text = QInputDialog::getText(0,
				    QObject::tr("Enter password"),
				    QObject::tr("Enter password for account ") +
				    accountInfo.userName(),
				    QLineEdit::Password, "", &ok);
				if (ok) {
					if (!text.isEmpty()) {
						password = text;
					}
				} else {
					return;
				}
			}
		}

		status = isdsLoginUserName(
		    isdsSessions.session(accountInfo.userName()),
		    accountInfo.userName(), password,
		    accountInfo.testAccount());

	/* Login method based on certificate only */
	} else if (accountInfo.loginMethod() == "certificate") {

		isds_pki_credentials *pki_credentials = NULL;

		pki_credentials = (struct isds_pki_credentials *)
		    malloc(sizeof(struct isds_pki_credentials));
		if (pki_credentials == NULL) {
			free(pki_credentials);
			return;
		}
		memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

		/* TODO - set correct cert format and certificate/key */
		pki_credentials->engine = NULL;
		pki_credentials->certificate_format = PKI_FORMAT_DER;
		pki_credentials->certificate = strdup(accountInfo.certPath().toStdString().c_str());
		pki_credentials->key_format = PKI_FORMAT_DER;
		pki_credentials->key = strdup(accountInfo.certPath().toStdString().c_str());
		pki_credentials->passphrase = NULL;

		QString iDbox = "TODO";
		/* TODO -- The account is not identified with a user name! */

		status = isdsLoginCert(isdsSessions.session(iDbox),
		    pki_credentials, accountInfo.testAccount());

		isds_pki_credentials_free(&pki_credentials);

	/* Login method based on certificate together with username */
	} else if (accountInfo.loginMethod() == "user_certificate") {

		isds_pki_credentials *pki_credentials = NULL;

		pki_credentials = (struct isds_pki_credentials *)
		    malloc(sizeof(struct isds_pki_credentials));
		if (pki_credentials == NULL) {
			free(pki_credentials);
			return;
		}
		memset(pki_credentials, 0, sizeof(struct isds_pki_credentials));

		/* TODO - set correct cert format and certificate/key */
		pki_credentials->engine = NULL;
		pki_credentials->certificate_format = PKI_FORMAT_DER;
		pki_credentials->certificate = strdup(accountInfo.certPath().toStdString().c_str());
		pki_credentials->key_format = PKI_FORMAT_DER;
		pki_credentials->key = strdup(accountInfo.certPath().toStdString().c_str());
		pki_credentials->passphrase = NULL;

		if (password.isNull()) {
			status = isdsLoginUserCert(
			    isdsSessions.session(accountInfo.userName()),
			    accountInfo.userName() /* boxId */,
			    pki_credentials, accountInfo.testAccount());
		} else {
			if (password.isEmpty()) {
				bool ok;
				QString text = "";
				while (text.isEmpty()) {
					text = QInputDialog::getText(0,
					    QObject::tr("Enter certificate password"),
					    QObject::tr("Enter certificate password for account ") +
					    accountInfo.userName(),
					    QLineEdit::Password, "", &ok);
					if (ok) {
						if (!text.isEmpty()) {
							password = text;
						}
					} else {
						isds_pki_credentials_free(&pki_credentials);
						return;
					}
				}
			}
			status = isdsLoginUserCertPwd(
			    isdsSessions.session(accountInfo.userName()),
			    accountInfo.userName(), password,
			    pki_credentials, accountInfo.testAccount());

			isds_pki_credentials_free(&pki_credentials);
		}

	/* Login method based username and otp */
	} else {
		if (password.isEmpty()) {
			bool ok;
			QString text = "";
			while (text.isEmpty()) {
				text = QInputDialog::getText(0,
				    QObject::tr("Enter password"),
				    QObject::tr("Enter password for account ") +
				    accountInfo.userName(),
				    QLineEdit::Password, "", &ok);
				if (ok) {
					if (!text.isEmpty()) {
						password = text;
					}
				} else {
					return;
				}
			}
		}
		isds_otp *opt = NULL;

		if (isdsSessions.holdsSession(accountInfo.userName())) {
			status = isdsLoginUserOtp(
			    isdsSessions.session(accountInfo.userName()),
			    accountInfo.userName(), password,
			    opt, accountInfo.testAccount());
		}

	}

	if (IE_NOT_LOGGED_IN == status) {
		QMessageBox::warning(0, QObject::tr("Authentication fails"),
		    QObject::tr("Authentication fails")
		    + "\n" + QObject::tr("ErrorType: ") + isds_strerror(status),
		    QMessageBox::Ok);
		return;
	} else if (IE_PARTIAL_SUCCESS == status) {
		QMessageBox::warning(0, QObject::tr("OTP authentication fails"),
		    QObject::tr("OTP authentication fails")
		    + "\n" + QObject::tr("ErrorType: ") + isds_strerror(status),
		    QMessageBox::Ok);
		return;
	} else if (IE_SUCCESS != status) {
		QMessageBox::warning(0, QObject::tr("Error occurred"),
		    QObject::tr("An error occurred while connect to ISDS")
		    + "\n" + QObject::tr("ErrorType: ") + isds_strerror(status),
		    QMessageBox::Ok);
		return;
	}
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
	    pwd.toStdString().c_str(), NULL, NULL);
}


/* ========================================================================= */
/*
 * Log in using certificate.
 */
isds_error isdsLoginCert(struct isds_ctx *isdsSession,
    isds_pki_credentials *pkiCredentials, bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(0 != pkiCredentials);

	/* TODO */
	return isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    NULL, NULL, pkiCredentials, NULL);
}


/* ========================================================================= */
/*
 * Log in using certificate.
 */
isds_error isdsLoginUserCert(struct isds_ctx *isdsSession,
    const QString &boxId, isds_pki_credentials *pkiCredentials,
    bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!boxId.isEmpty());
	Q_ASSERT(0 != pkiCredentials);

	/* TODO */
	return isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    boxId.toStdString().c_str(), NULL, pkiCredentials, NULL);
}


/* ========================================================================= */
/*!
 * @brief Log in using certificate.
 */
isds_error isdsLoginUserCertPwd(struct isds_ctx *isdsSession,
    const QString &boxId, const QString &pwd,
    isds_pki_credentials *pkiCredentials, bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!boxId.isEmpty());
	Q_ASSERT(!pwd.isEmpty());
	Q_ASSERT(0 != pkiCredentials);

	/* TODO */
	return isds_login(isdsSession,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    boxId.toStdString().c_str(), pwd.toStdString().c_str(),
	    pkiCredentials, NULL);
}


/* ========================================================================= */
/*
 * Log in using opt.
 */
isds_error isdsLoginUserOtp(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, isds_otp *opt,
    bool testingSession)
/* ========================================================================= */
{
	Q_ASSERT(0 != isdsSession);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!pwd.isEmpty());
	Q_ASSERT(0 != opt);

	/* TODO */
	return isds_login(isdsSession,
	    testingSession ? isds_otp_testing_locator : isds_otp_locator,
	    userName.toStdString().c_str(), pwd.toStdString().c_str(), NULL,
	    opt);
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


