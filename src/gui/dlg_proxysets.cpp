

#include "dlg_proxysets.h"
#include "ui_dlg_proxysets.h"


DlgProxysets::DlgProxysets(QWidget *parent)
    : QDialog(parent)
{
	setupUi(this);

	connect(this->proxyButtonBox, SIGNAL(accepted()),
	    this, SLOT(saveChanges(void)));

	connect(this->httpNoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpProxyPassword(bool)));
	connect(this->httpAutoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpProxyDetection(bool)));
	connect(this->httpManualProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(setActiveHttpProxyEdit(bool)));
	connect(this->httpAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(showHttpProxyPassword(int)));

	{
		ProxiesSettings::ProxySettings proxy =
		    ProxiesSettings::detectHttpProxy();
		if (!proxy.hostName.isEmpty() &&
		    (ProxiesSettings::noProxyStr != proxy.hostName)) {
			this->httpProxyDetectionLabel->setText(
			    tr("Proxy has been detected") + ": " +
			    proxy.hostName + ":" +
			    QString::number(proxy.port, 10));
		} else {
			this->httpProxyDetectionLabel->setText(
			    tr("No proxy detected, direct connection "
			        "will be used."));
		}
	}

	connect(this->httpsNoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpsProxyPassword(bool)));
	connect(this->httpsAutoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpsProxyDetection(bool)));
	connect(this->httpsManualProxyRadioButton, SIGNAL(toggled (bool)),
	    this, SLOT(setActiveHttpsProxyEdit(bool)));
	connect(this->httpsAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(showHttpsProxyPassword(int)));

	this->httpsProxyDetectionLabel->setText(
	    tr("No proxy detected, direct connection will be used."));

	loadProxyDialog(globProxSet);
}

void DlgProxysets::loadProxyDialog(const ProxiesSettings &proxySettings)
{
	this->httpProxyAuth->setHidden(true);
	this->httpAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::autoProxyStr == proxySettings.http.hostName) {
		this->httpNoProxyRadioButton->setChecked(false);
		this->httpAutoProxyRadioButton->setChecked(true);
		this->httpProxyDetectionLabel->setEnabled(true);
		this->httpManualProxyRadioButton->setChecked(false);
		this->httpLabel_1->setEnabled(false);
		this->httpLabel_2->setEnabled(false);
		this->httpHostnameLineEdit->setEnabled(false);
		this->httpPortLineEdit->setEnabled(false);

		this->httpUnameLabel->setEnabled(true);
		this->httpPwdLabel->setEnabled(true);
		this->httpUnameEdit->setEnabled(true);
		this->httpPwdEdit->setEnabled(true);
	} else if (ProxiesSettings::noProxyStr ==
	           proxySettings.http.hostName) {
		this->httpNoProxyRadioButton->setChecked(true);
		this->httpAutoProxyRadioButton->setChecked(false);
		this->httpProxyDetectionLabel->setEnabled(false);
		this->httpManualProxyRadioButton->setChecked(false);
		this->httpLabel_1->setEnabled(false);
		this->httpLabel_2->setEnabled(false);
		this->httpHostnameLineEdit->setEnabled(false);
		this->httpPortLineEdit->setEnabled(false);

		this->httpUnameLabel->setEnabled(false);
		this->httpPwdLabel->setEnabled(false);
		this->httpUnameEdit->setEnabled(false);
		this->httpPwdEdit->setEnabled(false);
	} else {
		this->httpNoProxyRadioButton->setChecked(false);
		this->httpAutoProxyRadioButton->setChecked(false);
		this->httpProxyDetectionLabel->setEnabled(false);
		this->httpManualProxyRadioButton->setChecked(true);
		this->httpLabel_1->setEnabled(true);
		this->httpLabel_2->setEnabled(true);
		this->httpHostnameLineEdit->setEnabled(true);
		this->httpHostnameLineEdit->setText(
		    proxySettings.http.hostName);
		this->httpPortLineEdit->setText(
		    (proxySettings.http.port >= 0) ?
		        QString::number(proxySettings.http.port, 10) :
		        QString());

		this->httpUnameLabel->setEnabled(true);
		this->httpPwdLabel->setEnabled(true);
		this->httpUnameEdit->setEnabled(true);
		this->httpPwdEdit->setEnabled(true);
	}
	this->httpUnameEdit->setText(globProxSet.http.userName);
	this->httpPwdEdit->setText(globProxSet.http.password);

	this->httpsProxyAuth->setHidden(true);
	this->httpsAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::autoProxyStr == proxySettings.https.hostName) {
		this->httpsNoProxyRadioButton->setChecked(false);
		this->httpsAutoProxyRadioButton->setChecked(true);
		this->httpsProxyDetectionLabel->setEnabled(true);
		this->httpsManualProxyRadioButton->setChecked(false);
		this->httpsLabel_1->setEnabled(false);
		this->httpsLabel_2->setEnabled(false);
		this->httpsHostnameLineEdit->setEnabled(false);
		this->httpsPortLineEdit->setEnabled(false);

		this->httpsUnameLabel->setEnabled(true);
		this->httpsPwdLabel->setEnabled(true);
		this->httpsUnameEdit->setEnabled(true);
		this->httpsPwdEdit->setEnabled(true);
	} else if (ProxiesSettings::noProxyStr ==
	           proxySettings.https.hostName) {
		this->httpsNoProxyRadioButton->setChecked(true);
		this->httpsAutoProxyRadioButton->setChecked(false);
		this->httpsProxyDetectionLabel->setEnabled(false);
		this->httpsManualProxyRadioButton->setChecked(false);
		this->httpsLabel_1->setEnabled(false);
		this->httpsLabel_2->setEnabled(false);
		this->httpsHostnameLineEdit->setEnabled(false);
		this->httpsPortLineEdit->setEnabled(false);

		this->httpsUnameLabel->setEnabled(false);
		this->httpsPwdLabel->setEnabled(false);
		this->httpsUnameEdit->setEnabled(false);
		this->httpsPwdEdit->setEnabled(false);
	} else {
		this->httpsNoProxyRadioButton->setChecked(false);
		this->httpsAutoProxyRadioButton->setChecked(false);
		this->httpsProxyDetectionLabel->setEnabled(false);
		this->httpsManualProxyRadioButton->setChecked(true);
		this->httpsLabel_1->setEnabled(true);
		this->httpsLabel_2->setEnabled(true);
		this->httpsHostnameLineEdit->setEnabled(true);
		this->httpsHostnameLineEdit->setText(
		    proxySettings.https.hostName);
		this->httpsPortLineEdit->setText(
		    (proxySettings.https.port >= 0) ?
		        QString::number(proxySettings.https.port, 10) :
		        QString());

		this->httpsUnameLabel->setEnabled(true);
		this->httpsPwdLabel->setEnabled(true);
		this->httpsUnameEdit->setEnabled(true);
		this->httpsPwdEdit->setEnabled(true);
	}
	this->httpsUnameEdit->setText(globProxSet.https.userName);
	this->httpsPwdEdit->setText(globProxSet.https.password);

