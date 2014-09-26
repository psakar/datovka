
#include <QMessageBox>

#include "dlg_change_pwd.h"
#include "src/io/isds_sessions.h"
#include "src/models/accounts_model.h"


DlgChangePwd::DlgChangePwd(const QString &boxId, QTreeView &accountList,
    const AccountModel::SettingsMap &accountInfo, QWidget *parent)
    : QDialog(parent),
    m_accountList(accountList),
    m_accountInfo(accountInfo),
    m_boxId(boxId)
{
	setupUi(this);
	initPwdChangeDialog();
}


/* ========================================================================= */
/*
 * Init dialog
 */
void DlgChangePwd::initPwdChangeDialog(void)
/* ========================================================================= */
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
	    SLOT(changePassword(void)));

	if (m_accountInfo.loginMethod() == "hotp" ||
	    m_accountInfo.loginMethod() == "topt") {
		this->secCodeLineEdit->setEnabled(true);
		this->label_7->setEnabled(true);
	} else {
		this->secCodeLineEdit->setEnabled(false);
		this->label_7->setEnabled(false);
	}

	pingTimer = new QTimer(this);
	pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(pingTimer, SIGNAL(timeout()), this,
	    SLOT(pingIsdsServer()));
}


/* ========================================================================= */
/*
 * Ping isds server, test if connection on isds server is active
 */
void DlgChangePwd::pingIsdsServer(void)
/* ========================================================================= */

{
	if (isdsSessions.isConnectToIsds(m_accountInfo.userName())) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}

/* ========================================================================= */
/*
 * Fill the new password in the textlines
 */
void DlgChangePwd::generatePassword(void)
/* ========================================================================= */
{
	QString pwd = generateRandomString();
	this->newPwdLineEdit->setText(pwd);
	this->NewPwdLineEdit2->setText(pwd);
}


/* ========================================================================= */
/*
 * Generate a new password string from set of char
 */
QString DlgChangePwd::generateRandomString(void)
/* ========================================================================= */
{
	QString randomString;
	for(int i=0; i<randomStringLength; ++i) {
		int index = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at(index);
		randomString.append(nextChar);
	}
	return randomString;
}


/* ========================================================================= */
/*
 * Check input textline, passwor length and activated OK button
 */
void DlgChangePwd::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled = !this->currentPwdLineEdit->text().isEmpty() &&
	    !this->newPwdLineEdit->text().isEmpty() &&
	    this->newPwdLineEdit->text().length() > 7 &&
	    !this->NewPwdLineEdit2->text().isEmpty() &&
	    this->NewPwdLineEdit2->text().length() > 7 &&
	    this->NewPwdLineEdit2->text() == this->newPwdLineEdit->text();
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    buttonEnabled);
}

/* ========================================================================= */
/*
 * Show/hide text represantion of password in the textlines
 */
void DlgChangePwd::showHidePasswordLine(void)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Sent new password request into ISDS
 */
void DlgChangePwd::changePassword(void)
/* ========================================================================= */
{
	isds_error status;
	char * refnumber = NULL;

	if (m_accountInfo.loginMethod() == "hotp" ||
	    m_accountInfo.loginMethod() == "topt") {

		struct isds_otp *otp = NULL;
		otp = (struct isds_otp *) malloc(sizeof(struct isds_otp));
		memset(otp, 0, sizeof(struct isds_otp));

		if (m_accountInfo.loginMethod() == "hotp") {
			otp->method = OTP_HMAC;
		} else {
			otp->method = OTP_TIME;
		}

		otp->otp_code = !this->secCodeLineEdit->text().isEmpty() ?
		    strdup(this->secCodeLineEdit->text().toStdString().c_str())
		    : NULL;

		status = isds_change_password(
		    isdsSessions.session(m_accountInfo.userName()),
		    this->currentPwdLineEdit->text().toStdString().c_str(),
		    this->newPwdLineEdit->text().toStdString().c_str(),
		    otp, &refnumber);

		free(otp->otp_code);
		free(otp);
	} else {
		status = isds_change_password(
		    isdsSessions.session(m_accountInfo.userName()),
		    this->currentPwdLineEdit->text().toStdString().c_str(),
		    this->newPwdLineEdit->text().toStdString().c_str(),
		    NULL, &refnumber);
	}

	QString result(refnumber);

	if (status == IE_SUCCESS) {
		QMessageBox::information(this, tr("Password has been changed"),
		    tr("Password has been changed successfully...") + "\n" +
		    tr("Reference number: ") + result,
		    QMessageBox::Ok);

		AccountModel *model = dynamic_cast<AccountModel*>(
		    m_accountList.model());
		QModelIndex index = m_accountList.currentIndex();
		QStandardItem *item = model->itemFromIndex(index);
		QStandardItem *itemTop = AccountModel::itemTop(item);
		AccountModel::SettingsMap itemSettings =
		    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		itemSettings[PWD] = this->newPwdLineEdit->text();
	} else {
		QMessageBox::warning(this, tr("Password error"),
		    tr("An error occurred while password was changed")
		    + "\n" + tr("ErrorType: ") + isds_strerror(status),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Set of possilbe char to generation fo passsword
 */
const QString DlgChangePwd::possibleCharacters(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
const int DlgChangePwd::randomStringLength = 10;
/* ========================================================================= */
