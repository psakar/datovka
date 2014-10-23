

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
		resStr = "<b>";
		resStr += QObject::tr("Message signature is not present.");
		resStr += "</b>";
	} else if (!m_messageDb.msgsVerified(m_dmId)) {
		iconPath = ICON_16x16_PATH "datovka-error.png";
		resStr = "<span style=\"color:#aa0000;\"><b>";
		resStr += QObject::tr("No");
		resStr += "</b></span>";
	} else {
		iconPath = ICON_16x16_PATH "datovka-ok.png";
		resStr = "<span style=\"color:#00aa00;\"><b>";
		resStr += QObject::tr("Yes");
		resStr += "</b></span>";
	}

	this->mSignatureImage->setIcon(QIcon(iconPath));
	this->mSignatureStatus->setTextFormat(Qt::RichText);
	this->mSignatureStatus->setText(resStr);
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
		resStr = "<b>";
		resStr += QObject::tr("Time stamp not present.");
		resStr += "</b>";
	} else {
		if (!valid) {
			iconPath = ICON_16x16_PATH "datovka-error.png";
			resStr = "<span style=\"color:#aa0000;\"><b>";
			resStr += QObject::tr("No");
			resStr += "</b></span>";
		} else {
			iconPath = ICON_16x16_PATH "datovka-ok.png";
			resStr = "<span style=\"color:#00aa00;\"><b>";
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
