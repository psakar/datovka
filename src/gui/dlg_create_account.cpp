
#include "dlg_create_account.h"
#include "src/models/accounts_model.h"

DlgCreateAccount::DlgCreateAccount(QTreeView &accountList, AccountDb &m_accountDb,
Action action,
    QWidget *parent)
    : QDialog(parent),
    m_accountList(accountList),
    m_accountDb(m_accountDb),
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
	if (ACT_EDIT == m_action) {
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

	AccountModel *model = dynamic_cast<AccountModel *>(
	    m_accountList.model());
	QModelIndex index = m_accountList.currentIndex();
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
		itemSettings = itemTop->data(ROLE_CONF_SETINGS).toMap();
	} else {
		isds_error status;
		struct isds_ctx *isds_session = NULL;
		this->usernameLineEdit->text();

		isds_session = isds_ctx_create();
		if (NULL == isds_session) {
			qDebug() << "Error creating ISDS session.";
			isds_ctx_free(&isds_session);
			return;
		}

		status = isds_set_timeout(isds_session, TIMEOUT_MS);
		if (IE_SUCCESS != status) {
			qDebug() << "Error setting time-out.";
			isds_ctx_free(&isds_session);
			return;
		}

		status = isdsLoginUserName(isds_session,
		    this->usernameLineEdit->text(),
		    this->passwordLineEdit->text(),
		    this->testAccountCheckBox->isChecked());
		if (IE_SUCCESS != status) {
			qDebug() << "Error isdsLoginUserName.";
			qDebug() << status << isds_strerror(status);
			return;
		}

		struct isds_DbOwnerInfo *db_owner_info = NULL;
		status = isds_GetOwnerInfoFromLogin(isds_session,
		    &db_owner_info);

		if (IE_SUCCESS != status) {
			qDebug() << "Error isds_GetOwnerInfoFromLogin.";
			qDebug() << status << isds_strerror(status);
			return;
		}

		QString username = this->usernameLineEdit->text() + "___True";

		QString bithDAte = "";
		if ((NULL != db_owner_info->birthInfo) &&
		    (NULL != db_owner_info->birthInfo->biDate)) {
			struct tm *birthDate = db_owner_info->birthInfo->biDate;
			bithDAte = QString::number(birthDate->tm_year) + "-" +
			QString::number(birthDate->tm_mon) + "-" +
			QString::number(birthDate->tm_mday);
		}

		m_accountDb.insertAccountIntoDb(
		    username,
		    db_owner_info->dbID,
		    convertDbTypeToString(*db_owner_info->dbType),
		    atoi(db_owner_info->ic),
		    db_owner_info->personName ?
		        db_owner_info->personName->pnFirstName : NULL,
		    db_owner_info->personName ?
		        db_owner_info->personName->pnMiddleName : NULL,
		    db_owner_info->personName ?
		        db_owner_info->personName->pnLastName : NULL,
		    db_owner_info->personName ?
		        db_owner_info->personName->pnLastNameAtBirth : NULL,
		    db_owner_info->firmName,
		    bithDAte,
		    db_owner_info->birthInfo ?
		        db_owner_info->birthInfo->biCity : NULL,
		    db_owner_info->birthInfo ?
		        db_owner_info->birthInfo->biCounty : NULL,
		    db_owner_info->birthInfo ?
		        db_owner_info->birthInfo->biState : NULL,
		    db_owner_info->address ?
		        db_owner_info->address->adCity : NULL,
		    db_owner_info->address ?
		        db_owner_info->address->adStreet : NULL,
		    db_owner_info->address ?
		        db_owner_info->address->adNumberInStreet : NULL,
		    db_owner_info->address ?
		        db_owner_info->address->adNumberInMunicipality : NULL,
		    db_owner_info->address ?
		        db_owner_info->address->adZipCode : NULL,
		    db_owner_info->address ?
		        db_owner_info->address->adState : NULL,
		    db_owner_info->nationality,
		    db_owner_info->identifier,
		    db_owner_info->registryCode,
		    (int)*db_owner_info->dbState,
		    (int)*db_owner_info->dbEffectiveOVM,
		    (int)*db_owner_info->dbOpenAddressing);

		status = isds_logout(isds_session);
		if (IE_SUCCESS != status) {
			qDebug() << "Error in ISDS logout procedure.";
			qDebug() << status << isds_strerror(status);
		}

		status = isds_ctx_free(&isds_session);
		if (IE_SUCCESS != status) {
			qDebug() << "Error freeing ISDS session.";
			qDebug() << status << isds_strerror(status);
		}

		status = isds_cleanup();
		if (IE_SUCCESS != status) {
			qDebug() << "Unsuccessful ISDS clean-up.";
			qDebug() << status << isds_strerror(status);
		}
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

	itemSettings[PWD]= this->passwordLineEdit->text();
	itemSettings[TEST]= this->testAccountCheckBox->isChecked();
	itemSettings[REMEMBER]= this->rememberPswcheckBox->isChecked();
	itemSettings[SYNC]= this->synchroCheckBox->isChecked();

	switch (m_action) {
	case ACT_EDIT:
		model->itemFromIndex(index)->
		    setText(this->accountLineEdit->text());
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

// js2t8p
// Heslo3.14
