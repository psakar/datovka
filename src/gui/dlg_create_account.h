#ifndef DLG_CREATENEWACCOUNTDIALOG_H
#define DLG_CREATENEWACCOUNTDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QTreeView>
#include "src/common.h"
#include "ui_dlg_create_account.h"


class CreateNewAccountDialog : public QDialog, public Ui::CreateNewAccountDialog {
	Q_OBJECT

public:
	CreateNewAccountDialog(QWidget *parent = 0, QTreeView *accountList = 0, QString action = "Add");

private slots:

	void setActiveButton(int);
	QString addCertificateFromFile(void);
	void saveAccount(void);

private:
	void initAccountDialog(QTreeView *accountList, QString action);
	void setCurrentAccountData(QTreeView *accountList);

	QTreeView *m_accountList;
};

#endif // DLG_CREATENEWACCOUNTDIALOG_H
