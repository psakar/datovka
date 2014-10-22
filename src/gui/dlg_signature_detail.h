

#ifndef _DLG_SIGNATURE_DETAIL_H_
#define _DLG_SIGNATURE_DETAIL_H_


#include <QDialog>

#include "src/common.h"
#include "src/io/message_db.h"
#include "ui_dlg_signature_detail.h"


class DlgSignatureDetail : public QDialog, public Ui::SignatureDetail {
    Q_OBJECT

public:
	DlgSignatureDetail(const MessageDb &messageDb, int dmId,
	    QWidget *parent = 0);

private:
	const MessageDb &m_messageDb;
	const int m_dmId;

	/*!
	 * @brief Check message signature, show result in dialog.
	 */
	void validate_message_signature(void);
};


#endif /* _DLG_SIGNATURE_DETAIL_H_ */
