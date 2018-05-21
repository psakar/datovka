/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include "src/crypto/crypto.h"
#include "src/crypto/crypto_funcs.h"
#include "src/global.h"
#include "src/gui/dlg_signature_detail.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "ui_dlg_signature_detail.h"

DlgSignatureDetail::DlgSignatureDetail(const QByteArray &msgDER,
    const QByteArray &tstDER, bool constructedFromDb, bool dbIsVerified,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgSignatureDetail),
    m_msgDER(msgDER),
    m_tstDER(tstDER),
    m_constructedFromDb(constructedFromDb),
    m_dbIsVerified(dbIsVerified),
    m_dlgSize()
{
	m_ui->setupUi(this);

	m_ui->verifyWidget->setHidden(true);
	connect(m_ui->showVerifyDetail, SIGNAL(stateChanged(int)),
	    this, SLOT(showVerificationDetail(int)));
	m_ui->certDetailWidget->setHidden(true);
	connect(m_ui->showCertDetail, SIGNAL(stateChanged(int)),
	    this, SLOT(showCertificateDetail(int)));

	validateMessageSignature();
	validateSigningCertificate();
	validateMessageTimestamp();

	/* Remember the size of the dialogue. */
	m_dlgSize = this->sizeHint();
}

DlgSignatureDetail::~DlgSignatureDetail(void)
{
	delete m_ui;
}

void DlgSignatureDetail::detail(const MessageDbSet &dbSet,
    const MessageDb::MsgId &msgId, QWidget *parent)
{
	if (Q_UNLIKELY((msgId.dmId < 0) || !msgId.deliveryTime.isValid())) {
		Q_ASSERT(0);
		return;
	}

	/* Obtain raw message and time stamp. */
	MessageDb *messageDb = dbSet.constAccessMessageDb(msgId.deliveryTime);
	if (Q_UNLIKELY(Q_NULLPTR == messageDb)) {
		Q_ASSERT(0);
		return;
	}
	QByteArray msgDER;
	MessageDb::MsgVerificationResult vRes =
	    messageDb->isMessageVerified(msgId.dmId);
	bool isMsgVerified = (vRes == MessageDb::MSG_SIG_OK);

	switch (vRes) {
	case MessageDb::MSG_SIG_OK:
	case MessageDb::MSG_SIG_BAD:
		msgDER = messageDb->getCompleteMessageRaw(msgId.dmId);
		break;
	default:
		logWarningNL("No complete message '%s' in the database for signature verification.",
		    QString::number(msgId.dmId).toUtf8().constData());
		break;
	}

	QByteArray tstDER(messageDb->getMessageTimestampRaw(msgId.dmId));

	DlgSignatureDetail dlg(msgDER, tstDER, true, isMsgVerified, parent);
	dlg.exec();
}

void DlgSignatureDetail::detail(const QByteArray &msgDER,
    const QByteArray &tstDER, QWidget *parent)
{
	DlgSignatureDetail dlg(msgDER, tstDER, false, false, parent);
	dlg.exec();
}

/*!
 * @brief Returns signing certificate inception and expiration date.
 *
 * @param[in]  DER Raw message or time stamp data.
 * @param[out] incTime Inception time.
 * @param[out] expTime Expiration time.
 * @return True on success.
 */
static
bool signingCertTimes(const QByteArray &DER, QDateTime &incTime,
    QDateTime &expTime)
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

bool DlgSignatureDetail::signingCertExpiresBefore(const QByteArray &DER,
    int days, QDateTime dDate)
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

void DlgSignatureDetail::showCertificateDetail(int checkState)
{
	m_ui->certDetailWidget->setHidden(Qt::Unchecked == checkState);
	this->setMaximumSize(m_dlgSize);
}

void DlgSignatureDetail::showVerificationDetail(int checkState)
{
	m_ui->verifyWidget->setHidden(Qt::Unchecked == checkState);
	this->setMaximumSize(m_dlgSize);
}

#define YES \
	("<span style=\"color:#008800;\"><b>" + \
	DlgSignatureDetail::tr("Yes") + "</b></span>")
#define NO \
	("<span style=\"color:#880000;\"><b>" + \
	DlgSignatureDetail::tr("No") + "</b></span>")
#define UNAVAILABLE \
	("<span style=\"color:#f7910e;\"><b>" + \
	DlgSignatureDetail::tr("Information not available") + "</b></span>")

void DlgSignatureDetail::validateMessageSignature(void)
{
	QString iconPath;
	QString resStr;

	if (m_msgDER.isEmpty()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = tr("Message signature is not present.");
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
			resStr = "<b>" + tr("Valid") + ": </b>";
			resStr += NO;
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr = "<b>" + tr("Valid") + ": </b>";
			resStr += YES;
		}
	}

	m_ui->mSignatureImage->setIcon(QIcon(iconPath));
	m_ui->mSignatureStatus->setTextFormat(Qt::RichText);
	m_ui->mSignatureStatus->setText(resStr);
}

