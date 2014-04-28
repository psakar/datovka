

#include <QSslSocket>

#include "pkcs7.h"


/* ========================================================================= */
CertStore::CertStore(void)
/* ========================================================================= */
{
	caCerts = QSslSocket::systemCaCertificates();
}


/* ========================================================================= */
bool certTimeValidAtTime(const QSslCertificate &cert,
    const QDateTime &dateTime)
/* ========================================================================= */
{
	QDateTime start, stop;

	start = cert.effectiveDate();
	stop = cert.expiryDate();

	return (start <= dateTime) && (dateTime <= stop);
}


/* ========================================================================= */
/*
 * Check whether the certificate was not on CRL on given date.
 */
bool certCrlValidAtTime(const QSslCertificate &cert,
    const QDateTime &dateTime)
/* ========================================================================= */
{
	QDateTime revocationDate = certRevocationDate(cert);

	if (!revocationDate.isValid()) {
		return true;
	} else {
		return revocationDate > dateTime;
	}
}


/* ========================================================================= */
/*
 * Get certificate revocation date.
 */
QDateTime certRevocationDate(const QSslCertificate &cert)
/* ========================================================================= */
{
	/* TODO */
	return QDateTime();
}
