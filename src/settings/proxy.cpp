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

#include <QNetworkProxyQuery>
#include <QUrl>

#include "src/common.h"
#include "src/log/log.h"
#include "src/settings/proxy.h"

ProxiesSettings globProxSet;

/* Defaults. */
static const
ProxiesSettings dfltGlobProxSet;

#define NO_PROXY "None"

#define NO_PORT -1

const QString ProxiesSettings::noProxyStr(QLatin1String(NO_PROXY));
const QString ProxiesSettings::autoProxyStr(QLatin1String("-1"));

ProxiesSettings::ProxySettings::ProxySettings(void)
    : hostName(QLatin1String(NO_PROXY)), /* static initialisation order fiasco */
    port(NO_PORT),
    userName(),
    password()
{
}

ProxiesSettings::ProxiesSettings(void)
    : https(),
    http()
{
}

void ProxiesSettings::loadFromSettings(const QSettings &settings)
{
	QString auxStr;
	bool ok;

	auxStr = settings.value("connection/https_proxy").toString();
	if (auxStr.isEmpty() || (noProxyStr == auxStr)) {
		https.hostName = noProxyStr;
		https.port = NO_PORT;
	} else if (autoProxyStr == auxStr) {
		https.hostName = autoProxyStr;
		https.port = NO_PORT;
	} else {
		if (auxStr.contains(":")) {
			https.hostName = auxStr.section(":", 0, -2);
			https.port =
			    auxStr.section(":", -1, -1).toInt(&ok, 10);
			if (!ok) {
				https.hostName = noProxyStr;
				https.port = NO_PORT;
			}
		} else {
			https.hostName = noProxyStr;
			https.port = NO_PORT;
		}
	}
	https.userName =
	    settings.value("connection/https_proxy_username").toString();
	https.password = fromBase64(
	    settings.value("connection/https_proxy_password").toString());

	auxStr = settings.value("connection/http_proxy").toString();
	if (auxStr.isEmpty() || (noProxyStr == auxStr)) {
		http.hostName = noProxyStr;
		http.port = NO_PORT;
	} else if (autoProxyStr == auxStr) {
		http.hostName = autoProxyStr;
		http.port = NO_PORT;
	} else {
		if (auxStr.contains(":")) {
			http.hostName = auxStr.section(":", 0, -2);
			http.port =
			    auxStr.section(":", -1, -1).toInt(&ok, 10);
			if (!ok) {
				http.hostName = noProxyStr;
				http.port = NO_PORT;
			}
		} else {
			http.hostName = noProxyStr;
			http.port = NO_PORT;
		}
	}
	http.userName =
	    settings.value("connection/http_proxy_username").toString();
	http.password = fromBase64(
	    settings.value("connection/http_proxy_password").toString());
}

void ProxiesSettings::saveToSettings(QSettings &settings) const
{
	QString auxStr;

	settings.beginGroup("connection");

	auxStr = https.hostName;
	if (https.port >= 0) {
		auxStr += ":" + QString::number(https.port, 10);
	}
	settings.setValue("https_proxy", auxStr);
	if (!https.userName.isEmpty()) {
		settings.setValue("https_proxy_username",
		    https.userName);
	}
	if (!https.password.isEmpty()) {
		settings.setValue("https_proxy_password",
		    toBase64(https.password));
	}

	auxStr = http.hostName;
	if (http.port >= 0) {
		auxStr += ":" + QString::number(http.port, 10);
	}
	settings.setValue("http_proxy", auxStr);
	if (!http.userName.isEmpty()) {
		settings.setValue("http_proxy_username", http.userName);
	}
	if (!http.password.isEmpty()) {
		settings.setValue("http_proxy_password",
		    toBase64(http.password));
	}

	settings.endGroup();
}

bool ProxiesSettings::setProxyEnvVars(void)
{
	/* Currently force some values. */

	qDebug() << "old http_proxy:" << qgetenv("http_proxy");
	qDebug() << "old https_proxy:" << qgetenv("https_proxy");

	qputenv("http_proxy", QByteArray("http://k:aaaa@127.0.0.1:3128"));
	qputenv("https_proxy", QByteArray("http://k:aaaba@127.0.0.1:3128"));

	qDebug() << "new http_proxy:" << qgetenv("http_proxy");
	qDebug() << "new https_proxy:" << qgetenv("https_proxy");

	return true;
}

ProxiesSettings::ProxySettings ProxiesSettings::detectHttpProxy(void)
{
	ProxySettings settings;

	settings.hostName = noProxyStr;
	settings.port = NO_PORT;

	/* TODO -- Try to contact the proxy to check whether it works? */

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	QByteArray envVar = qgetenv("http_proxy");
	QUrl proxyUrl(QString::fromLatin1(envVar));

	if (proxyUrl.isValid()) {
		logInfo("Detected proxy URL '%s', '%s' '%s'.\n",
		    proxyUrl.toString().toUtf8().constData(),
		    proxyUrl.userName().toUtf8().constData(),
		    proxyUrl.password().toUtf8().constData());
		settings.hostName = proxyUrl.host();
		settings.port = proxyUrl.port();
		settings.userName = proxyUrl.userName();
		settings.password = proxyUrl.password();
		return settings;
	}
#else
	QNetworkProxyQuery npq(QUrl("http://www.nic.cz"));

	QList<QNetworkProxy> listOfProxies =
	   QNetworkProxyFactory::systemProxyForQuery(npq);

	if (1 < listOfProxies.size()) {
		logWarning("%s/n", "Multiple proxies detected. Using first.");
	}

	foreach (QNetworkProxy p, listOfProxies) {
		if (!p.hostName().isEmpty()) {
			settings.hostName = p.hostName();
			settings.port = p.port();
			settings.userName = p.user();
			settings.password = p.password();
			return settings;
		}
	}
#endif /* Q_OS_UNIX && !Q_OS_MAC */

	return settings;
}
