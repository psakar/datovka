

#include <QDebug>

#include "isds_sessions.h"
#include "src/common.h"


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
		qDebug() << "Unsuccessful ISDS initialisation.";
		/* TODO -- What to do on failure? */
	}

	/* Logging. */
	isds_set_logging(ILF_ALL, ILL_ALL);
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
			qDebug() << "Error ISDS logout procedure.";
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


GlobIsdsSessions isdsSessions;


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
