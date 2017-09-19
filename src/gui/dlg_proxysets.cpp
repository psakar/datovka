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
	connect(m_ui->httpAuthCheckBox, SIGNAL(stateChanged(int)),
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
	connect(m_ui->httpsAuthCheckBox, SIGNAL(stateChanged(int)),
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
	m_ui->httpAuthCheckBox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::ProxySettings::AUTO_PROXY == proxySettings.http.usage) {
		m_ui->httpNoProxyRadioButton->setChecked(false);
		m_ui->httpAutoProxyRadioButton->setChecked(true);
		m_ui->httpProxyDetectionLabel->setEnabled(true);
		m_ui->httpManualProxyRadioButton->setChecked(false);
		m_ui->httpHostnameLabel->setEnabled(false);
		m_ui->httpHostnameLine->setEnabled(false);
		m_ui->httpPortLabel->setEnabled(false);
		m_ui->httpPortLine->setEnabled(false);

		m_ui->httpUnameLabel->setEnabled(true);
		m_ui->httpUnameLine->setEnabled(true);
		m_ui->httpPwdLabel->setEnabled(true);
		m_ui->httpPwdLine->setEnabled(true);
	} else if (ProxiesSettings::ProxySettings::NO_PROXY == proxySettings.http.usage) {
		m_ui->httpNoProxyRadioButton->setChecked(true);
		m_ui->httpAutoProxyRadioButton->setChecked(false);
		m_ui->httpProxyDetectionLabel->setEnabled(false);
		m_ui->httpManualProxyRadioButton->setChecked(false);
		m_ui->httpHostnameLabel->setEnabled(false);
		m_ui->httpHostnameLine->setEnabled(false);
		m_ui->httpPortLabel->setEnabled(false);
		m_ui->httpPortLine->setEnabled(false);

		m_ui->httpUnameLabel->setEnabled(false);
		m_ui->httpUnameLine->setEnabled(false);
		m_ui->httpPwdLabel->setEnabled(false);
		m_ui->httpPwdLine->setEnabled(false);
	} else {
		m_ui->httpNoProxyRadioButton->setChecked(false);
		m_ui->httpAutoProxyRadioButton->setChecked(false);
		m_ui->httpProxyDetectionLabel->setEnabled(false);
		m_ui->httpManualProxyRadioButton->setChecked(true);
		m_ui->httpHostnameLabel->setEnabled(true);
		m_ui->httpHostnameLine->setEnabled(true);
		m_ui->httpHostnameLine->setText(proxySettings.http.hostName);
		m_ui->httpPortLabel->setEnabled(true);
		m_ui->httpPortLine->setText(
		    (proxySettings.http.port >= 0) ?
		        QString::number(proxySettings.http.port, 10) :
		        QString());

		m_ui->httpUnameLabel->setEnabled(true);
		m_ui->httpUnameLine->setEnabled(true);
		m_ui->httpPwdLabel->setEnabled(true);
		m_ui->httpPwdLine->setEnabled(true);
	}
	m_ui->httpUnameLine->setText(globProxSet.http.userName);
	m_ui->httpPwdLine->setText(globProxSet.http.password);

	m_ui->httpsProxyAuth->setHidden(true);
	m_ui->httpsAuthCheckBox->setCheckState(Qt::Unchecked);

	if (ProxiesSettings::ProxySettings::AUTO_PROXY == proxySettings.https.usage) {
		m_ui->httpsNoProxyRadioButton->setChecked(false);
		m_ui->httpsAutoProxyRadioButton->setChecked(true);
		m_ui->httpsProxyDetectionLabel->setEnabled(true);
		m_ui->httpsManualProxyRadioButton->setChecked(false);
		m_ui->httpsHostnameLabel->setEnabled(false);
		m_ui->httpsHostnameLine->setEnabled(false);
		m_ui->httpsPortLabel->setEnabled(false);
		m_ui->httpsPortLine->setEnabled(false);

		m_ui->httpsUnameLabel->setEnabled(true);
		m_ui->httpsUnameLine->setEnabled(true);
		m_ui->httpsPwdLabel->setEnabled(true);
		m_ui->httpsPwdLine->setEnabled(true);
	} else if (ProxiesSettings::ProxySettings::NO_PROXY == proxySettings.https.usage) {
		m_ui->httpsNoProxyRadioButton->setChecked(true);
		m_ui->httpsAutoProxyRadioButton->setChecked(false);
		m_ui->httpsProxyDetectionLabel->setEnabled(false);
		m_ui->httpsManualProxyRadioButton->setChecked(false);
		m_ui->httpsHostnameLabel->setEnabled(false);
		m_ui->httpsHostnameLine->setEnabled(false);
		m_ui->httpsPortLabel->setEnabled(false);
		m_ui->httpsPortLine->setEnabled(false);

		m_ui->httpsUnameLabel->setEnabled(false);
		m_ui->httpsUnameLine->setEnabled(false);
		m_ui->httpsPwdLabel->setEnabled(false);
		m_ui->httpsPwdLine->setEnabled(false);
	} else {
		m_ui->httpsNoProxyRadioButton->setChecked(false);
		m_ui->httpsAutoProxyRadioButton->setChecked(false);
		m_ui->httpsProxyDetectionLabel->setEnabled(false);
		m_ui->httpsManualProxyRadioButton->setChecked(true);
		m_ui->httpsHostnameLabel->setEnabled(true);
		m_ui->httpsHostnameLine->setEnabled(true);
		m_ui->httpsHostnameLine->setText(proxySettings.https.hostName);
		m_ui->httpsPortLabel->setEnabled(true);
		m_ui->httpsPortLine->setText(
		    (proxySettings.https.port >= 0) ?
		        QString::number(proxySettings.https.port, 10) :
		        QString());

		m_ui->httpsUnameLabel->setEnabled(true);
		m_ui->httpsUnameLine->setEnabled(true);
		m_ui->httpsPwdLabel->setEnabled(true);
		m_ui->httpsPwdLine->setEnabled(true);
	}
	m_ui->httpsUnameLine->setText(globProxSet.https.userName);
	m_ui->httpsPwdLine->setText(globProxSet.https.password);
}

