/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#define HTTP_PROXY_VARMAME "http_proxy"
#define HTTPS_PROXY_VARMAME "https_proxy"

ProxiesSettings globProxSet;

#define NO_PROXY_STR "None"
#define AUTO_PROXY_STR "-1"

#define NO_PORT -1

const QByteArray ProxiesSettings::httpProxyEnvVar(qgetenv(HTTP_PROXY_VARMAME));
const QByteArray ProxiesSettings::httpsProxyEnvVar(qgetenv(HTTPS_PROXY_VARMAME));

ProxiesSettings::ProxySettings::ProxySettings(void)
    : usage(NO_PROXY), /* Default is no proxy. */
    userName(),
    password(),
    hostName(QLatin1String(NO_PROXY_STR)),
    port(NO_PORT)
{
}

QByteArray ProxiesSettings::ProxySettings::toEnvVal(void) const
{
	QByteArray val;

	switch (usage) {
	case NO_PROXY:
	case AUTO_PROXY:
		break;
	case DEFINED_PROXY:
		if (!userName.isEmpty() && !password.isEmpty()) {
			val = userName.toUtf8() + ":" + password.toUtf8() + "@";
		}
		if (!hostName.isEmpty() && port >= 0 && port <= 65535) {
			val += hostName.toUtf8() + ":" +
			    QByteArray::number(port, 10);
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return val;
}

ProxiesSettings::ProxiesSettings(void)
    : https(),
    http()
{
}

/*!
 * @brief Set up configuration strings.
 *
 * @param[in] type Whether to use HTTP or HTTPS settings.
 * @param[in] full Whether to prefix settings group name.
 * @param[out] connProxy Proxy entry name.
 * @param[out] connProxyUser Proxy user name entry.
 * @param[out] connProxyPwd Proxy password entry.
 */
static
void settingsStringSetUp(enum ProxiesSettings::Type type, bool full,
    QString &connProxy, QString &connProxyUser, QString &connProxyPwd)
{
	QString prefix;
	if (full) {
		prefix = QLatin1String("connection/");
	}

	switch (type) {
	case ProxiesSettings::HTTP:
		connProxy = prefix + QLatin1String("http_proxy");
		connProxyUser = prefix + QLatin1String("http_proxy_username");
		connProxyPwd = prefix + QLatin1String("http_proxy_password");
		break;
	case ProxiesSettings::HTTPS:
		connProxy = prefix + QLatin1String("https_proxy");
		connProxyUser = prefix + QLatin1String("https_proxy_username");
		connProxyPwd = prefix + QLatin1String("https_proxy_password");
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}

/*!
 * @brief Reads proxy configuration from settings,
 *
 * @param[in] settings Settings structure.
 * @param[in] type Whether to read HTTP or HTTPS proxy settings,
 * @return Proxy settings structure.
 */
static
ProxiesSettings::ProxySettings loadProxySettings(const QSettings &settings,
    enum ProxiesSettings::Type type)
{
	QString connProxy, connProxyUser, connProxyPwd;
	settingsStringSetUp(type, true, connProxy, connProxyUser, connProxyPwd);

	ProxiesSettings::ProxySettings proxy;

	QString auxStr = settings.value(connProxy).toString();
	if (auxStr.isEmpty() || (auxStr == QLatin1String(NO_PROXY_STR))) {
		/* Defaults. */
		proxy.usage = ProxiesSettings::ProxySettings::NO_PROXY;
	} else if (auxStr == QLatin1String(AUTO_PROXY_STR)) {
		proxy.usage = ProxiesSettings::ProxySettings::AUTO_PROXY;
	} else if (auxStr.contains(":")) {
		QString hostName(auxStr.section(":", 0, -2));
		bool ok;
		int port = auxStr.section(":", -1, -1).toInt(&ok, 10);
		if (ok) {
			proxy.usage = ProxiesSettings::ProxySettings::DEFINED_PROXY;
			proxy.hostName = hostName;
			proxy.port = port;
		}
	} else {
		/* Some bogus -> defaults. */
		proxy.usage = ProxiesSettings::ProxySettings::NO_PROXY;
	}

	proxy.userName = settings.value(connProxyUser).toString();
	proxy.password = fromBase64(settings.value(connProxyPwd).toString());

	return proxy;
}

void ProxiesSettings::loadFromSettings(const QSettings &settings)
{
	http = loadProxySettings(settings, HTTP);
	https = loadProxySettings(settings, HTTPS);
}

/*!
 * @brief Writes proxy configuration to settings.
 *
 * @param[out] settings Settings structure.
 * @param[in] type Whether to read HTTP or HTTPS proxy settings,
 * @param[in] proxy Proxy settings structure.
 */
static
void saveProxySettings(QSettings &settings, enum ProxiesSettings::Type type,
    const ProxiesSettings::ProxySettings &proxy)
{
	QString connProxy, connProxyUser, connProxyPwd;
	settingsStringSetUp(type, false, connProxy, connProxyUser,
	    connProxyPwd);

	switch (proxy.usage) {
	case ProxiesSettings::ProxySettings::NO_PROXY:
		/* No proxy is default. */
		//settings.setValue(connProxy, QLatin1String(NO_PROXY_STR));
		return;
		break;
	case ProxiesSettings::ProxySettings::AUTO_PROXY:
		settings.setValue(connProxy, QLatin1String(AUTO_PROXY_STR));
		break;
	case ProxiesSettings::ProxySettings::DEFINED_PROXY:
		if (proxy.hostName.isEmpty() ||
		    proxy.port < 0 || proxy.port > 65535) {
			/* Default to no proxy. */
			return;
		}
		settings.setValue(connProxy, proxy.hostName + ":" + QString::number(proxy.port, 10));
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	if (!proxy.userName.isEmpty()) {
		settings.setValue(connProxyUser, proxy.userName);
	}
	if (!proxy.password.isEmpty()) {
		settings.setValue(connProxyPwd, toBase64(proxy.password));
	}
}

void ProxiesSettings::saveToSettings(QSettings &settings) const
{
	settings.beginGroup("connection");

	saveProxySettings(settings, HTTP, http);
	saveProxySettings(settings, HTTPS, https);

	settings.endGroup();
}

/*!
 * @brief Detects proxy settings from environment.
 *
 * @note Takes values from stored environmental values or from system.
 *
 * @param[in] type Whether to obtain HPPT or HTTPS settings.
 * @returns Values as they would be stored in a http(s)_proxy variable (e.g.
 *     'http://a:aaa@127.0.0.1:3128', 'b:bb@192.168.1.1:3128',
 *     '172.0.0.1:3128').
 */
QByteArray ProxiesSettings::detectEnvironment(enum ProxiesSettings::Type type)
{
	QByteArray proxyEnvVar((type == HTTP) ?
	    httpProxyEnvVar : httpsProxyEnvVar);

	/* http(s)_proxy environment variable takes precedence if is a valid URL. */
	if (!proxyEnvVar.isEmpty()) {
		QUrl proxyUrl(QString::fromLatin1(proxyEnvVar));
		if (proxyUrl.isValid()) {
			return proxyEnvVar;
		}
	}

	proxyEnvVar.clear();

#if !defined(Q_OS_UNIX) || defined(Q_OS_MAC)
	QNetworkProxyQuery npq(QUrl((type == HTTP) ?
	    QLatin1String("http://www.nic.cz") :
	    QLatin1String("https://www.nic.cz")));

	QList<QNetworkProxy> listOfProxies(
	    QNetworkProxyFactory::systemProxyForQuery(npq));

	if (1 < listOfProxies.size()) {
		logWarning("%s/n", (type == HTTP) ?
		    "Multiple HTTP proxies detected. Using first." :
		    "Multiple HTTPS proxies detected. Using first.");
	}

	foreach (const QNetworkProxy &p, listOfProxies) {
		if (!p.hostName().isEmpty()) {
			proxyEnvVar = p.hostName().toUtf8();
			proxyEnvVar += ":" + QByteArray::number(p.port(), 10);

			if (!p.user().isEmpty() && !p.password().isEmpty()) {
				proxyEnvVar = p.user().toUtf8() + ":" +
				    p.password().toUtf8() + "@" + proxyEnvVar;
			}
			break; /* Use only first entry. */
		}
	}
#endif /* !Q_OS_UNIX || Q_OS_MAC */

	/* TODO -- Try contacting the proxy to check whether it works. */

	return proxyEnvVar;
}

/*!
 * @brief Sets proxy environmental variable.
 *
 * @param[in] type Whether to obtain HPPT or HTTPS settings.
 * @param[in] proxy Proxy settings structure.
 * @return True on success.
 */
static
bool setProxyEnvVar(enum ProxiesSettings::Type type,
    const ProxiesSettings::ProxySettings &proxy)
{
	const char *proxyVarName = NULL;
	const QByteArray *proxyStartUp = 0;
	switch (type) {
	case ProxiesSettings::HTTP:
		proxyVarName = HTTP_PROXY_VARMAME;
		proxyStartUp = &ProxiesSettings::httpProxyEnvVar;
		break;
	case ProxiesSettings::HTTPS:
		proxyVarName = HTTPS_PROXY_VARMAME;
		proxyStartUp = &ProxiesSettings::httpsProxyEnvVar;
		break;
	default:
		Q_ASSERT(0);
		return false;
		break;
	}
	Q_ASSERT(proxyVarName != NULL);
	Q_ASSERT(proxyStartUp != 0);

	QByteArray proxyOld(qgetenv(proxyVarName));

	QByteArray proxyNew;
	switch (proxy.usage) {
	case ProxiesSettings::ProxySettings::NO_PROXY:
		/* Leave new empty. */
		break;
	case ProxiesSettings::ProxySettings::AUTO_PROXY:
		proxyNew = ProxiesSettings::detectEnvironment(type);
		break;
	case ProxiesSettings::ProxySettings::DEFINED_PROXY:
		proxyNew = proxy.toEnvVal();
		break;
	default:
		Q_ASSERT(0);
		return false;
		break;
	}

	logInfoNL("Changing %s (system '%s'): old '%s' -> new '%s'",
	    proxyVarName, proxyStartUp->constData(), proxyOld.constData(),
	    proxyNew.constData());

	qputenv(proxyVarName, proxyNew);

	return true;
}

bool ProxiesSettings::setProxyEnvVars(void) const
{
	bool ret = true;

	ret = setProxyEnvVar(HTTP, http) && ret;
	ret = setProxyEnvVar(HTTPS, https) && ret;

	if (!ret) {
		logErrorNL("%s", "Setting proxy environment variables failed.");
	}

	return ret;
}

/*!
 * @brief Creates a proxy settings stricture from values returned when
 *     detecting environment.
 *
 * @param[in] Values as they would be stored in a http(s)_proxy variable (e.g.
 *     'http://a:aaa@127.0.0.1:3128', 'b:bb@192.168.1.1:3128',
 *     '172.0.0.1:3128').
 * @param[in] usage Specifies the type of the returned value.
 * @return Proxy settings structure.
 */
static
ProxiesSettings::ProxySettings fromEnvVal(QByteArray proxyEnv,
    ProxiesSettings::ProxySettings::Usage usage)
{
	ProxiesSettings::ProxySettings settings; /* noProxyStr, NO_PORT */

	int aux;

	/* Remove leading protocol. */
	aux = proxyEnv.indexOf("://");
	if (aux >= 0) {
		proxyEnv.remove(0, aux + 3); /* 3 == strlen("://") */
	}

	QByteArray userPwd;

	aux = proxyEnv.count('@');
	switch (aux) {
	case 0:
		/* proxyEnv contains host and port */
		break;
	case 1:
		{
			QList<QByteArray> list = proxyEnv.split('@');
			Q_ASSERT(list.size() == 2);
			userPwd = list[0];
			proxyEnv = list[1];
		}
		break;
	default:
		return ProxiesSettings::ProxySettings();
		break;
	}

	if (!userPwd.isEmpty()) { /* User name and password. */
		if (1 != userPwd.count(':')) {
			return ProxiesSettings::ProxySettings();
		}
		QList<QByteArray> list = userPwd.split(':');
		Q_ASSERT(list.size() == 2);
		settings.userName = list[0];
		settings.password = list[1];
	}

	if (1 != proxyEnv.count(':')) { /* Host and port. */
		return ProxiesSettings::ProxySettings();
	}
	QList<QByteArray> list = proxyEnv.split(':');
	settings.hostName = list[0];
	bool ok;
	settings.port = list[1].toInt(&ok, 10);
	if (!ok) {
		return ProxiesSettings::ProxySettings();
	}

	settings.usage = usage;

	return settings;
}

ProxiesSettings::ProxySettings ProxiesSettings::proxySettings(
    enum ProxiesSettings::Type type) const
{
	const ProxiesSettings::ProxySettings *proxy = 0;

	switch (type) {
	case HTTP:
		proxy = &http;
		break;
	case HTTPS:
		proxy = &https;
		break;
	default:
		Q_ASSERT(0);
		return ProxiesSettings::ProxySettings();
		break;
	}
	Q_ASSERT(proxy != 0);

	ProxiesSettings::ProxySettings returnedVal;

	switch (proxy->usage) {
	case ProxySettings::NO_PROXY:
		returnedVal.usage = ProxySettings::NO_PROXY;
		break;
	case ProxySettings::AUTO_PROXY:
		returnedVal = fromEnvVal(detectEnvironment(type),
		    ProxySettings::AUTO_PROXY);
		break;
	case ProxySettings::DEFINED_PROXY:
		returnedVal = *proxy;
		break;
	default:
		Q_ASSERT(0);
		return ProxiesSettings::ProxySettings();
		break;
	}

	return returnedVal;
}

ProxiesSettings::ProxySettings ProxiesSettings::detectHttpProxy(void)
{
	ProxySettings settings;

	settings.usage = ProxySettings::NO_PROXY;

	/* TODO -- Try to contact the proxy to check whether it works? */

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	QByteArray envVar = qgetenv(HTTP_PROXY_VARMAME);
	QUrl proxyUrl(QString::fromLatin1(envVar));

	if (proxyUrl.isValid()) {
		logInfo("Detected proxy URL '%s', '%s' '%s'.\n",
		    proxyUrl.toString().toUtf8().constData(),
		    proxyUrl.userName().toUtf8().constData(),
		    proxyUrl.password().toUtf8().constData());
		settings.usage = ProxySettings::AUTO_PROXY;
		settings.userName = proxyUrl.userName();
		settings.password = proxyUrl.password();
		settings.hostName = proxyUrl.host();
		settings.port = proxyUrl.port();
		return settings;
	}
#else
	QNetworkProxyQuery npq(QUrl(QLatin1String("http://www.nic.cz")));

	QList<QNetworkProxy> listOfProxies =
	   QNetworkProxyFactory::systemProxyForQuery(npq);

	if (1 < listOfProxies.size()) {
		logWarning("%s/n", "Multiple proxies detected. Using first.");
	}

	foreach (QNetworkProxy p, listOfProxies) {
		if (!p.hostName().isEmpty()) {
			settings.usage = ProxySettings::AUTO_PROXY;
			settings.userName = p.user();
			settings.password = p.password();
			settings.hostName = p.hostName();
			settings.port = p.port();
			return settings;
		}
	}
#endif /* Q_OS_UNIX && !Q_OS_MAC */

	return settings;
}
