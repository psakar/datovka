/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */


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
		QString proxyMsg(
		    tr("No proxy detected, direct connection will be used."));

		QByteArray proxyVal(ProxiesSettings::detectEnvironment(ProxiesSettings::HTTP));
		if (!proxyVal.isEmpty()) {
			proxyMsg =
			    tr("Proxy has been detected") + ": " + proxyVal;
		}
		this->httpProxyDetectionLabel->setText(proxyMsg);
	}

	connect(this->httpsNoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpsProxyPassword(bool)));
	connect(this->httpsAutoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpsProxyDetection(bool)));
	connect(this->httpsManualProxyRadioButton, SIGNAL(toggled (bool)),
	    this, SLOT(setActiveHttpsProxyEdit(bool)));
	connect(this->httpsAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(showHttpsProxyPassword(int)));

	{
		QString proxyMsg(
		    tr("No proxy detected, direct connection will be used."));

		QByteArray proxyVal(ProxiesSettings::detectEnvironment(ProxiesSettings::HTTPS));
		if (!proxyVal.isEmpty()) {
			proxyMsg =
			    tr("Proxy has been detected") + ": " + proxyVal;
		}
		this->httpsProxyDetectionLabel->setText(proxyMsg);
	}

	loadProxyDialog(globProxSet);
}

void DlgProxysets::loadProxyDialog(const ProxiesSettings &proxySettings)
{
	this->httpProxyAuth->setHidden(true);
	this->httpAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::ProxySettings::AUTO_PROXY == proxySettings.http.usage) {
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
	} else if (ProxiesSettings::ProxySettings::NO_PROXY == proxySettings.http.usage) {
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

	if (ProxiesSettings::ProxySettings::AUTO_PROXY == proxySettings.https.usage) {
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
	} else if (ProxiesSettings::ProxySettings::NO_PROXY == proxySettings.https.usage) {
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
		globProxSet.http.usage = ProxiesSettings::ProxySettings::NO_PROXY;
		globProxSet.http.hostName.clear();
		globProxSet.http.port = -1;
	} else if (this->httpAutoProxyRadioButton->isChecked()) {
		globProxSet.http.usage = ProxiesSettings::ProxySettings::AUTO_PROXY;
		globProxSet.http.hostName.clear();
		globProxSet.http.port = -1;
	} else {
		globProxSet.http.usage = ProxiesSettings::ProxySettings::DEFINED_PROXY;
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
		globProxSet.https.usage = ProxiesSettings::ProxySettings::NO_PROXY;
		globProxSet.https.hostName.clear();
		globProxSet.https.port = -1;
	} else if (this->httpsAutoProxyRadioButton->isChecked()) {
		globProxSet.https.usage = ProxiesSettings::ProxySettings::AUTO_PROXY;
		globProxSet.https.hostName.clear();
		globProxSet.https.port = -1;
	} else {
		globProxSet.https.usage = ProxiesSettings::ProxySettings::DEFINED_PROXY;
		globProxSet.https.hostName = this->httpsHostnameLineEdit->text();
		globProxSet.https.port =
		    this->httpsPortLineEdit->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.https.port = -1;
		}
	}
	globProxSet.https.userName = this->httpsUnameEdit->text();
	globProxSet.https.password = this->httpsPwdEdit->text();

	/*
	 * Apply to global variables, although, restart may be needed if
	 * already connected.
	 */
	globProxSet.setProxyEnvVars();
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
