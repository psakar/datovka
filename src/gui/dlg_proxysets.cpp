

#include "dlg_proxysets.h"
#include "ui_dlg_proxysets.h"


DlgProxysets::DlgProxysets(QWidget *parent)
    : QDialog(parent)
{
	setupUi(this);

	connect(this->proxyButtonBox, SIGNAL(accepted()),
	    this, SLOT(saveChanges(void)));

	connect(this->httpManualProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(setActiveHttpProxyEdit(bool)));
	connect(this->httpAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(toggleHttpProxyPassword(int)));

	connect(this->httpsManualProxyRadioButton, SIGNAL(toggled (bool)),
	    this, SLOT(setActiveHttpsProxyEdit(bool)));
	connect(this->httpsAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(toggleHttpsProxyPassword(int)));

	loadProxyDialog(globProxSet);
}

void DlgProxysets::loadProxyDialog(const GlobProxySettings &proxySettings)
{
	QStringList hostport;

	this->httpProxyAuth->setHidden(true);
	this->httpAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (proxySettings.http_proxy == "-1") {
		this->httpNoProxyRadioButton->setChecked(false);
		this->httpAutoProxyRadioButton->setChecked(true);
		this->httpManualProxyRadioButton->setChecked(false);
		this->httpHostnameLineEdit->setEnabled(false);
		this->httpPortLineEdit->setEnabled(false);
	} else if (proxySettings.http_proxy == "None") {
		this->httpNoProxyRadioButton->setChecked(true);
		this->httpAutoProxyRadioButton->setChecked(false);
		this->httpManualProxyRadioButton->setChecked(false);
		this->httpHostnameLineEdit->setEnabled(false);
		this->httpPortLineEdit->setEnabled(false);
	} else {
		this->httpNoProxyRadioButton->setChecked(false);
		this->httpAutoProxyRadioButton->setChecked(false);
		this->httpManualProxyRadioButton->setChecked(true);
		this->httpHostnameLineEdit->setEnabled(true);
		hostport = proxySettings.http_proxy.split(":");
		this->httpHostnameLineEdit->setText(hostport[0]);
		this->httpPortLineEdit->setText(hostport[1]);
	}

	this->httpsProxyAuth->setHidden(true);
	this->httpsAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	/* Currently ignore HTTPS proxy settings. */
	this->httpsNoProxyRadioButton->setChecked(true);
	this->httpsNoProxyRadioButton->setEnabled(false);
	this->httpsAutoProxyRadioButton->setChecked(false);
	this->httpsAutoProxyRadioButton->setEnabled(false);
	this->httpsManualProxyRadioButton->setChecked(false);
	this->httpsManualProxyRadioButton->setEnabled(false);
	this->httpsHostnameLineEdit->setEnabled(false);
	this->httpsPortLineEdit->setEnabled(false);
	this->httpsUnameEdit->setEnabled(false);
	this->httpsPwdEdit->setEnabled(false);

/*
	if (proxySettings.https_proxy == "-1") {
		this->httpsNoProxyRadioButton->setChecked(false);
		this->httpsAutoProxyRadioButton->setChecked(true);
		this->httpsManualProxyRadioButton->setChecked(false);
		this->httpsHostnameLineEdit->setEnabled(false);
		this->httpsPortLineEdit->setEnabled(false);
	} else if (proxySettings.https_proxy == "None") {
		this->httpsNoProxyRadioButton->setChecked(true);
		this->httpsAutoProxyRadioButton->setChecked(false);
		this->httpsManualProxyRadioButton->setChecked(false);
		this->httpsHostnameLineEdit->setEnabled(false);
		this->httpsPortLineEdit->setEnabled(false);
	} else {
		this->httpsNoProxyRadioButton->setChecked(false);
		this->httpsAutoProxyRadioButton->setChecked(false);
		this->httpsManualProxyRadioButton->setChecked(true);
		this->httpsHostnameLineEdit->setEnabled(true);
		hostport = proxySettings.https_proxy.split(":");
		this->httpsHostnameLineEdit->setText(hostport[0]);
		this->httpsPortLineEdit->setText(hostport[1]);
	}
*/
}


void DlgProxysets::toggleHttpProxyPassword(int state)
{
	this->httpProxyAuth->setHidden(Qt::Unchecked == state);
}


void DlgProxysets::toggleHttpsProxyPassword(int state)
{
	this->httpsProxyAuth->setHidden(Qt::Unchecked == state);
}


void DlgProxysets::saveChanges(void) const
{
	if (this->httpNoProxyRadioButton->isChecked()) {
		globProxSet.http_proxy = "None";
	} else if (this->httpAutoProxyRadioButton->isChecked()) {
		globProxSet.http_proxy = "-1";
	} else {
		globProxSet.http_proxy = this->httpHostnameLineEdit->text() +
		    ":" + this->httpPortLineEdit->text();
	}

	if (this->httpsNoProxyRadioButton->isChecked()) {
		globProxSet.https_proxy = "None";
	} else if (this->httpsAutoProxyRadioButton->isChecked()) {
		globProxSet.https_proxy = "-1";
	} else {
		globProxSet.https_proxy = this->httpsHostnameLineEdit->text() +
		    ":" + this->httpsPortLineEdit->text();
	}
}


void DlgProxysets::setActiveHttpProxyEdit(bool state)
{
	this->httpHostnameLineEdit->setEnabled(state);
	this->httpPortLineEdit->setEnabled(state);
}


void DlgProxysets::setActiveHttpsProxyEdit(bool state)
{
	this->httpsHostnameLineEdit->setEnabled(state);
	this->httpsPortLineEdit->setEnabled(state);
}
