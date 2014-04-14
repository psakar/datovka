#ifndef DLG_CHANGE_PWD_H
#define DLG_CHANGE_PWD_H

#include <QDialog>
#include <QTreeView>
#include "src/common.h"
#include "ui_dlg_change_pwd.h"

class changePassword : public QDialog, public Ui::changePassword {
	Q_OBJECT

public:
	changePassword(QWidget *parent = 0, QTreeView *accountList = 0, QString boxId = "");

private:
	void initPwdChangeDialog(QString idBox);
	QTreeView *m_accountList;
	QString  m_idBox;
};

#endif // DLG_CHANGE_PWD_H