#if 1
	/* Currently ignore HTTPS proxy settings. */
	this->httpsNoProxyRadioButton->setChecked(true);
	this->httpsNoProxyRadioButton->setEnabled(false);
	this->httpsAutoProxyRadioButton->setChecked(false);
	this->httpsAutoProxyRadioButton->setEnabled(false);
	this->httpsProxyDetectionLabel->setEnabled(false);
	this->httpsManualProxyRadioButton->setChecked(false);
	this->httpsManualProxyRadioButton->setEnabled(false);
	this->httpsLabel_1->setEnabled(false);
	this->httpsLabel_2->setEnabled(false);
	this->httpsHostnameLineEdit->setEnabled(false);
	this->httpsPortLineEdit->setEnabled(false);
	this->httpsUnameLabel->setEnabled(false);
	this->httpsPwdLabel->setEnabled(false);
	this->httpsUnameEdit->setEnabled(false);
	this->httpsPwdEdit->setEnabled(false);
#endif
}


void DlgProxysets::showHttpProxyPassword(int state)
{
	this->httpProxyAuth->setHidden(Qt::Unchecked == state);
}


void DlgProxysets::showHttpsProxyPassword(int state)
{
	this->httpsProxyAuth->setHidden(Qt::Unchecked == state);
}


void DlgProxysets::toggleHttpProxyDetection(bool state)
{
	this->httpProxyDetectionLabel->setEnabled(state);
}


void DlgProxysets::toggleHttpsProxyDetection(bool state)
{
	this->httpsProxyDetectionLabel->setEnabled(state);
}


void DlgProxysets::toggleHttpProxyPassword(bool state)
{
	this->httpUnameLabel->setEnabled(!state);
	this->httpPwdLabel->setEnabled(!state);
	this->httpUnameEdit->setEnabled(!state);
	this->httpPwdEdit->setEnabled(!state);
}


void DlgProxysets::toggleHttpsProxyPassword(bool state)
{
	this->httpsUnameLabel->setEnabled(!state);
	this->httpsPwdLabel->setEnabled(!state);
	this->httpsUnameEdit->setEnabled(!state);
	this->httpsPwdEdit->setEnabled(!state);
}


void DlgProxysets::saveChanges(void) const
{
	bool ok;

	/* TODO -- Checks and notification about incorrect values. */

	if (this->httpNoProxyRadioButton->isChecked()) {
		globProxSet.http.hostName = ProxiesSettings::noProxyStr;
		globProxSet.http.port = -1;
	} else if (this->httpAutoProxyRadioButton->isChecked()) {
		globProxSet.http.hostName = ProxiesSettings::autoProxyStr;
		globProxSet.http.port = -1;
	} else {
		globProxSet.http.hostName = this->httpHostnameLineEdit->text();
		globProxSet.http.port =
		    this->httpPortLineEdit->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.http.port = -1;
		}
	}
	globProxSet.http.userName = this->httpUnameEdit->text();
	globProxSet.http.password = this->httpPwdEdit->text();

	if (this->httpsNoProxyRadioButton->isChecked()) {
		globProxSet.https.hostName = ProxiesSettings::noProxyStr;
		globProxSet.https.port = -1;
	} else if (this->httpsAutoProxyRadioButton->isChecked()) {
		globProxSet.https.hostName = ProxiesSettings::autoProxyStr;
		globProxSet.https.port = -1;
	} else {
		globProxSet.https.hostName =
		    this->httpsHostnameLineEdit->text();
		globProxSet.https.port =
		    this->httpsPortLineEdit->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.https.port = -1;
		}
	}
	globProxSet.https.userName = this->httpsUnameEdit->text();
	globProxSet.https.password = this->httpsPwdEdit->text();
}


void DlgProxysets::setActiveHttpProxyEdit(bool state)
{
	this->httpLabel_1->setEnabled(state);
	this->httpLabel_2->setEnabled(state);
	this->httpHostnameLineEdit->setEnabled(state);
	this->httpPortLineEdit->setEnabled(state);
}


void DlgProxysets::setActiveHttpsProxyEdit(bool state)
{
	this->httpsLabel_1->setEnabled(state);
	this->httpsLabel_2->setEnabled(state);
	this->httpsHostnameLineEdit->setEnabled(state);
	this->httpsPortLineEdit->setEnabled(state);
}
