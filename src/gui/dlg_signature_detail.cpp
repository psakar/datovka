

#include <QDateTime>
#include <QSslCertificate>
#include <QTimeZone>

#include "dlg_signature_detail.h"
#include "src/crypto/crypto.h"
#include "src/log/log.h"
#include "ui_dlg_signature_detail.h"


/* ========================================================================= */
/*
 * Constructor.
 */
DlgSignatureDetail::DlgSignatureDetail(const MessageDb &messageDb, int dmId,
    QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_msgDER(messageDb.msgsVerificationAttempted(dmId) ?
        messageDb.msgsMessageDER(dmId): QByteArray()),
    m_tstDER(messageDb.msgsTimestampDER(dmId)),
    m_constructedFromDb(true),
    m_dbIsVerified(messageDb.msgsVerified(dmId))
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
    m_dbIsVerified(false)
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
}


/* ========================================================================= */
/*
 * Show/hide certificate details
 */
void DlgSignatureDetail::showCertificateDetail(int state)
/* ========================================================================= */
{
	this->certDetailWidget->setHidden(Qt::Unchecked == state);
}


/* ========================================================================= */
/*
 * Show/hide Verification Details
 */
void DlgSignatureDetail::showVerificationDetail(int state)
/* ========================================================================= */
{
	this->verifyWidget->setHidden(Qt::Unchecked == state);
}


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
			resStr = "<b>" + QObject::tr("Valid: ") + "</b>";
			resStr += "<span style=\"color:#aa0000;\"><b>";
			resStr += QObject::tr("No");
			resStr += "</b></span>";
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr = "<b>" + QObject::tr("Valid: ") + "</b>";
			resStr += "<span style=\"color:#00aa00;\"><b>";
			resStr += QObject::tr("Yes");
			resStr += "</b></span>";
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
		resStr += QObject::tr("Cannot check signing certificate");
		this->showCertDetail->setHidden(true);
		this->showVerifyDetail->setHidden(true);
	} else if (!msgSigningCertValid()) {
		iconPath = ICON_16x16_PATH "datovka-error.png";
		resStr = "<b>" + QObject::tr("Valid: ") + "</b>";
		resStr += "<span style=\"color:#aa0000;\"><b>";
		resStr += QObject::tr("No");
		resStr += "</b></span>";
	} else {
		iconPath = ICON_16x16_PATH "datovka-ok.png";
		resStr = "<b>" + QObject::tr("Valid: ") + "</b>";
		resStr += "<span style=\"color:#00aa00;\"><b>";
		resStr += QObject::tr("Yes");
		resStr += "</b></span>";
	}

	this->cImage->setIcon(QIcon(iconPath));
	this->cStatus->setTextFormat(Qt::RichText);
	this->cStatus->setText(resStr);
	this->cDetail->setText(QString());

	QString saId, saName;
	QSslCertificate signingCert = msgSigningCert(saId, saName);

	/* TODO -- Various check results. */

	resStr.clear();
	if (!signingCert.isNull()) {
		QStringList strList;

		/* Certificate information. */
		resStr = "<b>" + QObject::tr("Version: ") + "</b>" +
		    QString(signingCert.version()) + "<br/>";
		resStr += "<b>" + QObject::tr("Serial number: ") + "</b>" +
		    QString(signingCert.serialNumber()) + " (" +
		    QString::number( /* Convert do decimal. */
		        ("0x" + QString(signingCert.serialNumber()).replace(
		                    ":", "")).toUInt(0, 16), 10) + ")<br/>";
		resStr += "<b>" + QObject::tr("Signature algorithm: ") +
		    "</b>" + saId + " (" + saName + ")<br/>";

		resStr += "<b>" + QObject::tr("Issuer: ") + "</b><br/>";
		strList = signingCert.issuerInfo(
		    QSslCertificate::Organization);
		if (strList.size() > 0) {
			Q_ASSERT(1 == strList.size());
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Organisation: ") +
			    strList.first() + "<br/>";
		}
		strList = signingCert.issuerInfo(
		    QSslCertificate::CommonName);
		if (strList.size() > 0) {
			Q_ASSERT(1 == strList.size());
			resStr += "&nbsp;&nbsp;" + QObject::tr("Name: ") +
			    strList.first() + "<br/>";
		}
		strList = signingCert.issuerInfo(
		    QSslCertificate::CountryName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + QObject::tr("Country: ") +
			    strList.first() + "<br/>";
		}

		resStr += "<b>" + QObject::tr("Validity: ") + "</b><br/>";
		/*
		 * QSslCertificate::effectiveDate() and
		 * QSslCertificate::expiryDate() tend to wrong time zone
		 * conversion.
		 */
		QDateTime incept, expir;
		if (msgSigningCertTimes(incept, expir)) {
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Valid from: ") +
			    incept.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    incept.timeZone().abbreviation(incept) + "<br/>";
			resStr += "&nbsp;&nbsp;" + QObject::tr("Valid to: ") +
			    expir.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    expir.timeZone().abbreviation(expir) + "<br/>";
		}

		resStr += "<b>" + QObject::tr("Subject: ") + "</b><br/>";
		strList = signingCert.subjectInfo(
		    QSslCertificate::Organization);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Organisation: ") +
			    strList.first() + "<br/>";
		}
		strList = signingCert.subjectInfo(
		    QSslCertificate::CommonName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + QObject::tr("Name: ") +
			    strList.first() + "<br/>";
		}
		strList = signingCert.subjectInfo(
		    QSslCertificate::SerialNumber);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" +
			    QObject::tr("Serial number: ") +
			    strList.first() + "<br/>";
		}
		strList = signingCert.subjectInfo(
		    QSslCertificate::CountryName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + QObject::tr("Country: ") +
			    strList.first() + "<br/>";
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
		resStr = "<b>" + QObject::tr("Valid: ") + "</b>";
		if (1 != ret) {
			iconPath = ICON_16x16_PATH "datovka-error.png";
			resStr += "<span style=\"color:#aa0000;\"><b>";
			resStr += QObject::tr("No");
			resStr += "</b></span>";
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr += "<span style=\"color:#00aa00;\"><b>";
			resStr += QObject::tr("Yes");
			resStr += "</b></span>";

			detailStr = "<b>" + QObject::tr("Time:") + "</b> " +
			    tst.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    tst.timeZone().abbreviation(tst) + "<br/>";

			QString o, ou, n, c;
			tstInfo(o, ou, n, c);

			detailStr += "<b>" + QObject::tr("Issuer:") +
			    "</b><br/>";
			if (!o.isEmpty()) {
				detailStr += "&nbsp;&nbsp;" +
				    QObject::tr("Organisation: ") + o +
				    "<br/>";
			}
			if (!ou.isEmpty()) {
				detailStr += "&nbsp;&nbsp;" +
				    QObject::tr("Organisational unit: ") + ou +
				    "<br/>";
			}
			if (!n.isEmpty()) {
				detailStr += "&nbsp;&nbsp;" +
				    QObject::tr("Name: ") + n + "<br/>";
			}
			if (!c.isEmpty()) {
				detailStr += "&nbsp;&nbsp;" +
				    QObject::tr("Country: ") + c + "<br/>";
			}

			this->tDetail->setAlignment(Qt::AlignLeft);
			this->tDetail->setTextFormat(Qt::RichText);
			this->tDetail->setText(detailStr);
		}
	}

	this->tImage->setIcon(QIcon(iconPath));
	this->tStatus->setTextFormat(Qt::RichText);
	this->tStatus->setText(resStr);
}


