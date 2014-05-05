

#ifndef _ISDS_SESSIONS_H_
#define _ISDS_SESSIONS_H_


#include <cstdbool>
#include <isds.h>
#include <QMap>
#include <QString>


/* TODO -- Check whether session is active. */


/*!
 * @brief Holds the ISDS context structures.
 */
class GlobIsdsSessions {

public:
	GlobIsdsSessions(void);
	~GlobIsdsSessions(void);

	/*!
	 * @brief Returns true is active session exists.
	 */
	bool holdsSession(const QString &userName) const;

	/*!
	 * @brief Creates new session.
	 *
	 * @return Pointer to new session or NULL on failure.
	 */
	struct isds_ctx * createCleanSession(const QString &userName);

	/*!
	 * @brief Returns associated session.
	 */
	struct isds_ctx * session(const QString &userName) const;

private:
	QMap<QString, struct isds_ctx *> m_sessions;
};


/* Global ISDS context container instance. */
extern GlobIsdsSessions isdsSessions;


/*!
 * @brief Log in using user name and password.
 */
isds_error isdsLoginUserName(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, bool testingSession);


/*!
 * @brief Log in using certificate.
 */
isds_error isdsLoginCert(struct isds_ctx *isdsSession,
    isds_pki_credentials *pkiCredentials, bool testingSession);


/*!
 * @brief Log in using certificate.
 */
isds_error isdsLoginUserCert(struct isds_ctx *isdsSession,
    const QString &boxId, isds_pki_credentials *pkiCredentials,
    bool testingSession);


/*!
 * @brief Log in using certificate.
 */
isds_error isdsLoginUserCertPwd(struct isds_ctx *isdsSession,
    const QString &boxId, const QString &pwd,
    isds_pki_credentials *pkiCredentials, bool testingSession);


/*!
 * @brief Log in using opt.
 */
isds_error isdsLoginUserOtp(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, isds_otp *opt,
    bool testingSession);


#endif /* _ISDS_SESSIONS_H_ */
