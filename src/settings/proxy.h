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
	 * if they are set. Therefore, this application should automatically
	 * default to proxy usage if those variables are set.
	 */

	/*!
	 * @brief Sets environmental variables according to proxy settings.
	 *
	 * @return False on any error.
	 */
	static
	bool setProxyEnvVars(void);

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