/* ========================================================================= */
/*
 * Return whether signing certificate is valid.
 */
bool DlgSignatureDetail::msgSigningCertValid(void) const
/* ========================================================================= */
{
	struct x509_crt *signing_cert = NULL;
	int ret;

	debugFuncCall();

	if (m_msgDER.isEmpty()) {
		goto fail;
	}

	signing_cert = raw_cms_signing_cert(m_msgDER.data(), m_msgDER.size());
	if (NULL == signing_cert) {
		goto fail;
	}

	ret = x509_crt_verify(signing_cert);

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
QSslCertificate DlgSignatureDetail::msgSigningCert(QString &saId,
    QString &saName) const
/* ========================================================================= */
{
	struct x509_crt *x509_crt = NULL;
	void *der = NULL;
	size_t der_size;
	char *sa_id = NULL, *sa_name = NULL;

	debugFuncCall();

	if (m_msgDER.isEmpty()) {
		return QSslCertificate();
	}

	x509_crt = raw_cms_signing_cert(m_msgDER.data(), m_msgDER.size());
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
bool DlgSignatureDetail::msgSigningCertTimes(QDateTime &incTime,
    QDateTime &expTime) const
/* ========================================================================= */
{
	struct x509_crt *x509_crt = NULL;
	time_t incept, expir;

	debugFuncCall();

	if (m_msgDER.isEmpty()) {
		return false;
	}

	x509_crt = raw_cms_signing_cert(m_msgDER.data(), m_msgDER.size());
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
 * Time stamp certificate information.
 */
bool DlgSignatureDetail::tstInfo(QString &oStr, QString &ouStr, QString &nStr,
    QString &cStr) const
/* ========================================================================= */
{
	struct x509_crt *signing_cert = NULL;
	struct crt_issuer_info cii;

	debugFuncCall();

	crt_issuer_info_init(&cii);

	if (m_tstDER.isEmpty()) {
		return false;
	}

	signing_cert = raw_cms_signing_cert(m_tstDER.data(), m_tstDER.size());
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