/*!
 * @brief Returns signing certificate of message.
 *
 * @param[in]  DER Raw message or time stamp data.
 * @param[out] saId Signature algorithm identifier.
 * @param[out] saName Signature algorithm name.
 * @return Null certificate on failure.
 */
static
QSslCertificate signingCert(const QByteArray &DER,
    QString &saId, QString &saName)
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

/*!
 * @brief Return whether signing certificate is valid.
 *
 * @param[in] DER Raw message or time stamp data.
 * @return True is signing certificate was verified successfully.
 */
static
bool signingCertValid(const QByteArray &DER, struct crt_verif_outcome &cvo)
{
	struct x509_crt *signing_cert = NULL;
	int ret;

	debugFuncCall();

	if (DER.isEmpty()) {
		return false;
	}

	signing_cert = raw_cms_signing_cert(DER.data(), DER.size());
	if (NULL == signing_cert) {
		return false;
	}

	ret = x509_crt_verify(signing_cert);

	x509_crt_track_verification(signing_cert, &cvo);

	x509_crt_destroy(signing_cert); signing_cert = NULL;
	return 1 == ret;
}

void DlgSignatureDetail::validateSigningCertificate(void)
{
	QString iconPath;
	QString resStr;

	m_ui->showCertDetail->setHidden(false);
	m_ui->showVerifyDetail->setHidden(false);

	if (m_msgDER.isEmpty()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = tr("Message signature is not present.") + "<br/>";
		resStr += tr("Cannot check signing certificate.");
		m_ui->showCertDetail->setHidden(true);
		m_ui->showVerifyDetail->setHidden(true);

		return;
	}

	struct crt_verif_outcome cvo;

	resStr = "<b>" + tr("Valid") + ": </b>";

	if (!signingCertValid(m_msgDER, cvo)) {
		iconPath = ICON_16x16_PATH "datovka-error.png";
		resStr += NO;
	} else {
		iconPath = ICON_16x16_PATH "datovka-ok.png";
		resStr += YES;
	}

	if (!GlobInstcs::prefsPtr->checkCrl) {
//		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr += " <b>(" +
		    tr("Certificate revocation check is turned off!") + ")</b>";
	}

	m_ui->cImage->setIcon(QIcon(iconPath));
	m_ui->cStatus->setTextFormat(Qt::RichText);
	m_ui->cStatus->setText(resStr);
	m_ui->cDetail->setText(QString());

	QString saId, saName;
	QSslCertificate signingCrt = signingCert(m_msgDER, saId, saName);

	resStr.clear();
	if (!signingCrt.isNull()) {
		/* TODO -- Various check results. */
		QString checkResult;

		checkResult = crypto_certificates_loaded() ? YES : NO;
		resStr = "<b>" + tr("Trusted certificates were found") +
		    ": </b>" + checkResult + "<br/>";

#if 0
		resStr += "<b>" + tr("Signing algorithm supported") +
		    ": </b>" + "n/a<br/>";
#endif

		checkResult = cvo.parent_crt_not_found ? NO : YES;
		resStr += "<b>" + tr("Trusted parent certificate found") +
		    ": </b>" + checkResult + "<br/>";

		checkResult = cvo.time_validity_fail ? NO : YES;
		resStr += "<b>" + tr("Certificate time validity is ok") +
		    ": </b>" + checkResult + "<br/>";

		if (!GlobInstcs::prefsPtr->checkCrl) {
			checkResult = UNAVAILABLE;
		} else {
			checkResult = cvo.crt_revoked ? NO : YES;
		}
		resStr += "<b>" + tr("Certificate was not revoked") +
		    ": </b>" + checkResult + "<br/>";
		if (!GlobInstcs::prefsPtr->checkCrl) {
			resStr += "&nbsp;&nbsp;" "<i>" +
			    tr("Certificate revocation check is turned off!") +
			    "</i><br/>";
		}

		checkResult = cvo.crt_signature_invalid ? NO : YES;
		resStr += "<b>" + tr("Certificate signature verified") +
		    ": </b>" + checkResult + "<br/>";

		m_ui->vDetail->setTextFormat(Qt::RichText);
		m_ui->vDetail->setText(resStr);
	}

	resStr.clear();
	if (!signingCrt.isNull()) {
		QStringList strList;

		/* Certificate information. */
		resStr = "<b>" + tr("Version") + ": </b>" +
		    QString(signingCrt.version()) + "<br/>";
		resStr += "<b>" + tr("Serial number") + ": </b>" +
		    QString(signingCrt.serialNumber()) + " (" +
		    QString::number( /* Convert do decimal. */
		        ("0x" + QString(signingCrt.serialNumber()).replace(
		                    ":", "")).toUInt(0, 16), 10) + ")<br/>";
		resStr += "<b>" + tr("Signature algorithm") + ": </b>" +
		    saId + " (" + saName + ")<br/>";

		resStr += "<b>" + tr("Issuer") + ": </b><br/>";
		strList = signingCrt.issuerInfo(
		    QSslCertificate::Organization);
		if (strList.size() > 0) {
			Q_ASSERT(1 == strList.size());
			resStr += "&nbsp;&nbsp;" + tr("Organisation") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.issuerInfo(
		    QSslCertificate::CommonName);
		if (strList.size() > 0) {
			Q_ASSERT(1 == strList.size());
			resStr += "&nbsp;&nbsp;" + tr("Name") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.issuerInfo(
		    QSslCertificate::CountryName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + tr("Country") + ": " +
			    strList.first() + "<br/>";
		}

		resStr += "<b>" + tr("Validity") + ": </b><br/>";
		/*
		 * QSslCertificate::effectiveDate() and
		 * QSslCertificate::expiryDate() tend to wrong time zone
		 * conversion.
		 */
		QDateTime incept, expir;
		if (signingCertTimes(m_msgDER, incept, expir)) {
			resStr += "&nbsp;&nbsp;" +
			    tr("Valid from") + ": " +
			    incept.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    incept.timeZone().abbreviation(incept) + "<br/>";
			resStr += "&nbsp;&nbsp;" + tr("Valid to") + ": " +
			    expir.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    expir.timeZone().abbreviation(expir) + "<br/>";
		}

		resStr += "<b>" + tr("Subject") + ": </b><br/>";
		strList = signingCrt.subjectInfo(
		    QSslCertificate::Organization);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + tr("Organisation") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.subjectInfo(
		    QSslCertificate::CommonName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + tr("Name") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.subjectInfo(
		    QSslCertificate::SerialNumber);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + tr("Serial number") + ": " +
			    strList.first() + "<br/>";
		}
		strList = signingCrt.subjectInfo(
		    QSslCertificate::CountryName);
		if (strList.size() > 0) {
			resStr += "&nbsp;&nbsp;" + tr("Country") + ": " +
			    strList.first() + "<br/>";
		}

		m_ui->cDetail->setTextFormat(Qt::RichText);
		m_ui->cDetail->setText(resStr);
	}
}

/*!
 * @brief Signing certificate issuer information.
 *
 * @param[in]  DER Raw message or time stamp data.
 * @param[out] oStr Organisation name.
 * @param[out] ouStr Organisation unit name.
 * @param[out] nStr Common name.
 * @param[out] cStr Country name.
 * @return False on failure.
 */
static
bool signingCertIssuerInfo(const QByteArray &DER, QString &oStr, QString &ouStr,
    QString &nStr, QString &cStr)
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

void DlgSignatureDetail::validateMessageTimestamp(void)
{
	QString iconPath;
	QString resStr;
	QString detailStr;

	QDateTime tst;
	if (m_tstDER.isEmpty()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = tr("Time stamp not present.");
	} else {
		time_t utc_time = 0;
		int ret = raw_tst_verify(m_tstDER.data(), m_tstDER.size(),
		    &utc_time);

		if (-1 != ret) {
			tst = QDateTime::fromTime_t(utc_time);
		}
		resStr = "<b>" + tr("Valid") + ": </b>";
		if (1 != ret) {
			iconPath = ICON_16x16_PATH "datovka-error.png";
			resStr += NO;
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr += YES;
		}

		detailStr = "<b>" + tr("Time") + ": </b> " +
		    tst.toString("dd.MM.yyyy hh:mm:ss") + " " +
		    tst.timeZone().abbreviation(tst) + "<br/>";

		QString o, ou, n, c;
		signingCertIssuerInfo(m_tstDER, o, ou, n, c);

		detailStr += "<b>" + tr("Issuer") + ": </b><br/>";
		if (!o.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" + tr("Organisation") +
			    ": " + o + "<br/>";
		}
		if (!ou.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" +
			    tr("Organisational unit") + ": " + ou + "<br/>";
		}
		if (!n.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" + tr("Name") + ": " +
			    n + "<br/>";
		}
		if (!c.isEmpty()) {
			detailStr += "&nbsp;&nbsp;" + tr("Country") + ": " +
			    c + "<br/>";
		}

		QDateTime incept, expir;
		if (signingCertTimes(m_tstDER, incept, expir)) {
			detailStr += "<b>" + tr("Validity") + ": </b><br/>";
			detailStr += "&nbsp;&nbsp;" + tr("Valid from") + ": " +
			    incept.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    incept.timeZone().abbreviation(incept) + "<br/>";
			detailStr += "&nbsp;&nbsp;" + tr("Valid to") + ": " +
			    expir.toString("dd.MM.yyyy hh:mm:ss") + " " +
			    expir.timeZone().abbreviation(expir) + "<br/>";
		}

		m_ui->tDetail->setAlignment(Qt::AlignLeft);
		m_ui->tDetail->setTextFormat(Qt::RichText);
		m_ui->tDetail->setText(detailStr);
	}

	m_ui->tImage->setIcon(QIcon(iconPath));
	m_ui->tStatus->setTextFormat(Qt::RichText);
	m_ui->tStatus->setText(resStr);
}
