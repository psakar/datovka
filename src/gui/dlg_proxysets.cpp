/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include "src/gui/dlg_proxysets.h"
#include "ui_dlg_proxysets.h"

DlgProxysets::DlgProxysets(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgProxysets)
{
	m_ui->setupUi(this);

	connect(m_ui->proxyButtonBox, SIGNAL(accepted()),
	    this, SLOT(saveChanges(void)));

	connect(m_ui->httpNoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpProxyPassword(bool)));
	connect(m_ui->httpAutoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpProxyDetection(bool)));
	connect(m_ui->httpManualProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(setActiveHttpProxyEdit(bool)));
	connect(m_ui->httpAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(showHttpProxyPassword(int)));

	{
		QString proxyMsg(
		    tr("No proxy detected, direct connection will be used."));

		QByteArray proxyVal(ProxiesSettings::detectEnvironment(ProxiesSettings::HTTP));
		if (!proxyVal.isEmpty()) {
			proxyMsg =
			    tr("Proxy has been detected") + ": " + proxyVal;
		}
		m_ui->httpProxyDetectionLabel->setText(proxyMsg);
	}

	connect(m_ui->httpsNoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpsProxyPassword(bool)));
	connect(m_ui->httpsAutoProxyRadioButton, SIGNAL(toggled(bool)),
	    this, SLOT(toggleHttpsProxyDetection(bool)));
	connect(m_ui->httpsManualProxyRadioButton, SIGNAL(toggled (bool)),
	    this, SLOT(setActiveHttpsProxyEdit(bool)));
	connect(m_ui->httpsAuthenticationCheckbox, SIGNAL(stateChanged(int)),
	    this, SLOT(showHttpsProxyPassword(int)));

	{
		QString proxyMsg(
		    tr("No proxy detected, direct connection will be used."));

		QByteArray proxyVal(ProxiesSettings::detectEnvironment(ProxiesSettings::HTTPS));
		if (!proxyVal.isEmpty()) {
			proxyMsg =
			    tr("Proxy has been detected") + ": " + proxyVal;
		}
		m_ui->httpsProxyDetectionLabel->setText(proxyMsg);
	}

	loadProxyDialog(globProxSet);
}

DlgProxysets::~DlgProxysets(void)
{
	delete m_ui;
}

