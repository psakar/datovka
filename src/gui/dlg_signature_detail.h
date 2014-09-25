

#ifndef _DLG_SIGNATURE_DETAIL_H_
#define _DLG_SIGNATURE_DETAIL_H_


#include <QDialog>

#include "src/common.h"
#include "ui_dlg_signature_detail.h"


class DlgSignatureDetail : public QDialog, public Ui::SignatureDetail {
    Q_OBJECT

public:
	DlgSignatureDetail(QWidget *parent = 0);
};


#endif /* _DLG_SIGNATURE_DETAIL_H_ */
