
#include <QMessageBox>

#include "dlg_create_account.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"


DlgCreateAccount::DlgCreateAccount(QTreeView &accountList,
    AccountDb &m_accountDb, QModelIndex acntTopIdx, Action action,
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


/* Login method descriptors. */
#define LIM_USERNAME "username"
#define LIM_CERT "certificate"
#define LIM_USER_CERT "user_certificate"
#define LIM_HOTP "hotp"
#define LIM_TOTP "totp"


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
	if (ACT_ADDNEW != m_action) {
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

	switch (m_action) {
	case ACT_EDIT:
		this->setWindowTitle(tr("Update account") + " "
		    + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->usernameLineEdit->setEnabled(false);
		break;
	case ACT_PWD:
		this->setWindowTitle(tr("Enter password for account") + " "
		    + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->testAccountCheckBox->setEnabled(false);
		this->usernameLineEdit->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		break;
	case ACT_CERT:
		this->setWindowTitle(tr("Set certificate for account") + " "
		    + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->testAccountCheckBox->setEnabled(false);
		this->usernameLineEdit->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		break;
	case ACT_CERTPWD:
		this->setWindowTitle(tr("Enter password/certificate for account")
		    + " " + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->testAccountCheckBox->setEnabled(false);
		this->usernameLineEdit->setEnabled(false);
		break;
	case ACT_IDBOX:
		this->setWindowTitle(tr("Enter ID of your databox for account")
		    + " " + itemTop->text());
		this->accountLineEdit->setText(itemTop->text());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(itemSettings[USER].toString());
		this->testAccountCheckBox->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->usernameLabel->setText(tr("Databox ID:"));
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	const QString login_method = itemSettings[LOGIN].toString();
	if (LIM_USERNAME == login_method) {
		itemindex = USER_NAME;
	} else if (LIM_CERT == login_method) {
		itemindex = CERTIFICATE;
	} else if (LIM_USER_CERT == login_method) {
		itemindex = USER_CERTIFICATE;
	} else if (LIM_HOTP == login_method) {
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
		this->usernameLineEdit->setEnabled(false);
	} else if (itemindex == USER_CERTIFICATE) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(true);
		this->passwordLineEdit->setEnabled(true);
		this->rememberPswcheckBox->setEnabled(true);
		this->usernameLineEdit->setEnabled(true);
	} else {
		this->certificateLabel->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLabel->setEnabled((true));
		this->passwordLineEdit->setEnabled((true));
		this->rememberPswcheckBox->setEnabled(true);
		this->usernameLineEdit->setEnabled(false);
	}
	m_loginmethod = itemindex;
	checkInputFields();
}


/* ========================================================================= */
/*
 *  Create new or save account into dsgui.conf
 */
void DlgCreateAccount::saveAccount(void)
/* ========================================================================= */
{
	debug_func_call();

	/* get current account model */
	AccountModel *model = dynamic_cast<AccountModel*>(
	    m_accountList.model());
	QModelIndex index;
	QStandardItem *itemTop;
	AccountModel::SettingsMap itemSettings;

	/* set account index, itemTop and map itemSettings for account */
	switch (m_action) {
	case ACT_ADDNEW:
		itemTop = NULL;
		break;
	case ACT_EDIT:
		index = m_accountList.currentIndex();
		Q_ASSERT(index.isValid());
		itemTop = AccountModel::itemTop(model->itemFromIndex(index));
		Q_ASSERT(0 != itemTop);
		itemSettings = itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		break;
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
	case ACT_IDBOX:
		index = m_acntTopIdx;
		Q_ASSERT(index.isValid());
		itemTop = AccountModel::itemTop(model->itemFromIndex(index));
		Q_ASSERT(0 != itemTop);
		itemSettings = itemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();
		break;
	default:
		itemTop = NULL;
		Q_ASSERT(0);
		break;
	}

	/* set account items */
	itemSettings[NAME] = this->accountLineEdit->text();
	itemSettings[USER] = this->usernameLineEdit->text();
	itemSettings[REMEMBER] = this->rememberPswcheckBox->isChecked();
	itemSettings[PWD] = this->passwordLineEdit->text();
	itemSettings[TEST] = this->testAccountCheckBox->isChecked();
	itemSettings[SYNC] = this->synchroCheckBox->isChecked();

	if (this->loginmethodComboBox->currentIndex() == USER_NAME) {
		itemSettings[LOGIN] = LIM_USERNAME;
		itemSettings[P12FILE] = "";
	} else if (this->loginmethodComboBox->currentIndex() == CERTIFICATE) {
		itemSettings[LOGIN] = LIM_CERT;
		itemSettings[PWD] = "";
		itemSettings[P12FILE] = QDir::fromNativeSeparators(m_certPath);
	} else if (this->loginmethodComboBox->currentIndex() == USER_CERTIFICATE) {
		itemSettings[LOGIN] = LIM_USER_CERT;
		itemSettings[P12FILE] = QDir::fromNativeSeparators(m_certPath);
	} else if (this->loginmethodComboBox->currentIndex() == HOTP) {
		itemSettings[LOGIN] = LIM_HOTP;
		itemSettings[P12FILE] = "";
	} else if (this->loginmethodComboBox->currentIndex() == TOTP) {
		itemSettings[LOGIN] = LIM_TOTP;
		itemSettings[P12FILE] = "";
	} else {
		Q_ASSERT(0);
	}

	/* create new account / save current account */
	switch (m_action) {
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
	case ACT_IDBOX:
		itemTop->setText(this->accountLineEdit->text());
		itemTop->setData(itemSettings);
		/* TODO -- Save/update related account DB entry? */
		break;
	case ACT_ADDNEW:
		index = model->addAccount(this->accountLineEdit->text(),
		    itemSettings);
		/* Change selection. */
		qDebug() << "Changing selection" << index;
		m_accountList.selectionModel()->setCurrentIndex(index,
		    QItemSelectionModel::ClearAndSelect);
		/* Expand the tree. */
		m_accountList.expand(index);
		emit getAccountUserDataboxInfo(index, true);
		/* TODO -- Save/update related account DB entry. */
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}
