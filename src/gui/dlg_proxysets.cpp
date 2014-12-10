

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
		QPair<QString, int> proxy = detectHttpProxy();
		if (!proxy.first.isEmpty()) {
			this->httpProxyDetectionLabel->setText(
			    tr("Proxy has been detected") + ": " +
			    proxy.first + ":" +
			    QString::number(proxy.second, 10));
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

void DlgProxysets::loadProxyDialog(const GlobProxySettings &proxySettings)
{
	QStringList hostport;

	this->httpProxyAuth->setHidden(true);
	this->httpAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (proxySettings.http_proxy == "-1") {
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
	} else if (proxySettings.http_proxy == "None") {
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
		hostport = proxySettings.http_proxy.split(":");
		this->httpHostnameLineEdit->setText(hostport[0]);
		this->httpPortLineEdit->setText(hostport[1]);

		this->httpUnameLabel->setEnabled(true);
		this->httpPwdLabel->setEnabled(true);
		this->httpUnameEdit->setEnabled(true);
		this->httpPwdEdit->setEnabled(true);
	}
	this->httpUnameEdit->setText(globProxSet.http_proxy_username);
	this->httpPwdEdit->setText(globProxSet.http_proxy_password);

	this->httpsProxyAuth->setHidden(true);
	this->httpsAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (proxySettings.https_proxy == "-1") {
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
	} else if (proxySettings.https_proxy == "None") {
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
		hostport = proxySettings.https_proxy.split(":");
		this->httpsHostnameLineEdit->setText(hostport[0]);
		this->httpsPortLineEdit->setText(hostport[1]);

		this->httpsUnameLabel->setEnabled(true);
		this->httpsPwdLabel->setEnabled(true);
		this->httpsUnameEdit->setEnabled(true);
		this->httpsPwdEdit->setEnabled(true);
	}
	this->httpsUnameEdit->setText(globProxSet.https_proxy_username);
	this->httpsPwdEdit->setText(globProxSet.https_proxy_password);

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
	if (this->httpNoProxyRadioButton->isChecked()) {
		globProxSet.http_proxy = "None";
	} else if (this->httpAutoProxyRadioButton->isChecked()) {
		globProxSet.http_proxy = "-1";
	} else {
		globProxSet.http_proxy = this->httpHostnameLineEdit->text() +
		    ":" + this->httpPortLineEdit->text();
	}
	globProxSet.http_proxy_username = this->httpUnameEdit->text();
	globProxSet.http_proxy_password = this->httpPwdEdit->text();

	if (this->httpsNoProxyRadioButton->isChecked()) {
		globProxSet.https_proxy = "None";
	} else if (this->httpsAutoProxyRadioButton->isChecked()) {
		globProxSet.https_proxy = "-1";
	} else {
		globProxSet.https_proxy = this->httpsHostnameLineEdit->text() +
		    ":" + this->httpsPortLineEdit->text();
	}
	globProxSet.https_proxy_username = this->httpsUnameEdit->text();
	globProxSet.https_proxy_password = this->httpsPwdEdit->text();
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
