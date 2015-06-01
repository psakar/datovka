/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include <QDateTime>
#include <QSslCertificate>
#include <QTimeZone>

#include "dlg_signature_detail.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_funcs.h"
#include "src/log/log.h"
#include "ui_dlg_signature_detail.h"


/* ========================================================================= */
/*
 * Constructor.
 */
DlgSignatureDetail::DlgSignatureDetail(const MessageDb &messageDb, qint64 dmId,
    QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_msgDER(messageDb.msgsVerificationAttempted(dmId) ?
        messageDb.msgsMessageRaw(dmId): QByteArray()),
    m_tstDER(messageDb.msgsTimestampRaw(dmId)),
    m_constructedFromDb(true),
    m_dbIsVerified(messageDb.msgsVerified(dmId)),
    dSize(QSize())
{
	setupUi(this);

	this->verifyWidget->setHidden(true);
	connect(this->showVerifyDetail, SIGNAL(stateChanged(int)),
	    this, SLOT(showVerificationDetail(int)));
	this->certDetailWidget->setHidden(true);
	connect(this->showCertDetail, SIGNAL(stateChanged(int)),
	    this, SLOT(showCertificateDetail(int)));

	validateMessageSignature();
	validateSigningCertificate();
	validateMessageTimestamp();

	/* remember size of dialog */
	dSize = this->sizeHint();
}


/* ========================================================================= */
/*
 * Constructor.
 */
DlgSignatureDetail::DlgSignatureDetail(const void *msgDER, size_t msgSize,
    const void *tstDER, size_t tstSize, QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_msgDER((char *) msgDER, msgSize),
    m_tstDER((char *) tstDER, tstSize),
    m_constructedFromDb(false),
    m_dbIsVerified(false),
    dSize(QSize())
{
	setupUi(this);

	this->verifyWidget->setHidden(true);
	connect(this->showVerifyDetail, SIGNAL(stateChanged(int)),
	    this, SLOT(showVerificationDetail(int)));
	this->certDetailWidget->setHidden(true);
	connect(this->showCertDetail, SIGNAL(stateChanged(int)),
	    this, SLOT(showCertificateDetail(int)));

	validateMessageSignature();
	validateSigningCertificate();
	validateMessageTimestamp();

	/* Remember the size of the dialog. */
	dSize = this->sizeHint();
}


/* ========================================================================= */
/*
 * Return whether signing certificate is valid.
 */
bool DlgSignatureDetail::signingCertValid(const QByteArray &DER,
    struct crt_verif_outcome &cvo)
/* ========================================================================= */
{
	struct x509_crt *signing_cert = NULL;
	int ret;

	debugFuncCall();

	if (DER.isEmpty()) {
		goto fail;
	}

	signing_cert = raw_cms_signing_cert(DER.data(), DER.size());
	if (NULL == signing_cert) {
		goto fail;
	}

	ret = x509_crt_verify(signing_cert);

	x509_crt_track_verification(signing_cert, &cvo);

	x509_crt_destroy(signing_cert); signing_cert = NULL;
	return 1 == ret;

fail:
	if (NULL != signing_cert) {
		x509_crt_destroy(signing_cert);
	}
	return false;
}


/* ========================================================================= */
/*
 * Returns signing certificate of message.
 */
QSslCertificate DlgSignatureDetail::signingCert(const QByteArray &DER,
     QString &saId, QString &saName)
/* ========================================================================= */
{
	struct x509_crt *x509_crt = NULL;
	void *der = NULL;
	size_t der_size;
	char *sa_id = NULL, *sa_name = NULL;

	debugFuncCall();

	if (DER.isEmpty()) {
		return QSslCertificate();
	}

	x509_crt = raw_cms_signing_cert(DER.data(), DER.size());
	if (NULL == x509_crt) {
		return QSslCertificate();
	}

	if (0 != x509_crt_to_der(x509_crt, &der, &der_size)) {
		x509_crt_destroy(x509_crt); x509_crt = NULL;
		return QSslCertificate();
	}

	if (0 != x509_crt_algorithm_info(x509_crt, &sa_id, &sa_name)) {
		x509_crt_destroy(x509_crt); x509_crt = NULL;
		return QSslCertificate();
	}

	x509_crt_destroy(x509_crt); x509_crt = NULL;

	saId = sa_id;
	saName = sa_name;

	free(sa_id); sa_id = NULL;
	free(sa_name); sa_name = NULL;

	QByteArray certRawBytes((char *) der, der_size);
	free(der); der = NULL;

	return QSslCertificate(certRawBytes, QSsl::Der);
}


/* ========================================================================= */
/*
 * Returns signing certificate inception and expiration date.
 */
bool DlgSignatureDetail::signingCertTimes(const QByteArray &DER,
    QDateTime &incTime, QDateTime &expTime)
/* ========================================================================= */
{
	struct x509_crt *x509_crt = NULL;
	time_t incept, expir;

	debugFuncCall();

	if (DER.isEmpty()) {
		return false;
	}

	x509_crt = raw_cms_signing_cert(DER.data(), DER.size());
	if (NULL == x509_crt) {
		return false;
	}

	if (0 != x509_crt_date_info(x509_crt, &incept, &expir)) {
		x509_crt_destroy(x509_crt); x509_crt = NULL;
		return false;
	}

	x509_crt_destroy(x509_crt); x509_crt = NULL;

	incTime = QDateTime::fromTime_t(incept);
	expTime = QDateTime::fromTime_t(expir);

	return true;
}


/* ========================================================================= */
/*
 * Check whether certificate expires before specified limit.
 */
bool DlgSignatureDetail::signingCertExpiresBefore(const QByteArray &DER,
    int days, QDateTime dDate)
/* ========================================================================= */
{
	QDateTime expir;
	QDateTime now;
	{
		QDateTime incep;
		if (!signingCertTimes(DER, incep, expir)) {
			return false;
		}
	}

	if (dDate.isValid()) {
		now = dDate;
	} else {
		now = QDateTime::currentDateTime();
	}

	int difference = now.daysTo(expir);

	return difference < days;
}


/* ========================================================================= */
/*
 * Signing certificate issuer information.
 */
bool DlgSignatureDetail::signingCertIssuerInfo(const QByteArray &DER,
    QString &oStr, QString &ouStr, QString &nStr, QString &cStr)
/* ========================================================================= */
{
	struct x509_crt *signing_cert = NULL;
	struct crt_issuer_info cii;

	debugFuncCall();

	crt_issuer_info_init(&cii);

	if (DER.isEmpty()) {
		return false;
	}

	signing_cert = raw_cms_signing_cert(DER.data(), DER.size());
	if (NULL == signing_cert) {
		goto fail;
	}

	if (0 != x509_crt_issuer_info(signing_cert, &cii)) {
		goto fail;
	}

	x509_crt_destroy(signing_cert); signing_cert = NULL;

	if (NULL != cii.o) {
		oStr = cii.o;
	}

	if (NULL != cii.ou) {
		ouStr = cii.ou;
	}

	if (NULL != cii.n) {
		nStr = cii.n;
	}

	if (NULL != cii.c) {
		cStr = cii.c;
	}

	crt_issuer_info_clear(&cii);

	return true;

fail:
	if (NULL != signing_cert) {
		x509_crt_destroy(signing_cert);
	}
	crt_issuer_info_clear(&cii);
	return false;
}


/* ========================================================================= */
/*
 * Show/hide certificate details
 */
void DlgSignatureDetail::showCertificateDetail(int state)
/* ========================================================================= */
{
	this->certDetailWidget->setHidden(Qt::Unchecked == state);
	this->setMaximumSize(dSize);
}


/* ========================================================================= */
/*
 * Show/hide Verification Details
 */
void DlgSignatureDetail::showVerificationDetail(int state)
/* ========================================================================= */
{
	this->verifyWidget->setHidden(Qt::Unchecked == state);
	this->setMaximumSize(dSize);
}


#define YES \
	("<span style=\"color:#008800;\"><b>" + QObject::tr("Yes") + \
	"</b></span>")
#define NO \
	("<span style=\"color:#880000;\"><b>" + QObject::tr("No") + \
	"</b></span>")
#define UNAVAILABLE \
	("<span style=\"color:#f7910e;\"><b>" + \
	QObject::tr("Information not available") + "</b></span>")


/* ========================================================================= */
/*
 * Check message signature, show result in dialog.
 */
void DlgSignatureDetail::validateMessageSignature(void)
/* ========================================================================= */
{
	QString iconPath;
	QString resStr;

	if (m_msgDER.isEmpty()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = QObject::tr("Message signature is not present.");
	} else {
		bool verified = false;

		if (m_constructedFromDb) {
			verified = m_dbIsVerified;
		} else {
			verified =
			    1 == raw_msg_verify_signature(m_msgDER.data(),
			        m_msgDER.size(), 0, 0);
		}

		if (!verified) {
			iconPath = ICON_16x16_PATH "datovka-error.png";
			resStr = "<b>" + QObject::tr("Valid") + ": </b>";
			resStr += NO;
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr = "<b>" + QObject::tr("Valid") + ": </b>";
			resStr += YES;
		}
	}

	this->mSignatureImage->setIcon(QIcon(iconPath));
	this->mSignatureStatus->setTextFormat(Qt::RichText);
	this->mSignatureStatus->setText(resStr);
}


/* ========================================================================= */
/*
 * Validate signing certificate, show result in dialog.
 */
void DlgSignatureDetail::validateSigningCertificate(void)
/* ========================================================================= */
{
	QString iconPath;
	QString resStr;

	this->showCertDetail->setHidden(false);
	this->showVerifyDetail->setHidden(false);

	if (m_msgDER.isEmpty()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = QObject::tr("Message signature is not present.") +
		    "<br/>";
		resStr += QObject::tr("Cannot check signing certificate.");
		this->showCertDetail->setHidden(true);
		this->showVerifyDetail->setHidden(true);

		return;
	}

	struct crt_verif_outcome cvo;

	resStr = "<b>" + QObject::tr("Valid") + ": </b>";

	if (!signingCertValid(m_msgDER, cvo)) {
		iconPath = ICON_16x16_PATH "datovka-error.png";
		resStr += NO;
	} else {
		iconPath = ICON_16x16_PATH "datovka-ok.png";
		resStr += YES;
	}

	if (!globPref.check_crl) {
//		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr += " <b>(" +
		    QObject::tr("Certificate revocation check is "
		        "turned off!") + ")</b>";
	}

	this->cImage->setIcon(QIcon(iconPath));
	this->cStatus->setTextFormat(Qt::RichText);
	this->cStatus->setText(resStr);
	this->cDetail->setText(QString());

	QString saId, saName;
	QSslCertificate signingCrt = signingCert(m_msgDER, saId, saName);

	resStr.clear();
	if (!signingCrt.isNull()) {
		/* TODO -- Various check results. */
		QString checkResult;

		checkResult = crypto_certificates_loaded() ? YES : NO;
		resStr = "<b>" +
		    QObject::tr("Trusted certificates were found") +
		    ": </b>" + checkResult + "<br/>";

#if 0
		resStr += "<b>" +
		    QObject::tr("Signing algorithm supported") +
		    ": </b>" + "n/a<br/>";
#endif

		checkResult = cvo.parent_crt_not_found ? NO : YES;
		resStr += "<b>" +
		    QObject::tr("Trusted parent certificate found") +
		    ": </b>" + checkResult + "<br/>";

		checkResult = cvo.time_validity_fail ? NO : YES;
		resStr += "<b>" +
		    QObject::tr("Certificate time validity is ok") +
		    ": </b>" + checkResult + "<br/>";

		if (!globPref.check_crl) {
			checkResult = UNAVAILABLE;
		} else {
			checkResult = cvo.crt_revoked ? NO : YES;
		}
		resStr += "<b>" +
		    QObject::tr("Certificate was not revoked") +
		    ": </b>" + checkResult + "<br/>";
		if (!globPref.check_crl) {
			resStr += "&nbsp;&nbsp;" "<i>" +
			    QObject::tr("Certificate revocation check is "
			        "turned off!") + "</i><br/>";
		}

		checkResult = cvo.crt_signature_invalid ? NO : YES;
		resStr += "<b>" +
		    QObject::tr("Certificate signature verified") +
		    ": </b>" + checkResult + "<br/>";

		this->vDetail->setTextFormat(Qt::RichText);
		this->vDetail->setText(resStr);
	}

	resStr.clear();
	if (!signingCrt.isNull()) {
		QStringList strList;

		/* Certificate information. */
		resStr = "<b>" + QObject::tr("Version") + ": </b>" +
		    QString(signingCrt.version()) + "<br/>";
		resStr += "<b>" + QObject::tr("Serial number") + ": </b>" +
		    QString(signingCrt.serialNumber()) + " (" +
		    QString::number( /* Convert do decimal. */
		        ("0x" + QString(signingCrt.serialNumber()).replace(
		                    ":", "")).toUInt(0, 16), 10) + ")<br/>";
		resStr += "<b>" + QObject::tr("Signature algorithm") +
		    ": </b>" + saId + " (" + saName + ")<br/>";

		resStr += "<b>" + QObject::tr("Issuer") + ": </b><br/>";
		strList = signingCrt.issuerInfo(
		    QSslCertificate::Organization);
		if (strList.size() > 0) {
			Q_ASSERT(1 == strList.size());
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Organisation") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.issuerInfo(
		    QSslCertificate::CommonName);
		if (strList.size() > 0) {
			Q_ASSERT(1 == strList.size());
			resStr += "&nbsp;&nbsp;" + QObject::tr("Name") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.issuerInfo(
		    QSslCertificate::CountryName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + QObject::tr("Country") +
			    ": " + strList.first() + "<br/>";
		}

		resStr += "<b>" + QObject::tr("Validity") + ": </b><br/>";
		/*
		 * QSslCertificate::effectiveDate() and
		 * QSslCertificate::expiryDate() tend to wrong time zone
		 * conversion.
		 */
		QDateTime incept, expir;
		if (signingCertTimes(m_msgDER, incept, expir)) {
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Valid from") + ": " +
			    incept.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    incept.timeZone().abbreviation(incept) + "<br/>";
			resStr += "&nbsp;&nbsp;" + QObject::tr("Valid to") +
			    ": " +expir.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    expir.timeZone().abbreviation(expir) + "<br/>";
		}

		resStr += "<b>" + QObject::tr("Subject") + ": </b><br/>";
		strList = signingCrt.subjectInfo(
		    QSslCertificate::Organization);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Organisation") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.subjectInfo(
		    QSslCertificate::CommonName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + QObject::tr("Name") +
			    ": " + strList.first() + "<br/>";
		}
		strList = signingCrt.subjectInfo(
		    QSslCertificate::SerialNumber);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Serial number") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.subjectInfo(
		    QSslCertificate::CountryName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + QObject::tr("Country") +
			    ": " + strList.first() + "<br/>";
		}

		this->cDetail->setTextFormat(Qt::RichText);
		this->cDetail->setText(resStr);
	}
}


/* ========================================================================= */
/*
 * Check time stamp signature, show detail in dialog.
 */
void DlgSignatureDetail::validateMessageTimestamp(void)
/* ========================================================================= */
{
	QString iconPath;
	QString resStr;
	QString detailStr;

	QDateTime tst;
	if (m_tstDER.isEmpty()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = QObject::tr("Time stamp not present.");
	} else {
		time_t utc_time = 0;
		int ret = raw_tst_verify(m_tstDER.data(), m_tstDER.size(),
		    &utc_time);

		if (-1 != ret) {
			tst = QDateTime::fromTime_t(utc_time);
		}
		resStr = "<b>" + QObject::tr("Valid") + ": </b>";
		if (1 != ret) {
			iconPath = ICON_16x16_PATH "datovka-error.png";
			resStr += NO;
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr += YES;
		}

		detailStr = "<b>" + QObject::tr("Time") + ": </b> " +
		    tst.toString("dd.MM.yyyy hh:mm:ss") + " " +
		    tst.timeZone().abbreviation(tst) + "<br/>";

		QString o, ou, n, c;
		signingCertIssuerInfo(m_tstDER, o, ou, n, c);

		detailStr += "<b>" + QObject::tr("Issuer") + ": </b><br/>";
		if (!o.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" +
			    QObject::tr("Organisation") + ": " + o + "<br/>";
		}
		if (!ou.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" +
			    QObject::tr("Organisational unit") + ": " + ou +
			    "<br/>";
		}
		if (!n.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" + QObject::tr("Name") +
			    ": " + n + "<br/>";
		}
		if (!c.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" +
			    QObject::tr("Country") + ": " + c + "<br/>";
		}

		QDateTime incept, expir;
		if (signingCertTimes(m_tstDER, incept, expir)) {
			detailStr += "<b>" + QObject::tr("Validity") +
			    ": </b><br/>";
			detailStr += "&nbsp;&nbsp;" +
			    QObject::tr("Valid from") + ": " +
			    incept.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    incept.timeZone().abbreviation(incept) + "<br/>";
			detailStr += "&nbsp;&nbsp;" +
			    QObject::tr("Valid to") + ": " +
			    expir.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    expir.timeZone().abbreviation(expir) + "<br/>";
		}

		this->tDetail->setAlignment(Qt::AlignLeft);
		this->tDetail->setTextFormat(Qt::RichText);
		this->tDetail->setText(detailStr);
	}

	this->tImage->setIcon(QIcon(iconPath));
	this->tStatus->setTextFormat(Qt::RichText);
	this->tStatus->setText(resStr);
}
