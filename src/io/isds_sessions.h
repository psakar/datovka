

#ifndef _ISDS_SESSIONS_H_
#define _ISDS_SESSIONS_H_


#include <cstdbool>
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
	 * @brief Creates new session.
	 *
	 * @return Pointer to new session or NULL on failure.
	 */
	struct isds_ctx * createCleanSession(const QString &userName);

	/*!
	 * @brief Returns associated session.
	 */
	struct isds_ctx * session(const QString &userName) const;


	void connectToIsds(const AccountModel::SettingsMap &accountInfo);

	bool isConnectToIsds(const QString userName);

private:
	QMap<QString, struct isds_ctx *> m_sessions;
};


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
void isds_DbOwnerInfo_search(struct isds_list **result, const QString &userName,
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

#endif /* _ISDS_SESSIONS_H_ */
