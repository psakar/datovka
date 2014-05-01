

#include "src/models/accounts_model.h"
#include "dlg_change_pwd.h"


DlgChangePwd::DlgChangePwd(const QString &boxId, QTreeView &accountList,
    QWidget *parent)
    : QDialog(parent),
    m_accountList(accountList),
    m_boxId(boxId)
{
	setupUi(this);
	initPwdChangeDialog();
}

void DlgChangePwd::initPwdChangeDialog(void)
{
	this->accountLineEdit->setText(m_boxId);
	connect(this->generateButton, SIGNAL(clicked()), this,
	    SLOT(generatePassword()));
	connect(this->showHideButton, SIGNAL(clicked()), this,
	    SLOT(showHidePasswordLine()));
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(this->newPwdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->currentPwdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->NewPwdLineEdit2, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(saveChange(void)));
}

void DlgChangePwd::generatePassword(void)
{
	QString pwd = generateRandomString();
	this->newPwdLineEdit->setText(pwd);
	this->NewPwdLineEdit2->setText(pwd);
}

QString DlgChangePwd::generateRandomString(void)
{
	QString randomString;
	for(int i=0; i<randomStringLength; ++i) {
		int index = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at(index);
		randomString.append(nextChar);
	}
	return randomString;
}



void DlgChangePwd::checkInputFields(void)
{
	bool buttonEnabled = !this->newPwdLineEdit->text().isEmpty() &&
	    !this->currentPwdLineEdit->text().isEmpty() &&
	    !this->NewPwdLineEdit2->text().isEmpty();
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    buttonEnabled);
}


void DlgChangePwd::showHidePasswordLine(void)
{
	if (this->currentPwdLineEdit->echoMode() == QLineEdit::Password) {
		this->currentPwdLineEdit->setEchoMode(QLineEdit::Normal);
		this->newPwdLineEdit->setEchoMode(QLineEdit::Normal);
		this->NewPwdLineEdit2->setEchoMode(QLineEdit::Normal);
		this->showHideButton->setText(tr("Hide"));
	} else {
		this->currentPwdLineEdit->setEchoMode(QLineEdit::Password);
		this->newPwdLineEdit->setEchoMode(QLineEdit::Password);
		this->NewPwdLineEdit2->setEchoMode(QLineEdit::Password);
		this->showHideButton->setText(tr("Show"));
	}
}

void DlgChangePwd::saveChange(void)
{
	/* TODO */
}


const QString DlgChangePwd::possibleCharacters(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
const int DlgChangePwd::randomStringLength = 10;
