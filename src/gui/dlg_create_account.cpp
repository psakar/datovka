
#include <QMessageBox>

#include "dlg_create_account.h"
#include "src/io/dbs.h"
#include "src/models/accounts_model.h"

DlgCreateAccount::DlgCreateAccount(QTreeView &accountList, AccountDb &m_accountDb,
QModelIndex acntTopIdx, Action action,
    QWidget *parent)
    : QDialog(parent),
    m_accountList(accountList),
    m_accountDb(m_accountDb),
    m_acntTopIdx(acntTopIdx),
    m_action(action),
    m_loginmethod(0),
    m_certPath("")
{
	setupUi(this);
	initAccountDialog();
}


/* ========================================================================= */
/*
 * Init dialog
 */
void DlgCreateAccount::initAccountDialog(void)
/* ========================================================================= */
{
	this->loginmethodComboBox->addItem(tr("Password"));
	this->loginmethodComboBox->addItem(tr("Certificate"));
	this->loginmethodComboBox->addItem(tr("Certificate + Password"));
	this->loginmethodComboBox->addItem(tr("Password + Secure code"));
	this->loginmethodComboBox->addItem(tr("Password + Secure SMS"));
	this->certificateLabel->setEnabled(false);
	this->accountButtonBox->button(
	    QDialogButtonBox::Ok)->setEnabled(false);
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

	/* if account exists then we set all items */
	if (ACT_EDIT == m_action || ACT_PWD == m_action) {
		setCurrentAccountData();
	}
}


/* ========================================================================= */
/*
 * Set current account data from dsgui.conf (exist account edit)
 */
void DlgCreateAccount::setCurrentAccountData(void)
/* ========================================================================= */
{
	int itemindex;
	QModelIndex index;
	AccountModel *model = dynamic_cast<AccountModel *>(m_accountList.model());

	if (ACT_EDIT == m_action) {
		index = m_accountList.currentIndex();
	} else  {
		index = m_acntTopIdx;
	}

	const QStandardItem *item = model->itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);

	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (ACT_EDIT == m_action) {
		this->setWindowTitle(tr("Update account") + " " + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->usernameLineEdit->setEnabled(false);

	} else  {
		this->setWindowTitle(tr("Enter password for account") + " " + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->testAccountCheckBox->setEnabled(false);
		this->usernameLineEdit->setEnabled(false);
	}

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


/* ========================================================================= */
/*
 * Open load dialog and set certificate file path
 */
void DlgCreateAccount::addCertificateFromFile(void)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 *  Check content of mandatory items in dialog and activate save button
 */
void DlgCreateAccount::checkInputFields(void)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Set active/inactive buttons based on login method
 */
void DlgCreateAccount::setActiveButton(int itemindex)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 *  Create and save a new account into dsgui.conf
 */
void DlgCreateAccount::saveAccount(void)
/* ========================================================================= */
{
	AccountModel *model = dynamic_cast<AccountModel*>(
	    m_accountList.model());
	QModelIndex index = m_accountList.currentIndex();
	QStandardItem *item;
	QStandardItem *itemTop;
	AccountModel::SettingsMap itemSettings;

	if (ACT_EDIT == m_action) {
		/* Model must be present. */
		Q_ASSERT(index.isValid());
		item = model->itemFromIndex(index);
		Q_ASSERT(0 != item);
		itemTop = AccountModel::itemTop(item);
		Q_ASSERT(0 != itemTop);
		itemSettings = itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	} else if (ACT_PWD == m_action) {
		index = m_acntTopIdx;
		/* Model must be present. */
		Q_ASSERT(index.isValid());
		item = model->itemFromIndex(index);
		Q_ASSERT(0 != item);
		itemTop = AccountModel::itemTop(item);
		Q_ASSERT(0 != itemTop);
		itemSettings = itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
	}

	itemSettings[NAME] = this->accountLineEdit->text();
	itemSettings[USER] = this->usernameLineEdit->text();

	if (this->loginmethodComboBox->currentIndex() == USER_NAME) {
		itemSettings[LOGIN] = "username";
	} else if (this->loginmethodComboBox->currentIndex() == CERTIFICATE) {
		itemSettings[LOGIN] = "certificate";
		itemSettings[P12FILE] = QDir::fromNativeSeparators(m_certPath);
	} else if (this->loginmethodComboBox->currentIndex() ==
	           USER_CERTIFICATE) {
		itemSettings[LOGIN] = "user_certificate";
		itemSettings[P12FILE] = QDir::fromNativeSeparators(m_certPath);
	} else if (this->loginmethodComboBox->currentIndex() == HOTP) {
		itemSettings[LOGIN] = "hotp";
	} else {
		itemSettings[LOGIN] = "totp";
	}


	itemSettings[REMEMBER]= this->rememberPswcheckBox->isChecked();
	//if (this->rememberPswcheckBox->isChecked()) {
		itemSettings[PWD]= this->passwordLineEdit->text();
	//} else {
	//	itemSettings[PWD] = "";
	//}
	itemSettings[TEST]= this->testAccountCheckBox->isChecked();
	itemSettings[SYNC]= this->synchroCheckBox->isChecked();

	switch (m_action) {
	case ACT_EDIT:
	case ACT_PWD:
		itemTop->setText(this->accountLineEdit->text());
		itemTop->setData(itemSettings);
		/* TODO -- Save/update related account DB entry? */
		break;
	case ACT_ADDNEW:
		model->addAccount(this->accountLineEdit->text(), itemSettings);
		m_accountList.expandAll();
		/* TODO -- Save/update related account DB entry. */
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}
