

#ifndef _ISDS_SESSIONS_H_
#define _ISDS_SESSIONS_H_

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <isds.h>
#include <QMap>
#include <QString>

#include "src/common.h"
#include "src/models/accounts_model.h"


/* TODO -- Check whether session is active. */

/* Global ISDS context container instance. */
class GlobIsdsSessions;
extern GlobIsdsSessions isdsSessions;


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
	 * @brief Returns associated session.
	 */
	struct isds_ctx * session(const QString &userName) const;

	/*!
	 * @brief Ping of ISDS. Test if connection is active.
	 */
	bool isConnectToIsds(const QString userName);

	/*!
	 * @brief Creates new session.
	 *
	 * @return Pointer to new session or NULL on failure.
	 */
	struct isds_ctx * createCleanSession(const QString &userName);


private:
	QMap<QString, struct isds_ctx *> m_sessions;
};


/*!
 * @brief Log in using user name and password.
 */
isds_error isdsLoginUserName(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, bool testingSession);


/*!
 * @brief Log in using system certificate.
 */
isds_error isdsLoginSystemCert(struct isds_ctx *isdsSession,
    const QString &certPath, bool testingSession);


/*!
 * @brief Log in using user certificate without password.
 * NOTE: It need ID of Databox instead username
 */
isds_error isdsLoginUserCert(struct isds_ctx *isdsSession,
    const QString &idBox, const QString &certPath, bool testingSession);


/*!
 * @brief Log in using user certificate with password.
 */
isds_error isdsLoginUserCertPwd(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, const QString &certPath,
    bool testingSession);


/*!
 * @brief Log in using username, pwd and OTP.
 */
isds_error isdsLoginUserOtp(struct isds_ctx *isdsSession,
    const QString &userName, const QString &pwd, bool testingSession,
    const QString &otpMethod, const QString &otpCode, isds_otp_resolution &res);

/*!
 * @brief Add items into isds_PersonName structure.
 */
 isds_PersonName * isds_PersonName_add(const QString &pnFirstName,
    const QString &pnMiddleName, const QString &pnLastName,
    const QString &pnLastNameAtBirth);

/*!
 * @brief Add items into isds_Address structure.
 */
isds_Address * isds_Address_add(const QString &adCity,
    const QString &adStreet, const QString &adNumberInStreet,
    const QString &adNumberInMunicipality, const QString &adZipCode,
    const QString &adState);

/*!
 * @brief Add items into isds_BirthInfo structure.
 */
 isds_BirthInfo * isds_BirthInfo_add(struct tm *biDate,
    const QString &biCity, const QString &biCountry,
    const QString &biState);

/*!
 * @brief Create DbOwnerInfo structure and Search DataBoxes.
 */
isds_error isds_DbOwnerInfo_search(struct isds_list **result, const QString &userName,
    const QString &dbID,
    isds_DbType dbType, const QString &ic,
    struct isds_PersonName *personName, const QString &firmName,
    struct isds_BirthInfo *birthInfo, struct isds_Address *address,
    const QString &nationality, const QString &email, const QString telNumber,
    const QString &identifier, const QString &registryCode, long int dbState,
    bool dbEffectiveOVM, bool dbOpenAddressing);


/*!
 * @brief Create DbUserInfo structure.
 */
isds_DbUserInfo  * isds_DbOwnerInfo_add(const QString &userID,
    isds_UserType userType, long int userPrivils,
    struct isds_PersonName *personName, struct isds_Address *address,
    const QString &ic, const QString &firmName, const QString &caStreet,
    const QString &caCity, const QString &caZipCode, const QString &caState);


/*!
 * @brief Create a isds message from zfo file.
 *
 * @param[in] isdsSession Pointer to session context.
 * @param[in] fName       File name.
 * @return Pointer to message structure, NULL on error.
 */
struct isds_message * loadZfoFile(struct isds_ctx *isdsSession,
    const QString &fName);

#endif /* _ISDS_SESSIONS_H_ */
