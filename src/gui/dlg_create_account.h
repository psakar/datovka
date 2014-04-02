#ifndef DLG_CREATENEWACCOUNTDIALOG_H
#define DLG_CREATENEWACCOUNTDIALOG_H

#include <QDialog>

#include "ui_dlg_create_account.h"

class CreateNewAccountDialog : public QDialog, public Ui::CreateNewAccountDialog {

	Q_OBJECT

public:
	CreateNewAccountDialog(QWidget *parent = 0);

private:

};

#endif // DLG_CREATENEWACCOUNTDIALOG_H
