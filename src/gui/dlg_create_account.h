#ifndef DLG_CREATENEWACCOUNTDIALOG_H
#define DLG_CREATENEWACCOUNTDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include "src/common.h"
#include "ui_dlg_create_account.h"


class CreateNewAccountDialog : public QDialog, public Ui::CreateNewAccountDialog
{
	Q_OBJECT

public:
	CreateNewAccountDialog(QWidget *parent = 0);

private slots:

	void setActiveButton(int);
	QString addCertificateFromFile(void);
	void saveAccount(void);

private:
	void initAccountDialog(void);
};

#endif // DLG_CREATENEWACCOUNTDIALOG_H
