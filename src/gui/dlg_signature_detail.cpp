

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

	validate_message_signature();
}


/* ========================================================================= */
/*
 * Check message signature, show result in dialog.
 */
void DlgSignatureDetail::validate_message_signature(void)
/* ========================================================================= */
{
	QString iconPath;
	QString descStr;

	if (!m_messageDb.msgsVerificationAttempted(m_dmId)) {
		iconPath = ICON_3PARTY_PATH "warning_16.png";
		descStr = "<b>";
		descStr += QObject::tr("Message signature is not present.");
		descStr += "</b>";
	} else if (!m_messageDb.msgsVerified(m_dmId)) {
		iconPath = ICON_16x16_PATH "datovka-error.png";
		descStr = "<span style=\"color:#aa0000;\">";
		descStr += QObject::tr("No");
		descStr += "</span>";
	} else {
		iconPath = ICON_16x16_PATH "datovka-ok.png";
		descStr = "<span style=\"color:#00aa00;\">";
		descStr += QObject::tr("Yes");
		descStr += "</span>";
	}

	this->mSignatureImage->setIcon(QIcon(iconPath));
	this->mSignatureStatus->setTextFormat(Qt::RichText);
	this->mSignatureStatus->setText(descStr);
}