void DlgProxysets::showHttpProxyPassword(int checkState)
{
	m_ui->httpProxyAuth->setHidden(Qt::Unchecked == checkState);
}

void DlgProxysets::showHttpsProxyPassword(int checkState)
{
	m_ui->httpsProxyAuth->setHidden(Qt::Unchecked == checkState);
}

void DlgProxysets::toggleHttpProxyDetection(bool enabled)
{
	m_ui->httpProxyDetectionLabel->setEnabled(enabled);
}

void DlgProxysets::toggleHttpsProxyDetection(bool enabled)
{
	m_ui->httpsProxyDetectionLabel->setEnabled(enabled);
}

void DlgProxysets::toggleHttpProxyPassword(bool checked)
{
	m_ui->httpUnameLabel->setEnabled(!checked);
	m_ui->httpUnameLine->setEnabled(!checked);
	m_ui->httpPwdLabel->setEnabled(!checked);
	m_ui->httpPwdLine->setEnabled(!checked);
}

void DlgProxysets::toggleHttpsProxyPassword(bool checked)
{
	m_ui->httpsUnameLabel->setEnabled(!checked);
	m_ui->httpsUnameLine->setEnabled(!checked);
	m_ui->httpsPwdLabel->setEnabled(!checked);
	m_ui->httpsPwdLine->setEnabled(!checked);
}

void DlgProxysets::setActiveHttpProxyEdit(bool enabled)
{
	m_ui->httpHostnameLabel->setEnabled(enabled);
	m_ui->httpHostnameLine->setEnabled(enabled);
	m_ui->httpPortLabel->setEnabled(enabled);
	m_ui->httpPortLine->setEnabled(enabled);
}

void DlgProxysets::setActiveHttpsProxyEdit(bool enabled)
{
	m_ui->httpsHostnameLabel->setEnabled(enabled);
	m_ui->httpsHostnameLine->setEnabled(enabled);
	m_ui->httpsPortLabel->setEnabled(enabled);
	m_ui->httpsPortLine->setEnabled(enabled);
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
		globProxSet.http.hostName = m_ui->httpHostnameLine->text();
		globProxSet.http.port = m_ui->httpPortLine->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.http.port = -1;
		}
	}
	globProxSet.http.userName = m_ui->httpUnameLine->text();
	globProxSet.http.password = m_ui->httpPwdLine->text();

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
		globProxSet.https.hostName = m_ui->httpsHostnameLine->text();
		globProxSet.https.port = m_ui->httpsPortLine->text().toInt(&ok, 10);
		if (!ok) {
			globProxSet.https.port = -1;
		}
	}
	globProxSet.https.userName = m_ui->httpsUnameLine->text();
	globProxSet.https.password = m_ui->httpsPwdLine->text();

	/*
	 * Apply to global variables, although, restart may be needed if
	 * already connected.
	 */
	globProxSet.setProxyEnvVars();
}
