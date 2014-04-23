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

void changePassword::generatePassword(void)
{
	QString pwd = getRandomString();
	this->newPwdLineEdit->setText(pwd);
	this->NewPwdLineEdit2->setText(pwd);
}

QString changePassword::getRandomString(void) const
{
	QString randomString;
	for(int i=0; i<randomStringLength; ++i) {
		int index = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at(index);
		randomString.append(nextChar);
	}
	return randomString;
}



void changePassword::checkInputFields(void)
{
	bool buttonEnabled = !this->newPwdLineEdit->text().isEmpty()
		    && !this->currentPwdLineEdit->text().isEmpty()
		    && !this->NewPwdLineEdit2->text().isEmpty();
	this->buttonBox->button(QDialogButtonBox::Ok)->
	    setEnabled(buttonEnabled);
}


void changePassword::showHidePasswordLine(void)
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

void changePassword::saveChange(void)
{

}