void DlgProxysets::loadProxyDialog(const ProxiesSettings &proxySettings)
{
	m_ui->httpProxyAuth->setHidden(true);
	m_ui->httpAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::ProxySettings::AUTO_PROXY == proxySettings.http.usage) {
		m_ui->httpNoProxyRadioButton->setChecked(false);
		m_ui->httpAutoProxyRadioButton->setChecked(true);
		m_ui->httpProxyDetectionLabel->setEnabled(true);
		m_ui->httpManualProxyRadioButton->setChecked(false);
		m_ui->httpLabel_1->setEnabled(false);
		m_ui->httpLabel_2->setEnabled(false);
		m_ui->httpHostnameLineEdit->setEnabled(false);
		m_ui->httpPortLineEdit->setEnabled(false);

		m_ui->httpUnameLabel->setEnabled(true);
		m_ui->httpPwdLabel->setEnabled(true);
		m_ui->httpUnameEdit->setEnabled(true);
		m_ui->httpPwdEdit->setEnabled(true);
	} else if (ProxiesSettings::ProxySettings::NO_PROXY == proxySettings.http.usage) {
		m_ui->httpNoProxyRadioButton->setChecked(true);
		m_ui->httpAutoProxyRadioButton->setChecked(false);
		m_ui->httpProxyDetectionLabel->setEnabled(false);
		m_ui->httpManualProxyRadioButton->setChecked(false);
		m_ui->httpLabel_1->setEnabled(false);
		m_ui->httpLabel_2->setEnabled(false);
		m_ui->httpHostnameLineEdit->setEnabled(false);
		m_ui->httpPortLineEdit->setEnabled(false);

		m_ui->httpUnameLabel->setEnabled(false);
		m_ui->httpPwdLabel->setEnabled(false);
		m_ui->httpUnameEdit->setEnabled(false);
		m_ui->httpPwdEdit->setEnabled(false);
	} else {
		m_ui->httpNoProxyRadioButton->setChecked(false);
		m_ui->httpAutoProxyRadioButton->setChecked(false);
		m_ui->httpProxyDetectionLabel->setEnabled(false);
		m_ui->httpManualProxyRadioButton->setChecked(true);
		m_ui->httpLabel_1->setEnabled(true);
		m_ui->httpLabel_2->setEnabled(true);
		m_ui->httpHostnameLineEdit->setEnabled(true);
		m_ui->httpHostnameLineEdit->setText(
		    proxySettings.http.hostName);
		m_ui->httpPortLineEdit->setText(
		    (proxySettings.http.port >= 0) ?
		        QString::number(proxySettings.http.port, 10) :
		        QString());

		m_ui->httpUnameLabel->setEnabled(true);
		m_ui->httpPwdLabel->setEnabled(true);
		m_ui->httpUnameEdit->setEnabled(true);
		m_ui->httpPwdEdit->setEnabled(true);
	}
	m_ui->httpUnameEdit->setText(globProxSet.http.userName);
	m_ui->httpPwdEdit->setText(globProxSet.http.password);

	m_ui->httpsProxyAuth->setHidden(true);
	m_ui->httpsAuthenticationCheckbox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::ProxySettings::AUTO_PROXY == proxySettings.https.usage) {
		m_ui->httpsNoProxyRadioButton->setChecked(false);
		m_ui->httpsAutoProxyRadioButton->setChecked(true);
		m_ui->httpsProxyDetectionLabel->setEnabled(true);
		m_ui->httpsManualProxyRadioButton->setChecked(false);
		m_ui->httpsLabel_1->setEnabled(false);
		m_ui->httpsLabel_2->setEnabled(false);
		m_ui->httpsHostnameLineEdit->setEnabled(false);
		m_ui->httpsPortLineEdit->setEnabled(false);

		m_ui->httpsUnameLabel->setEnabled(true);
		m_ui->httpsPwdLabel->setEnabled(true);
		m_ui->httpsUnameEdit->setEnabled(true);
		m_ui->httpsPwdEdit->setEnabled(true);
	} else if (ProxiesSettings::ProxySettings::NO_PROXY == proxySettings.https.usage) {
		m_ui->httpsNoProxyRadioButton->setChecked(true);
		m_ui->httpsAutoProxyRadioButton->setChecked(false);
		m_ui->httpsProxyDetectionLabel->setEnabled(false);
		m_ui->httpsManualProxyRadioButton->setChecked(false);
		m_ui->httpsLabel_1->setEnabled(false);
		m_ui->httpsLabel_2->setEnabled(false);
		m_ui->httpsHostnameLineEdit->setEnabled(false);
		m_ui->httpsPortLineEdit->setEnabled(false);

		m_ui->httpsUnameLabel->setEnabled(false);
		m_ui->httpsPwdLabel->setEnabled(false);
		m_ui->httpsUnameEdit->setEnabled(false);
		m_ui->httpsPwdEdit->setEnabled(false);
	} else {
		m_ui->httpsNoProxyRadioButton->setChecked(false);
		m_ui->httpsAutoProxyRadioButton->setChecked(false);
		m_ui->httpsProxyDetectionLabel->setEnabled(false);
		m_ui->httpsManualProxyRadioButton->setChecked(true);
		m_ui->httpsLabel_1->setEnabled(true);
		m_ui->httpsLabel_2->setEnabled(true);
		m_ui->httpsHostnameLineEdit->setEnabled(true);
		m_ui->httpsHostnameLineEdit->setText(
		    proxySettings.https.hostName);
		m_ui->httpsPortLineEdit->setText(
		    (proxySettings.https.port >= 0) ?
		        QString::number(proxySettings.https.port, 10) :
		        QString());

		m_ui->httpsUnameLabel->setEnabled(true);
		m_ui->httpsPwdLabel->setEnabled(true);
		m_ui->httpsUnameEdit->setEnabled(true);
		m_ui->httpsPwdEdit->setEnabled(true);
	}
	m_ui->httpsUnameEdit->setText(globProxSet.https.userName);
	m_ui->httpsPwdEdit->setText(globProxSet.https.password);
}

