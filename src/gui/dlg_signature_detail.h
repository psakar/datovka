#ifndef DLG_SIGNATURE_DETAIL_H
#define DLG_SIGNATURE_DETAIL_H

#include <QDialog>

#include "src/common.h"
#include "ui_dlg_signature_detail.h"

class dlg_signature_detail : public QDialog, public Ui::dlg_signature_detail {
    Q_OBJECT

public:
	dlg_signature_detail(QWidget *parent = 0);
};

#endif // DLG_SIGNATURE_DETAIL_H
