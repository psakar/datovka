#include "dlg_create_account.h"
#include "ui_dlg_create_account.h"

CreateNewAccountDialog::CreateNewAccountDialog(QWidget *parent) :
	QDialog(parent)
{
	setupUi(this);
	initAccountDialog();
}

void CreateNewAccountDialog::initAccountDialog(void)
{
	this->loginmethodComboBox->addItem(QString(tr("Password")));
	this->loginmethodComboBox->addItem(QString(tr("Certificate")));
	this->loginmethodComboBox->addItem(QString(tr("Certificate + Password")));
	this->loginmethodComboBox->addItem(QString(tr("Password + Secure code")));
	this->loginmethodComboBox->addItem(QString(tr("Password + Secure SMS")));
	this->certificateLabel->setEnabled(false);
	this->addCertificateButton->setEnabled(false);
	connect(this->loginmethodComboBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(setActiveButton(int)));
	connect(this->addCertificateButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFromFile()));
	connect(this->accountButtonBox, SIGNAL(accepted()), this,
	    SLOT(saveAccount(void)));
}

QString CreateNewAccountDialog::addCertificateFromFile()
{
	QString certFileName = QFileDialog::getOpenFileName(this,
	    tr("Open Certificate"), "", tr("Certificate File (*.p12)"));
	if (certFileName != NULL) {
		this->addCertificateButton->setText(certFileName);
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("key_16.png")));
	} else {
		this->addCertificateButton->setText(tr("Add"));
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("plus_16.png")));
	}

	return certFileName;
}

void CreateNewAccountDialog::setActiveButton(int itemindex)
{
	if (itemindex == 1) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->rememberPswcheckBox->setEnabled(false);
	} else if (itemindex == 2) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(true);
		this->passwordLineEdit->setEnabled(true);
		this->rememberPswcheckBox->setEnabled(true);
	} else {
		this->certificateLabel->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLabel->setEnabled((true));
		this->passwordLineEdit->setEnabled((true));
		this->rememberPswcheckBox->setEnabled(true);
	}
}

void CreateNewAccountDialog::saveAccount(void)
{
	qDebug() << "saveAccount";
}


