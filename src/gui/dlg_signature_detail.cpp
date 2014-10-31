

#include <QSslCertificate>

#include "dlg_signature_detail.h"
#include "ui_dlg_signature_detail.h"


/* ========================================================================= */
/*
 * Constructor.
 */
DlgSignatureDetail::DlgSignatureDetail(const MessageDb &messageDb, int dmId,
    QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_messageDb(messageDb),
    m_dmId(dmId)
{
	setupUi(this);
//	this->cImage->setIcon(QIcon(ICON_3PARTY_PATH "warning_16.png"));
//	this->tImage->setIcon(QIcon(ICON_16x16_PATH "datovka-ok.png"));

	validateMessageSignature();
	validateSigningCertificate();
	validateMessageTimestamp();
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

	if (!m_messageDb.msgsVerificationAttempted(m_dmId)) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = QObject::tr("Message signature is not present.");
	} else if (!m_messageDb.msgsVerified(m_dmId)) {
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

	if (!m_messageDb.msgsVerificationAttempted(m_dmId)) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = QObject::tr("Message signature is not present.") +
		    "<br/>";
		resStr += QObject::tr("Cannot check signing certificate");
	} else if (!m_messageDb.msgsSigningCertValid(m_dmId)) {
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

	QString saId, saName;
	QSslCertificate signingCert =
	    m_messageDb.rmsgdtSigningCertificate(m_dmId, saId, saName);

	/* TODO -- Various check results. */

	resStr.clear();
	if (!signingCert.isNull()) {
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
	bool valid = m_messageDb.msgsCheckTimestamp(m_dmId, tst);
	QString timeStampStr;
	if (!tst.isValid()) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		resStr = QObject::tr("Time stamp not present.");
	} else {
		resStr = "<b>" + QObject::tr("Valid: ") + "</b>";
		if (!valid) {
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
			    tst.toString("dd.MM.yyyy hh:mm:ss") + "<br/>";

			QString o, ou, n, c;
			m_messageDb.msgsTimestampInfo(m_dmId, o, ou, n, c);

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