void DlgProxysets::showHttpProxyPassword(int state)
{
	m_ui->httpProxyAuth->setHidden(Qt::Unchecked == state);
}

void DlgProxysets::showHttpsProxyPassword(int state)
{
	m_ui->httpsProxyAuth->setHidden(Qt::Unchecked == state);
}

void DlgProxysets::toggleHttpProxyDetection(bool state)
{
	m_ui->httpProxyDetectionLabel->setEnabled(state);
}

void DlgProxysets::toggleHttpsProxyDetection(bool state)
{
	m_ui->httpsProxyDetectionLabel->setEnabled(state);
}

void DlgProxysets::toggleHttpProxyPassword(bool state)
{
	m_ui->httpUnameLabel->setEnabled(!state);
	m_ui->httpPwdLabel->setEnabled(!state);
	m_ui->httpUnameEdit->setEnabled(!state);
	m_ui->httpPwdEdit->setEnabled(!state);
}

void DlgProxysets::toggleHttpsProxyPassword(bool state)
{
	m_ui->httpsUnameLabel->setEnabled(!state);
	m_ui->httpsPwdLabel->setEnabled(!state);
	m_ui->httpsUnameEdit->setEnabled(!state);
	m_ui->httpsPwdEdit->setEnabled(!state);
}

void DlgProxysets::saveChanges(void) const
{
	bool ok;

	/* TODO -- Checks and notification about incorrect values. */

	if (m_ui->httpNoProxyRadioButton->isChecked()) {
		globProxSet.http.usage = ProxiesSettings::ProxySettings::NO_PROXY;
		globProxSet.http.hostName.clear();
		globProxSet.http.port = -1;
	} else if (m_ui->httpAutoProxyRadioButton->isChecked()) {
		globProxSet.http.usage = ProxiesSettings::ProxySettings::AUTO_PROXY;
		globProxSet.http.hostName.clear();
		globProxSet.http.port = -1;
	} else {
		globProxSet.http.usage = ProxiesSettings::ProxySettings::DEFINED_PROXY;
		globProxSet.http.hostName = m_ui->httpHostnameLineEdit->text();
		globProxSet.http.port =
		    m_ui->httpPortLineEdit->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.http.port = -1;
		}
	}
	globProxSet.http.userName = m_ui->httpUnameEdit->text();
	globProxSet.http.password = m_ui->httpPwdEdit->text();

	if (m_ui->httpsNoProxyRadioButton->isChecked()) {
		globProxSet.https.usage = ProxiesSettings::ProxySettings::NO_PROXY;
		globProxSet.https.hostName.clear();
		globProxSet.https.port = -1;
	} else if (m_ui->httpsAutoProxyRadioButton->isChecked()) {
		globProxSet.https.usage = ProxiesSettings::ProxySettings::AUTO_PROXY;
		globProxSet.https.hostName.clear();
		globProxSet.https.port = -1;
	} else {
		globProxSet.https.usage = ProxiesSettings::ProxySettings::DEFINED_PROXY;
		globProxSet.https.hostName = m_ui->httpsHostnameLineEdit->text();
		globProxSet.https.port =
		    m_ui->httpsPortLineEdit->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.https.port = -1;
		}
	}
	globProxSet.https.userName = m_ui->httpsUnameEdit->text();
	globProxSet.https.password = m_ui->httpsPwdEdit->text();

	/*
	 * Apply to global variables, although, restart may be needed if
	 * already connected.
	 */
	globProxSet.setProxyEnvVars();
}

void DlgProxysets::setActiveHttpProxyEdit(bool state)
{
	m_ui->httpLabel_1->setEnabled(state);
	m_ui->httpLabel_2->setEnabled(state);
	m_ui->httpHostnameLineEdit->setEnabled(state);
	m_ui->httpPortLineEdit->setEnabled(state);
}

void DlgProxysets::setActiveHttpsProxyEdit(bool state)
{
	m_ui->httpsLabel_1->setEnabled(state);
	m_ui->httpsLabel_2->setEnabled(state);
	m_ui->httpsHostnameLineEdit->setEnabled(state);
	m_ui->httpsPortLineEdit->setEnabled(state);
}
