#include "src/models/accounts_model.h"
#include "dlg_change_pwd.h"

changePassword::changePassword(QWidget *parent, QTreeView *accountList, QString idBox) :
    QDialog(parent),
    m_accountList(accountList),
    m_idBox(idBox)
{
	setupUi(this);
	initPwdChangeDialog(m_idBox);
}

void changePassword::initPwdChangeDialog(QString idBox)
{
	this->accountLineEdit->setText(idBox);
}

