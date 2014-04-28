

#ifndef _PKCS7_H_
#define _PKCS7_H_


#include <QDateTime>
#include <QSslCertificate>


/*!
 * Certificate storage.
 */
class CertStore {
public:
	CertStore(void);

	QList<QSslCertificate> caCerts; /*!< List of system CA certificates. */
};


/*!
 * @brief Check certificate validity at given time.
 */
bool certTimeValidAtTime(const QSslCertificate &cert,
    const QDateTime &dateTime);

/*!
 * @brief Check whether the certificate was not on CRL on given date.
 */
bool certCrlValidAtTime(const QSslCertificate &cert,
    const QDateTime &dateTime);

/*!
 * @brief Get certificate revocation date.
 */
QDateTime certRevocationDate(const QSslCertificate &cert);


#endif /* _PKCS7_H_ */
