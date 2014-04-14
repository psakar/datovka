#include "src/models/accounts_model.h"
#include "dlg_create_account.h"

CreateNewAccountDialog::CreateNewAccountDialog(QWidget *parent,
    QTreeView *accountList, QString action)
    : QDialog(parent),
    m_accountList(accountList),
    m_action(action),
    m_loginmethod(0),
    m_certPath("")
{
	setupUi(this);
	initAccountDialog(accountList, action);
}

void CreateNewAccountDialog::initAccountDialog(QTreeView *accountList, QString action)
{
	this->loginmethodComboBox->addItem(QString(tr("Password")));
	this->loginmethodComboBox->addItem(QString(tr("Certificate")));
	this->loginmethodComboBox->addItem(QString(tr("Certificate + Password")));
	this->loginmethodComboBox->addItem(QString(tr("Password + Secure code")));
	this->loginmethodComboBox->addItem(QString(tr("Password + Secure SMS")));
	this->certificateLabel->setEnabled(false);
	this->accountButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->addCertificateButton->setEnabled(false);
	connect(this->loginmethodComboBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(setActiveButton(int)));
	connect(this->addCertificateButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFromFile()));
	connect(this->accountButtonBox, SIGNAL(accepted()), this,
	    SLOT(saveAccount(void)));
	connect(this->accountLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->usernameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->passwordLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	if (action == "Edit") {
		setCurrentAccountData(accountList);
	}
}

void CreateNewAccountDialog::setCurrentAccountData(QTreeView *accountList)
{
	int itemindex;

	AccountModel *model = dynamic_cast<AccountModel*>(accountList->model());
	QModelIndex index = accountList->currentIndex();
	const QStandardItem *item = model->itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);
	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_CONF_SETINGS).toMap();

	this->setWindowTitle(tr("Update account") + " " + itemTop->text());
	this->accountLineEdit->setText(itemTop->text());
	this->usernameLineEdit->setText(itemSettings[USER].toString());
	this->usernameLineEdit->setEnabled(false);

	QString login_method = itemSettings[LOGIN].toString();
	if (login_method == "username") {
		itemindex = USER_NAME;
	} else if(login_method == "certificate") {
		itemindex = CERTIFICATE;
	} else if(login_method == "user_certificate") {
		itemindex = USER_CERTIFICATE;
	} else if(login_method == "hotp") {
		itemindex = HOTP;
	} else {
		itemindex = TOTP;
	}
	this->loginmethodComboBox->setCurrentIndex(itemindex);
	setActiveButton(itemindex);

	this->passwordLineEdit->setText(itemSettings[PWD].toString());
	this->testAccountCheckBox->setChecked(itemSettings[TEST].toBool());
	this->rememberPswcheckBox->setChecked(itemSettings[REMEMBER].toBool());
	this->synchroCheckBox->setChecked(itemSettings[SYNC].toBool());

	if (itemSettings[P12FILE].toString() != NULL) {
		this->addCertificateButton->setText(QDir::
		    toNativeSeparators(itemSettings[P12FILE].toString()));
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("key_16.png")));
		m_certPath = QDir::toNativeSeparators(itemSettings[P12FILE].
		   toString());
	}
}


void CreateNewAccountDialog::addCertificateFromFile()
{
	QString certFileName = QFileDialog::getOpenFileName(this,
	    tr("Open Certificate"), "", tr("Certificate File (*.p12)"));
	if (certFileName != NULL) {
		this->addCertificateButton->setText(certFileName);
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("key_16.png")));
		m_certPath = certFileName;
		checkInputFields();
	} else {
		this->addCertificateButton->setText(tr("Add"));
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("plus_16.png")));
		m_certPath = "";
		checkInputFields();
	}
}

void CreateNewAccountDialog::checkInputFields()
{
	bool buttonEnabled;
	if (m_loginmethod == CERTIFICATE) {
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
	} else if (m_loginmethod == USER_CERTIFICATE) {
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
	} else {
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty();
	}
	this->accountButtonBox->button(QDialogButtonBox::Ok)->
	    setEnabled(buttonEnabled);
}

void CreateNewAccountDialog::setActiveButton(int itemindex)
{
	if (itemindex == CERTIFICATE) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->rememberPswcheckBox->setEnabled(false);
	} else if (itemindex == USER_CERTIFICATE) {
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
	m_loginmethod = itemindex;
	checkInputFields();
}

void CreateNewAccountDialog::saveAccount(void)
{
	AccountModel *model = dynamic_cast<AccountModel*>
	    (m_accountList->model());
	QModelIndex index = m_accountList->currentIndex();
	QStandardItem *item = model->itemFromIndex(index);
	QStandardItem *itemTop = AccountModel::itemTop(item);
	AccountModel::SettingsMap itemSettings =
	    itemTop->data(ROLE_CONF_SETINGS).toMap();

	itemSettings[NAME]= this->accountLineEdit->text();
	itemSettings[USER]= this->usernameLineEdit->text();

	if (this->loginmethodComboBox->currentIndex() == USER_NAME) {
		itemSettings[LOGIN] = "username";
	} else if(this->loginmethodComboBox->currentIndex() == CERTIFICATE) {
		itemSettings[LOGIN] = "certificate";
		itemSettings[P12FILE] = QDir::fromNativeSeparators(m_certPath);
	} else if(this->loginmethodComboBox->currentIndex() == USER_CERTIFICATE) {
		itemSettings[LOGIN] = "user_certificate";
		itemSettings[P12FILE] = QDir::fromNativeSeparators(m_certPath);
	} else if(this->loginmethodComboBox->currentIndex() == HOTP) {
		itemSettings[LOGIN] = "hotp";
	} else {
		itemSettings[LOGIN] = "totp";
	}

	itemSettings[PWD]= this->passwordLineEdit->text();
	itemSettings[TEST]= this->testAccountCheckBox->isChecked();
	itemSettings[REMEMBER]= this->rememberPswcheckBox->isChecked();
	itemSettings[SYNC]= this->synchroCheckBox->isChecked();

	if (m_action == "Edit") {
		model->itemFromIndex(index)->
		    setText(this->accountLineEdit->text());
		itemTop->setData(itemSettings);
	} else {
		model->addAccount(this->accountLineEdit->text(), itemSettings);
		m_accountList->expandAll();
	}
}
