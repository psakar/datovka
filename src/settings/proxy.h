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

#ifndef _PROXY_H_
#define _PROXY_H_

#include <QSettings>
#include <QString>

class ProxiesSettings {
public:
	/*!
	 * @brief Connection type.
	 */
	enum Type {
		HTTP,
		HTTPS
	};

	static
	const QByteArray httpProxyEnvVar; /*!< http_proxy environment variable at application start-up. */
	static
	const QByteArray httpsProxyEnvVar; /*!< https_proxy environment variable at application start-up. */

	class ProxySettings {
	public:
		/*!
		 * @brief Proxy usage.
		 */
		enum Usage {
			NO_PROXY, /*!< Disable proxy. */
			AUTO_PROXY, /*!< Automatic proxy detection. */
			DEFINED_PROXY /*!< Use defined values. */
		};

		/*!
		 * @brief Constructor.
		 */
		ProxySettings(void);

		enum Usage usage; /*!< How to interpret the proxy data.  */

		QString userName; /*!< Proxy user name. */
		QString password; /*!< Proxy password. */

		QString hostName; /*!< Proxy host. */
		int port; /*!< Proxy port. */

		/*!
		 * @brief Converts value to value as would be stored in
		 *     environment variable value.
		 *
		 * @return Values as they would be stored in a http(s)_proxy
		 *     variable without protocol specification (e.g.
		 *     'b:bb@192.168.1.1:3128', '172.0.0.1:3128') if contains
		 *     DEFINED_PROXY data, no data else.
		 */
		QByteArray toEnvVal(void) const;
	};

	/*!
	 * @brief Constructor.
	 */
	ProxiesSettings(void);

	ProxySettings https; /*!< HTTPS proxy settings. */
	ProxySettings http; /*!< HTTP proxy settings. */

	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] settings Settings structure.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 *
	 * @param[out] settings Settings structure.
	 */
	void saveToSettings(QSettings &settings) const;

	/*
	 * Libcurl automatically uses the variables http_proxy and https_proxy
	 * if they are set. Therefore, if no proxy is forced from the
	 * configuration file, then these two environmental variables are going
	 * to be cleared inside this application.
	 */

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
	static
	QByteArray detectEnvironment(enum Type type);

	/*!
	 * @brief Sets environmental variables according to proxy settings.
	 *
	 * @return False on any error.
	 */
	bool setProxyEnvVars(void) const;

	/*!
	 * @brief Detect HTTP proxy.
	 *
	 * @return Host and port number.
	 */
	static
	ProxiesSettings::ProxySettings detectHttpProxy(void);
};

/*!
 * @brief Global instance of the structure.
 */
extern ProxiesSettings globProxSet;

#endif /* _PROXY_H_ */
